/*
 * out_format.c		Formatted Output
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
#include <bmon/conf.h>
#include <bmon/output.h>
#include <bmon/group.h>
#include <bmon/element.h>
#include <bmon/input.h>
#include <bmon/utils.h>
#include <bmon/attr.h>

static int c_quit_after = -1;
static char *c_format;
static int c_debug = 0;
static FILE *c_fd;

enum {
	OT_STRING,
	OT_TOKEN
};

static struct out_token {
	int ot_type;
	char *ot_str;
} *out_tokens;

static int token_index;
static int out_tokens_size;

static char *get_token(struct element_group *g, struct element *e,
		       const char *token, char *buf, size_t len)
{
	if (!strncasecmp(token, "group:", 6)) {
		const char *n = token + 6;

		if (!strcasecmp(n, "nelements")) {
			snprintf(buf, len, "%u", g->g_nelements);
			return buf;
		} else if (!strcasecmp(n, "name"))
			return g->g_name;
		else if (!strcasecmp(n, "title"))
			return g->g_hdr->gh_title;

	} else if (!strncasecmp(token, "element:", 8)) {
		const char *n = token + 8;

		if (!strcasecmp(n, "name"))
			return e->e_name;
		else if (!strcasecmp(n, "description"))
			return e->e_description;
		else if (!strcasecmp(n, "nattrs")) {
			snprintf(buf, len, "%u", e->e_nattrs);
			return buf;
		} else if (!strcasecmp(n, "lifecycles")) {
			snprintf(buf, len, "%u", e->e_lifecycles);
			return buf;
		} else if (!strcasecmp(n, "level")) {
			snprintf(buf, len, "%u", e->e_level);
			return buf;
		} else if (!strcasecmp(n, "parent"))
			return e->e_parent ? e->e_parent->e_name : "";
		else if (!strcasecmp(n, "id")) {
			snprintf(buf, len, "%u", e->e_id);
			return buf;
		} else if (!strcasecmp(n, "rxusage")) {
			snprintf(buf, len, "%2.0f",
				e->e_rx_usage == FLT_MAX ? e->e_rx_usage : 0.0f);
			return buf;
		} else if (!strcasecmp(n, "txusage")) {
			snprintf(buf, len, "%2.0f",
				e->e_tx_usage == FLT_MAX ? e->e_tx_usage : 0.0f);
			return buf;
		} else if (!strcasecmp(n, "haschilds")) {
			snprintf(buf, len, "%u",
				list_empty(&e->e_childs) ? 0 : 1);
			return buf;
		}
	} else if (!strncasecmp(token, "attr:", 5)) {
		const char *type = token + 5;
		char *name = strchr(type, ':');
		struct attr_def *def;
		struct attr *a;

		if (!name) {
			fprintf(stderr, "Invalid attribute field \"%s\"\n",
				type);
			goto out;
		}

		name++;

		def = attr_def_lookup(name);
		if (!def) {
			fprintf(stderr, "Undefined attribute \"%s\"\n", name);
			goto out;
		}

		if (!(a = attr_lookup(e, def->ad_id)))
			goto out;

		if (!strncasecmp(type, "rx:", 3)) {
			snprintf(buf, len, "%" PRIu64, rate_get_total(&a->a_rx_rate));
			return buf;
		} else if (!strncasecmp(type, "tx:", 3)) {
			snprintf(buf, len, "%" PRIu64, rate_get_total(&a->a_tx_rate));
			return buf;
		} else if (!strncasecmp(type, "rxrate:", 7)) {
			snprintf(buf, len, "%.2f", a->a_rx_rate.r_rate);
			return buf;
		} else if (!strncasecmp(type, "txrate:", 7)) {
			snprintf(buf, len, "%.2f", a->a_tx_rate.r_rate);
			return buf;
		}
	}

	fprintf(stderr, "Unknown field \"%s\"\n", token);
out:
	return "unknown";
}

static void draw_element(struct element_group *g, struct element *e, void *arg)
{
	int i;

	for (i = 0; i < token_index; i++) {
		char buf[128];
		char *p;

		if (out_tokens[i].ot_type == OT_STRING)
			p = out_tokens[i].ot_str;
		else if (out_tokens[i].ot_type == OT_TOKEN)
			p = get_token(g, e, out_tokens[i].ot_str,
				      buf, sizeof(buf));
		else
			BUG();

		if (p)
			fprintf(c_fd, "%s", p);
	}
}

static void format_draw(void)
{
	group_foreach_recursive(draw_element, NULL);
	fflush(stdout);

	if (c_quit_after > 0)
		if (--c_quit_after == 0)
			exit(0);
}

static inline void add_token(int type, char *data)
{
	if (!out_tokens_size) {
		out_tokens_size = 32;
		out_tokens = calloc(out_tokens_size, sizeof(struct out_token));
		if (out_tokens == NULL)
			quit("Cannot allocate out token array\n");
	}

	if (out_tokens_size <= token_index) {
		out_tokens_size += 32;
		out_tokens = realloc(out_tokens, out_tokens_size * sizeof(struct out_token));
		if (out_tokens == NULL)
			quit("Cannot reallocate out token array\n");
	}


	out_tokens[token_index].ot_type = type;
	out_tokens[token_index].ot_str = data;
	token_index++;
}

static int format_probe(void)
{
	int new_one = 1;
	char *p, *e;

	for (p = c_format; *p; p++) {
		if (*p == '$') {
			char *s = p;
			s++;
			if (*s == '(') {
				s++;
				if (!*s)
					goto unexpected_end;
				e = strchr(s, ')');
				if (e == NULL)
					goto invalid;

				*p = '\0';
				*e = '\0';
				add_token(OT_TOKEN, s);
				new_one = 1;
				p = e;
				continue;
			}
		}

		if (*p == '\\') {
			char *s = p;
			s++;
			switch (*s) {
				case 'n':
					*s = '\n';
					goto finish_escape;
				case 't':
					*s = '\t';
					goto finish_escape;
				case 'r':
					*s = '\r';
					goto finish_escape;
				case 'v':
					*s = '\v';
					goto finish_escape;
				case 'b':
					*s = '\b';
					goto finish_escape;
				case 'f':
					*s = '\f';
					goto finish_escape;
				case 'a':
					*s = '\a';
					goto finish_escape;
			}

			goto out;

finish_escape:
			*p = '\0';
			add_token(OT_STRING, s);
			p = s;
			new_one = 0;
			continue;
		}

out:
		if (new_one) {
			add_token(OT_STRING, p);
			new_one = 0;
		}
	}

	if (c_debug) {
		int i;
		for (i = 0; i < token_index; i++)
			printf(">>%s<\n", out_tokens[i].ot_str);
	}

	return 1;

unexpected_end:
	fprintf(stderr, "Unexpected end of format string\n");
	return 0;

invalid:
	fprintf(stderr, "Missing ')' in format string\n");
	return 0;
}

static void print_help(void)
{
	printf(
	"format - Formatable Output\n" \
	"\n" \
	"  Formatable ASCII output for scripts. Calls a drawing function for\n" \
	"  every item per node and outputs according to the specified format\n" \
	"  string. The format string consists of normal text and placeholders\n" \
	"  in the form of $(placeholder).\n" \
	"\n" \
	"  Author: Thomas Graf <tgraf@suug.ch>\n" \
	"\n" \
	"  Options:\n" \
	"    fmt=FORMAT     Format string\n" \
	"    stderr         Write to stderr instead of stdout\n" \
	"    quitafter=NUM  Quit bmon after NUM outputs\n" \
	"\n" \
	"  Placeholders:\n" \
	"    group:nelements       Number of elements this group\n" \
	"         :name            Name of group\n" \
	"         :title           Title of group\n" \
	"    element:name          Name of element\n" \
	"           :desc          Description of element\n" \
	"           :nattrs        Number of attributes\n" \
	"           :lifecycles    Number of lifecycles\n" \
	"           :level         Indentation level\n" \
	"           :parent        Name of parent element\n" \
	"           :rxusage       RX usage in percent\n" \
	"           :txusage       TX usage in percent)\n" \
	"           :id            ID of element\n" \
	"           :haschilds     Indicate if element has children (0|1)\n" \
	"    attr:rx:<name>        RX counter of attribute <name>\n" \
	"        :tx:<name>        TX counter of attribute <name>\n" \
	"        :rxrate:<name>    RX rate of attribute <name>\n" \
	"        :txrate:<name>    TX rate of attribute <name>\n" \
	"\n" \
	"  Supported Escape Sequences: \\n, \\t, \\r, \\v, \\b, \\f, \\a\n" \
	"\n" \
	"  Examples:\n" \
	"    '$(element:name)\\t$(attr:rx:bytes)\\t$(attr:tx:bytes)\\n'\n" \
	"    lo      12074   12074\n" \
	"\n" \
	"    '$(element:name) $(attr:rxrate:packets) $(attr:txrate:packets)\\n'\n" \
	"    eth0 33 5\n" \
	"\n" \
	"    'Item: $(element:name)\\nBytes Rate: $(attr:rxrate:bytes)/" \
	"$(attr:txrate:bytes)\\nPackets Rate: $(attr:rxrate:packets)/" \
	"$(attr:txrate:packets)\\n'\n" \
	"    Item: eth0\n" \
	"    Bytes Rate: 49130/2119\n" \
	"    Packets Rate: 40/11\n" \
	"\n");
}

static void format_parse_opt(const char *type, const char *value)
{
	if (!strcasecmp(type, "stderr"))
		c_fd = stderr;
	else if (!strcasecmp(type, "debug"))
		c_debug = 1;
	else if (!strcasecmp(type, "fmt")) {
		if (c_format)
			free(c_format);
		c_format = strdup(value);
	} else if (!strcasecmp(type, "quitafter") &&
			       value)
		c_quit_after = strtol(value, NULL, 0);
	else if (!strcasecmp(type, "help")) {
		print_help();
		exit(0);
	}
}

static struct bmon_module format_ops = {
	.m_name		= "format",
	.m_do		= format_draw,
	.m_probe	= format_probe,
	.m_parse_opt	= format_parse_opt,
};

static void __init ascii_init(void)
{
	c_fd = stdout;
	c_format = strdup("$(element:name) $(attr:rx:bytes) $(attr:tx:bytes) " \
	    "$(attr:rx:packets) $(attr:tx:packets)\\n");

	output_register(&format_ops);
}
