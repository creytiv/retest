/**
 * @file h264.c H.264 Testcode
 *
 * Copyright (C) 2010 Creytiv.com
 */

#include <inttypes.h>
#include <limits.h>
#include <string.h>
#include <assert.h>
#include <re.h>
#include <rem.h>
#include "test.h"


#define DEBUG_MODULE "h264test"
#define DEBUG_LEVEL 5
#include <re_dbg.h>


int test_h264(void)
{
	struct h264_nal_header hdr, hdr2;
	struct mbuf *mb;
	static const uint8_t nal = 0x25;
	int err;

	mb = mbuf_alloc(1);
	if (!mb)
		return ENOMEM;

	hdr.f = 0;
	hdr.nri = 1;
	hdr.type = H264_NALU_IDR_SLICE;

	err = h264_nal_header_encode(mb, &hdr);
	if (err)
		goto out;

	TEST_EQUALS(1, mb->pos);
	TEST_EQUALS(1, mb->end);
	TEST_EQUALS(nal, mb->buf[0]);

	mb->pos = 0;

	err = h264_nal_header_decode(&hdr2, mb);
	if (err)
		goto out;

	TEST_EQUALS(1, mb->pos);
	TEST_EQUALS(1, mb->end);

	TEST_EQUALS(0, hdr2.f);
	TEST_EQUALS(1, hdr2.nri);
	TEST_EQUALS(5, hdr2.type);

 out:
	mem_deref(mb);
	return err;
}


#define MAX_SPS_COUNT           32
#define MAX_LOG2_MAX_FRAME_NUM  12


/**
 * H.264 Sequence Parameter Set (SPS)
 */
struct h264_sps {
	uint8_t profile_idc;
	uint8_t level_idc;
	unsigned seq_parameter_set_id;

	unsigned log2_max_frame_num;
	unsigned pic_order_cnt_type;

	/* pic_order_cnt_type = 0 */
	unsigned log2_max_pic_order_cnt_lsb;

	unsigned max_num_ref_frames;
	unsigned gaps_in_frame_num_value_allowed_flag;
	unsigned pic_width_in_mbs;
	unsigned pic_height_in_map_units;
};


struct getbitcontext {
	const uint8_t *buffer, *buffer_end;
	size_t pos;
	size_t size;
};


static void getbit_init(struct getbitcontext *s, const uint8_t *buffer,
			size_t bit_size)
{
	size_t buffer_size;

	buffer_size = (bit_size + 7) >> 3;

	s->buffer             = buffer;
	s->buffer_end         = buffer + buffer_size;
	s->pos                = 0;
	s->size               = bit_size;
}


static size_t getbit_get_left(const struct getbitcontext *gb)
{
	return gb->size - gb->pos;
}


static unsigned get_bit(struct getbitcontext *gb)
{
	const uint8_t *p = gb->buffer;
	unsigned tmp;

	assert(gb->pos < gb->size);

	tmp = ((*(p + (gb->pos >> 0x3))) >> (0x7 - (gb->pos & 0x7))) & 0x1;

	++gb->pos;

	return tmp;
}


static unsigned get_bits(struct getbitcontext *gb, uint8_t bits)
{
	unsigned value = 0;
	uint8_t i;

	for (i = 0; i < bits; i++) {
		value = (value << 1) | (get_bit(gb) ? 1 : 0);
	}

	return value;
}


static int get_ue_golomb(struct getbitcontext *gb, unsigned *valp)
{
	uint32_t zeros = 0;
	unsigned info;

	while (1) {

		if (getbit_get_left(gb) < 1)
			return ENODATA;

		if (0 == get_bit(gb))
			++zeros;
		else
			break;
	}

	info = 1 << zeros;

	for (int32_t i = zeros - 1; i >= 0; i--) {

		if (getbit_get_left(gb) < 1)
			return ENODATA;

		info |= get_bit(gb) << i;
	}

	if (valp)
		*valp = info - 1;

	return 0;
}


