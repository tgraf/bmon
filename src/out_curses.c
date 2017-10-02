/*
 * out_curses.c          Curses Output
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
#include <bmon/attr.h>
#include <bmon/element.h>
#include <bmon/element_cfg.h>
#include <bmon/input.h>
#include <bmon/history.h>
#include <bmon/graph.h>
#include <bmon/output.h>
#include <bmon/utils.h>

enum {
	GRAPH_DISPLAY_SIDE_BY_SIDE = 1,
	GRAPH_DISPLAY_STANDARD = 2,
};

enum {
	KEY_TOGGLE_LIST		= 'l',
	KEY_TOGGLE_GRAPH	= 'g',
	KEY_TOGGLE_DETAILS	= 'd',
	KEY_TOGGLE_INFO		= 'i',
	KEY_COLLECT_HISTORY	= 'h',
	KEY_CTRL_N	= 14,
	KEY_CTRL_P	= 16,
};

#define DETAILS_COLS		40

#define LIST_COL_1		31
#define LIST_COL_2		55

/* Set to element_current() before drawing */
static struct element *current_element;

static struct attr *current_attr;

/* Length of list to draw, updated in draw_content() */
static int list_length;

static int list_req;

/* Number of graphs to draw (may be < c_ngraph) */
static int ngraph;

/*
 * Offset in number of lines within the the element list of the currently
 * selected element. Updated while summing up required lines.
 */
static unsigned int selection_offset;

/*
 * Offset in number of lines of the first element to be drawn. Updated
 * in draw_content()
 */
static int offset;

/*
 * Offset to the first graph to draw in number of attributes with graphs.
 */
static unsigned int graph_offset;

static int graph_display = GRAPH_DISPLAY_STANDARD;

/*
 * Number of detail columns
 */
static int detail_cols;
static int info_cols;

static int initialized;
static int print_help;
static int quit_mode;
static int help_page;

/* Current row */
static int row;

/* Number of rows */
static int rows;

/* Number of columns */
static int cols;

static int c_show_graph = 1;
static int c_ngraph = 1;
static int c_use_colors = 1;
static int c_show_details = 0;
static int c_show_list = 1;
static int c_show_info = 0;
static int c_list_min = 6;

static struct graph_cfg c_graph_cfg = {
	.gc_width		= 60,
	.gc_height		= 6,
	.gc_foreground		= '|',
	.gc_background		= '.',
	.gc_noise		= ':',
	.gc_unknown		= '?',
};

#define NEXT_ROW()			\
	do {				\
		row++;			\
		if (row >= rows - 1)	\
			return;		\
		move(row, 0);		\
	} while(0)

static void apply_layout(int layout)
{
	if (c_use_colors)
		attrset(COLOR_PAIR(layout) | cfg_layout[layout].l_attr);
	else
		attrset(cfg_layout[layout].l_attr);
}

static char *float2str(double value, int width, int prec, char *buf, size_t len)
{
	snprintf(buf, len, "%'*.*f", width, value == 0.0f ? 0 : prec, value);

	return buf;
}

static void put_line(const char *fmt, ...)
{
	va_list args;
	char *buf;
	int len;
	int x, y __unused__;

	getyx(stdscr, y, x);

	len = cols - x;
	buf = xcalloc(len+1, 1);

	va_start(args, fmt);
	vsnprintf(buf, len+1, fmt, args);
	va_end(args);

	if (strlen(buf) < len)
		memset(&buf[strlen(buf)], ' ', len - strlen(buf));

	addstr(buf);
	xfree(buf);
}

static void center_text(const char *fmt, ...)
{
	va_list args;
	char *str;
	unsigned int col;

	va_start(args, fmt);
	if (vasprintf(&str, fmt, args) < 0) {
		fprintf(stderr, "vasprintf: Out of memory\n");
		exit(ENOMEM);
	}
	va_end(args);

	col = (cols / 2) - (strlen(str) / 2);
	if (col > cols - 1)
		col = cols - 1;

	move(row, col);
	addstr(str);
	move(row, 0);

	free(str);
}

