/**
 * @file uri.c URI Testcode
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include <string.h>
#include <re.h>
#include "test.h"


#define DEBUG_MODULE "testuri"
#define DEBUG_LEVEL 5
#include <re_dbg.h>


int test_uri(void)
{
	const char *uriv[] = {
		"sip:user:pass@host:5060;transport=udp",
		"sip:user@host:5060;transport=udp",
		"sip:user@host:5060",
		"sip:host:5060",
		"sip:host",
		"sip:user@81.187.91.2:28481",
		"sip:user@81.187.91.2:28481",
		"sip:81.187.91.2:28481",
		"sips:81.187.91.2:28481",

		/* RFC 3261 */
		"sip:alice@atlanta.com",
		"sip:alice:secretword@atlanta.com;transport=tcp",
		"sips:alice@atlanta.com?subject=project%20"
		   "&priority=urgent",
		"sip:+1-212-555-1212:1234@gateway.com;user=phone",
		"sips:1212@gateway.com",
		"sip:alice@192.0.2.4",
		"sip:atlanta.com;method=REGISTER?to=alice%40atlanta.com",
		"sip:alice;day=tuesday@atlanta.com",

#ifdef HAVE_INET6
		/* IPv6 */
		"sip:[::1];transport=udp",
		"sip:[::1]:1234;transport=udp",
		"sip:user@[::1];transport=udp",
		"sip:user:pass@[::1];transport=udp",
		"sip:user@[::1]:1234;transport=udp",
		"sip:user:pass@[::1]:1234;transport=udp",
		"sip:user@[::1]:1234;transport=udp?foo=bar",
		"sip:user:pass@[::1]:1234;transport=udp?foo=bar",

		/* draft-ietf-sipping-ipv6-torture-tests-00 */
		"sip:[2001:db8::10]",
		"sip:[2001:db8::10:5070]",
		"sip:[2001:db8::10]:5070",
		"sip:user@[2001:db8::10]",
#endif
	};
	struct uri uri;
	struct mbuf mb;
	int err = EINVAL;
	size_t i;

	mbuf_init(&mb);

	for (i=0; i<ARRAY_SIZE(uriv); i++) {
		struct pl pl0, pl;

		/* Decode */
		pl_set_str(&pl0, uriv[i]);
		err = uri_decode(&uri, &pl0);
		if (err) {
			DEBUG_WARNING("uri: uri_decode() failed (%s) (%m)\n",
				      uriv[i], err);
			goto out;
		}

		/* Encode */
		mbuf_reset(&mb);
		err = mbuf_printf(&mb, "%H", uri_encode, &uri);
		if (err) {
			goto out;
		}

		/* Compare */
		pl.p = (const char *)mb.buf;
		pl.l = mb.end;
		err = pl_cmp(&pl, &pl0);
		if (err) {
			DEBUG_WARNING("uri comp: ref=(%s), gen=(%r) (%m)\n",
				      &uriv[i], &pl, err);
			goto out;
		}
	}

 out:
	mbuf_reset(&mb);
	return err;
}


int test_uri_encode(void)
{
	const struct {
		struct uri uri;
		const char *enc;
	} uriv[] = {
		{{PL("sip"),
		  PL("user"),
		  PL("pass"),
		  PL("host"), 0,
		  5060,
		  PL(";transport=udp"),
		  PL("")},
		 "sip:user:pass@host:5060;transport=udp"
		},
#ifdef HAVE_INET6
		{{PL("sip"),
		  PL("user"),
		  PL(""),
		  PL("::1"), AF_INET6,
		  1234,
		  PL(";transport=udp"),
		  PL("")},
		 "sip:user@[::1]:1234;transport=udp"
		}
#endif
	};
	struct mbuf mb;
	int err = EINVAL;
	size_t i;

	mbuf_init(&mb);

	for (i=0; i<ARRAY_SIZE(uriv); i++) {
		struct pl pl;

		/* Encode */
		mb.pos = 0;
		mb.end = 0;
		err = mbuf_printf(&mb, "%H", uri_encode, &uriv[i].uri);
		if (err)
			goto out;

		/* Compare */
		pl.p = (const char *)mb.buf;
		pl.l = mb.end;
		err = pl_strcmp(&pl, uriv[i].enc);
		if (err) {
			DEBUG_WARNING("uri enc: ref=(%s), gen=(%r) (%m)\n",
				      &uriv[i].enc, &pl, err);
			goto out;
		}
	}

 out:
	mbuf_reset(&mb);
	return err;
}


