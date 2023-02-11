/*
 * bmon.h              All-include Header
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

#ifndef __BMON_BMON_H_
#define __BMON_BMON_H_

#include <bmon/config.h>
#include <bmon/list.h>

extern int start_time;

typedef struct timestamp_s
{
	int64_t		tv_sec;
	int64_t		tv_usec;
} timestamp_t;

typedef struct xdate_s
{
	struct tm	d_tm;
	unsigned int	d_usec;
} xdate_t;

enum {
	EMPTY_LIST = 1,
	END_OF_LIST = 2
};

#define BUG()								\
	do {								\
		fprintf(stderr, "BUG: %s:%d\n", __FILE__, __LINE__);	\
                assert(0); \
		exit(EINVAL);						\
	} while (0);

#define ARRAY_SIZE(X) (sizeof(X) / sizeof((X)[0]))

#if defined __GNUC__
#define __init __attribute__ ((constructor))
#define __exit __attribute__ ((destructor))
#define __unused__ __attribute__ ((unused))
#else
#define __init
#define __exit
#define __unused__
#endif

#ifdef DEBUG
#define DBG(FMT, ARG...)					\
	do {							\
		fprintf(stderr,					\
			"[DBG] %20s:%-4u %s: " FMT "\n",	\
			__FILE__, __LINE__,			\
			__PRETTY_FUNCTION__, ##ARG);		\
	} while (0)
#else
#define DBG(FMT, ARG...)					\
	do {							\
	} while (0)
#endif

#endif
