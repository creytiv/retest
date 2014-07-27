/**
 * @file natbd.c NAT Behavior Discovery Testcode
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include <string.h>
#include <re.h>
#include "test.h"


#define DEBUG_MODULE "test_natbd"
#define DEBUG_LEVEL 5
#include <re_dbg.h>


struct fixture {
	struct stunserver *srv;
	int err;
	uint16_t scode;
	bool hairpin_supp;
	int genalg_status;
};


static void nat_hairpinning_handler(int err, bool supported, void *arg)
{
	struct fixture *fix = arg;

	fix->err = err;
	fix->hairpin_supp = supported;

	re_cancel();
}


static int test_natbd_hairpinning(struct fixture *fix)
{
	struct nat_hairpinning *nh;
	int err;

	err = nat_hairpinning_alloc(&nh, &fix->srv->laddr, IPPROTO_UDP,
				    NULL, nat_hairpinning_handler, fix);
	if (err)
		return err;

	err = nat_hairpinning_start(nh);
	if (err)
		goto out;

	err = re_main_timeout(200);
	if (err)
		goto out;

	if (fix->err) {
		err = fix->err;
		goto out;
	}

	/* we have no NAT so hairpinning must be supported */
	TEST_ASSERT(fix->hairpin_supp);

 out:
	mem_deref(nh);

	return err;
}


static void nat_genalg_handler(int err, uint16_t scode, const char *reason,
			       int status, const struct sa *map, void *arg)
{
	struct fixture *fix = arg;
	(void)reason;
	(void)map;

	fix->err = err;
	fix->scode = scode;
	fix->genalg_status = status;

	re_cancel();
}


static int test_natbd_genalg(struct fixture *fix)
{
	struct nat_genalg *ng;
	int err;

	err = nat_genalg_alloc(&ng, &fix->srv->laddr, IPPROTO_UDP, NULL,
			       nat_genalg_handler, fix);
	if (err)
		return err;

	err = nat_genalg_start(ng);
	if (err)
		goto out;

	err = re_main_timeout(200);
	if (err)
		goto out;

	if (fix->err) {
		err = fix->err;
		goto out;
	}

	/* we have no NAT so there should be no Generic ALG */
	TEST_EQUALS(0, fix->scode);
	TEST_EQUALS(-1, fix->genalg_status);

 out:

	mem_deref(ng);
	return err;
}


int test_natbd(void)
{
	struct fixture fix;
	int err;

	memset(&fix, 0, sizeof(fix));

	err = stunserver_alloc(&fix.srv);
	if (err)
		return err;

	err = test_natbd_hairpinning(&fix);
	if (err)
		goto out;

	err = test_natbd_genalg(&fix);
	if (err)
		goto out;

	TEST_ASSERT(fix.srv->nrecv > 0);

 out:
	mem_deref(fix.srv);
	return err;
}
