/*
 * src/bmon.c		   Bandwidth Monitor
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
#include <bmon/attr.h>
#include <bmon/utils.h>
#include <bmon/input.h>
#include <bmon/output.h>
#include <bmon/module.h>
#include <bmon/group.h>

int start_time;

struct reader_timing rtiming;

static char *usage_text =
"Usage: bmon [OPTION]...\n" \
"\n" \
"Options:\n" \
"Startup:\n" \
"   -i, --input=MODPARM             Input module(s)\n" \
"   -o, --output=MODPARM            Output module(s)\n" \
"   -f, --configfile=PATH           Alternative path to configuration file\n" \
"   -h, --help                      Show this help text\n" \
"   -V, --version                   Show version\n" \
"\n" \
"Input:\n" \
"   -p, --policy=POLICY             Element display policy (see below)\n" \
"   -a, --show-all                  Show all elements (even disabled elements)\n" \
"   -r, --read-interval=FLOAT       Read interval in seconds (float)\n" \
"   -R, --rate-interval=FLOAT       Rate interval in seconds (float)\n" \
"   -s, --sleep-interval=FLOAT      Sleep time in seconds (float)\n" \
"   -L, --lifetime=LIFETIME         Lifetime of an element in seconds (float)\n" \
"\n" \
"Output:\n" \
"   -U, --use-si                    Use SI units\n" \
"   -b, --use-bit                   Display in bits instead of bytes\n" \
"\n" \
"Module configuration:\n" \
"   modparm := MODULE:optlist,MODULE:optlist,...\n" \
"   optlist := option;option;...\n" \
"   option  := TYPE[=VALUE]\n" \
"\n" \
"   Examples:\n" \
"       -o curses:ngraph=2\n" \
"       -o list            # Shows a list of available modules\n" \
"       -o curses:help     # Shows a help text for curses module\n" \
"\n" \
"Interface selection:\n" \
"   policy  := [!]simple_regexp,[!]simple_regexp,...\n" \
"\n" \
"   Example: -p 'eth*,lo*,!eth1'\n" \
"\n" \
"Please see the bmon(8) man pages for full documentation.\n";

static void do_shutdown(void)
{
	static int done;
	
	if (!done) {
		done = 1;
		module_shutdown();
	}
}

static void sig_exit(void)
{
	do_shutdown();
}

void quit(const char *fmt, ...)
{
	static int done;
	va_list args;

	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);

	if (!done) {
		done = 1;
		do_shutdown();
	}

	exit(1);
}

static inline void print_version(void)
{
	printf("bmon %s\n", PACKAGE_VERSION);
	printf("Copyright (C) 2001-2015 by Thomas Graf <tgraf@suug.ch>\n");
	printf("Copyright (C) 2013 Red Hat, Inc.\n");
	printf("bmon comes with ABSOLUTELY NO WARRANTY. This is free " \
	       "software, and you\nare welcome to redistribute it under " \
	       "certain conditions. See the source\ncode for details.\n");
}

static void parse_args_pre(int argc, char *argv[])
{
	DBG("Parsing arguments pre state");

	for (;;)
	{
		char *gostr = "+:hvVf:";

		struct option long_opts[] = {
			{"help", 0, NULL, 'h'},
			{"version", 0, NULL, 'v'},
			{"configfile", 1, NULL, 'f'},
			{NULL, 0, NULL, 0},
		};
		int c = getopt_long(argc, argv, gostr, long_opts, NULL);
		if (c == -1)
			break;
		
		switch (c)
		{
			case 'f':
				set_configfile(optarg);
				break;

			case 'h':
				print_version();
				printf("\n%s", usage_text);
				exit(1);
			case 'V':
			case 'v':
				print_version();
				exit(0);
		}
	}
}

static int parse_args_post(int argc, char *argv[])
{
	DBG("Parsing arguments post state");

	optind = 1;

	for (;;)
	{
		char *gostr = "i:o:p:r:R:s:aUb" \
			      "L:hvVf:";

		struct option long_opts[] = {
			{"input", 1, NULL, 'i'},
			{"output", 1, NULL, 'o'},
			{"policy", 1, NULL, 'p'},
			{"read-interval", 1, NULL, 'r'},
			{"rate-interval", 1, NULL, 'R'},
			{"sleep-interval", 1, NULL, 's'},
			{"show-all", 0, NULL, 'a'},
			{"use-si", 0, NULL, 'U'},
			{"use-bit", 0, NULL, 'b'},
			{"lifetime", 1, NULL, 'L'},
			{NULL, 0, NULL, 0},
		};
		int c = getopt_long(argc, argv, gostr, long_opts, NULL);
		if (c == -1)
			break;
		
		switch (c)
		{
			case 'i':
				if (input_set(optarg))
					return 1;
				break;

			case 'o':
				if (output_set(optarg))
					return 1;
				break;

			case 'p':
				cfg_setstr(cfg, "policy", optarg);
				break;

			case 'r':
				cfg_setfloat(cfg, "read_interval", strtod(optarg, NULL));
				break;

			case 'R':
				cfg_setfloat(cfg, "rate_interval", strtod(optarg, NULL));
				break;

			case 's':
				cfg_setint(cfg, "sleep_time", strtoul(optarg, NULL, 0));
				break;

			case 'a':
				cfg_setbool(cfg, "show_all", cfg_true);
				break;

			case 'U':
				cfg_setbool(cfg, "use_si", cfg_true);
				break;

			case 'b':
				cfg_setbool(cfg, "use_bit", cfg_true);
				break;

			case 'L':
				cfg_setint(cfg, "lifetime", strtoul(optarg, NULL, 0));
				break;

			case 'f':
				/* Already handled in pre getopt loop */
				break;

			default:
				quit("Aborting...\n");
				break;

		}
	}

	return 0;
}

