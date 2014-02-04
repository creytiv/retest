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


struct agent {
	struct ice *ice;
	struct icem *icem;
	struct udp_sock *us;
	struct sa laddr;
	struct mbuf *mb;
	struct ice_test *it;  /* parent */
	struct pf *pf;
	enum ice_mode mode;
	char name[16];
	uint8_t compid;
	bool offerer;

	/* results: */
	bool gathering_ok;
	bool conncheck_ok;
};

struct ice_test {
	struct stunserver *stun;
	struct agent *a;
	struct agent *b;
	struct tmr tmr;
	int err;
};


static void icetest_check_gatherings(struct ice_test *it);
static void icetest_check_connchecks(struct ice_test *it);


/*
 * Test tools
 */


static void complete_test(struct ice_test *it, int err)
{
	it->err = err;

#if 0
	re_printf("\n\x1b[32m%H\x1b[;m\n", ice_debug, it->a->ice);
	re_printf("\n\x1b[36m%H\x1b[;m\n", ice_debug, it->b->ice);
#endif

	re_cancel();
}


static bool find_debug_string(struct ice *ice, const char *str)
{
	char buf[1024];

	if (re_snprintf(buf, sizeof(buf), "%H", ice_debug, ice) < 0)
		return false;

	return 0 == re_regex(buf, strlen(buf), str);
}


/*
 * ICE Agent
 */


static void agent_destructor(void *arg)
{
	struct agent *agent = arg;

	mem_deref(agent->icem);
	mem_deref(agent->ice);
	mem_deref(agent->pf);
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

	if (agent->mode == ICE_MODE_LITE) {
		err |= mbuf_printf(agent->mb, "a=%s\r\n", ice_attr_lite);
	}

	return err;
}


static int agent_verify_offer(const struct agent *agent)
{
	char buf[1024];
	int err = 0;

	if (re_snprintf(buf, sizeof(buf),
			"a=candidate:7f000001 %u UDP 2113929465"
			" 127.0.0.1 %u typ host\r\n"
			"a=ice-ufrag:%s\r\n"
			"a=ice-pwd:%s\r\n"
			"%s",
			agent->compid, sa_port(&agent->laddr),
			ice_ufrag(agent->ice),
			ice_pwd(agent->ice),
			agent->mode == ICE_MODE_LITE ? "a=ice-lite\r\n" : ""
			) < 0) {
		return ENOMEM;
	}

	TEST_STRCMP(buf, str_len(buf), agent->mb->buf, agent->mb->end);

 out:
	return err;
}


static int agent_decode_sdp(struct agent *agent, struct mbuf *sdp)
{
	struct pl pl;
	const char *pmax;
	int err = 0;

	sdp->pos = 0;
	pl_set_mbuf(&pl, sdp);
	pmax = pl.p + pl.l;

	while (pl.p < pmax) {
		struct pl n = PL_INIT, v = PL_INIT;
		char *name = NULL, *value = NULL;

		err = re_regex(pl.p, pl.l, "a=[^:]+:[^\r\n]+",
			       &n, &v);
		if (err) {
			v = pl_null;
			err = re_regex(pl.p, pl.l, "a=[^\r\n]+", &n);
			if (err)
				break;
		}

		err |= pl_strdup(&name, &n);
		if (pl_isset(&v))
			err |= pl_strdup(&value, &v);

		if (!err) {
			if (0 == strcmp(name, "ice-lite"))
				err = ice_sdp_decode(agent->ice, name, value);
			else
				err = icem_sdp_decode(agent->icem,
						      name, value);
		}

		mem_deref(name);
		mem_deref(value);

		if (err)
			break;

		if (pl_isset(&v))
			pl_advance(&pl, (v.p - pl.p) + v.l + 2);
		else
			pl_advance(&pl, (n.p - pl.p) + n.l + 2);
	}

	return err;
}


