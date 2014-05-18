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
	static const char *pattern = "mmmmmmmmm";
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

	mb.pos = mb.end = 0;
	err = mbuf_fill(&mb, 'm', 9);
	if (err)
		goto out;
	if (mb.pos != strlen(pattern) ||
	    mb.end != strlen(pattern) ||
	    0 != memcmp(mb.buf, pattern, 9)) {
		err = EBADMSG;
		goto out;
	}

 out:
	mbuf_reset(&mb);
	mem_deref(str);

	return err;
}
