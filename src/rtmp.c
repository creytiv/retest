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


#define WINDOW_ACK_SIZE 2500000


#define NUM_MEDIA_PACKETS 5


#define TS_OFFSET 100


#define DUMMY_STREAM_ID 42


enum mode {
	MODE_PLAY,
	MODE_PUBLISH,
};


struct rtmp_endpoint {
	struct rtmp_endpoint *other;
	struct rtmp_conn *conn;
	struct rtmp_stream *stream;
	struct tcp_sock *ts;     /* server only */
	const char *tag;
	enum mode mode;
	bool is_client;
	unsigned n_estab;
	unsigned n_cmd;
	unsigned n_close;
	unsigned n_ready;
	unsigned n_play;
	unsigned n_publish;
	unsigned n_deletestream;
	unsigned n_audio;
	unsigned n_video;
	unsigned n_data;
	unsigned n_begin;
	int err;

	struct tcp_helper *th;
	size_t packet_count;
	bool fuzzing;
};


static const uint8_t fake_audio_packet[6] = {
	0x5b, 0xb2, 0xfb, 0x11, 0x46, 0xe9
};

static const uint8_t fake_video_packet[8] = {
	0xcb, 0x9c, 0xb5, 0x60, 0x7f, 0xe9, 0xbd, 0xe1
};

static const char *fake_stream_name = "sample.mp4";


static void endpoint_terminate(struct rtmp_endpoint *ep, int err)
{
	if (err) {
		DEBUG_WARNING("[ %s ] terminate: %m\n", ep->tag, err);
	}

	ep->err = err;
	re_cancel();
}


/* criteria for test to be finished */
static bool client_is_finished(const struct rtmp_endpoint *ep)
{
	switch (ep->mode) {

	case MODE_PLAY:
		return ep->n_ready > 0 &&
			ep->n_begin > 0 &&
			ep->n_audio >= NUM_MEDIA_PACKETS &&
			ep->n_video >= NUM_MEDIA_PACKETS;

	case MODE_PUBLISH:
		return ep->n_ready > 0 &&
			ep->n_begin > 0;
	}

	return false;
}


static bool server_is_finished(const struct rtmp_endpoint *ep)
{
	switch (ep->mode) {

	case MODE_PLAY:
		return ep->n_play > 0;

	case MODE_PUBLISH:
		return ep->n_publish > 0 &&
			ep->n_audio >= NUM_MEDIA_PACKETS &&
			ep->n_video >= NUM_MEDIA_PACKETS;
	}

	return false;
}


static bool endpoints_are_finished(const struct rtmp_endpoint *ep)
{
	if (ep->is_client) {
		return client_is_finished(ep) &&
			server_is_finished(ep->other);
	}
	else {
		return client_is_finished(ep->other) &&
			server_is_finished(ep);
	}
}


static void stream_command_handler(const struct rtmp_amf_message *msg,
				   void *arg)
{
	struct rtmp_endpoint *ep = arg;

	(void)msg;
	(void)ep;
}


static void test_done(struct rtmp_endpoint *ep)
{
	struct rtmp_endpoint *client;

	re_printf("test is done -- shutdown\n");

	if (ep->is_client)
		client = ep;
	else
		client = ep->other;

	/* Force destruction here to test robustness */
	client->conn = mem_deref(client->conn);
}


static void stream_control_handler(enum rtmp_event_type event, void *arg)
{
	struct rtmp_endpoint *ep = arg;

	DEBUG_NOTICE("[ %s ] got control event:  event=%d\n",
		     ep->tag, event);

	switch (event) {

	case RTMP_EVENT_STREAM_BEGIN:
		++ep->n_begin;

		/* Test complete ? */
		if (endpoints_are_finished(ep)) {

			test_done(ep);
			return;
		}
		break;

	default:
		break;
	}
}


