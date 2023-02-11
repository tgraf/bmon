/*
 * attr.c		Attributes
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
#include <bmon/history.h>
#include <bmon/element.h>
#include <bmon/unit.h>
#include <bmon/input.h>
#include <bmon/utils.h>

#if 0

#define MAX_POLICY             255

static char * allowed_attrs[MAX_POLICY];
static char * denied_attrs[MAX_POLICY];
static int allow_all_attrs;

#endif

static LIST_HEAD(attr_def_list);
static int attr_id_gen = 1;

struct attr_def *attr_def_lookup(const char *name)
{
	struct attr_def *def;

	list_for_each_entry(def, &attr_def_list, ad_list)
		if (!strcmp(name, def->ad_name))
			return def;

	return NULL;
}

struct attr_def *attr_def_lookup_id(int id)
{
	struct attr_def *def;

	list_for_each_entry(def, &attr_def_list, ad_list)
		if (def->ad_id == id)
			return def;

	return NULL;
}

#if 0
int foreach_attr_type(int (*cb)(struct attr_type *, void *), void *arg)
{
	int i, err = 0;

	for (i = 0; i < NAME_HASHSIZ; i++) {
		struct attr_type *t;
		for (t = attr_ht_name[i]; t; t = t->at_next_name) {
			err = cb(t, arg);
			if (err < 0)
				break;
		}
	}

	return err;
}
#endif

int attr_def_add(const char *name, const char *desc, struct unit *unit,
		 int type, int flags)
{
	struct attr_def *def;

	if ((def = attr_def_lookup(name)))
		return def->ad_id;

	def = xcalloc(1, sizeof(*def));

	def->ad_id = attr_id_gen++;
	def->ad_name = strdup(name);

	def->ad_description = strdup(desc ? : "");
	def->ad_type = type;
	def->ad_unit = unit;
	def->ad_flags = flags;

	list_add_tail(&def->ad_list, &attr_def_list);

	DBG("New attribute %s desc=\"%s\" unit=%s type=%d",
	    def->ad_name, def->ad_description, def->ad_unit->u_name, type);

	return def->ad_id;
}

static void attr_def_free(struct attr_def *def)
{
	if (!def)
		return;

	xfree(def->ad_name);
	xfree(def->ad_description);
	xfree(def);
}

int attr_map_load(struct attr_map *map, size_t size)
{
	int i, nfailed = 0;

	for (i = 0; i < size; i++) {
		struct attr_map *m = &map[i];
		struct unit *u;

		if (!(u = unit_lookup(m->unit))) {
			nfailed++;
			continue;
		}

		m->attrid = attr_def_add(m->name, m->description, u,
					 m->type, m->flags);
	}

	return nfailed;
}

static inline unsigned int attr_hash(int id)
{
	return id % ATTR_HASH_SIZE;
}

struct attr *attr_lookup(const struct element *e, int id)
{
	unsigned int hash = attr_hash(id);
	struct attr *attr;

	list_for_each_entry(attr, &e->e_attrhash[hash], a_list)
		if (attr->a_def->ad_id == id)
			return attr;

	return NULL;
}

static int collect_history(struct element *e, struct attr_def *def)
{
	int n;

	if (def->ad_flags & ATTR_FORCE_HISTORY)
		return 1;

	for (n = 0; n <= GT_MAX; n++)
		if (e->e_key_attr[n] == def)
			return 1;

	return 0;
#if 0
	if (!allowed_attrs[0] && !denied_attrs[0]) {
		if (!strcmp(ad->ad_name, "bytes") ||
		    !strcmp(ad->ad_name, "packets"))
			return 1;
		else
			return 0;
	}

	if (!allowed_attrs[0]) {
		for (n = 0; n < MAX_POLICY && denied_attrs[n]; n++)
			if (!strcasecmp(denied_attrs[n], ad->ad_name))
				return 0;
		return 1;
	}

	for (n = 0; n < MAX_POLICY && denied_attrs[n]; n++)
		if (!strcasecmp(denied_attrs[n], ad->ad_name))
			return 0;
	
	for (n=0; n < MAX_POLICY && allowed_attrs[n]; n++)
		if (!strcasecmp(allowed_attrs[n], ad->ad_name))
			return 1;
#endif
}

#if 0

void attr_parse_policy(const char *policy)
{
	static int set = 0;
	int i, a = 0, d = 0, f = 0;
	char *p, *s;

	if (set)
		return;
	set = 1;

	if (!strcasecmp(policy, "all")) {
		allow_all_attrs = 1;
		return ;
	}

	s = strdup(policy);
	
	for (i = 0, p = s; ; i++) {
		if (s[i] == ',' || s[i] == '\0') {

			f = s[i] == '\0' ? 1 : 0;
			s[i] = '\0';
			
			if ('!' == *p) {
				if (d > (MAX_POLICY - 1))
					break;
				denied_attrs[d++] = strdup(++p);
			} else {
				if(a > (MAX_POLICY - 1))
					break;
				allowed_attrs[a++] = strdup(p);
			}
			
			if (f)
				break;
			
			p = &s[i+1];
		}
	}
	
	xfree(s);
}

#endif

void attr_start_collecting_history(struct attr *attr)
{
	if (attr->a_flags & ATTR_DOING_HISTORY)
		return;

	DBG("Starting to collect history for attribute %s",
	    attr->a_def->ad_name);

	history_attach(attr);
	attr->a_flags |= ATTR_DOING_HISTORY;
}

static int attrcmp(struct element *e, struct attr *a, struct attr *b)
{
	/* major key attribute is always first */
	if (e->e_key_attr[GT_MAJOR] == b->a_def)
		return 1;

	/* minor key attribte is always second */
	if (e->e_key_attr[GT_MINOR] == b->a_def)
		return (e->e_key_attr[GT_MAJOR] == a->a_def) ? -1 : 1;

	/* otherwise sort by alphabet */
	return strcasecmp(a->a_def->ad_description, b->a_def->ad_description);
}

