/*
 * in_netlink.c            Netlink input
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
#include <bmon/input.h>
#include <bmon/element.h>
#include <bmon/attr.h>
#include <bmon/conf.h>
#include <bmon/input.h>
#include <bmon/utils.h>

#ifndef SYS_BSD

static int c_notc = 0;
static struct element_group *grp;
static struct bmon_module netlink_ops;

#include <linux/if.h>

#include <netlink/netlink.h>
#include <netlink/cache.h>
#include <netlink/utils.h>
#include <netlink/route/link.h>
#include <netlink/route/tc.h>
#include <netlink/route/qdisc.h>
#include <netlink/route/class.h>
#include <netlink/route/classifier.h>
#include <netlink/route/qdisc/htb.h>

/* These counters are not available prior to libnl 3.2.25. Set them to -1 so
 * rtnl_link_get_stat() won't be called for them. */
#if LIBNL_CURRENT < 220
# define RTNL_LINK_ICMP6_CSUMERRORS	-1
# define RTNL_LINK_IP6_CSUMERRORS	-1
# define RTNL_LINK_IP6_NOECTPKTS	-1
# define RTNL_LINK_IP6_ECT1PKTS		-1
# define RTNL_LINK_IP6_ECT0PKTS		-1
# define RTNL_LINK_IP6_CEPKTS		-1
#endif

/* Not available prior to libnl 3.2.29 */
#if LIBNL_CURRENT < 224
# define RTNL_LINK_RX_NOHANDLER		-1
#endif