static int curses_init(void)
{
	if (!initscr()) {
		fprintf(stderr, "Unable to initialize curses screen\n");
		return -EOPNOTSUPP;
	}

	initialized = 1;
	
	if (!has_colors())
		c_use_colors = 0;

	if (c_use_colors) {
		int i;
		
		start_color();

#if defined HAVE_USE_DEFAULT_COLORS
		use_default_colors();
#endif
		for (i = 1; i < LAYOUT_MAX+1; i++)
			init_pair(i, cfg_layout[i].l_fg, cfg_layout[i].l_bg);
	}
		
	keypad(stdscr, TRUE);
	nonl();
	cbreak();
	noecho();
	nodelay(stdscr, TRUE);	  /* getch etc. must be non-blocking */
	clear();
	curs_set(0);

	return 0;
}

static void curses_shutdown(void)
{
	if (initialized)
		endwin();
}

struct detail_arg
{
	int nattr;
};

static void draw_attr_detail(struct element *e, struct attr *a, void *arg)
{
	char *rx_u, *tx_u, buf1[32], buf2[32];
	int rxprec, txprec, ncol;
	struct detail_arg *da = arg;

	double rx = unit_value2str(rate_get_total(&a->a_rx_rate),
				   a->a_def->ad_unit,
				   &rx_u, &rxprec);
	double tx = unit_value2str(rate_get_total(&a->a_tx_rate),
				   a->a_def->ad_unit,
				   &tx_u, &txprec);

	if (da->nattr >= detail_cols) {
		NEXT_ROW();
		da->nattr = 0;
	}

	ncol = (da->nattr * DETAILS_COLS) - 1;
	move(row, ncol);
	if (ncol > 0)
		addch(ACS_VLINE);

	put_line(" %-14.14s %8s%-3s %8s%-3s",
		 a->a_def->ad_description,
		 (a->a_flags & ATTR_RX_ENABLED) ?
		 float2str(rx, 8, rxprec, buf1, sizeof(buf1)) : "-", rx_u,
		 (a->a_flags & ATTR_TX_ENABLED) ?
		 float2str(tx, 8, txprec, buf2, sizeof(buf2)) : "-", tx_u);

	da->nattr++;
}

static void draw_details(void)
{
	int i;
	struct detail_arg arg = {
		.nattr = 0,
	};

	if (!current_element->e_nattrs)
		return;

	for (i = 1; i < detail_cols; i++)
		mvaddch(row, (i * DETAILS_COLS) - 1, ACS_TTEE);

	NEXT_ROW();
	put_line("");
	for (i = 0; i < detail_cols; i++) {
		if (i > 0)
			mvaddch(row, (i * DETAILS_COLS) - 1, ACS_VLINE);
		move(row, (i * DETAILS_COLS) + 22);
		put_line("RX          TX");
	}

	NEXT_ROW();
	element_foreach_attr(current_element, draw_attr_detail, &arg);

	/*
	 * If the last row was incomplete, not all vlines have been drawn.
	 * draw them here
	 */
	for (i = 1; i < detail_cols; i++)
		mvaddch(row, (i * DETAILS_COLS - 1), ACS_VLINE);
}

static void print_message(const char *text)
{
	int i, y = (rows/2) - 2;
	int len = strlen(text);
	int x = (cols/2) - (len / 2);

	attrset(A_STANDOUT);
	mvaddch(y - 2, x - 1, ACS_ULCORNER);
	mvaddch(y + 2, x - 1, ACS_LLCORNER);
	mvaddch(y - 2, x + len, ACS_URCORNER);
	mvaddch(y + 2, x + len, ACS_LRCORNER);

	for (i = 0; i < 3; i++) {
		mvaddch(y - 1 + i, x + len, ACS_VLINE);
		mvaddch(y - 1 + i, x - 1 ,ACS_VLINE);
	}

	for (i = 0; i < len; i++) {
		mvaddch(y - 2, x + i, ACS_HLINE);
		mvaddch(y - 1, x + i, ' ');
		mvaddch(y + 1, x + i, ' ');
		mvaddch(y + 2, x + i, ACS_HLINE);
	}

	mvaddstr(y, x, text);
	attroff(A_STANDOUT);

	row = y + 2;
}