/**
 * Test URI Comparison - See RFC 3261 Section 19.1.4
 */
int test_uri_cmp(void)
{
	const struct {
		const char *l;
		const char *r;
		bool match;
	} uriv[] = {
		/* The URIs within each of the following sets are
		   equivalent: */
#if 0 /* todo handle escape */
		{"sip:%61lice@atlanta.com;transport=TCP",
		 "sip:alice@AtLanTa.CoM;Transport=tcp",
		 true},
#endif
		{"sip:carol@chicago.com",
		 "sip:carol@chicago.com;newparam=5",
		 true},
		{"sip:carol@chicago.com",
		 "sip:carol@chicago.com;security=on",
		 true},
		{"sip:carol@chicago.com;newparam=5",
		 "sip:carol@chicago.com;security=on",
		 true},
		{"sip:biloxi.com;transport=tcp;method=REGISTER"
		 "?to=sip:bob%40biloxi.com",
		 "sip:biloxi.com;method=REGISTER;transport=tcp"
		 "?to=sip:bob%40biloxi.com",
		 true},
		{"sip:alice@atlanta.com?subject=project%20x"
		 "&priority=urgent",
		 "sip:alice@atlanta.com?priority=urgent"
		 "&subject=project%20x",
		 true},
		{"SIP:alice@AtLanTa.CoM;Transport=udp",
		 "sip:alice@AtLanTa.CoM;Transport=UDP",
		 true},
		{"SIP:alice:secret@AtLanTa.CoM;Transport=udp",
		 "sip:alice:secret@AtLanTa.CoM;Transport=UDP",
		 true},
		{"sip:domain.com;foo=5;bar",
		 "sip:domain.com;FOO=5;BAR",
		 true},

		/*The URIs within each of the following sets are
		  not equivalent: */
		{"SIP:ALICE@AtLanTa.CoM;Transport=udp",
		 "sip:alice@AtLanTa.CoM;Transport=UDP",
		 false},
		{"sip:bob@biloxi.com",
		 "sip:bob@biloxi.com:5060",
		 false},
		{"sip:bob@biloxi.com",
		 "sip:bob@biloxi.com;transport=udp",
		 false},
		{"sip:bob@biloxi.com",
		 "sip:bob@biloxi.com:6000;transport=tcp",
		 false},
		{"sip:carol@chicago.com",
		 "sip:carol@chicago.com?Subject=next%20meeting",
		 false},
		{"sip:bob@phone21.boxesbybob.com",
		 "sip:bob@192.0.2.4",
		 false},
		{"sip:carol@chicago.com;security=on",
		 "sip:carol@chicago.com;security=off",
		 false},
		{"sip:foo",
		 "sips:foo",
		 false},
		{"SIP:alice:sEcrEt@AtLanTa.CoM;Transport=udp",
		 "sip:alice:secret@AtLanTa.CoM;Transport=UDP",
		 false},
	};
	int err = 0;
	size_t i;

	for (i=0; i<ARRAY_SIZE(uriv); i++) {
		struct uri l, r;
		struct pl pll, plr;
		bool match;

		pl_set_str(&pll, uriv[i].l);
		pl_set_str(&plr, uriv[i].r);

		/* Decode */
		err = uri_decode(&l, &pll);
		if (err)
			break;
		err = uri_decode(&r, &plr);
		if (err)
			break;

		/* Compare */
		match = uri_cmp(&l, &r);
		if (!uriv[i].match != !match) {
			DEBUG_WARNING("uri cmp: (%s) expected %d, got %d\n",
				      uriv[i].l, uriv[i].match, match);
			err = EINVAL;
			break;
		}

		/* Swap */
		err = uri_decode(&r, &pll);
		if (err)
			break;
		err = uri_decode(&l, &plr);
		if (err)
			break;

		/* Compare */
		match = uri_cmp(&l, &r);
		if (!uriv[i].match != !match) {
			DEBUG_WARNING("uri cmp: (%s) expected %d, got %d\n",
				      uriv[i].l, uriv[i].match, match);
			err = EINVAL;
			break;
		}
	}

	return err;
}


