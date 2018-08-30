/**
 * @file test.c  Regression testcode
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_IO_H
#include <io.h>
#endif
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#ifdef HAVE_PTHREAD
#include <pthread.h>
#endif
#include <re.h>
#include "test.h"


#define DEBUG_MODULE "test"
#define DEBUG_LEVEL 5
#include <re_dbg.h>


typedef int (test_exec_h)(void);

struct test {
	test_exec_h *exec;
	const char *name;
};

#define TEST(a) {a, #a}

static const struct test tests[] = {
	TEST(test_aes),
	TEST(test_aes_gcm),
	TEST(test_aubuf),
	TEST(test_auresamp),
	TEST(test_base64),
	TEST(test_bfcp),
	TEST(test_bfcp_bin),
	TEST(test_conf),
	TEST(test_crc32),
	TEST(test_dns_hdr),
	TEST(test_dns_rr),
	TEST(test_dns_dname),
	TEST(test_dsp),
#ifdef USE_TLS
	TEST(test_dtls),
	TEST(test_dtls_srtp),
#endif
	TEST(test_dtmf),
	TEST(test_fir),
	TEST(test_fmt_human_time),
	TEST(test_fmt_param),
	TEST(test_fmt_pl),
	TEST(test_fmt_pl_u32),
	TEST(test_fmt_pl_u64),
	TEST(test_fmt_pl_x3264),
	TEST(test_fmt_pl_float),
	TEST(test_fmt_print),
	TEST(test_fmt_regex),
	TEST(test_fmt_snprintf),
	TEST(test_fmt_str),
	TEST(test_fmt_str_error),
	TEST(test_fmt_unicode),
	TEST(test_fmt_unicode_decode),
	TEST(test_g711_alaw),
	TEST(test_g711_ulaw),
	TEST(test_hash),
	TEST(test_hmac_sha1),
	TEST(test_hmac_sha256),
	TEST(test_http),
	TEST(test_http_loop),
#ifdef USE_TLS
	TEST(test_https_loop),
#endif
	TEST(test_httpauth_chall),
	TEST(test_httpauth_resp),
	TEST(test_ice_cand),
	TEST(test_ice_loop),
	TEST(test_ice_loop_turn),
	TEST(test_ice_lite),
	TEST(test_jbuf),
	TEST(test_json),
	TEST(test_json_file),
	TEST(test_json_unicode),
	TEST(test_json_bad),
	TEST(test_json_array),
	TEST(test_list),
	TEST(test_list_ref),
	TEST(test_list_sort),
	TEST(test_mbuf),
	TEST(test_md5),
	TEST(test_mem),
	TEST(test_mem_reallocarray),
	TEST(test_mem_secure),
	TEST(test_mqueue),
	TEST(test_natbd),
	TEST(test_odict),
	TEST(test_odict_array),
	TEST(test_remain),
	TEST(test_rtp),
	TEST(test_rtcp_encode),
	TEST(test_rtcp_encode_afb),
	TEST(test_rtcp_decode),
	TEST(test_rtcp_packetloss),
	TEST(test_sa_class),
	TEST(test_sa_cmp),
	TEST(test_sa_decode),
	TEST(test_sa_ntop),
	TEST(test_sdp_all),
	TEST(test_sdp_bfcp),
	TEST(test_sdp_parse),
	TEST(test_sdp_oa),
	TEST(test_sdp_extmap),
	TEST(test_sha1),
	TEST(test_sip_addr),
	TEST(test_sip_apply),
	TEST(test_sip_hdr),
	TEST(test_sip_param),
	TEST(test_sip_parse),
	TEST(test_sip_via),
	TEST(test_sipevent),
	TEST(test_sipreg_udp),
	TEST(test_sipreg_tcp),
#ifdef USE_TLS
	TEST(test_sipreg_tls),
#endif
	TEST(test_sipsess),
	TEST(test_srtp),
	TEST(test_srtcp),
	TEST(test_srtp_gcm),
	TEST(test_srtcp_gcm),
	TEST(test_stun_req),
	TEST(test_stun_resp),
	TEST(test_stun_reqltc),
	TEST(test_stun),
	TEST(test_sys_div),
	TEST(test_sys_endian),
	TEST(test_sys_rand),
	TEST(test_tcp),
	TEST(test_telev),
#ifdef USE_TLS
	TEST(test_tls),
	/*TEST(test_tls_ec),*/
	/*TEST(test_tls_selfsigned),*/
	TEST(test_tls_certificate),