static void draw_help(void)
{
#define HW 46
#define HH 19
	int i, y = (rows/2) - (HH/2);
	int x = (cols/2) - (HW/2);
	char pad[HW+1];

	memset(pad, ' ', sizeof(pad));
	pad[sizeof(pad) - 1] = '\0';

	attron(A_STANDOUT);

	for (i = 0; i < HH; i++)
		mvaddnstr(y + i, x, pad, -1);

	mvaddch(y  - 1, x - 1, ACS_ULCORNER);
	mvaddch(y + HH, x - 1, ACS_LLCORNER);
	
	mvaddch(y  - 1, x + HW, ACS_URCORNER);
	mvaddch(y + HH, x + HW, ACS_LRCORNER);

	for (i = 0; i < HH; i++) {
		mvaddch(y + i, x -  1, ACS_VLINE);
		mvaddch(y + i, x + HW, ACS_VLINE);
	}

	for (i = 0; i < HW; i++) {
		mvaddch(y  - 1, x + i, ACS_HLINE);
		mvaddch(y + HH, x + i, ACS_HLINE);
	}

	attron(A_BOLD);
	mvaddnstr(y- 1, x+15, "QUICK REFERENCE", -1);
	attron(A_UNDERLINE);
	mvaddnstr(y+ 0, x+1, "Navigation", -1);
	attroff(A_BOLD | A_UNDERLINE);

	mvaddnstr(y+ 1, x+3, "Up, Down      Previous/Next element", -1);
	mvaddnstr(y+ 2, x+3, "PgUp, PgDown  Scroll up/down entire page", -1);
	mvaddnstr(y+ 3, x+3, "Left, Right   Previous/Next attribute", -1);
	mvaddnstr(y+ 4, x+3, "[, ]          Previous/Next group", -1);
	mvaddnstr(y+ 5, x+3, "?             Toggle quick reference", -1);
	mvaddnstr(y+ 6, x+3, "q             Quit bmon", -1);

	attron(A_BOLD | A_UNDERLINE);
	mvaddnstr(y+ 8, x+1, "Display Settings", -1);
	attroff(A_BOLD | A_UNDERLINE);

	mvaddnstr(y+ 9, x+3, "d             Toggle detailed statistics", -1);
	mvaddnstr(y+10, x+3, "l             Toggle element list", -1);
	mvaddnstr(y+11, x+3, "i             Toggle additional info", -1);

	attron(A_BOLD | A_UNDERLINE);
	mvaddnstr(y+13, x+1, "Graph Settings", -1);
	attroff(A_BOLD | A_UNDERLINE);

	mvaddnstr(y+14, x+3, "g             Toggle graphical statistics", -1);
	mvaddnstr(y+15, x+3, "H             Start recording history data", -1);
	mvaddnstr(y+16, x+3, "TAB           Switch time unit of graph", -1);
	mvaddnstr(y+17, x+3, "<, >          Change number of graphs", -1);
	mvaddnstr(y+18, x+3, "r             Reset counter of element", -1);

	attroff(A_STANDOUT);

	row = y + HH;
}

static int lines_required_for_header(void)
{
	return 1;
}

static void draw_header(void)
{
	apply_layout(LAYOUT_STATUSBAR);

	if (current_element)
		put_line(" %s %c%s%c",
			current_element->e_name,
			current_element->e_description ? '(' : ' ',
			current_element->e_description ? : "",
			current_element->e_description ? ')' : ' ');
	else
		put_line("");

	move(row, COLS - strlen(PACKAGE_STRING) - 1);
	put_line("%s", PACKAGE_STRING);
	move(row, 0);
	apply_layout(LAYOUT_LIST);
}

static int lines_required_for_statusbar(void)
{
	return 1;
}

static void draw_statusbar(void)
{
	static const char *help_text = "Press ? for help";
	char s[27];
	time_t t = time(NULL);

	apply_layout(LAYOUT_STATUSBAR);

	asctime_r(localtime(&t), s);
	s[strlen(s) - 1] = '\0';
	
	row = rows-1;
	move(row, 0);
	put_line(" %s", s);

	move(row, COLS - strlen(help_text) - 1);
	put_line("%s", help_text);

	move(row, 0);
}

static void count_attr_graph(struct element *g, struct attr *a, void *arg)
{
	if (a == current_attr)
		graph_offset = ngraph;

	ngraph++;
}

static int lines_required_for_graph(void)
{
	int lines = 0;

	ngraph = 0;

	if (c_show_graph && current_element) {
		graph_display = GRAPH_DISPLAY_STANDARD;

		element_foreach_attr(current_element, count_attr_graph, NULL);

		if (ngraph > c_ngraph)
			ngraph = c_ngraph;

		/* check if we have room to draw graphs on the same level */
		if (cols > (2 * (c_graph_cfg.gc_width + 10)))
			graph_display = GRAPH_DISPLAY_SIDE_BY_SIDE;

		/* +2 = header + time axis */
		lines = ngraph * (graph_display * (c_graph_cfg.gc_height + 2));
	}

	return lines + 1;
}

