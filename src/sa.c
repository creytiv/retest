/**
 * @file sa.c Socket address Testcode
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include <string.h>
#include <re.h>
#include "test.h"


#define DEBUG_MODULE "test_sa"
#define DEBUG_LEVEL 5
#include <re_dbg.h>


int test_sa_cmp(void)
{
	const struct {
		const char *host1;
		uint16_t port1;
		const char *host2;
		uint16_t port2;
		bool eq;
	} testv[] = {
		{
			"1.2.3.4", 12345,
			"1.2.3.4", 12345,
			true
		},
		{
			"1.2.3.4", 12345,
			"1.2.3.5", 12345,
			false
		},
		{
			"1.2.3.4", 12345,
			"1.2.3.4", 12344,
			false
		},
#ifdef HAVE_INET6
		{
			"0:0:0:0:0:0:0:0", 123,
			"::", 123,
			true
		},
		{
			"0:0:0:0:0:0:0:1", 123,
			"::1", 123,
			true
		},
		{
			"0:0:0:0:0:0:1:1", 123,
			"::1", 123,
			false
		},
		{
			"2001:0:53aa:64c:0:fbff:ab2e:1eac",        2001,
			"2001:0000:53aa:064c:0000:fbff:ab2e:1eac", 2001,
			true
		},
		{
			"3001:0:53aa:64c:0:fbff:ab2e:1eac",        2001,
			"2001:0000:53aa:064c:0000:fbff:ab2e:1eac", 2001,
			false
		},
		{
			"192.168.1.1",                             123,
			"2001:0000:53aa:064c:0000:fbff:ab2e:1eac", 123,
			false
		},
		{ /* IPv6-mapped IPv4-address */
			"::ffff:208.68.208.201",                   3478,
			"208.68.208.201",                          3478,
			true
		}
#endif
	};
	size_t i;
	int err = 0;

	for (i=0; i<ARRAY_SIZE(testv); i++) {
		struct sa sa1, sa2;
		bool eq;

		err = sa_set_str(&sa1, testv[i].host1, testv[i].port1);
		if (err)
			break;
		err = sa_set_str(&sa2, testv[i].host2, testv[i].port2);
		if (err)
			break;

		eq = sa_cmp(&sa1, &sa2, SA_ALL);
		if (!testv[i].eq == !eq)
			continue;

		DEBUG_WARNING("sa cmp %u: (%J) (%J) expected (%d),"
			      " got (%d)\n", i, &sa1, &sa2,
			      testv[i].eq, eq);
		return EINVAL;
	}

	return err;
}


int test_sa_decode(void)
{
	const struct {
		int err;
		int af;
		const char *str;
		const char *addr;
		uint16_t port;
	} testv[] = {
		{0,      AF_INET,  "1.2.3.4:1234",  "1.2.3.4", 1234},
		{0,      AF_INET,  "1.2.3.4:0",     "1.2.3.4", 0},
		{EINVAL, AF_INET,  "1.2.3.4",       "",        0},
		{EINVAL, AF_INET,  "1.2.3.4.:1234", "",        0},
#ifdef HAVE_INET6
		{0, AF_INET6, "[::1]:1", "::1", 1},
		{0, AF_INET6, "[fe80:c827:1507:7707:7b75:5489:feb7:2c45]:3333",
		 "fe80:c827:1507:7707:7b75:5489:feb7:2c45", 3333},
		{EINVAL, AF_INET6, "[::1]", "", 0},
#endif
	};
	uint32_t i;
	int err = 0;

	for (i=0; i<ARRAY_SIZE(testv); i++) {
		struct sa sa, sa2;
		char buf[64];
		int e;

		e = sa_decode(&sa, testv[i].str, strlen(testv[i].str));
		if (testv[i].err != e) {
			DEBUG_WARNING("sa_decode: test %u:"
				      " expected (%m) got (%m) [%s]\n", i,
				      testv[i].err, err, testv[i].str);
			err = EINVAL;
			break;
		}
		if (e)
			continue;

		if (testv[i].af != sa_af(&sa)) {
			DEBUG_WARNING("sa_decode: af mismatch %d != %d\n",
				      testv[i].af, sa_af(&sa));
			err = EINVAL;
			break;
		}

		err = sa_set_str(&sa2, testv[i].addr, testv[i].port);
		if (err)
			break;

		if (!sa_cmp(&sa, &sa2, SA_ALL)) {
			DEBUG_WARNING("sa_decode: sa_cmp() failed\n");
			err = EINVAL;
			break;
		}

		(void)re_snprintf(buf, sizeof(buf), "%J", &sa);
		if (0 != strcmp(testv[i].str, buf)) {
			DEBUG_WARNING("%u: strcmp: %s != %s\n",
				      testv[i].str, buf);
			err = EINVAL;
			break;
		}
	}

	return err;
}


