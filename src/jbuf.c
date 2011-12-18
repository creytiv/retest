/**
 * @file jbuf.c Jitterbuffer Testcode
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include <string.h>
#include <re.h>
#include "test.h"


#define DEBUG_MODULE "testjbuf"
#define DEBUG_LEVEL 5
#include <re_dbg.h>


int test_jbuf(void)
{
	struct rtp_header hdr, hdr2;
	struct jbuf *jb;
	char *frv[3];
	uint32_t i;
	void *mem = NULL;
	int err;

	memset(frv, 0, sizeof(frv));

	err = jbuf_alloc(&jb, 0, 10);
	if (err)
		return err;

	for (i=0; i<ARRAY_SIZE(frv); i++) {
		frv[i] = mem_alloc(sizeof(frv[0]), NULL);
		if (!frv[i]) {
			err = ENOMEM;
			goto out;
		}
	}

	/* Empty list */
	DEBUG_INFO("test frame: Empty list\n");
	if (ENOENT != jbuf_get(jb, &hdr2, &mem)) {
		err = EINVAL;
		goto out;
	}


	/* One frame */
	DEBUG_INFO("test frame: One frame\n");
	hdr.seq = 160;
	err = jbuf_put(jb, &hdr, frv[0]);
	if (err)
		goto out;
	if ((EALREADY != jbuf_put(jb, &hdr, frv[0]))) {err = EINVAL; goto out;}

	err = jbuf_get(jb, &hdr2, &mem);
	if (err)
		goto out;
	if (160 != hdr2.seq) {err = EINVAL; goto out;}
	if (mem != frv[0]) {err = EINVAL; goto out;}
	mem = mem_deref(mem);

	if (ENOENT != jbuf_get(jb, &hdr2, &mem)) {err = EINVAL; goto out;}


	/* Two frames */
	DEBUG_INFO("test frame: Two frames\n");
	hdr.seq = 320;
	err = jbuf_put(jb, &hdr, frv[0]);
	if (err)
		goto out;
	hdr.seq = 480;
	err = jbuf_put(jb, &hdr, frv[1]);
	if (err)
		goto out;

	err = jbuf_get(jb, &hdr2, &mem);
	if (err)
		goto out;
	if (320 != hdr2.seq) {err = EINVAL; goto out;}
	if (mem != frv[0]) {err = EINVAL; goto out;}
	mem = mem_deref(mem);

	err = jbuf_get(jb, &hdr2, &mem);
	if (err)
		goto out;
	if (480 != hdr2.seq) {err = EINVAL; goto out;}
	if (mem != frv[1]) {err = EINVAL; goto out;}
	mem = mem_deref(mem);

	if (ENOENT != jbuf_get(jb, &hdr2, &mem)) {err = EINVAL; goto out;}


	/* Three frames */
	DEBUG_INFO("test frame: Three frames\n");
	hdr.seq = 640;
	err = jbuf_put(jb, &hdr, frv[0]);
	if (err)
		goto out;
	hdr.seq = 800;
	err = jbuf_put(jb, &hdr, frv[1]);
	if (err)
		goto out;
	hdr.seq = 960;
	err = jbuf_put(jb, &hdr, frv[2]);
	if (err)
		goto out;

	err = jbuf_get(jb, &hdr2, &mem);
	if (err)
		goto out;
	if (640 != hdr2.seq) {err = EINVAL; goto out;}
	if (mem != frv[0]) {err = EINVAL; goto out;}
	mem = mem_deref(mem);

	err = jbuf_get(jb, &hdr2, &mem);
	if (err)
		goto out;
	if (800 != hdr2.seq) {err = EINVAL; goto out;}
	if (mem != frv[1]) {err = EINVAL; goto out;}
	mem = mem_deref(mem);

	err = jbuf_get(jb, &hdr2, &mem);
	if (err)
		goto out;
	if (960 != hdr2.seq) {err = EINVAL; goto out;}
	if (mem != frv[2]) {err = EINVAL; goto out;}
	mem = mem_deref(mem);

	if (ENOENT != jbuf_get(jb, &hdr2, &mem)) {err = EINVAL; goto out;}


 out:
	mem_deref(jb);
	mem_deref(mem);
	for (i=0; i<ARRAY_SIZE(frv); i++)
		mem_deref(frv[i]);

	return err;
}