static int lines_required_for_details(void)
{
	int lines = 1;

	if (c_show_details && current_element) {
		lines++;	/* header */

		detail_cols = cols / DETAILS_COLS;

		if (!detail_cols)
			detail_cols = 1;

		lines += (current_element->e_nattrs / detail_cols);
		if (current_element->e_nattrs % detail_cols)
			lines++;
	}

	return lines;
}

static void count_element_lines(struct element_group *g, struct element *e,
				void *arg)
{
	int *lines = arg;

	if (e == current_element)
		selection_offset = *lines;
	
	(*lines)++;
}

static void count_group_lines(struct element_group *g, void *arg)
{
	int *lines = arg;

	/* group title */
	(*lines)++;

	group_foreach_element(g, &count_element_lines, arg);
}

static int lines_required_for_list(void)
{
	int lines = 0;

	if (c_show_list)
		group_foreach(&count_group_lines, &lines);
	else
		lines = 1;

	return lines;
}

static inline int line_visible(int line)
{
	return line >= offset && line < (offset + list_length);
}

static void draw_attr(double rate1, int prec1, char *unit1,
		      double rate2, int prec2, char *unit2,
		      float usage, int ncol)
{
	char buf[32];

	move(row, ncol);
	addch(ACS_VLINE);
	printw("%7s%-3s",
		float2str(rate1, 7, prec1, buf, sizeof(buf)), unit1);

	printw("%7s%-3s",
		float2str(rate2, 7, prec2, buf, sizeof(buf)), unit2);

	if (usage != FLT_MAX)
		printw("%2.0f%%", usage);
	else
		printw("%3s", "");
}

static void draw_element(struct element_group *g, struct element *e,
			 void *arg)
{
	int *line = arg;

	apply_layout(LAYOUT_LIST);

	if (line_visible(*line)) {
		char *rxu1 = "", *txu1 = "", *rxu2 = "", *txu2 = "";
		double rx1 = 0.0f, tx1 = 0.0f, rx2 = 0.0f, tx2 = 0.0f;
		char pad[IFNAMSIZ + 32];
		int rx1prec = 0, tx1prec = 0, rx2prec = 0, tx2prec = 0;
		struct attr *a;

		NEXT_ROW();

		if (e->e_key_attr[GT_MAJOR] &&
		    (a = attr_lookup(e, e->e_key_attr[GT_MAJOR]->ad_id)))
			attr_rate2float(a, &rx1, &rxu1, &rx1prec,
					&tx1, &txu1, &tx1prec);

		if (e->e_key_attr[GT_MINOR] &&
		    (a = attr_lookup(e, e->e_key_attr[GT_MINOR]->ad_id)))
			attr_rate2float(a, &rx2, &rxu2, &rx2prec,
					&tx2, &txu2, &tx2prec);

		memset(pad, 0, sizeof(pad));
		memset(pad, ' ', e->e_level < 6 ? e->e_level * 2 : 12);

		strncat(pad, e->e_name, sizeof(pad) - strlen(pad) - 1);

		if (e->e_description) {
			strncat(pad, " (", sizeof(pad) - strlen(pad) - 1);
			strncat(pad, e->e_description, sizeof(pad) - strlen(pad) - 1);
			strncat(pad, ")", sizeof(pad) - strlen(pad) - 1);
		}

		if (*line == offset) {
			attron(A_BOLD);
			addch(ACS_UARROW);
			attroff(A_BOLD);
			addch(' ');
		} else if (e == current_element) {
			apply_layout(LAYOUT_SELECTED);
			addch(' ');
			attron(A_BOLD);
			addch(ACS_RARROW);
			attroff(A_BOLD);
			apply_layout(LAYOUT_LIST);
		} else if (*line == offset + list_length - 1 &&
		           *line < (list_req - 1)) {
			attron(A_BOLD);
			addch(ACS_DARROW);
			attroff(A_BOLD);
			addch(' ');
		} else
			printw("  ");

		put_line("%-30.30s", pad);

		draw_attr(rx1, rx1prec, rxu1, rx2, rx2prec, rxu2,
			  e->e_rx_usage, LIST_COL_1);
	
		draw_attr(tx1, tx1prec, txu1, tx2, tx2prec, txu2,
			  e->e_tx_usage, LIST_COL_2);
		
	}

	(*line)++;
}

