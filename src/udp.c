/**
 * @file udp.c  UDP testcode
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include <string.h>
#include <re.h>
#include "test.h"


#define DEBUG_MODULE "udptest"
#define DEBUG_LEVEL 5
#include <re_dbg.h>


static char dummy_arg = 'a';
static const struct pl data = PL("test data");
static const struct pl data2 = PL("data2mplajksdjqooz ka;osdkalsjdlkj");
static struct tmr tmr;
static struct sa cli, srv;
static struct udp_sock *usc, *uss, *uss2;
static struct mbuf mbs;
static int udp_err = 0;
static int tindex;


static void udp_recv_handler(const struct sa *src, struct mbuf *mb,
			     void *arg)
{
	if (&dummy_arg != arg)
		udp_err = EINVAL;

	switch (tindex++) {
	case 0:
		if (mb->end != data.l)
			udp_err = EBADMSG;

		if (0 != memcmp(mb->buf, data.p, mb->end))
			udp_err = EBADMSG;
		if (udp_err)
			break;

		if (!sa_cmp(src, &cli, SA_ALL)) {
			udp_err = EINVAL;
			break;
		}

		/* Send from no UDP socket */
		mbuf_reset(&mbs);
		udp_err = mbuf_write_pl(&mbs, &data2);
		if (udp_err)
			break;

		mbs.pos = 0;
		udp_err = udp_send_anon(&srv, &mbs);
		break;

	case 1:
		if (mb->end != data2.l)
			udp_err = EBADMSG;

		if (0 != memcmp(mb->buf, data2.p, mb->end))
			udp_err = EBADMSG;
		break;

	default:
		udp_err = EINVAL;
		break;
	}

	if (tindex >= 2)
		re_cancel();
}


static void timeout_handler(void *arg)
{
	(void)arg;
	udp_err = ENOMEM;
	re_cancel();
}


int test_udp(void)
{
	int err;

	udp_err = 0;
	mbuf_init(&mbs);
	tmr_init(&tmr);

	err = sa_set_str(&cli, "127.0.0.1", 0);
	if (err)
		goto out;
	err = sa_set_str(&srv, "127.0.0.1", 0);
	if (err)
		goto out;

	err = udp_listen(&usc, &cli, udp_recv_handler, &dummy_arg);
	if (err)
		goto out;

	err = udp_listen(&uss, &srv, udp_recv_handler, &dummy_arg);
	if (err)
		goto out;

	err = udp_local_get(usc, &cli);
	if (err)
		goto out;
	err = udp_local_get(uss, &srv);
	if (err)
		goto out;

	/* expect failure */
	if (!udp_listen(&uss2, &srv, udp_recv_handler, &dummy_arg)) {
		err = EINVAL;
		goto out;
	}

	err = mbuf_write_pl(&mbs, &data);
	if (err)
		goto out;

	tindex = 0;

	/* Send from connected client UDP socket */
	udp_connect(usc, true);

	mbs.pos = 0;
	err = udp_send(usc, &srv, &mbs);
	if (err)
		goto out;

	tmr_start(&tmr, 100, timeout_handler, NULL);

	err = re_main(NULL);
	if (err)
		goto out;

	if (udp_err)
		err = udp_err;

 out:
	mbuf_reset(&mbs);
	tmr_cancel(&tmr);
	usc = mem_deref(usc);
	uss = mem_deref(uss);
	uss2 = mem_deref(uss2);

	return err;
}
