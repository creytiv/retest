/**
 * @file mock/stunsrv.c Mock STUN server
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include <re.h>
#include "test.h"


#define DEBUG_MODULE "mock/stunsrv"
#define DEBUG_LEVEL 5
#include <re_dbg.h>


static void stunserver_udp_recv(const struct sa *src, struct mbuf *mb,
				void *arg)
{
	struct stunserver *stun = arg;
	struct stun_msg *msg;
	int err;

	stun->nrecv++;

	err = stun_msg_decode(&msg, mb, NULL);
	if (err)
		return;

	TEST_EQUALS(0x0001, stun_msg_type(msg));
	TEST_EQUALS(STUN_CLASS_REQUEST, stun_msg_class(msg));
	TEST_EQUALS(STUN_METHOD_BINDING, stun_msg_method(msg));

	err = stun_reply(IPPROTO_UDP, stun->us, src,
			 0, msg, NULL, 0, false, 1,
			 STUN_ATTR_XOR_MAPPED_ADDR, src);

 out:
	if (err) {
		(void)stun_ereply(IPPROTO_UDP, stun->us, src, 0, msg, 400,
				  "Bad Request", NULL, 0, false, 0);
	}

	mem_deref(msg);
}


static void stunserver_destructor(void *arg)
{
	struct stunserver *stun = arg;

	mem_deref(stun->us);
}


int stunserver_alloc(struct stunserver **stunp)
{
	struct stunserver *stun;
	int err;

	if (!stunp)
		return EINVAL;

	stun = mem_zalloc(sizeof(*stun), stunserver_destructor);
	if (!stun)
		return ENOMEM;

	err = sa_set_str(&stun->laddr, "127.0.0.1", 0);
	if (err)
		goto out;

	err = udp_listen(&stun->us, &stun->laddr, stunserver_udp_recv, stun);
	if (err)
		goto out;

	err = udp_local_get(stun->us, &stun->laddr);
	if (err)
		goto out;

 out:
	if (err)
		mem_deref(stun);
	else
		*stunp = stun;

	return err;
}
