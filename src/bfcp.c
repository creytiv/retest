/**
 * @file bfcp.c BFCP Testcode
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include <string.h>
#include <re.h>
#include "test.h"


#define DEBUG_MODULE "bfcptest"
#define DEBUG_LEVEL 5
#include <re_dbg.h>


static const uint8_t bfcp_msg[] =

	/* FloorRequest */
	"\x20\x01\x00\x04"  /* | ver | primitive | length  | */
	"\x01\x02\x03\x04"  /* |       conference id       | */
	"\xfe\xdc\xba\x98"  /* | transaction id | user id  | */
	""
	"\x04\x04\x00\x01"  /* FLOOR-ID */
	"\x02\x04\x00\x02"  /* BENEFICIARY-ID */
	"\x10\x03\x58\x00"  /* PARTICIPANT-PROVIDED-INFO */
       	"\x08\x04\x40\x00"  /* PRIORITY */

	/* FloorRelease */
	"\x20\x02\x00\x01"  /* | ver | primitive | length  | */
	"\x01\x02\x03\x04"  /* |       conference id       | */
	"\xfe\xdc\xba\x98"  /* | transaction id | user id  | */
	""
	"\x06\x04\x00\x03"  /* FLOOR-REQUEST-ID */

	/* UserStatus w/FLOOR-REQUEST-INFORMATION */
	"\x20\x06\x00\x12"  /* | ver | primitive | length  | */
	"\x01\x02\x03\x04"  /* |       conference id       | */
	"\xfe\xdc\xba\x98"  /* | transaction id | user id  | */
	""
	"\x1e\x48\x88\x99"  /* FLOOR-ID */
       	"\x24\x0c\x74\xad"  /* OVERALL-REQUEST-STATUS */
	"\x0a\x04\x04\x02"
	"\x12\x04\x4f\x4b"
       	"\x22\x0c\x00\x02"  /* FLOOR-REQUEST-STATUS #1 */
	"\x0a\x04\x02\x02"
	"\x12\x04\x6f\x6b"
       	"\x22\x0c\x00\x04"  /* FLOOR-REQUEST-STATUS #2 */
	"\x0a\x04\x07\x03"
	"\x12\x04\x6a\x61"
	"\x1c\x0c\x00\x01"  /* BENEFICIARY-INFORMATION */
	"\x18\x03\x61\x00"
	"\x1a\x03\x62\x00"
	"\x20\x0c\x00\x02"  /* REQUESTED-BY-INFORMATION */
	"\x18\x03\x63\x00"
	"\x1a\x03\x64\x00"
	"\x08\x04\x40\x00"  /* PRIORITY */
	"\x10\x03\x78\x00"  /* PARTICIPANT-PROVIDED-INFO */

	/* Hello */
	"\x20\x0b\x00\x00"  /* | ver | primitive | length  | */
	"\x01\x02\x03\x04"  /* |       conference id       | */
	"\xfe\xdc\xba\x98"  /* | transaction id | user id  | */
	""

	"";


static int parse_msg(const uint8_t *p, size_t n)
{
	struct mbuf *mb = mbuf_alloc(512);
	int err;
	if (!mb)
		return ENOMEM;

	err = mbuf_write_mem(mb, p, n);
	if (err)
		return err;

	mb->pos = 0;

	while (mbuf_get_left(mb) >= 4) {
		struct bfcp_msg *msg;

		err = bfcp_msg_decode(&msg, mb);
		if (err)
			break;

		mem_deref(msg);
	}

	mem_deref(mb);
	return err;
}


