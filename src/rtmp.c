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


static void rtmp_header_init(struct rtmp_header *hdr,
			     unsigned fmt, uint32_t chunk_id)
{
	memset(hdr, 0, sizeof(*hdr));

	hdr->format   = fmt;
	hdr->chunk_id = chunk_id;
}


static int test_rtmp_header_type0(void)
{
	struct rtmp_header hdr0, hdr;
	struct mbuf *mb;
	const uint32_t chunk_id = 3;
	const uint32_t timestamp = 160;
	const uint32_t msg_length = 194;
	const uint8_t msg_type_id = 20;
	const uint32_t msg_stream_id = 1234567;
	int err;

	mb = mbuf_alloc(512);
	if (!mb)
		return ENOMEM;

	rtmp_header_init(&hdr0, 0, chunk_id);

	hdr0.timestamp = timestamp;
	hdr0.length    = msg_length;
	hdr0.type_id   = msg_type_id;
	hdr0.stream_id = msg_stream_id;

	err = rtmp_header_encode(mb, &hdr0);
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
	TEST_EQUALS(msg_length,      hdr.length);
	TEST_EQUALS(msg_type_id,     hdr.type_id);
	TEST_EQUALS(msg_stream_id,   hdr.stream_id);

 out:
	mem_deref(mb);

	return err;
}


static int test_rtmp_header_type1(void)
{
	struct rtmp_header hdr0, hdr;
	struct mbuf *mb;
	const uint32_t chunk_id = 3;
	const uint32_t timestamp_delta = 160;
	const uint32_t msg_length = 194;
	const uint8_t msg_type_id = 20;
	int err;

	mb = mbuf_alloc(512);
	if (!mb)
		return ENOMEM;

	rtmp_header_init(&hdr0, 1, chunk_id);

	hdr0.timestamp_delta = timestamp_delta;
	hdr0.length    = msg_length;
	hdr0.type_id   = msg_type_id;

	err = rtmp_header_encode(mb, &hdr0);
	TEST_ERR(err);
	TEST_EQUALS(1+7, mb->end);

	mb->pos = 0;

	err = rtmp_header_decode(&hdr, mb);
	TEST_ERR(err);

	/* compare */
	TEST_EQUALS(1,               hdr.format);
	TEST_EQUALS(chunk_id,        hdr.chunk_id);
	TEST_EQUALS(timestamp_delta, hdr.timestamp_delta);
	TEST_EQUALS(msg_length,      hdr.length);
	TEST_EQUALS(msg_type_id,     hdr.type_id);

 out:
	mem_deref(mb);

	return err;
}


static int test_rtmp_header_type2(void)
{
	struct rtmp_header hdr0, hdr;
	struct mbuf *mb;
	const uint32_t chunk_id = 3;
	const uint32_t timestamp_delta = 160;
	int err;

	mb = mbuf_alloc(512);
	if (!mb)
		return ENOMEM;

	rtmp_header_init(&hdr0, 2, chunk_id);

	hdr0.timestamp_delta = timestamp_delta;

	err = rtmp_header_encode(mb, &hdr0);
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
	struct rtmp_header hdr0, hdr;
	struct mbuf *mb;
	const uint32_t chunk_id = 3;
	int err;

	mb = mbuf_alloc(512);
	if (!mb)
		return ENOMEM;

	rtmp_header_init(&hdr0, 3, chunk_id);

	err = rtmp_header_encode(mb, &hdr0);
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
	struct rtmp_header hdr0, hdr;
	struct mbuf *mb;
	const uint32_t timestamp = 160;
	const uint32_t msg_length = 194;
	const uint8_t msg_type_id = 20;
	const uint32_t msg_stream_id = 1234567;
	int err;

	mb = mbuf_alloc(512);
	if (!mb)
		return ENOMEM;

	rtmp_header_init(&hdr0, 0, chunk_id);

	hdr0.timestamp = timestamp;
	hdr0.length    = msg_length;
	hdr0.type_id   = msg_type_id;
	hdr0.stream_id = msg_stream_id;

	err = rtmp_header_encode(mb, &hdr0);
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
	TEST_EQUALS(msg_length,      hdr.length);
	TEST_EQUALS(msg_type_id,     hdr.type_id);
	TEST_EQUALS(msg_stream_id,   hdr.stream_id);

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
	TEST_EQUALS(82,              hdr.length);
	TEST_EQUALS(RTMP_TYPE_AUDIO, hdr.type_id);

	TEST_MEMCMP(packet_bytes + HDR_SIZE, sizeof(packet_bytes) - HDR_SIZE,
		    mbuf_buf(mb), mbuf_get_left(mb));

	/* XXX: decode control header ? */

 out:
	mem_deref(mb);
	return err;
}


