/*
 * bmon/element.h		Elements
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

#ifndef __BMON_ELEMENT_H_
#define __BMON_ELEMENT_H_

#include <bmon/bmon.h>
#include <bmon/group.h>
#include <bmon/attr.h>

#define MAX_GRAPHS 32
#define IFNAME_MAX 32

struct policy
{
	char *			p_rule;
	struct list_head	p_list;
};

struct element_cfg;

struct info
{
	char *			i_name;
	char *			i_value;
	struct list_head	i_list;
};

struct element
{
	char *			e_name;
	char *			e_description;
	uint32_t		e_id;
	uint32_t		e_flags;
	unsigned int		e_lifecycles;
	unsigned int		e_level;	/* recursion level */

	struct element *	e_parent;
	struct element_group *	e_group;

	struct list_head	e_list;
	struct list_head	e_childs;

	unsigned int		e_nattrs;
	struct list_head	e_attrhash[ATTR_HASH_SIZE];
	struct list_head	e_attr_sorted;

	unsigned int		e_ninfo;
	struct list_head	e_info_list;

	struct attr_def *	e_key_attr[__GT_MAX];
	struct attr_def *	e_usage_attr;

	float			e_rx_usage,
				e_tx_usage;

	struct element_cfg *	e_cfg;

	struct attr *		e_current_attr;
};

#define ELEMENT_CREAT		(1 << 0)

extern struct element *		element_lookup(struct element_group *,
					       const char *, uint32_t,
					       struct element *, int);

extern void			element_free(struct element *);

extern void			element_reset_update_flag(struct element_group *,
							  struct element *,
							  void *);
extern void			element_notify_update(struct element *,
						      timestamp_t *);
extern void			element_lifesign(struct element *, int);
extern void			element_check_if_dead(struct element_group *,
						      struct element *, void *);

extern int			element_set_key_attr(struct element *, const char *, const char *);
extern int			element_set_usage_attr(struct element *, const char *);

#define ELEMENT_FLAG_FOLDED	(1 << 0)
#define ELEMENT_FLAG_UPDATED	(1 << 1)
#define ELEMENT_FLAG_EXCLUDE	(1 << 2)
#define ELEMENT_FLAG_CREATED	(1 << 3)

extern void			element_foreach_attr(struct element *,
					void (*cb)(struct element *,
						   struct attr *, void *),
					void *);

extern struct element *		element_current(void);
extern struct element *		element_select_first(void);
extern struct element *		element_select_last(void);
extern struct element *		element_select_next(void);
extern struct element *		element_select_prev(void);

extern int			element_allowed(const char *, struct element_cfg *);
extern void			element_parse_policy(const char *);

extern void			element_update_info(struct element *,
						    const char *,
						    const char *);

extern void			element_set_txmax(struct element *, uint64_t);
extern void			element_set_rxmax(struct element *, uint64_t);

#endif
