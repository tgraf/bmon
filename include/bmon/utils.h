/*
 * utils.h             General purpose utilities
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

#ifndef __BMON_UTILS_H_
#define __BMON_UTILS_H_

#include <bmon/bmon.h>

extern void * xcalloc(size_t, size_t);
extern void * xrealloc(void *, size_t);
extern void xfree(void *);
extern void quit (const char *, ...);

extern float timestamp_to_float(timestamp_t *);
extern int64_t timestamp_to_int(timestamp_t *);

extern void float_to_timestamp(timestamp_t *, float);
extern void int_to_timestamp(timestamp_t *, int);

extern void timestamp_add(timestamp_t *, timestamp_t *, timestamp_t *);
extern void timestamp_sub(timestamp_t *, timestamp_t *, timestamp_t *);

extern int timestamp_le(timestamp_t *, timestamp_t *);
extern int timestamp_is_negative(timestamp_t *ts);

extern void update_timestamp(timestamp_t *);
extern void copy_timestamp(timestamp_t *, timestamp_t *);

extern float timestamp_diff(timestamp_t *, timestamp_t *);

#if 0



#if __BYTE_ORDER == __BIG_ENDIAN
#define xntohll(N) (N)
#define xhtonll(N) (N)
#else
#define xntohll(N) ((((uint64_t) ntohl(N)) << 32) + ntohl((N) >> 32))
#define xhtonll(N) ((((uint64_t) htonl(N)) << 32) + htonl((N) >> 32))
#endif

enum {
	U_NUMBER,
	U_BYTES,
	U_BITS
};

extern float read_delta;


const char * xinet_ntop(struct sockaddr *, char *, socklen_t);


extern float time_diff(timestamp_t *, timestamp_t *);
extern float diff_now(timestamp_t *);

extern uint64_t parse_size(const char *);

extern int parse_date(const char *str, xdate_t *dst);

#ifndef HAVE_STRDUP
extern char *strdup(const char *);
#endif

extern char * timestamp2str(timestamp_t *ts, char *buf, size_t len);

static inline char *xstrncat(char *dest, const char *src, size_t n)
{
	return strncat(dest, src, n - strlen(dest) - 1);
}

static inline void xdate_to_ts(timestamp_t *dst, xdate_t *src)
{
	dst->tv_sec = mktime(&src->d_tm);
	dst->tv_usec = src->d_usec;
};
#endif

#endif