static struct attr_map link_attrs[] = {
{
	.name		= "bytes",
	.type		= ATTR_TYPE_COUNTER,
	.unit		= UNIT_BYTE,
	.description	= "Bytes",
	.rxid		= RTNL_LINK_RX_BYTES,
	.txid		= RTNL_LINK_TX_BYTES,
},
{
	.name		= "packets",
	.type		= ATTR_TYPE_COUNTER,
	.unit		= UNIT_NUMBER,
	.description	= "Packets",
	.rxid		= RTNL_LINK_RX_PACKETS,
	.txid		= RTNL_LINK_TX_PACKETS,
},
{
	.name		= "errors",
	.type		= ATTR_TYPE_COUNTER,
	.unit		= UNIT_NUMBER,
	.description	= "Errors",
	.rxid		= RTNL_LINK_RX_ERRORS,
	.txid		= RTNL_LINK_TX_ERRORS,
},
{
	.name		= "drop",
	.type		= ATTR_TYPE_COUNTER,
	.unit		= UNIT_NUMBER,
	.description	= "Dropped",
	.rxid		= RTNL_LINK_RX_DROPPED,
	.txid		= RTNL_LINK_TX_DROPPED,
},
{
	.name		= "compressed",
	.type		= ATTR_TYPE_COUNTER,
	.unit		= UNIT_NUMBER,
	.description	= "Compressed",
	.rxid		= RTNL_LINK_RX_COMPRESSED,
	.txid		= RTNL_LINK_TX_COMPRESSED,
},
{
	.name		= "nohandler",
	.type		= ATTR_TYPE_COUNTER,
	.unit		= UNIT_NUMBER,
	.description	= "No Handler",
	.rxid		= RTNL_LINK_RX_NOHANDLER,
	.txid		= -1,
},
{
	.name		= "fifoerr",
	.type		= ATTR_TYPE_COUNTER,
	.unit		= UNIT_NUMBER,
	.description	= "FIFO Error",
	.rxid		= RTNL_LINK_RX_FIFO_ERR,
	.txid		= RTNL_LINK_TX_FIFO_ERR,
},
{
	.name		= "lenerr",
	.type		= ATTR_TYPE_COUNTER,
	.unit		= UNIT_NUMBER,
	.description	= "Length Error",
	.rxid		= RTNL_LINK_RX_LEN_ERR,
	.txid		= -1,
},
{
	.name		= "overerr",
	.type		= ATTR_TYPE_COUNTER,
	.unit		= UNIT_NUMBER,
	.description	= "Over Error",
	.rxid		= RTNL_LINK_RX_OVER_ERR,
	.txid		= -1,
},
{
	.name		= "crcerr",
	.type		= ATTR_TYPE_COUNTER,
	.unit		= UNIT_NUMBER,
	.description	= "CRC Error",
	.rxid		= RTNL_LINK_RX_CRC_ERR,
	.txid		= -1,
},
{
	.name		= "frameerr",
	.type		= ATTR_TYPE_COUNTER,
	.unit		= UNIT_NUMBER,
	.description	= "Frame Error",
	.rxid		= RTNL_LINK_RX_FRAME_ERR,
	.txid		= -1,
},
{
	.name		= "misserr",
	.type		= ATTR_TYPE_COUNTER,
	.unit		= UNIT_NUMBER,
	.description	= "Missed Error",
	.rxid		= RTNL_LINK_RX_MISSED_ERR,
	.txid		= -1,
},
{
	.name		= "aborterr",
	.type		= ATTR_TYPE_COUNTER,
	.unit		= UNIT_NUMBER,
	.description	= "Abort Error",
	.rxid		= -1,
	.txid		= RTNL_LINK_TX_ABORT_ERR,
},
{
	.name		= "carrerr",
	.type		= ATTR_TYPE_COUNTER,
	.unit		= UNIT_NUMBER,
	.description	= "Carrier Error",
	.rxid		= -1,
	.txid		= RTNL_LINK_TX_CARRIER_ERR,
},
{
	.name		= "hbeaterr",
	.type		= ATTR_TYPE_COUNTER,
	.unit		= UNIT_NUMBER,
	.description	= "Heartbeat Error",
	.rxid		= -1,
	.txid		= RTNL_LINK_TX_HBEAT_ERR,
},
{
	.name		= "winerr",
	.type		= ATTR_TYPE_COUNTER,
	.unit		= UNIT_NUMBER,
	.description	= "Window Error",
	.rxid		= -1,
	.txid		= RTNL_LINK_TX_WIN_ERR,
},
{
	.name		= "coll",
	.type		= ATTR_TYPE_COUNTER,
	.unit		= UNIT_NUMBER,
	.description	= "Collisions",
	.rxid		= -1,
	.txid		= RTNL_LINK_COLLISIONS,
},
{
	.name		= "mcast",
	.type		= ATTR_TYPE_COUNTER,
	.unit		= UNIT_NUMBER,
	.description	= "Multicast",
	.rxid		= -1,
	.txid		= RTNL_LINK_MULTICAST,
},
{
	.name		= "ip6pkts",
	.type		= ATTR_TYPE_COUNTER,
	.unit		= UNIT_NUMBER,
	.description	= "Ip6Pkts",
	.rxid		= RTNL_LINK_IP6_INPKTS,
	.txid		= RTNL_LINK_IP6_OUTPKTS,
},
{
	.name		= "ip6discards",
	.type		= ATTR_TYPE_COUNTER,
	.unit		= UNIT_NUMBER,
	.description	= "Ip6Discards",
	.rxid		= RTNL_LINK_IP6_INDISCARDS,
	.txid		= RTNL_LINK_IP6_OUTDISCARDS,
},
{
	.name		= "ip6octets",
	.type		= ATTR_TYPE_COUNTER,
	.unit		= UNIT_BYTE,
	.description	= "Ip6Octets",
	.rxid		= RTNL_LINK_IP6_INOCTETS,
	.txid		= RTNL_LINK_IP6_OUTOCTETS,
},
{
	.name		= "ip6bcastp",
	.type		= ATTR_TYPE_COUNTER,
	.unit		= UNIT_NUMBER,
	.description	= "Ip6 Broadcast Packets",
	.rxid		= RTNL_LINK_IP6_INBCASTPKTS,
	.txid		= RTNL_LINK_IP6_OUTBCASTPKTS,
},
{
	.name		= "ip6bcast",
	.type		= ATTR_TYPE_COUNTER,
	.unit		= UNIT_BYTE,
	.description	= "Ip6 Broadcast",
	.rxid		= RTNL_LINK_IP6_INBCASTOCTETS,
	.txid		= RTNL_LINK_IP6_OUTBCASTOCTETS,
},
{
	.name		= "ip6mcastp",
	.type		= ATTR_TYPE_COUNTER,
	.unit		= UNIT_NUMBER,
	.description	= "Ip6 Multicast Packets",
	.rxid		= RTNL_LINK_IP6_INMCASTPKTS,
	.txid		= RTNL_LINK_IP6_OUTMCASTPKTS,
},
{
	.name		= "ip6mcast",
	.type		= ATTR_TYPE_COUNTER,
	.unit		= UNIT_BYTE,
	.description	= "Ip6 Multicast",
	.rxid		= RTNL_LINK_IP6_INMCASTOCTETS,
	.txid		= RTNL_LINK_IP6_OUTMCASTOCTETS,
},
{
	.name		= "ip6noroute",
	.type		= ATTR_TYPE_COUNTER,
	.unit		= UNIT_NUMBER,
	.description	= "Ip6 No Route",
	.rxid		= RTNL_LINK_IP6_INNOROUTES,
	.txid		= RTNL_LINK_IP6_OUTNOROUTES,
},
{
	.name		= "ip6forward",
	.type		= ATTR_TYPE_COUNTER,
	.unit		= UNIT_NUMBER,
	.description	= "Ip6 Forwarded",
	.rxid		= -1,
	.txid		= RTNL_LINK_IP6_OUTFORWDATAGRAMS,
},
{
	.name		= "ip6delivers",
	.type		= ATTR_TYPE_COUNTER,
	.unit		= UNIT_NUMBER,
	.description	= "Ip6 Delivers",
	.rxid		= RTNL_LINK_IP6_INDELIVERS,
	.txid		= -1,
},
{
	.name		= "icmp6",
	.type		= ATTR_TYPE_COUNTER,
	.unit		= UNIT_NUMBER,
	.description	= "ICMPv6",
	.rxid		= RTNL_LINK_ICMP6_INMSGS,
	.txid		= RTNL_LINK_ICMP6_OUTMSGS,
},
{
	.name		= "icmp6err",
	.type		= ATTR_TYPE_COUNTER,
	.unit		= UNIT_NUMBER,
	.description	= "ICMPv6 Errors",
	.rxid		= RTNL_LINK_ICMP6_INERRORS,
	.txid		= RTNL_LINK_ICMP6_OUTERRORS,
},
{
	.name		= "icmp6csumerr",
	.type		= ATTR_TYPE_COUNTER,
	.unit		= UNIT_NUMBER,
	.description	= "ICMPv6 Checksum Errors",
	.rxid		= RTNL_LINK_ICMP6_CSUMERRORS,
	.txid		= -1,
},
{
	.name		= "ip6inhdrerr",
	.type		= ATTR_TYPE_COUNTER,
	.unit		= UNIT_NUMBER,
	.description	= "Ip6 Header Error",
	.rxid		= RTNL_LINK_IP6_INHDRERRORS,
	.txid		= -1,
},
{
	.name		= "ip6toobigerr",
	.type		= ATTR_TYPE_COUNTER,
	.unit		= UNIT_NUMBER,
	.description	= "Ip6 Too Big Error",
	.rxid		= RTNL_LINK_IP6_INTOOBIGERRORS,
	.txid		= -1,
},
{
	.name		= "ip6trunc",
	.type		= ATTR_TYPE_COUNTER,
	.unit		= UNIT_NUMBER,
	.description	= "Ip6 Truncated Packets",
	.rxid		= RTNL_LINK_IP6_INTRUNCATEDPKTS,
	.txid		= -1,
},
{
	.name		= "ip6unkproto",
	.type		= ATTR_TYPE_COUNTER,
	.unit		= UNIT_NUMBER,
	.description	= "Ip6 Unknown Protocol Error",
	.rxid		= RTNL_LINK_IP6_INUNKNOWNPROTOS,
	.txid		= -1,
},
{
	.name		= "ip6addrerr",
	.type		= ATTR_TYPE_COUNTER,
	.unit		= UNIT_NUMBER,
	.description	= "Ip6 Address Error",
	.rxid		= RTNL_LINK_IP6_INADDRERRORS,
	.txid		= -1,
},
{
	.name		= "ip6csumerr",
	.type		= ATTR_TYPE_COUNTER,
	.unit		= UNIT_NUMBER,
	.description	= "Ip6 Checksum Error",
	.rxid		= RTNL_LINK_IP6_CSUMERRORS,
	.txid		= -1,
},
{
	.name		= "ip6reasmtimeo",
	.type		= ATTR_TYPE_COUNTER,
	.unit		= UNIT_NUMBER,
	.description	= "Ip6 Reassembly Timeouts",
	.rxid		= RTNL_LINK_IP6_REASMTIMEOUT,
	.txid		= -1,
},
{
	.name		= "ip6fragok",
	.type		= ATTR_TYPE_COUNTER,
	.unit		= UNIT_NUMBER,
	.description	= "Ip6 Reasm/Frag OK",
	.rxid		= RTNL_LINK_IP6_REASMOKS,
	.txid		= RTNL_LINK_IP6_FRAGOKS,
},
{
	.name		= "ip6fragfail",
	.type		= ATTR_TYPE_COUNTER,
	.unit		= UNIT_NUMBER,
	.description	= "Ip6 Reasm/Frag Failures",
	.rxid		= RTNL_LINK_IP6_REASMFAILS,
	.txid		= RTNL_LINK_IP6_FRAGFAILS,
},
{
	.name		= "ip6fragcreate",
	.type		= ATTR_TYPE_COUNTER,
	.unit		= UNIT_NUMBER,
	.description	= "Ip6 Reasm/Frag Requests",
	.rxid		= RTNL_LINK_IP6_REASMREQDS,
	.txid		= RTNL_LINK_IP6_FRAGCREATES,
},
{
	.name		= "ip6noectpkts",
	.type		= ATTR_TYPE_COUNTER,
	.unit		= UNIT_NUMBER,
	.description	= "Ip6 Non-ECT Packets",
	.rxid		= RTNL_LINK_IP6_NOECTPKTS,
	.txid		= -1,
},
{
	.name		= "ip6ect1pkts",
	.type		= ATTR_TYPE_COUNTER,
	.unit		= UNIT_NUMBER,
	.description	= "Ip6 ECT(1) Packets",
	.rxid		= RTNL_LINK_IP6_ECT1PKTS,
	.txid		= -1,
},
{
	.name		= "ip6ect0pkts",
	.type		= ATTR_TYPE_COUNTER,
	.unit		= UNIT_NUMBER,
	.description	= "Ip6 ECT(0) Packets",
	.rxid		= RTNL_LINK_IP6_ECT0PKTS,
	.txid		= -1,
},
{
	.name		= "ip6cepkts",
	.type		= ATTR_TYPE_COUNTER,
	.unit		= UNIT_NUMBER,
	.description	= "Ip6 CE Packets",
	.rxid		= RTNL_LINK_IP6_CEPKTS,
	.txid		= -1,
}
};

