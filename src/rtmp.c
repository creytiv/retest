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


static int test_rtmp_header_type0(void)
{
	struct rtmp_header hdr;
	struct mbuf *mb;
	const uint32_t chunk_id = 3;
	const uint32_t timestamp = 160;
	const uint32_t msg_length = 194;
	const uint8_t msg_type_id = 20;
	const uint32_t msg_stream_id = 1234567;
	int err;

	mb = mbuf_alloc(512);

	err = rtmp_header_encode_type0(mb, chunk_id,
				       timestamp, msg_length,
				       msg_type_id, msg_stream_id);
	TEST_ERR(err);
	TEST_EQUALS(1+11, mb->end);

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


static int test_rtmp_header_type1(void)
{
	struct rtmp_header hdr;
	struct mbuf *mb;
	const uint32_t chunk_id = 3;
	const uint32_t timestamp_delta = 160;
	const uint32_t msg_length = 194;
	const uint8_t msg_type_id = 20;
	int err;

	mb = mbuf_alloc(512);

	err = rtmp_header_encode_type1(mb, chunk_id,
				       timestamp_delta, msg_length,
				       msg_type_id);
	TEST_ERR(err);
	TEST_EQUALS(1+7, mb->end);

	mb->pos = 0;

	err = rtmp_header_decode(&hdr, mb);
	TEST_ERR(err);

	/* compare */
	TEST_EQUALS(1,               hdr.format);
	TEST_EQUALS(chunk_id,        hdr.chunk_id);
	TEST_EQUALS(timestamp_delta, hdr.timestamp_delta);
	TEST_EQUALS(msg_length,      hdr.message_length);
	TEST_EQUALS(msg_type_id,     hdr.message_type_id);

 out:
	mem_deref(mb);

	return err;
}


static int test_rtmp_header_type2(void)
{
	struct rtmp_header hdr;
	struct mbuf *mb;
	const uint32_t chunk_id = 3;
	const uint32_t timestamp_delta = 160;
	int err;

	mb = mbuf_alloc(512);

	err = rtmp_header_encode_type2(mb, chunk_id, timestamp_delta);
	TEST_ERR(err);
	TEST_EQUALS(1+3, mb->end);

	mb->pos = 0;

	err = rtmp_header_decode(&hdr, mb);
	TEST_ERR(err);

	/* compare */
	TEST_EQUALS(2,               hdr.format);
	TEST_EQUALS(chunk_id,        hdr.chunk_id);
	TEST_EQUALS(timestamp_delta, hdr.timestamp_delta);

 out:
	mem_deref(mb);

	return err;
}


static int test_rtmp_header_type3(void)
{
	struct rtmp_header hdr;
	struct mbuf *mb;
	const uint32_t chunk_id = 3;
	int err;

	mb = mbuf_alloc(512);

	err = rtmp_header_encode_type3(mb, chunk_id);
	TEST_ERR(err);
	TEST_EQUALS(1+0, mb->end);

	mb->pos = 0;

	err = rtmp_header_decode(&hdr, mb);
	TEST_ERR(err);

	/* compare */
	TEST_EQUALS(3,               hdr.format);
	TEST_EQUALS(chunk_id,        hdr.chunk_id);

 out:
	mem_deref(mb);

	return err;
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

	err = rtmp_header_encode_type0(mb, chunk_id,
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


struct test {
	struct rtmp_dechunker *rd;
	size_t n_chunk;
	size_t total_bytes;

	size_t n_msg;
	uint8_t *buf;
	size_t buf_len;
};


static void msg_handler(const uint8_t *msg, size_t len, void *arg)
{
	struct test *test = arg;

	++test->n_msg;

	mem_deref(test->buf);

	test->buf = mem_ref((void *)msg);
	test->buf_len = len;
}


static void chunk_handler(const uint8_t *hdr, size_t hdr_len,
			  const uint8_t *pld, size_t pld_len, void *arg)
{
	struct test *test = arg;
	struct mbuf *mb = mbuf_alloc(1024);
	int err = 0;

	re_printf("chunk: hdr=%zu pld=%zu [%w]\n", hdr_len, pld_len,
		  pld, pld_len);

	++test->n_chunk;

	test->total_bytes += (hdr_len + pld_len);

	err |= mbuf_write_mem(mb, hdr, hdr_len);
	err |= mbuf_write_mem(mb, pld, pld_len);

	mb->pos = 0;

	/* feed the chunks into the de-chunker */
	err = rtmp_dechunker_receive(test->rd, mb);

	mem_deref(mb);
}


static int test_rtmp_chunking(void)
{
	static const uint8_t short_payload[] = {
		0xba,
	};
	static const uint8_t amf_payload[] = {
		0x02, 0x00, 0x07, 0x63,
		0x6f, 0x6e, 0x6e, 0x65, 0x63, 0x74, 0x00, 0x3f,
		0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03,
		0x00, 0x03, 0x61, 0x70, 0x70, 0x02, 0x00, 0x03,
		0x76, 0x6f, 0x64, 0x00, 0x08, 0x66, 0x6c, 0x61,
		0x73, 0x68, 0x56, 0x65, 0x72, 0x02, 0x00, 0x0d,
		0x4c, 0x4e, 0x58, 0x20, 0x39, 0x2c, 0x30, 0x2c,
		0x31, 0x32, 0x34, 0x2c, 0x32, 0x00, 0x05, 0x74,
		0x63, 0x55, 0x72, 0x6c, 0x02, 0x00, 0x1e, 0x72,
		0x74, 0x6d, 0x70, 0x3a, 0x2f, 0x2f, 0x31, 0x38,
		0x34, 0x2e, 0x37, 0x32, 0x2e, 0x32, 0x33, 0x39,
		0x2e, 0x31, 0x34, 0x39, 0x3a, 0x31, 0x39, 0x33,
		0x35, 0x2f, 0x76, 0x6f, 0x64, 0x00, 0x04, 0x66,
		0x70, 0x61, 0x64, 0x01, 0x00, 0x00, 0x0c, 0x63,
		0x61, 0x70, 0x61, 0x62, 0x69, 0x6c, 0x69, 0x74,
		0x69, 0x65, 0x73, 0x00, 0x40, 0x2e, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x0b, 0x61, 0x75,
		0x64, 0x69, 0x6f, 0x43, 0x6f, 0x64, 0x65, 0x63,
		0x73, 0x00, 0x40, 0xaf, 0xce, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x0b, 0x76, 0x69, 0x64, 0x65,
		0x6f, 0x43, 0x6f, 0x64, 0x65, 0x63, 0x73, 0x00,
		0x40, 0x6f, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x0d, 0x76, 0x69, 0x64, 0x65, 0x6f, 0x46,
		0x75, 0x6e, 0x63, 0x74, 0x69, 0x6f, 0x6e, 0x00,
		0x3f, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x09
	};
	uint8_t large_payload[1024] = {0x00};
	struct test test;
	struct rtmp_dechunker *rd = NULL;
	const uint32_t timestamp = 0;
	const uint32_t msg_stream_id = 0;
	int err;
	size_t i;

	for (i=0; i<sizeof(large_payload); i++) {
		large_payload[i] = i & 0xff;
	}

	err = rtmp_dechunker_alloc(&rd, msg_handler, &test);
	TEST_ERR(err);

#if 0
	/* Short */
	memset(&test, 0, sizeof(test));
	test.rd = rd;

	err = rtmp_chunker(3, timestamp, RTMP_TYPE_AUDIO, msg_stream_id,
			   short_payload, sizeof (short_payload),
			   chunk_handler, &test);
	TEST_ERR(err);

	TEST_EQUALS(1, test.n_chunk);
	TEST_EQUALS(12+1, test.total_bytes);

	TEST_EQUALS(1, test.n_msg);
	TEST_MEMCMP(short_payload, sizeof(short_payload),
		    test.buf, test.buf_len);
#endif


#if 1
	/* Medium */
	memset(&test, 0, sizeof(test));
	test.rd = rd;

	err = rtmp_chunker(3, timestamp, RTMP_TYPE_AMF0, msg_stream_id,
			   amf_payload, sizeof (amf_payload),
			   chunk_handler, &test);
	TEST_ERR(err);

	TEST_EQUALS(2, test.n_chunk);
	TEST_EQUALS(212, test.total_bytes);

	TEST_EQUALS(1, test.n_msg);
	TEST_MEMCMP(amf_payload, sizeof(amf_payload),
		    test.buf, test.buf_len);

	test.buf = mem_deref(test.buf);
#endif


#if 1
	/* Large */
	memset(&test, 0, sizeof(test));
	test.rd = rd;

	err = rtmp_chunker(3, timestamp, RTMP_TYPE_AMF0, msg_stream_id,
			   large_payload, sizeof (large_payload),
			   chunk_handler, &test);
	TEST_ERR(err);

	TEST_EQUALS(8, test.n_chunk);
	TEST_EQUALS(12 + 7 + 8*128, test.total_bytes);

	TEST_EQUALS(1, test.n_msg);
	TEST_MEMCMP(large_payload, sizeof(large_payload),
		    test.buf, test.buf_len);
#endif

 out:
	mem_deref(rd);
	mem_deref(test.buf);

	return err;
}


int test_rtmp(void)
{
	int err;

	/* Test headers */
	err = test_rtmp_header_type0();
	TEST_ERR(err);
	err = test_rtmp_header_type1();
	TEST_ERR(err);
	err = test_rtmp_header_type2();
	TEST_ERR(err);
	err = test_rtmp_header_type3();
	TEST_ERR(err);

	err = test_rtmp_header(63);
	TEST_ERR(err);
	err = test_rtmp_header(319);
	TEST_ERR(err);
	err = test_rtmp_header(65599);
	TEST_ERR(err);

	/* Test packet decoding */
	err = test_rtmp_decode_audio();
	TEST_ERR(err);

	/* Test chunking */
	err = test_rtmp_chunking();
	TEST_ERR(err);

 out:
	return err;
}
