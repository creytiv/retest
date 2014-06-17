/**
 * @file turn.c  TURN testcode
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include <string.h>
#include <re.h>
#include "test.h"


#define DEBUG_MODULE "test_turn"
#define DEBUG_LEVEL 5
#include <re_dbg.h>


struct turntest {
	struct udp_sock *us_cli;
	struct udp_sock *us_srv;
	struct turnc *turnc;
	struct sa cli;
	struct sa srv;
	struct tmr tmr;
	int err;
};


static const char *payload = "guten tag Herr TURN server";


static void destructor(void *arg)
{
	struct turntest *tt = arg;

	tmr_cancel(&tt->tmr);

	mem_deref(tt->us_srv);
	mem_deref(tt->us_cli);
	mem_deref(tt->turnc);
}


static void complete_test(struct turntest *tt, int err)
{
	tt->err = err;
	re_cancel();
}


static void timeout_handler(void *arg)
{
	complete_test(arg, ENOMEM);
}


/* Simulated TURN server */
static void srv_udp_recv(const struct sa *src, struct mbuf *mb, void *arg)
{
	struct turntest *tt = arg;
	struct stun_msg *msg = NULL;
	int err;

	err = stun_msg_decode(&msg, mb, NULL);
	if (err) {
		complete_test(tt, err);
		return;
	}

	switch (stun_msg_method(msg)) {

	case STUN_METHOD_ALLOCATE:
		err = stun_reply(IPPROTO_UDP, tt->us_srv, src, 0,
				 msg, NULL, 0, false,
				 2,
				 STUN_ATTR_XOR_MAPPED_ADDR, src,
				 STUN_ATTR_XOR_RELAY_ADDR, src);

		break;

	case STUN_METHOD_SEND: {
		struct stun_attr *attr;

		attr = stun_msg_attr(msg, STUN_ATTR_DATA);
		TEST_ASSERT(attr != NULL);

		TEST_MEMCMP(payload, strlen(payload),
			    mbuf_buf(&attr->v.data),
			    mbuf_get_left(&attr->v.data));

		/* Stop the test now */
		complete_test(tt, 0);
	}
		break;

	default:
		DEBUG_WARNING("unknown method %d\n", stun_msg_method(msg));
		err = EPROTO;
		break;
	}

	if (err)
		goto out;

 out:
	mem_deref(msg);

	if (err)
		complete_test(tt, err);
}


static int send_payload(struct turntest *tt, const char *str)
{
	struct mbuf *mb = mbuf_alloc(512);
	int err;

	mb->pos = 36;

	err = mbuf_write_str(mb, str);

	mb->pos = 36;

	if (!err)
		err = udp_send(tt->us_cli, &tt->srv, mb);

	mem_deref(mb);

	return err;
}


static void turnc_handler(int err, uint16_t scode, const char *reason,
			  const struct sa *relay_addr,
			  const struct sa *mapped_addr,
			  const struct stun_msg *msg,
			  void *arg)
{
	struct turntest *tt = arg;

	(void)reason;
	(void)msg;

	if (err) {
		complete_test(tt, err);
		return;
	}

	if (scode) {
		complete_test(tt, EPROTO);
		return;
	}

	if (!sa_cmp(relay_addr, &tt->cli, SA_ALL))
		err = EPROTO;
	if (!sa_cmp(mapped_addr, &tt->cli, SA_ALL))
		err = EPROTO;

	if (err) {
		complete_test(tt, err);
		return;
	}

	err = send_payload(tt, payload);
	if (err) {
		DEBUG_WARNING("failed to send payload (%m)\n", err);
		complete_test(tt, err);
	}
}


static int turntest_alloc(struct turntest **ttp)
{
	struct turntest *tt;
	struct sa laddr;
	int err;

	tt = mem_zalloc(sizeof(*tt), destructor);
	if (!tt)
		return ENOMEM;

	err = sa_set_str(&laddr, "127.0.0.1", 0);

	err |= udp_listen(&tt->us_cli, &laddr, NULL, NULL);
	err |= udp_listen(&tt->us_srv, &laddr, srv_udp_recv, tt);
	if (err)
		goto out;

	err  = udp_local_get(tt->us_cli, &tt->cli);
	err |= udp_local_get(tt->us_srv, &tt->srv);
	if (err)
		goto out;

	err = turnc_alloc(&tt->turnc, NULL, IPPROTO_UDP, tt->us_cli,
			  0, &tt->srv,
			  "username", "password", 600, turnc_handler, tt);
	if (err)
		goto out;

	/* OOM helper */
	tmr_start(&tt->tmr, 100, timeout_handler, tt);

 out:
	if (err)
		mem_deref(tt);
	else if (ttp)
		*ttp = tt;

	return err;
}


int test_turn(void)
{
	struct turntest *tt;
	int err;

	err = turntest_alloc(&tt);
	if (err)
		return err;

	(void)re_main(NULL);

	err = tt->err;

	mem_deref(tt);

	return err;
}