static int test_rtmp_decode_window_ack_size(void)
{
	static const uint8_t packet_bytes[] = {
		0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x05,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x26, 0x25, 0xa0
	};

#define HDR_SIZE 8

	struct rtmp_header hdr;
	struct mbuf *mb;
	const void *p;
	uint32_t value;
	int err;

	mb = mbuf_packet(packet_bytes, sizeof(packet_bytes));

	err = rtmp_header_decode(&hdr, mb);
	TEST_ERR(err);

	/* compare */
	TEST_EQUALS(0,                         hdr.format);
	TEST_EQUALS(2,                         hdr.chunk_id);
	TEST_EQUALS(0,                         hdr.timestamp);
	TEST_EQUALS(4,                         hdr.length);
	TEST_EQUALS(RTMP_TYPE_WINDOW_ACK_SIZE, hdr.type_id);
	TEST_EQUALS(0,                         hdr.stream_id);

	TEST_EQUALS(4, mbuf_get_left(mb));
	p = mbuf_buf(mb);
	value = ntohl( *(uint32_t *)p );

	TEST_EQUALS(2500000, value);

 out:
	mem_deref(mb);
	return err;
}


static int test_rtmp_decode_ping_request(void)
{
	static const uint8_t packet_bytes[] = {
		0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x04,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00,
		0x50, 0x2e
	};

#define HDR_SIZE 8

	struct rtmp_header hdr;
	struct mbuf *mb;
	const void *p;
	uint16_t value;
	int err;

	mb = mbuf_packet(packet_bytes, sizeof(packet_bytes));

	err = rtmp_header_decode(&hdr, mb);
	TEST_ERR(err);

	/* compare */
	TEST_EQUALS(0,                         hdr.format);
	TEST_EQUALS(RTMP_CHUNK_ID_CONTROL,     hdr.chunk_id);
	TEST_EQUALS(0,                         hdr.timestamp);
	TEST_EQUALS(6,                         hdr.length);
	TEST_EQUALS(RTMP_TYPE_USER_CONTROL_MSG, hdr.type_id);
	TEST_EQUALS(0,                         hdr.stream_id);

	TEST_EQUALS(6, mbuf_get_left(mb));
	p = mbuf_buf(mb);
	value = ntohs( *(uint16_t *)p );

	TEST_EQUALS(6, value);  /* Ping Request */

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


static void msg_handler(struct rtmp_message *msg, void *arg)
{
	struct test *test = arg;

	++test->n_msg;

	mem_deref(test->buf);

	test->buf = mem_ref(msg->buf);
	test->buf_len = msg->length;
}


static int chunk_handler(const struct rtmp_header *hdr,
			 const uint8_t *pld, size_t pld_len, void *arg)
{
	struct test *test = arg;
	struct mbuf *mb;
	int err = 0;

	mb = mbuf_alloc(1024);
	if (!mb)
		return ENOMEM;

	++test->n_chunk;

	err = rtmp_header_encode(mb, hdr);
	if (err)
		goto out;

	err |= mbuf_write_mem(mb, pld, pld_len);
	if (err)
		goto out;

	test->total_bytes += (mb->end);

	mb->pos = 0;

	/* feed the chunks into the de-chunker */
	err = rtmp_dechunker_receive(test->rd, mb);
	if (err)
		goto out;

 out:
	mem_deref(mb);

	return err;
}


/*
 * XXX: add test for incomplete message
 */
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

	/* Short */
	memset(&test, 0, sizeof(test));
	test.rd = rd;

	err = rtmp_chunker(0, 3, timestamp, 0, RTMP_TYPE_AUDIO, msg_stream_id,
			   short_payload, sizeof (short_payload),
			   chunk_handler, &test);
	if (err)
		goto out;

	TEST_EQUALS(1, test.n_chunk);
	TEST_EQUALS(12+1, test.total_bytes);

	TEST_EQUALS(1, test.n_msg);
	TEST_MEMCMP(short_payload, sizeof(short_payload),
		    test.buf, test.buf_len);

	test.buf = mem_deref(test.buf);


	/* Medium */
	memset(&test, 0, sizeof(test));
	test.rd = rd;

	err = rtmp_chunker(0, 3, timestamp, 0, RTMP_TYPE_AMF0, msg_stream_id,
			   amf_payload, sizeof (amf_payload),
			   chunk_handler, &test);
	if (err)
		goto out;

	TEST_EQUALS(2, test.n_chunk);
	TEST_EQUALS(212, test.total_bytes);

	TEST_EQUALS(1, test.n_msg);
	TEST_MEMCMP(amf_payload, sizeof(amf_payload),
		    test.buf, test.buf_len);

	test.buf = mem_deref(test.buf);


	/* Large */
	memset(&test, 0, sizeof(test));
	test.rd = rd;

	err = rtmp_chunker(0, 3, timestamp, 0, RTMP_TYPE_AMF0, msg_stream_id,
			   large_payload, sizeof (large_payload),
			   chunk_handler, &test);
	if (err)
		goto out;

	TEST_EQUALS(8, test.n_chunk);
	TEST_EQUALS(12 + 7 + 8*128, test.total_bytes);

	TEST_EQUALS(1, test.n_msg);
	TEST_MEMCMP(large_payload, sizeof(large_payload),
		    test.buf, test.buf_len);

	test.buf = mem_deref(test.buf);

 out:
	mem_deref(rd);
	mem_deref(test.buf);

	return err;
}


