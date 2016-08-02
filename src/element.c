/*
 * src/element.c		Elements
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
#include <bmon/element.h>
#include <bmon/element_cfg.h>
#include <bmon/group.h>
#include <bmon/input.h>
#include <bmon/utils.h>

static LIST_HEAD(allowed);
static LIST_HEAD(denied);

static int match_mask(const struct policy *p, const char *str)
{
	int i, n;
	char c;

	if (!p || !str)
		return 0;
	
	for (i = 0, n = 0; p->p_rule[i] != '\0'; i++) {
		if (p->p_rule[i] == '*') {
			c = tolower(p->p_rule[i + 1]);
			
			if (c == '\0')
				return 1;
			
			while (tolower(str[n]) != c)
				if (str[n++] == '\0')
					return 0;
		} else if (tolower(p->p_rule[i]) != tolower(str[n++]))
			return 0;
	}

	return str[n] == '\0' ? 1 : 0;
}

int element_allowed(const char *name, struct element_cfg *cfg)
{
	struct policy *p;

	if (cfg) {
		if (cfg->ec_flags & ELEMENT_CFG_HIDE)
			return 0;
		else if (cfg->ec_flags & ELEMENT_CFG_SHOW)
			return 1;
	}

	list_for_each_entry(p, &denied, p_list)
		if (match_mask(p, name))
			return 0;

	if (!list_empty(&allowed)) {
		list_for_each_entry(p, &allowed, p_list)
			if (match_mask(p, name))
				return 1;

		return 0;
	}

	return 1;
}

void element_parse_policy(const char *policy)
{
	char *start, *copy, *save = NULL, *tok;
	struct policy *p;

	if (!policy)
		return;

	copy = strdup(policy);
	start = copy;

	while ((tok = strtok_r(start, ",", &save)) != NULL) {
		start = NULL;

		p = xcalloc(1, sizeof(*p));

		if (*tok == '!') {
			p->p_rule = strdup(++tok);
			list_add_tail(&p->p_list, &denied);
		} else {
			p->p_rule = strdup(tok);
			list_add_tail(&p->p_list, &allowed);
		}
	}
	
	xfree(copy);
}

static struct element *__lookup_element(struct element_group *group,
					const char *name, uint32_t id,
					struct element *parent)
{
	struct list_head *list;
	struct element *e;

	if (parent)
		list = &parent->e_childs;
	else
		list = &group->g_elements;

	list_for_each_entry(e, list, e_list)
		if (!strcmp(name, e->e_name) && e->e_id == id)
			return e;

	return NULL;
}

struct element *element_lookup(struct element_group *group, const char *name,
			       uint32_t id, struct element *parent, int flags)
{
	struct element_cfg *cfg;
	struct element *e;
	int i;

	if (!group)
		BUG();

	if ((e = __lookup_element(group, name, id, parent)))
		return e;

	if (!(flags & ELEMENT_CREAT))
		return NULL;

	cfg = element_cfg_lookup(name);
	if (!element_allowed(name, cfg))
		return NULL;

	DBG("Creating element %d \"%s\"", id, name);

	e = xcalloc(1, sizeof(*e));

	init_list_head(&e->e_list);
	init_list_head(&e->e_childs);
	init_list_head(&e->e_info_list);
	init_list_head(&e->e_attr_sorted);

	for (i = 0; i < ATTR_HASH_SIZE; i++)
		init_list_head(&e->e_attrhash[i]);

	e->e_name = strdup(name);
	e->e_id = id;
	e->e_parent = parent;
	e->e_group = group;
	e->e_lifecycles = get_lifecycles();
	e->e_flags = ELEMENT_FLAG_CREATED;
	e->e_cfg = cfg;

	if (e->e_cfg) {
		if (e->e_cfg->ec_description)
			element_update_info(e, "Description",
					    e->e_cfg->ec_description);
	
		element_set_rxmax(e, e->e_cfg->ec_rxmax);
		element_set_txmax(e, e->e_cfg->ec_txmax);
	}

	if (parent) {
		DBG("Attached to parent %d \"%s\"", parent->e_id, parent->e_name);
		list_add_tail(&e->e_list, &parent->e_childs);
	} else {
		DBG("Attached to group %s", group->g_name);
		list_add_tail(&e->e_list, &group->g_elements);
	}

	group->g_nelements++;

	return e;
}

void element_free(struct element *e)
{
	struct info *info, *ninfo;
	struct element *c, *cnext;
	struct attr *a, *an;
	int i;

	list_for_each_entry_safe(c, cnext, &e->e_childs, e_list)
		element_free(c);

	list_for_each_entry_safe(info, ninfo, &e->e_info_list, i_list) {
		xfree(info->i_name);
		xfree(info->i_value);
		list_del(&info->i_list);
		xfree(info);
	}

	for (i = 0; i < ATTR_HASH_SIZE; i++)
		list_for_each_entry_safe(a, an, &e->e_attrhash[i], a_list)
			attr_free(a);

	if (e->e_group->g_current == e) {
		element_select_prev();
		if (e->e_group->g_current == e)
			e->e_group->g_current = NULL;
	}

	list_del(&e->e_list);
	e->e_group->g_nelements--;

	xfree(e->e_name);
	xfree(e);
}

#if 0
	if (item->i_group->g_selected == item) {
		if (item_select_prev() == END_OF_LIST) {
			if (group_select_prev() == END_OF_LIST) {
				if (node_select_prev() != END_OF_LIST)
					item_select_last();
			} else
				item_select_last();
		}
	}
#endif

#if 0

void item_delete(struct item *item)
{
	int m;
	struct item *child;

	for (m = 0; m < ATTR_HASH_MAX; m++) {
		struct attr *a, *next;
		for (a = item->i_attrs[m]; a; a = next) {
			next = a->a_next;
			xfree(a);
		}
	}

	if (item->i_group->g_selected == item) {
		if (item_select_prev() == END_OF_LIST) {
			if (group_select_prev() == END_OF_LIST) {
				if (node_select_prev() != END_OF_LIST)
					item_select_last();
			} else
				item_select_last();
		}
	}

	unlink_item(item);
	item->i_group->g_nitems--;

	for (child = item->i_childs; child; child = child->i_next)
		item_delete(child);

	if (item->i_path)
		xfree(item->i_path);
	
	xfree(item);
}

#endif

void element_reset_update_flag(struct element_group *g,
			       struct element *e, void *arg)
{
	DBG("Reseting update flag of %s", e->e_name);
	e->e_flags &= ~ELEMENT_FLAG_UPDATED;
}

/**
 * Needs to be called after updating all attributes of an element
 */
