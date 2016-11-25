/*
 * utils.c             General purpose utilities
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
#include <bmon/utils.h>

#ifdef __MACH__
#include <mach/clock.h>
#include <mach/mach.h>
#endif

void *xcalloc(size_t n, size_t s)
{
	void *d = calloc(n, s);

	if (NULL == d) {
		fprintf(stderr, "xalloc: Out of memory\n");
		exit(ENOMEM);
	}

	return d;
}

void *xrealloc(void *p, size_t s)
{
	void *d = realloc(p, s);

	if (NULL == d) {
		fprintf(stderr, "xrealloc: Out of memory!\n");
		exit(ENOMEM);
	}

	return d;
}

void xfree(void *d)
{
	if (d)
		free(d);
}

float timestamp_to_float(timestamp_t *src)
{
	return (float) src->tv_sec + ((float) src->tv_usec / 1000000.0f);
}

int64_t timestamp_to_int(timestamp_t *src)
{
	return (src->tv_sec * 1000000ULL) + src->tv_usec;
}

void float_to_timestamp(timestamp_t *dst, float src)
{
	dst->tv_sec = (time_t) src;
	dst->tv_usec = (src - ((float) ((time_t) src))) * 1000000.0f;
}

void timestamp_add(timestamp_t *dst, timestamp_t *src1, timestamp_t *src2)
{
	dst->tv_sec = src1->tv_sec + src2->tv_sec;
	dst->tv_usec = src1->tv_usec + src2->tv_usec;

	if (dst->tv_usec >= 1000000) {
		dst->tv_sec++;
		dst->tv_usec -= 1000000;
	}
}

void timestamp_sub(timestamp_t *dst, timestamp_t *src1, timestamp_t *src2)
{
	dst->tv_sec = src1->tv_sec - src2->tv_sec;
	dst->tv_usec = src1->tv_usec - src2->tv_usec;
	if (dst->tv_usec < 0) {
		dst->tv_sec--;
		dst->tv_usec += 1000000;
	}
}

int timestamp_le(timestamp_t *a, timestamp_t *b)
{
	if (a->tv_sec > b->tv_sec)
		return 0;

	if (a->tv_sec < b->tv_sec || a->tv_usec <= b->tv_usec)
		return 1;
	
	return 0;
}

int timestamp_is_negative(timestamp_t *ts)
{
	return (ts->tv_sec < 0 || ts->tv_usec < 0);
}

void update_timestamp(timestamp_t *dst)
{
#ifdef __MACH__
	clock_serv_t cclock;
	mach_timespec_t tp;

	host_get_clock_service(mach_host_self(), SYSTEM_CLOCK, &cclock);
	clock_get_time(cclock, &tp);
	mach_port_deallocate(mach_task_self(), cclock);
#else
	struct timespec tp;

	clock_gettime(CLOCK_MONOTONIC, &tp);
#endif

	dst->tv_sec = tp.tv_sec;
	dst->tv_usec = tp.tv_nsec / 1000;
}

void copy_timestamp(timestamp_t *ts1, timestamp_t *ts2)
{
	ts1->tv_sec = ts2->tv_sec;
	ts1->tv_usec = ts2->tv_usec;
}

float timestamp_diff(timestamp_t *t1, timestamp_t *t2)
{
	float diff;

	diff = t2->tv_sec - t1->tv_sec;
	diff += (((float) t2->tv_usec - (float) t1->tv_usec) / 1000000.0f);

	return diff;
}

#if 0


float diff_now(timestamp_t *t1)
{
	timestamp_t now;
	update_ts(&now);
	return time_diff(t1, &now);
}

const char *xinet_ntop(struct sockaddr *src, char *dst, socklen_t cnt)
{
	void *s;
	int family;

	if (src->sa_family == AF_INET6) {
		s = &((struct sockaddr_in6 *) src)->sin6_addr;
		family = AF_INET6;
	} else if (src->sa_family == AF_INET) {
		s = &((struct sockaddr_in *) src)->sin_addr;
		family = AF_INET;
	} else
		return NULL;

	return inet_ntop(family, s, dst, cnt);
}

uint64_t parse_size(const char *str)
{
	char *p;
	uint64_t l = strtol(str, &p, 0);
	if (p == str)
		return -1;

	if (*p) {
		if (!strcasecmp(p, "kb") || !strcasecmp(p, "k"))
			l *= 1024;
		else if (!strcasecmp(p, "gb") || !strcasecmp(p, "g"))
			l *= 1024*1024*1024;
		else if (!strcasecmp(p, "gbit"))
			l *= 1024*1024*1024/8;
		else if (!strcasecmp(p, "mb") || !strcasecmp(p, "m"))
			l *= 1024*1024;
		else if (!strcasecmp(p, "mbit"))
			l *= 1024*1024/8;
		else if (!strcasecmp(p, "kbit"))
			l *= 1024/8;
		else if (strcasecmp(p, "b") != 0)
			return -1;
	}

	return l;
}

#ifndef HAVE_STRDUP
char *strdup(const char *s)
{
	char *t = xcalloc(1, strlen(s) + 1);
	memcpy(t, s, strlen(s));
	return s;
}
#endif

char *timestamp2str(timestamp_t *ts, char *buf, size_t len)
{
	int i, split[5];
	char *units[] = {"d", "h", "m", "s", "usec"};
	time_t sec = ts->tv_sec;

#define _SPLIT(idx, unit) if ((split[idx] = sec / unit) > 0) sec %= unit
	_SPLIT(0, 86400);	/* days */
	_SPLIT(1, 3600);	/* hours */
	_SPLIT(2, 60);		/* minutes */
	_SPLIT(3, 1);		/* seconds */