/* Test classification of loopback and link-local IP address */
int test_sa_class(void)
{
	const struct {
		bool lo;
		bool ll;
		bool any;
		const char *addr;
	} testv[] = {
		{false, false, true,  "0.0.0.0"},
		{false, false, false, "1.2.3.4"},
		{false, false, false, "127.0.0.0"},
		{true,  false, false, "127.0.0.1"},
		{false, true,  false, "169.254.1.2"},
#ifdef HAVE_INET6
		{false, false, true,  "::"},
		{true,  false, false, "::1"},
		{false, true,  false,
		 "fe80:c827:1507:7707:7b75:5489:feb7:2c45"},
		{false, false, false, "2610:a0:c779:b::d1ad:35b4"}
#endif
	};
	uint32_t i;
	int err;

	for (i=0; i<ARRAY_SIZE(testv); i++) {
		struct sa sa;
		int lo, ll, any;

		err = sa_set_str(&sa, testv[i].addr, 0);
		if (err)
			goto out;

		lo = sa_is_loopback(&sa);
		if ((int)testv[i].lo != lo) {
			DEBUG_WARNING("%u: %s: loopback mismatch %d!=%d\n",
				      i, testv[i].addr, testv[i].lo, lo);
			err = EINVAL;
			goto out;
		}

		ll = sa_is_linklocal(&sa);
		if ((int)testv[i].ll != ll) {
			DEBUG_WARNING("%u: %s: linklocal mismatch %d!=%d\n",
				      i, testv[i].addr, testv[i].ll, ll);
			err = EINVAL;
			goto out;
		}

		any = sa_is_any(&sa);
		if ((int)testv[i].any != any) {
			DEBUG_WARNING("%u: %s: any mismatch %d!=%d\n",
				      i, testv[i].addr, testv[i].any, any);
			err = EINVAL;
			goto out;
		}
	}

#if 0
	{
		struct sa sax;
		TEST_ASSERT(sizeof(sax.u) <= sizeof(sax.u.padding));
	}
#endif

 out:
	return err;
}


int test_sa_ntop(void)
{
	const struct {
		int af;
		const char *addr;
	} testv[] = {
		{AF_INET,  "0.0.0.0"},
		{AF_INET,  "1.2.3.4"},
		{AF_INET,  "255.254.253.128"},
#ifdef HAVE_INET6
		{AF_INET6, "::1"},
		{AF_INET6, "fe80:c827:1507:7707:7b75:5489:feb7:2c45"},
		{AF_INET6, "2610:a0:c779:b::d1ad:35b4"}
#endif
	};
	uint32_t i;
	int err = 0;

	for (i=0; i<ARRAY_SIZE(testv); i++) {
		struct sa sa0, sa;
		char buf[64];

		err = sa_set_str(&sa0, testv[i].addr, 0);
		if (err)
			break;

		if (testv[i].af != sa_af(&sa0)) {
			DEBUG_WARNING("ntop: af mismatch %d != %d\n",
				      testv[i].af, sa_af(&sa0));
			err = EINVAL;
			break;
		}

		err = sa_ntop(&sa0, buf, sizeof(buf));
		if (err)
			break;

		if (0 != strcmp(buf, testv[i].addr)) {
			DEBUG_WARNING("ntop: addr mismatch (%s) != (%s)\n",
				      testv[i].addr, buf);
			err = EINVAL;
			break;
		}

		err = sa_set_sa(&sa, &sa0.u.sa);
		if (err) {
			DEBUG_WARNING("sa_set_sa: %m\n", err);
			break;
		}
		if (testv[i].af != sa_af(&sa)) {
			err = EINVAL;
			DEBUG_WARNING("af mismatch (test=%d sa=%d)\n",
				      testv[i].af, sa_af(&sa));
			break;
		}
	}

	return err;
}