static void audio_handler(uint32_t timestamp,
			  const uint8_t *pld, size_t len, void *arg)
{
	struct rtmp_endpoint *ep = arg;
	int err = 0;

	re_printf("recv audio (%u) \n", timestamp);

	TEST_EQUALS(ep->n_audio, timestamp);

	++ep->n_audio;

	TEST_MEMCMP(fake_audio_packet, sizeof(fake_audio_packet), pld, len);

	/* Test complete ? */
	if (endpoints_are_finished(ep)) {

		test_done(ep);
		return;
	}

 out:
	if (err)
		endpoint_terminate(ep, err);
}


static void video_handler(uint32_t timestamp,
			  const uint8_t *pld, size_t len, void *arg)
{
	struct rtmp_endpoint *ep = arg;
	int err = 0;

	re_printf("recv video (%u) \n", timestamp);

	TEST_EQUALS(TS_OFFSET + ep->n_video, timestamp);

	++ep->n_video;

	TEST_MEMCMP(fake_video_packet, sizeof(fake_video_packet), pld, len);

	/* Test complete ? */
	if (endpoints_are_finished(ep)) {

		test_done(ep);
		return;
	}

 out:
	if (err)
		endpoint_terminate(ep, err);
}


static void stream_data_handler(const struct rtmp_amf_message *msg, void *arg)
{
	struct rtmp_endpoint *ep = arg;
	const char *command;
	bool ret;
	bool value;
	int err = 0;

	++ep->n_data;

	command = rtmp_amf_message_string(msg, 0);
	TEST_STRCMP("|RtmpSampleAccess", 17, command, str_len(command));

	ret = rtmp_amf_message_get_boolean(msg, &value, 1);
	TEST_ASSERT(ret);
	TEST_ASSERT(!value);

	ret = rtmp_amf_message_get_boolean(msg, &value, 2);
	TEST_ASSERT(ret);
	TEST_ASSERT(!value);

 out:
	if (err)
		endpoint_terminate(ep, err);
}


static void stream_create_resp_handler(const struct rtmp_amf_message *msg,
				       void *arg)
{
	struct rtmp_endpoint *ep = arg;
	int err;

	re_printf("create stream resp: %H\n",
		  odict_debug, rtmp_amf_message_dict(msg));

	++ep->n_ready;

	switch (ep->mode) {

	case MODE_PLAY:
		err = rtmp_play(ep->stream, fake_stream_name);
		if (err)
			goto error;
		break;

	case MODE_PUBLISH:
		err = rtmp_publish(ep->stream, fake_stream_name);
		if (err)
			goto error;
		break;

	default:
		err = EPROTO;
		goto error;
	}

	return;

 error:
	endpoint_terminate(ep, err);
}


static void estab_handler(void *arg)
{
	struct rtmp_endpoint *ep = arg;
	int err = 0;

	DEBUG_INFO("[%s] Established\n", ep->tag);

	++ep->n_estab;

	if (ep->is_client) {

		err = rtmp_stream_create(&ep->stream, ep->conn,
					 stream_create_resp_handler,
					 stream_command_handler,
					 stream_control_handler,
					 audio_handler,
					 video_handler, stream_data_handler,
					 ep);
		if (err)
			goto error;
	}

	return;

 error:
	endpoint_terminate(ep, err);
}


/* Server */
static int server_send_reply(struct rtmp_conn *conn,
			     const struct rtmp_amf_message *req)
{
	const char *code = "NetConnection.Connect.Success";
	const char *descr = "Connection succeeded.";
	int err;

	err = rtmp_amf_reply(conn, 0, true, req,
				2,

		RTMP_AMF_TYPE_OBJECT, 3,
			RTMP_AMF_TYPE_STRING, "fmsVer",       "FMS/3,5,7,7009",
			RTMP_AMF_TYPE_NUMBER, "capabilities", 31.0,
			RTMP_AMF_TYPE_NUMBER, "mode",         1.0,

		RTMP_AMF_TYPE_OBJECT, 6,
			RTMP_AMF_TYPE_STRING, "level",        "status",
			RTMP_AMF_TYPE_STRING, "code",         code,
			RTMP_AMF_TYPE_STRING, "description",  descr,
			RTMP_AMF_TYPE_ECMA_ARRAY,  "data",         1,
			    RTMP_AMF_TYPE_STRING, "version",      "3,5,7,7009",
			RTMP_AMF_TYPE_NUMBER, "clientid",     734806661.0,
			RTMP_AMF_TYPE_NUMBER, "objectEncoding", 0.0);

	return err;
}


