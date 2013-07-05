/*
 * element_cfg.c		Element Configuration
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
#include <bmon/utils.h>

static LIST_HEAD(cfg_list);

struct element_cfg *element_cfg_alloc(const char *name)
{
	struct element_cfg *ec;

	if ((ec = element_cfg_lookup(name))) {
		ec->ec_refcnt++;
		return ec;
	}

	ec = xcalloc(1, sizeof(*ec));
	ec->ec_name = strdup(name);
	ec->ec_refcnt = 1;

	list_add_tail(&ec->ec_list, &cfg_list);

	return ec;
}

struct element_cfg *element_cfg_create(const char *name)
{
	struct element_cfg *cfg;

	if (!(cfg = element_cfg_lookup(name)))
		cfg = element_cfg_alloc(name);

	return cfg;
}

static void __element_cfg_free(struct element_cfg *ec)
{
	list_del(&ec->ec_list);
	
	xfree(ec->ec_name);
	xfree(ec->ec_description);
	xfree(ec);
}

void element_cfg_free(struct element_cfg *ec)
{
	if (!ec || --ec->ec_refcnt)
		return;

	__element_cfg_free(ec);
}

struct element_cfg *element_cfg_lookup(const char *name)
{
	struct element_cfg *ec;

	list_for_each_entry(ec, &cfg_list, ec_list)
		if (!strcmp(name, ec->ec_name))
			return ec;
	
	return NULL;
}

static void __exit __element_cfg_exit(void)
{
	struct element_cfg *ec, *n;

	list_for_each_entry_safe(ec, n, &cfg_list, ec_list)
		__element_cfg_free(ec);
}
