/*
 * out_ascii.c           ASCII Output
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
#include <bmon/output.h>
#include <bmon/group.h>
#include <bmon/element.h>
#include <bmon/attr.h>
#include <bmon/graph.h>
#include <bmon/history.h>
#include <bmon/utils.h>

typedef enum diagram_type_e {
	D_LIST,
	D_GRAPH,
	D_DETAILS
} diagram_type_t;

static struct graph_cfg graph_cfg = {
	.gc_foreground = '*',
	.gc_background = ' ',
	.gc_noise = '.',
	.gc_unknown = '?',
	.gc_height = 6,
};

static diagram_type_t c_diagram_type = D_LIST;
static char *c_hist = "second";
static int c_quit_after = -1;

static void print_list(struct element *e)
{
	char *rxu1 = "", *txu1 = "", *rxu2 = "", *txu2 = "";
	double rx1 = 0.0f, tx1 = 0.0f, rx2 = 0.0f, tx2 = 0.0f;
	int rx1prec = 0, tx1prec = 0, rx2prec = 0, tx2prec = 0;
	char pad[IFNAMSIZ + 32];
	struct attr *a;

	if (e->e_key_attr[GT_MAJOR] &&
	    (a = attr_lookup(e, e->e_key_attr[GT_MAJOR]->ad_id)))
		attr_rate2float(a, &rx1, &rxu1, &rx1prec,
				   &tx1, &txu1, &tx1prec);

	if (e->e_key_attr[GT_MINOR] &&
	    (a = attr_lookup(e, e->e_key_attr[GT_MINOR]->ad_id)))
		attr_rate2float(a, &rx2, &rxu2, &rx2prec,
				   &tx2, &txu2, &tx2prec);

	memset(pad, 0, sizeof(pad));
	memset(pad, ' ', e->e_level < 15 ? e->e_level : 15);

	strncat(pad, e->e_name, sizeof(pad) - strlen(pad) - 1);

	if (e->e_description) {
		strncat(pad, " (", sizeof(pad) - strlen(pad) - 1);
		strncat(pad, e->e_description, sizeof(pad) - strlen(pad) - 1);
		strncat(pad, ")", sizeof(pad) - strlen(pad) - 1);
	}

	printf("  %-36s %8.*f%-3s %8.*f%-3s ",
	       pad, rx1prec, rx1, rxu1, rx2prec, rx2, rxu2);

	if (e->e_rx_usage == FLT_MAX)
		printf("   ");
	else
		printf("%2.0f%%", e->e_rx_usage);

	printf("  %8.*f%-3s %8.*f%-3s ", tx1prec, tx1, txu1, tx2prec, tx2, txu2);

	if (e->e_tx_usage == FLT_MAX)
		printf("   \n");
	else
		printf("%2.0f%%\n", e->e_tx_usage);
}

static void print_attr_detail(struct element *e, struct attr *a, void *arg)
{
	char *rx_u, *tx_u;
	int rxprec, txprec;

	double rx = unit_value2str(rate_get_total(&a->a_rx_rate),
				   a->a_def->ad_unit,
				   &rx_u, &rxprec);
	double tx = unit_value2str(rate_get_total(&a->a_tx_rate),
				   a->a_def->ad_unit,
				   &tx_u, &txprec);

	printf("  %-36s %12.*f%-3s %12.*f%-3s\n",
	       a->a_def->ad_description, rxprec, rx, rx_u, txprec, tx, tx_u);
}

static void print_details(struct element *e)
{
	printf(" %s", e->e_name);

	if (e->e_id)
		printf(" (%u)", e->e_id);

	printf("\n");

	element_foreach_attr(e, print_attr_detail, NULL);

	printf("\n");
}

static void print_table(struct graph *g, struct graph_table *tbl, const char *hdr)
{
	int i;

	if (!tbl->gt_table)
		return;

	printf("%s   %s\n", hdr, tbl->gt_y_unit);

	for (i = (g->g_cfg.gc_height - 1); i >= 0; i--)
		printf("%8.2f %s\n", tbl->gt_scale[i],
		    tbl->gt_table + (i * graph_row_size(&g->g_cfg)));
	
	printf("         1   5   10   15   20   25   30   35   40   " \
		"45   50   55   60\n");
}

static void __print_graph(struct element *e, struct attr *a, void *arg)
{
	struct history *h;
	struct graph *g;

	if (!(a->a_flags & ATTR_DOING_HISTORY))
		return;

	graph_cfg.gc_unit = a->a_def->ad_unit;

	list_for_each_entry(h, &a->a_history_list, h_list) {
		if (strcasecmp(c_hist, h->h_definition->hd_name))
			continue;

		g = graph_alloc(h, &graph_cfg);
		graph_refill(g, h);

		printf("Interface: %s\n", e->e_name);
		printf("Attribute: %s\n", a->a_def->ad_description);

		print_table(g, &g->g_rx, "RX");
		print_table(g, &g->g_tx, "TX");

		graph_free(g);

	}
}

static void print_graph(struct element *e)
{
	element_foreach_attr(e, __print_graph, NULL);
}

static void ascii_draw_element(struct element_group *g, struct element *e,
			       void *arg)
{
	switch (c_diagram_type) {
		case D_LIST:
			print_list(e);
			break;

		case D_DETAILS:
			print_details(e);
			break;

		case D_GRAPH:
			print_graph(e);
			break;
	}
}

static void ascii_draw_group(struct element_group *g, void *arg)
{
	if (c_diagram_type == D_LIST)
		printf("%-37s%10s %11s      %%%10s %11s      %%\n",
		       g->g_hdr->gh_title,
		       g->g_hdr->gh_column[0],
		       g->g_hdr->gh_column[1],
		       g->g_hdr->gh_column[2],
		       g->g_hdr->gh_column[3]);
	else
		printf("%s\n", g->g_hdr->gh_title);

	group_foreach_element(g, ascii_draw_element, NULL);
}

static void ascii_draw(void)
{
	group_foreach(ascii_draw_group, NULL);
	fflush(stdout);

	if (c_quit_after > 0)
		if (--c_quit_after == 0)
			exit(0);
}

static void print_help(void)
{
	printf(
	"ascii - ASCII Output\n" \
	"\n" \
	"  Plain configurable ASCII output.\n" \
	"\n" \
	"  scriptable: (output graph for eth1 10 times)\n" \
	"      bmon -p eth1 -o 'ascii:diagram=graph;quitafter=10'\n" \
	"  show details for all ethernet interfaces:\n" \
	"      bmon -p 'eth*' -o 'ascii:diagram=details;quitafter=1'\n" \
	"\n" \
	"  Author: Thomas Graf <tgraf@suug.ch>\n" \
	"\n" \
	"  Options:\n" \
	"    diagram=TYPE   Diagram type\n" \
	"    fgchar=CHAR    Foreground character (default: '*')\n" \
	"    bgchar=CHAR    Background character (default: '.')\n" \
	"    nchar=CHAR     Noise character (default: ':')\n" \
	"    uchar=CHAR     Unknown character (default: '?')\n" \
	"    height=NUM     Height of graph (default: 6)\n" \
	"    xunit=UNIT     X-Axis Unit (default: seconds)\n" \
	"    yunit=UNIT     Y-Axis Unit (default: dynamic)\n" \
	"    quitafter=NUM  Quit bmon after NUM outputs\n");
}

static void ascii_parse_opt(const char *type, const char *value)
{
	if (!strcasecmp(type, "diagram") && value) {
		if (tolower(*value) == 'l')
			c_diagram_type = D_LIST;
		else if (tolower(*value) == 'g')
			c_diagram_type = D_GRAPH;
		else if (tolower(*value) == 'd')
			c_diagram_type = D_DETAILS;
		else
			quit("Unknown diagram type '%s'\n", value);
	} else if (!strcasecmp(type, "fgchar") && value)
		graph_cfg.gc_foreground = value[0];
	else if (!strcasecmp(type, "bgchar") && value)
		graph_cfg.gc_background = value[0];
	else if (!strcasecmp(type, "nchar") && value)
		graph_cfg.gc_noise = value[0];
#if 0
	else if (!strcasecmp(type, "uchar") && value)
		set_unk_char(value[0]);
#endif
	else if (!strcasecmp(type, "xunit") && value)
		c_hist = strdup(value);
#if 0
	else if (!strcasecmp(type, "yunit") && value) {
		struct unit *u;

		if (!(u = unit_lookup(value)))
			quit("Unknown unit '%s'\n", value);

		graph_cfg.gc_unit = u;
#endif
	else if (!strcasecmp(type, "height") && value)
		graph_cfg.gc_height = strtol(value, NULL, 0);
	else if (!strcasecmp(type, "quitafter") && value)
		c_quit_after = strtol(value, NULL, 0);
	else if (!strcasecmp(type, "help")) {
		print_help();
		exit(0);
	} else
		quit("Unknown module option '%s'\n", type);
}

static struct bmon_module ascii_ops = {
	.m_name		= "ascii",
	.m_do		= ascii_draw,
	.m_parse_opt	= ascii_parse_opt,
};

static void __init ascii_init(void)
{
	output_register(&ascii_ops);
}