static void command_handler(const struct rtmp_amf_message *msg, void *arg)
{
	struct rtmp_endpoint *ep = arg;
	const char *name;
	int err = 0;

	TEST_ASSERT(!ep->is_client);

	name = rtmp_amf_message_string(msg, 0);

	re_printf("  .. command: %s\n", name);

	++ep->n_cmd;

	if (0 == str_casecmp(name, "connect")) {

		err = rtmp_control(ep->conn, RTMP_TYPE_WINDOW_ACK_SIZE,
				   (uint32_t)WINDOW_ACK_SIZE);
		if (err)
			goto error;

		err = rtmp_control(ep->conn, RTMP_TYPE_SET_PEER_BANDWIDTH,
				   (uint32_t)WINDOW_ACK_SIZE, 2);
		if (err)
			goto error;

		/* Stream Begin */
		err = rtmp_control(ep->conn, RTMP_TYPE_USER_CONTROL_MSG,
				   RTMP_EVENT_STREAM_BEGIN,
				   RTMP_CONTROL_STREAM_ID);
		if (err)
			goto error;

		err = server_send_reply(ep->conn, msg);
		if (err) {
			re_printf("rtmp: reply failed (%m)\n", err);
			goto error;
		}
	}
	else if (0 == str_casecmp(name, "createStream")) {

		uint32_t stream_id = DUMMY_STREAM_ID;

		err = rtmp_stream_alloc(&ep->stream, ep->conn, stream_id,
					stream_command_handler,
					stream_control_handler, audio_handler,
					video_handler, stream_data_handler,
					ep);
		if (err) {
			goto error;
		}

		err = rtmp_amf_reply(ep->conn, 0, true, msg,
					2,
				RTMP_AMF_TYPE_NULL,
				RTMP_AMF_TYPE_NUMBER, (double)stream_id);
		if (err) {
			re_printf("rtmp: reply failed (%m)\n", err);
			goto error;
		}
	}
	else if (0 == str_casecmp(name, "play")) {

		const char *stream_name;
		uint64_t tid;
		uint32_t i;

		++ep->n_play;

		if (!rtmp_amf_message_get_number(msg, &tid, 1)) {
			err = EPROTO;
			goto out;
		}
		TEST_EQUALS(0, tid);

		stream_name = rtmp_amf_message_string(msg, 3);
		TEST_STRCMP(fake_stream_name, strlen(fake_stream_name),
			    stream_name, str_len(stream_name));

		/* Stream Begin */
		err = rtmp_control(ep->conn, RTMP_TYPE_USER_CONTROL_MSG,
				   RTMP_EVENT_STREAM_BEGIN,
				   (uint32_t)DUMMY_STREAM_ID);
		if (err)
			goto error;

		rtmp_amf_data(ep->conn, DUMMY_STREAM_ID,
				 "|RtmpSampleAccess",
				 2,
				   RTMP_AMF_TYPE_BOOLEAN, false,
				   RTMP_AMF_TYPE_BOOLEAN, false);


		/* Send some dummy media packets to client */

		for (i=0; i<NUM_MEDIA_PACKETS; i++) {

			err = rtmp_send_audio(ep->stream, i,
					      fake_audio_packet,
					      sizeof(fake_audio_packet));
			if (err)
				goto error;

			err = rtmp_send_video(ep->stream, TS_OFFSET + i,
					      fake_video_packet,
					      sizeof(fake_video_packet));
			if (err)
				goto error;
		}
	}
	else if (0 == str_casecmp(name, "publish")) {

		struct rtmp_endpoint *ep_cli = ep->other;
		const char *stream_name;
		uint64_t tid;
		uint32_t i;

		++ep->n_publish;

		if (!rtmp_amf_message_get_number(msg, &tid, 1)) {
			err = EPROTO;
			goto out;
		}
		TEST_EQUALS(0, tid);

		stream_name = rtmp_amf_message_string(msg, 3);
		TEST_STRCMP(fake_stream_name, strlen(fake_stream_name),
			    stream_name, str_len(stream_name));


		/* Stream Begin */
		err = rtmp_control(ep->conn, RTMP_TYPE_USER_CONTROL_MSG,
				   RTMP_EVENT_STREAM_BEGIN,
				   (uint32_t)DUMMY_STREAM_ID);
		if (err)
			goto error;

		/* Send some dummy media packets to server */
		for (i=0; i<NUM_MEDIA_PACKETS; i++) {

			err = rtmp_send_audio(ep_cli->stream, i,
					      fake_audio_packet,
					      sizeof(fake_audio_packet));
			if (err)
				goto error;

			err = rtmp_send_video(ep_cli->stream, TS_OFFSET + i,
					      fake_video_packet,
					      sizeof(fake_video_packet));
			if (err)
				goto error;
		}
	}
	else if (0 == str_casecmp(name, "deleteStream")) {

		struct rtmp_stream *strm;
		uint64_t stream_id;

		++ep->n_deletestream;

		if (!rtmp_amf_message_get_number(msg, &stream_id, 3)) {
			err = EPROTO;
			goto out;
		}

		TEST_EQUALS(DUMMY_STREAM_ID, stream_id);

		strm = rtmp_stream_find(ep->conn, (uint32_t)stream_id);
		TEST_ASSERT(strm != NULL);

		ep->stream = mem_deref(ep->stream);

		/* re_main will be stopped when the
		 * TCP connection is closed
		 */
	}
	else {
		DEBUG_NOTICE("rtmp: server: command not handled (%s)\n",
			     name);
		err = ENOTSUP;
		goto error;
	}

	return;

 out:
 error:
	if (err)
		endpoint_terminate(ep, err);
}


