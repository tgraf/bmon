/*
 * output.c               Output API
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
#include <bmon/output.h>
#include <bmon/module.h>
#include <bmon/conf.h>
#include <bmon/group.h>
#include <bmon/utils.h>

static struct bmon_subsys output_subsys;

void output_register(struct bmon_module *m)
{
	module_register(&output_subsys, m);
}

static void activate_default(void)
{
	/*
	 * Try to activate a default output module if the user did not make
	 * a selection
	 */
	if (!output_subsys.s_nmod) {
		struct bmon_module *m;

		if (!output_set("curses"))
			return;

		if (!output_set("ascii"))
			return;

		/* Fall back to anything that could act as default */
		list_for_each_entry(m, &output_subsys.s_mod_list, m_list) {
			if (m->m_flags & BMON_MODULE_DEFAULT)
				if (!output_set(m->m_name))
					return;
		}

		quit("No output module found\n");
	}
}

void output_pre(void)
{
	module_foreach_run_enabled_pre(&output_subsys);
}
					
void output_draw(void)
{
	module_foreach_run_enabled(&output_subsys);
}

void output_post(void)
{
	module_foreach_run_enabled_post(&output_subsys);
}

int output_set(const char *name)
{
	return module_set(&output_subsys, name);
}

static struct bmon_subsys output_subsys = {
	.s_name			= "output",
	.s_activate_default	= &activate_default,
	.s_mod_list		= LIST_SELF(output_subsys.s_mod_list),
};

static void __init __output_init(void)
{
	return module_register_subsys(&output_subsys);
}
