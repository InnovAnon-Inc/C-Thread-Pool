AM_CPPFLAGS = -D__STDC_INT64__

SUBDIRS = ../glitter
AM_CPPFLAGS += -I $(top_srcdir)/glitter/src

LIBTHPOOL_LIBADD = -lpthread

lib_LTLIBRARIES = libthpool.la
libthpool_la_SOURCES = thpool.c
libthpool_la_LIBADD = $(LIBTHIPE_LIBADD)
include_HEADERS = thpool.h
