/*
 * graph.c             Graph creation utility
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
#include <bmon/graph.h>
#include <bmon/input.h>
#include <bmon/conf.h>
#include <bmon/history.h>
#include <bmon/conf.h>
#include <bmon/unit.h>
#include <bmon/utils.h>

size_t graph_row_size(struct graph_cfg *cfg)
{
	/* +1 for trailing \0 */
	return cfg->gc_width + 1;
}

static inline size_t table_size(struct graph_cfg *cfg)
{
	return cfg->gc_height * graph_row_size(cfg);
}

static inline char *at_row(struct graph_cfg *cfg, char *col, int nrow)
{
	return col + (nrow * graph_row_size(cfg));
}

static inline char *at_col(char *row, int ncol)
{
	return row + ncol;
}

static inline char *tbl_pos(struct graph_cfg *cfg, char *tbl, int nrow, int ncol)
{
	return at_col(at_row(cfg, tbl, nrow), ncol);
}

static void fill_table(struct graph *g, struct graph_table *tbl,
		       struct history *h, struct history_store *data)
{
	struct graph_cfg *cfg = &g->g_cfg;
	uint64_t max = 0;
	double v;
	int i, n, t;
	float half_step, step;

	if (!tbl->gt_table) {
		tbl->gt_table = xcalloc(table_size(cfg), sizeof(char));
		tbl->gt_scale = xcalloc(cfg->gc_height, sizeof(double));
	}

	memset(tbl->gt_table, cfg->gc_background, table_size(cfg));

	/* end each line with a \0 */
	for (i = 0; i < cfg->gc_height; i++)
		*tbl_pos(cfg, tbl->gt_table, i, cfg->gc_width) = '\0';

	/* leave table blank if there is no data */
	if (!h || !data->hs_data)
		return;

	if (cfg->gc_width > h->h_definition->hd_size)
		BUG();

	/* find the largest peak */
	for (n = h->h_index, i = 0; i < cfg->gc_width; i++) {
		if (--n < 0)
			n = h->h_definition->hd_size - 1;
		v = history_data(h, data, n);
		if (v != HISTORY_UNKNOWN && max < v)
			max = v;
	}

	step = (double) max / (double) cfg->gc_height;
	half_step = step / 2.0f;

	for (i = 0; i < cfg->gc_height; i++)
		tbl->gt_scale[i] = (double) (i + 1) * step;

	for (n = h->h_index, i = 0; i < cfg->gc_width; i++) {
		char * col = at_col(tbl->gt_table, i);

		if (--n < 0)
			n = h->h_definition->hd_size - 1;
		v = history_data(h, data, n);

		if (v == HISTORY_UNKNOWN) {
			for (t = 0; t < cfg->gc_height; t++)
				*(at_row(cfg, col, t)) = cfg->gc_unknown;
		} else if (v > 0) {
			*(at_row(cfg, col, 0)) = cfg->gc_noise;

			for (t = 0; t < cfg->gc_height; t++)
				if (v >= (tbl->gt_scale[t] - half_step))
					*(at_row(cfg, col, t)) = cfg->gc_foreground;
		}
	}

	n = (cfg->gc_height / 3) * 2;
	if (n >= cfg->gc_height)
		n = (cfg->gc_height - 1);

	v = unit_divisor(tbl->gt_scale[n], cfg->gc_unit,
			 &tbl->gt_y_unit, NULL);
	
	for (i = 0; i < cfg->gc_height; i++)
		tbl->gt_scale[i] /= v;
}

struct graph *graph_alloc(struct history *h, struct graph_cfg *cfg)
{
	struct graph *g;

	if (!cfg->gc_height)
		BUG();

	g = xcalloc(1, sizeof(*g));

	memcpy(&g->g_cfg, cfg, sizeof(*cfg));

	if (h != NULL &&
	    (cfg->gc_width > h->h_definition->hd_size || !cfg->gc_width))
		g->g_cfg.gc_width = h->h_definition->hd_size;

	if (!g->g_cfg.gc_width)
		BUG();

	return g;
}

void graph_refill(struct graph *g, struct history *h)
{
	fill_table(g, &g->g_rx, h, h ? &h->h_rx : NULL);
	fill_table(g, &g->g_tx, h, h ? &h->h_tx : NULL);
}

void graph_free(struct graph *g)
{
	if (!g)
		return;

	xfree(g->g_rx.gt_table);
	xfree(g->g_rx.gt_scale);
	xfree(g->g_tx.gt_table);
	xfree(g->g_tx.gt_scale);
	xfree(g);
}

#if 0

void new_graph(void)
{
	if (ngraphs >= (MAX_GRAPHS - 1))
		return;
	set_ngraphs_hard(ngraphs + 1);
}

void del_graph(void)
{
	if (ngraphs <= 1)
		return;
	set_ngraphs_hard(ngraphs - 1);
}

int next_graph(void)
{
	struct item *it = item_current();
	if (it == NULL)
		return EMPTY_LIST;

	if (it->i_graph_sel >= (ngraphs - 1))
		it->i_graph_sel = 0;
	else
		it->i_graph_sel++;

	return 0;
}
	
int prev_graph(void)
{
	struct item *it = item_current();
	if (it == NULL)
		return EMPTY_LIST;

	if (it->i_graph_sel <= 0)
		it->i_graph_sel = ngraphs - 1;
	else
		it->i_graph_sel--;

	return 0;
}

#endif