void attr_update(struct element *e, int id, uint64_t rx, uint64_t tx, int flags)
{
	struct attr *attr, *n;
	int update_ts = 0;

	if (!(attr = attr_lookup(e, id))) {
		unsigned int hash = attr_hash(id);
		struct attr_def *def;

		if (!(def = attr_def_lookup_id(id)))
			return;

		DBG("Tracking new attribute %d (\"%s\") of element %s",
		    def->ad_id, def->ad_name, e->e_name);

		attr = xcalloc(1, sizeof(*attr));
		attr->a_def = def;
		attr->a_flags = def->ad_flags;

		init_list_head(&attr->a_history_list);

		if (collect_history(e, def))
			attr_start_collecting_history(attr);

		list_add_tail(&attr->a_list, &e->e_attrhash[hash]);
		e->e_nattrs++;

		list_for_each_entry(n, &e->e_attr_sorted, a_sort_list) {
			if (attrcmp(e, attr, n) < 0) {
				list_add_tail(&attr->a_sort_list,
					      &n->a_sort_list);
				goto inserted;
			}
		}

		list_add_tail(&attr->a_sort_list, &e->e_attr_sorted);
	}

inserted:
	if (flags & UPDATE_FLAG_RX) {
		attr->a_rx_rate.r_current = rx;
		attr->a_flags |= ATTR_RX_ENABLED;
		update_ts = 1;
	}

	if (flags & UPDATE_FLAG_TX) {
		attr->a_tx_rate.r_current = tx;
		attr->a_flags |= ATTR_TX_ENABLED;
		update_ts = 1;
	}

	if (update_ts)
		update_timestamp(&attr->a_last_update);

	DBG("Updated attribute %d (\"%s\") of element %s", id, attr->a_def->ad_name, e->e_name);
}

void attr_free(struct attr *a)
{
	struct history *h, *n;

	list_for_each_entry_safe(h, n, &a->a_history_list, h_list)
		history_free(h);

	list_del(&a->a_list);

	xfree(a);
}