static struct attr_map tc_attrs[] = {
{
	.name		= "tc_bytes",
	.type		= ATTR_TYPE_COUNTER,
	.unit		= UNIT_BYTE,
	.description	= "Bytes",
	.rxid		= -1,
	.txid		= RTNL_TC_BYTES,
},
{
	.name		= "tc_packets",
	.type		= ATTR_TYPE_COUNTER,
	.unit		= UNIT_NUMBER,
	.description	= "Packets",
	.rxid		= -1,
	.txid		= RTNL_TC_PACKETS,
},
{
	.name		= "tc_overlimits",
	.type		= ATTR_TYPE_COUNTER,
	.unit		= UNIT_NUMBER,
	.description	= "Overlimits",
	.rxid		= -1,
	.txid		= RTNL_TC_OVERLIMITS,
},
{
	.name		= "tc_drop",
	.type		= ATTR_TYPE_COUNTER,
	.unit		= UNIT_NUMBER,
	.description	= "Dropped",
	.rxid		= -1,
	.txid		= RTNL_TC_DROPS,
},
{
	.name		= "tc_bps",
	.type		= ATTR_TYPE_RATE,
	.unit		= UNIT_BYTE,
	.description	= "Byte Rate/s",
	.rxid		= -1,
	.txid		= RTNL_TC_RATE_BPS,
},
{
	.name		= "tc_pps",
	.type		= ATTR_TYPE_RATE,
	.unit		= UNIT_NUMBER,
	.description	= "Packet Rate/s",
	.rxid		= -1,
	.txid		= RTNL_TC_RATE_PPS,
},
{
	.name		= "tc_qlen",
	.type		= ATTR_TYPE_RATE,
	.unit		= UNIT_NUMBER,
	.description	= "Queue Length",
	.rxid		= -1,
	.txid		= RTNL_TC_QLEN,
},
{
	.name		= "tc_backlog",
	.type		= ATTR_TYPE_RATE,
	.unit		= UNIT_NUMBER,
	.description	= "Backlog",
	.rxid		= -1,
	.txid		= RTNL_TC_BACKLOG,
},
{
	.name		= "tc_requeues",
	.type		= ATTR_TYPE_COUNTER,
	.unit		= UNIT_NUMBER,
	.description	= "Requeues",
	.rxid		= -1,
	.txid		= RTNL_TC_REQUEUES,
}
};

