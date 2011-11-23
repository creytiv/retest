/**
 * @file test.c  Regression testcode
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include <string.h>
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
	TEST(test_base64),
	TEST(test_bfcp),
	TEST(test_bfcp_bin),
	TEST(test_conf),
	TEST(test_crc32),
	TEST(test_fmt_pl),
	TEST(test_fmt_pl_u32),
	TEST(test_fmt_pl_u64),
	TEST(test_fmt_pl_x3264),
	TEST(test_fmt_print),
	TEST(test_fmt_regex),
	TEST(test_fmt_snprintf),
	TEST(test_fmt_str),
	TEST(test_g711_alaw),
	TEST(test_g711_ulaw),
	TEST(test_hash),
	TEST(test_hmac_sha1),
	TEST(test_jbuf),
	TEST(test_list),
	TEST(test_list_ref),
	TEST(test_mbuf),
	TEST(test_mbuf_human_time),
	TEST(test_md5),
	TEST(test_mem),
	TEST(test_rtp),
	TEST(test_rtcp_encode),
	TEST(test_rtcp_decode),
	TEST(test_sa_class),
	TEST(test_sa_cmp),
	TEST(test_sa_decode),
	TEST(test_sa_ntop),
	TEST(test_sdp_all),
	TEST(test_sdp_bfcp),
	TEST(test_sdp_parse),
	TEST(test_sdp_oa),
	TEST(test_sha1),
	TEST(test_sip_addr),
	TEST(test_sip_apply),
	TEST(test_sip_hdr),
	TEST(test_sip_param),
	TEST(test_sip_parse),
	TEST(test_sip_via),
	TEST(test_sipsess),
	TEST(test_stun_req),
	TEST(test_stun_resp),
	TEST(test_stun_reqltc),
	TEST(test_sys_div),
	TEST(test_sys_endian),
	TEST(test_tcp),
	TEST(test_telev),
	TEST(test_tmr),
	TEST(test_turn),
	TEST(test_udp),
	TEST(test_uri),
	TEST(test_uri_cmp),
	TEST(test_uri_encode),
	TEST(test_uri_headers),
	TEST(test_uri_user),
	TEST(test_uri_params_headers),
	TEST(test_vidconv),
};


typedef int (ftest_exec_h)(struct mbuf *mb);

struct ftest {
	ftest_exec_h *exec;
	const char *name;
};

#define FTEST(a) {a, #a}

static const struct ftest fuztests[] = {
	FTEST(fuzzy_bfcp),
	FTEST(fuzzy_rtp),
	FTEST(fuzzy_rtcp),
	FTEST(fuzzy_sipmsg),
	FTEST(fuzzy_stunmsg),
	FTEST(fuzzy_sdpsess),
};


static const struct test *find_test(const char *name)
{
	size_t i;

	for (i=0; i<ARRAY_SIZE(tests); i++) {

		if (0 == str_casecmp(name, tests[i].name))
			return &tests[i];
	}

	return NULL;
}


static const struct ftest *find_ftest(const char *name)
{
	size_t i;

	for (i=0; i<ARRAY_SIZE(fuztests); i++) {

		if (0 == str_casecmp(name, fuztests[i].name))
			return &fuztests[i];
	}

	return NULL;
}


static int testcase_oom(const struct test *test, int levels, int *max_alloc)
{
	int j;
	int err = 0;
	bool oom = false;

	(void)re_fprintf(stderr, "  %-24s: ", test->name);

	/* All memory levels */
	for (j=levels; j>=0; j--) {
		mem_threshold_set(j);

		err = test->exec();
		if (!err)
			continue;

		if (ENOMEM == err) {
			*max_alloc = max(j, *max_alloc);
			if (!oom) {
				(void)re_fprintf(stderr, "max %d\n", j);
				if (j >= (int)levels) {
					DEBUG_WARNING("levels=%u\n",
						      levels);
				}
			}
			oom = true;
			continue;
		}

		DEBUG_WARNING("%s: oom threshold=%u: %s\n",
			      test->name, j, strerror(err));
		break;
	}

	if (err && ENOMEM != err) {
		DEBUG_WARNING("%s: oom test failed (%s)\n", test->name,
			      strerror(err));
		return err;
	}
	else if (0 == err) {
		(void)re_fprintf(stderr, "no allocs\n");
	}

	return 0;
}


