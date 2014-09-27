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


static bool running;


#ifdef HAVE_SIGNAL
static void signal_handler(int signum)
{
	(void)signum;

	if (running) {
		running = false;
		return;
	}

	/* Check for memory leaks */
	tmr_debug();
	mem_debug();

	exit(0);
}
#endif


#ifdef HAVE_GETOPT
static void usage(void)
{
	(void)re_fprintf(stderr, "Usage: retest [-rotal] [-p n] [-hv]"
			 " <testcase>\n");

	(void)re_fprintf(stderr, "\ntest group options:\n");
	(void)re_fprintf(stderr, "\t-r        Run regular tests\n");
	(void)re_fprintf(stderr, "\t-o        Run OOM memory tests\n");
	(void)re_fprintf(stderr, "\t-p n      Run performance tests\n");
	(void)re_fprintf(stderr, "\t-t        Run tests in multi-threads\n");
	(void)re_fprintf(stderr, "\t-a        Run all tests (default)\n");
	(void)re_fprintf(stderr, "\t-l        List all testcases and exit\n");

	(void)re_fprintf(stderr, "\ncommon options:\n");
	(void)re_fprintf(stderr, "\t-h        Help\n");
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
	bool do_all = false;
	bool do_list = false;
	bool do_thread = false;
	bool ansi = true;
	bool verbose = false;
	const char *name = NULL;
	uint32_t n = 10;
	int err = 0;

#ifdef HAVE_SIGNAL
	signal(SIGINT, signal_handler);
	signal(SIGTERM, signal_handler);
#endif

	(void)sys_coredump_set(true);

#ifdef HAVE_GETOPT
	for (;;) {
		const int c = getopt(argc, argv, "hrop:altv");
		if (0 > c)
			break;

		switch (c) {

		case '?':
		case 'h':
			usage();
			return -2;

		case 'r':
			do_reg = true;
			break;

		case 'o':
			do_oom = true;
			break;

		case 'p':
			do_perf = true;
			n = atoi(optarg);
			break;

		case 'a':
			do_all = true;
			break;

		case 'l':
			do_list = true;
			break;

		case 't':
			do_thread = true;
			break;

		case 'v':
			verbose = true;
			break;
		}
	}

	argc -= optind;

	if (argc < 0 || argc > 1) {
		usage();
		return -2;
	}

	/* no arguments - run all tests */
	if (optind == 1)
		do_all = true;

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
#if defined(WIN32) && !defined(CYGWIN)
	ansi = false;
#endif
	dbg_init(DBG_INFO, ansi ? DBG_ANSI : 0);

	/* Initialise library */
	err = libre_init();
	if (err)
		goto out;

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
	}

	if (do_reg) {
		err = test_reg(name, verbose);
		if (err)
			(void)re_fprintf(stderr, "Failed (%m)\n", err);
	}

	if (do_oom) {
		err = test_oom(name);
		if (err)
			(void)re_fprintf(stderr, "Failed (%m)\n", err);
	}

	if (do_perf) {
		err = test_perf(name, n, verbose);
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