struct rdata {
	struct nl_cache *	class_cache;
	struct element *	parent;
	int 			level;
};

static struct nl_sock *sock;
static struct nl_cache *link_cache, *qdisc_cache;

static void update_tc_attrs(struct element *e, struct rtnl_tc *tc)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(tc_attrs); i++) {
		uint64_t c_tx = rtnl_tc_get_stat(tc, tc_attrs[i].txid);
		attr_update(e, tc_attrs[i].attrid, 0, c_tx, UPDATE_FLAG_TX);
	}
}

static void update_tc_infos(struct element *e, struct rtnl_tc *tc)
{
	char buf[64];

	snprintf(buf, sizeof(buf), "%u", rtnl_tc_get_mtu(tc));
	element_update_info(e, "MTU", buf);

	snprintf(buf, sizeof(buf), "%u", rtnl_tc_get_mpu(tc));
	element_update_info(e, "MPU", buf);

	snprintf(buf, sizeof(buf), "%u", rtnl_tc_get_overhead(tc));
	element_update_info(e, "Overhead", buf);

	snprintf(buf, sizeof(buf), "%#x", rtnl_tc_get_handle(tc));
	element_update_info(e, "Id", buf);

	snprintf(buf, sizeof(buf), "%#x", rtnl_tc_get_parent(tc));
	element_update_info(e, "Parent", buf);

}

