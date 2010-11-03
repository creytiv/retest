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


enum {N_TIMERS=64};
static struct tmr _timers[N_TIMERS];
static int timers_ok;


static void timeout(void *arg)
{
	(void)arg;

	DEBUG_INFO("timedout (timers_ok=%d)\n", timers_ok);

	++timers_ok;

	if (timers_ok >= N_TIMERS) {
		re_cancel();
	}
}


int test_tmr(void)
{
	int i, err;

	timers_ok = 0;

	for (i=0; i<N_TIMERS; i++) {
		const uint64_t delay = rand_u16() % 10;

		tmr_init(&_timers[i]);
		tmr_start(&_timers[i], delay, timeout, NULL);
	}

	err = re_main(NULL);

	/* cleanup */
	for (i=0; i<N_TIMERS; i++)
		tmr_cancel(&_timers[i]);

	return err;
}
