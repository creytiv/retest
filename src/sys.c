/**
 * @file sys.c System Testcode
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include <ctype.h>
#include <string.h>
#include <re.h>
#include "test.h"


#define DEBUG_MODULE "test_sys"
#define DEBUG_LEVEL 4
#include <re_dbg.h>


int test_sys_div(void)
{
	static const struct {
		uint64_t a, b, res;
	} testv[] = {
		{15, 5, 3},
		{0x100000000ULL, 0x10000UL, 0x10000},
		{846744073709551616ULL, 8474407795116ULL, 99917},
		{1446744073709551615ULL, 2, 723372036854775807ULL}
	};
	uint32_t i;
	int err = 0;

	for (i=0; i<ARRAY_SIZE(testv); i++) {
		const uint64_t res = testv[i].a / testv[i].b;
		if (res != testv[i].res) {
			DEBUG_WARNING("div test %u failed (%llu)\n", i, res);
			err = EDOM;
		}
	}

	return err;
}


int test_sys_endian(void)
{
	uint16_t s_le, s_ho;
	uint8_t *s = (uint8_t *)&s_le;
	uint32_t l_le, l_ho;
	uint8_t *l = (uint8_t *)&l_le;
	uint64_t ll0, ll1 = 0x0102030405060708ULL;

	/* Little endian: LSB first - 0x1234
	 *
	 * 0x0000: 0x34
	 * 0x0001: 0x12
	 */

	s[0] = 0x34;
	s[1] = 0x12;

	s_ho = sys_ltohs(s_le);
	if (0x1234 != s_ho) {
		DEBUG_WARNING("endian short: 0x%04x\n", s_ho);
		return EINVAL;
	}

	if (s_le != sys_htols(s_ho)) {
		DEBUG_WARNING("sys_htols failed: 0x%04x\n", sys_htols(s_ho));
		return EINVAL;
	}

	/* 0x12345678
	 *
	 * 0x0000: 0x78
	 * 0x0001: 0x56
	 * 0x0002: 0x34
	 * 0x0003: 0x12
	 */

	l[0] = 0x78;
	l[1] = 0x56;
	l[2] = 0x34;
	l[3] = 0x12;

	l_ho = sys_ltohl(l_le);
	if (0x12345678 != l_ho) {
		DEBUG_WARNING("endian long: 0x%08x\n", l_ho);
		return EINVAL;
	}

	if (l_le != sys_htoll(l_ho)) {
		DEBUG_WARNING("sys_htoll failed: 0x%08x\n", sys_htoll(l_ho));
		return EINVAL;
	}

	/* Test 64-bit */
	ll0 = sys_ntohll(sys_htonll(ll1));

	if (ll0 != ll1) {
		DEBUG_WARNING("endian long-long: 0x%llx\n", ll0);
		return EINVAL;
	}

	return 0;
}


int test_sys_rand(void)
{
	char str[64];
	uint8_t buf[64];
	size_t i;
	int err = 0;

	volatile uint16_t u16 = rand_u16();
	volatile uint32_t u32 = rand_u32();
	volatile uint64_t u64 = rand_u64();
	volatile char ch      = rand_char();

	(void)u16;
	(void)u32;
	(void)u64;

	TEST_ASSERT(ch > 0);
	TEST_ASSERT(isprint(ch));

	rand_str(str, sizeof(str));
	rand_bytes(buf, sizeof(buf));

	for (i = 0; i < (sizeof(str)-1); i++) {
		TEST_ASSERT(str[i] > 0);
		TEST_ASSERT(isprint(str[i]));
	}

 out:
	return err;
}