void attr_rate2float(struct attr *a, double *rx, char **rxu, int *rxprec,
				     double *tx, char **txu, int *txprec)
{
	struct unit *u = a->a_def->ad_unit;

	*rx = unit_value2str(a->a_rx_rate.r_rate, u, rxu, rxprec);
	*tx = unit_value2str(a->a_tx_rate.r_rate, u, txu, txprec);
}

struct attr *attr_select_first(void)
{
	struct element *e;

	if (!(e = element_current()))
		return NULL;

	if (list_empty(&e->e_attr_sorted))
		e->e_current_attr = NULL;
	else
		e->e_current_attr = list_first_entry(&e->e_attr_sorted,
					struct attr, a_sort_list);

	return e->e_current_attr;
}

struct attr *attr_select_last(void)
{
	struct element *e;

	if (!(e = element_current()))
		return NULL;

	if (list_empty(&e->e_attr_sorted))
		e->e_current_attr = NULL;
	else
		e->e_current_attr = list_entry(&e->e_attr_sorted,
					   struct attr, a_sort_list);

	return e->e_current_attr;
}

struct attr *attr_select_next(void)
{
	struct element *e;
	struct attr *a;

	if (!(e = element_current()))
		return NULL;

	if (!(a = e->e_current_attr))
		return attr_select_first();

	if (a->a_sort_list.next != &e->e_attr_sorted)
		e->e_current_attr = list_entry(a->a_sort_list.next,
					   struct attr, a_sort_list);
	else
		return attr_select_first();

	return e->e_current_attr;
}

struct attr *attr_select_prev(void)
{
	struct element *e;
	struct attr *a;

	if (!(e = element_current()))
		return NULL;

	if (!(a = e->e_current_attr))
		return attr_select_last();

	if (a->a_sort_list.prev != &e->e_attr_sorted)
		e->e_current_attr = list_entry(a->a_sort_list.prev,
					   struct attr, a_sort_list);
	else
		return attr_select_last();

	return e->e_current_attr;

}

struct attr *attr_current(void)
{
	struct element *e;

	if (!(e = element_current()))
		return NULL;

	if (!e->e_current_attr)
		return attr_select_first();

	return e->e_current_attr;
}

#if 0
int __first_attr(struct item *item, int graph)
{
	int i;
	struct attr *a;

	for (i = 0; i < ATTR_HASH_MAX; i++) {
		for (a = item->i_attrs[i]; a; a = a->a_next) {
			if (a->a_flags & ATTR_FLAG_HISTORY) {
				item->i_attr_sel[graph] = a;
				return 0;
			}
		}
	}

	return EMPTY_LIST;
}

int __next_attr(struct item *item, int graph)
{
	int hash;
	struct attr *attr, *next;

	if (item->i_attr_sel[graph] == NULL)
		return __first_attr(item, graph);

	attr = item->i_attr_sel[graph];
	hash = attr_hash(attr->a_def->ad_id);
	next = attr->a_next;

	if (next == NULL)
		hash++;

	for (; hash < ATTR_HASH_MAX; hash++) {
		if (next) {
			attr = next;
			next = NULL;
		} else
			attr = item->i_attrs[hash];

		for (; attr; attr = attr->a_next) {
			if (!(attr->a_flags & ATTR_FLAG_HISTORY))
				continue;
			item->i_attr_sel[graph] = attr;
			return 0;
		}
	}

	return __first_attr(item, graph);
}

struct attr *attr_current(struct item *item, int graph)
{
	if (item->i_attr_sel[graph] == NULL)
		__first_attr(item, graph);

	return item->i_attr_sel[graph];
}

int attr_first(void)
{
	struct item *item = item_current();
	
	if (item == NULL)
		return EMPTY_LIST;

	return __first_attr(item, item->i_graph_sel);
}

int attr_next(void)
{
	struct item *item = item_current();
	
	if (item == NULL)
		return EMPTY_LIST;

	return __next_attr(item, item->i_graph_sel);
}
#endif

static float __calc_usage(double rate, uint64_t max)
{
	if (!max)
		return FLT_MAX;

	if (!rate)
		return 0.0f;

	return 100.0f / ((double) max / (rate * cfg_rate_interval));
}

