/*
 * bmon/unit.h		Units
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

#ifndef __BMON_UNIT_H_
#define __BMON_UNIT_H_

#include <bmon/bmon.h>

#define DYNAMIC_EXP	(-1)

enum {
	UNIT_DEFAULT,
	UNIT_SI,
	UNIT_BIT,
	__UNIT_MAX,
};

#define UNIT_MAX (__UNIT_MAX - 1)

#define UNIT_BYTE		"byte"
#define UNIT_NUMBER		"number"

struct fraction {
	float			f_divisor;
	char *			f_name;

	struct list_head	f_list;
};

struct unit {
	char *			u_name;
	struct list_head	u_div[__UNIT_MAX];

	struct list_head	u_list;
};

extern struct unit *	unit_lookup(const char *);
extern struct unit *	unit_add(const char *name);
extern void		unit_add_div(struct unit *, int, const char *, float);
extern double		unit_divisor(uint64_t, struct unit *, char **, int *);
extern double		unit_value2str(uint64_t, struct unit *, char **, int *);
extern void		fraction_free(struct fraction *);

extern char *		unit_bytes2str(uint64_t, char *, size_t);
extern char *		unit_bit2str(uint64_t, char *, size_t);

#endif
