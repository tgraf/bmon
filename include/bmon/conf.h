/*
 * conf.h                         Config Crap
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

#ifndef __BMON_CONF_H_
#define __BMON_CONF_H_

#include <bmon/bmon.h>

extern cfg_t *cfg;

extern float			cfg_read_interval;
extern float			cfg_rate_interval;
extern float			cfg_rate_variance;
extern float			cfg_history_variance;
extern int			cfg_show_all;
extern int			cfg_unit_exp;

extern void			conf_init_pre(void);
extern void			conf_init_post(void);
extern void			configfile_read(void);
extern void			set_configfile(const char *);

extern unsigned int		get_lifecycles(void);

extern void			conf_set_float(const char *, double);
extern double			conf_get_float(const char *);
extern void			conf_set_int(const char *, long);
extern long			conf_get_int(const char *);
extern void			conf_set_string(const char *, const char *);
extern const char *		conf_get_string(const char *);

typedef struct tv_s
{
	char *			tv_type;
	char *			tv_value;
	struct list_head	tv_list;
} tv_t;

typedef struct module_conf_s
{
	char *			m_name;
	struct list_head	m_attrs;
	struct list_head	m_list;
} module_conf_t;

extern int parse_module_param(const char *, struct list_head *);

enum {
	LAYOUT_UNSPEC,
	LAYOUT_DEFAULT,
	LAYOUT_STATUSBAR,
	LAYOUT_HEADER,
	LAYOUT_LIST,
	LAYOUT_SELECTED,
    LAYOUT_RX_GRAPH,
    LAYOUT_TX_GRAPH,
	__LAYOUT_MAX
};

#define LAYOUT_MAX (__LAYOUT_MAX - 1)

struct layout
{
	int	l_fg,
		l_bg,
		l_attr;
};

extern struct layout cfg_layout[];

#endif
