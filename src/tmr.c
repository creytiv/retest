/**
 * @file tmr.c  Timers testcode
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include <string.h>
#include <re.h>
#include "test.h"


#define DEBUG_MODULE "testtmr"
#define DEBUG_LEVEL 5
#include <re_dbg.h>


enum {N_TIMERS=32};

struct tmr_test {
	struct tmr tmrv[N_TIMERS];
	size_t timers_ok;
};


static void timeout(void *arg)
{
	struct tmr_test *tt = arg;

	++tt->timers_ok;

	if (tt->timers_ok >= N_TIMERS)
		re_cancel();
}


int test_tmr(void)
{
	struct tmr_test tt;
	struct list *lst;
	struct le *le;
	uint64_t prev = 0;
	size_t i;
	int err;

	memset(&tt, 0, sizeof(tt));

	for (i=0; i<N_TIMERS; i++)
		tmr_start(&tt.tmrv[i], rand_u16() % 10, timeout, &tt);

	/* verify that timers are sorted by jfs */
	lst = tt.tmrv[0].le.list;
	for (le = lst->head; le; le = le->next) {

		const struct tmr *tmr = le->data;

		if (tmr->jfs < prev) {
			err = ETIMEDOUT;
			goto out;
		}

		prev = tmr->jfs;
	}

	err = re_main_timeout(500);

	if (!err && tt.timers_ok != N_TIMERS)
		err = EINVAL;

 out:
	/* cleanup */
	for (i=0; i<N_TIMERS; i++)
		tmr_cancel(&tt.tmrv[i]);

	return err;
}