static int send_sdp(struct agent *agent)
{
	int err;

	/* verify ICE states */
	TEST_ASSERT(!icem_mismatch(agent->icem));

	/* after gathering is complete we expect:
	 *   1 local candidate
	 *   0 remote candidates
	 *   checklist and validlist is empty
	 */
	TEST_EQUALS(1, list_count(icem_lcandl(agent->icem)));
	TEST_EQUALS(0, list_count(icem_rcandl(agent->icem)));
	TEST_EQUALS(0, list_count(icem_checkl(agent->icem)));
	TEST_EQUALS(0, list_count(icem_validl(agent->icem)));

	/* verify that default candidate is our local address */
	TEST_ASSERT(sa_cmp(&agent->laddr,
			   icem_cand_default(agent->icem, agent->compid),
			   SA_ALL));

	/* we should not have selected candidate-pairs yet */
	TEST_ASSERT(!icem_selected_laddr(agent->icem, agent->compid));

	err = agent_encode_sdp(agent);
	if (err)
		return err;

	err = agent_verify_offer(agent);
	if (err)
		return err;

 out:
	return err;
}


static void agent_gather_handler(int err, uint16_t scode, const char *reason,
				 void *arg)
{
	struct agent *agent = arg;

	if (err)
		goto out;
	if (scode) {
		DEBUG_WARNING("gathering failed: %u %s\n", scode, reason);
		complete_test(agent->it, EPROTO);
		return;
	}

	agent->gathering_ok = true;

	err = send_sdp(agent);
	if (err)
		goto out;

	icetest_check_gatherings(agent->it);

	return;

 out:
	complete_test(agent->it, err);
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

		complete_test(agent->it, err);
		return;
	}

	if (agent->offerer ^ update) {
		DEBUG_WARNING("error in update flag\n");
		complete_test(agent->it, EPROTO);
		return;
	}

	agent->conncheck_ok = true;

	/* verify ICE states */
	TEST_ASSERT(!icem_mismatch(agent->icem));

	/* after connectivity checks are complete we expect:
	 *   1 local candidate
	 *   1 remote candidates
	 */
	TEST_EQUALS(1, list_count(icem_lcandl(agent->icem)));
	TEST_EQUALS(1, list_count(icem_rcandl(agent->icem)));
	TEST_EQUALS(0, list_count(icem_checkl(agent->icem)));
	TEST_EQUALS(1, list_count(icem_validl(agent->icem)));

	laddr = icem_selected_laddr(agent->icem, agent->compid);
	raddr = &agent_other(agent)->laddr;

	if (!sa_cmp(&agent->laddr, laddr, SA_ALL)) {
		DEBUG_WARNING("unexpected selected address: %J\n", laddr);
		complete_test(agent->it, EPROTO);
		return;
	}

	if (!icem_verify_support(agent->icem, agent->compid, raddr)) {
		complete_test(agent->it, EPROTO);
		return;
	}

#if 0
	(void)re_printf("Agent %s -- Selected address: local=%J  remote=%J\n",
			agent->name, laddr, raddr);
#endif

	icetest_check_connchecks(agent->it);

 out:
	if (err)
		complete_test(agent->it, err);
}