#endif
	TEST(test_tmr),
	TEST(test_turn),
	TEST(test_turn_tcp),
	TEST(test_udp),
	TEST(test_uri),
	TEST(test_uri_cmp),
	TEST(test_uri_encode),
	TEST(test_uri_headers),
	TEST(test_uri_user),
	TEST(test_uri_params_headers),
	TEST(test_uri_escape),
	TEST(test_vid),
	TEST(test_vidconv),
	TEST(test_vidconv_scaling),
	TEST(test_vidconv_pixel_formats),
	TEST(test_websock),

#ifdef USE_TLS
	/* combination tests: */
	TEST(test_dtls_turn),
#endif
};


#ifdef DATA_PATH
static char datapath[256] = DATA_PATH;
#else
static char datapath[256] = "./data";
#endif


static const struct test *find_test(const char *name)
{
	size_t i;

	for (i=0; i<ARRAY_SIZE(tests); i++) {

		if (0 == str_casecmp(name, tests[i].name))
			return &tests[i];
	}

	return NULL;
}


/**
 * Run a single testcase in OOM (Out-of-memory) mode.
 *
 * Start with 0 blocks free, and increment by 1 until the test passes.
 *
 *
 *  Blocks
 *  Free
 *
 *    /'\
 *     |
 *   5 |           #
 *     |         # #
 *     |       # # #
 *     |     # # # #
 *   1 |   # # # # #
 *     '--------------> time
 */
static int testcase_oom(const struct test *test, int levels, bool verbose)
{
	int i;
	int err = 0;

	if (verbose)
		(void)re_fprintf(stderr, "  %-24s: ", test->name);

	/* All memory levels */
	for (i=0; i<levels; i++) {

		mem_threshold_set(i);

		err = test->exec();
		if (err == 0) {
			/* success, stop now */
			break;
		}
		else if (err == ENOMEM) {
			/* OOM, as expected */
			err = 0;
		}
		else if (err == ETIMEDOUT) {
			/* test timed out, stop now */
			err = 0;
			goto out;
		}
		else if (err == ENOSYS) {
			err = 0;
			break;
		}
		else if (err == ESKIPPED) {
			err = 0;
			break;
		}
		else {
			DEBUG_WARNING("oom: %s: unexpected error code at"
				      " %d blocks free (%m)\n",
				      test->name, i, err);
			goto out;
		}
	}

 out:
	if (verbose)
		(void)re_fprintf(stderr, "oom max %d\n", i);

	return err;
}


int test_oom(const char *name, bool verbose)
{
	size_t i;
	const int levels = 64;
	int err = 0;

	(void)re_fprintf(stderr, "oom tests %u levels: \n", levels);

	if (name) {
		const struct test *test = find_test(name);
		if (!test) {
			(void)re_fprintf(stderr, "no such test: %s\n", name);
			return ENOENT;
		}

		err = testcase_oom(test, levels, verbose);
	}
	else {
		/* All test cases */
		for (i=0; i<ARRAY_SIZE(tests); i++) {
			err = testcase_oom(&tests[i], levels, verbose);
			if (err)
				break;
		}
	}

	mem_threshold_set(-1);

	if (err) {
		DEBUG_WARNING("oom: %m\n", err);
	}
	else {
		(void)re_fprintf(stderr, "\x1b[32mOK\x1b[;m\t\n");
	}

	return err;
}


