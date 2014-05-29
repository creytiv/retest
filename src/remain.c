/**
 * @file remain.c Testcode for re main loop
 *
 * Copyright (C) 2010 Creytiv.com
 */

#include <string.h>
#include <re.h>
#ifdef HAVE_PTHREAD
#include <pthread.h>
#endif
#include "test.h"


#define DEBUG_MODULE "remain"
#define DEBUG_LEVEL 5
#include <re_dbg.h>


#ifdef HAVE_PTHREAD


struct data {
	pthread_t tid;
	bool thread_started;
	bool thread_exited;
	unsigned tmr_called;
	int err;
};


static void tmr_handler(void *arg)
{
	struct data *data = arg;
	int err = 0;

	/* verify that timer is called from the new thread */
	TEST_ASSERT(0 != pthread_equal(data->tid, pthread_self()));

	++data->tmr_called;

 out:
	if (err)
		data->err = err;
	re_cancel();
}


static void *thread_handler(void *arg)
{
	struct data *data = arg;
	struct tmr tmr;
	int err;

	data->thread_started = true;

	tmr_init(&tmr);

	err = re_thread_init();
	if (err) {
		DEBUG_WARNING("re thread init: %m\n", err);
		data->err = err;
		return NULL;
	}

	tmr_start(&tmr, 1, tmr_handler, data);

	/* run the main loop now */
	err = re_main(NULL);
	if (err) {
		data->err = err;
	}

	tmr_cancel(&tmr);

	/* cleanup */
	tmr_debug();
	re_thread_close();

	data->thread_exited = true;

	return NULL;
}


static int test_remain_thread(void)
{
	struct data data;
	int i, err;

	memset(&data, 0, sizeof(data));

	err = pthread_create(&data.tid, NULL, thread_handler, &data);
	if (err)
		return err;

	/* wait for timer to be called */
	for (i=0; i<500; i++) {
		if (data.tmr_called)
			break;
		if (data.err)
			break;
		sys_msleep(1);
	}

	/* wait for thread to end */
	pthread_join(data.tid, NULL);

	if (data.err)
		return data.err;

	TEST_ASSERT(data.thread_started);
	TEST_ASSERT(data.thread_exited);
	TEST_EQUALS(1, data.tmr_called);
	TEST_EQUALS(0, data.err);

 out:
	return err;
}


#endif


int test_remain(void)
{
	int err = 0;

#ifdef HAVE_PTHREAD
	err = test_remain_thread();
	if (err)
		return err;
#endif

	return err;
}
