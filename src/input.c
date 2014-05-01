/*
 * input.c            Input API
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
#include <bmon/module.h>
#include <bmon/utils.h>

static struct bmon_subsys input_subsys;

void input_register(struct bmon_module *m)
{
	module_register(&input_subsys, m);
}

static void activate_default(void)
{
	/*
	 * Try to activate a default input module if the user did not make
	 * a selection
	 */
	if (!input_subsys.s_nmod) {
		struct bmon_module *m;

#ifdef SYS_LINUX
		if (!input_set("netlink"))
			return;

		if (!input_set("proc"))
			return;
#endif

#ifdef SYS_BSD
		if (!input_set("sysctl"))
			return;
#endif

		/* Fall back to anything that could act as default */
		list_for_each_entry(m, &input_subsys.s_mod_list, m_list) {
			if (m->m_flags & BMON_MODULE_DEFAULT)
				if (!input_set(m->m_name))
					return;
		}

		quit("No input module found\n");
	}
}

void input_read(void)
{
	module_foreach_run_enabled(&input_subsys);
}

int input_set(const char *name)
{
	return module_set(&input_subsys, name);
}

static struct bmon_subsys input_subsys = {
	.s_name			= "input",
	.s_activate_default	= &activate_default,
	.s_mod_list		= LIST_SELF(input_subsys.s_mod_list),
};

static void __init __input_init(void)
{
	module_register_subsys(&input_subsys);
}
