## Process this file with automake to produce Makefile.in

AUTOMAKE_OPTIONS = foreign

INCLUDES = -I$(top_srcdir)/include

lib_LTLIBRARIES = libjam.la

libjam_CURRENT  = 2
libjam_REVISION = 5
libjam_AGE      = 1

libjam_la_LDFLAGS = -export-dynamic -version-info $(libjam_CURRENT):$(libjam_REVISION):$(libjam_AGE)

libjam_la_SOURCES = crc32.c lastread.c mbase.c message.c structrw.c subpack.c

noinst_HEADERS = structrw.h
