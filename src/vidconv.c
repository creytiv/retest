/**
 * @file vidconv.c Video conversion Testcode
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include <string.h>
#include <re.h>
#include <rem.h>
#include "test.h"


#define WIDTH 320
#define HEIGHT 240
#define SCALE 2


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
int test_vidconv(void)
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
