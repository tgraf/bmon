/*
 * element_cfg.h	Element Configuration
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

#ifndef __BMON_ELEMENT_CFG_H_
#define __BMON_ELEMENT_CFG_H_

#include <bmon/bmon.h>
#include <bmon/conf.h>

#define ELEMENT_CFG_SHOW	(1 << 0)
#define ELEMENT_CFG_HIDE	(1 << 1)

struct element_cfg
{
	char *			ec_name;	/* Name of element config */
	char *			ec_parent;	/* Name of parent */
	char *			ec_description;	/* Human readable description  */
	uint64_t		ec_rxmax;	/* Maximum RX value expected */
	uint64_t		ec_txmax;	/* Minimum TX value expected */
	unsigned int		ec_flags;	/* Flags */

	struct list_head	ec_list;	/* Internal, do not modify */
	int			ec_refcnt;	/* Internal, do not modify */
};

extern struct element_cfg *	element_cfg_alloc(const char *);
extern struct element_cfg *	element_cfg_create(const char *);
extern void			element_cfg_free(struct element_cfg *);
extern struct element_cfg *	element_cfg_lookup(const char *);

#endif