static void draw_group(struct element_group *g, void *arg)
{
	apply_layout(LAYOUT_HEADER);
	int *line = arg;

	if (line_visible(*line)) {
		NEXT_ROW();
		attron(A_BOLD);
		put_line("%s", g->g_hdr->gh_title);

		attroff(A_BOLD);
		mvaddch(row, LIST_COL_1, ACS_VLINE);
		attron(A_BOLD);
		put_line("%7s   %7s     %%",
			g->g_hdr->gh_column[0],
			g->g_hdr->gh_column[1]);

		attroff(A_BOLD);
		mvaddch(row, LIST_COL_2, ACS_VLINE);
		attron(A_BOLD);
		put_line("%7s   %7s     %%",
			g->g_hdr->gh_column[2],
			g->g_hdr->gh_column[3]);
	}

	(*line)++;

	group_foreach_element(g, draw_element, arg);
}

static void draw_element_list(void)
{
	int line = 0;

	group_foreach(draw_group, &line);
}

static inline int attr_visible(int nattr)
{
	return nattr >= graph_offset && nattr < (graph_offset + ngraph);
}

static void draw_graph_centered(struct graph *g, int row, int ncol,
				const char *text)
{
	int hcenter = (g->g_cfg.gc_width / 2) - (strlen(text) / 2) + 8;

	if (hcenter < 9)
		hcenter = 9;

	mvprintw(row, ncol + hcenter, "%.*s", g->g_cfg.gc_width, text);
}

static void draw_table(struct graph *g, struct graph_table *tbl,
		       struct attr *a, struct history *h,
		       const char *hdr, int ncol, int layout)
{
	int i, save_row;
	char buf[32];

	if (!tbl->gt_table) {
		for (i = g->g_cfg.gc_height; i >= 0; i--) {
			move(++row, ncol);
			put_line("");
		}
		return;
	}

	move(++row, ncol);
	put_line("%8s", tbl->gt_y_unit ? : "");

	snprintf(buf, sizeof(buf), "(%s %s/%s)",
		 hdr, a->a_def->ad_description,
		 h ? h->h_definition->hd_name : "?");

	draw_graph_centered(g, row, ncol, buf);

	//move(row, ncol + g->g_cfg.gc_width - 3);
	//put_line("[err %.2f%%]", rtiming.rt_variance.v_error);

    memset(buf, 0, strlen(buf));
	for (i = (g->g_cfg.gc_height - 1); i >= 0; i--) {
		move(++row, ncol);
        sprintf(buf, "%'8.2f ", tbl->gt_scale[i]);
        addstr(buf);
        apply_layout(layout);
        put_line("%s", tbl->gt_table + (i * graph_row_size(&g->g_cfg)));
        apply_layout(LAYOUT_LIST);
	}

	move(++row, ncol);
	put_line("         1");

	for (i = 1; i <= g->g_cfg.gc_width; i++) {
		if (i % 5 == 0) {
			move(row, ncol + i + 7);
			printw("%2d", i);
		}
	}

	if (!h) {
		const char *t1 = " No history data available. ";
		const char *t2 = " Press h to start collecting history. ";
		int vcenter = g->g_cfg.gc_height / 2;

		save_row = row;
		draw_graph_centered(g, save_row - vcenter - 1, ncol, t1);
		draw_graph_centered(g, save_row - vcenter, ncol, t2);
		row = save_row;
	}
}

static void draw_history_graph(struct attr *a, struct history *h)
{
	struct graph *g;
	int ncol = 0, save_row;

	g = graph_alloc(h, &c_graph_cfg);
	graph_refill(g, h);

	save_row = row;
	draw_table(g, &g->g_rx, a, h, "RX", ncol, LAYOUT_RX_GRAPH);

	if (graph_display == GRAPH_DISPLAY_SIDE_BY_SIDE) {
		ncol = cols / 2;
		row = save_row;
	}

	draw_table(g, &g->g_tx, a, h, "TX", ncol, LAYOUT_TX_GRAPH);

	graph_free(g);
}

