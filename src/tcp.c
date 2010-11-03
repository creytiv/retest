/**
 * @file tcp.c  TCP testcode
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include <string.h>
#include <re.h>
#include "test.h"


#define DEBUG_MODULE "tcptest"
#define DEBUG_LEVEL 5
#include <re_dbg.h>


/*
 *               .------.                            .------.
 *               |Client|                            |Server|
 *               '------'                            '------'
 *
 *                                                       <------ tcp_listen()
 * tcp_connect() --->
 *                   ------------- TCP [SYN] ----------->
 *                                                       -----> tcp_conn_h
 *
 *                                                       <----- tcp_accept()
 *                   <-------- TCP [SYN, ACK] ----------
 *                   --------- TCP [ACK] -------------->
 * tcp_estab_h   <---
 *
 * tcp_send()    ===>
 *                    ======= TCP [PSH, ACK] ==========>
 *                   <======= TCP [ACK] ===============
 *                                                       =====> tcp_recv_h
 */

static struct tmr tmr;
static const struct pl ping = PL("ping from client to server\n");
static const struct pl pong = PL("pong from server 2 client\n");
static int tcp_err = 0;
static struct tcp_conn *tc2 = NULL;


static void abort_test(int err)
{
	DEBUG_INFO("Abort Test (%s)\n", strerror(err));
	tcp_err = err;
	tmr_cancel(&tmr);
	re_cancel();
}


static int send_pl(struct tcp_conn *tc, const struct pl *data)
{
	struct mbuf mb;
	int err;

	mbuf_init(&mb);

	err = mbuf_write_pl(&mb, data);
	if (err)
		goto out;

	mb.pos = 0;
	err = tcp_send(tc, &mb);
	if (err)
		goto out;

 out:
	mbuf_reset(&mb);
	return err;
}


static void tcp_server_recv_handler(struct mbuf *mb, void *arg)
{
	int err;

	(void)arg;

	DEBUG_INFO("Server: TCP Receive data (%u bytes)\n",
		   mbuf_get_left(mb));

	if (!mb)
		err = ENOMEM;
	else if (mb->end != ping.l)
		err = EINVAL;
	else if (0 != memcmp(mb->buf, ping.p, mb->end))
		err = EINVAL;
	else
		err = 0;

	if (err)
		abort_test(err);

	err = send_pl(tc2, &pong);
	if (err)
		abort_test(err);
}


static void tcp_server_close_handler(int err, void *arg)
{
	(void)arg;
	DEBUG_INFO("Server: TCP Close (%s)\n", strerror(err));
	abort_test(err);
}


static void tcp_server_conn_handler(const struct sa *peer, void *arg)
{
	struct tcp_sock **ts = arg;
	int err;

	(void)peer;
	(void)arg;

	DEBUG_INFO("Server: Incoming CONNECT from %J\n", peer);

	err = tcp_accept(&tc2, *ts, NULL, tcp_server_recv_handler,
			 tcp_server_close_handler, NULL);
	if (err) {
		abort_test(err);
	}
}


static void tcp_client_estab_handler(void *arg)
{
	struct tcp_conn **tc = arg;
	int err;

	DEBUG_INFO("Client: TCP Established\n");

	err = send_pl(*tc, &ping);
	if (err)
		abort_test(err);
}


static void tcp_client_recv_handler(struct mbuf *mb, void *arg)
{
	int err;

	(void)arg;

	DEBUG_INFO("Client: TCP receive: %u bytes\n", mbuf_get_left(mb));

	if (!mb)
		err = ENOMEM;
	else if (mb->end != pong.l)
		err = EINVAL;
	else if (0 != memcmp(mb->buf, pong.p, mb->end))
		err = EINVAL;
	else
		err = 0;

	abort_test(err);
}


static void tcp_client_close_handler(int err, void *arg)
{
	(void)arg;
	DEBUG_NOTICE("Client: TCP Close (%s)\n", strerror(err));

	abort_test(err);
}


static void timeout_handler(void *arg)
{
	(void)arg;
	abort_test(ENOMEM);
}


int test_tcp(void)
{
	struct tcp_sock *ts = NULL;
	struct tcp_conn *tc = NULL;
	struct sa srv;
	int err;

	tcp_err = 0;
	tmr_init(&tmr);

	err = sa_set_str(&srv, "127.0.0.1", 0);
	if (err)
		goto out;

	tmr_start(&tmr, 1000, timeout_handler, NULL);

	err = tcp_listen(&ts, &srv, tcp_server_conn_handler, &ts);
	if (err)
		goto out;

	err = tcp_local_get(ts, &srv);
	if (err)
		goto out;

	DEBUG_INFO("TCP Server listening on %J\n", &srv);

	err = tcp_connect(&tc, &srv, tcp_client_estab_handler,
			  tcp_client_recv_handler, tcp_client_close_handler,
			  &tc);
	if (err)
		goto out;

	err = re_main(NULL);
	if (err)
		goto out;

	if (tcp_err)
		err = tcp_err;

	DEBUG_INFO("Test done (%s)\n", strerror(err));

 out:
	tmr_cancel(&tmr);
	tc = mem_deref(tc);
	tc2 = mem_deref(tc2);
	ts = mem_deref(ts);

	return err;
}