static const uint8_t amf_connect[] = {

	/* string */
	0x02,
	0x00, 0x07,
	0x63, 0x6f, 0x6e, 0x6e, 0x65, 0x63, 0x74,

	/* number */
	0x00,
	0x3f, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

	/* object */
	0x03,

	/* app:vod */
	0x00, 0x03,
	0x61, 0x70, 0x70,
	0x02,
	0x00, 0x03,
	0x76, 0x6f, 0x64,

	/* flashVer */
	0x00, 0x08,
	0x66, 0x6c, 0x61, 0x73, 0x68, 0x56, 0x65, 0x72,
	0x02,
	0x00, 0x0d,
	0x4c, 0x4e, 0x58, 0x20, 0x39, 0x2c, 0x30, 0x2c,
	0x31, 0x32, 0x34, 0x2c, 0x32,

	/* tcUrl */
	0x00, 0x05,
	0x74, 0x63, 0x55, 0x72, 0x6c,
	0x02, 0x00, 0x1e,
	0x72, 0x74, 0x6d, 0x70, 0x3a, 0x2f, 0x2f, 0x31, 0x38, 0x34,
	0x2e, 0x37, 0x32, 0x2e, 0x32, 0x33, 0x39, 0x2e, 0x31, 0x34,
	0x39, 0x3a, 0x31, 0x39, 0x33, 0x35, 0x2f, 0x76, 0x6f, 0x64,

	/* fpad */
	0x00, 0x04,
	0x66, 0x70, 0x61, 0x64,
	0x01,
	0x00,

	/* capabilities */
	0x00, 0x0c,
	0x63, 0x61, 0x70, 0x61, 0x62, 0x69, 0x6c, 0x69, 0x74, 0x69, 0x65, 0x73,
	0x00,
	0x40, 0x2e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

	/* audioCodecs */
	0x00, 0x0b,
	0x61, 0x75, 0x64, 0x69, 0x6f, 0x43, 0x6f, 0x64, 0x65, 0x63, 0x73,
	0x00,
	0x40, 0xaf, 0xce, 0x00, 0x00, 0x00, 0x00, 0x00,

	/* videoCodecs */
	0x00, 0x0b,
	0x76, 0x69, 0x64, 0x65, 0x6f, 0x43, 0x6f, 0x64, 0x65, 0x63, 0x73,
	0x00,
	0x40, 0x6f, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00,

	/* videoFunction */
	0x00, 0x0d,
	0x76, 0x69, 0x64, 0x65, 0x6f, 0x46, 0x75, 0x6e,
	0x63, 0x74, 0x69, 0x6f, 0x6e,
	0x00,
	0x3f, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

	/* object end */
	0x00, 0x00, 0x09
};


