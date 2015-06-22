/**
 * @file mock/nat.c Mock NAT-box
 *
 * Copyright (C) 2010 Creytiv.com
 */

#include <string.h>
#include <re.h>
#include "test.h"


#define DEBUG_MODULE "mock/nat"
#define DEBUG_LEVEL 5
#include <re_dbg.h>


static void nat_binding_add(struct nat *nat, const struct sa *addr)
{
	if (!nat || !addr)
		return;

	if (nat->bindingc >= ARRAY_SIZE(nat->bindingv)) {
		DEBUG_WARNING("NAT-box at max capacity\n");
		return;
	}

	nat->bindingv[nat->bindingc++] = *addr;
}


static struct sa *nat_binding_find(struct nat *nat, uint16_t port)
{
	unsigned i;

	if (!nat || !port)
		return NULL;

	for (i=0; i<nat->bindingc; i++) {

		if (port == sa_port(&nat->bindingv[i]))
			return &nat->bindingv[i];
	}

	return NULL;
}


static bool nat_helper_send(int *err, struct sa *dst,
			    struct mbuf *mb, void *arg)
{
	struct nat *nat = arg;
	struct sa *cli;
	(void)mb;

	cli = nat_binding_find(nat, sa_port(dst));

#if 0
	re_printf("nat: send INGRESS %J -> %J\n", dst, cli);
#endif

	if (cli) {
		*dst = *cli;
		return false;
	}
	else {
		*err = ENOTCONN;
		DEBUG_WARNING("nat: binding to %J not found\n", dst);
		return true;
	}
}


static bool nat_helper_recv(struct sa *src,
			    struct mbuf *mb, void *arg)
{
	struct nat *nat = arg;
	struct sa map;
	(void)mb;

	if (!nat_binding_find(nat, sa_port(src))) {
		nat_binding_add(nat, src);
	}

	map = nat->public_addr;
	sa_set_port(&map, sa_port(src));

#if 0
	re_printf("nat: recv EGRESS %J -> %J\n", src, &map);
#endif

	*src = map;

	return false;
}


static void nat_destructor(void *arg)
{
	struct nat *nat = arg;

	mem_deref(nat->uh);
	mem_deref(nat->us);
}


/* inbound NAT */
int nat_alloc(struct nat **natp, struct udp_sock *us,
	      const struct sa *public_addr)
{
	struct nat *nat;
	int err = 0;

	if (!natp || !us || !public_addr)
		return EINVAL;

	nat = mem_zalloc(sizeof(*nat), nat_destructor);
	if (!nat)
		return ENOMEM;

	nat->public_addr = *public_addr;
	nat->us = mem_ref(us);
	err = udp_register_helper(&nat->uh, us, -1000,
				  nat_helper_send, nat_helper_recv, nat);
	if (err)
		goto out;

 out:
	if (err)
		mem_deref(nat);
	else
		*natp = nat;

	return err;
}
