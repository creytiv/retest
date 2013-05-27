/**
 * @file ice.c ICE Testcode
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include <string.h>
#include <re.h>
#include "test.h"


#define DEBUG_MODULE "test_ice"
#define DEBUG_LEVEL 5
#include <re_dbg.h>


/*
 * Protocol testcode for 2 ICE agents. We setup 2 ICE agents A and B,
 * with only a basic host candidate. Gathering is done using a small
 * STUN Server running on localhost, SDP is exchanged with Offer/Answer.
 * Finally the connectivity checks are run and the result is verified.
 *
 *
 *                    .-------------.
 *                    | STUN Server |
 *                    '-------------'
 *              STUN /               \ STUN
 *                  /                 \
 *                 /                   \
 *  .-------------.                     .-------------.
 *  | ICE-Agent A |---------------------| ICE-Agent B |
 *  '-------------'     Connectivity    '-------------'
 *                         checks
 *
 */


struct stunserver {
	struct udp_sock *us;
	struct sa laddr;
};

struct agent {
	struct ice *ice;
	struct icem *icem;
	struct udp_sock *us;
	struct sa laddr;
	struct mbuf *mb;
	struct ice_test *it;  /* parent */
	char name[16];
	uint8_t compid;
	bool offerer;
	bool gathering_ok;
	bool conncheck_ok;
};

struct ice_test {
	struct stunserver *stun;
	struct agent *a;
	struct agent *b;
	struct tmr tmr;
};


static int gerr = 0;  /* global error */


static void icetest_check_gatherings(struct ice_test *it);
static void icetest_check_connchecks(struct ice_test *it);


/*
 * Test tools
 */


static void complete_test(int err)
{
	gerr = err;
	re_cancel();
}


/*
 * STUN Server
 */


static void stunserver_udp_recv(const struct sa *src, struct mbuf *mb,
				void *arg)
{
	struct stunserver *stun = arg;
	struct stun_msg *msg;
	int err;

	err = stun_msg_decode(&msg, mb, NULL);
	if (err) {
		complete_test(err);
		return;
	}

	switch (stun_msg_class(msg)) {

	case STUN_CLASS_REQUEST:
		err = stun_reply(IPPROTO_UDP, stun->us, src,
				 0, msg, NULL, 0, false, 1,
				 STUN_ATTR_XOR_MAPPED_ADDR, src);
		break;

	default:
		break;
	}

	mem_deref(msg);
}


static void stunserver_destructor(void *arg)
{
	struct stunserver *stun = arg;

	mem_deref(stun->us);
}


static int stunserver_alloc(struct stunserver **stunp)
{
	struct stunserver *stun;
	int err;

	stun = mem_zalloc(sizeof(*stun), stunserver_destructor);
	if (!stun)
		return ENOMEM;

	err = sa_set_str(&stun->laddr, "127.0.0.1", 0);
	if (err)
		goto out;

	err = udp_listen(&stun->us, &stun->laddr, stunserver_udp_recv, stun);
	if (err)
		goto out;

	err = udp_local_get(stun->us, &stun->laddr);
	if (err)
		goto out;

 out:
	if (err)
		mem_deref(stun);
	else
		*stunp = stun;

	return err;
}


/*
 * ICE Agent
 */


static void agent_destructor(void *arg)
{
	struct agent *agent = arg;

	mem_deref(agent->icem);
	mem_deref(agent->ice);
	mem_deref(agent->us);
	mem_deref(agent->mb);
}


static struct agent *agent_other(struct agent *agent)
{
	if (agent->it->a == agent)
		return agent->it->b;
	else
		return agent->it->a;
}