static int test_unit(const char *name, bool verbose)
{
	size_t skipv[ARRAY_SIZE(tests)] = {0};
	size_t i;
	int err = 0;

	if (name) {
		const struct test *test = find_test(name);
		if (!test) {
			(void)re_fprintf(stderr, "no such test: %s\n", name);
			return ENOENT;
		}

		err = test->exec();
		if (err) {
			DEBUG_WARNING("%s: test failed (%m)\n", name, err);
			return err;
		}
	}
	else {
		unsigned n_skipped = 0;

		for (i=0; i<ARRAY_SIZE(tests); i++) {

			if (verbose) {
				re_printf("test %u -- %s\n",
					  i, tests[i].name);
			}

			err = tests[i].exec();
			if (err) {
				if (err == ESKIPPED || err == ENOSYS) {

					skipv[n_skipped] = i;

					++n_skipped;
					err = 0;
					continue;
				}

				DEBUG_WARNING("%s: test failed (%m)\n",
					      tests[i].name, err);
				return err;
			}
		}

		if (n_skipped) {
			re_fprintf(stderr, "skipped:%u\n", n_skipped);

			/* show any skipped testcase */
			for (i=0; i<n_skipped; i++) {
				size_t ix = skipv[i];
				re_fprintf(stderr, "skip %s\n",
					   tests[ix].name );
			}
		}
	}

	return err;
}


static uint64_t tmr_microseconds(void)
{
	struct timeval now;
	uint64_t usec;

	if (0 != gettimeofday(&now, NULL)) {
		DEBUG_WARNING("jiffies: gettimeofday() failed (%m)\n", errno);
		return 0;
	}

	usec  = (uint64_t)now.tv_sec * (uint64_t)1000000;
	usec += now.tv_usec;

	return usec;
}


/* baseunits here is [usec] (micro-seconds) */
static int testcase_perf(const struct test *test, double *usec_avgp)
{
#define DRYRUN_MIN        2
#define DRYRUN_MAX      100
#define DRYRUN_USEC 10*1000

#define REPEATS_MIN         3
#define REPEATS_MAX      1000
#define REPEATS_USEC 100*1000

	uint64_t usec_start, usec_stop;
	double usec_avg;
	size_t i, n;
	int err = 0;

	/* dry run */
	usec_start = tmr_microseconds();
	for (i=0; i<DRYRUN_MAX; i++) {

		err = test->exec();
		if (err)
			return err;

		usec_stop = tmr_microseconds();

		if ((usec_stop - usec_start) > DRYRUN_USEC)
			break;
	}

	usec_avg = 1.0 * (usec_stop - usec_start) / i;

	n = usec_avg ? (REPEATS_USEC / usec_avg) : 0;
	n = min(REPEATS_MAX, max(n, REPEATS_MIN));

	/* now for the real measurement */
	usec_start = tmr_microseconds();
	for (i=0; i<n; i++) {
		err = test->exec();
		if (err)
			return err;
	}
	usec_stop = tmr_microseconds();

	if (usec_stop <= usec_start) {
		DEBUG_WARNING("perf: cannot measure, test is too fast\n");
		return EINVAL;
	}

	usec_avg = (1.0 * (usec_stop - usec_start)) / i;

	if (usec_avgp)
		*usec_avgp = usec_avg;

	re_printf("%-32s:  %10.2f usec  [%6u repeats]\n",
		  test->name, usec_avg, i);

	return 0;
}


struct timing {
	const struct test *test;
	uint64_t nsec_avg;
};


/*
 * The comparison function must return an integer less than, equal to,
 * or greater than zero if the first argument  is  considered to  be
 * respectively  less  than,  equal  to, or greater than the second.
 *
 * If two members compare as equal, their order in the sorted array
 * is undefined.
 */
static int timing_cmp(const void *p1, const void *p2)
{
	const struct timing *v1 = p1;
	const struct timing *v2 = p2;

	if (v1->nsec_avg < v2->nsec_avg)
		return 1;
	else if (v1->nsec_avg > v2->nsec_avg)
		return -1;
	else
		return 0;
}


