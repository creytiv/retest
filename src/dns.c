/**
 * @file dns.c DNS Testcode
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include <string.h>
#include <re.h>
#include "test.h"


enum {NUM_TESTS = 32};


static int mkstr(char **strp)
{
	size_t sz = 8;
	char *str;

	str = mem_alloc(sz, NULL);
	if (!str)
		return ENOMEM;

	rand_str(str, sz);

	*strp = str;

	return 0;
}


static int mkrr(struct dnsrr *rr, uint16_t type)
{
	int err;

	err = mkstr(&rr->name);
	if (err)
		return err;

	rr->type = type;
	rr->dnsclass = DNS_CLASS_IN;
	rr->ttl = 3600;
	rr->rdlen = 2;

	switch (type) {

	case DNS_TYPE_A:
		rr->rdata.a.addr = rand_u32();
		break;

	case DNS_TYPE_NS:
		err |= mkstr(&rr->rdata.ns.nsdname);
		break;

	case DNS_TYPE_CNAME:
		err |= mkstr(&rr->rdata.cname.cname);
		break;

	case DNS_TYPE_SOA:
		err |= mkstr(&rr->rdata.soa.mname);
		err |= mkstr(&rr->rdata.soa.rname);
		rr->rdata.soa.serial  = rand_u32();
		rr->rdata.soa.refresh = rand_u32();
		rr->rdata.soa.retry   = rand_u32();
		rr->rdata.soa.expire  = rand_u32();
		rr->rdata.soa.ttlmin  = rand_u32();
		break;

	case DNS_TYPE_PTR:
		err |= mkstr(&rr->rdata.ptr.ptrdname);
		break;

	case DNS_TYPE_MX:
		rr->rdata.mx.pref = rand_u16();
		err |= mkstr(&rr->rdata.mx.exchange);
		break;

	case DNS_TYPE_AAAA:
		rand_bytes(rr->rdata.aaaa.addr, 16);
		break;

	case DNS_TYPE_SRV:
		rr->rdata.srv.pri    = rand_u16();
		rr->rdata.srv.weight = rand_u16();
		rr->rdata.srv.port   = rand_u16();
		err |= mkstr(&rr->rdata.srv.target);
		break;

	case DNS_TYPE_NAPTR:
		rr->rdata.naptr.order = rand_u16();
		rr->rdata.naptr.pref  = rand_u16();
		err |= mkstr(&rr->rdata.naptr.flags);
		err |= mkstr(&rr->rdata.naptr.services);
		err |= mkstr(&rr->rdata.naptr.regexp);
		err |= mkstr(&rr->rdata.naptr.replace);
		break;
	}

	return err;
}


int test_dns_hdr(void)
{
	struct mbuf *mb;
	uint16_t u16 = 9753;  /* pseudo-random (predictable) */
	size_t i;
	int err = 0;

	mb = mbuf_alloc(512);
	if (!mb)
		return ENOMEM;

	for (i=0; i<NUM_TESTS; i++) {

		struct dnshdr hdr, hdr2;

		memset(&hdr, 0, sizeof(hdr));
		memset(&hdr2, 0, sizeof(hdr2));

		hdr.id     = u16;
		hdr.qr     = u16 & 1;
		hdr.opcode = u16 & 0xf;
		hdr.aa     = u16 & 1;
		hdr.tc     = u16 & 1;
		hdr.rd     = u16 & 1;
		hdr.ra     = u16 & 1;
		hdr.z      = u16 & 0x7;
		hdr.rcode  = u16 & 0xf;
		hdr.nq     = u16;
		hdr.nans   = u16;
		hdr.nauth  = u16;
		hdr.nadd   = u16;

		mb->pos = mb->end = 0;
		err = dns_hdr_encode(mb, &hdr);
		if (err)
			break;

		mb->pos = 0;
		err = dns_hdr_decode(mb, &hdr2);
		if (err)
			break;

		if (0 != memcmp(&hdr, &hdr2, sizeof(hdr))) {
			(void)re_fprintf(stderr,
					 "dnshdr mismatch:\n%02w\n%02w\n",
					 &hdr, sizeof(hdr),
					 &hdr2, sizeof(hdr2));
			err = EBADMSG;
			break;
		}

		u16 *= 17;
	}

	mem_deref(mb);

	return err;
}


int test_dns_rr(void)
{
	struct hash *ht = NULL;
	struct dnsrr *rr = NULL, *rr2 = NULL;
	struct mbuf *mb;
	size_t i;
	int err = ENOMEM;

	static const uint16_t typev[] = {
		DNS_TYPE_A,    DNS_TYPE_NS,  DNS_TYPE_CNAME,
		DNS_TYPE_SOA,  DNS_TYPE_PTR, DNS_TYPE_MX,
		DNS_TYPE_AAAA, DNS_TYPE_SRV, DNS_TYPE_NAPTR};

	mb = mbuf_alloc(512);
	if (!mb)
		return ENOMEM;

	err = hash_alloc(&ht, 32);
	if (err)
		goto out;

	for (i=0; i<ARRAY_SIZE(typev); i++) {

		hash_flush(ht);

		rr = dns_rr_alloc();
		if (!rr) {
			err = ENOMEM;
			break;
		}

		err = mkrr(rr, typev[i]);
		if (err)
			break;

		mb->pos = mb->end = 0;
		err = dns_rr_encode(mb, rr, 0, ht, 0);
		if (err)
			break;

		mb->pos = 0;
		err = dns_rr_decode(mb, &rr2, 0);
		if (err)
			break;

		if (!dns_rr_cmp(rr, rr2, false)) {
			(void)re_fprintf(stderr,
					 "dns_rr:\nrr:  %02w\n\nrr2: %02w\n",
					 rr, sizeof(*rr), rr2, sizeof(*rr2));
			hexdump(stderr, mb->buf, mb->end);
			err = EBADMSG;
			break;
		}

		rr = mem_deref(rr);
		rr2 = mem_deref(rr2);
	}

 out:
	hash_flush(ht);
	mem_deref(ht);
	mem_deref(rr2);
	mem_deref(rr);
	mem_deref(mb);

	return err;
}


/* Testcase to reproduce dname_decode looping error */
int test_dns_dname(void)
{
	static const uint8_t dname[] =
		"\xc0\x00\x00\x0c\x00\x01\x00\x00"
		"\x0e\x10\x00\x27\x25\x32\x4a\x57"
		"\x4d\x6e\x38\x37\x74\x58\x36\x43"
		"\x55\x41\x59\x77\x54\x70\x53\x61"
		"\x4c\x4c\x62\x67\x43\x72\x6e\x34"
		"\x75\x42\x4e\x36\x42\x36\x59\x57"
		"\x52\x4e\x00";
	struct mbuf mb;
	char *name;
	int err;

	mb.buf = (uint8_t *)dname;
	mb.pos = 0;
	mb.end = mb.size = sizeof(dname) - 1;

	/* Expect EINVAL */
	err = dns_dname_decode(&mb, &name, 0);
	if (err != EINVAL)
		return err ? err : EINVAL;

	return 0;
}