static void draw_attr_graph(struct element *e, struct attr *a, void *arg)
{
	int *nattr = arg;

	if (attr_visible(*nattr)) {
		struct history_def *sel;
		struct history *h;

		sel = history_current();
		c_graph_cfg.gc_unit = a->a_def->ad_unit;

		list_for_each_entry(h, &a->a_history_list, h_list) {
			if (h->h_definition != sel)
				continue;

			draw_history_graph(a, h);
			goto out;
		}

		draw_history_graph(a, NULL);
	}

out:
	(*nattr)++;
}

static void draw_graph(void)
{
	int nattr = 0;

	element_foreach_attr(current_element, &draw_attr_graph, &nattr);
}

static int lines_required_for_info(void)
{
	int lines = 1;

	if (c_show_info) {
		info_cols = cols / DETAILS_COLS;

		if (!info_cols)
			info_cols = 1;

		lines += (current_element->e_ninfo / info_cols);
		if (current_element->e_ninfo % info_cols)
			lines++;
	}

	return lines;
}

static void __draw_info(struct element *e, struct info *info, int *ninfo)
{
	int ncol;

	ncol = ((*ninfo) * DETAILS_COLS) - 1;
	move(row, ncol);
	if (ncol > 0)
		addch(ACS_VLINE);

	put_line(" %-14.14s %22.22s", info->i_name, info->i_value);

	if (++(*ninfo) >= info_cols) {
		NEXT_ROW();
		*ninfo = 0;
	}
}

static void draw_info(void)
{
	struct info *info;
	int i, ninfo = 0;

	if (!current_element->e_ninfo)
		return;

	for (i = 1; i < detail_cols; i++)
		mvaddch(row, (i * DETAILS_COLS) - 1,
			c_show_details ? ACS_PLUS : ACS_TTEE);

	NEXT_ROW();
	list_for_each_entry(info, &current_element->e_info_list, i_list)
		__draw_info(current_element, info, &ninfo);

	/*
	 * If the last row was incomplete, not all vlines have been drawn.
	 * draw them here
	 */
	for (i = 1; i < info_cols; i++)
		mvaddch(row, (i * DETAILS_COLS - 1), ACS_VLINE);
}

static void draw_content(void)
{
	int graph_req, details_req, lines_available, total_req;
	int info_req, empty_lines;
	int disable_graph = 0, disable_details = 0, disable_info = 0;

	if (!current_element)
		return;

	/*
	 * Reset selection offset. Will be set in lines_required_for_list().
	 */
	selection_offset = 0;
	offset = 0;

	/* Reset graph offset, will be set in lines_required_for_graph() */
	graph_offset = 0;

	lines_available = rows - lines_required_for_statusbar()
			  - lines_required_for_header();

	list_req = lines_required_for_list();
	graph_req = lines_required_for_graph();
	details_req = lines_required_for_details();
	info_req = lines_required_for_info();

	total_req = list_req + graph_req + details_req + info_req;

	if (total_req <= lines_available) {
		/*
		 * Enough lines available for all data to displayed, all
		 * is good. Display the full list.
		 */
		list_length = list_req;
		goto draw;
	}

	/*
	 * Not enough lines available for full list and all details
	 * requested...
	 */

	if (c_show_list) {
		/*
		 * ... try shortening the list first.
		 */
		list_length = lines_available - (total_req - list_req);
		if (list_length >= c_list_min)
			goto draw;
	}

	if (c_show_info) {
		/* try disabling info */
		list_length = lines_available - (total_req - info_req + 1);
		if (list_length >= c_list_min) {
			disable_info = 1;
			goto draw;
		}
	}

	if (c_show_details) {
		/* ... try disabling details */
		list_length = lines_available - (total_req - details_req + 1);
		if (list_length >= c_list_min) {
			disable_details = 1;
			goto draw;
		}
	}

	/* ... try disabling graph, details, and info */
	list_length = lines_available - 1 - 1 - 1;
	if (list_length >= c_list_min) {
		disable_graph = 1;
		disable_details = 1;
		disable_info = 1;
		goto draw;
	}

	NEXT_ROW();
	put_line("A minimum of %d lines is required to display content.\n",
		 (rows - lines_available) + c_list_min + 2);
	return;

draw:
	if (selection_offset && list_length > 0) {
		/*
		 * Vertically align the selected element in the middle
		 * of the list.
		 */
		offset = selection_offset - (list_length / 2);

		/*
		 * If element 0..(list_length/2) is selected, offset is
		 * negative here. Start drawing from first element.
		 */
		if (offset < 0)
			offset = 0;

		/*
		 * Ensure the full list length is used if one of the
		 * last (list_length/2) elements is selected.
		 */
		if (offset > (list_req - list_length))
			offset = (list_req - list_length);

		if (offset >= list_req)
			BUG();
	}

	if (c_show_list) {
		draw_element_list();
	} else {
		NEXT_ROW();
		hline(ACS_HLINE, cols);
		center_text(" Press %c to enable list view ",
			    KEY_TOGGLE_LIST);
	}

	/*
	 * Graphical statistics
	 */
	NEXT_ROW();
	hline(ACS_HLINE, cols);
	if (c_show_list) {
		mvaddch(row, LIST_COL_1, ACS_BTEE);
		mvaddch(row, LIST_COL_2, ACS_BTEE);
	}

	if (!c_show_graph)
		center_text(" Press %c to enable graphical statistics ",
			    KEY_TOGGLE_GRAPH);
	else {
		if (disable_graph)
			center_text(" Increase screen height to see graphical statistics ");
		else
			draw_graph();
	}

	empty_lines = rows - row - details_req - info_req
		      - lines_required_for_statusbar() - 1;

	while (empty_lines-- > 0) {
		NEXT_ROW();
		put_line("");
	}

	/*
	 * Detailed statistics
	 */
	NEXT_ROW();
	hline(ACS_HLINE, cols);

	if (!c_show_details)
		center_text(" Press %c to enable detailed statistics ",
			    KEY_TOGGLE_DETAILS);
	else {
		if (disable_details)
			center_text(" Increase screen height to see detailed statistics ");
		else
			draw_details();
	}

	/*
	 * Additional information
	 */
	NEXT_ROW();
	hline(ACS_HLINE, cols);

	if (c_show_details) {
		int i;
		for (i = 1; i < detail_cols; i++)
			mvaddch(row, (i * DETAILS_COLS) - 1, ACS_BTEE);
	}

	if (!c_show_info)
		center_text(" Press %c to enable additional information ",
			    KEY_TOGGLE_INFO);
	else {
		if (disable_info)
			center_text(" Increase screen height to see additional information ");
		else
			draw_info();
	}
}


