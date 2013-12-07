/**
 * @file vid.c Video Testcode
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include <string.h>
#include <re.h>
#include <rem.h>
#include "test.h"


#define DEBUG_MODULE "vid"
#define DEBUG_LEVEL 5
#include <re_dbg.h>


static int test_vidsz_cmp(void)
{
	struct vidsz a = {64, 64}, b = {64, 64}, c = {32, 32};

	TEST_ASSERT(!vidsz_cmp(NULL, NULL));
	TEST_ASSERT( vidsz_cmp(&a, &a));
	TEST_ASSERT( vidsz_cmp(&a, &b));
	TEST_ASSERT(!vidsz_cmp(&a, &c));

	return 0;
}


static int test_vidrect_cmp(void)
{
	struct vidrect a = {10, 10, 30, 30};
	struct vidrect b = {10, 10, 30, 30};
	struct vidrect c = {10, 10, 40, 40};
	struct vidrect d = {20, 20, 30, 30};

	TEST_ASSERT(!vidrect_cmp(NULL, NULL));
	TEST_ASSERT( vidrect_cmp(&a, &a));
	TEST_ASSERT( vidrect_cmp(&a, &b));
	TEST_ASSERT(!vidrect_cmp(&a, &c));
	TEST_ASSERT(!vidrect_cmp(&a, &d));

	return 0;
}


int test_vid(void)
{
	int err;

	err = test_vidsz_cmp();
	if (err)
		return err;

	err = test_vidrect_cmp();
	if (err)
		return err;

	return err;
}
