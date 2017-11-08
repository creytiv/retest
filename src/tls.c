/**
 * @file tls.c  TLS testcode
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include <string.h>
#include <re.h>
#include "test.h"


#define DEBUG_MODULE "tlstest"
#define DEBUG_LEVEL 5
#include <re_dbg.h>


struct tls_test {
	struct tls *tls;
	struct tls_conn *sc_cli;
	struct tls_conn *sc_srv;
	struct tcp_sock *ts;
	struct tcp_conn *tc_cli;
	struct tcp_conn *tc_srv;
	enum tls_keytype keytype;
	bool estab_cli;
	bool estab_srv;
	size_t recv_cli;
	size_t recv_srv;
	int err;
};


static const char *payload = "0123456789";


static void check(struct tls_test *tt, int err)
{
	if (tt->err == 0)
		tt->err = err;

	if (tt->err)
		re_cancel();
}


static void can_send(struct tls_test *tt)
{
	struct mbuf *mb;
	int err = 0;

	if (!tt->estab_cli || !tt->estab_srv)
		return;

	mb = mbuf_alloc(256);
	if (!mb) {
		err = ENOMEM;
		goto out;
	}

	err = mbuf_write_str(mb, payload);
	if (err)
		goto out;

	mb->pos = 0;
	err = tcp_send(tt->tc_cli, mb);

 out:
	mem_deref(mb);

	check(tt, err);
}


static void client_estab_handler(void *arg)
{
	struct tls_test *tt = arg;
	const char *cipher = tls_cipher_name(tt->sc_cli);
	int err = 0;

	if (tt->keytype == TLS_KEYTYPE_RSA) {

		TEST_ASSERT(NULL == strstr(cipher, "ECDSA"));
	}
	else if (tt->keytype == TLS_KEYTYPE_EC) {

		if (NULL == strstr(cipher, "ECDH")) {
			DEBUG_WARNING("no ECDH in cipher (%s)\n", cipher);
			err = EPROTO;
			goto out;
		}
	}

	tt->estab_cli = true;
	can_send(tt);

 out:
	check(tt, err);
}


static void client_recv_handler(struct mbuf *mb, void *arg)
{
	struct tls_test *tt = arg;
	int err = 0;

	if (!tt->estab_cli) {
		(void)re_fprintf(stderr, "unexpected data received"
				 " on client [%02w]\n",
				 mbuf_buf(mb), mbuf_get_left(mb));
		check(tt, EPROTO);
	}

	++tt->recv_cli;

	TEST_MEMCMP(payload, strlen(payload),
		    mbuf_buf(mb), mbuf_get_left(mb));

 out:
	check(tt, err);

	/* We are done */
	re_cancel();
}


static void client_close_handler(int err, void *arg)
{
	struct tls_test *tt = arg;

	if (!tt->estab_cli)
		check(tt, err);
}


static void server_estab_handler(void *arg)
{
	struct tls_test *tt = arg;
	tt->estab_srv = true;
	can_send(tt);
}


static void server_recv_handler(struct mbuf *mb, void *arg)
{
	struct tls_test *tt = arg;
	int err = 0;

	if (!tt->estab_srv) {
		check(tt, EPROTO);
		return;
	}

	++tt->recv_srv;

	TEST_MEMCMP(payload, strlen(payload),
		    mbuf_buf(mb), mbuf_get_left(mb));

	/* echo */
	err = tcp_send(tt->tc_srv, mb);
	if (err) {
		DEBUG_WARNING("server: tcp_send error (%m)\n", err);
	}

 out:
	check(tt, err);
}


static void server_close_handler(int err, void *arg)
{
	struct tls_test *tt = arg;

	if (!tt->estab_cli)
		check(tt, err);
}


static void server_conn_handler(const struct sa *peer, void *arg)
{
	struct tls_test *tt = arg;
	int err;
	(void)peer;

	err = tcp_accept(&tt->tc_srv, tt->ts, server_estab_handler,
			 server_recv_handler, server_close_handler, tt);
	check(tt, err);

	err = tls_start_tcp(&tt->sc_srv, tt->tls, tt->tc_srv, 0);
	check(tt, err);
}


static int test_tls_base(enum tls_keytype keytype)
{
	struct tls_test tt;
	struct sa srv;
	int err;

	memset(&tt, 0, sizeof(tt));

	tt.keytype = keytype;

	err = sa_set_str(&srv, "127.0.0.1", 0);
	if (err)
		goto out;

	err = tls_alloc(&tt.tls, TLS_METHOD_SSLV23, NULL, NULL);
	if (err)
		goto out;

	switch (keytype) {

	case TLS_KEYTYPE_RSA:
		err = tls_set_certificate(tt.tls, test_certificate_rsa,
					  strlen(test_certificate_rsa));
		if (err)
			goto out;
		break;

	case TLS_KEYTYPE_EC:
		err = tls_set_certificate(tt.tls, test_certificate_ecdsa,
					  strlen(test_certificate_ecdsa));
		if (err)
			goto out;
		break;

	default:
		err = EINVAL;
		goto out;
	}

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

	err = tls_start_tcp(&tt.sc_cli, tt.tls, tt.tc_cli, 0);
	if (err)
		goto out;

	err = re_main_timeout(800);
	if (err)
		goto out;

	if (tt.err) {
		err = tt.err;
		goto out;
	}

	TEST_EQUALS(true, tt.estab_cli);
	TEST_EQUALS(true, tt.estab_srv);
	TEST_EQUALS(1, tt.recv_cli);
	TEST_EQUALS(1, tt.recv_srv);

 out:
	/* NOTE: close context first */
	mem_deref(tt.tls);

	mem_deref(tt.sc_cli);
	mem_deref(tt.sc_srv);
	mem_deref(tt.tc_cli);
	mem_deref(tt.tc_srv);
	mem_deref(tt.ts);

	return err;
}


int test_tls(void)
{
	return test_tls_base(TLS_KEYTYPE_RSA);
}


int test_tls_ec(void)
{
	return test_tls_base(TLS_KEYTYPE_EC);
}


/* XXX: replace with ECDSA certs */
int test_tls_selfsigned(void)
{
	struct tls *tls = NULL;
	uint8_t fp[20];
	int err;

	err = tls_alloc(&tls, TLS_METHOD_SSLV23, NULL, NULL);
	if (err)
		goto out;

	err = tls_set_selfsigned(tls, "re@test");
	TEST_ERR(err);

	/* verify fingerprint of the self-signed certificate */
	err = tls_fingerprint(tls, TLS_FINGERPRINT_SHA1, fp, sizeof(fp));
	TEST_ERR(err);

 out:
	mem_deref(tls);
	return err;
}


int test_tls_certificate(void)
{
	struct tls *tls = NULL;
	static const uint8_t test_fingerprint[20] =
		"\xD4\x86\x12\xB4\x28\x27\x5A\x74\x07\xCA"
		"\x09\x51\xA3\x1A\x79\x2A\x7E\x3C\xC3\x21";
	uint8_t fp[20];
	int err;

	err = tls_alloc(&tls, TLS_METHOD_SSLV23, NULL, NULL);
	if (err)
		goto out;

	err = tls_set_certificate(tls, test_certificate_rsa,
				  strlen(test_certificate_rsa));
	TEST_EQUALS(0, err);

	/* verify fingerprint of the certificate */
	err = tls_fingerprint(tls, TLS_FINGERPRINT_SHA1, fp, sizeof(fp));
	TEST_ERR(err);

	TEST_MEMCMP(test_fingerprint, sizeof(test_fingerprint),
		    fp, sizeof(fp));

 out:
	mem_deref(tls);
	return err;
}