static int agent_alloc(struct agent **agentp, struct ice_test *it,
		       struct stunserver *stun, enum ice_mode mode,
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
	agent->mode = mode;

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

#if 0
	err = pf_create(&agent->pf, agent->us, name);
	if (err)
		goto out;
#endif

	err = udp_local_get(agent->us, &agent->laddr);
	if (err)
		goto out;

	err = ice_alloc(&agent->ice, mode, offerer);
	if (err)
		goto out;

	/* verify Mode and Role using debug strings (temp) */
	if (mode == ICE_MODE_FULL) {
		TEST_ASSERT(find_debug_string(agent->ice, "local_mode=Full"));
	}
	else {
		TEST_ASSERT(find_debug_string(agent->ice, "local_mode=Lite"));
	}

	if (offerer) {
		TEST_ASSERT(find_debug_string(agent->ice,
					      "local_role=Controlling"));
	}
	else {
		TEST_ASSERT(find_debug_string(agent->ice,
					      "local_role=Controlled"));
	}

#if 0
	ice_conf(agent->ice)->debug = true;
#endif

	err = icem_alloc(&agent->icem, agent->ice, IPPROTO_UDP, 0,
			 agent_gather_handler, agent_connchk_handler, agent);
	if (err)
		goto out;

	icem_set_name(agent->icem, name);

	err = icem_comp_add(agent->icem, compid, agent->us);
	if (err)
		goto out;

	err = icem_cand_add(agent->icem, compid, 0, "eth0", &agent->laddr);
	if (err)
		goto out;

	/* Start gathering now -- full mode only
	 *
	 * A lite implementation doesn't gather candidates;
	 * it includes only host candidates for any media stream.
	 */
	if (mode == ICE_MODE_FULL) {

		err = icem_gather_srflx(agent->icem, &stun->laddr);
		if (err)
			goto out;
	}
	else { /* Lite mode */

		err = icem_lite_set_default_candidates(agent->icem);
		if (err)
			goto out;

		err = send_sdp(agent);
		if (err)
			goto out;
	}

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


static int verify_after_sdp_exchange(struct agent *agent)
{
	struct agent *other = agent_other(agent);
	int err = 0;

	/* verify remote mode (after SDP exchange) */
	if (other->mode == ICE_MODE_FULL) {
		TEST_ASSERT(find_debug_string(agent->ice, "remote_mode=Full"));
	}
	else {
		TEST_ASSERT(find_debug_string(agent->ice, "remote_mode=Lite"));
	}

	/* verify ICE states */
	TEST_ASSERT(!icem_mismatch(agent->icem));

	/* after SDP was exchanged, we expect:
	 *   1 local candidate
	 *   1 remote candidates
	 *   checklist and validlist is empty
	 */
	TEST_EQUALS(1, list_count(icem_lcandl(agent->icem)));
	TEST_EQUALS(1, list_count(icem_rcandl(agent->icem)));
	TEST_EQUALS(0, list_count(icem_checkl(agent->icem)));
	TEST_EQUALS(0, list_count(icem_validl(agent->icem)));

	/* verify that default candidate is our local address */
	TEST_ASSERT(sa_cmp(&agent->laddr,
			   icem_cand_default(agent->icem, agent->compid),
			   SA_ALL));

	/* we should not have selected candidate-pairs yet */
	TEST_ASSERT(!icem_selected_laddr(agent->icem, agent->compid));

 out:
	if (err) {
		DEBUG_WARNING("agent %s failed\n", agent->name);
	}
	return err;
}


static int agent_start(struct agent *agent)
{
	int err = 0;

	/* verify that check-list is empty before we start */
	TEST_EQUALS(0, list_count(icem_checkl(agent->icem)));
	TEST_EQUALS(0, list_count(icem_validl(agent->icem)));

	if (agent->mode == ICE_MODE_FULL) {

		err = ice_conncheck_start(agent->ice);
		TEST_ERR(err);

		TEST_EQUALS(1, list_count(icem_checkl(agent->icem)));
	}
	else {
		/* Formation of check lists is performed
		   only by full implementations. */
		TEST_EQUALS(0, list_count(icem_checkl(agent->icem)));
	}

	TEST_EQUALS(0, list_count(icem_validl(agent->icem)));

 out:
	return err;
}


static int agent_verify_completed(struct agent *agent)
{
	uint32_t validc;
	int err = 0;

	if (agent->mode == ICE_MODE_FULL) {
		TEST_ASSERT(agent->gathering_ok);
		TEST_ASSERT(agent->conncheck_ok);
	}

	TEST_EQUALS(0, list_count(icem_checkl(agent->icem)));
	validc = list_count(icem_validl(agent->icem));
	if (agent->mode == ICE_MODE_FULL) {
		TEST_EQUALS(1, validc);
	}
	else {
		TEST_ASSERT(validc==0 || validc==1);
	}

 out:
	return err;
}


static void icetest_check_gatherings(struct ice_test *it)
{
	int err;

	if (it->a->mode == ICE_MODE_FULL && !it->a->gathering_ok)
		return;
	if (it->b->mode == ICE_MODE_FULL && !it->b->gathering_ok)
		return;

	/* both gatherings are complete
	 * exchange SDP and start conncheck
	 */

	err = agent_decode_sdp(it->a, it->b->mb);
	if (err)
		goto out;
	err = agent_decode_sdp(it->b, it->a->mb);
	if (err)
		goto out;

	err = verify_after_sdp_exchange(it->a);
	if (err)
		goto error;
	err = verify_after_sdp_exchange(it->b);
	if (err)
		goto error;

	err  = agent_start(it->a);
	if (err)
		goto out;
	err = agent_start(it->b);
	if (err)
		goto out;

	return;

 out:
 error:
	complete_test(it, err);
}


static void tmr_handler(void *arg)
{
	struct ice_test *it = arg;

#if 0
	re_printf("\n\x1b[32m%H\x1b[;m\n", ice_debug, it->a->ice);
	re_printf("\n\x1b[36m%H\x1b[;m\n", ice_debug, it->b->ice);
#endif

	complete_test(it, 0);
}


static void icetest_check_connchecks(struct ice_test *it)
{
	if (it->a->mode == ICE_MODE_FULL && !it->a->conncheck_ok)
		return;
	if (it->b->mode == ICE_MODE_FULL && !it->b->conncheck_ok)
		return;

	/* start an async timer to let the socket traffic complete */
	tmr_start(&it->tmr, 1, tmr_handler, it);
}


static void icetest_destructor(void *arg)
{
	struct ice_test *it = arg;

	tmr_cancel(&it->tmr);
	mem_deref(it->b);
	mem_deref(it->a);
	mem_deref(it->stun);
}


static int icetest_alloc(struct ice_test **itp,
			 enum ice_mode mode_a, enum ice_mode mode_b)
{
	struct ice_test *it;
	int err;

	it = mem_zalloc(sizeof(*it), icetest_destructor);
	if (!it)
		return ENOMEM;

	err = stunserver_alloc(&it->stun);
	if (err)
		goto out;

	err = agent_alloc(&it->a, it, it->stun, mode_a, "A", 7, true);
	if (err)
		goto out;

	err = agent_alloc(&it->b, it, it->stun, mode_b, "B", 7, false);
	if (err)
		goto out;

 out:
	if (err)
		mem_deref(it);
	else
		*itp = it;

	return err;
}


static int test_ice_loop(enum ice_mode mode_a, enum ice_mode mode_b)
{
	struct ice_test *it = NULL;
	int err;

	err = icetest_alloc(&it, mode_a, mode_b);
	if (err)
		goto out;

	err = re_main_timeout(300);
	if (err)
		goto out;

	/* read back global errorcode */
	if (it->err) {
		err = it->err;
		goto out;
	}

	/* now verify all results after test was finished */
	err  = agent_verify_completed(it->a);
	err |= agent_verify_completed(it->b);
	if (err)
		goto out;

	if (mode_a == ICE_MODE_FULL && mode_b == ICE_MODE_FULL) {
		TEST_EQUALS(2, it->stun->nrecv);
	}
	else {
		TEST_EQUALS(1, it->stun->nrecv);
	}

 out:
	mem_deref(it);

	return err;
}


int test_ice(void)
{
	int err = 0;

	err |= test_ice_loop(ICE_MODE_FULL, ICE_MODE_FULL);
	err |= test_ice_loop(ICE_MODE_FULL, ICE_MODE_LITE);

	return err;
}
