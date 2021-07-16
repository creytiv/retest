/**
 * @file test/util.c  Test utilities
 *
 * Copyright (C) 2010 Creytiv.com
 */

#include <string.h>
#include <math.h>
#include <float.h>
#include <re.h>
#include "test.h"


#define DEBUG_MODULE "test"
#define DEBUG_LEVEL 5
#include <re_dbg.h>


static bool cmp_double(double a, double b)
{
	return fabs(a - b) < DBL_EPSILON;
}


static bool odict_value_compare(const struct odict_entry *e1,
				const struct odict_entry *e2)
{
	if (!e1 || !e2)
		return false;

	if (odict_entry_type(e1) != odict_entry_type(e2)) {
		re_printf("type mismatch\n");
		return false;
	}

	switch (odict_entry_type(e1)) {

	case ODICT_OBJECT:
		return odict_compare(odict_entry_object(e1),
				     odict_entry_object(e2));

	case ODICT_ARRAY:
		return odict_compare(odict_entry_array(e1),
				     odict_entry_array(e2));

	case ODICT_INT:
		if (odict_entry_int(e1) != odict_entry_int(e2)) {
			re_printf("integer diff: %lld != %lld\n",
				  odict_entry_int(e1), odict_entry_int(e2));
		}
		else
			return true;
		break;

	case ODICT_DOUBLE:
		if (cmp_double(odict_entry_dbl(e1), odict_entry_dbl(e2))) {
			return true;
		}
		else {
			re_printf("double differs: %f != %f\n",
				  odict_entry_dbl(e1), odict_entry_dbl(e2));
		}
		break;

	case ODICT_STRING:
		if ( 0 == str_cmp(odict_entry_str(e1), odict_entry_str(e2)))
			return true;
		else {
			re_printf("string differs\n");
		}
		break;

	case ODICT_BOOL:
		if (odict_entry_boolean(e1) == odict_entry_boolean(e2))
			return true;
		else {
			re_printf("bool differs\n");
		}
		break;

	case ODICT_NULL: /* no check */
		return true;

	default:
		return false;
	}

	return false;
}


/* return TRUE if equal */
bool odict_compare(const struct odict *dict1, const struct odict *dict2)
{
	struct le *le1, *le2;

	if (!dict1 || !dict2)
		return false;

	if (odict_count(dict1, true) != odict_count(dict2, true)) {
		re_printf("count mismatch\n");
		return false;
	}

	for (le1 = dict1->lst.head, le2 = dict2->lst.head;
	     le1 && le2;
	     le1 = le1->next, le2 = le2->next) {

		struct odict_entry *e1 = le1->data;
		struct odict_entry *e2 = le2->data;

		if (0 != str_cmp(odict_entry_key(e1), odict_entry_key(e2))) {
			re_printf("key mismatch\n");
			re_printf("(%s) %r\n", odict_entry_key(e1));
			re_printf("(%s) %r\n", odict_entry_key(e2));
			return false;
		}

		if (!odict_value_compare(e1, e2))
			return false;
	}

	return true;  /* equal */
}
