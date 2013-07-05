/*
 * bmon/history.h		History Management
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

#ifndef __BMON_HISTORY_H_
#define __BMON_HISTORY_H_

#include <bmon/bmon.h>
#include <bmon/attr.h>

#define HISTORY_UNKNOWN		((uint64_t) -1)
#define HBEAT_TRIGGER		60.0f

enum {
	HISTORY_TYPE_8	= 1,
	HISTORY_TYPE_16	= 2,
	HISTORY_TYPE_32	= 4,
	HISTORY_TYPE_64	= 8,
};

struct history_def
{
	char *			hd_name;
	int			hd_size,
				hd_type;
	float			hd_interval;

	struct list_head	hd_list;
};

struct history_store
{
	/* TODO? store error ratio? */
	void *			hs_data;
	uint64_t		hs_prev_total;
};

struct history
{
	/* index to current entry in data array */
	int			h_index;
	struct history_def *	h_definition;
	/* time of last history update */
	timestamp_t		h_last_update;
	struct list_head	h_list;

	float			h_min_interval,
				h_max_interval;

	struct history_store	h_rx,
				h_tx;

};

extern struct history_def *	history_def_lookup(const char *);
extern struct history_def *	history_def_alloc(const char *);

extern uint64_t			history_data(struct history *,
					     struct history_store *, int);
extern void			history_update(struct attr *,
					       struct history *, timestamp_t *);
extern struct history *		history_alloc(struct history_def *);
extern void			history_free(struct history *);
extern void			history_attach(struct attr *);

extern struct history_def *	history_select_first(void);
extern struct history_def *	history_select_last(void);
extern struct history_def *	history_select_next(void);
extern struct history_def *	history_select_prev(void);
extern struct history_def *	history_current(void);

#endif