static const uint8_t amf_result[] = {
	0x02, 0x00, 0x07, 0x5f,
	0x72, 0x65, 0x73, 0x75, 0x6c, 0x74, 0x00, 0x3f,
	0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03,
	0x00, 0x06, 0x66, 0x6d, 0x73, 0x56, 0x65, 0x72,
	0x02, 0x00, 0x0e, 0x46, 0x4d, 0x53, 0x2f, 0x33,
	0x2c, 0x35, 0x2c, 0x37, 0x2c, 0x37, 0x30, 0x30,
	0x39, 0x00, 0x0c, 0x63, 0x61, 0x70, 0x61, 0x62,
	0x69, 0x6c, 0x69, 0x74, 0x69, 0x65, 0x73, 0x00,
	0x40, 0x3f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x04, 0x6d, 0x6f, 0x64, 0x65, 0x00, 0x3f,
	0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x09, 0x03, 0x00, 0x05, 0x6c, 0x65, 0x76,
	0x65, 0x6c, 0x02, 0x00, 0x06, 0x73, 0x74, 0x61,
	0x74, 0x75, 0x73, 0x00, 0x04, 0x63, 0x6f, 0x64,
	0x65, 0x02, 0x00, 0x1d, 0x4e, 0x65, 0x74, 0x43,
	0x6f, 0x6e, 0x6e, 0x65, 0x63, 0x74, 0x69, 0x6f,
	0x6e, 0x2e, 0x43, 0x6f, 0x6e, 0x6e, 0x65, 0x63,
	0x74, 0x2e, 0x53, 0x75, 0x63, 0x63, 0x65, 0x73,
	0x73, 0x00, 0x0b, 0x64, 0x65, 0x73, 0x63, 0x72,
	0x69, 0x70, 0x74, 0x69, 0x6f, 0x6e, 0x02, 0x00,
	0x15, 0x43, 0x6f, 0x6e, 0x6e, 0x65, 0x63, 0x74,
	0x69, 0x6f, 0x6e, 0x20, 0x73, 0x75, 0x63, 0x63,
	0x65, 0x65, 0x64, 0x65, 0x64, 0x2e, 0x00, 0x04,
	0x64, 0x61, 0x74, 0x61, 0x08, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x07, 0x76, 0x65, 0x72, 0x73, 0x69,
	0x6f, 0x6e, 0x02, 0x00, 0x0a, 0x33, 0x2c, 0x35,
	0x2c, 0x37, 0x2c, 0x37, 0x30, 0x30, 0x39, 0x00,
	0x00, 0x09, 0x00, 0x08, 0x63, 0x6c, 0x69, 0x65,
	0x6e, 0x74, 0x69, 0x64, 0x00, 0x41, 0xc2, 0xc9,
	0xb8, 0x6c, 0x00, 0x00, 0x00, 0x00, 0x0e, 0x6f,
	0x62, 0x6a, 0x65, 0x63, 0x74, 0x45, 0x6e, 0x63,
	0x6f, 0x64, 0x69, 0x6e, 0x67, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x09
};


static const uint8_t amf_connect_result[] = {
	0x02, 0x00, 0x07, 0x5f,
	0x72, 0x65, 0x73, 0x75, 0x6c, 0x74, 0x00, 0x3f,
	0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03,
	0x00, 0x06, 0x66, 0x6d, 0x73, 0x56, 0x65, 0x72,
	0x02, 0x00, 0x0e, 0x46, 0x4d, 0x53, 0x2f, 0x33,
	0x2c, 0x35, 0x2c, 0x37, 0x2c, 0x37, 0x30, 0x30,
	0x39, 0x00, 0x0c, 0x63, 0x61, 0x70, 0x61, 0x62,
	0x69, 0x6c, 0x69, 0x74, 0x69, 0x65, 0x73, 0x00,
	0x40, 0x3f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x04, 0x6d, 0x6f, 0x64, 0x65, 0x00, 0x3f,
	0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x09, 0x03, 0x00, 0x05, 0x6c, 0x65, 0x76,
	0x65, 0x6c, 0x02, 0x00, 0x06, 0x73, 0x74, 0x61,
	0x74, 0x75, 0x73, 0x00, 0x04, 0x63, 0x6f, 0x64,
	0x65, 0x02, 0x00, 0x1d, 0x4e, 0x65, 0x74, 0x43,
	0x6f, 0x6e, 0x6e, 0x65, 0x63, 0x74, 0x69, 0x6f,
	0x6e, 0x2e, 0x43, 0x6f, 0x6e, 0x6e, 0x65, 0x63,
	0x74, 0x2e, 0x53, 0x75, 0x63, 0x63, 0x65, 0x73,
	0x73, 0x00, 0x0b, 0x64, 0x65, 0x73, 0x63, 0x72,
	0x69, 0x70, 0x74, 0x69, 0x6f, 0x6e, 0x02, 0x00,
	0x15, 0x43, 0x6f, 0x6e, 0x6e, 0x65, 0x63, 0x74,
	0x69, 0x6f, 0x6e, 0x20, 0x73, 0x75, 0x63, 0x63,
	0x65, 0x65, 0x64, 0x65, 0x64, 0x2e, 0x00, 0x04,
	0x64, 0x61, 0x74, 0x61, 0x08, 0x00, 0x00, 0x00,
	0x01, 0x00, 0x07, 0x76, 0x65, 0x72, 0x73, 0x69,
	0x6f, 0x6e, 0x02, 0x00, 0x0a, 0x33, 0x2c, 0x35,
	0x2c, 0x37, 0x2c, 0x37, 0x30, 0x30, 0x39, 0x00,
	0x00, 0x09, 0x00, 0x08, 0x63, 0x6c, 0x69, 0x65,
	0x6e, 0x74, 0x69, 0x64, 0x00, 0x41, 0xc5, 0xe6,
	0x21, 0x42, 0x80, 0x00, 0x00, 0x00, 0x0e, 0x6f,
	0x62, 0x6a, 0x65, 0x63, 0x74, 0x45, 0x6e, 0x63,
	0x6f, 0x64, 0x69, 0x6e, 0x67, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x09
};

