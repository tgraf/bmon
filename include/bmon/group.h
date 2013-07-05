/*
 * bmon/group.h		Group Management
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

#ifndef __BMON_GROUP_H_
#define __BMON_GROUP_H_

#include <bmon/bmon.h>

struct element;

enum {
	GT_MAJOR,
	GT_MINOR,
	__GT_MAX,
};

#define GT_MAX (__GT_MAX - 1)

#define GROUP_COL_MAX (__GT_MAX * 2)

struct group_hdr
{
	char *			gh_name;
	char *			gh_title;
	char *			gh_column[GROUP_COL_MAX];
	struct list_head	gh_list;
};

extern struct group_hdr *	group_lookup_hdr(const char *);
extern int			group_new_hdr(const char *, const char *,
					      const char *, const char *,
					      const char *, const char *);
extern int			group_new_derived_hdr(const char *,
						      const char *,
						      const char *);

struct element_group
{
	char *			g_name;
	struct group_hdr *	g_hdr;

	struct list_head	g_elements;
	unsigned int		g_nelements;

	/* Currently selected element in this group */
	struct element *	g_current;

	struct list_head	g_list;
};

#define GROUP_CREATE		1

extern struct element_group *	group_lookup(const char *, int);
extern void			reset_update_flags(void);
extern void			free_unused_elements(void);
extern void			calc_rates(void);

extern void			group_foreach(
					void (*cb)(struct element_group *,
						   void *),
					void *);
extern void			group_foreach_recursive(
					void (*cb)(struct element_group *,
						   struct element *, void *),
					void *);

extern void			group_foreach_element(struct element_group *,
					void (*cb)(struct element_group *,
						   struct element *, void *),
					void *);

extern struct element_group *	group_current(void);
extern struct element_group *	group_select_first(void);
extern struct element_group *	group_select_last(void);
extern struct element_group *	group_select_next(void);
extern struct element_group *	group_select_prev(void);

#endif