static void curses_draw(void)
{
	row = 0;
	move(0,0);
	
	getmaxyx(stdscr, rows, cols);

	if (rows < 4) {
		clear();
		put_line("Screen must be at least 4 rows in height");
		goto out;
	}

	if (cols < 48) {
		clear();
		put_line("Screen must be at least 48 columns width");
		goto out;
	}

	current_element = element_current();
	current_attr = attr_current();

	draw_header();

	apply_layout(LAYOUT_DEFAULT);
	draw_content();

	/* fill empty lines with blanks */
	while (row < (rows - 1 - lines_required_for_statusbar())) {
		move(++row, 0);
		put_line("");
	}

	draw_statusbar();

	if (quit_mode)
		print_message(" Really Quit? (y/n) ");
	else if (print_help) {
		if (help_page == 0)
			draw_help();
#if 0
		else
			draw_help_2();
#endif
	}

out:
	attrset(0);
	refresh();
}

static void __reset_attr_counter(struct element *e, struct attr *a, void *arg)
{
	attr_reset_counter(a);
}

static void reset_counters(void)
{
	element_foreach_attr(current_element, __reset_attr_counter, NULL);
}

static int handle_input(int ch)
{
	switch (ch) 
	{
		case 'q':
			if (print_help)
				print_help = 0;
			else
				quit_mode = quit_mode ? 0 : 1;
			return 1;

		case 0x1b:
			quit_mode = 0;
			print_help = 0;
			return 1;

		case 'y':
			if (quit_mode)
				exit(0);
			break;

		case 'n':
			if (quit_mode)
				quit_mode = 0;
			return 1;

		case 12:
		case KEY_CLEAR:
#ifdef HAVE_REDRAWWIN
			redrawwin(stdscr);
#endif
			clear();
			return 1;

		case '?':
			clear();
			print_help = print_help ? 0 : 1;
			return 1;

		case KEY_TOGGLE_GRAPH:
			c_show_graph = !c_show_graph;
			if (c_show_graph && !c_ngraph)
				c_ngraph = 1;
			return 1;

		case KEY_TOGGLE_DETAILS:
			c_show_details = !c_show_details;
			return 1;

		case KEY_TOGGLE_LIST:
			c_show_list = !c_show_list;
			return 1;

		case KEY_TOGGLE_INFO:
			c_show_info = !c_show_info;
			return 1;

		case KEY_COLLECT_HISTORY:
			if (current_attr) {
				attr_start_collecting_history(current_attr);
				return 1;
			}
			break;

		case KEY_PPAGE:
			{
				int i;
				for (i = 1; i < list_length; i++)
					element_select_prev();
			}
			return 1;

		case KEY_NPAGE:
			{
				int i;
				for (i = 1; i < list_length; i++)
					element_select_next();
			}
			return 1;

		case KEY_DOWN:
		case KEY_CTRL_N:
			element_select_next();
			return 1;

		case KEY_UP:
		case KEY_CTRL_P:
			element_select_prev();
			return 1;

		case KEY_LEFT:
			attr_select_prev();
			return 1;

		case KEY_RIGHT:
			attr_select_next();
			return 1;

		case ']':
			group_select_next();
			return 1;

		case '[':
			group_select_prev();
			return 1;

		case '<':
			c_ngraph--;
			if (c_ngraph <= 1)
				c_ngraph = 1;
			return 1;

		case '>':
			c_ngraph++;
			if (c_ngraph > 32)
				c_ngraph = 32;
			return 1;

		case '\t':
			history_select_next();
			return 1;

		case 'r':
			reset_counters();
			return 1;
	}

	return 0;
}