static const uint8_t amf_createstream[] = {
	0x02, 0x00, 0x0c, 0x63, 0x72, 0x65, 0x61, 0x74,
	0x65, 0x53, 0x74, 0x72, 0x65, 0x61, 0x6d, 0x00,
	0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x05
};

static const uint8_t amf_publish[] = {
	0x02, 0x00, 0x07, 0x70,
	0x75, 0x62, 0x6c, 0x69, 0x73, 0x68, 0x00, 0x40,
	0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05,
	0x02, 0x00, 0x10, 0x76, 0x79, 0x62, 0x51, 0x4e,
	0x46, 0x65, 0x37, 0x59, 0x72, 0x62, 0x30, 0x4e,
	0x33, 0x4f, 0x51, 0x02, 0x00, 0x04, 0x6c, 0x69,
	0x76, 0x65
};


static int test_rtmp_amf_encode_connect(void)
{
	struct mbuf *mb = NULL;
	int err;

	mb = mbuf_alloc(512);
	if (!mb)
		return ENOMEM;

	err  = rtmp_amf_encode_string(mb, "connect");
	err |= rtmp_amf_encode_number(mb, 1.0);

	err |= rtmp_amf_encode_object(mb, false, 8,
		     AMF_TYPE_STRING, "app", "vod",
		     AMF_TYPE_STRING, "flashVer", "LNX 9,0,124,2",
		     AMF_TYPE_STRING, "tcUrl","rtmp://184.72.239.149:1935/vod",
		     AMF_TYPE_BOOLEAN, "fpad", false,
		     AMF_TYPE_NUMBER, "capabilities", 15.0,
		     AMF_TYPE_NUMBER, "audioCodecs", 4071.0,
		     AMF_TYPE_NUMBER, "videoCodecs", 252.0,
		     AMF_TYPE_NUMBER, "videoFunction", 1.0);
	TEST_ERR(err);

	TEST_MEMCMP(amf_connect, sizeof(amf_connect), mb->buf, mb->end);

 out:
	mem_deref(mb);

	return err;
}


static int test_rtmp_amf_encode_connect_result(void)
{
	struct mbuf *mb = NULL;
	int err;

	mb = mbuf_alloc(512);
	if (!mb)
		return ENOMEM;

	err  = rtmp_amf_encode_string(mb, "_result");
	err |= rtmp_amf_encode_number(mb, 1);

	err |= rtmp_amf_encode_object(mb, false, 3,
			     AMF_TYPE_STRING, "fmsVer", "FMS/3,5,7,7009",
			     AMF_TYPE_NUMBER, "capabilities", 31.0,
			     AMF_TYPE_NUMBER, "mode", 1.0);
	TEST_ERR(err);

	err |= rtmp_amf_encode_object(mb, false, 6,
	      AMF_TYPE_STRING, "level", "status",
	      AMF_TYPE_STRING, "code", "NetConnection.Connect.Success",
	      AMF_TYPE_STRING, "description", "Connection succeeded.",
	      AMF_TYPE_ARRAY, "data", 1,
		      AMF_TYPE_STRING, "version", "3,5,7,7009",
	      AMF_TYPE_NUMBER, "clientid", 734806661.0,
	      AMF_TYPE_NUMBER, "objectEncoding", 0.0);
	TEST_ERR(err);

	TEST_MEMCMP(amf_connect_result, sizeof(amf_connect_result),
		    mb->buf, mb->end);

 out:
	mem_deref(mb);

	return err;
}