int test_uri_user(void)
{
	const struct {
		const char *enc;
		const char *dec;
	} uriv[] = {
		{"alice%40atlanta.com", "alice@atlanta.com"},
		{"project%20x", "project x"},
		{"*21%23", "*21#"}
	};
	struct mbuf mbe, mbd;
	int err = EINVAL;
	size_t i;

	mbuf_init(&mbd);
	mbuf_init(&mbe);

	for (i=0; i<ARRAY_SIZE(uriv); i++) {
		struct pl ple, pld, pl;

		/* Decode and compare */
		pl_set_str(&ple, uriv[i].enc);
		mbuf_reset(&mbd);
		err = mbuf_printf(&mbd, "%H", uri_user_unescape, &ple);
		if (err)
			break;

		pl.p = (const char *)mbd.buf;
		pl.l = mbd.end;
		err = pl_strcmp(&pl, uriv[i].dec);
		if (err) {
			DEBUG_WARNING("uri dec comp: ref=(%s), gen=(%r)\n",
				      uriv[i].dec, &pl);
			break;
		}

		/* Encode and compare */
		pl_set_str(&pld, uriv[i].dec);
		mbuf_reset(&mbe);
		err = mbuf_printf(&mbe, "%H", uri_user_escape, &pld);
		if (err)
			break;

		/* Compare */
		pl.p = (const char *)mbe.buf;
		pl.l = mbe.end;
		err = pl_strcmp(&pl, uriv[i].enc);
		if (err) {
			DEBUG_WARNING("uri enc comp: ref=(%s), gen=(%r)\n",
				      uriv[i].enc, &pl);
			break;
		}
	}

	mbuf_reset(&mbe);
	mbuf_reset(&mbd);
	return err;
}


int test_uri_headers(void)
{
	/* TODO: These are header _values_ only. -- should decode headers? */
	const struct {
		struct pl enc;
		struct pl dec;
	} uriv[] = {
		{PL("alice%40atlanta.com"),
		 PL("alice@atlanta.com")
		},
		{PL("project%20x"),
		 PL("project x")
		},
		{PL("%3Chttp://www.foo.com%3E"),
		 PL("<http://www.foo.com>")
		}
#if 0
		{PL("88edf54%2Da493a459%40192%2E168%2E15%2E123"
		    "%3Bfrom-tag%3Db6a79b1834ba7b65o0"
		    "%3Bto-tag%3D407bf99cfb3d8f23i0"),

		 PL("88edf54-a493a459@192.168.15.123"
		    ";from-tag=b6a79b1834ba7b65o0"
		    ";to-tag=407bf99cfb3d8f23i0")
		}
#endif
	};
	struct mbuf mbe, mbd;
	int err = EINVAL;
	size_t i;

	mbuf_init(&mbd);
	mbuf_init(&mbe);

	for (i=0; i<ARRAY_SIZE(uriv); i++) {
		struct pl pl;

		/* Decode and compare */
		mbuf_reset(&mbd);
		err = mbuf_printf(&mbd, "%H", uri_header_unescape,
				  &uriv[i].enc);
		if (err)
			break;

		pl.p = (const char *)mbd.buf;
		pl.l = mbd.end;
		err = pl_cmp(&pl, &uriv[i].dec);
		if (err) {
			DEBUG_WARNING("uri dec comp: ref=(%r), gen=(%r)\n",
				      &uriv[i].dec, &pl);
			break;
		}

		/* Encode and compare */
		mbuf_reset(&mbe);
		err = mbuf_printf(&mbe, "%H", uri_header_escape, &uriv[i].dec);
		if (err)
			break;

		/* Compare */
		pl.p = (const char *)mbe.buf;
		pl.l = mbe.end;
		err = pl_cmp(&pl, &uriv[i].enc);
		if (err) {
			DEBUG_WARNING("uri enc comp: ref=(%r), gen=(%r)\n",
				      &uriv[i].enc, &pl);
			break;
		}
	}

	mbuf_reset(&mbe);
	mbuf_reset(&mbd);
	return err;
}


