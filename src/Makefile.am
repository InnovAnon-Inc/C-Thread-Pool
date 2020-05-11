AM_CPPFLAGS = -D__STDC_INT64__

#if USE_SUBMODULES
#SUBDIRS = ../glitter
#AM_CPPFLAGS += -I $(top_srcdir)/glitter/src
#endif

#include Makefile-LIBADD.inc
dist_bin_SCRIPTS = libthpool_config

lib_LTLIBRARIES = libthpool.la
libthpool_la_SOURCES = thpool.c
libthpool_la_LIBADD = $(LIBTHPOOL_LIBADD)
include_HEADERS = thpool.h