int test_perf(const char *name, bool verbose)
{
	int err = 0;
	unsigned i;
	(void)verbose;

	if (name) {
		const struct test *test;

		test = find_test(name);
		if (!test) {
			(void)re_fprintf(stderr, "no such test: %s\n", name);
			return ENOENT;
		}

		err = testcase_perf(test, NULL);
		if (err)
			return err;
	}
	else {
		struct timing timingv[ARRAY_SIZE(tests)];

		memset(&timingv, 0, sizeof(timingv));

		/* All test cases */
		for (i=0; i<ARRAY_SIZE(tests); i++) {

			struct timing *tim = &timingv[i];
			double usec_avg;

			tim->test = &tests[i];

			err = testcase_perf(&tests[i],
					    &usec_avg);
			if (err) {
				if (err == ESKIPPED || err == ENOSYS) {
					re_printf("skipped: %s\n",
						  tests[i].name);
					tim->test = NULL;
					continue;
				}
				DEBUG_WARNING("perf: %s failed (%m)\n",
					      tests[i].name, err);
				return err;
			}

			tim->nsec_avg = 1000.0 * usec_avg;
		}

		/* sort the timing table by average time */
		qsort(timingv, ARRAY_SIZE(timingv), sizeof(timingv[0]),
		      timing_cmp);

		re_fprintf(stderr,
			   "\nsorted by average timing (slowest on top):\n");

		for (i=0; i<ARRAY_SIZE(timingv); i++) {

			struct timing *tim = &timingv[i];
			double usec_avg = tim->nsec_avg / 1000.0;

			if (!tim->test)
				continue;

			re_fprintf(stderr, "%-32s: %10.2f usec\n",
				   tim->test->name, usec_avg);
		}
		re_fprintf(stderr, "\n");
	}

	return 0;
}


int test_reg(const char *name, bool verbose)
{
	int err;

	(void)re_fprintf(stderr, "regular tests:       ");
	err = test_unit(name, verbose);
	if (err)
		return err;
	(void)re_fprintf(stderr, "\x1b[32mOK\x1b[;m\n");

	return err;
}


#ifdef HAVE_PTHREAD
struct thread {
	const struct test *test;
	pthread_t tid;
	int err;
};


static void *thread_handler(void *arg)
{
	struct thread *thr = arg;
	int err;

	err = re_thread_init();
	if (err) {
		DEBUG_WARNING("thread: re_thread_init failed %m\n", err);
		thr->err = err;
		return NULL;
	}

	err = thr->test->exec();
	if (err) {
		if (err == ESKIPPED) {
			err = 0;
		}
		else {
			DEBUG_WARNING("%s: test failed (%m)\n",
					thr->test->name, err);
		}
	}

	re_thread_close();

	/* safe to write it, main thread is waiting for us */
	thr->err = err;

	return NULL;
}


/* Run all test-cases in multiple threads */
int test_multithread(void)
{
#define NUM_REPEAT 2
#define NUM_TOTAL  (NUM_REPEAT * ARRAY_SIZE(tests))

	struct thread threadv[NUM_TOTAL];
	unsigned n=0;
	size_t test_index=0;
	size_t i;
	int err = 0;

	memset(threadv, 0, sizeof(threadv));

	(void)re_fprintf(stderr, "multithread: %u tests"
			 " with %d repeats (total %u threads): ",
			 ARRAY_SIZE(tests), NUM_REPEAT, NUM_TOTAL);

	for (i=0; i<ARRAY_SIZE(threadv); i++) {

		size_t ti = (test_index++ % ARRAY_SIZE(tests));

		threadv[i].test = &tests[ti];
		threadv[i].err = -1;           /* error not set */

		err = pthread_create(&threadv[i].tid, NULL,
				     thread_handler, (void *)&threadv[i]);
		if (err) {
			DEBUG_WARNING("pthread_create failed (%m)\n", err);
			break;
		}

		++n;
	}

	for (i=0; i<ARRAY_SIZE(threadv); i++) {

		pthread_join(threadv[i].tid, NULL);
	}

	for (i=0; i<ARRAY_SIZE(threadv); i++) {

		if (threadv[i].err != 0) {
			re_printf("%u failed: %-30s  [%d] [%m]\n", i,
				  threadv[i].test->name,
				  threadv[i].err, threadv[i].err);
			err = threadv[i].err;
		}
	}

	if (err)
		return err;
	(void)re_fprintf(stderr, "\x1b[32mOK\x1b[;m\n");

	return err;
}
#endif


