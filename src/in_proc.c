/*
 * in_proc.c		       /proc/net/dev Input (Linux)
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
#include <bmon/input.h>
#include <bmon/element.h>
#include <bmon/group.h>
#include <bmon/attr.h>
#include <bmon/utils.h>

static const char *c_path = "/proc/net/dev";
static const char *c_group = DEFAULT_GROUP;
static struct element_group *grp;

enum {
	PROC_BYTES,
	PROC_PACKETS,
	PROC_ERRORS,
	PROC_DROP,
	PROC_COMPRESSED,
	PROC_FIFO,
	PROC_FRAME,
	PROC_MCAST,
	NUM_PROC_VALUE,
};

static struct attr_map link_attrs[NUM_PROC_VALUE] = {
{
	.name		= "bytes",
	.type		= ATTR_TYPE_COUNTER,
	.unit		= UNIT_BYTE,
	.description	= "Bytes",
},
{
	.name		= "packets",
	.type		= ATTR_TYPE_COUNTER,
	.unit		= UNIT_NUMBER,
	.description	= "Packets",
},
{
	.name		= "errors",
	.type		= ATTR_TYPE_COUNTER,
	.unit		= UNIT_NUMBER,
	.description	= "Errors",
},
{
	.name		= "drop",
	.type		= ATTR_TYPE_COUNTER,
	.unit		= UNIT_NUMBER,
	.description	= "Dropped",
},
{
	.name		= "compressed",
	.type		= ATTR_TYPE_COUNTER,
	.unit		= UNIT_NUMBER,
	.description	= "Compressed",
},
{
	.name		= "fifoerr",
	.type		= ATTR_TYPE_COUNTER,
	.unit		= UNIT_NUMBER,
	.description	= "FIFO Error",
},
{
	.name		= "frameerr",
	.type		= ATTR_TYPE_COUNTER,
	.unit		= UNIT_NUMBER,
	.description	= "Frame Error",
},
{
	.name		= "mcast",
	.type		= ATTR_TYPE_COUNTER,
	.unit		= UNIT_NUMBER,
	.description	= "Multicast",
}
};

static void proc_read(void)
{
	struct element *e;
	FILE *fd;
	char buf[512], *p, *s, *unused __unused__;
	int w;
	
	if (!(fd = fopen(c_path, "r")))
		quit("Unable to open file %s: %s\n", c_path, strerror(errno));

	/* Ignore header */
	unused = fgets(buf, sizeof(buf), fd);
	unused = fgets(buf, sizeof(buf), fd);
	
	for (; fgets(buf, sizeof(buf), fd);) {
		uint64_t data[NUM_PROC_VALUE][2];
		int i;
		
		if (buf[0] == '\r' || buf[0] == '\n')
			continue;

		if (!(p = strchr(buf, ':')))
			continue;
		*p = '\0';
		s = (p + 1);
		
		for (p = &buf[0]; *p == ' '; p++);

		w = sscanf(s, "%" SCNu64 " %" SCNu64 " %" SCNu64 " %" SCNu64 " "
			      "%" SCNu64 " %" SCNu64 " %" SCNu64 " %" SCNu64 " "
			      "%" SCNu64 " %" SCNu64 " %" SCNu64 " %" SCNu64 " "
			      "%" SCNu64 " %" SCNu64 " %" SCNu64 " %" SCNu64
			      "\n",
			      &data[PROC_BYTES][0],
			      &data[PROC_PACKETS][0],
			      &data[PROC_ERRORS][0],
			      &data[PROC_DROP][0],
			      &data[PROC_FIFO][0],
			      &data[PROC_FRAME][0],
			      &data[PROC_COMPRESSED][0],
			      &data[PROC_MCAST][0],
			      &data[PROC_BYTES][1],
			      &data[PROC_PACKETS][1],
			      &data[PROC_ERRORS][1],
			      &data[PROC_DROP][1],
			      &data[PROC_FIFO][1],
			      &data[PROC_FRAME][1],
			      &data[PROC_COMPRESSED][1],
			      &data[PROC_MCAST][1]);

		if (w != 16)
			continue;

		if (!(e = element_lookup(grp, p, 0, NULL, ELEMENT_CREAT)))
			goto skip;

		if (e->e_flags & ELEMENT_FLAG_CREATED) {
			if (element_set_key_attr(e, "bytes", "packets") ||
			    element_set_usage_attr(e, "bytes"))
				BUG();

			e->e_flags &= ~ELEMENT_FLAG_CREATED;
		}

		for (i = 0; i < ARRAY_SIZE(link_attrs); i++) {
			struct attr_map *m = &link_attrs[i];

			attr_update(e, m->attrid, data[i][0], data[i][1],
				    UPDATE_FLAG_RX | UPDATE_FLAG_TX);
		}

		element_notify_update(e, NULL);
		element_lifesign(e, 1);
	}
skip:
	fclose(fd);
}

static void print_help(void)
{
	printf(
	"proc - procfs statistic collector for Linux" \
	"\n" \
	"  Reads statistics from procfs (/proc/net/dev)\n" \
	"  Author: Thomas Graf <tgraf@suug.ch>\n" \
	"\n" \
	"  Options:\n" \
	"    file=PATH	    Path to statistics file (default: /proc/net/dev)\n"
	"    group=NAME     Name of group\n");
}

static void proc_parse_opt(const char *type, const char *value)
{
	if (!strcasecmp(type, "file") && value)
		c_path = value;
	else if (!strcasecmp(type, "group") && value)
		c_group = value;
	else if (!strcasecmp(type, "help")) {
		print_help();
		exit(0);
	}
}

static int proc_do_init(void)
{
	if (attr_map_load(link_attrs, ARRAY_SIZE(link_attrs)) ||
	    !(grp = group_lookup(c_group, GROUP_CREATE)))
		BUG();

	return 0;
}

static int proc_probe(void)
{
	FILE *fd = fopen(c_path, "r");

	if (fd) {
		fclose(fd);
		return 1;
	}
	return 0;
}

static struct bmon_module proc_ops = {
	.m_name		= "proc",
	.m_do		= proc_read,
	.m_parse_opt	= proc_parse_opt,
	.m_probe	= proc_probe,
	.m_init		= proc_do_init,
};

static void __init proc_init(void)
{
	input_register(&proc_ops);
}
