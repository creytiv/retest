/**
 * @file tls.c  TLS testcode
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include <string.h>
#include <re.h>
#include "test.h"


struct tls_test {
	struct tls *tls;
	struct tls_conn *sc;
	struct tcp_sock *ts;
	struct tcp_conn *tc_cli;
	struct tcp_conn *tc_srv;
	struct tmr tmr;
	bool estab_cli;
	bool estab_srv;
	bool recv_srv;
	int err;
};


static const uint8_t tls_handshake[] =
	"\x16\x03\x01\x01";


static void signal_handler(int sig)
{
	(void)sig;
	re_cancel();
}


static void check(struct tls_test *tt, int err)
{
	if (tt->err == 0)
		tt->err = err;

	if (tt->err)
		re_cancel();
}


static void client_estab_handler(void *arg)
{
	struct tls_test *tt = arg;
	tt->estab_cli = true;
}


static void client_recv_handler(struct mbuf *mb, void *arg)
{
	struct tls_test *tt = arg;

	if (!tt->estab_cli) {
		(void)re_fprintf(stderr, "unexpected data received"
				 " on client [%02w]\n",
				 mbuf_buf(mb), mbuf_get_left(mb));
		check(tt, EPROTO);
	}
}


static void client_close_handler(int err, void *arg)
{
	struct tls_test *tt = arg;
	check(tt, err);
}


static void server_estab_handler(void *arg)
{
	struct tls_test *tt = arg;
	tt->estab_srv = true;
}


static void server_recv_handler(struct mbuf *mb, void *arg)
{
	struct tls_test *tt = arg;
	const size_t n = sizeof(tls_handshake) - 1;

	if (!tt->estab_srv) {
		check(tt, EPROTO);
		return;
	}

	if (mbuf_get_left(mb) < n) {
		(void)re_fprintf(stderr, "server received short"
				 " packet: %u < %u\n",
				 mbuf_get_left(mb), n);
		check(tt, EBADMSG);
		return;
	}

	if (0 != memcmp(mbuf_buf(mb), tls_handshake, n)) {
		(void)re_fprintf(stderr, "TLS handshake mismatch\n");
		(void)re_fprintf(stderr, "ref:  %02w\n", tls_handshake, n);
		(void)re_fprintf(stderr, "recv: %02w\n", mbuf_buf(mb), n);

		check(tt, EBADMSG);
		return;
	}

	tt->recv_srv = true;

	/* We are done */
	re_cancel();
}


static void server_close_handler(int err, void *arg)
{
	struct tls_test *tt = arg;
	check(tt, err);
}


static void server_conn_handler(const struct sa *peer, void *arg)
{
	struct tls_test *tt = arg;
	(void)peer;

	tt->err = tcp_accept(&tt->tc_srv, tt->ts, server_estab_handler,
			     server_recv_handler, server_close_handler, tt);
	check(tt, tt->err);
}


static void tmr_handler(void *arg)
{
	struct tls_test *tt = arg;
	check(tt, ENOMEM);
}


int test_tls(void)
{
	struct tls_test tt;
	struct sa srv;
	int err;

	memset(&tt, 0, sizeof(tt));

	err = sa_set_str(&srv, "127.0.0.1", 0);
	if (err)
		goto out;

	err = tls_alloc(&tt.tls, TLS_METHOD_SSLV23, NULL, NULL);
	if (err)
		goto out;

	err = tcp_listen(&tt.ts, &srv, server_conn_handler, &tt);
	if (err)
		goto out;

	err = tcp_sock_local_get(tt.ts, &srv);
	if (err)
		goto out;

	err = tcp_connect(&tt.tc_cli, &srv, client_estab_handler,
			  client_recv_handler, client_close_handler, &tt);
	if (err)
		goto out;

	err = tls_start_tcp(&tt.sc, tt.tls, tt.tc_cli, 0);
	if (err)
		goto out;

	tmr_start(&tt.tmr, 100, tmr_handler, &tt);

	(void)re_main(signal_handler);

	if (tt.err)
		goto out;

	if (!tt.estab_srv || !tt.recv_srv)
		err = EPROTO;

 out:
	tmr_cancel(&tt.tmr);
	mem_deref(tt.sc);
	mem_deref(tt.tc_cli);
	mem_deref(tt.tc_srv);
	mem_deref(tt.ts);
	mem_deref(tt.tls);

	return err | tt.err;
}
