/**
 * @file vidconv.c Video conversion Testcode
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include <string.h>
#include <re.h>
#include <rem.h>
#include "test.h"


#define DEBUG_MODULE "vidconv"
#define DEBUG_LEVEL 5
#include <re_dbg.h>


#define WIDTH 320
#define HEIGHT 240
#define SCALE 2


/*
 * http://en.wikipedia.org/wiki/YCbCr
 *
 * ITU-R BT.601 conversion
 *
 * Digital YCbCr can derived from digital R'dG'dB'd
 * (8 bits per sample, each using the full range with
 * zero representing black and 255 representing white)
 * according to the following equations:
 */


static double rgb2y_ref(int r, int g, int b)
{
	return 16.0 + (65.738*r)/256 + (129.057*g)/256 + (25.064*b)/256;
}

static double rgb2u_ref(int r, int g, int b)
{
	return 128 - 37.945*r/256 - 74.494*g/256 + 112.439*b/256;
}

static double rgb2v_ref(int r, int g, int b)
{
	return 128 + 112.439*r/256 - 94.154*g/256 - 18.285*b/256;
}


static int testx(const char *name, int rmin, int rmax, double x, double xref)
{
	double diff = x - xref;
	int err = 0;

	/* the difference should be within the range (-1.0, 1.0) */
	if (diff > 1.0 || diff < -1.0) {
		DEBUG_WARNING("component '%s' -- diff is too large: %f\n",
			      name, diff);
		return EBADMSG;
	}
	TEST_ASSERT(x >= rmin && x <= rmax);

 out:
	return err;
}


static int test_vid_rgb2yuv_color(int r, int g, int b)
{
	int err = 0;

	err |= testx("Y", 16, 235, rgb2y(r, g, b), rgb2y_ref(r, g, b));
	err |= testx("U", 16, 240, rgb2u(r, g, b), rgb2u_ref(r, g, b));
	err |= testx("V", 16, 240, rgb2v(r, g, b), rgb2v_ref(r, g, b));

	return err;
}


static int test_vid_rgb2yuv(void)
{
	int r, g, b;
	int err = 0;

	/* full range of 1 color component only */
	for (r=0; r<256; r++)
		err |= test_vid_rgb2yuv_color(r, 0, 0);
	for (g=0; g<256; g++)
		err |= test_vid_rgb2yuv_color(0, g, 0);
	for (b=0; b<256; b++)
		err |= test_vid_rgb2yuv_color(0, 0, b);
	if (err)
		return err;

	/* combine 2 color components */
	for (r=0; r<256; r++)
		for (g=0; g<256; g++)
			err |= test_vid_rgb2yuv_color(r, g, 0);

	for (r=0; r<256; r++)
		for (b=0; b<256; b++)
			err |= test_vid_rgb2yuv_color(r, 0, b);

	for (g=0; g<256; g++)
		for (b=0; b<256; b++)
			err |= test_vid_rgb2yuv_color(0, g, b);

	return err;
}


static bool vidframe_cmp(const struct vidframe *a, const struct vidframe *b)
{
	int i;

	if (!a || !b)
		return false;

	if (a->fmt != b->fmt)
		return false;

	for (i=0; i<4; i++) {

		if (a->linesize[i] != b->linesize[i])
			return false;

		if (!a->data[i] != !b->data[i])
			return false;

		if (a->data[i] && b->data[i]) {

			if (memcmp(a->data[i], b->data[i], a->linesize[i]))
				return false;
		}
	}

	return vidsz_cmp(&a->size, &b->size);
}


static void vidframe_dump(const struct vidframe *f)
{
	int i;

	if (!f)
		return;

	(void)re_printf("vidframe %u x %u:\n", f->size.w, f->size.h);

	for (i=0; i<4; i++) {

		if (f->linesize[i] && f->data[i]) {

			(void)re_printf("%d: %u bytes [%w]\n",
					i, f->linesize[i], f->data[i],
					(size_t)min(f->linesize[i], 16));
		}
	}
}


/**
 * Test vidconv module by scaling a random image up and then down.
 * The two images should then be pixel accurate.
 */