uint64_t rate_get_total(struct rate *r)
{
	return r->r_total - r->r_reset;
}

void attr_calc_usage(struct attr *a, float *rx, float *tx,
		     uint64_t rxmax, uint64_t txmax)
{
	if (a->a_def->ad_type == ATTR_TYPE_PERCENT) {
		*rx = a->a_rx_rate.r_total;
		*tx = a->a_tx_rate.r_total;
	} else {
		*rx = __calc_usage(a->a_rx_rate.r_rate, rxmax);
		*tx = __calc_usage(a->a_tx_rate.r_rate, txmax);
	}
}

static void calc_counter_rate(struct attr *a, struct rate *rate,
			      timestamp_t *ts)
{
	uint64_t delta, prev_total;
	float diff, old_rate;

	if (rate->r_current < rate->r_prev) {
		/* Overflow detected */
		if (a->a_flags & ATTR_IGNORE_OVERFLOWS)
			delta = rate->r_current;
		else {
			if (a->a_flags & ATTR_TRUE_64BIT)
				delta = 0xFFFFFFFFFFFFFFFFULL - rate->r_prev;
			else
				delta = 0xFFFFFFFFULL - rate->r_prev;

			delta += rate->r_current + 1;
		}
	} else
		delta = rate->r_current - rate->r_prev;

	prev_total = rate->r_total;
	rate->r_total += delta;
	rate->r_prev = rate->r_current;

	if (!prev_total) {
		/*
		 * No previous records exists, reset time to now to
		 * avoid doing unnecessary calculation, this behaviour
		 * continues as long as the counter stays 0.
		 */
		goto out;
	}
	
	diff = timestamp_diff(&rate->r_last_calc, ts);
	if (diff < (cfg_rate_interval - cfg_rate_variance))
		return;

	old_rate = rate->r_rate;

	if (rate->r_total < prev_total) {
		/* overflow */
		delta = 0xFFFFFFFFFFFFFFFFULL - prev_total;
		delta += rate->r_total + 1;
	} else
		delta = rate->r_total - prev_total;

	rate->r_rate = delta / diff;

	if (old_rate)
		rate->r_rate = ((rate->r_rate * 3.0f) + old_rate) / 4.0f;

out:
	copy_timestamp(&rate->r_last_calc, ts);
}

static void calc_rate_total(struct attr *a, struct rate *rate, timestamp_t *ts)
{
	rate->r_prev = rate->r_rate = rate->r_total = rate->r_current;
	copy_timestamp(&rate->r_last_calc, ts);
}

void attr_notify_update(struct attr *a, timestamp_t *ts)
{
	switch (a->a_def->ad_type) {
	case ATTR_TYPE_RATE:
	case ATTR_TYPE_PERCENT:
		calc_rate_total(a, &a->a_rx_rate, ts);
		calc_rate_total(a, &a->a_tx_rate, ts);
		break;

	case ATTR_TYPE_COUNTER:
		calc_counter_rate(a, &a->a_rx_rate, ts);
		calc_counter_rate(a, &a->a_tx_rate, ts);
		break;
	default:
		DBG("Attribute update of unknown type");
		break;
	}

	if (a->a_flags & ATTR_DOING_HISTORY) {
		struct history *h;

		DBG("Updating history of attribute %d (\"%s\")", a->a_def->ad_id, a->a_def->ad_name);

		list_for_each_entry(h, &a->a_history_list, h_list)
			history_update(a, h, ts);
	}
}

void attr_reset_counter(struct attr *a)
{
	if (a->a_def->ad_type == ATTR_TYPE_COUNTER) {
		a->a_rx_rate.r_reset = a->a_rx_rate.r_total;
		a->a_tx_rate.r_reset = a->a_tx_rate.r_total;
	}
}

static void __exit attr_exit(void)
{
	struct attr_def *ad, *n;

	list_for_each_entry_safe(ad, n, &attr_def_list, ad_list)
		attr_def_free(ad);
}
