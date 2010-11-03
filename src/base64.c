/**
 * @file base64.c Base64 Testcode
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include <string.h>
#include <re.h>
#include "test.h"


#define DEBUG_MODULE "test_base64"
#define DEBUG_LEVEL 5
#include <re_dbg.h>


int test_base64(void)
{
	const struct {
		struct pl pl;
		struct pl b64;
	} testv[] = {
		{PL(""),       PL("")    },
		{PL("f"),      PL("Zg==")},
		{PL("fo"),     PL("Zm8=")},
		{PL("foo"),    PL("Zm9v")},
		{PL("foob"),   PL("Zm9vYg==")},
		{PL("fooba"),  PL("Zm9vYmE=")},
		{PL("foobar"), PL("Zm9vYmFy")},

		{PL("asdlkjqopinzidfj84r77fsgljsdf9823r"),
		 PL("YXNkbGtqcW9waW56aWRmajg0cjc3ZnNnbGpzZGY5ODIzcg==")},
		{PL("918nvbakishdl8317237dlakskdkaldj"),
		 PL("OTE4bnZiYWtpc2hkbDgzMTcyMzdkbGFrc2tka2FsZGo=")},
		{PL("very10long..testxyzstring/.,-=-3029===7823#'];'#';]#'"),
		 PL("dmVyeTEwbG9uZy4udGVzdHh5enN0cmluZy8uLC0"
		    "9LTMwMjk9PT03ODIzIyddOycjJztdIyc=")}
	};
	uint32_t i;
	int err = 0;

	for (i=0; i<ARRAY_SIZE(testv); i++) {
		const struct pl *pl = &testv[i].pl;
		char buf[128];
		uint8_t b64_buf[128];
		size_t olen;
		const struct pl *b;

		/* Encode */
		olen = sizeof(buf);
		err = base64_encode((uint8_t *)pl->p, pl->l, buf, &olen);
		if (err)
			break;

		if (olen != testv[i].b64.l) {
			DEBUG_WARNING("b64_encode %u failed: l=%u olen=%u\n",
				      i, testv[i].b64.l, olen);
			err = EINVAL;
			break;
		}
		if (0 != memcmp(testv[i].b64.p, buf, olen)) {
			DEBUG_WARNING("b64_encode %u failed: ref=%r, enc=%b\n",
				      i, &testv[i].b64, buf, olen);
			err = EINVAL;
			break;
		}

		/* Decode */
		b = &testv[i].b64;
		olen = sizeof(b64_buf);
		err = base64_decode(b->p, b->l, b64_buf, &olen);
		if (err)
			break;

		if (olen != testv[i].pl.l) {
			DEBUG_WARNING("b64_decode %u failed: l=%u olen=%u\n",
				      i, testv[i].pl.l, olen);
			err = EINVAL;
			break;
		}
		if (0 != memcmp(testv[i].pl.p, b64_buf, olen)) {
			DEBUG_WARNING("b64_decode %u failed: ref=%r, enc=%b\n",
				      i, &testv[i].pl, b64_buf, olen);
			err = EINVAL;
			break;
		}
	}

	return err;
}
