# -*- Makefile -*-

bin_PROGRAMS = bmon

bmon_CFLAGS = \
	-I${top_srcdir}/include \
	-I${top_builddir}/include \
	-DSYSCONFDIR=\"$(sysconfdir)\" \
	-D_GNU_SOURCE \
	-Wall \
	$(CURSES_CFLAGS) \
	$(CONFUSE_CFLAGS) \
	$(LIBNL_CFLAGS) \
	$(LIBNL_ROUTE_CFLAGS)

bmon_LDADD = \
	$(CURSES_LIB) \
	$(CONFUSE_LIBS) \
	$(LIBNL_LIBS) \
	$(LIBNL_ROUTE_LIBS)

bmon_SOURCES = \
	utils.c \
	unit.c \
	conf.c \
	input.c \
	output.c \
	group.c \
	element.c \
	attr.c \
	element_cfg.c \
	history.c \
	graph.c \
	bmon.c \
	module.c \
	in_netlink.c \
	in_null.c \
	in_dummy.c \
	in_proc.c \
	in_sysctl.c \
	out_null.c \
	out_format.c \
	out_ascii.c \
	out_curses.c
