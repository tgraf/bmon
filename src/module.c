/*
 * module.c               Module API
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
#include <bmon/module.h>
#include <bmon/utils.h>

static LIST_HEAD(subsys_list);

static void module_foreach(struct bmon_subsys *ss, void (*cb)(struct bmon_module *))
{
	struct bmon_module *m;

	list_for_each_entry(m, &ss->s_mod_list, m_list)
		cb(m);
}

void module_foreach_run_enabled_pre(struct bmon_subsys *ss)
{
	struct bmon_module *m;

	list_for_each_entry(m, &ss->s_mod_list, m_list)
		if (m->m_flags & BMON_MODULE_ENABLED && m->m_pre)
			m->m_pre();
}

void module_foreach_run_enabled(struct bmon_subsys *ss)
{
	struct bmon_module *m;

	list_for_each_entry(m, &ss->s_mod_list, m_list)
		if (m->m_flags & BMON_MODULE_ENABLED && m->m_do)
			m->m_do();
}

void module_foreach_run_enabled_post(struct bmon_subsys *ss)
{
	struct bmon_module *m;

	list_for_each_entry(m, &ss->s_mod_list, m_list)
		if (m->m_flags & BMON_MODULE_ENABLED && m->m_post)
			m->m_post();
}

static struct bmon_module *module_lookup(struct bmon_subsys *ss, const char *name)
{
	struct bmon_module *m;

	list_for_each_entry(m, &ss->s_mod_list, m_list)
		if (!strcmp(m->m_name, name))
			return m;

	return NULL;
}

static void module_list(struct bmon_subsys *ss, struct list_head *list)
{
	printf("%s modules:\n", ss->s_name);
	if (list_empty(list))
		printf("\tNo %s modules found.\n", ss->s_name);
	else {
		struct bmon_module *m;

		list_for_each_entry(m, list, m_list)
			printf("\t%s\n", m->m_name);
	}
}

static int module_configure(struct bmon_module *m, module_conf_t *cfg)
{
	DBG("Configuring module %s", m->m_name);

	if (m->m_parse_opt && cfg) {
		tv_t *tv;

		list_for_each_entry(tv, &cfg->m_attrs, tv_list)
			m->m_parse_opt(tv->tv_type, tv->tv_value);
	}

	if (m->m_probe && !m->m_probe())
		return -EINVAL;

	m->m_flags |= BMON_MODULE_ENABLED;
	m->m_subsys->s_nmod++;

	return 0;
}

int module_register(struct bmon_subsys *ss, struct bmon_module *m)
{
	if (m->m_subsys)
		return -EBUSY;

	list_add_tail(&m->m_list, &ss->s_mod_list);
	m->m_subsys = ss;
	return 0;
}

static void __auto_load(struct bmon_module *m)
{
	if ((m->m_flags & BMON_MODULE_AUTO) &&
	    !(m->m_flags & BMON_MODULE_ENABLED)) {
		if (module_configure(m, NULL) == 0)
			DBG("Auto-enabled module %s", m->m_name);
	}
}

int module_set(struct bmon_subsys *ss, const char *name)
{
	struct bmon_module *mod;
	LIST_HEAD(tmp_list);
	module_conf_t *m;

	if (!name || !strcasecmp(name, "list")) {
		module_list(ss, &ss->s_mod_list);
		return 1;
	}
	
	parse_module_param(name, &tmp_list);

	list_for_each_entry(m, &tmp_list, m_list) {
		if (!(mod = module_lookup(ss, m->m_name)))
			quit("Unknown %s module: %s\n", ss->s_name, m->m_name);

		if (module_configure(mod, m) == 0)
			DBG("Enabled module %s", mod->m_name);
	}

	module_foreach(ss, __auto_load);

	if (!ss->s_nmod)
		quit("No working %s module found\n", ss->s_name);

	DBG("Configured modules");

	return 0;
}

static void __module_init(struct bmon_module *m)
{
	if (m->m_init) {
		DBG("Initializing %s...", m->m_name);
		if (m->m_init())
			m->m_flags &= ~BMON_MODULE_ENABLED;
	}
}

void module_init(void)
{
	struct bmon_subsys *ss;

	DBG("Initializing modules");

	list_for_each_entry(ss, &subsys_list, s_list) {
		if (ss->s_activate_default)
			ss->s_activate_default();

		module_foreach(ss, __module_init);
	}
}

static void __module_shutdown(struct bmon_module *m)
{
	if (m->m_shutdown) {
		DBG("Shutting down %s...", m->m_name);
		m->m_shutdown();
		m->m_flags &= ~BMON_MODULE_ENABLED;
	}
}

void module_shutdown(void)
{
	DBG("Shutting down modules");

	struct bmon_subsys *ss;

	list_for_each_entry(ss, &subsys_list, s_list)
		module_foreach(ss, __module_shutdown);
}

void module_register_subsys(struct bmon_subsys *ss)
{
	DBG("Module %s registered", ss->s_name);

	list_add_tail(&ss->s_list, &subsys_list);
}