static int test_rtmp_amf_encode_createstream(void)
{
	struct mbuf *mb = 0;
	int err;

	mb = mbuf_alloc(512);
	if (!mb)
		return ENOMEM;

	err  = rtmp_amf_encode_string(mb, "createStream");
	err |= rtmp_amf_encode_number(mb, 2);
	err |= rtmp_amf_encode_null(mb);

	TEST_MEMCMP(amf_createstream, sizeof(amf_createstream),
		    mb->buf, mb->end);

 out:
	mem_deref(mb);

	return err;
}


static int test_rtmp_amf_encode_publish(void)
{
	struct mbuf *mb = NULL;
	int err;

	mb = mbuf_alloc(512);
	if (!mb)
		return ENOMEM;

	err  = rtmp_amf_encode_string(mb, "publish");
	err |= rtmp_amf_encode_number(mb, 5);
	err |= rtmp_amf_encode_null(mb);
	err |= rtmp_amf_encode_string(mb, "vybQNFe7Yrb0N3OQ");
	err |= rtmp_amf_encode_string(mb, "live");

	TEST_MEMCMP(amf_publish, sizeof(amf_publish), mb->buf, mb->end);

 out:
	mem_deref(mb);

	return err;
}


static int test_rtmp_amf_decode(const uint8_t *buf, size_t len,
				size_t count, size_t count_all,
				const char *command_name)
{
	struct odict *dict = NULL;
	struct odict_entry *e;
	struct mbuf *mb = NULL;
	int err;

	mb = mbuf_packet(buf, len);

	err = odict_alloc(&dict, 32);
	if (err)
		goto out;

	err = rtmp_amf_decode(dict, mb);
	TEST_ERR(err);

#if 0
	re_printf("ODICT: %H\n", odict_debug, dict);
#endif

	TEST_EQUALS(count,     odict_count(dict, false));
	TEST_EQUALS(count_all, odict_count(dict, true));

	e = list_ledata(dict->lst.head);
	TEST_ASSERT(e != NULL);
	TEST_EQUALS(ODICT_STRING, e->type);
	TEST_STRCMP(command_name, str_len(command_name),
		    e->u.str, str_len(e->u.str));

	/* todo: verify decoded object */

 out:
	mem_deref(dict);
	mem_deref(mb);

	return err;
}


static int test_rtmp_amf_random_input(void)
{
#define NUM_BYTES (16*1024)
	struct odict *dict = NULL;
	struct mbuf *mb = NULL;
	size_t i;
	int err;

	mb = mbuf_alloc(NUM_BYTES);
	if (!mb)
		return ENOMEM;

	mb->pos = 0;
	mb->end = NUM_BYTES;

	rand_bytes(mb->buf, mb->end);

	err = odict_alloc(&dict, 4096);
	if (err)
		goto out;

	for (i = 0; i < NUM_BYTES; i++) {

		int e;

		if (mbuf_get_left(mb) == 0)
			break;

		e = rtmp_amf_decode(dict, mb);
		if (e == 0) {
			re_printf("Success!\n");
		}
	}

 out:
	mem_deref(dict);
	mem_deref(mb);

	return err;
}


struct rtmp_endpoint {
	struct rtmp_endpoint *other;
	struct rtmp_conn *conn;
	struct tcp_sock *ts;     /* server only */
	const char *tag;
	bool is_client;
	unsigned n_estab;
	unsigned n_close;
	int err;
};


static void estab_handler(void *arg)
{
	struct rtmp_endpoint *ep = arg;

	DEBUG_NOTICE("[%s] Established\n", ep->tag);

	++ep->n_estab;

	if (ep->other->n_estab)
		re_cancel();
}


static void close_handler(int err, void *arg)
{
	struct rtmp_endpoint *ep = arg;

	DEBUG_NOTICE("rtmp connection closed (%m)\n", err);

	ep->err = err;
	++ep->n_close;

	re_cancel();
}