static void handle_qdisc(struct nl_object *obj, void *);
static void find_classes(uint32_t, struct rdata *);
static void find_qdiscs(int, uint32_t, struct rdata *);

static struct element *handle_tc_obj(struct rtnl_tc *tc, const char *prefix,
				     const struct rdata *rdata)
{
	char buf[IFNAME_MAX], name[IFNAME_MAX];
	uint32_t id = rtnl_tc_get_handle(tc);
	struct element *e;

	rtnl_tc_handle2str(id, buf, sizeof(buf));
	snprintf(name, sizeof(name), "%s %s (%s)",
		 prefix, buf, rtnl_tc_get_kind(tc));

	if (!rdata || !rdata->parent)
		BUG();

	if (!(e = element_lookup(grp, name, id, rdata->parent, ELEMENT_CREAT)))
		return NULL;

	if (e->e_flags & ELEMENT_FLAG_CREATED) {
		e->e_level = rdata->level;

		if (element_set_key_attr(e, "tc_bytes", "tc_packets") ||
		    element_set_usage_attr(e, "tc_bytes"))
			BUG();

		update_tc_infos(e, tc);

		e->e_flags &= ~ELEMENT_FLAG_CREATED;
	}

	update_tc_attrs(e, tc);

	element_notify_update(e, NULL);
	element_lifesign(e, 1);

	return e;
}

static void handle_cls(struct nl_object *obj, void *arg)
{
	struct rtnl_cls *cls = (struct rtnl_cls *) obj;
	struct rdata *rdata = arg;

	handle_tc_obj((struct rtnl_tc *) cls, "cls", rdata);
}