static int h264_sps_decode(struct h264_sps *sps, const uint8_t *p, size_t len)
{
	struct getbitcontext gb;
	uint8_t profile_idc;
	unsigned seq_parameter_set_id;
	unsigned log2_max_frame_num_minus4;
	int err;

	if (!sps || !p || !len)
		return EINVAL;

	getbit_init(&gb, p, len*8);

	if (getbit_get_left(&gb) < 24)
		return ENODATA;

	profile_idc = get_bits(&gb, 8);
	(void)get_bits(&gb, 8);
	sps->level_idc = get_bits(&gb, 8);

	err = get_ue_golomb(&gb, &seq_parameter_set_id);
	if (err)
		return err;

	if (seq_parameter_set_id >= MAX_SPS_COUNT) {
		re_printf("sps_id %u out of range\n", seq_parameter_set_id);
		return EBADMSG;
	}

	if (profile_idc == 100 ||
	    profile_idc == 110 ||
	    profile_idc == 122 ||
	    profile_idc == 244 ||
	    profile_idc ==  44 ||
	    profile_idc ==  83 ||
	    profile_idc ==  86 ||
	    profile_idc == 118 ||
	    profile_idc == 128 ||
	    profile_idc == 138 ||
	    profile_idc == 144) {

		unsigned seq_scaling_matrix_present_flag;
		unsigned chroma_format_idc;

		err = get_ue_golomb(&gb, &chroma_format_idc);
		if (err)
			return err;
		if (chroma_format_idc == 3)
			return ENOTSUP;

		/* bit_depth_luma/chroma */
		err  = get_ue_golomb(&gb, NULL);
		err |= get_ue_golomb(&gb, NULL);
		if (err)
			return err;

		/* qpprime_y_zero_transform_bypass_flag */
		get_bits(&gb, 1);

		seq_scaling_matrix_present_flag = get_bits(&gb, 1);
		if (seq_scaling_matrix_present_flag)
			return ENOTSUP;
	}

	err = get_ue_golomb(&gb, &log2_max_frame_num_minus4);
	if (err)
		return err;
	if (log2_max_frame_num_minus4 > MAX_LOG2_MAX_FRAME_NUM) {
		re_printf("sps: log2_max_frame_num_minus4"
			  " out of range (0-12): %d\n",
			  log2_max_frame_num_minus4);
		return EBADMSG;
	}

	err = get_ue_golomb(&gb, &sps->pic_order_cnt_type);
	if (err)
		return err;

	if (sps->pic_order_cnt_type == 0) {

		unsigned t;

		err = get_ue_golomb(&gb, &t);
		if (err)
			return err;
		if (t > 12) {
			re_printf("sps: log2_max_poc_lsb (%d)"
				  " is out of range\n", t);
			return ENOTSUP;
		}

		sps->log2_max_pic_order_cnt_lsb = t + 4;
	}
	else if (sps->pic_order_cnt_type == 2) {
	}
	else {
		re_printf("sps: WARNING: unknown pic_order_cnt_type (%u)\n",
			  sps->pic_order_cnt_type);
		return ENOTSUP;
	}

	err = get_ue_golomb(&gb, &sps->max_num_ref_frames);
	if (err)
		return err;
	sps->gaps_in_frame_num_value_allowed_flag = get_bits(&gb, 1);

	err  = get_ue_golomb(&gb, &sps->pic_width_in_mbs);
	err |= get_ue_golomb(&gb, &sps->pic_height_in_map_units);
	if (err)
		return err;

	++sps->pic_width_in_mbs;
	++sps->pic_height_in_map_units;

	if (sps->pic_height_in_map_units >= INT_MAX / 2U) {
		re_printf("sps: height overflow\n");
		return EBADMSG;
	}

	/* success */
	sps->profile_idc = profile_idc;
	sps->seq_parameter_set_id = seq_parameter_set_id;
	sps->log2_max_frame_num = log2_max_frame_num_minus4 + 4;

	re_printf("sps: done. read %zu bits, %zu bits left\n",
		  gb.pos, getbit_get_left(&gb));

	return 0;
}


