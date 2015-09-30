/**
 * @file cplusplus.cpp C++ Testcode
 *
 * Copyright (C) 2010 - 2015 Creytiv.com
 */
#include <re.h>
#include <rem.h>
#include "test.h"


#define DEBUG_MODULE "cplusplus"
#define DEBUG_LEVEL 5
#include <re_dbg.h>


int test_cplusplus(void)
{
	const char *version = sys_libre_version_get();
	int err = 0;

	TEST_ASSERT(str_isset(version));

 out:
	return err;
}
