/**
 * @file http.c HTTP Testcode
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include <re.h>
#include "test.h"


int test_http(void)
{
	static const char req[] =
		"GET /path/file.html HTTP/1.1\r\n"
		"From: plopp@klukk.no\r\n"
		"User-Agent: libre HTTP client/1.0\r\n"
		"Allow: GET, HEAD, PUT\r\n"
		"\r\n"
		"";
	struct mbuf *mb;
	struct http_msg *msg = NULL;
	const struct http_hdr *hdr;
	int err;

	mb = mbuf_alloc(512);
	if (!mb)
		return ENOMEM;

	err = mbuf_write_str(mb, req);
	if (err)
		goto out;

	mb->pos = 0;
	err = http_msg_decode(&msg, mb, true);
	if (err)
		goto out;

	if (0 != pl_strcmp(&msg->met, "GET"))              goto badmsg;
	if (0 != pl_strcmp(&msg->path, "/path/file.html")) goto badmsg;
	if (0 != pl_strcmp(&msg->ver, "1.1"))              goto badmsg;
	if (pl_isset(&msg->prm))                           goto badmsg;

	hdr = http_msg_hdr(msg, HTTP_HDR_FROM);
	if (!hdr || 0 != pl_strcmp(&hdr->val, "plopp@klukk.no"))
		goto badmsg;
	hdr = http_msg_hdr(msg, HTTP_HDR_USER_AGENT);
	if (!hdr || 0 != pl_strcmp(&hdr->val, "libre HTTP client/1.0"))
		goto badmsg;
	hdr = http_msg_hdr(msg, HTTP_HDR_CONTENT_TYPE);
	if (hdr)
		goto badmsg;
	if (msg->clen != 0)
		goto badmsg;

	if (!http_msg_hdr_has_value(msg, HTTP_HDR_ALLOW, "GET")  ||
	    !http_msg_hdr_has_value(msg, HTTP_HDR_ALLOW, "HEAD") ||
	    !http_msg_hdr_has_value(msg, HTTP_HDR_ALLOW, "PUT"))
		goto badmsg;
	if (3 != http_msg_hdr_count(msg, HTTP_HDR_ALLOW))
		goto badmsg;

	goto out;

 badmsg:
	(void)re_fprintf(stderr, "%H\n", http_msg_print, msg);
	err = EBADMSG;

 out:
	mem_deref(msg);
	mem_deref(mb);

	return err;
}