static int uri_param_handler(const struct pl *name, const struct pl *val,
			     void *arg)
{
	uint32_t *n = arg;
	int err;

	switch ((*n)++) {

	case 0:
		err  = pl_strcmp(name, "rport");
		break;

	case 1:
		err  = pl_strcmp(name, "transport");
		err |= pl_strcmp(val, "udp");
		break;

	default:
		return EINVAL;
	}

	return err;
}


static int uri_header_handler(const struct pl *name, const struct pl *val,
			     void *arg)
{
	uint32_t *n = arg;
	int err;

	switch ((*n)++) {

	case 0:
		err  = pl_strcmp(name, "Subject");
		err |= pl_strcmp(val, "Urgent");
		break;

	case 1:
		err  = pl_strcmp(name, "bar");
		err |= pl_strcmp(val, "2");
		break;

	default:
		return EINVAL;
	}

	return err;
}


int test_uri_params_headers(void)
{
	const char *paramv[] = {
		";rport;transport=udp"
	};
	const char *headerv[] = {
		"?Subject=Urgent&bar=2"
	};
	int err;
	uint32_t i;

	err = ENOENT;
	for (i=0; i<ARRAY_SIZE(paramv); i++) {
		static const struct pl transp = PL("transport");
		struct pl pl, val;
		uint32_t n = 0;

		pl_set_str(&pl, paramv[i]);
		err = uri_param_get(&pl, &transp, &val);
		if (err)
			break;
		err = pl_strcmp(&val, "udp");
		if (err)
			break;
		err = uri_params_apply(&pl, uri_param_handler, &n);
		if (err)
			break;
		if (!n)
			err = ENOENT;
	}
	if (err)
		goto out;

	err = ENOENT;
	for (i=0; i<ARRAY_SIZE(headerv); i++) {
		static const struct pl subj = PL("Subject");
		struct pl pl, val;
		uint32_t n = 0;

		pl_set_str(&pl, headerv[i]);

		err = uri_header_get(&pl, &subj, &val);
		if (err)
			break;

		err = pl_strcmp(&val, "Urgent");
		if (err)
			break;

		err = uri_headers_apply(&pl, uri_header_handler, &n);
		if (err)
			break;
		if (!n)
			err = ENOENT;
	}

 out:
	return err;
}


static int devnull_print_handler(const char *p, size_t size, void *arg)
{
	(void)p;
	(void)size;
	(void)arg;
	return 0;
}


int test_uri_escape(void)
{
	struct re_printf pf_devnull = {devnull_print_handler, NULL};
	const struct pl pl1 = PL("%");
	const struct pl pl2 = PL("%0");
	int e, err = 0;

	/* silence warnings */
	dbg_init(DBG_ERR, 0);

	e = uri_user_unescape(&pf_devnull, &pl1);
	TEST_EQUALS(EBADMSG, e);

	e = uri_user_unescape(&pf_devnull, &pl2);
	TEST_EQUALS(EBADMSG, e);

 out:
	dbg_init(DBG_WARNING, 0);

	return err;
}
