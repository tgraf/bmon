/*
 * group.c		Group Management
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
#include <bmon/element.h>
#include <bmon/group.h>
#include <bmon/utils.h>

static LIST_HEAD(titles_list);
static LIST_HEAD(group_list);
static unsigned int ngroups;
static struct element_group *current_group;

static void __group_foreach_element(struct element_group *g,
				    struct list_head *list,
				    void (*cb)(struct element_group *,
				    	       struct element *, void *),
				    void *arg)
{
	struct element *e, *n;

	list_for_each_entry_safe(e, n, list, e_list) {
		cb(g, e, arg);

		if (!list_empty(&e->e_childs))
			__group_foreach_element(g, &e->e_childs, cb, arg);
	}
}

void group_foreach_element(struct element_group *g,
			   void (*cb)(struct element_group *,
				      struct element *, void *),
			   void *arg)
{
	__group_foreach_element(g, &g->g_elements, cb, arg);
}

void group_foreach_recursive(void (*cb)(struct element_group *,
					struct element *, void *),
			     void *arg)
{
	struct element_group *g, *n;

	list_for_each_entry_safe(g, n, &group_list, g_list)
		__group_foreach_element(g, &g->g_elements, cb, arg);
}

void group_foreach(void (*cb)(struct element_group *, void *), void *arg)
{
	struct element_group *g, *n;

	list_for_each_entry_safe(g, n, &group_list, g_list)
		cb(g, arg);
}

struct element_group *group_select_first(void)
{
	if (list_empty(&group_list))
		current_group = NULL;
	else
		current_group = list_first_entry(&group_list,
						 struct element_group, g_list);

	return current_group;
}

struct element_group *group_select_last(void)
{
	if (list_empty(&group_list))
		current_group = NULL;
	else
		current_group = list_entry(group_list.prev,
					   struct element_group, g_list);

	return current_group;
}

struct element_group *group_select_next(void)
{
	if (!current_group)
		return group_select_first();

	if (current_group->g_list.next != &group_list)
		current_group = list_entry(current_group->g_list.next,
					   struct element_group,
					   g_list);
	else
		return group_select_first();

	return current_group;
}

struct element_group *group_select_prev(void)
{
	if (!current_group)
		return group_select_last();

	if (current_group->g_list.prev != &group_list)
		current_group = list_entry(current_group->g_list.prev,
					   struct element_group,
					   g_list);
	else
		return group_select_last();

	return current_group;
}

struct element_group *group_current(void)
{
	if (current_group == NULL)
		current_group = group_select_first();

	return current_group;
}

struct element_group *group_lookup(const char *name, int flags)
{
	struct element_group *g;
	struct group_hdr *hdr;

	list_for_each_entry(g, &group_list, g_list)
		if (!strcmp(name, g->g_name))
			return g;

	if (!(flags & GROUP_CREATE))
		return NULL;

	if (!(hdr = group_lookup_hdr(name))) {
		fprintf(stderr, "Cannot find title for group \"%s\"\n", name);
		return NULL;
	}

	g = xcalloc(1, sizeof(*g));

	init_list_head(&g->g_elements);

	g->g_name = hdr->gh_name;
	g->g_hdr = hdr;

	list_add_tail(&g->g_list, &group_list);
	ngroups++;

	return g;
}

static void group_free(struct element_group *g)
{
	struct element *e, *n;
	struct element_group *next;

	if (current_group == g) {
		next = group_select_next();
		if (!next || next == g)
			current_group = NULL;
	}

	list_for_each_entry_safe(e, n, &g->g_elements, e_list)
		element_free(e);

	xfree(g);
}

void reset_update_flags(void)
{
	group_foreach_recursive(&element_reset_update_flag, NULL);
}

void free_unused_elements(void)
{
	group_foreach_recursive(&element_check_if_dead, NULL);
}

struct group_hdr *group_lookup_hdr(const char *name)
{
	struct group_hdr *hdr;

	list_for_each_entry(hdr, &titles_list, gh_list)
		if (!strcmp(hdr->gh_name, name))
		    return hdr;

	return NULL;
}

int group_new_hdr(const char *name, const char *title,
		  const char *col1, const char *col2,
		  const char *col3, const char *col4)
{
	struct group_hdr *hdr;

	if (group_lookup_hdr(name))
		return -EEXIST;

	hdr = xcalloc(1, sizeof(*hdr));

	init_list_head(&hdr->gh_list);

	hdr->gh_name = strdup(name);
	hdr->gh_title = strdup(title);

	hdr->gh_column[0] = strdup(col1);
	hdr->gh_column[1] = strdup(col2);
	hdr->gh_column[2] = strdup(col3);
	hdr->gh_column[3] = strdup(col4);

	list_add_tail(&hdr->gh_list, &titles_list);

	DBG("New group title %s \"%s\"", name, title);

	return 0;
}

int group_new_derived_hdr(const char *name, const char *title,
			  const char *template)
{
	struct group_hdr *t;

	if (group_lookup_hdr(name))
		return -EEXIST;

	if (!(t = group_lookup_hdr(template)))
		return -ENOENT;

	return group_new_hdr(name, title, t->gh_column[0], t->gh_column[1],
			     t->gh_column[2], t->gh_column[3]);
}

static void  group_hdr_free(struct group_hdr *hdr)
{
	xfree(hdr->gh_name);
	xfree(hdr->gh_title);
	xfree(hdr->gh_column[0]);
	xfree(hdr->gh_column[1]);
	xfree(hdr->gh_column[2]);
	xfree(hdr->gh_column[3]);
	xfree(hdr);
}

static void __init group_init(void)
{
	DBG("init");

	group_new_hdr(DEFAULT_GROUP, "Interfaces",
		      "RX bps", "pps", "TX bps", "pps");
}

static void __exit group_exit(void)
{
	struct element_group *g, *next;
	struct group_hdr *hdr, *gnext;

	list_for_each_entry_safe(g, next, &group_list, g_list)
		group_free(g);

	list_for_each_entry_safe(hdr, gnext, &titles_list, gh_list)
		group_hdr_free(hdr);
}