#if 0
static void calc_variance(timestamp_t *c, timestamp_t *ri)
{
	float v = (ts_to_float(c) / ts_to_float(ri)) * 100.0f;

	rtiming.rt_variance.v_error = v;
	rtiming.rt_variance.v_total += v;

	if (v > rtiming.rt_variance.v_max)
		rtiming.rt_variance.v_max = v;

	if (v < rtiming.rt_variance.v_min)
		rtiming.rt_variance.v_min = v;
}
#endif

int main(int argc, char *argv[])
{
	unsigned long sleep_time;
	double read_interval;
	
	start_time = time(NULL);
	memset(&rtiming, 0, sizeof(rtiming));
	rtiming.rt_variance.v_min = FLT_MAX;

	/*
	 * Early initialization before reading config
	 */
	conf_init_pre();
	parse_args_pre(argc, argv);

	/*
	 * Reading the configuration file */
	configfile_read();

	/*
	 * Late initialization after reading config
	 */
	if (parse_args_post(argc, argv))
		return 1;
	conf_init_post();

	module_init();

	read_interval = cfg_read_interval;
	sleep_time = cfg_getint(cfg, "sleep_time");

	if (((double) sleep_time / 1000000.0f) > read_interval)
		sleep_time = (unsigned long) (read_interval * 1000000.0f);

	DBG("Entering mainloop...");

	do {
		/*
		 * E  := Elapsed time
		 * NR := Next Read
		 * LR := Last Read
		 * RI := Read Interval
		 * ST := Sleep Time
		 * C  := Correction
		 */
		timestamp_t e, ri, tmp;
		unsigned long st;

		float_to_timestamp(&ri, read_interval);

		/*
		 * NR := NOW
		 */
		update_timestamp(&rtiming.rt_next_read);
		
		for (;;) {
			output_pre();

			/*
			 * E := NOW
			 */
			update_timestamp(&e);

			/*
			 * IF NR <= E THEN
			 */
			if (timestamp_le(&rtiming.rt_next_read, &e)) {
				timestamp_t c;

				/*
				 * C :=  (NR - E)
				 */
				timestamp_sub(&c, &rtiming.rt_next_read, &e);

				//calc_variance(&c, &ri);

				/*
				 * LR := E
				 */
				copy_timestamp(&rtiming.rt_last_read, &e);

				/*
				 * NR := E + RI + C
				 */
				timestamp_add(&rtiming.rt_next_read, &e, &ri);
				timestamp_add(&rtiming.rt_next_read,
				       &rtiming.rt_next_read, &c);


				reset_update_flags();
				input_read();
				free_unused_elements();
				output_draw();
				output_post();
			}

			/*
			 * ST := Configured ST
			 */
			st = sleep_time;

			/*
			 * IF (NR - E) < ST THEN
			 */
			timestamp_sub(&tmp, &rtiming.rt_next_read, &e);

			if (tmp.tv_sec < 0)
				continue;

			if (tmp.tv_sec == 0 && tmp.tv_usec < st) {
				if (tmp.tv_usec < 0)
					continue;
				/*
				 * ST := (NR - E)
				 */
				st = tmp.tv_usec;
			}
			
			/*
			 * SLEEP(ST)
			 */
			usleep(st);
		}
	} while (0);

	return 0; /* buddha says i'll never be reached */
}

static void __init bmon_init(void)
{
	atexit(&sig_exit);
}