static void handle_class(struct nl_object *obj, void *arg)
{
	struct rtnl_tc *tc = (struct rtnl_tc *) obj;
	struct element *e;
	const struct rdata *rdata = arg;
	struct rdata ndata = {
		.class_cache = rdata->class_cache,
		.level = rdata->level + 1,
	};

	if (!(e = handle_tc_obj(tc, "class", rdata)))
		return;

	ndata.parent = e;

	if (!strcmp(rtnl_tc_get_kind(tc), "htb"))
		element_set_txmax(e, rtnl_htb_get_rate((struct rtnl_class *) tc));

	find_classes(rtnl_tc_get_handle(tc), &ndata);
	find_qdiscs(rtnl_tc_get_ifindex(tc), rtnl_tc_get_handle(tc), &ndata);
}

static void find_qdiscs(int ifindex, uint32_t parent, struct rdata *rdata)
{
	struct rtnl_qdisc *filter;

	if (!(filter = rtnl_qdisc_alloc()))
		return;

	rtnl_tc_set_parent((struct rtnl_tc *) filter, parent);
	rtnl_tc_set_ifindex((struct rtnl_tc *) filter, ifindex);

	nl_cache_foreach_filter(qdisc_cache, OBJ_CAST(filter),
				handle_qdisc, rdata);

	rtnl_qdisc_put(filter);
}

static void find_cls(int ifindex, uint32_t parent, struct rdata *rdata)
{
	struct nl_cache *cls_cache;

	if (rtnl_cls_alloc_cache(sock, ifindex, parent, &cls_cache) < 0)
		return;

	nl_cache_foreach(cls_cache, handle_cls, rdata);

	nl_cache_free(cls_cache);
}

static void find_classes(uint32_t parent, struct rdata *rdata)
{
	struct rtnl_class *filter;

	if (!(filter = rtnl_class_alloc()))
		return;

	rtnl_tc_set_parent((struct rtnl_tc *) filter, parent);

	nl_cache_foreach_filter(rdata->class_cache, OBJ_CAST(filter),
				handle_class, rdata);

	rtnl_class_put(filter);
}

static void handle_qdisc(struct nl_object *obj, void *arg)
{
	struct rtnl_tc *tc = (struct rtnl_tc *) obj;
	struct element *e;
	const struct rdata *rdata = arg;
	struct rdata ndata = {
		.class_cache = rdata->class_cache,
		.level = rdata->level + 1,
	};

	if (!(e = handle_tc_obj(tc, "qdisc", rdata)))
		return;

	ndata.parent = e;

	find_cls(rtnl_tc_get_ifindex(tc), rtnl_tc_get_handle(tc), &ndata);

	if (rtnl_tc_get_parent(tc) == TC_H_ROOT) {
		find_cls(rtnl_tc_get_ifindex(tc), TC_H_ROOT, &ndata);
		find_classes(TC_H_ROOT, &ndata);
	}

	find_classes(rtnl_tc_get_handle(tc), &ndata);
}

static void handle_tc(struct element *e, struct rtnl_link *link)
{
	struct rtnl_qdisc *qdisc;
	struct nl_cache *class_cache;
	int ifindex = rtnl_link_get_ifindex(link);
	struct rdata rdata = {
		.level = 1,
		.parent = e,
	};

	if (rtnl_class_alloc_cache(sock, ifindex, &class_cache) < 0)
		return;

	rdata.class_cache = class_cache;

	qdisc = rtnl_qdisc_get_by_parent(qdisc_cache, ifindex, TC_H_ROOT);
	if (qdisc) {
		handle_qdisc(OBJ_CAST(qdisc), &rdata);
		rtnl_qdisc_put(qdisc);
	}

	qdisc = rtnl_qdisc_get_by_parent(qdisc_cache, ifindex, 0);
	if (qdisc) {
		handle_qdisc(OBJ_CAST(qdisc), &rdata);
		rtnl_qdisc_put(qdisc);
	}

	qdisc = rtnl_qdisc_get_by_parent(qdisc_cache, ifindex, TC_H_INGRESS);
	if (qdisc) {
		handle_qdisc(OBJ_CAST(qdisc), &rdata);
		rtnl_qdisc_put(qdisc);
	}

	nl_cache_free(class_cache);
}

