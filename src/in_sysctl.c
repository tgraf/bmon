/*
 * in_sysctl.c                 sysctl (BSD)
 *
 * $Id: in_sysctl.c 20 2004-10-30 22:46:16Z tgr $
 *
 * Copyright (c) 2001-2004 Thomas Graf <tgraf@suug.ch>
 * Copyright (c) 2014 Žilvinas Valinskas <zilvinas.valinskas@gmail.com>
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
#include <bmon/conf.h>
#include <bmon/group.h>
#include <bmon/element.h>
#include <bmon/utils.h>

#if defined SYS_BSD
#include <sys/socket.h>
#include <net/if.h>
#include <sys/param.h>
#include <sys/sysctl.h>
#include <net/if_dl.h>
#include <net/route.h>

static int c_debug = 0;
static struct element_group *grp;

enum {
	SYSCTL_RX_BYTES = 0x100,
	SYSCTL_TX_BYTES,
	SYSCTL_RX_PACKETS,
	SYSCTL_TX_PACKETS,
	SYSCTL_RX_ERRORS,
	SYSCTL_TX_ERRORS,
	SYSCTL_RX_DROPS,
	SYSCTL_RX_MCAST,
	SYSCTL_TX_MCAST,
	SYSCTL_TX_COLLS,
};
static struct attr_map link_attrs[] = {
{
	.name		= "bytes",
	.type		= ATTR_TYPE_COUNTER,
	.unit		= UNIT_BYTE,
	.rxid		= SYSCTL_RX_BYTES,
	.txid		= SYSCTL_TX_BYTES,
	.description	= "Bytes",
},
{
	.name		= "packets",
	.type		= ATTR_TYPE_COUNTER,
	.unit		= UNIT_NUMBER,
	.rxid		= SYSCTL_RX_PACKETS,
	.txid		= SYSCTL_TX_PACKETS,
	.description	= "Packets",
},
{
	.name		= "errors",
	.type		= ATTR_TYPE_COUNTER,
	.unit		= UNIT_NUMBER,
	.rxid		= SYSCTL_RX_ERRORS,
	.txid		= SYSCTL_TX_ERRORS,
	.description	= "Errors",
},
{
	.name		= "drop",
	.type		= ATTR_TYPE_COUNTER,
	.unit		= UNIT_NUMBER,
	.rxid		= SYSCTL_RX_DROPS,
	.description	= "Dropped",
},
{
	.name		= "coll",
	.type		= ATTR_TYPE_COUNTER,
	.unit		= UNIT_NUMBER,
	.txid		= SYSCTL_TX_COLLS,
	.description	= "Collisions",
},
{
	.name		= "mcast",
	.type		= ATTR_TYPE_COUNTER,
	.unit		= UNIT_NUMBER,
	.rxid		= SYSCTL_RX_MCAST,
	.txid		= SYSCTL_TX_MCAST,
	.description	= "Multicast",
}
};

uint64_t sysctl_get_stats(const struct if_msghdr *ifm, int what)
{
	switch(what) {
	case SYSCTL_RX_BYTES:
		return ifm->ifm_data.ifi_ibytes;
	case SYSCTL_TX_BYTES:
		return ifm->ifm_data.ifi_obytes;

	case SYSCTL_RX_PACKETS:
		return ifm->ifm_data.ifi_ipackets;
	case SYSCTL_TX_PACKETS:
		return ifm->ifm_data.ifi_opackets;

	case SYSCTL_RX_ERRORS:
		return ifm->ifm_data.ifi_ierrors;
	case SYSCTL_TX_ERRORS:
		return ifm->ifm_data.ifi_oerrors;

	case SYSCTL_RX_DROPS:
		return ifm->ifm_data.ifi_iqdrops;

	case SYSCTL_RX_MCAST:
		return ifm->ifm_data.ifi_imcasts;
	case SYSCTL_TX_MCAST:
		return ifm->ifm_data.ifi_omcasts;
	case SYSCTL_TX_COLLS:
		return ifm->ifm_data.ifi_collisions;

	default:
		return 0;
	};
}

static void
sysctl_read(void)
{
	int mib[] = {CTL_NET, PF_ROUTE, 0, 0, NET_RT_IFLIST, 0};
	size_t n;
	char *buf, *next, *lim;

	if (sysctl(mib, 6, NULL, &n, NULL, 0) < 0)
		quit("sysctl() failed");

	if (c_debug)
		fprintf(stderr, "sysctl 1-pass n=%zd\n", n);

	buf = xcalloc(1, n);

	if (sysctl(mib, 6, buf, &n, NULL, 0) < 0)
		quit("sysctl() failed");

	if (c_debug)
		fprintf(stderr, "sysctl 2-pass n=%zd\n", n);

	lim = (buf + n);
	for (next = buf; next < lim; ) {
		struct element *e, *e_parent = NULL;
		struct if_msghdr *ifm, *nextifm;
		struct sockaddr_dl *sdl;
		char info_buf[64];

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

		if (!cfg_show_all && !(ifm->ifm_flags & IFF_UP))
			continue;

		if (sdl->sdl_family != AF_LINK)
			continue;

		if (c_debug)
			fprintf(stderr, "Processing %s\n", sdl->sdl_data);

		sdl->sdl_data[sdl->sdl_nlen] = '\0';
		e = element_lookup(grp,
				   sdl->sdl_data, sdl->sdl_index,
				   e_parent, ELEMENT_CREAT);
		if (!e)
			continue;

		if (e->e_flags & ELEMENT_FLAG_CREATED) {
			if (e->e_parent)
				e->e_level = e->e_parent->e_level + 1;

			if (element_set_key_attr(e, "bytes", "packets") ||
			    element_set_usage_attr(e, "bytes"))
				BUG();

			e->e_flags &= ~ELEMENT_FLAG_CREATED;
		}

		int i;
		for (i = 0; i < ARRAY_SIZE(link_attrs); i++) {
			struct attr_map *m = &link_attrs[i];
			uint64_t rx = 0, tx = 0;
			int flags = 0;

			if (m->rxid) {
				rx = sysctl_get_stats(ifm, m->rxid);
				flags |= UPDATE_FLAG_RX;
			}

			if (m->txid) {
				tx = sysctl_get_stats(ifm, m->txid);
				flags |= UPDATE_FLAG_TX;
			}

			attr_update(e, m->attrid, rx, tx, flags);
		}

		snprintf(info_buf, sizeof(info_buf), "%ju", (uintmax_t)ifm->ifm_data.ifi_mtu);
		element_update_info(e, "MTU", info_buf);

		snprintf(info_buf, sizeof(info_buf), "%ju", (uintmax_t)ifm->ifm_data.ifi_metric);
		element_update_info(e, "Metric", info_buf);

#ifndef __NetBSD__
		snprintf(info_buf, sizeof(info_buf), "%u", ifm->ifm_data.ifi_recvquota);
		element_update_info(e, "RX-Quota", info_buf);

		snprintf(info_buf, sizeof(info_buf), "%u", ifm->ifm_data.ifi_xmitquota);
		element_update_info(e, "TX-Quota", info_buf);
#endif

		element_notify_update(e, NULL);
		element_lifesign(e, 1);
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
sysctl_set_opts(const char* type, const char* value)
{
	if (!strcasecmp(type, "debug"))
		c_debug = 1;
	else if (!strcasecmp(type, "help")) {
		print_help();
		exit(0);
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

static int sysctl_do_init(void)
{
	if (attr_map_load(link_attrs, ARRAY_SIZE(link_attrs)))
		BUG();

	grp = group_lookup(DEFAULT_GROUP, GROUP_CREATE);
	if (!grp)
		BUG();

	return 0;
}

static struct bmon_module kstat_ops = {
	.m_name = "sysctl",
	.m_do = sysctl_read,
	.m_parse_opt = sysctl_set_opts,
	.m_probe = sysctl_probe,
	.m_init = sysctl_do_init,
};

static void __init
sysctl_init(void)
{
	input_register(&kstat_ops);
}

#endif
