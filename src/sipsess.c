/**
 * @file sipsess.c SIP Session regression testcode
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include <string.h>
#include <re.h>
#include "test.h"


static struct sip *sip = NULL;
static struct sipsess_sock *sock = NULL;
static struct sipsess *sess_a = NULL;
static struct sipsess *sess_b = NULL;
static struct tmr tmr;
static int gerr;


static void stop_test(void)
{
	re_cancel();
}


static void abort_test(int err)
{
	gerr = err;
	re_cancel();
}


static void exit_handler(void *arg)
{
	(void)arg;
	re_cancel();
}


static void signal_handler(int ret)
{
	(void)re_printf("signal %d\n", ret);

	abort_test(ECONNABORTED);
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


static void estab_handler(const struct sip_msg *msg, void *arg)
{
	const char *user = arg;

	(void)msg;

	if (str_casecmp(user, "A"))
		sess_a = mem_deref(sess_a);
	else
		sess_b = mem_deref(sess_b);
}


static void close_handler(int err, const struct sip_msg *msg, void *arg)
{
	(void)err;
	(void)msg;
	(void)arg;

	stop_test();
}


static void conn_handler(const struct sip_msg *msg, void *arg)
{
	int err;

	(void)arg;

	err = sipsess_accept(&sess_b, sock, msg, 200, "OK",
			     "b", "application/sdp", NULL, NULL, NULL, false,
			     offer_handler, answer_handler, estab_handler,
			     NULL, NULL, close_handler, (void *)"B", NULL);
	if (err) {
		abort_test(err);
	}
}


static void timeout_handler(void *arg)
{
	(void)arg;
	abort_test(ENOMEM);
}


int test_sipsess(void)
{
	struct sa laddr;
	char to_uri[256];
	int i, err;
	uint16_t port;

	gerr = 0;

	/* slurp warnings from SIP (todo: temp) */
	(void)freopen("/dev/null", "w", stderr);

	err = sip_alloc(&sip, NULL, 32, 32, 32, "retest", exit_handler, NULL);
	if (err)
		goto out;

	for (i=0; i<64; i++) {
		port = 1024 + rand_u16() % 64512;
		(void)sa_set_str(&laddr, "127.0.0.1", port);
		err = sip_transp_add(sip, SIP_TRANSP_UDP, &laddr);
		if (!err)
			break;
	}
	if (err)
		goto out;

	err = sipsess_listen(&sock, sip, 32, conn_handler, NULL);
	if (err)
		goto out;

	/* Connect to "b" */
	(void)re_snprintf(to_uri, sizeof(to_uri), "sip:b@127.0.0.1:%u", port);
	err = sipsess_connect(&sess_a, sock, to_uri, NULL,
			      "sip:a@127.0.0.1", "a", NULL, 0,
			      "application/sdp", NULL, NULL, NULL, false,
			      offer_handler, answer_handler, NULL,
			      estab_handler, NULL, NULL,
			      close_handler, (void *)"A", NULL);
	if (err)
		goto out;

	/* OOM helper */
	tmr_start(&tmr, 100, timeout_handler, NULL);

	(void)re_main(signal_handler);

	if (gerr)
		err = gerr;

 out:
	tmr_cancel(&tmr);

	sess_a = mem_deref(sess_a);
	sess_b = mem_deref(sess_b);

	sipsess_close_all(sock);
	sock = mem_deref(sock);

	sip_close(sip, false);
	sip = mem_deref(sip);

	/* Restore stderr */
	freopen("/dev/tty", "w", stderr);

	return err;
}