static void endpoint_destructor(void *data)
{
	struct rtmp_endpoint *ep = data;

	mem_deref(ep->conn);
	mem_deref(ep->ts);
}


static struct rtmp_endpoint *rtmp_endpoint_alloc(bool is_client)
{
	struct rtmp_endpoint *ep;

	ep = mem_zalloc(sizeof(*ep), endpoint_destructor);

	ep->is_client = is_client;

	ep->tag = is_client ? "Client" : "Server";

	return ep;
}


static void tcp_conn_handler(const struct sa *peer, void *arg)
{
	struct rtmp_endpoint *ep = arg;
	int err;

	re_printf("incoming TCP connect from %J\n", peer);

	err = rtmp_accept(&ep->conn, ep->ts, estab_handler, close_handler, ep);
	if (err) {
		ep->err = err;
		re_cancel();
	}
}


static int test_rtmp_client_server_conn(void)
{
	struct rtmp_endpoint *cli, *srv;
	struct sa srv_addr;
	char uri[256];
	int err = 0;

	re_printf("- - - client/server loop - - - start\n");

	cli = rtmp_endpoint_alloc(true);
	srv = rtmp_endpoint_alloc(false);
	TEST_ASSERT(cli != NULL);
	TEST_ASSERT(srv != NULL);

	cli->other = srv;
	srv->other = cli;

	err = sa_set_str(&srv_addr, "127.0.0.1", 0);
	TEST_ERR(err);

	err = tcp_listen(&srv->ts, &srv_addr, tcp_conn_handler, srv);
	TEST_ERR(err);

	err = tcp_local_get(srv->ts, &srv_addr);
	TEST_ERR(err);

	re_printf("TCP listening on %J\n", &srv_addr);

	re_snprintf(uri, sizeof(uri), "rtmp://%J/vod/foo", &srv_addr);

	err = rtmp_connect(&cli->conn, uri, estab_handler, close_handler, cli);
	TEST_ERR(err);

	err = re_main_timeout(2000);
	if (err)
		goto out;

	if (cli->err == ENOMEM || srv->err == ENOMEM)
		goto out;

	TEST_EQUALS(0, cli->err);
	TEST_EQUALS(0, srv->err);

	TEST_EQUALS(1, cli->n_estab);
	TEST_EQUALS(1, srv->n_estab);
	TEST_EQUALS(0, cli->n_close);
	TEST_EQUALS(0, srv->n_close);

	TEST_EQUALS(2500000, rtmp_window_ack_size(cli->conn));
	TEST_EQUALS(2500000, rtmp_window_ack_size(srv->conn));

	re_printf("- - - client/server loop - - - end\n");

 out:
	mem_deref(srv);
	mem_deref(cli);

	return err;
}


int test_rtmp(void)
{
	int err = 0;

	/* Test headers */
	err |= test_rtmp_header_type0();
	err |= test_rtmp_header_type1();
	err |= test_rtmp_header_type2();
	err |= test_rtmp_header_type3();
	if (err)
		return err;

	err |= test_rtmp_header(63);
	err |= test_rtmp_header(319);
	err |= test_rtmp_header(65599);
	if (err)
		return err;

	/* Test packet decoding */
	err |= test_rtmp_decode_audio();
	err |= test_rtmp_decode_window_ack_size();
	err |= test_rtmp_decode_ping_request();
	if (err)
		return err;

	/* Test chunking */
	err |= test_rtmp_chunking();
	if (err)
		return err;

	/* AMF */
	err  = test_rtmp_amf_encode_connect();
	err |= test_rtmp_amf_encode_connect_result();
	err |= test_rtmp_amf_encode_createstream();
	err |= test_rtmp_amf_encode_publish();

	err |= test_rtmp_amf_decode(amf_connect, sizeof(amf_connect), 3, 10,
				    "connect");
	err |= test_rtmp_amf_decode(amf_result, sizeof(amf_result), 4, 11,
				    "_result");
	err |= test_rtmp_amf_decode(amf_connect_result,
				    sizeof(amf_connect_result), 4, 11,
				    "_result");
	err |= test_rtmp_amf_decode(amf_publish,
				    sizeof(amf_publish), 5, 5,
				    "publish");
	if (err)
		return err;

	err = test_rtmp_client_server_conn();
	if (err)
		return err;

	return err;
}


int test_rtmp_fuzzing(void)
{
	int err = 0;

	err |= test_rtmp_amf_random_input();

	return err;
}
