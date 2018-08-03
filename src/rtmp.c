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


static struct mbuf *mbuf_packet(const uint8_t *pkt, size_t len)
{
	struct mbuf *mb;

	mb = mbuf_alloc(len);
	if (!mb)
		return NULL;

	(void)mbuf_write_mem(mb, pkt, len);

	mb->pos = 0;

	return mb;
}


static int test_rtmp_header(uint32_t chunk_id)
{
	struct rtmp_header hdr;
	struct mbuf *mb;
	const uint32_t timestamp = 160;
	const uint32_t msg_length = 194;
	const uint8_t msg_type_id = 20;
	const uint32_t msg_stream_id = 1234567;
	int err;

	mb = mbuf_alloc(512);

	err = rtmp_header_encode(mb, chunk_id,
				 timestamp, msg_length,
				 msg_type_id, msg_stream_id);
	TEST_ERR(err);

	mb->pos = 0;

	err = rtmp_header_decode(&hdr, mb);
	TEST_ERR(err);

#if 0
	re_printf("%H\n", rtmp_header_print, &hdr);
#endif

	/* compare */
	TEST_EQUALS(0,               hdr.format);
	TEST_EQUALS(chunk_id,        hdr.chunk_id);
	TEST_EQUALS(timestamp,       hdr.timestamp);
	TEST_EQUALS(msg_length,      hdr.message_length);
	TEST_EQUALS(msg_type_id,     hdr.message_type_id);
	TEST_EQUALS(msg_stream_id,   hdr.message_stream_id);

 out:
	mem_deref(mb);

	return err;
}


static int test_rtmp_decode_audio(void)
{
	/*
	 * ffplay rtmp://184.72.239.149/vod/mp4:bigbuckbunny_450.mp4
	 */
static const uint8_t packet_bytes[] = {
  0x46, 0x00, 0x00, 0x00, 0x00, 0x00, 0x52, 0x08,
  0xaf, 0x00, 0x11, 0x90, 0x08, 0xc4, 0x00, 0x00,
  0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00
};

#define HDR_SIZE 8

	struct rtmp_header hdr;
	struct mbuf *mb;
	int err;

	mb = mbuf_packet(packet_bytes, sizeof(packet_bytes));

	err = rtmp_header_decode(&hdr, mb);
	TEST_ERR(err);

	/* compare */
	TEST_EQUALS(1,               hdr.format);
	TEST_EQUALS(6,               hdr.chunk_id);
	TEST_EQUALS(0,               hdr.timestamp_delta);
	TEST_EQUALS(82,              hdr.message_length);
	TEST_EQUALS(RTMP_TYPE_AUDIO, hdr.message_type_id);

	TEST_MEMCMP(packet_bytes + HDR_SIZE, sizeof(packet_bytes) - HDR_SIZE,
		    mbuf_buf(mb), mbuf_get_left(mb));

	/* XXX: decode control header ? */

 out:
	mem_deref(mb);
	return err;
}


int test_rtmp(void)
{
	int err;

	err = test_rtmp_header(63);
	TEST_ERR(err);
	err = test_rtmp_header(319);
	TEST_ERR(err);
	err = test_rtmp_header(65599);
	TEST_ERR(err);

	err = test_rtmp_decode_audio();
	TEST_ERR(err);

 out:
	return err;
}