int test_oom(const char *name)
{
	size_t i;
	int max_alloc = 0;
	const int levels = 100;
	int err = 0;

	(void)re_fprintf(stderr, "oom tests %u levels: \n", levels);

	if (name) {
		const struct test *test = find_test(name);
		if (!test) {
			(void)re_fprintf(stderr, "no such test: %s\n", name);
			return ENOENT;
		}

		err = testcase_oom(test, levels, &max_alloc);
	}
	else {
		/* All test cases */
		for (i=0; i<ARRAY_SIZE(tests) && !err; i++) {
			err = testcase_oom(&tests[i], levels, &max_alloc);
		}
	}

	mem_threshold_set(-1);

	if (err) {
		DEBUG_WARNING("oom: %s\n", strerror(err));
	}
	else {
		(void)re_fprintf(stderr, "\x1b[32mOK\x1b[;m\t"
				 "(max alloc %d)\n", max_alloc);
	}

	return err;
}


static int test_unit(const char *name)
{
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
			DEBUG_WARNING("%s: test failed (%s)\n",
				      name, strerror(err));
			return err;
		}
	}
	else {
		for (i=0; i<ARRAY_SIZE(tests); i++) {
			err = tests[i].exec();
			if (err) {
				DEBUG_WARNING("%s: test failed (%s)\n",
					      tests[i].name, strerror(err));
				return err;
			}
		}
	}

	return err;
}


int test_perf(const char *name, uint32_t n)
{
	uint64_t tick, tock;
	uint32_t i;

	(void)re_fprintf(stderr, "performance tests:   ");

	tick = tmr_jiffies();

	for (i=0; i<n; i++) {
		int err;

		err = test_unit(name);
		if (err)
			return err;
	}

	tock = tmr_jiffies();

	(void)re_fprintf(stderr, "\x1b[32mOK\x1b[;m");

	(void)re_fprintf(stderr, "\t(%u tests took %lu ms)\n",
			 n, (uint32_t)(tock - tick));

	return 0;
}


int test_reg(const char *name)
{
	int err;

	(void)re_fprintf(stderr, "regular tests:       ");
	err = test_unit(name);
	if (err)
		return err;
	(void)re_fprintf(stderr, "\x1b[32mOK\x1b[;m\n");

	return err;
}


int test_fuzzy(const char *name)
{
	struct mbuf *mb;
	uint16_t len;
	size_t i;
	int err = 0;
	static size_t n = 0;

	len = rand_u16();

	(void)re_fprintf(stderr, "\r%u: %u bytes    ", n++, len);

	mb = mbuf_alloc(len);
	if (!mb)
		return ENOMEM;

	rand_bytes(mb->buf, len);
	mb->end = len;

	if (name) {
		const struct ftest *test = find_ftest(name);
		if (!test) {
			(void)re_fprintf(stderr, "no such test: %s\n", name);
			err = ENOENT;
			goto out;
		}

		err = test->exec(mb);
	}
	else {
		for (i=0; i<ARRAY_SIZE(fuztests) && !err; i++) {
			mb->pos = 0;
			err = fuztests[i].exec(mb);
		}
	}

 out:
	mem_deref(mb);
	return err;
}


void test_listcases(void)
{
	size_t i, n;

	n = ARRAY_SIZE(tests);

	(void)re_printf("\n%u test cases:\n", n);

	for (i=0; i<(n+1)/2; i++) {

		(void)re_printf("    %-32s    %s\n",
				tests[i].name, tests[i+(n+1)/2].name);
	}

	(void)re_printf("\n%u fuzzy test cases:\n", ARRAY_SIZE(fuztests));

	for (i=0; i<ARRAY_SIZE(fuztests); i++) {

		(void)re_printf("    %s\n", fuztests[i].name);
	}

	(void)re_printf("\n");
}
