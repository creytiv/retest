/**
 * @file hash.c Hash Testcode
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include <string.h>
#include <re.h>
#include "test.h"


#define DEBUG_MODULE "testhash"
#define DEBUG_LEVEL 4
#include <re_dbg.h>


struct my_elem {
	struct le he;
	const struct pl *name;   /* hash key */
};

const struct pl martin = PL("Martin"),
	alfred = PL("Alfred"),
	atle = PL("Atle");

static struct my_elem elems[3];
static const size_t nelems = ARRAY_SIZE(elems);


static bool hash_cmp_handler(struct le *le, void *arg)
{
	const struct my_elem *me = le->data;
	const struct pl *name = arg;

	return 0==pl_cmp(me->name, name);
}


int test_hash(void)
{
	struct hash *h;
	struct my_elem *elem;
	uint32_t i;
	int err;

	/* Clear hash elements */
	for (i=0; i<nelems; i++)
		memset(&elems[i].he, 0, sizeof(elems[i].he));
	elems[0].name = &martin;
	elems[1].name = &alfred;
	elems[2].name = &atle;

	err = hash_alloc(&h, 4);
	if (err)
		return err;

	/* API test */
	if (EINVAL != hash_alloc(NULL, 4)) {
		err = EINVAL;
		goto out;
	}
	if (EINVAL != hash_alloc(&h, 0)) {
		err = EINVAL;
		goto out;
	}
	hash_append(h, 0, NULL, NULL);
	hash_append(NULL, 0, &elems[0].he, NULL);
	hash_unlink(NULL);
	if (hash_lookup(NULL, hash_joaat_pl(elems[0].name),
			hash_cmp_handler, (void *)elems[0].name)) {
		err = EINVAL;
		goto out;
	}
	if (hash_lookup(h, hash_joaat_pl(elems[0].name),
			NULL, (void *)elems[0].name)) {
		err = EINVAL;
		goto out;
	}

	/* Hashtable is empty */
	hash_unlink(&elems[0].he);
	hash_unlink(&elems[1].he);

	/* Hashtable with 1 element */
	hash_append(h, hash_joaat_pl(elems[0].name), &elems[0].he, &elems[0]);

	elem = list_ledata(hash_lookup(h, hash_joaat_pl(elems[0].name),
				       hash_cmp_handler,
				       (void *)elems[0].name));
	if (elem != &elems[0]) {
		err = EINVAL;
		goto out;
	}

	elem = list_ledata(hash_lookup(h, hash_joaat_pl(elems[1].name),
				       hash_cmp_handler,
				       (void *)elems[1].name));
	if (elem) {
		err = EINVAL;
		goto out;
	}

	/* Hashtable with 2 elements */
	hash_append(h, hash_joaat_pl(elems[1].name), &elems[1].he, &elems[1]);

	elem = list_ledata(hash_lookup(h, hash_joaat_pl(elems[0].name),
				       hash_cmp_handler,
				       (void *)elems[0].name));
	if (elem != &elems[0]) {
		err = EINVAL;
		goto out;
	}

	elem = list_ledata(hash_lookup(h, hash_joaat_pl(elems[1].name),
				       hash_cmp_handler,
				       (void *)elems[1].name));
	if (elem != &elems[1]) {
		err = EINVAL;
		goto out;
	}

#if 0
	/* Hashtable with many elements */
	for (i=0; i<1024; i++) {
		struct my_elem *myelem = &elems[i%nelems];
		hash_append(h, hash_joaat_pl(myelem->name),
			    &myelem->he, myelem);
	}

	for (i=0; i<1024; i++) {
		struct my_elem *myelem = &elems[i%nelems];
		hash_unlink(&myelem->he);
	}
#endif

	hash_unlink(&elems[0].he);
	hash_unlink(&elems[1].he);

	err = 0;
 out:
	h = mem_deref(h);
	return err;
}
