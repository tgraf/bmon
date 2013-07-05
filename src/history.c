/*
 * history.c		History Management
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
#include <bmon/history.h>
#include <bmon/utils.h>

static LIST_HEAD(def_list);

static struct history_def *current_history;

struct history_def *history_def_lookup(const char *name)
{
	struct history_def *def;

	list_for_each_entry(def, &def_list, hd_list)
		if (!strcmp(def->hd_name, name))
		    return def;

	return NULL;
}

struct history_def *history_def_alloc(const char *name)
{
	struct history_def *def;

	if ((def = history_def_lookup(name)))
		return def;

	def = xcalloc(1, sizeof(*def));
	def->hd_name = strdup(name);

	list_add_tail(&def->hd_list, &def_list);

	DBG("New history definition %s", name);

	return def;
}

static void history_def_free(struct history_def *def)
{
	if (!def)
		return;

	xfree(def->hd_name);
	xfree(def);
}

static void *history_alloc_data(struct history_def *def)
{
	return xcalloc(def->hd_size, def->hd_type);
}

static void history_store_data(struct history *h, struct history_store *hs,
			       uint64_t total, float diff)
{
	uint64_t delta;

	if (!hs->hs_data) {
		if (total == HISTORY_UNKNOWN)
			return;

		hs->hs_data = history_alloc_data(h->h_definition);
	}

	if (total == HISTORY_UNKNOWN)
		delta = HISTORY_UNKNOWN;
	else {
		delta = (total - hs->hs_prev_total);

		if (delta > 0)
			delta /= diff;
		hs->hs_prev_total = total;
	}

	switch (h->h_definition->hd_type) {
	case HISTORY_TYPE_8:
		((uint8_t *) hs->hs_data)[h->h_index] = (uint8_t) delta;
		break;
	
	case HISTORY_TYPE_16:
		((uint16_t *) hs->hs_data)[h->h_index] = (uint16_t) delta;
		break;

	case HISTORY_TYPE_32:
		((uint32_t *) hs->hs_data)[h->h_index] = (uint32_t) delta;
		break;

	case HISTORY_TYPE_64:
		((uint64_t *) hs->hs_data)[h->h_index] = (uint64_t) delta;
		break;

	default:
		BUG();
	}
}

static inline void inc_history_index(struct history *h)
{
	if (h->h_index < (h->h_definition->hd_size - 1))
		h->h_index++;
	else
		h->h_index = 0;
}

uint64_t history_data(struct history *h, struct history_store *hs, int index)
{
	switch (h->h_definition->hd_type) {
	case HISTORY_TYPE_8: {
		uint8_t v = ((uint8_t *) hs->hs_data)[index];
		return (v == (uint8_t) -1) ? HISTORY_UNKNOWN : v;
	}
	
	case HISTORY_TYPE_16: {
		uint16_t v = ((uint16_t *) hs->hs_data)[index];
		return (v == (uint16_t) -1) ? HISTORY_UNKNOWN : v;
	}
 
	case HISTORY_TYPE_32: {
		uint32_t v = ((uint32_t *) hs->hs_data)[index];
		return (v == (uint32_t) -1) ? HISTORY_UNKNOWN : v;
	}

	case HISTORY_TYPE_64: {
		uint64_t v = ((uint64_t *) hs->hs_data)[index];
		return (v == (uint64_t) -1) ? HISTORY_UNKNOWN : v;
	}

	default:
		BUG();
	}
}

void history_update(struct attr *a, struct history *h, timestamp_t *ts)
{
	struct history_def *def = h->h_definition;
	float timediff;

	if (h->h_last_update.tv_sec)
		timediff = timestamp_diff(&h->h_last_update, ts);
	else {
		timediff = 0.0f; /* initial history update */

		/* Need a delta when working with counters */
		if (a->a_def->ad_type == ATTR_TYPE_COUNTER)
			goto update_prev_total;
	}

	/*
	 * A read interval greater than the desired history interval
	 * can't possibly result in anything useful. Discard it and
	 * mark history data as invalid. The user has to adjust the
	 * read interval.
	 */
	if (cfg_read_interval > def->hd_interval)
		goto discard;

	/*
	 * If the history interval matches the read interval it makes
	 * sense to update upon every read. The reader timing already
	 * took care of being as close as possible to the desired
	 * interval.
	 */
	if (cfg_read_interval == def->hd_interval)
		goto update;

	if (timediff > h->h_max_interval)
		goto discard;

	if (timediff < h->h_min_interval)
		return;

update:
	history_store_data(h, &h->h_rx, a->a_rx_rate.r_total, timediff);
	history_store_data(h, &h->h_tx, a->a_tx_rate.r_total, timediff);
	inc_history_index(h);

	goto update_ts;

discard:
	while(timediff >= (def->hd_interval / 2)) {
		history_store_data(h, &h->h_rx, HISTORY_UNKNOWN, 0.0f);
		history_store_data(h, &h->h_tx, HISTORY_UNKNOWN, 0.0f);

		inc_history_index(h);
		timediff -= def->hd_interval;
	}

update_prev_total:
	h->h_rx.hs_prev_total = a->a_rx_rate.r_total;
	h->h_tx.hs_prev_total = a->a_tx_rate.r_total;

update_ts:
	copy_timestamp(&h->h_last_update, ts);
}

struct history *history_alloc(struct history_def *def)
{
	struct history *h;

	h  = xcalloc(1, sizeof(*h));

	init_list_head(&h->h_list);

	h->h_definition = def;

	h->h_min_interval = (def->hd_interval - (cfg_read_interval / 2.0f));
	h->h_max_interval = (def->hd_interval / cfg_history_variance);

	return h;
}

void history_free(struct history *h)
{
	if (!h)
		return;

	xfree(h->h_rx.hs_data);
	xfree(h->h_tx.hs_data);

	list_del(&h->h_list);

	xfree(h);
}

void history_attach(struct attr *attr)
{
	struct history_def *def;
	struct history *h;

	list_for_each_entry(def, &def_list, hd_list) {
		h = history_alloc(def);
		list_add_tail(&h->h_list, &attr->a_history_list);
	}
}

struct history_def *history_select_first(void)
{
	if (list_empty(&def_list))
		current_history = NULL;
	else
		current_history = list_first_entry(&def_list,
					 struct history_def, hd_list);

	return current_history;
}

struct history_def *history_select_last(void)
{
	if (list_empty(&def_list))
		current_history = NULL;
	else
		current_history = list_entry(def_list.prev,
					struct history_def, hd_list);

	return current_history;
}

struct history_def *history_select_next(void)
{
	if (current_history && current_history->hd_list.next != &def_list) {
		current_history = list_entry(current_history->hd_list.next,
					struct history_def, hd_list);
		return current_history;
	}

	return history_select_first();
}

struct history_def *history_select_prev(void)
{
	if (current_history && current_history->hd_list.prev != &def_list) {
		current_history = list_entry(current_history->hd_list.prev,
					struct history_def, hd_list);
		return current_history;
	}

	return history_select_last();
}

struct history_def *history_current(void)
{
	if (!current_history)
		current_history = history_select_first();

	return current_history;
}

static void __exit history_exit(void)
{
	struct history_def *def, *n;

	list_for_each_entry_safe(def, n, &def_list, hd_list)
		history_def_free(def);
}