static void update_link_infos(struct element *e, struct rtnl_link *link)
{
	char buf[64];

	snprintf(buf, sizeof(buf), "%u", rtnl_link_get_mtu(link));
	element_update_info(e, "MTU", buf);

	rtnl_link_flags2str(rtnl_link_get_flags(link), buf, sizeof(buf));
	element_update_info(e, "Flags", buf);

	rtnl_link_operstate2str(rtnl_link_get_operstate(link),
				buf, sizeof(buf));
	element_update_info(e, "Operstate", buf);

	snprintf(buf, sizeof(buf), "%u", rtnl_link_get_ifindex(link));
	element_update_info(e, "IfIndex", buf);

	nl_addr2str(rtnl_link_get_addr(link), buf, sizeof(buf));
	element_update_info(e, "Address", buf);

	nl_addr2str(rtnl_link_get_broadcast(link), buf, sizeof(buf));
	element_update_info(e, "Broadcast", buf);

	rtnl_link_mode2str(rtnl_link_get_linkmode(link),
			   buf, sizeof(buf));
	element_update_info(e, "Mode", buf);

	snprintf(buf, sizeof(buf), "%u", rtnl_link_get_txqlen(link));
	element_update_info(e, "TXQlen", buf);

	nl_af2str(rtnl_link_get_family(link), buf, sizeof(buf));
	element_update_info(e, "Family", buf);

	element_update_info(e, "Alias",
		rtnl_link_get_ifalias(link) ? : "");

	element_update_info(e, "Qdisc",
		rtnl_link_get_qdisc(link) ? : "");

	if (rtnl_link_get_link(link)) {
		snprintf(buf, sizeof(buf), "%u", rtnl_link_get_link(link));
		element_update_info(e, "SlaveOfIndex", buf);
	}
}

static void do_link(struct nl_object *obj, void *arg)
{
	struct rtnl_link *link = (struct rtnl_link *) obj;
	struct element *e, *e_parent = NULL;
	int i, master_ifindex;

	if (!cfg_show_all && !(rtnl_link_get_flags(link) & IFF_UP)) {
		/* FIXME: delete element */
		return;
	}

	/* Check if the interface is a slave of another interface */
	if ((master_ifindex = rtnl_link_get_link(link))) {
		char parent[IFNAMSIZ+1];

		rtnl_link_i2name(link_cache, master_ifindex,
				 parent, sizeof(parent));

		e_parent = element_lookup(grp, parent, master_ifindex, NULL, 0);
	}

	if (!(e = element_lookup(grp, rtnl_link_get_name(link),
				 rtnl_link_get_ifindex(link), e_parent, ELEMENT_CREAT)))
		return;

	if (e->e_flags & ELEMENT_FLAG_CREATED) {
		if (e->e_parent)
			e->e_level = e->e_parent->e_level + 1;

		if (element_set_key_attr(e, "bytes", "packets") ||
		    element_set_usage_attr(e, "bytes"))
			BUG();

		/* FIXME: Update link infos every 1s or so */
		update_link_infos(e, link);

		e->e_flags &= ~ELEMENT_FLAG_CREATED;
	}

	for (i = 0; i < ARRAY_SIZE(link_attrs); i++) {
		struct attr_map *m = &link_attrs[i];
		uint64_t c_rx = 0, c_tx = 0;
		int flags = 0;

		if (m->rxid >= 0) {
			c_rx = rtnl_link_get_stat(link, m->rxid);
			flags |= UPDATE_FLAG_RX;
		}

		if (m->txid >= 0) {
			c_tx = rtnl_link_get_stat(link, m->txid);
			flags |= UPDATE_FLAG_TX;
		}

		attr_update(e, m->attrid, c_rx, c_tx, flags);
	}

	if (!c_notc && qdisc_cache)
		handle_tc(e, link);

	element_notify_update(e, NULL);
	element_lifesign(e, 1);
}

