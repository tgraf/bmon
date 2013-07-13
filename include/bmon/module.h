/*
 * module.h               Module API
 *
 * Copyright (c) 2001-2013 Thomas Graf <tgraf@suug.ch>
 * Copyright 2013 Red Hat, Inc.
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

#ifndef __BMON_MODULE_H_
#define __BMON_MODULE_H_

#include <bmon/bmon.h>
#include <bmon/conf.h>

#define BMON_MODULE_ENABLED		(1 << 0) /* Enabled */
#define BMON_MODULE_DEFAULT		(1 << 1) /* Suitable as default */
#define BMON_MODULE_AUTO		(1 << 2) /* Auto enable */

struct bmon_subsys;

struct bmon_module
{
	char *			m_name;

	int		      (*m_init)(void);
	int		      (*m_probe)(void);
	void		      (*m_shutdown)(void);

	void		      (*m_parse_opt)(const char *, const char *);

	void		      (*m_pre)(void);
	void		      (*m_do)(void);
	void		      (*m_post)(void);

	int			m_flags;
	struct list_head	m_list;
	struct bmon_subsys     *m_subsys;
};

struct bmon_subsys
{
	char *			s_name;
	int			s_nmod;
	struct list_head	s_mod_list;

	void		      (*s_activate_default)(void);

	struct list_head	s_list;
};

extern void		module_foreach_run_enabled_pre(struct bmon_subsys *);
extern void		module_foreach_run_enabled(struct bmon_subsys *);
extern void		module_foreach_run_enabled_post(struct bmon_subsys *);

extern int		module_register(struct bmon_subsys *, struct bmon_module *);
extern int		module_set(struct bmon_subsys *, const char *);

extern void		module_init(void);
extern void		module_shutdown(void);
extern void		module_register_subsys(struct bmon_subsys *);

#endif
