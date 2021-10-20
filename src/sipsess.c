/**
 * @file sipsess.c SIP Session regression testcode
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include <string.h>
#include <re.h>
#include "test.h"


#define DEBUG_MODULE "test_sipsess"
#define DEBUG_LEVEL 5
#include <re_dbg.h>


struct test {
	struct sip *sip;
	struct sipsess_sock *sock;
	struct sipsess *a;
	struct sipsess *b;
	bool estab_a;
	bool estab_b;
	bool blind_transfer;
	uint16_t altaddr_port;
	int err;
};


static void stop_test(void)
{
	re_cancel();
}


static void abort_test(struct test *test, int err)
{
	test->err = err;
	re_cancel();
}


static void exit_handler(void *arg)
{
	(void)arg;
	re_cancel();
}


static int offer_handler(struct mbuf **descp, const struct sip_msg *msg,
			 void *arg)
{
	(void)descp;
	(void)msg;
	(void)arg;
	return 0;
}


static int answer_handler(const struct sip_msg *msg, void *arg)
{
	(void)msg;
	(void)arg;
	return 0;
}


static void estab_handler_a(const struct sip_msg *msg, void *arg)
{
	struct test *test = arg;

	(void)msg;

	test->estab_a = true;

	if (test->estab_b)
		stop_test();
}


static void estab_handler_b(const struct sip_msg *msg, void *arg)
{
	struct test *test = arg;

	(void)msg;

	test->estab_b = true;

	if (test->estab_a)
		stop_test();
}


static void close_handler(int err, const struct sip_msg *msg, void *arg)
{
	struct test *test = arg;

	(void)err;
	(void)msg;
	(void)arg;

	abort_test(test, err ? err : ENOMEM);
}


static void conn_handler(const struct sip_msg *msg, void *arg)
{
	struct test *test = arg;
	int err;

	(void)arg;

	err = sipsess_accept(&test->b, test->sock, msg, 200, "OK",
			     "b", "application/sdp", NULL, NULL, NULL, false,
			     offer_handler, answer_handler, estab_handler_b,
			     NULL, NULL, close_handler, test, NULL);
	if (err) {
		abort_test(test, err);
	}
}


static void conn_transfer_handler(const struct sip_msg *msg, void *arg)
{
	struct test *test = arg;
	int err = 0;

	if (test->blind_transfer) {
		conn_handler(msg, arg);
	}
	else {
		err = sip_replyf(test->sip, msg, 302, "Moved Temporarily",
			"Contact: \"alt retest\" "
			"<sip:127.0.0.1:%u>\r\n\r\n", test->altaddr_port);
		if (err) {
			abort_test(test, err);
		}
	}

	return;
}


static void redirect_handler(const struct sip_msg *msg, const char *uri,
	void *arg)
{
	struct test *test = arg;

	(void) msg;
	(void) uri;

	test->blind_transfer = true;
}


int test_sipsess(void)
{
	struct test test;
	struct sa laddr;
	char to_uri[256];
	int err;
	uint16_t port;
	char *callid;

	memset(&test, 0, sizeof(test));

#ifndef WIN32
	/* slurp warnings from SIP (todo: temp) */
	(void)freopen("/dev/null", "w", stderr);
