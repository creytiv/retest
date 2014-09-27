/**
 * @file http.c HTTP Testcode
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include <string.h>
#include <re.h>
#include "test.h"


#define DEBUG_MODULE "test_http"
#define DEBUG_LEVEL 5
#include <re_dbg.h>


static int test_http_response_no_reasonphrase(void)
{
	struct http_msg *msg = NULL;
	struct mbuf *mb;
	int err;

	mb = mbuf_alloc(512);
	if (!mb)
		return ENOMEM;

	err = mbuf_write_str(mb, /*       _---- no space here! */
			     "HTTP/1.1 429\r\n"
			     "Server: nginx\r\n"
			     "Content-Length: 0\r\n"
			     "\r\n");
	if (err)
		goto out;

	mb->pos = 0;

	err = http_msg_decode(&msg, mb, false);
	if (err)
		goto out;

	TEST_STRCMP("1.1", 3, msg->ver.p, msg->ver.l);
	TEST_EQUALS(429, msg->scode);
	TEST_STRCMP("", 0, msg->reason.p, msg->reason.l);

 out:
	mem_deref(msg);
	mem_deref(mb);
	return err;
}


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

	err = test_http_response_no_reasonphrase();
	if (err)
		goto out;

	goto out;

 badmsg:
	(void)re_fprintf(stderr, "%H\n", http_msg_print, msg);
	err = EBADMSG;

 out:
	mem_deref(msg);
	mem_deref(mb);

	return err;
}


struct test {
	uint32_t n_request;
	uint32_t n_response;
	int err;
};


static void abort_test(struct test *t, int err)
{
	t->err = err;
	re_cancel();
}


static void http_req_handler(struct http_conn *conn,
			     const struct http_msg *msg, void *arg)
{
	struct test *t = arg;
	int err = 0;

	++t->n_request;

	/* verify HTTP request */
	TEST_STRCMP("1.1", 3, msg->ver.p, msg->ver.l);
	TEST_STRCMP("GET", 3, msg->met.p, msg->met.l);
	TEST_STRCMP("/index.html", 11, msg->path.p, msg->path.l);
	TEST_STRCMP("", 0, msg->prm.p, msg->prm.l);
	TEST_EQUALS(0, msg->clen);

	err = http_reply(conn, 200, "OK", NULL);

 out:
	if (err)
		abort_test(t, err);
}


static void http_resp_handler(int err, const struct http_msg *msg, void *arg)
{
	struct test *t = arg;

	if (err) {
		/* translate error code */
		err = ENOMEM;
		goto out;
	}

	++t->n_response;

	/* verify HTTP response */
	TEST_STRCMP("1.1", 3, msg->ver.p, msg->ver.l);
	TEST_STRCMP("", 0, msg->met.p, msg->met.l);
	TEST_STRCMP("", 0, msg->path.p, msg->path.l);
	TEST_STRCMP("", 0, msg->prm.p, msg->prm.l);
	TEST_EQUALS(200, msg->scode);
	TEST_STRCMP("OK", 2, msg->reason.p, msg->reason.l);
	TEST_EQUALS(0, msg->clen);

	re_cancel();

 out:
	if (err)
		abort_test(t, err);
}


int test_http_loop(void)
{
	struct http_sock *sock = NULL;
	struct http_cli *cli = NULL;
	struct http_req *req = NULL;
	struct dnsc *dnsc = NULL;
	struct sa srv, dns;
	struct test t;
	char url[256];
	int err = 0;

	memset(&t, 0, sizeof(t));

	err |= sa_set_str(&srv, "127.0.0.1", 0);
	err |= sa_set_str(&dns, "127.0.0.1", 53);    /* note: unused */
	if (err)
		goto out;

	err = http_listen(&sock, &srv, http_req_handler, &t);
	if (err)
		goto out;

	err = tcp_sock_local_get(http_sock_tcp(sock), &srv);
	if (err)
		goto out;

	err = dnsc_alloc(&dnsc, NULL, &dns, 1);
	if (err)
		goto out;

	err = http_client_alloc(&cli, dnsc);
	if (err)
		goto out;

	(void)re_snprintf(url, sizeof(url),
			  "http://127.0.0.1:%u/index.html", sa_port(&srv));
	err = http_request(&req, cli, "GET", url,
			   http_resp_handler, NULL, &t,
			   NULL);
	if (err)
		goto out;

	err = re_main_timeout(500);
	if (err)
		goto out;

	if (t.err) {
		err = t.err;
		goto out;
	}

	/* verify results after HTTP traffic */
	TEST_EQUALS(1, t.n_request);
	TEST_EQUALS(1, t.n_response);

 out:
	mem_deref(req);
	mem_deref(cli);
	mem_deref(dnsc);
	mem_deref(sock);

	return err;
}
