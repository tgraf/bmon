/*
 * bmon/attr.h		Attributes
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

#ifndef __BMON_ATTR_H_
#define __BMON_ATTR_H_

#include <bmon/bmon.h>
#include <bmon/unit.h>

struct element;

struct rate
{
	/* Total value of attribute with eventual overflows accumulated.  */
	uint64_t		r_total;

	/* Current value of counter */
	uint64_t		r_current;

	/* Value of r_current at last read */
	uint64_t		r_prev;

	/* Reset value to substract to emulate statistics reset */
	uint64_t		r_reset;

	/* Rate per second calculated every `rate_interval' */
	float			r_rate;

	/* Time of last calculation */
	timestamp_t		r_last_calc;
};

extern uint64_t			rate_get_total(struct rate *);

enum {
	ATTR_TYPE_UNSPEC,
	ATTR_TYPE_COUNTER,
	ATTR_TYPE_RATE,
	ATTR_TYPE_PERCENT,
};

struct attr_def {
	int			ad_id;
	char *			ad_name;
	char *			ad_description;
	int			ad_type;
	int			ad_flags;
	struct unit *		ad_unit;

	struct list_head	ad_list;
};

struct attr_map {
	const char *	name;
	const char *	description;
	const char *	unit;
	int		attrid,
			type,
			rxid,
			txid,
			flags;
};

extern int			attr_def_add(const char *, const char *,
					     struct unit *, int, int);
extern struct attr_def *	attr_def_lookup(const char *);
extern struct attr_def *	attr_def_lookup_id(int);

extern int			attr_map_load(struct attr_map *map, size_t size);

#define ATTR_FORCE_HISTORY		0x01	/* collect history */
#define ATTR_IGNORE_OVERFLOWS		0x02
#define ATTR_TRUE_64BIT			0x04
#define ATTR_RX_ENABLED			0x08	/* has RX counter */
#define ATTR_TX_ENABLED			0x10	/* has TX counter */
#define ATTR_DOING_HISTORY		0x20	/* history collected */

struct attr
{
	struct rate		a_rx_rate,
				a_tx_rate;

	uint8_t			a_flags;
	struct attr_def *	a_def;
	timestamp_t		a_last_update;

	struct list_head	a_history_list;

	struct list_head	a_list;
	struct list_head	a_sort_list;
};

extern struct attr *		attr_lookup(const struct element *, int);
extern void			attr_update(struct element *, int,
					    uint64_t, uint64_t , int );
extern void			attr_notify_update(struct attr *,
						   timestamp_t *);
extern void			attr_free(struct attr *);

extern void			attr_rate2float(struct attr *,
						double *, char **, int *,
						double *, char **, int *);

extern void			attr_calc_usage(struct attr *, float *, float *,
						uint64_t, uint64_t);

#define ATTR_HASH_SIZE 32

#define UPDATE_FLAG_RX			0x01
#define UPDATE_FLAG_TX			0x02
#define UPDATE_FLAG_64BIT		0x04

extern struct attr *		attr_select_first(void);
extern struct attr *		attr_select_last(void);
extern struct attr *		attr_select_next(void);
extern struct attr *		attr_select_prev(void);
extern struct attr *		attr_current(void);

extern void			attr_start_collecting_history(struct attr *);
extern void			attr_reset_counter(struct attr *a);

#endif