static void curses_pre(void)
{
	static int init = 0;

	if (!init) {
		curses_init();
		init = 1;
	}

	for (;;) {
		int ch = getch();

		if (ch == -1)
			break;

		if (handle_input(ch))
			curses_draw();
	}
}

static void print_module_help(void)
{
	printf(
	"curses - Curses Output\n" \
	"\n" \
	"  Interactive curses UI. Press '?' to see help.\n" \
	"  Author: Thomas Graf <tgraf@suug.ch>\n" \
	"\n" \
	"  Options:\n" \
	"    fgchar=CHAR    Foreground character (default: '|')\n" \
	"    bgchar=CHAR    Background character (default: '.')\n" \
	"    nchar=CHAR     Noise character (default: ':')\n" \
	"    uchar=CHAR     Unknown character (default: '?')\n" \
	"    gheight=NUM    Height of graph (default: 6)\n" \
	"    gwidth=NUM     Width of graph (default: 60)\n" \
	"    ngraph=NUM     Number of graphs (default: 1)\n" \
	"    nocolors       Do not use colors\n" \
	"    graph          Show graphical stats by default\n" \
	"    details        Show detailed stats by default\n" \
	"    info           Show additional info screen by default\n" \
	"    minlist=INT    Minimum item list length\n");
}

static void curses_parse_opt(const char *type, const char *value)
{
	if (!strcasecmp(type, "fgchar") && value)
		c_graph_cfg.gc_foreground = value[0];
	else if (!strcasecmp(type, "bgchar") && value)
		c_graph_cfg.gc_background = value[0];
	else if (!strcasecmp(type, "nchar") && value)
		c_graph_cfg.gc_noise = value[0];
	else if (!strcasecmp(type, "uchar") && value)
		c_graph_cfg.gc_unknown = value[0];
	else if (!strcasecmp(type, "gheight") && value)
		c_graph_cfg.gc_height = strtol(value, NULL, 0);
	else if (!strcasecmp(type, "gwidth") && value)
		c_graph_cfg.gc_width = strtol(value, NULL, 0);
	else if (!strcasecmp(type, "ngraph") && value) {
		c_ngraph = strtol(value, NULL, 0);
		c_show_graph = !!c_ngraph;
	} else if (!strcasecmp(type, "details"))
		c_show_details = 1;
	else if (!strcasecmp(type, "info"))
		c_show_info = 1;
	else if (!strcasecmp(type, "nocolors"))
		c_use_colors = 0;
	else if (!strcasecmp(type, "minlist") && value)
		c_list_min = strtol(value, NULL, 0);
	else if (!strcasecmp(type, "help")) {
		print_module_help();
		exit(0);
	}
}

static struct bmon_module curses_ops = {
	.m_name		= "curses",
	.m_flags	= BMON_MODULE_DEFAULT,
	.m_shutdown	= curses_shutdown,
	.m_pre		= curses_pre,
	.m_do		= curses_draw,
	.m_parse_opt	= curses_parse_opt,
};

static void __init do_curses_init(void)
{
	output_register(&curses_ops);
}
