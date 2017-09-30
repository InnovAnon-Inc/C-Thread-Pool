AM_CPPFLAGS = -D__STDC_INT64__

SUBDIRS = ../glitter
AM_CPPFLAGS += -I $(top_srcdir)/glitter/src

include Makefile-LIBADD.inc

lib_LTLIBRARIES = libthpool.la
libthpool_la_SOURCES = thpool.c
libthpool_la_LIBADD = $(LIBTHPOOL_LIBADD)
include_HEADERS = thpool.h
