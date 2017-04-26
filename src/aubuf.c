/**
 * @file aubuf.c Audio-buffer Testcode
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include <string.h>
#include <re.h>
#include <rem.h>
#include "test.h"


#define DEBUG_MODULE "test_aubuf"
#define DEBUG_LEVEL 5
#include <re_dbg.h>


int test_aubuf(void)
{
	struct aubuf *ab = NULL;
	int16_t sampv_in[160];
	int16_t sampv_out[160];
	unsigned i;
	int err;

	for (i=0; i<ARRAY_SIZE(sampv_in); i++)
		sampv_in[i] = i;
	memset(sampv_out, 0, sizeof(sampv_out));

	err = aubuf_alloc(&ab, 320, 0);
	if (err)
		goto out;

	TEST_EQUALS(0, aubuf_cur_size(ab));

	err |= aubuf_write_samp(ab,  sampv_in, 80);
	err |= aubuf_write_samp(ab, &sampv_in[80], 80);
	if (err)
		goto out;

	TEST_EQUALS(320, aubuf_cur_size(ab));

	aubuf_read_samp(ab, sampv_out, ARRAY_SIZE(sampv_out));

	TEST_MEMCMP(sampv_in, sizeof(sampv_in), sampv_out, sizeof(sampv_out));
	TEST_EQUALS(0, aubuf_cur_size(ab));

 out:
	mem_deref(ab);
	return err;
}
