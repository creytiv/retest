/**
 * @file main.c  Main regression testcode
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include <stdlib.h>
#ifdef SOLARIS
#define __EXTENSIONS__ 1
#endif
#include <stdio.h>
#include <signal.h>
#include <string.h>
#ifdef HAVE_GETOPT
#include <getopt.h>
#endif
#include <re.h>
#include "test.h"


#define DEBUG_MODULE "retest"
#define DEBUG_LEVEL 5
#include <re_dbg.h>


#ifdef HAVE_SIGNAL
static void signal_handler(int num)
{
	re_fprintf(stderr, "forced exit by signal %d -- test aborted\n", num);

	exit(0);
}
#endif


#ifdef HAVE_GETOPT
static void usage(void)
{
	(void)re_fprintf(stderr, "Usage: retest [-rotalp] [-hmv]"
			 " <testcase>\n");

	(void)re_fprintf(stderr, "\ntest group options:\n");
	(void)re_fprintf(stderr, "\t-r        Run regular tests\n");
	(void)re_fprintf(stderr, "\t-o        Run OOM memory tests\n");
	(void)re_fprintf(stderr, "\t-p        Run performance tests\n");
	(void)re_fprintf(stderr, "\t-t        Run tests in multi-threads\n");
	(void)re_fprintf(stderr, "\t-a        Run all tests (default)\n");
	(void)re_fprintf(stderr, "\t-l        List all testcases and exit\n");

	(void)re_fprintf(stderr, "\ncommon options:\n");
	(void)re_fprintf(stderr, "\t-d <path> Path to data files\n");
	(void)re_fprintf(stderr, "\t-h        Help\n");
	(void)re_fprintf(stderr, "\t-m <met>  Async polling method to use\n");
	(void)re_fprintf(stderr, "\t-v        Verbose output\n");
}
#endif


static void dbg_handler(int level, const char *p, size_t len, void *arg)
{
	(void)level;
	(void)arg;

	printf("%.*s", (int)len, p);
}


int main(int argc, char *argv[])
{
	bool do_reg = false;
	bool do_oom = false;
	bool do_perf = false;
	bool do_all = true;    /* run all tests is default */
	bool do_list = false;
	bool do_thread = false;
	bool ansi = true;
	bool verbose = false;
	const char *name = NULL;
	enum poll_method method = poll_method_best();
	int err = 0;

#ifdef HAVE_SIGNAL
	signal(SIGINT, signal_handler);
	signal(SIGTERM, signal_handler);
#endif

	(void)sys_coredump_set(true);

#ifdef HAVE_GETOPT
	for (;;) {
		const int c = getopt(argc, argv, "hropaltvm:d:");
		if (0 > c)
			break;

		switch (c) {

		case '?':
		case 'h':
			usage();
			return -2;

		case 'r':
			do_reg = true;
			do_all = false;
			break;

		case 'o':
			do_oom = true;
			do_all = false;
			break;

		case 'p':
			do_perf = true;
			do_all = false;
			break;

		case 'a':
			do_all = true;
			break;

		case 'l':
			do_list = true;
			do_all = false;
			break;

		case 't':
			do_thread = true;
			do_all = false;
			break;

		case 'v':
			verbose = true;
			break;

		case 'm': {
			struct pl pollname;
			pl_set_str(&pollname, optarg);
			err = poll_method_type(&method, &pollname);
			if (err) {
				re_fprintf(stderr,
					   "could not resolve async polling"
					   " method '%r'\n", &pollname);
				return err;
			}
		}
			break;

		case 'd':
			test_set_datapath(optarg);
			break;
		}
	}

	argc -= optind;

	if (argc < 0 || argc > 1) {
		usage();
		return -2;
	}

	if (argc >= 1) {
		name = argv[optind];
		printf("single testcase: %s\n", name);
	}

#else
	(void)argc;
	(void)argv;
	do_reg = true;
	do_oom = true;
	do_perf = true;
#endif

	/* Initialise debugging */
#if defined(WIN32)
	ansi = false;
#endif
	dbg_init(DBG_INFO, ansi ? DBG_ANSI : 0);

	/* Initialise library */
	err = libre_init();
	if (err)
		goto out;

	err = poll_method_set(method);
	if (err) {
		DEBUG_WARNING("could not set polling method '%s' (%m)\n",
			      poll_method_name(method), err);
		goto out;
	}

	dbg_handler_set(dbg_handler, 0);

	DEBUG_NOTICE("libre version %s (%s/%s)\n", sys_libre_version_get(),
		     sys_arch_get(), sys_os_get());

	dbg_handler_set(NULL, 0);

	if (do_all) {
		do_reg = true;
		do_oom = true;
		do_thread = true;
	}

	if (do_list) {
		test_listcases();
		goto out;
	}

	/*
	 * Different test-groups specified below:
	 */

	re_printf("using async polling method '%s'\n",
		  poll_method_name(method));

	if (verbose) {
		re_printf("using datapath '%s'\n", test_datapath());
	}

	if (do_reg) {
		err = test_reg(name, verbose);
		if (err)
			(void)re_fprintf(stderr, "Failed (%m)\n", err);
	}

	if (do_oom) {
		err = test_oom(name, verbose);
		if (err)
			(void)re_fprintf(stderr, "Failed (%m)\n", err);
	}

	if (do_perf) {
		err = test_perf(name, verbose);
		if (err)
			(void)re_fprintf(stderr, "Failed (%m)\n", err);
	}

	if (do_thread) {
#ifdef HAVE_PTHREAD
		err = test_multithread();
		if (err)
			(void)re_fprintf(stderr, "Failed (%m)\n", err);
#else
		(void)re_fprintf(stderr, "no support for threads\n");
		err = ENOSYS;
		goto out;
#endif
	}

 out:
	libre_close();

	/* Check for memory leaks */
	tmr_debug();
	mem_debug();

	return err;
}
