/**
 * @file mbuf.c Mbuffer Testcode
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include <string.h>
#include <re.h>
#include "test.h"


#define DEBUG_MODULE "test_mbuf"
#define DEBUG_LEVEL 5
#include <re_dbg.h>


int test_mbuf(void)
{
	struct mbuf mb;
	struct pl pl, hei = PL("hei"), foo = PL("foo");
	char *str = NULL;
	int err;

	mbuf_init(&mb);

	/* write */
	err = mbuf_write_u8(&mb, 0x5a);
	if (err)
		goto out;
	err = mbuf_write_u16(&mb, 0x5a5a);
	if (err)
		goto out;
	err = mbuf_write_u32(&mb, 0x5a5a5a5a);
	if (err)
		goto out;
	err = mbuf_write_str(&mb, "hei foo");
	if (err)
		goto out;

	/* read */
	mb.pos = 0;
	if (0x5a != mbuf_read_u8(&mb)) {
		err = EINVAL;
		goto out;
	}
	if (0x5a5a != mbuf_read_u16(&mb)) {
		err = EINVAL;
		goto out;
	}
	if (0x5a5a5a5a != mbuf_read_u32(&mb)) {
		err = EINVAL;
		goto out;
	}
	pl.p = (char *)mbuf_buf(&mb);
	pl.l = 3;
	err = pl_cmp(&hei, &pl);
	if (err)
		goto out;

	mb.pos += 4;
	pl.p = (char *)mbuf_buf(&mb);
	pl.l = mbuf_get_left(&mb);
	err = pl_cmp(&foo, &pl);
	if (err)
		goto out;

	/* Test mbuf_strdup() */
	err = mbuf_strdup(&mb, &str, 3);
	if (err)
		goto out;
	err = pl_strcmp(&foo, str);

 out:
	mbuf_reset(&mb);
	mem_deref(str);

	return err;
}


int test_mbuf_human_time(void)
{
	const struct pl ref1 = PL("1 day 2 hours 3 mins 4 secs");
	const struct pl ref2 = PL("1 min 2 secs");
	struct pl pl;
	struct mbuf mb;
	uint32_t sec;
	int err;

	mbuf_init(&mb);

	sec = 1*24*60*60 + 2*60*60 + 3*60 + 4;
	err = mbuf_printf(&mb, "%H", fmt_human_time, &sec);
	if (err)
		goto out;
	if (!mb.buf) {
		err = ENOMEM;
		goto out;
	}
	pl.p = (char *)mb.buf;
	pl.l = mb.end;
	err = pl_cmp(&pl, &ref1);
	if (err)
		goto out;

	mbuf_reset(&mb);

	sec = 0*24*60*60 + 0*60*60 + 1*60 + 2;
	err = mbuf_printf(&mb, "%H", fmt_human_time, &sec);
	if (err)
		goto out;
	if (!mb.buf) {
		err = ENOMEM;
		goto out;
	}
	pl.p = (char *)mb.buf;
	pl.l = mb.end;
	err = pl_cmp(&pl, &ref2);
	if (err)
		goto out;

	err = 0;

 out:
	mbuf_reset(&mb);

	return err;
}
