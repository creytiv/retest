/**
 * @file vid.c Video Testcode
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include <string.h>
#include <re.h>
#include <rem.h>
#include "test.h"


#define DEBUG_MODULE "vid"
#define DEBUG_LEVEL 5
#include <re_dbg.h>


static const enum vidfmt fmtv[VID_FMT_N] = {
	VID_FMT_YUV420P,
	VID_FMT_YUYV422,
	VID_FMT_UYVY422,
	VID_FMT_RGB32,
	/* todo VID_FMT_ARGB, */
	VID_FMT_RGB565,
	VID_FMT_RGB555,
	VID_FMT_NV12,
	VID_FMT_NV21,
};


static int test_vidsz_cmp(void)
{
	struct vidsz a = {64, 64}, b = {64, 64}, c = {32, 32};

	TEST_ASSERT(!vidsz_cmp(NULL, NULL));
	TEST_ASSERT( vidsz_cmp(&a, &a));
	TEST_ASSERT( vidsz_cmp(&a, &b));
	TEST_ASSERT(!vidsz_cmp(&a, &c));

	return 0;
}


static int test_vidrect_cmp(void)
{
	struct vidrect a = {10, 10, 30, 30};
	struct vidrect b = {10, 10, 30, 30};
	struct vidrect c = {10, 10, 40, 40};
	struct vidrect d = {20, 20, 30, 30};

	TEST_ASSERT(!vidrect_cmp(NULL, NULL));
	TEST_ASSERT( vidrect_cmp(&a, &a));
	TEST_ASSERT( vidrect_cmp(&a, &b));
	TEST_ASSERT(!vidrect_cmp(&a, &c));
	TEST_ASSERT(!vidrect_cmp(&a, &d));

	return 0;
}


static int test_vidframe_size(void)
{
	static const struct vidsz vidsz = {32, 32};
	size_t i;

	for (i=0; i<ARRAY_SIZE(fmtv); i++) {

		size_t sz = vidframe_size(fmtv[i], &vidsz);

		if (sz == 0) {
			DEBUG_WARNING("unexpected zero size for format %s\n",
				      vidfmt_name(fmtv[i]));
			return EINVAL;
		}
	}

	return 0;
}


static int test_vidframe_alloc(void)
{
	static const struct vidsz vidsz = {32, 32};
	struct vidframe *vf = NULL;
	size_t i;
	int err = ENOENT;

	for (i=0; i<ARRAY_SIZE(fmtv); i++) {

		err = vidframe_alloc(&vf, fmtv[i], &vidsz);
		if (err)
			break;

		TEST_ASSERT(vidframe_isvalid(vf));

		TEST_NOT_EQUALS(0, vf->linesize[0]);
		TEST_ASSERT(vidsz_cmp(&vidsz, &vf->size));
		TEST_EQUALS(fmtv[i], vf->fmt);

		vf = mem_deref(vf);
	}

	return err;
}


int test_vid(void)
{
	int err;

	err = test_vidsz_cmp();
	if (err)
		return err;

	err = test_vidrect_cmp();
	if (err)
		return err;

	err = test_vidframe_size();
	if (err)
		return err;

	err = test_vidframe_alloc();
	if (err)
		return err;

	return err;
}
