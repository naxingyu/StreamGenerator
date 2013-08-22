
EXTRA_DIST = Makefile.mak

INCLUDES = -I$(top_srcdir)/include

bin_PROGRAMS = stream_generator

hts_engine_SOURCES = stream_generator.c 

hts_engine_LDADD = ../lib/stream_engine.a

DISTCLEANFILES = *.log *.out *~

MAINTAINERCLEANFILES = Makefile.in