int test_bfcp(void)
{
	const size_t sz = sizeof(bfcp_msg) - 1;
	struct mbuf *mb;
	struct bfcp_reqstatus oreqstatus, reqstatus1, reqstatus2;
	uint16_t floorid = 1, bfid = 2, frid = 3, freqid;
	uint16_t ofreqid, floorid1, floorid2, rbid;
	enum bfcp_priority prio = BFCP_PRIO_NORMAL;
	int n, err = 0;

	mb = mbuf_alloc(512);
	if (!mb)
		return ENOMEM;

	err = bfcp_msg_encode(mb, 1, false, BFCP_FLOOR_REQUEST,
			      0x01020304, 0xfedc, 0xba98, 4,
			      BFCP_FLOOR_ID,       0, &floorid,
			      BFCP_BENEFICIARY_ID, 0, &bfid,
			      BFCP_PART_PROV_INFO, 0, "X",
			      BFCP_PRIORITY,       0, &prio);
	if (err)
		goto out;

	err = bfcp_msg_encode(mb, 1, false, BFCP_FLOOR_RELEASE,
			      0x01020304, 0xfedc, 0xba98, 1,
			      BFCP_FLOOR_REQUEST_ID, 0, &frid);
	if (err)
		goto out;

	freqid = 0x8899;

	ofreqid           = 0x74ad;
	oreqstatus.status = BFCP_DENIED;
	oreqstatus.qpos   = 2;

	floorid1          = 2;
	reqstatus1.status = BFCP_ACCEPTED;
	reqstatus1.qpos   = 2;

	floorid2          = 4;
	reqstatus2.status = BFCP_REVOKED;
	reqstatus2.qpos   = 3;

	bfid = 1;
	rbid = 2;
	prio = BFCP_PRIO_NORMAL;

	err = bfcp_msg_encode(mb, 1, false, BFCP_USER_STATUS,
			      0x01020304, 0xfedc, 0xba98, 1,

			      BFCP_FLOOR_REQ_INFO,     7, &freqid,

			      BFCP_OVERALL_REQ_STATUS, 2, &ofreqid,
			      BFCP_REQUEST_STATUS,     0, &oreqstatus,
			      BFCP_STATUS_INFO,        0, "OK",

			      BFCP_FLOOR_REQ_STATUS,   2, &floorid1,
			      BFCP_REQUEST_STATUS,     0, &reqstatus1,
			      BFCP_STATUS_INFO,        0, "ok",

			      BFCP_FLOOR_REQ_STATUS,   2, &floorid2,
			      BFCP_REQUEST_STATUS,     0, &reqstatus2,
			      BFCP_STATUS_INFO,        0, "ja",

			      BFCP_BENEFICIARY_INFO,   2, &bfid,
			      BFCP_USER_DISP_NAME,     0, "a",
			      BFCP_USER_URI,           0, "b",

			      BFCP_REQUESTED_BY_INFO,  2, &rbid,
			      BFCP_USER_DISP_NAME,     0, "c",
			      BFCP_USER_URI,           0, "d",

			      BFCP_PRIORITY,           0, &prio,

			      BFCP_PART_PROV_INFO,     0, "x");
	if (err)
		goto out;

	err = bfcp_msg_encode(mb, 1, false, BFCP_HELLO,
			      0x01020304, 0xfedc, 0xba98, 0);
	if (err)
		goto out;

	if (mb->end != sz) {
		DEBUG_WARNING("expected %u bytes, got %u bytes\n",
			      sz, mb->end);

		(void)re_printf("\nEncoded message:\n");
		hexdump(stderr, mb->buf, mb->end);

		err = EPROTO;
		goto out;
	}
	if (!err) {
		n = memcmp(mb->buf, bfcp_msg, mb->end);
		if (0 != n) {
			err = EBADMSG;
			DEBUG_WARNING("error offset: %d\n", n);
		}
	}

	if (err) {
		DEBUG_WARNING("BFCP encode error: %m\n", err);

		(void)re_printf("\nReference message:\n");
		hexdump(stderr, bfcp_msg, sz);

		(void)re_printf("\nEncoded message:\n");
		hexdump(stderr, mb->buf, mb->end);
		goto out;
	}

 out:
	mem_deref(mb);
	return err;
}


int test_bfcp_bin(void)
{
	static const uint8_t msg[] =

		"\x20\x04\x00\x04"
		"\x00\x00\x00\x01"
		"\x00\x01\x00\x01"

		"\x1e\x10\x00\x01"
		"\x24\x08\x00\x01"
		"\x0a\x04\x03\x00"
		"\x22\x04\x00\x02"

		"";
	int err = 0;

	err |= parse_msg(msg, sizeof(msg) - 1);
	err |= parse_msg(bfcp_msg, sizeof(bfcp_msg) - 1);

	return err;
}
