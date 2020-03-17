/**
 * @file h264.c H.264 Testcode
 *
 * Copyright (C) 2010 Creytiv.com
 */

#include <string.h>
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


#define MAX_SPS_COUNT          32
#define MAX_LOG2_MAX_FRAME_NUM    (12)


/**
 * H.264 Sequence Parameter Set (SPS)
 */
struct sps {
	uint8_t profile_idc;
	/* constraint_set0_flag */
	/* reserved_zero_2bits */
	uint8_t level_idc;
	unsigned seq_parameter_set_id;

	unsigned log2_max_frame_num;
	unsigned pic_order_cnt_type;

	/* pic_order_cnt_type=0 */
	unsigned log2_max_pic_order_cnt_lsb;

	unsigned max_num_ref_frames;
	unsigned gaps_in_frame_num_value_allowed_flag;
	unsigned pic_width_in_mbs;
	unsigned pic_height_in_map_units;
};


static unsigned get_bit(const uint8_t *p, unsigned offset)
{
	return ((*(p + (offset >> 0x3))) >> (0x7 - (offset & 0x7))) & 0x1;
}


static unsigned get_bits(const uint8_t *p,
			 unsigned *offset, uint8_t bits)
{
	unsigned value = 0;

	for (int i = 0; i < bits; i++) {
		value = (value << 1) | (get_bit(p, (*offset)++) ? 1 : 0);
	}

	return value;
}


static uint32_t get_ue_golomb(const uint8_t *p, uint32_t *offset)
{
	uint32_t zeros = 0;

	while (0 == get_bit(p, (*offset)++))
		++zeros;

	uint32_t info = 1 << zeros;

	for (int32_t i = zeros - 1; i >= 0; i--) {
		info |= get_bit(p, (*offset)++) << i;
	}

	return info - 1;
}


static int sps_decode(struct sps *sps, const uint8_t *p, size_t len)
{
	unsigned offset = 0;
	uint8_t profile_idc;
	unsigned seq_parameter_set_id;
	unsigned log2_max_frame_num_minus4;

	if (!sps || !p || len < 4)
		return EINVAL;

	profile_idc = get_bits(p, &offset, 8);
	get_bits(p, &offset, 8);
	sps->level_idc = get_bits(p, &offset, 8);
	seq_parameter_set_id = get_ue_golomb(p, &offset);

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

		return ENOTSUP;
	}

	log2_max_frame_num_minus4 = get_ue_golomb(p, &offset);
	if (log2_max_frame_num_minus4 > MAX_LOG2_MAX_FRAME_NUM) {
		re_printf("sps: log2_max_frame_num_minus4"
			  " out of range (0-12): %d\n",
			  log2_max_frame_num_minus4);
		return EBADMSG;
	}

	sps->pic_order_cnt_type = get_ue_golomb(p, &offset);

	if (sps->pic_order_cnt_type == 0) {

		unsigned t = get_ue_golomb(p, &offset);

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

	sps->max_num_ref_frames = get_ue_golomb(p, &offset);
	sps->gaps_in_frame_num_value_allowed_flag = get_bits(p, &offset, 1);

	sps->pic_width_in_mbs = get_ue_golomb(p, &offset) + 1;
	sps->pic_height_in_map_units = get_ue_golomb(p, &offset) + 1;

	if (sps->pic_height_in_map_units >= INT_MAX / 2U) {
		re_printf("sps: height overflow\n");
		return EBADMSG;
	}

	/* success */
	sps->profile_idc = profile_idc;
	sps->seq_parameter_set_id = seq_parameter_set_id;
	sps->log2_max_frame_num = log2_max_frame_num_minus4 + 4;

	re_printf("done. offset=%u bits\n", offset);

	if (offset > 8*len) {
		re_printf("sps: WARNING: read past end (%u > %u)\n",
			  offset, 8*len);
		return EBADMSG;
	}

	return 0;
}


static void sps_print(const struct sps *sps)
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
		uint8_t buf[14];
		struct sps sps;

	} testv[] = {

		{
			.buf = {0x42, 0x00, 0x0a, 0xf8, 0x41, 0xa2},
			.sps = {
				 66,10,0,4,0,4,0,0,8,6
			 }
		},

		{
			.buf = {0x42, 0x00, 0x1e, 0xab, 0x40, 0xb0, 0x4b,
				0x4d, 0x40, 0x40, 0x41, 0x80, 0x80},
			.sps = {
				 66,30,0,5,0,6,1,0,22,18
			 }
		},

		/* rv
		 * sps:0 profile:66/52 poc:2 ref:1 120x68 FRM 8B8
		 * crop:0/0/0/8 VUI 420 1/360 b8 reo:0
		 */
		{
			.buf = {0x42, 0xc0, 0x34, 0xda, 0x01, 0xe0, 0x08,
				0x9f, 0x96, 0x10, 0x00, 0x00, 0x03, 0x00},
			.sps = {
				 66,52,0,4,2,6,1,0,120,68
			 }
		},

	};
	size_t i;
	int err;

	for (i=0; i<ARRAY_SIZE(testv); i++) {

		const struct test *test = &testv[i];
		struct sps ref = test->sps;
		struct sps sps;

		err = sps_decode(&sps, test->buf, sizeof(test->buf));
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

		sps_print(&sps);
	}

 out:
	return err;
}