static int test_vidconv_scaling(void)
{
	struct vidframe *f0 = NULL, *f1 = NULL, *f2 = NULL;
	const struct vidsz size0 = {WIDTH, HEIGHT};
	const struct vidsz size1 = {WIDTH*SCALE, HEIGHT*SCALE};
	struct vidrect rect1 = {0, 0, WIDTH*SCALE, HEIGHT*SCALE};
	struct vidrect rect2 = {0, 0, WIDTH, HEIGHT};
	int i, err = 0;

	err |= vidframe_alloc(&f0, VID_FMT_YUV420P, &size0);
	err |= vidframe_alloc(&f1, VID_FMT_YUV420P, &size1);
	err |= vidframe_alloc(&f2, VID_FMT_YUV420P, &size0);
	if (err)
		goto out;

	/* generate a random image */
	for (i=0; i<4; i++) {

		if (f0->data[i])
			rand_bytes(f0->data[i], f0->linesize[i]);
	}

	vidconv(f1, f0, &rect1);
	vidconv(f2, f1, &rect2);

	if (!vidframe_cmp(f2, f0)) {

		vidframe_dump(f0);
		vidframe_dump(f2);

		err = EBADMSG;
		goto out;
	}

 out:
	mem_deref(f2);
	mem_deref(f1);
	mem_deref(f0);

	return err;
}


/*
 * verify that pixel conversion between different planar and packed
 * pixel formats is working
 */
static int test_vidconv_pixel_formats(void)
{
	struct plane {
		size_t sz;
		const char *data;
	};
	static const struct test {
		enum vidfmt src_fmt;
		struct plane src_planev[3];
		enum vidfmt dst_fmt;
		struct plane dst_planev[3];
	} testv [] = {

		/* UYVY422 to YUV420P */
		{
			VID_FMT_UYVY422,
			{ {8, "\x22\x11\x33\x11\x22\x11\x33\x11"},
			  {0,0},
			  {0,0}
			},

			VID_FMT_YUV420P,
			{ {4, "\x11\x11\x11\x11"},
			  {1, "\x22"},
			  {1, "\x33"}
			},
		},

		/* NV12 to YUV420P */
		{
			VID_FMT_NV12,
			{ {4, "\x11\x11\x11\x11"},
			  {2, "\x22\x33"},
			  {0,0}
			},

			VID_FMT_YUV420P,
			{ {4, "\x11\x11\x11\x11"},
			  {1, "\x22"},
			  {1, "\x33"}
			},
		},

#if 0
		/* YUYV422 to NV12 */
		{
			VID_FMT_YUYV422,
			{ {8, "\x11\x22\x11\x33\x11\x22\x11\x33"},
			  {0,0},
			  {0,0}
			},

			VID_FMT_NV12,
			{ {4, "\x11\x11\x11\x11"},
			  {2, "\x22\x33"},
			  {0,0}
			},
		},
#endif
	};
	struct vidframe *fsrc = NULL, *fdst = NULL;
	const struct vidsz sz = {2, 2};
	unsigned i, p;
	int err = 0;

	for (i=0; i<ARRAY_SIZE(testv); i++) {

		const struct test *test = &testv[i];

#if 0
		re_printf("test[%u] %s to %s\n", i,
			  vidfmt_name(test->src_fmt),
			  vidfmt_name(test->dst_fmt));
#endif

		err |= vidframe_alloc(&fsrc, test->src_fmt, &sz);
		err |= vidframe_alloc(&fdst, test->dst_fmt, &sz);
		if (err)
			goto out;

		for (p=0; p<3; p++) {
			if (test->src_planev[p].sz) {
				memcpy(fsrc->data[p],
				       test->src_planev[p].data,
				       test->src_planev[p].sz);
			}
		}

		vidconv(fdst, fsrc, 0);

		for (p=0; p<3; p++) {

			TEST_MEMCMP(test->dst_planev[p].data,
				    test->dst_planev[p].sz,
				    fdst->data[p],
				    test->dst_planev[p].sz);
		}

		fdst = mem_deref(fdst);
		fsrc = mem_deref(fsrc);
	}

 out:
	mem_deref(fsrc);
	mem_deref(fdst);

	return err;
}


int test_vidconv(void)
{
	int err;

	err = test_vid_rgb2yuv();
	if (err)
		return err;

	err = test_vidconv_scaling();
	if (err)
		return err;

	err = test_vidconv_pixel_formats();
	if (err)
		return err;

	return err;
}