void element_notify_update(struct element *e, timestamp_t *ts)
{
	struct attr *a;
	int i;

	e->e_flags |= ELEMENT_FLAG_UPDATED;

	if (ts == NULL)
		ts = &rtiming.rt_last_read;

	for (i = 0; i < ATTR_HASH_SIZE; i++)
		list_for_each_entry(a, &e->e_attrhash[i], a_list)
			attr_notify_update(a, ts);

	if (e->e_usage_attr && e->e_cfg &&
	    (a = attr_lookup(e, e->e_usage_attr->ad_id))) {
		attr_calc_usage(a, &e->e_rx_usage, &e->e_tx_usage,
				e->e_cfg->ec_rxmax, e->e_cfg->ec_txmax);
	} else {
		e->e_rx_usage = FLT_MAX;
		e->e_tx_usage = FLT_MAX;
	}
}

void element_lifesign(struct element *e, int n)
{
	e->e_lifecycles = n * get_lifecycles();
}

void element_check_if_dead(struct element_group *g,
			   struct element *e, void *arg)
{
	if (--(e->e_lifecycles) <= 0) {
		element_free(e);
		DBG("Deleting dead element %s", e->e_name);
	}
}

void element_foreach_attr(struct element *e,
			  void (*cb)(struct element *e,
			  	     struct attr *, void *),
			  void *arg)
{
	struct attr *a;

	list_for_each_entry(a, &e->e_attr_sorted, a_sort_list)
		cb(e, a, arg);
}

int element_set_key_attr(struct element *e, const char *major,
			  const char * minor)
{
	if (!(e->e_key_attr[GT_MAJOR] = attr_def_lookup(major)))
		return -ENOENT;

	if (!(e->e_key_attr[GT_MINOR] = attr_def_lookup(minor)))
		return -ENOENT;

	return 0;
}

int element_set_usage_attr(struct element *e, const char *usage)
{
	if (!(e->e_usage_attr = attr_def_lookup(usage)))
		return -ENOENT;

	return 0;
}