void test_listcases(void)
{
	size_t i, n;

	n = ARRAY_SIZE(tests);

	(void)re_printf("\n%u test cases:\n", n);

	for (i=0; i<(n+1)/2; i++) {

		(void)re_printf("    %-32s    %s\n",
				tests[i].name,
				(i+(n+1)/2) < n ? tests[i+(n+1)/2].name : "");
	}

	(void)re_printf("\n");
}


void test_hexdump_dual(FILE *f,
		       const void *ep, size_t elen,
		       const void *ap, size_t alen)
{
	const uint8_t *ebuf = ep;
	const uint8_t *abuf = ap;
	size_t i, j, len;
#define WIDTH 8

	if (!f || !ep || !ap)
		return;

	len = max(elen, alen);

	(void)re_fprintf(f, "\nOffset:   Expected (%zu bytes):    "
			 "   Actual (%zu bytes):\n", elen, alen);

	for (i=0; i < len; i += WIDTH) {

		(void)re_fprintf(f, "0x%04zx   ", i);

		for (j=0; j<WIDTH; j++) {
			const size_t pos = i+j;
			if (pos < elen) {
				bool wrong = pos >= alen;

				if (wrong)
					(void)re_fprintf(f, "\x1b[35m");
				(void)re_fprintf(f, " %02x", ebuf[pos]);
				if (wrong)
					(void)re_fprintf(f, "\x1b[;m");
			}
			else
				(void)re_fprintf(f, "   ");
		}

		(void)re_fprintf(f, "    ");

		for (j=0; j<WIDTH; j++) {
			const size_t pos = i+j;
			if (pos < alen) {
				bool wrong;

				if (pos < elen)
					wrong = ebuf[pos] != abuf[pos];
				else
					wrong = true;

				if (wrong)
					(void)re_fprintf(f, "\x1b[33m");
				(void)re_fprintf(f, " %02x", abuf[pos]);
				if (wrong)
					(void)re_fprintf(f, "\x1b[;m");
			}
			else
				(void)re_fprintf(f, "   ");
		}

		(void)re_fprintf(f, "\n");
	}

	(void)re_fprintf(f, "\n");
}


static void oom_watchdog_timeout(void *arg)
{
	int *err = arg;

	*err = ETIMEDOUT;

	re_cancel();
}


static void signal_handler(int sig)
{
	re_fprintf(stderr, "test interrupted by signal %d\n", sig);
	re_cancel();
}


int re_main_timeout(uint32_t timeout_ms)
{
	struct tmr tmr;
	int err = 0;

	tmr_init(&tmr);

#ifdef TEST_TIMEOUT
	timeout_ms = TEST_TIMEOUT;
#endif

	tmr_start(&tmr, timeout_ms, oom_watchdog_timeout, &err);
	(void)re_main(signal_handler);

	tmr_cancel(&tmr);
	return err;
}


int test_load_file(struct mbuf *mb, const char *filename)
{
	int err = 0, fd = open(filename, O_RDONLY);
	if (fd < 0)
		return errno;

	for (;;) {
		uint8_t buf[1024];

		const ssize_t n = read(fd, (void *)buf, sizeof(buf));
		if (n < 0) {
			err = errno;
			break;
		}
		else if (n == 0)
			break;

		err = mbuf_write_mem(mb, buf, n);
		if (err)
			break;
	}

	(void)close(fd);

	return err;
}


int test_write_file(struct mbuf *mb, const char *filename)
{
	int err = 0, fd = open(filename, O_CREAT | O_WRONLY, 0644);
	if (fd < 0)
		return errno;

	for (;;) {
		uint8_t buf[1024];
		ssize_t n;

		n = min(sizeof(buf), mbuf_get_left(mb));
		if (n == 0)
			break;

		err = mbuf_read_mem(mb, buf, n);
		if (err)
			break;

		n = write(fd, (void *)buf, n);
		if (n < 0) {
			err = errno;
			break;
		}
		else if (n == 0)
			break;

	}

	(void)close(fd);

	return err;
}


void test_set_datapath(const char *path)
{
	str_ncpy(datapath, path, sizeof(datapath));
}


const char *test_datapath(void)
{
	return datapath;
}
