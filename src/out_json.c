/*
 * out_json.c           ASCII Output
 *
 * Copyright (c) 2017 Paul Miller <paul@jettero.pl>
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
    
static int c_quit_after = -1;
static char uschar = 10;
static int e_count, g_count;



static void print_attr_detail(struct element *e, struct attr *a, void *arg) {
    char *rx_u, *tx_u;
    int rxprec, txprec;

    double rx = unit_value2str(rate_get_total(&a->a_rx_rate), a->a_def->ad_unit, &rx_u, &rxprec);
    double tx = unit_value2str(rate_get_total(&a->a_tx_rate), a->a_def->ad_unit, &tx_u, &txprec);

    printf(",\n      \"%s\": { \"desc\": \"%s\", \"rx\": [%.*f,\"%s\"], \"tx\": [%.*f,\"%s\"]}",
            a->a_def->ad_name,
            a->a_def->ad_description,
            rxprec, rx, rx_u,
            txprec, tx, tx_u);
}

static void json_draw_element(struct element_group *g, struct element *e, void *arg) {
    if( e_count > 0 )
        printf(",");
    printf("\n    {\n");
    printf("      \"name\": \"%s\"", e->e_name);
    if (e->e_id)
        printf(",\n      \"id\": %u", e->e_id);
    if (e->e_parent)
        printf(",\n      \"parent\": \"%s\"", e->e_parent->e_name);
    element_foreach_attr(e, print_attr_detail, NULL);
    printf("\n    }");
    e_count ++;
}

static void json_draw_group(struct element_group *g, void *arg) {
    if (g_count > 0)
        printf(",\n");
    printf("  { \"name\": \"%s\", \"elements\": [", g->g_name);
    e_count = 0;
    group_foreach_element(g, json_draw_element, NULL);
    if( e_count > 0 )
        printf("\n  ");
    printf("]}", g->g_name);
    g_count ++;
}

static void json_draw(void) {
    printf("[\n");
    g_count = 0;
    group_foreach(json_draw_group, NULL);
    if( g_count > 0 )
        printf("\n");
    printf("]\n%c", uschar);
    fflush(stdout);

    if (c_quit_after > 0)
        if (--c_quit_after == 0)
            exit(0);
}

static void print_help(void) {
    printf("json - json output\n" \
        "\n" \
        "  Options:\n" \
	"    quitafter=NUM  Quit bmon after NUM outputs\n" \
	"    uschar=CHAR    Unit separator character (default: \\x0a)\n" \
    );
}

static void json_parse_opt(const char *type, const char *value) {
    if (!strcasecmp(type, "help")) {
        print_help();
        exit(0);

    }
    
    else if (!strcasecmp(type, "uschar") && value)
        uschar = value[0];

    else if (!strcasecmp(type, "quitafter") && value)
        c_quit_after = strtol(value, NULL, 0);

    else quit("Unknown module option '%s'\n", type);
}

static struct bmon_module json_ops = {
    .m_name      = "json",
    .m_do        = json_draw,
    .m_parse_opt = json_parse_opt,
};

static void __init json_init(void) {
    output_register(&json_ops);
}