static void close_handler(int err, void *arg)
{
	struct rtmp_endpoint *ep = arg;

	DEBUG_NOTICE("[ %s ] rtmp connection closed (%m)\n", ep->tag, err);

	++ep->n_close;

	endpoint_terminate(ep, err);
}


static void endpoint_destructor(void *data)
{
	struct rtmp_endpoint *ep = data;

	mem_deref(ep->conn);
	mem_deref(ep->ts);
}


static struct rtmp_endpoint *rtmp_endpoint_alloc(enum mode mode,
						 bool is_client)
{
	struct rtmp_endpoint *ep;

	ep = mem_zalloc(sizeof(*ep), endpoint_destructor);
	if (!ep)
		return NULL;

	ep->is_client = is_client;
	ep->mode = mode;

	ep->tag = is_client ? "Client" : "Server";

	return ep;
}


static void apply_fuzzing(struct rtmp_endpoint *ep, struct mbuf *mb)
{
	const size_t len = mbuf_get_left(mb);
	size_t pos;
	bool flip;
	unsigned bit;

	++ep->packet_count;

	pos = rand_u16() % len;
	bit = rand_u16() % 8;

	/* percent change of corrupt packet */
	flip = ((rand_u16() % 100) < 33);

	if (flip) {
		/* flip a random bit */
		mbuf_buf(mb)[pos] ^= 1<<bit;
	}
}


static bool helper_send_handler(int *err, struct mbuf *mb, void *arg)
{
	struct rtmp_endpoint *ep = arg;
	(void)err;

	apply_fuzzing(ep, mb);

	return false;
}


static bool helper_recv_handler(int *err, struct mbuf *mb, bool *estab,
				void *arg)
{
	struct rtmp_endpoint *ep = arg;
	(void)err;
	(void)estab;

	apply_fuzzing(ep, mb);

	return false;
}


