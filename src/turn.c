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
	struct udp_sock *us1;
	struct udp_sock *us2;
	struct turnc *turnc;
	struct sa cli;
	struct tmr tmr;
	int err;
};


static void destructor(void *arg)
{
	struct turntest *tt = arg;

	tmr_cancel(&tt->tmr);

	mem_deref(tt->us2);
	mem_deref(tt->us1);
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
static void udp_recv(const struct sa *src, struct mbuf *mb, void *arg)
{
	struct turntest *tt = arg;
	struct stun_msg *msg = NULL;
	int err;

	err = stun_msg_decode(&msg, mb, NULL);
	if (err) {
		complete_test(tt, err);
		return;
	}

	if (STUN_METHOD_ALLOCATE != stun_msg_method(msg)) {
		err = EPROTO;
		goto out;
	}

	err = stun_reply(IPPROTO_UDP, tt->us2, src, 0, msg, NULL, 0, false, 2,
			 STUN_ATTR_XOR_MAPPED_ADDR, src,
			 STUN_ATTR_XOR_RELAY_ADDR, src);
	if (err)
		goto out;

 out:
	mem_deref(msg);

	if (err)
		complete_test(tt, err);
}


static void turnc_handler(int err, uint16_t scode, const char *reason,
			  const struct sa *relay_addr,
			  const struct sa *mapped_addr,
			  void *arg)
{
	struct turntest *tt = arg;

	(void)reason;

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

	complete_test(tt, err);
}


static int turntest_alloc(struct turntest **ttp)
{
	struct turntest *tt;
	struct sa srv;
	int err;

	tt = mem_zalloc(sizeof(*tt), destructor);
	if (!tt)
		return ENOMEM;

	err = sa_set_str(&srv, "127.0.0.1", 0);

	err |= udp_listen(&tt->us1, &srv, NULL, NULL);
	err |= udp_listen(&tt->us2, &srv, udp_recv, tt);
	if (err)
		goto out;

	err  = udp_local_get(tt->us1, &tt->cli);
	err |= udp_local_get(tt->us2, &srv);
	if (err)
		goto out;

	err = turnc_alloc(&tt->turnc, NULL, IPPROTO_UDP, tt->us1, 0, &srv,
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