#endif

	err = sip_alloc(&test.sip, NULL, 32, 32, 32,
			"retest", exit_handler, NULL);
	if (err)
		goto out;

	(void)sa_set_str(&laddr, "127.0.0.1", 0);
	err = sip_transp_add(test.sip, SIP_TRANSP_UDP, &laddr);
	if (err)
		goto out;

	err = sip_transp_laddr(test.sip, &laddr, SIP_TRANSP_UDP, NULL);
	if (err)
		goto out;

	port = sa_port(&laddr);

	err = sipsess_listen(&test.sock, test.sip, 32, conn_handler, &test);
	if (err)
		goto out;

	err = str_x64dup(&callid, rand_u64());
	if (err)
		goto out;

	/* Connect to "b" */
	(void)re_snprintf(to_uri, sizeof(to_uri), "sip:b@127.0.0.1:%u", port);
	err = sipsess_connect(&test.a, test.sock, to_uri, NULL,
			      "sip:a@127.0.0.1", "a", NULL, 0,
			      "application/sdp", NULL, NULL, NULL, NULL, false,
			      callid,
			      offer_handler, answer_handler, NULL,
			      estab_handler_a, NULL, NULL,
			      close_handler, &test, NULL);
	mem_deref(callid);
	if (err)
		goto out;

	err = re_main_timeout(200);
	if (err)
		goto out;

	if (test.err) {
		err = test.err;
		goto out;
	}

	/* okay here -- verify */
	TEST_ASSERT(test.estab_a);
	TEST_ASSERT(test.estab_b);

 out:
	test.a = mem_deref(test.a);
	test.b = mem_deref(test.b);

	sipsess_close_all(test.sock);
	test.sock = mem_deref(test.sock);

	sip_close(test.sip, false);
	test.sip = mem_deref(test.sip);

#ifndef WIN32
	/* Restore stderr */
	freopen("/dev/tty", "w", stderr);
#endif

	return err;
}


int test_sipsess_blind_transfer(void)
{
	struct test test;
	struct sa laddr, altaddr;
	char to_uri[256];
	int err;
	uint16_t port;
	char *callid;

	memset(&test, 0, sizeof(test));

	err = sip_alloc(&test.sip, NULL, 32, 32, 32,
			"retest", exit_handler, NULL);
	TEST_ERR(err);

	(void)sa_set_str(&laddr, "127.0.0.1", 0);
	err = sip_transp_add(test.sip, SIP_TRANSP_UDP, &laddr);
	TEST_ERR(err);

	err = sip_transp_laddr(test.sip, &laddr, SIP_TRANSP_UDP, NULL);
	TEST_ERR(err);

	port = sa_port(&laddr);

	err = sipsess_listen(&test.sock, test.sip, 32, conn_transfer_handler,
		&test);
	TEST_ERR(err);

	(void)sa_set_str(&altaddr, "127.0.0.1", 0);
	err = sip_transp_add(test.sip, SIP_TRANSP_UDP, &altaddr);
	TEST_ERR(err);

	err = sip_transp_laddr(test.sip, &altaddr, SIP_TRANSP_UDP, NULL);
	TEST_ERR(err);

	test.altaddr_port = sa_port(&altaddr);

	err = str_x64dup(&callid, rand_u64());
	if (err)
		goto out;

	/* Connect to "b" */
	(void)re_snprintf(to_uri, sizeof(to_uri), "sip:b@127.0.0.1:%u", port);
	err = sipsess_connect(&test.a, test.sock, to_uri, NULL,
			      "sip:a@127.0.0.1", "a", NULL, 0,
			      "application/sdp", NULL, NULL, NULL, NULL, false,
			      callid,
			      offer_handler, answer_handler, NULL,
			      estab_handler_a, NULL, NULL,
			      close_handler, &test, NULL);
	mem_deref(callid);
	TEST_ERR(err);

	err = sipsess_set_redirect_handler(test.a, redirect_handler);
	TEST_ERR(err);

	err = re_main_timeout(200);
	TEST_ERR(err);

	if (test.err) {
		err = test.err;
		TEST_ERR(err);
	}

	/* okay here -- verify */
	TEST_ASSERT(test.blind_transfer);
	TEST_ASSERT(test.estab_a);
	TEST_ASSERT(test.estab_b);

 out:
	test.a = mem_deref(test.a);
	test.b = mem_deref(test.b);

	sipsess_close_all(test.sock);
	test.sock = mem_deref(test.sock);

	sip_close(test.sip, false);
	test.sip = mem_deref(test.sip);

	return err;
}
