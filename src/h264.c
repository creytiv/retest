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


int test_h264_sps(void)
{
	static const struct test {
		const char *buf;
		struct h264_sps sps;
		struct vidsz size;
	} testv[] = {

		/* rv
		 *
		 * sps:0 profile:66/52 poc:2 ref:1 120x68 FRM 8B8
		 * crop:0/0/0/8 VUI 420 1/360 b8 reo:0
		 */
		{
			.buf = "42c034da01e0089f961000000300",
			.sps = {
				66,52,0,1,
				4,2,1,120,68,
				0,0,0,8
			},
			.size = {1920, 1080}
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
				100,40,0,1,
				4,0,3,120,68,
				0,0,0,8
			},
			.size = {1920, 1080}
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
				100,31,0,1,
				4,0,4,80,45,
				0,0,0,0
			},
			.size = {1280, 720}
		},

		/* px
		 *
		 * sps: 42c01f95a014016c8400001f40000753023c2211a8
		 *
		 * sps:0 profile:66/31 poc:2 ref:1 80x45 FRM
		 *       crop:0/0/0/0 VUI 420 2000/120000 b8 reo:0
		 */
		{
			.buf =
			"42c01f95a014016c8400001f40000753023c2211a8",
			.sps = {
				66,31,0,1,
				8,2,1,80,45,
				0,0,0,0
			},
			.size = {1280, 720}
		},

		/* allonsy 854x480
		 *
		 * 4d401ee8806c1ef37808800000030080000019078b1689
		 *
		 * sps:0 profile:77/30 poc:0 ref:3 54x30 FRM 8B8 crop:0/10/0/0
		 * VUI 420 1/50 b8 reo:1
		 *
		 */
		{
			.buf =
			"4d401ee8806c1ef37808800000030080000019078b1689",
			.sps = {
				77,30,0,1,
				4,0,3,54,30,
				0,10,0,0
			},
			.size = {854, 480}
		},

		/* sony 1920x1080
		 *
		 * 7a0028b6cd940780227e27011000000300100000030320f1831960
		 *
		 * sps:0 profile:122/40 poc:0 ref:4 120x68 FRM 8B8
		 *       crop:0/0/0/8 VUI 422 1/50 b10 reo:2
		 *
		 */
		{
			.buf =
			"7a0028b6cd940780227e2701100"
			"0000300100000030320f1831960",
			.sps = {
				122,40,0,2,
				4,0,4,120,68,
				0,0,0,8
			},
			.size = {1920, 1080}
		},

		/* testsrc2 yuv444 400x200
		 *
		 * f4000d919b283237f13808800000030080000019078a14cb
		 *
		 * sps:0 profile:244/13 poc:0 ref:4 25x13 FRM 8B8
		 *       crop:0/0/0/8 VUI 444 1/50 b8 reo:2
		 *
		 */
		{
			.buf =
			"f4000d919b283237f13808800000030080000019078a14cb",
			.sps = {
				244,13,0,3,
				4,0,4,25,13,
				0,0,0,8
			},
			.size = {400, 200}
		},
	};
	struct h264_sps sps;
	size_t i;
	int e, err;

	static const uint8_t dummy_sps[] = {
		0x64, 0x00, 0x1f, 0xac, 0xd9, 0x40, 0x50, 0x05
	};

	for (i=0; i<ARRAY_SIZE(testv); i++) {

		const struct test *test = &testv[i];
		struct h264_sps ref = test->sps;
		uint8_t buf[256];
		size_t len = str_len(test->buf)/2;
		struct vidsz size;

		err = str_hex(buf, len, test->buf);
		if (err)
			return err;

		err = h264_sps_decode(&sps, buf, len);
		if (err)
			return err;

		h264_sps_resolution(&sps, &size);

		re_printf("sps test %zu: %u x %u (%s)\n",
			  i,
			  size.w, size.h,
			  h264_sps_chroma_format_name(sps.chroma_format_idc));

#if 0
		h264_sps_print(&sps);
#endif

		TEST_EQUALS(ref.profile_idc, sps.profile_idc);

		TEST_EQUALS(ref.level_idc, sps.level_idc);

		TEST_EQUALS(ref.seq_parameter_set_id,
			    sps.seq_parameter_set_id);

		TEST_EQUALS(ref.chroma_format_idc,
			    sps.chroma_format_idc);

		TEST_EQUALS(ref.log2_max_frame_num,
			    sps.log2_max_frame_num);

		TEST_EQUALS(ref.pic_order_cnt_type,
			    sps.pic_order_cnt_type);

		TEST_EQUALS(ref.max_num_ref_frames,
			    sps.max_num_ref_frames);

		TEST_EQUALS(ref.pic_width_in_mbs,
			    sps.pic_width_in_mbs);

		TEST_EQUALS(ref.pic_height_in_map_units,
			    sps.pic_height_in_map_units);

		TEST_EQUALS(ref.frame_crop_left_offset,
			    sps.frame_crop_left_offset);
		TEST_EQUALS(ref.frame_crop_right_offset,
			    sps.frame_crop_right_offset);
		TEST_EQUALS(ref.frame_crop_top_offset,
			    sps.frame_crop_top_offset);
		TEST_EQUALS(ref.frame_crop_bottom_offset,
			    sps.frame_crop_bottom_offset);

		/* verify correct resolution */
		TEST_EQUALS(test->size.w, size.w);
		TEST_EQUALS(test->size.h, size.h);
	}

	re_printf("-- Test short read:\n");


	for (i=1; i <= sizeof(dummy_sps); i++) {

		size_t len = i;

		re_printf("short read: %zu bytes\n", len);

		e = h264_sps_decode(&sps, dummy_sps, len);
		TEST_EQUALS(EBADMSG, e);
	}

 out:
	return err;
}