static void tcp_conn_handler(const struct sa *peer, void *arg)
{
	struct rtmp_endpoint *ep = arg;
	int err;
	(void)peer;

	err = rtmp_accept(&ep->conn, ep->ts, estab_handler,
			  command_handler, close_handler, ep);
	if (err)
		goto error;

	/* Enable fuzzing on the server */
	if (ep->fuzzing) {
		err = tcp_register_helper(&ep->th, rtmp_conn_tcpconn(ep->conn),
					  -1000,
					  0, helper_send_handler,
					  helper_recv_handler, ep);
		if (err)
			goto error;
	}

	return;

 error:
	if (err)
		endpoint_terminate(ep, err);
}


static int test_rtmp_client_server_conn(enum mode mode, bool fuzzing)
{
	struct rtmp_endpoint *cli, *srv;
	struct sa srv_addr;
	char uri[256];
	int err = 0;

	cli = rtmp_endpoint_alloc(mode, true);
	srv = rtmp_endpoint_alloc(mode, false);
	if (!cli || !srv) {
		err = ENOMEM;
		goto out;
	}

	cli->fuzzing = fuzzing;
	srv->fuzzing = fuzzing;

	cli->other = srv;
	srv->other = cli;

	err = sa_set_str(&srv_addr, "127.0.0.1", 0);
	TEST_ERR(err);

	err = tcp_listen(&srv->ts, &srv_addr, tcp_conn_handler, srv);
	TEST_ERR(err);

	err = tcp_local_get(srv->ts, &srv_addr);
	TEST_ERR(err);

	re_snprintf(uri, sizeof(uri), "rtmp://%J/vod/foo", &srv_addr);

	err = rtmp_connect(&cli->conn, NULL, uri, estab_handler,
			   command_handler, close_handler, cli);
	if (err)
		goto out;

	err = re_main_timeout(1000);
	if (err)
		goto out;

	re_printf("main loop done.\n");

	if (cli->err) {
		err = cli->err;
		goto out;
	}
	if (srv->err) {
		err = srv->err;
		goto out;
	}

	TEST_EQUALS(1, cli->n_estab);
	/*TEST_EQUALS(1, srv->n_estab);*/
	TEST_EQUALS(0, cli->n_cmd);
	TEST_EQUALS(4, srv->n_cmd);
	TEST_EQUALS(0, cli->n_close);
	TEST_EQUALS(1, srv->n_close);

	TEST_EQUALS(1, cli->n_ready);
	TEST_EQUALS(0, srv->n_ready);
	TEST_EQUALS(0, cli->n_deletestream);
	TEST_EQUALS(1, srv->n_deletestream);
	TEST_EQUALS(1, cli->n_begin);
	TEST_EQUALS(0, srv->n_begin);

	switch (mode) {

	case MODE_PLAY:
		TEST_EQUALS(1, srv->n_play);
		TEST_EQUALS(0, srv->n_publish);

		TEST_EQUALS(5, cli->n_audio);
		TEST_EQUALS(5, cli->n_video);
		TEST_EQUALS(0, srv->n_audio);
		TEST_EQUALS(0, srv->n_video);
		break;

	case MODE_PUBLISH:
		TEST_EQUALS(0, srv->n_play);
		TEST_EQUALS(1, srv->n_publish);

		TEST_EQUALS(0, cli->n_audio);
		TEST_EQUALS(0, cli->n_video);
		TEST_EQUALS(5, srv->n_audio);
		TEST_EQUALS(5, srv->n_video);
		break;
	}

 out:
	mem_deref(srv);
	mem_deref(cli);

	return err;
}


int test_rtmp(void)
{
	int err = 0;

	err = test_rtmp_client_server_conn(MODE_PLAY, false);
	if (err)
		return err;

	err = test_rtmp_client_server_conn(MODE_PUBLISH, false);
	if (err)
		return err;

	return err;
}


int test_rtmp_fuzzing(void)
{
	int err = 0, e;
	int i;

	for (i=0; i<32; i++) {

		/* Client/Server loop */
		e = test_rtmp_client_server_conn(MODE_PLAY, true);

		(void)e;  /* ignore result */
	}

	return err;
}