static void netlink_read(void)
{
	int err;

	if ((err = nl_cache_resync(sock, link_cache, NULL, NULL)) < 0) {
		fprintf(stderr, "Unable to resync link cache: %s\n", nl_geterror(err));
		goto disable;
	}

	if (qdisc_cache &&
	    (err = nl_cache_resync(sock, qdisc_cache, NULL, NULL)) < 0) {
		fprintf(stderr, "Unable to resync qdisc cache: %s\n", nl_geterror(err));
		goto disable;
	}

	nl_cache_foreach(link_cache, do_link, NULL);

	return;

disable:
	netlink_ops.m_flags &= ~BMON_MODULE_ENABLED;
}

static void netlink_shutdown(void)
{
	nl_cache_free(link_cache);
	nl_cache_free(qdisc_cache);
	nl_socket_free(sock);
}

static void netlink_use_bit(struct attr_map *map, const int size)
{
	if(cfg_getbool(cfg, "use_bit")) {
		for(int i = 0; i < size; ++i) {
			if(!strcmp(map[i].description, "Bytes")) {
				map[i].description = "Bits";
			}
		}
	}
}

static int netlink_do_init(void)
{
	int err;

	if (!(sock = nl_socket_alloc())) {
		fprintf(stderr, "Unable to allocate netlink socket\n");
		goto disable;
	}

	if ((err = nl_connect(sock, NETLINK_ROUTE)) < 0) {
		fprintf(stderr, "Unable to connect netlink socket: %s\n", nl_geterror(err));
		goto disable;
	}

	if ((err = rtnl_link_alloc_cache(sock, AF_UNSPEC, &link_cache)) < 0) {
		fprintf(stderr, "Unable to allocate link cache: %s\n", nl_geterror(err));
		goto disable;
	}

	if ((err = rtnl_qdisc_alloc_cache(sock, &qdisc_cache)) < 0) {
		fprintf(stderr, "Warning: Unable to allocate qdisc cache: %s\n", nl_geterror(err));
		fprintf(stderr, "Disabling QoS statistics.\n");
		qdisc_cache = NULL;
	}

	netlink_use_bit(link_attrs, ARRAY_SIZE(link_attrs));
	netlink_use_bit(tc_attrs, ARRAY_SIZE(tc_attrs));
	if (attr_map_load(link_attrs, ARRAY_SIZE(link_attrs)) ||
	    attr_map_load(tc_attrs, ARRAY_SIZE(tc_attrs)))
		BUG();

	if (!(grp = group_lookup(DEFAULT_GROUP, GROUP_CREATE)))
		BUG();

	return 0;

disable:
	return -EOPNOTSUPP;
}

static int netlink_probe(void)
{
	struct nl_sock *sock;
	struct nl_cache *lc;
	int ret = 0;

	if (!(sock = nl_socket_alloc()))
		return 0;

	if (nl_connect(sock, NETLINK_ROUTE) < 0)
		return 0;

	if (rtnl_link_alloc_cache(sock, AF_UNSPEC, &lc) == 0) {
		nl_cache_free(lc);
		ret = 1;
	}

	nl_socket_free(sock);

	return ret;
}

static void print_help(void)
{
	printf(
	"netlink - Netlink statistic collector for Linux\n" \
	"\n" \
	"  Powerful statistic collector for Linux using netlink sockets\n" \
	"  to collect link and traffic control statistics.\n" \
	"  Author: Thomas Graf <tgraf@suug.ch>\n" \
	"\n" \
	"  Options:\n" \
	"    notc           Do not collect traffic control statistics\n");
}

static void netlink_parse_opt(const char *type, const char *value)
{
	if (!strcasecmp(type, "notc"))
		c_notc = 1;
	else if (!strcasecmp(type, "help")) {
		print_help();
		exit(0);
	}
}

static struct bmon_module netlink_ops = {
	.m_name		= "netlink",
	.m_flags	= BMON_MODULE_DEFAULT,
	.m_do		= netlink_read,
	.m_shutdown	= netlink_shutdown,
	.m_parse_opt	= netlink_parse_opt,
	.m_probe	= netlink_probe,
	.m_init		= netlink_do_init,
};

static void __init netlink_init(void)
{
	input_register(&netlink_ops);
}
#endif
