/**
 * @file hmac.c HMAC Testcode
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include <string.h>
#include <re.h>
#include <re_sha.h>
#include <re_hmac.h>
#include "test.h"


#define DEBUG_MODULE "testhmac"
#define DEBUG_LEVEL 4
#include <re_dbg.h>


int test_hmac_sha1(void)
{
	/* RFC 2202 */
	const struct {
		const void *key;
		uint32_t key_len;
		const void *data;
		uint32_t data_len;
		char digest[43];
	} testv[] = {
		{"\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b"
		 "\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b", 20,
		 "Hi There", 8,
		 "0xb617318655057264e28bc0b6fb378c8ef146be00"},

		{"Jefe", 4,
		 "what do ya want for nothing?", 28,
		 "0xeffcdf6ae5eb2fa2d27416d5f184df9c259a7c79"},

		{"\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa"
		 "\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa", 20,
		 "\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd"
		 "\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd"
		 "\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd"
		 "\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd"
		 "\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd\xdd", 50,
		 "0x125d7342b9ac11cd91a39af48aa17b4f63f175d3"},

		{"01234567890123456789", 20,
 		 "dalskdmlkasndoiqwjeoi3hjoijqweolk6y52fsdfsfgh66h"
		 "91928ha8shdoalijelwjeoriwjeorijwe98fj98j98j384jo"
		 "dalskdmlkasndoiqwjeoi3hjoijqweolk6y52fsdfsfgh66h"
		 "91928ha8shdoalijelwjeoriwjeorijwe98fj98j98j38s4f"
		 "dalskdmlkasndoiqwjeoi3hjoijqweolk6y52fsdfsfghsda"
		 "91928ha8shdoalijelwjeoriwjeorijwe98fj98j98j384jo"
		 "dalskdmlkasndoiqwjeoi3hjoijqweolk6y52fsdfsfgh66h"
		 "91928ha8shdoalijelwjeoriwjeorijwe98fj98jqwe98j38", 384,
		 "0x4b00628735c763b3c0dc398deb4370e99f822490"}
	};
	uint32_t i;

	for (i=0; i<ARRAY_SIZE(testv); i++) {
		char buf[43];
		uint8_t md[SHA_DIGEST_LENGTH];
		uint32_t md_len = SHA_DIGEST_LENGTH;

		hmac_sha1(testv[i].key,      /* secret key */
			  testv[i].key_len,  /* length of the key in bytes */
			  testv[i].data,     /* data */
			  testv[i].data_len, /* length of data in bytes */
			  md,          /* output buffer, at least "t" bytes */
			  md_len);

		(void)re_snprintf(buf, sizeof(buf), "0x%02w", md, sizeof(md));
		if (0 != strcmp(testv[i].digest, buf)) {
			DEBUG_WARNING("testcase %u: HMAC failed"
				      " (expected %s, got %s)\n",
				      i, testv[i].digest, buf);
			return EINVAL;
		}
	}

	return 0;
}
