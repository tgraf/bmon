/*
 * bmon/graph.h            Graph creation utility
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

#ifndef __BMON_GRAPH_H_
#define __BMON_GRAPH_H_

#include <bmon/bmon.h>
#include <bmon/unit.h>

struct history;

struct graph_cfg {
	int			gc_height,
				gc_width,
				gc_flags;

	char			gc_background,
				gc_foreground,
				gc_noise,
				gc_unknown;
	
	struct unit *		gc_unit;
};

struct graph_table {
	char *			gt_table;
	char *			gt_y_unit;
	float *			gt_scale;
};

struct graph {
	struct graph_cfg	g_cfg;
	struct graph_table	g_rx,
				g_tx;

	struct list_head	g_list;
};
	
extern void			graph_free(struct graph *);
extern struct graph *		graph_alloc(struct history *, struct graph_cfg *);
extern void			graph_refill(struct graph *, struct history *);

extern size_t			graph_row_size(struct graph_cfg *);

extern void new_graph(void);
extern void del_graph(void);
extern int next_graph(void);
extern int prev_graph(void);

#endif
