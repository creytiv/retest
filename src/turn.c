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
	struct turnc *turnc;
	struct turnserver *turnsrv;
	struct udp_sock *us_cli;
	struct udp_sock *us_peer;
	struct sa cli;
	struct sa peer;
	int err;

	size_t n_alloc_resp;
	size_t n_chan_resp;
	size_t n_peer_recv;
};


static const char *test_payload = "guten tag Herr TURN server";


static void destructor(void *arg)
{
	struct turntest *tt = arg;

	mem_deref(tt->us_cli);
	mem_deref(tt->us_peer);
	mem_deref(tt->turnc);
	mem_deref(tt->turnsrv);
}


static bool is_complete(struct turntest *tt)
{
	return tt->n_chan_resp >= 1 && tt->n_peer_recv >= 2;
}


static void complete_test(struct turntest *tt, int err)
{
	tt->err = err;
	re_cancel();
}


static int send_payload(struct turntest *tt, size_t offset,
			const struct sa *dst, const char *str)
{
	struct mbuf *mb = mbuf_alloc(offset + str_len(str));
	int err;

	if (!mb)
		return ENOMEM;

	mb->pos = offset;

	err = mbuf_write_str(mb, str);

	mb->pos = offset;

	if (!err)
		err = udp_send(tt->us_cli, dst, mb);

	mem_deref(mb);

	return err;
}


static void turnc_chan_handler(void *arg)
{
	struct turntest *tt = arg;
	int err = 0;

	++tt->n_chan_resp;

	/*err |= send_payload(tt, 0, &tt->peer, test_payload);*/
	err |= send_payload(tt, 4, &tt->peer, test_payload);
	if (err) {
		DEBUG_WARNING("failed to send payload (%m)\n", err);
		complete_test(tt, err);
	}

	if (err)
		complete_test(tt, err);
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

	++tt->n_alloc_resp;

	if (err) {
		complete_test(tt, err);
		return;
	}

	if (scode) {
		complete_test(tt, EPROTO);
		return;
	}

	if (!sa_cmp(relay_addr, &tt->turnsrv->relay, SA_ALL))
		err = EPROTO;
	if (!sa_cmp(mapped_addr, &tt->cli, SA_ALL))
		err = EPROTO;

	if (err) {
		complete_test(tt, err);
		return;
	}

	/* Permission is needed for sending data */
	err = turnc_add_perm(tt->turnc, &tt->peer, NULL, NULL);
	if (err)
		goto out;

	/*err  = send_payload(tt,  0, &tt->peer, test_payload);*/
	err |= send_payload(tt, 36, &tt->peer, test_payload);
	if (err) {
		DEBUG_WARNING("failed to send payload (%m)\n", err);
		complete_test(tt, err);
	}

	err = turnc_add_chan(tt->turnc, &tt->peer,
			     turnc_chan_handler, tt);
	if (err)
		goto out;

 out:
	if (err)
		complete_test(tt, err);
}


static void peer_udp_recv(const struct sa *src, struct mbuf *mb, void *arg)
{
	struct turntest *tt = arg;
	int err = 0;
	(void)src;

	++tt->n_peer_recv;

	TEST_MEMCMP(test_payload, strlen(test_payload),
		    mbuf_buf(mb), mbuf_get_left(mb));

 out:
	if (err || is_complete(tt))
		complete_test(tt, err);
}


static int turntest_alloc(struct turntest **ttp)
{
	struct turntest *tt;
	struct sa laddr;
	int err;

	tt = mem_zalloc(sizeof(*tt), destructor);
	if (!tt)
		return ENOMEM;

	err  = sa_set_str(&laddr, "127.0.0.1", 0);
	if (err)
		goto out;

	err |= udp_listen(&tt->us_cli, &laddr, NULL, NULL);
	if (err)
		goto out;

	err = udp_local_get(tt->us_cli, &tt->cli);
	if (err)
		goto out;

	err = udp_listen(&tt->us_peer, &laddr, peer_udp_recv, tt);
	if (err)
		goto out;

	err = udp_local_get(tt->us_peer, &tt->peer);
	if (err)
		goto out;

	err = turnserver_alloc(&tt->turnsrv);
	if (err)
		goto out;

	err = turnc_alloc(&tt->turnc, NULL, IPPROTO_UDP, tt->us_cli,
			  0, &tt->turnsrv->laddr,
			  "username", "password", 600, turnc_handler, tt);
	if (err)
		goto out;

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

	err = re_main_timeout(200);
	if (err)
		goto out;

	if (tt->err) {
		err = tt->err;
		goto out;
	}

	/* verify results after test is complete */

	TEST_EQUALS(1, tt->n_alloc_resp);
	TEST_EQUALS(1, tt->n_chan_resp);
	TEST_EQUALS(2, tt->n_peer_recv);

	TEST_ASSERT(tt->turnsrv->n_allocate >= 1);
	TEST_ASSERT(tt->turnsrv->n_chanbind >= 1);
	TEST_EQUALS(1, tt->turnsrv->n_send);
	TEST_EQUALS(1, tt->turnsrv->n_raw);

 out:
	mem_deref(tt);

	return err;
}
