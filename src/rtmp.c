/**
 * @file rtmp.c RTMP Testcode
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include <string.h>
#include <re.h>
#include "test.h"


#define DEBUG_MODULE "rtmp"
#define DEBUG_LEVEL 5
#include <re_dbg.h>


static int test_rtmp_header(void)
{
	struct rtmp_header hdr;
	struct mbuf *mb;
	const uint8_t chunk_stream_id = 3;
	const uint32_t timestamp = 123;
	const uint32_t msg_length = 194;
	const uint8_t msg_type_id = 20;
	const uint32_t msg_stream_id = 1234567;
	int err;

	mb = mbuf_alloc(512);

	err = rtmp_header_encode(mb, chunk_stream_id,
				 timestamp, msg_length,
				 msg_type_id, msg_stream_id);
	TEST_ERR(err);

	mb->pos = 0;

	err = rtmp_header_decode(&hdr, mb);
	TEST_ERR(err);

	/* compare */
	TEST_EQUALS(0,               hdr.format);
	TEST_EQUALS(chunk_stream_id, hdr.chunk_stream_id);
	TEST_EQUALS(timestamp,       hdr.timestamp);
	TEST_EQUALS(msg_length,      hdr.message_length);
	TEST_EQUALS(msg_type_id,     hdr.message_type_id);
	TEST_EQUALS(msg_stream_id,   hdr.message_stream_id);

 out:
	mem_deref(mb);

	return err;
}


int test_rtmp(void)
{
	int err;

	err = test_rtmp_header();
	if (err)
		return err;

	return 0;
}
