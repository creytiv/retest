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