static void h264_sps_print(const struct h264_sps *sps)
{
	re_printf("--- SPS ---\n");
	re_printf("profile_idc          %u\n", sps->profile_idc);
	re_printf("level_idc            %u\n", sps->level_idc);
	re_printf("seq_parameter_set_id %u\n", sps->seq_parameter_set_id);
	re_printf("\n");

	re_printf("log2_max_frame_num         %u\n",
		  sps->log2_max_frame_num);
	re_printf("pic_order_cnt_type         %u\n",
		  sps->pic_order_cnt_type);
	re_printf("log2_max_pic_order_cnt_lsb %u\n",
		  sps->log2_max_pic_order_cnt_lsb);
	re_printf("\n");

	re_printf("max_num_ref_frames                   %u\n",
		  sps->max_num_ref_frames);
	re_printf("gaps_in_frame_num_value_allowed_flag %u\n",
		  sps->gaps_in_frame_num_value_allowed_flag);
	re_printf("pic_width_in_mbs                     %u\n",
		  sps->pic_width_in_mbs);
	re_printf("pic_height_in_map_units              %u\n",
		  sps->pic_height_in_map_units);
	re_printf("\n");
}


int test_h264_sps(void)
{
	static const struct test {
		const char *buf;
		struct h264_sps sps;

	} testv[] = {

		{
			.buf = "42001eab40b04b4d4040418080",
			.sps = {
				 66,30,0,
				 5,0,6,1,0,22,18
			 }
		},

		/* rv
		 * sps:0 profile:66/52 poc:2 ref:1 120x68 FRM 8B8
		 * crop:0/0/0/8 VUI 420 1/360 b8 reo:0
		 */
		{
			.buf = "42c034da01e0089f961000000300",
			.sps = {
				 66,52,0,
				 4,2,6,1,0,120,68
			 }
		},

		/* confcall
		 *
		 * .... sps: 67640028acd100780227e5c05a8080
		 *           80a0000003002000000781e3062240
		 *
		 * sps:0 profile:100/40 poc:0 ref:3 120x68 FRM
		 * 8B8 crop:0/0/0/8 VUI 420 1/60 b8 reo:1
		 */

		{
			.buf =
			"640028acd100780227e5c05a808080"
			"a0000003002000000781e3062240",
			.sps = {
				 100,40,0,
				 4,0,5,3,0,120,68
			 }
		},

		/* expert
		 *
		 * sps: 64001facd9405005bb011000000300100000030320f1831960
		 *
		 * sps:0 profile:100/31 poc:0 ref:4 80x45 FRM
		 */
		{
			.buf =
			"64001facd9405005bb011000000300100000030320f1831960",
			.sps = {
				 100,31,0,
				 4,0,6,4,0,80,45
			 }
		},

	};
	struct h264_sps sps;
	size_t i;
	int err;

	for (i=0; i<ARRAY_SIZE(testv); i++) {

		const struct test *test = &testv[i];
		struct h264_sps ref = test->sps;
		uint8_t buf[256];
		size_t len = str_len(test->buf)/2;

		err = str_hex(buf, len, test->buf);
		if (err)
			return err;

		err = h264_sps_decode(&sps, buf, len);
		if (err)
			return err;

		TEST_EQUALS(ref.profile_idc, sps.profile_idc);

		TEST_EQUALS(ref.level_idc, sps.level_idc);

		TEST_EQUALS(ref.seq_parameter_set_id,
			    sps.seq_parameter_set_id);

		TEST_EQUALS(ref.log2_max_frame_num,
			    sps.log2_max_frame_num);

		TEST_EQUALS(ref.pic_order_cnt_type,
			    sps.pic_order_cnt_type);

		TEST_EQUALS(ref.log2_max_pic_order_cnt_lsb,
			    sps.log2_max_pic_order_cnt_lsb);

		TEST_EQUALS(ref.max_num_ref_frames,
			    sps.max_num_ref_frames);

		TEST_EQUALS(ref.gaps_in_frame_num_value_allowed_flag,
			    sps.gaps_in_frame_num_value_allowed_flag);

		TEST_EQUALS(ref.pic_width_in_mbs,
			    sps.pic_width_in_mbs);

		TEST_EQUALS(ref.pic_height_in_map_units,
			    sps.pic_height_in_map_units);

		h264_sps_print(&sps);
	}

	re_printf("-- Test short read:\n");

	static const uint8_t dummy[] = {
		0x64, 0x00, 0x1f, 0xac, 0xd9, 0x40, 0x50, 0x05
	};

	int e;

	for (i=1; i <= sizeof(dummy); i++) {

		size_t len = i;

		re_printf("short read: %zu bytes\n", len);

		e = h264_sps_decode(&sps, dummy, len);
		TEST_EQUALS(ENODATA, e);
	}

 out:
	return err;
}
