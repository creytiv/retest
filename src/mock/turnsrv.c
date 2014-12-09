/**
 * @file mock/turnsrv.c Mock TURN server
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include <string.h>
#include <re.h>
#include "test.h"


#define DEBUG_MODULE "mock/turnsrv"
#define DEBUG_LEVEL 5
#include <re_dbg.h>


static struct channel *find_channel_numb(struct turnserver *tt, uint16_t nr)
{
	size_t i;

	if (!tt)
		return NULL;

	for (i=0; i<tt->chanc; i++) {

		if (tt->chanv[i].nr == nr)
			return &tt->chanv[i];
	}

	return NULL;
}


static struct channel *find_channel_peer(struct turnserver *tt,
					 const struct sa *peer)
{
	size_t i;

	if (!tt)
		return NULL;

	for (i=0; i<tt->chanc; i++) {

		if (sa_cmp(&tt->chanv[i].peer, peer, SA_ALL))
			return &tt->chanv[i];
	}

	return NULL;
}


/* Receive packet on the "relayed" address -- relay to the client */
static void relay_udp_recv(const struct sa *src, struct mbuf *mb, void *arg)
{
	struct turnserver *turn = arg;
	struct channel *chan;
	int err = 0;

	chan = find_channel_peer(turn, src);
	if (chan) {
		uint16_t len = mbuf_get_left(mb);
		size_t start;

		if (mb->pos < 4) {
			DEBUG_WARNING("relay_udp_recv: mb pos < 4\n");
			return;
		}

		mb->pos -= 4;
		start = mb->pos;

		(void)mbuf_write_u16(mb, htons(chan->nr));
		(void)mbuf_write_u16(mb, htons(len));

		mb->pos = start;

		err = udp_send(turn->us, &turn->cli, mb);
	}
	else {
		err = stun_indication(IPPROTO_UDP, turn->us,
				      &turn->cli, 0, STUN_METHOD_DATA,
				      NULL, 0, false, 2,
				      STUN_ATTR_XOR_PEER_ADDR, src,
				      STUN_ATTR_DATA, mb);
	}

	if (err) {
		DEBUG_WARNING("relay_udp_recv: error %m\n", err);
	}
}


/* Simulated TURN server */
static void srv_udp_recv(const struct sa *src, struct mbuf *mb, void *arg)
{
	struct turnserver *turn = arg;
	struct stun_msg *msg = NULL;
	struct sa laddr;
	int err = 0;

	if (stun_msg_decode(&msg, mb, NULL)) {

		uint16_t numb, len;
		struct channel *chan;

		if (!turn->us_relay)
			return;

		++turn->n_raw;

		numb = ntohs(mbuf_read_u16(mb));
		len  = ntohs(mbuf_read_u16(mb));

		if (mbuf_get_left(mb) < len) {
			DEBUG_WARNING("short length: %zu < %u\n",
				      mbuf_get_left(mb), len);
		}

		chan = find_channel_numb(turn, numb);
		if (!chan)
			return;

		/* relay data from channel to peer */
		(void)udp_send(turn->us_relay, &chan->peer, mb);
		return;
	}

	switch (stun_msg_method(msg)) {

	case STUN_METHOD_ALLOCATE:
		/* Max 1 allocation for now */
		++turn->n_allocate;

		if (turn->us_relay) {
			err = EALREADY;
			goto out;
		}

		turn->cli = *src;

		err = sa_set_str(&laddr, "127.0.0.1", 0);
		if (err)
			goto out;

		err = udp_listen(&turn->us_relay, &laddr,
				 relay_udp_recv, turn);
		if (err)
			goto out;

		err = udp_local_get(turn->us_relay, &turn->relay);
		if (err)
			goto out;

		udp_rxbuf_presz_set(turn->us_relay, 4);

		err = stun_reply(IPPROTO_UDP, turn->us, src, 0,
				 msg, NULL, 0, false,
				 2,
				 STUN_ATTR_XOR_MAPPED_ADDR, src,
				 STUN_ATTR_XOR_RELAY_ADDR, &turn->relay);
		break;

	case STUN_METHOD_CREATEPERM:
		++turn->n_createperm;

		/* todo: install permissions and check them */
		err = stun_reply(IPPROTO_UDP, turn->us, src, 0,
				 msg, NULL, 0, false,
				 0);
		break;

	case STUN_METHOD_CHANBIND: {
		struct stun_attr *chnr, *peer;

		++turn->n_chanbind;

		TEST_ASSERT(turn->us_relay != NULL);

		chnr = stun_msg_attr(msg, STUN_ATTR_CHANNEL_NUMBER);
		peer = stun_msg_attr(msg, STUN_ATTR_XOR_PEER_ADDR);
		if (!chnr || !peer) {
			DEBUG_WARNING("CHANBIND: missing chnr/peer attrib\n");
		}

		TEST_ASSERT(turn->chanc < ARRAY_SIZE(turn->chanv));
		turn->chanv[turn->chanc].nr   = chnr->v.channel_number;
		turn->chanv[turn->chanc].peer = peer->v.xor_peer_addr;
		++turn->chanc;

		err = stun_reply(IPPROTO_UDP, turn->us, src, 0,
				 msg, NULL, 0, false,
				 0);
	}
		break;

	case STUN_METHOD_SEND: {
		struct stun_attr *peer, *data;

		++turn->n_send;

		TEST_ASSERT(turn->us_relay != NULL);

		peer = stun_msg_attr(msg, STUN_ATTR_XOR_PEER_ADDR);
		data = stun_msg_attr(msg, STUN_ATTR_DATA);

		/* todo: check for valid Permission */

		if (!peer || !data) {
			DEBUG_WARNING("SEND: missing peer/data attrib\n");
			goto out;
		}

		err = udp_send(turn->us_relay, &peer->v.xor_peer_addr,
			       &data->v.data);
	}
		break;

	default:
		DEBUG_WARNING("unknown STUN method: %s\n",
			      stun_method_name(stun_msg_method(msg)));
		err = EPROTO;
		break;
	}

	if (err)
		goto out;

 out:
	if (err && stun_msg_class(msg) == STUN_CLASS_REQUEST) {
		(void)stun_ereply(IPPROTO_UDP, turn->us, src, 0, msg,
				  500, "Server Error",
				  NULL, 0, false, 0);
	}

	mem_deref(msg);
}


static void destructor(void *arg)
{
	struct turnserver *turn = arg;

	mem_deref(turn->us);
	mem_deref(turn->us_relay);
}


int turnserver_alloc(struct turnserver **turnp)
{
	struct turnserver *turn;
	struct sa laddr;
	int err = 0;

	if (!turnp)
		return EINVAL;

	turn = mem_zalloc(sizeof(*turn), destructor);
	if (!turn)
		return ENOMEM;

	err = sa_set_str(&laddr, "127.0.0.1", 0);
	if (err)
		goto out;

	err = udp_listen(&turn->us, &laddr, srv_udp_recv, turn);
	if (err)
		goto out;

	err = udp_local_get(turn->us, &turn->laddr);
	if (err)
		goto out;

 out:
	if (err)
		mem_deref(turn);
	else
		*turnp = turn;

	return err;
}