static int agent_encode_sdp(struct agent *agent)
{
	struct le *le;
	int err = 0;

	for (le = icem_lcandl(agent->icem)->head; le; le = le->next) {

		struct cand *cand = le->data;

		err = mbuf_printf(agent->mb, "a=candidate:%H\r\n",
				  ice_cand_encode, cand);
		if (err)
			break;
	}

	err |= mbuf_printf(agent->mb, "a=ice-ufrag:%s\r\n",
			    ice_ufrag(agent->ice));
	err |= mbuf_printf(agent->mb, "a=ice-pwd:%s\r\n",
			   ice_pwd(agent->ice));

	return err;
}


static int agent_verify_offer(const struct agent *agent)
{
	char buf[1024];

	if (re_snprintf(buf, sizeof(buf),
			"a=candidate:7f000001 %u UDP 2113929465"
			" 127.0.0.1 %u typ host\r\n"
			"a=ice-ufrag:%s\r\n"
			"a=ice-pwd:%s\r\n",
			agent->compid, sa_port(&agent->laddr),
			ice_ufrag(agent->ice),
			ice_pwd(agent->ice)) < 0) {
		return ENOMEM;
	}

	if (agent->mb->end != str_len(buf) ||
	    0 != memcmp(buf, agent->mb->buf, agent->mb->end)) {
		DEBUG_WARNING("SDP offer mismatch\n");
		(void)re_printf("ref: \n%s\n", buf);
		(void)re_printf("gen: \n%b\n", agent->mb->buf, agent->mb->end);
		return EBADMSG;
	}

	return 0;
}


static int agent_decode_sdp(struct agent *agent, struct mbuf *sdp)
{
	struct pl pl;
	const char *pmax;
	int err;

	sdp->pos = 0;
	pl_set_mbuf(&pl, sdp);
	pmax = pl.p + pl.l;

	while (pl.p < pmax) {
		struct pl n, v;
		char *name = NULL, *value = NULL;

		err = re_regex(pl.p, pl.l, "a=[^:]+:[^\r\n]+", &n, &v);
		if (err)
			break;

		err |= pl_strdup(&name, &n);
		err |= pl_strdup(&value, &v);

		if (!err)
			err = icem_sdp_decode(agent->icem, name, value);

		mem_deref(name);
		mem_deref(value);

		if (err)
			break;

		pl_advance(&pl, (v.p - pl.p) + v.l + 2);
	}

	return err;
}


static void agent_gather_handler(int err, uint16_t scode, const char *reason,
				 void *arg)
{
	struct agent *agent = arg;

	if (err)
		goto error;
	if (scode) {
		DEBUG_WARNING("gathering failed: %u %s\n", scode, reason);
		complete_test(EPROTO);
		return;
	}

	err = agent_encode_sdp(agent);
	if (err)
		goto error;

	err = agent_verify_offer(agent);
	if (err)
		goto error;

	agent->gathering_ok = true;
	icetest_check_gatherings(agent->it);

	return;

 error:
	complete_test(err);
}


static void agent_connchk_handler(int err, bool update, void *arg)
{
	struct agent *agent = arg;
	const struct sa *laddr, *raddr;

	if (err) {
		if (err != ENOMEM) {
			DEBUG_WARNING("%s: connectivity checks failed: %m\n",
				      agent->name, err);
		}

		complete_test(err);
		return;
	}

	if (agent->offerer ^ update) {
		DEBUG_WARNING("error in update flag\n");
		complete_test(EPROTO);
	}

	laddr = icem_selected_laddr(agent->icem, agent->compid);
	raddr = &agent_other(agent)->laddr;

	if (!sa_cmp(&agent->laddr, laddr, SA_ALL)) {
		DEBUG_WARNING("unexpected selected address: %J\n", laddr);
		complete_test(EPROTO);
		return;
	}

	if (!icem_verify_support(agent->icem, agent->compid, raddr)) {
		complete_test(EPROTO);
		return;
	}

#if 0
	(void)re_printf("Agent %s -- Selected address: local=%J  remote=%J\n",
			agent->name, laddr, raddr);
#endif

	agent->conncheck_ok = true;
	icetest_check_connchecks(agent->it);
}


