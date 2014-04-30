/*
 * in_sysctl.c                 sysctl (BSD)
 *
 * $Id: in_sysctl.c 20 2004-10-30 22:46:16Z tgr $
 *
 * Copyright (c) 2001-2004 Thomas Graf <tgraf@suug.ch>
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
#include <bmon/intf.h>
#include <bmon/conf.h>
#include <bmon/node.h>
#include <bmon/utils.h>

#if defined SYS_BSD
#include <sys/socket.h>
#include <net/if.h>
#include <sys/param.h>
#include <sys/sysctl.h>
#include <net/if_dl.h>
#include <net/route.h>

static int c_debug = 0;

static void
sysctl_read(void)
{
	int mib[] = {CTL_NET, PF_ROUTE, 0, 0, NET_RT_IFLIST, 0};
	size_t n;
	char *buf, *next, *lim;

	if (sysctl(mib, 6, NULL, &n, NULL, 0) < 0)
		quit("sysctl() failed");

	if (c_debug)
		fprintf(stderr, "sysctl 1-pass n=%d\n", n);

	buf = xcalloc(1, n);

	if (sysctl(mib, 6, buf, &n, NULL, 0) < 0)
		quit("sysctl() failed");

	if (c_debug)
		fprintf(stderr, "sysctl 2-pass n=%d\n", n);

	lim = (buf + n);

	for (next = buf; next < lim; ) {
		struct if_msghdr *ifm, *nextifm;
		struct sockaddr_dl *sdl;
		intf_t *i;

		ifm = (struct if_msghdr *) next;
		if (ifm->ifm_type != RTM_IFINFO)
			break;

		next += ifm->ifm_msglen;

		while (next < lim) {
			nextifm = (struct if_msghdr *) next;
			if (nextifm->ifm_type != RTM_NEWADDR)
				break;
			next += nextifm->ifm_msglen;
		}

		sdl = (struct sockaddr_dl *) (ifm + 1);

		if (get_show_only_running() && !(ifm->ifm_flags & IFF_UP))
			continue;

		if(sdl->sdl_family != AF_LINK)
			continue;

		if (c_debug)
			fprintf(stderr, "Processing %s\n", sdl->sdl_data);

		i = lookup_intf(get_local_node(), sdl->sdl_data, 0, 0);

		if (NULL == i)
			continue;

		i->i_rx_bytes.r_total = ifm->ifm_data.ifi_ibytes;
		i->i_tx_bytes.r_total = ifm->ifm_data.ifi_obytes;
		i->i_rx_packets.r_total = ifm->ifm_data.ifi_ipackets;
		i->i_tx_packets.r_total = ifm->ifm_data.ifi_opackets;

		update_attr(i, ERRORS, ifm->ifm_data.ifi_ierrors,
			ifm->ifm_data.ifi_oerrors, RX_PROVIDED | TX_PROVIDED);
		update_attr(i, COLLISIONS, 0, ifm->ifm_data.ifi_collisions,
			TX_PROVIDED);
		update_attr(i, MULTICAST, ifm->ifm_data.ifi_imcasts, 0, RX_PROVIDED);
		update_attr(i, DROP, 0, ifm->ifm_data.ifi_iqdrops, TX_PROVIDED);

		notify_update(i);
		increase_lifetime(i, 1);
	}

	xfree(buf);
}

static void
print_help(void)
{
	printf(
		"sysctl - sysctl statistic collector for BSD and Darwin\n" \
		"\n" \
		"  BSD and Darwin statistic collector using sysctl()\n" \
		"  Author: Thomas Graf <tgraf@suug.ch>\n" \
		"\n");
}

static void
sysctl_set_opts(tv_t *attrs)
{
	while (attrs) {
		if (!strcasecmp(attrs->type, "debug"))
			c_debug = 1;
		else if (!strcasecmp(attrs->type, "help")) {
			print_help();
			exit(0);
		}
		attrs = attrs->next;
	}
}

static int
sysctl_probe(void)
{
	size_t n;
	int mib[] = {CTL_NET, PF_ROUTE, 0, 0, NET_RT_IFLIST, 0};
	if (sysctl(mib, 6, NULL, &n, NULL, 0) < 0)
		return 0;
	return 1;
}

static struct input_module kstat_ops = {
	.im_name = "sysctl",
	.im_read = sysctl_read,
	.im_set_opts = sysctl_set_opts,
	.im_probe = sysctl_probe,
};

static void __init
sysctl_init(void)
{
	register_input_module(&kstat_ops);
}

#endif
