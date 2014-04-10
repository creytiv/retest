/**
 * @file aes.c AES (Advanced Encryption Standard) Testcode
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include <string.h>
#include <re.h>
#include "test.h"


#define DEBUG_MODULE "aestest"
#define DEBUG_LEVEL 5
#include <re_dbg.h>


/*
 * http://www.inconteam.com/software-development/41-encryption/
 *  55-aes-test-vectors#aes-crt
 *
 * AES CTR 128-bit encryption mode
 */
static int test_aes_ctr_loop(void)
{
	const char *init_vec_str = "f0f1f2f3f4f5f6f7f8f9fafbfcfdfeff";
	uint8_t encr_key[16];
	uint8_t iv_enc[AES_BLOCK_SIZE];
	uint8_t iv_dec[AES_BLOCK_SIZE];
	size_t i;
	int err = 0;
	struct aes *enc = NULL, *dec = NULL;

	static const struct {
		char test_str[33];
		char *ciph_str;
	} testv[] = {

		{"6bc1bee22e409f96e93d7e117393172a",
		 "874d6191b620e3261bef6864990db6ce"},

		{"ae2d8a571e03ac9c9eb76fac45af8e51",
		 "9806f66b7970fdff8617187bb9fffdff"},

		{"30c81c46a35ce411e5fbc1191a0a52ef",
		 "5ae4df3edbd5d35e5b4f09020db03eab"},

		{"f69f2445df4f9b17ad2b417be66c3710",
		 "1e031dda2fbe03d1792170a0f3009cee"},
	};

	err |= str_hex(encr_key, sizeof(encr_key),
		       "2b7e151628aed2a6abf7158809cf4f3c");
	err |= str_hex(iv_enc, sizeof(iv_enc), init_vec_str);
	err |= str_hex(iv_dec, sizeof(iv_dec), init_vec_str);
	if (err)
		return err;

	err  = aes_alloc(&enc, AES_MODE_CTR, encr_key, 128, iv_enc);
	err |= aes_alloc(&dec, AES_MODE_CTR, encr_key, 128, iv_dec);
	if (err)
		goto out;

	for (i=0; i<ARRAY_SIZE(testv); i++) {

		uint8_t test_vector[16];
		uint8_t cipher_text[16];
		uint8_t out[16];
		uint8_t clear[16];

		err |= str_hex(test_vector, sizeof(test_vector),
			       testv[i].test_str);
		err |= str_hex(cipher_text, sizeof(cipher_text),
			       testv[i].ciph_str);
		if (err)
			break;

		err = aes_encr(enc, out, test_vector, 16);
		if (err)
			break;

		TEST_MEMCMP(cipher_text, sizeof(cipher_text),
			    out, sizeof(out));

		err = aes_decr(dec, clear, out, 16);
		if (err)
			break;

		TEST_MEMCMP(test_vector, sizeof(test_vector),
			    clear, sizeof(clear));
	}

 out:
	mem_deref(enc);
	mem_deref(dec);

	return err;
}


static bool have_aes(void)
{
	static const uint8_t nullkey[AES_BLOCK_SIZE];
	struct aes *aes = NULL;
	int err;

	err = aes_alloc(&aes, AES_MODE_CTR, nullkey, 128, NULL);

	mem_deref(aes);

	return err != ENOSYS;
}


int test_aes(void)
{
	int err;

	if (!have_aes()) {
		(void)re_printf("skipping aes test\n");
		return 0;
	}

	err = test_aes_ctr_loop();
	if (err)
		return err;

	return err;
}
