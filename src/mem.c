/**
 * @file mem.c Memory Testcode
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include <re.h>
#include "test.h"


#define DEBUG_MODULE "test_mem"
#define DEBUG_LEVEL 5
#include <re_dbg.h>


#define PATTERN 0xfcfcfcfc

struct obj {
	uint32_t pattern;
};

static void destructor(void *arg)
{
	struct obj *obj = arg;

	if (PATTERN != obj->pattern) {
		DEBUG_WARNING("destroy error: %08x\n", obj->pattern);
	}
}


int test_mem(void)
{
	struct obj *obj, *old;
	int err = EINVAL;

	obj = mem_alloc(sizeof(*obj), destructor);
	if (!obj)
		return ENOMEM;

	obj->pattern = PATTERN;

	if (mem_nrefs(obj) != 1)
		goto error;

	obj = mem_ref(obj);

	if (mem_nrefs(obj) != 2)
		goto error;

	mem_deref(obj);

	/* TODO: check this */
	old = obj;
	obj = mem_realloc(old, sizeof(*obj) + 16);
	if (!obj) {
		mem_deref(old);
		err = ENOMEM;
		goto error;
	}

	err = 0;

 error:
	mem_deref(obj);
	return err;
}


#ifndef SIZE_MAX
#define SIZE_MAX    (~((size_t)0))
#endif


int test_mem_reallocarray(void)
{
	void *a, *b;
	int err = 0;

	/* expect success */
	a = mem_reallocarray(NULL, 10, 10, NULL);
	if (!a)
		return ENOMEM;

	/* expect failure */
	b = mem_reallocarray(NULL, SIZE_MAX, SIZE_MAX, NULL);
	TEST_ASSERT(b == NULL);

 out:
	mem_deref(a);

	return err;
}


int test_mem_secure(void)
{
	int r, err = 0;

	/* compare */
	r = mem_seccmp(NULL, NULL, 42);
	TEST_ASSERT(r < 0);

	r = mem_seccmp((uint8_t *)"abc", (uint8_t *)"abc", 3);
	TEST_EQUALS(0, r);

	r = mem_seccmp((uint8_t *)"aaa", (uint8_t *)"bbb", 3);
	TEST_ASSERT(r > 0);

	r = mem_seccmp((uint8_t *)"ccc", (uint8_t *)"aaa", 3);
	TEST_ASSERT(r > 0);

 out:
	return err;
}