#undef  _SPLIT
	split[4] = ts->tv_usec;

	memset(buf, 0, len);

	for (i = 0; i < ARRAY_SIZE(split); i++) {
		if (split[i] > 0) {
			char t[64];
			snprintf(t, sizeof(t), "%s%d%s",
				 strlen(buf) ? " " : "", split[i], units[i]);
			strncat(buf, t, len - strlen(buf) - 1);
		}
	}

	return buf;
}

int parse_date(const char *str, xdate_t *dst)
{
	time_t now = time(NULL);
	struct tm *now_tm = localtime(&now);
	const char *next;
	char *p;
	struct tm backup;

	memset(dst, 0, sizeof(*dst));

	if (strchr(str, '-')) {
		next = strptime(str, "%Y-%m-%d", &dst->d_tm);
		if (next == NULL ||
		    (*next != '\0' && *next != ' '))
			goto invalid_date;
	} else {
		dst->d_tm.tm_mday  = now_tm->tm_mday;
		dst->d_tm.tm_mon   = now_tm->tm_mon;
		dst->d_tm.tm_year  = now_tm->tm_year;
		dst->d_tm.tm_wday  = now_tm->tm_wday;
		dst->d_tm.tm_yday  = now_tm->tm_yday;
		dst->d_tm.tm_isdst = now_tm->tm_isdst;
		next = str;
	}

	if (*next == '\0')
		return 0;

	while (*next == ' ')
		next++;

	if (*next == '.')
		goto read_us;

	/* Make a backup, we can't rely on strptime to not screw
	 * up what we've got so far. */
	memset(&backup, 0, sizeof(backup));
	memcpy(&backup, &dst->d_tm, sizeof(backup));

	next = strptime(next, "%H:%M:%S", &dst->d_tm);
	if (next == NULL ||
	    (*next != '\0' && *next != '.'))
		goto invalid_date;

	dst->d_tm.tm_mday  = backup.tm_mday;
	dst->d_tm.tm_mon   = backup.tm_mon;
	dst->d_tm.tm_year  = backup.tm_year;
	dst->d_tm.tm_wday  = backup.tm_wday;
	dst->d_tm.tm_yday  = backup.tm_yday;
	dst->d_tm.tm_isdst = backup.tm_isdst;

	if (*next == '\0')
		return 0;
read_us:
	if (*next == '.')
		next++;
	else
		BUG();

	dst->d_usec = strtoul(next, &p, 10);

	if (*p != '\0')
		goto invalid_date;
	
	return 0;

invalid_date:
	fprintf(stderr, "Invalid date \"%s\"\n", str);
	return -1;
}

static inline void print_token(FILE *fd, struct db_token *tok)
{
	fprintf(fd, "%s", tok->t_name);

	if (tok->t_flags & DB_T_ATTR)
		fprintf(fd, "<attr>");
}

void db_print_filter(FILE *fd, struct db_filter *filter)
{
	if (filter->f_node)
		print_token(fd, filter->f_node);

	if (filter->f_group) {
		fprintf(fd, ".");
		print_token(fd, filter->f_group);
	}

	if (filter->f_item) {
		fprintf(fd, ".");
		print_token(fd, filter->f_item);
	}

	if (filter->f_attr) {
		fprintf(fd, ".");
		print_token(fd, filter->f_attr);
	}

	if (filter->f_field)
		fprintf(fd, "@%s", filter->f_field);
}

void *db_filter__scan_string(const char *);
void db_filter__switch_to_buffer(void *);
int db_filter_parse(void);

struct db_filter * parse_db_filter(const char *buf)
{
	struct db_filter *f;
	struct db_token *tok, *t;

	void *state = db_filter__scan_string(buf);
	db_filter__switch_to_buffer(state);

	if (db_filter_parse()) {
		fprintf(stderr, "Failed to parse filter \"%s\"\n", buf);
		return NULL;
	}

	tok = db_filter_out; /* generated by yacc */
	if (tok == NULL)
		return NULL;
	
	f = xcalloc(1, sizeof(*f));

	f->f_node = tok;

	if (!tok->t_next)
		goto out;
	tok = tok->t_next;

	if (tok->t_flags & DB_T_ATTR) {
		fprintf(stderr, "Node may not contain an attribute field\n");
		goto errout;
	}

	f->f_group = tok;
	if (!tok->t_next)
		goto out;
	tok = tok->t_next;

	if (tok->t_flags & DB_T_ATTR) {
		fprintf(stderr, "Group may not contain an attribute field\n");
		goto errout;
	}

	f->f_item = tok;

	if (!tok->t_next)
		goto out;
	tok = tok->t_next;

	if (tok->t_flags & DB_T_ATTR) {
		if (!tok->t_name)
			BUG();
		f->f_field = tok->t_name;
		goto out;
	} else
		f->f_attr = tok;

	if (!tok->t_next)
		goto out;
	tok = tok->t_next;

	if (tok->t_flags & DB_T_ATTR) {
		if (!tok->t_name)
			BUG();
		f->f_field = tok->t_name;
	} else {
		fprintf(stderr, "Unexpected additional token after attribute\n");
		goto errout;
	}

out:
	/* free unused tokens */
	for (tok = tok->t_next ; tok; tok = t) {
		t = tok->t_next;
		if (tok->t_name)
			free(tok->t_name);
		free(tok);
	}

	return f;

errout:
	free(f);
	f = NULL;
	tok = db_filter_out;
	goto out;
}
#endif
