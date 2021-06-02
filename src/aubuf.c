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

#define AUDIO_TIMEBASE 1000000U


static int test_aubuf_raw(void)
{
	struct aubuf *ab = NULL;
	int16_t sampv_in[160];
	int16_t sampv_out[160];
	struct mbuf *mb;
	unsigned i;
	int err;

	mb = mbuf_alloc(80 * sizeof(int16_t));
	if (!mb)
		return ENOMEM;

	for (i=0; i<ARRAY_SIZE(sampv_in); i++)
		sampv_in[i] = i;
	memset(sampv_out, 0, sizeof(sampv_out));

	err = aubuf_alloc(&ab, 320, 0);
	TEST_ERR(err);
	TEST_EQUALS(0, aubuf_cur_size(ab));

	err = aubuf_write(ab, (uint8_t *)sampv_in, 80 * sizeof(int16_t));
	TEST_ERR(err);
	TEST_EQUALS(160, aubuf_cur_size(ab));

	(void)mbuf_write_mem(mb, (uint8_t *)&sampv_in[80],
			     80 * sizeof(int16_t));
	mb->pos = 0;

	err = aubuf_append(ab, mb);
	TEST_ERR(err);
	TEST_EQUALS(320, aubuf_cur_size(ab));

	memset(sampv_out, 0, sizeof(sampv_out));
	aubuf_read(ab, (uint8_t *)sampv_out, 160 * sizeof(int16_t));
	TEST_MEMCMP(sampv_in, sizeof(sampv_in), sampv_out, sizeof(sampv_out));
	TEST_EQUALS(0, aubuf_cur_size(ab));

 out:
	mem_deref(ab);
	mem_deref(mb);
	return err;
}


static int test_aubuf_samp(void)
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
	TEST_ERR(err);

	TEST_EQUALS(0, aubuf_cur_size(ab));

	err |= aubuf_write_samp(ab,  sampv_in, 80);
	err |= aubuf_write_samp(ab, &sampv_in[80], 80);
	TEST_ERR(err);

	TEST_EQUALS(320, aubuf_cur_size(ab));

	aubuf_read_samp(ab, sampv_out, ARRAY_SIZE(sampv_out));
	TEST_MEMCMP(sampv_in, sizeof(sampv_in), sampv_out, sizeof(sampv_out));
	TEST_EQUALS(0, aubuf_cur_size(ab));

 out:
	mem_deref(ab);
	return err;
}


static int test_aubuf_auframe(void)
{
	struct aubuf *ab = NULL;
	float sampv_in[160];
	float sampv_out[160];

	struct auframe af_in;
	struct auframe af_out;
	unsigned i;
	int err;

	for (i=0; i<ARRAY_SIZE(sampv_in); i++)
		sampv_in[i] = (float)i;
	memset(sampv_out, 0, sizeof(sampv_out));

	err = aubuf_alloc(&ab, 320, 0);
	TEST_ERR(err);

	TEST_EQUALS(0, aubuf_cur_size(ab));

	auframe_init(&af_in, AUFMT_FLOAT, sampv_in, 80, 48000, 2);
	af_in.timestamp = 0;

	err |= aubuf_write_auframe(ab, &af_in);

	af_in.sampv = &sampv_in[80];
	af_in.sampc = 80;
	af_in.timestamp = 80 * AUDIO_TIMEBASE / (af_in.srate * af_in.ch);
	err |= aubuf_write_auframe(ab, &af_in);
	TEST_ERR(err);

	TEST_EQUALS(160 * sizeof(float), aubuf_cur_size(ab));

	af_out.fmt = AUFMT_FLOAT;
	af_out.sampv = sampv_out;
	af_out.sampc = 40;
	aubuf_read_auframe(ab, &af_out);
	TEST_EQUALS(120 * sizeof(float), aubuf_cur_size(ab));
	TEST_EQUALS(0, af_out.timestamp);

	af_out.sampv = &sampv_out[40];
	af_out.sampc = 40;
	aubuf_read_auframe(ab, &af_out);
	TEST_EQUALS(80 * sizeof(float), aubuf_cur_size(ab));
	TEST_EQUALS(40 * AUDIO_TIMEBASE / (af_in.srate * af_in.ch),
		    af_out.timestamp);

	af_out.sampv = &sampv_out[80];
	af_out.sampc = 80;
	aubuf_read_auframe(ab, &af_out);

	TEST_EQUALS(2, af_out.ch);
	TEST_EQUALS(48000, af_out.srate);
	TEST_EQUALS(80 * AUDIO_TIMEBASE / (af_in.srate * af_in.ch),
		    af_out.timestamp);

	TEST_MEMCMP(sampv_in, sizeof(sampv_in), sampv_out, sizeof(sampv_out));
	TEST_EQUALS(0, aubuf_cur_size(ab));

 out:
	mem_deref(ab);
	return err;
}


int test_aubuf(void)
{
	int err;

	err = test_aubuf_raw();
	TEST_ERR(err);

	err = test_aubuf_samp();
	TEST_ERR(err);

	err = test_aubuf_auframe();
	TEST_ERR(err);

out:
	return err;
}