static int agent_alloc(struct agent **agentp, struct ice_test *it,
		       struct stunserver *stun,
		       const char *name, uint8_t compid, bool offerer)
{
	struct agent *agent;
	int err;

	agent = mem_zalloc(sizeof(*agent), agent_destructor);
	if (!agent)
		return ENOMEM;

	agent->it = it;
	strcpy(agent->name, name);
	agent->compid = compid;
	agent->offerer = offerer;

	agent->mb = mbuf_alloc(512);
	if (!agent->mb) {
		err = ENOMEM;
		goto out;
	}

	err = sa_set_str(&agent->laddr, "127.0.0.1", 0);
	if (err)
		goto out;

	err = udp_listen(&agent->us, &agent->laddr, 0, 0);
	if (err)
		goto out;

	err = udp_local_get(agent->us, &agent->laddr);
	if (err)
		goto out;

	err = ice_alloc(&agent->ice, ICE_MODE_FULL, offerer);
	if (err)
		goto out;

#if 0
	ice_conf(agent->ice)->debug = true;
#endif

	err = icem_alloc(&agent->icem, agent->ice, IPPROTO_UDP, 0,
			 agent_gather_handler, agent_connchk_handler, agent);
	if (err)
		goto out;

	err = icem_comp_add(agent->icem, compid, agent->us);
	if (err)
		goto out;

	err = icem_cand_add(agent->icem, compid, 0, "eth0", &agent->laddr);
	if (err)
		goto out;

	/* Start gathering now */
	err = icem_gather_srflx(agent->icem, &stun->laddr);
	if (err)
		goto out;

 out:
	if (err)
		mem_deref(agent);
	else
		*agentp = agent;

	return err;
}


/*
 * ICE Test
 */


static void icetest_timeout(void *arg)
{
	struct ice_test *it = arg;
	(void)it;

	complete_test(ENOMEM);
}


static void icetest_check_gatherings(struct ice_test *it)
{
	int err;

	if (!it->a->gathering_ok || !it->b->gathering_ok)
		return;

	/* both gatherings complete
	 * exchange SDP and start conncheck
	 */

	err = agent_decode_sdp(it->a, it->b->mb);
	if (err)
		goto error;
	err = agent_decode_sdp(it->b, it->a->mb);
	if (err)
		goto error;

	err = ice_conncheck_start(it->a->ice);
	if (err)
		goto error;
	err = ice_conncheck_start(it->b->ice);
	if (err)
		goto error;

	return;

 error:
	complete_test(err);
}


static void icetest_check_connchecks(struct ice_test *it)
{
	if (!it->a->conncheck_ok || !it->b->conncheck_ok)
		return;

	complete_test(0);
}


static void icetest_destructor(void *arg)
{
	struct ice_test *it = arg;

	tmr_cancel(&it->tmr);
	mem_deref(it->b);
	mem_deref(it->a);
	mem_deref(it->stun);
}


static int icetest_alloc(struct ice_test **itp)
{
	struct ice_test *it;
	int err;

	it = mem_zalloc(sizeof(*it), icetest_destructor);
	if (!it)
		return ENOMEM;

	err = stunserver_alloc(&it->stun);
	if (err)
		goto out;

	err = agent_alloc(&it->a, it, it->stun, "A", 7, true);
	if (err)
		goto out;

	err = agent_alloc(&it->b, it, it->stun, "B", 7, false);
	if (err)
		goto out;

	tmr_start(&it->tmr, 500, icetest_timeout, it);

 out:
	if (err)
		mem_deref(it);
	else
		*itp = it;

	return err;
}


int test_ice(void)
{
	struct ice_test *it = NULL;
	int err;

	err = icetest_alloc(&it);
	if (err)
		goto out;

	(void)re_main(NULL);

	/* read back global errorcode */
	err = gerr;

 out:
	mem_deref(it);

	return err;
}