void element_pick_from_policy(struct element_group *g)
{
	if (!list_empty(&allowed)) {
		struct policy *p;

		list_for_each_entry(p, &allowed, p_list) {
			struct element *e;

			list_for_each_entry(e, &g->g_elements, e_list) {
				if (match_mask(p, e->e_name)) {
					g->g_current = e;
					return;
				}
			}
		}
	}

	element_select_first();
}

struct element *element_current(void)
{
	struct element_group *g;

	if (!(g = group_current()))
		return NULL;

	/*
	 * If no element is picked yet, pick a default interface according to
	 * the selection policy.
	 */
	if (!g->g_current)
		element_pick_from_policy(g);

	return g->g_current;
}

struct element *element_select_first(void)
{
	struct element_group *g;

	if (!(g = group_current()))
		return NULL;

	if (list_empty(&g->g_elements))
		g->g_current = NULL;
	else
		g->g_current = list_first_entry(&g->g_elements,
						struct element, e_list);

	return g->g_current;
}

struct element *element_select_last(void)
{
	struct element_group *g;

	if (!(g = group_current()))
		return NULL;

	if (list_empty(&g->g_elements))
		g->g_current = NULL;
	else {
		struct element *e;

		e = list_entry(g->g_elements.prev, struct element, e_list);

		while (!list_empty(&e->e_childs))
			e = list_entry(e->e_childs.prev, struct element,
				       e_list);

		g->g_current = e;
	}

	return g->g_current;
}

struct element *element_select_next(void)
{
	struct element_group *g;
	struct element *e;

	if (!(g = group_current()))
		return NULL;

	if (!(e = g->g_current))
		return element_select_first();

	if (!list_empty(&e->e_childs))
		e = list_first_entry(&e->e_childs, struct element, e_list);
	else {
		/*
		 * move upwards until we have no parent or there is a next
		 * entry in the list
		 */
		while (e->e_parent && e->e_list.next == &e->e_parent->e_childs)
			e = e->e_parent;

		if (!e->e_parent && e->e_list.next == &g->g_elements) {
			group_select_next();
			return element_select_first();
		} else
			e = list_entry(e->e_list.next, struct element, e_list);
	}

	g->g_current = e;

	return e;
}

struct element *element_select_prev(void)
{
	struct element_group *g;
	struct element *e;

	if (!(g = group_current()))
		return NULL;

	if (!(e = g->g_current))
		return element_select_last();

	if (!e->e_parent && e->e_list.prev == &g->g_elements) {
		group_select_prev();
		return element_select_last();
	}

	if (e->e_parent && e->e_list.prev == &e->e_parent->e_childs)
		e = e->e_parent;
	else {
		e = list_entry(e->e_list.prev, struct element, e_list);

		while (!list_empty(&e->e_childs))
			e = list_entry(e->e_childs.prev, struct element,
				       e_list);
	}

	g->g_current = e;

	return e;
}

static struct info *element_info_lookup(struct element *e, const char *name)
{
	struct info *i;

	list_for_each_entry(i, &e->e_info_list, i_list)
		if (!strcmp(i->i_name, name))
			return i;

	return NULL;
}

void element_update_info(struct element *e, const char *name, const char *value)
{
	struct info *i;

	if ((i = element_info_lookup(e, name))) {
		xfree(i->i_value);
		i->i_value = strdup(value);
		return;
	}

	DBG("Created element info %s (\"%s\")", name, value);

	i = xcalloc(1, sizeof(*i));
	i->i_name = strdup(name);
	i->i_value = strdup(value);

	e->e_ninfo++;

	list_add_tail(&i->i_list, &e->e_info_list);
}

void element_set_txmax(struct element *e, uint64_t max)
{
	char buf[32];

	if (!e->e_cfg)
		e->e_cfg = element_cfg_create(e->e_name);

	if (e->e_cfg->ec_txmax != max)
		e->e_cfg->ec_txmax = max;

	unit_bit2str(e->e_cfg->ec_txmax * 8, buf, sizeof(buf));
	element_update_info(e, "TxMax", buf);
}

void element_set_rxmax(struct element *e, uint64_t max)
{
	char buf[32];

	if (!e->e_cfg)
		e->e_cfg = element_cfg_create(e->e_name);

	if (e->e_cfg->ec_rxmax != max)
		e->e_cfg->ec_rxmax = max;

	unit_bit2str(e->e_cfg->ec_rxmax * 8, buf, sizeof(buf));
	element_update_info(e, "RxMax", buf);
}
