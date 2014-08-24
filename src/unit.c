/*
 * unit.c		Unit Definitions
 *
 * Copyright (c) 2001-2013 Thomas Graf <tgraf@suug.ch>
 * Copyright (c) 2013 Red Hat, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include <bmon/bmon.h>
#include <bmon/conf.h>
#include <bmon/utils.h>
#include <bmon/unit.h>

static struct unit *byte_unit, *bit_unit;

static LIST_HEAD(units);

static struct list_head *get_flist(struct unit *unit)
{
	int div = UNIT_DEFAULT;

	if (cfg_getbool(cfg, "use_bit"))
		div = UNIT_BIT;
	else if (cfg_getbool(cfg, "use_si"))
		div = UNIT_SI;
	
	if (!list_empty(&unit->u_div[div]))
		return &unit->u_div[div];
	else
		return &unit->u_div[UNIT_DEFAULT];
}

struct unit *unit_lookup(const char *name)
{
	struct unit *unit;

	list_for_each_entry(unit, &units, u_list)
		if (!strcmp(name, unit->u_name))
			return unit;

	return NULL;
}

/**
 * Lookup best divisor to use for a certain situation
 * @hint		Value in question (for DYNAMIC_EXP)
 * @unit		Unit of value
 * @name		Place to store name of divisor used
 * @prec		Place to store suggested precision
 *
 * Searches for the best divisor to be used depending on the unit
 * exponent configured by the user. If a dynamic exponent is
 * configured, the divisor is selected based on the value of hint
 * so that hint is dividied into a small float >= 1.0. The name
 * of the divisor used is stored in *name.
 *
 * If prec points to a vaild integer, a number of precision digits
 * is suggested to avoid n.00 to make pretty printing easier.
 */
double unit_divisor(uint64_t hint, struct unit *unit, char **name,
		    int *prec)
{
	struct list_head *flist = get_flist(unit);
	struct fraction *f;

	if (prec)
		*prec = 2;

	if (cfg_unit_exp == DYNAMIC_EXP) {
		list_for_each_entry_reverse(f, flist, f_list) {
			if (hint >= f->f_divisor)
				goto found_it;
		}
	} else {
		int n = cfg_unit_exp;
		list_for_each_entry(f, flist, f_list) {
			if (--n == 0)
				goto found_it;
		}
	}

	*name = "";
	return 1;

found_it:
	if (f->f_divisor == 1.0f && prec)
		*prec = 0;

	*name = f->f_name;
	return f->f_divisor;
}

double unit_value2str(uint64_t value, struct unit *unit,
		      char **name, int *prec)
{
	double div = unit_divisor(value, unit, name, prec);
	double v = (double) value;

	if (fmod(v, div) == 0.0f && prec)
		*prec = 0;

	return v / div;
}

void fraction_free(struct fraction *f)
{
	if (!f)
		return;

	list_del(&f->f_list);
	xfree(f->f_name);
	xfree(f);
}

void unit_add_div(struct unit *unit, int type, const char *txt, float div)
{
	struct fraction *f;

	if (!unit)
		BUG();

	f = xcalloc(1, sizeof(*f));

	init_list_head(&f->f_list);

	f->f_divisor = div;
	f->f_name = strdup(txt);

	list_add_tail(&f->f_list, &unit->u_div[type]);
}

struct unit *unit_add(const char *name)
{
	struct unit *unit;
	int i;

	if (!(unit = unit_lookup(name))) {
		unit = xcalloc(1, sizeof(*unit));
		unit->u_name = strdup(name);

		for (i = 0; i < __UNIT_MAX; i++)
			init_list_head(&unit->u_div[i]);

		list_add_tail(&unit->u_list, &units);
	}

	return unit;
}

static void unit_free(struct unit *u)
{
	struct fraction *f, *n;

	if (!u)
		return;

	list_for_each_entry_safe(f, n, &u->u_div[UNIT_DEFAULT], f_list)
		fraction_free(f);

	list_for_each_entry_safe(f, n, &u->u_div[UNIT_SI], f_list)
		fraction_free(f);

	xfree(u->u_name);
	xfree(u);
}

char *unit_bytes2str(uint64_t bytes, char *buf, size_t len)
{
	char *ustr;
	int prec;
	double v;

	if (byte_unit) {
		v = unit_value2str(bytes, byte_unit, &ustr, &prec);
		snprintf(buf, len, "%'.*f%3s", prec, v, ustr);
	} else
		snprintf(buf, len, "%" PRIu64, bytes);

	return buf;
}

char *unit_bit2str(uint64_t bits, char *buf, size_t len)
{
	char *ustr;
	int prec;
	double v;

	if (bit_unit) {
		v = unit_value2str(bits, bit_unit, &ustr, &prec);
		snprintf(buf, len, "%'.*f%3s", prec, v, ustr);
	} else
		snprintf(buf, len, "%" PRIu64, bits);

	return buf;
}

static void __exit unit_exit(void)
{
	struct unit *u, *n;

	list_for_each_entry_safe(u, n, &units, u_list)
		unit_free(u);
}
