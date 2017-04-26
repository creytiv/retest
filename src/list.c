/**
 * @file list.c Linked-lists Testcode
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include <string.h>
#include <re.h>
#include "test.h"


#define DEBUG_MODULE "testlist"
#define DEBUG_LEVEL 5
#include <re_dbg.h>


struct node {
	struct le le;
	int value;
};

int test_list(void)
{
	struct node node1, node2;
	struct list list;
	int err = EINVAL;

	list_init(&list);

	memset(&node1, 0, sizeof(node1));
	memset(&node2, 0, sizeof(node2));

	/* Test empty list */
	TEST_EQUALS(0, list_count(&list));

	/* Test with one node */
	list_append(&list, &node1.le, &node1);
	TEST_EQUALS(1, list_count(&list));

	list_unlink(&node1.le);

	TEST_EQUALS(0, list_count(&list));

	/* Test with two nodes */
	list_append(&list, &node1.le, &node1);
	list_append(&list, &node2.le, &node2);
	TEST_EQUALS(2, list_count(&list));

	list_unlink(&node1.le);

	TEST_EQUALS(1, list_count(&list));

	list_unlink(&node2.le);

	/* Test empty list */
	TEST_EQUALS(0, list_count(&list));

	err = 0;

 out:
	return err;
}


static void node_destructor(void *arg)
{
	struct node *node = arg;

	if (node->le.prev || node->le.next || node->le.list || node->le.data) {
		DEBUG_WARNING("le: prev=%p next=%p data=%p\n",
			      node->le.prev, node->le.next, node->le.data);
	}

	list_unlink(&node->le);
}


/**
 * Test linked list with external reference to objects
 */
int test_list_ref(void)
{
	struct list list;
	struct node *node, *node2;
	int err = 0;

	list_init(&list);

	node = mem_zalloc(sizeof(*node), node_destructor);
	node2 = mem_zalloc(sizeof(*node2), node_destructor);
	if (!node || !node2) {
		err = ENOMEM;
		goto out;
	}

	mem_ref(node);

	list_append(&list, &node->le, node);
	list_append(&list, &node2->le, node2);

 out:
	list_flush(&list);
	memset(&list, 0xa5, sizeof(list));  /* mark as deleted */

	/* note: done after list_flush() */
	mem_deref(node);

	return err;
}


static bool sort_handler(struct le *le1, struct le *le2, void *arg)
{
	struct node *node1 = le1->data;
	struct node *node2 = le2->data;
	(void)arg;

	/* NOTE: important to use less than OR equal to, otherwise
	   the list_sort function may be stuck in a loop */
	return node1->value <= node2->value;
}


#define NUM_ELEMENTS 100
int test_list_sort(void)
{
	struct list lst;
	struct le *le;
	int prev_value = 0;
	bool prev_value_set = false;
	unsigned i;
	unsigned value_counter = 7;
	int err = 0;

	list_init(&lst);

	/* add many elements with a random value */
	for (i=0; i<NUM_ELEMENTS; i++) {

		struct node *node;

		node = mem_zalloc(sizeof(*node), node_destructor);
		if (!node) {
			err = ENOMEM;
			goto out;
		}

		node->value = -50 + (value_counter % 100);
		value_counter *= 3;

		list_append(&lst, &node->le, node);
	}

	/* sort the list in ascending order */
	list_sort(&lst, sort_handler, NULL);

	/* verify that the list is sorted */
	for (le = lst.head; le; le = le->next) {

		struct node *node = le->data;

		if (prev_value_set) {
			TEST_ASSERT(node->value >= prev_value);
		}

		prev_value = node->value;
		prev_value_set = true;
	}

 out:
	list_flush(&lst);

	return err;
}
