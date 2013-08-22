
EXTRA_DIST = Makefile.mak

INCLUDES = -I$(top_srcdir)/include

lib_LIBRARIES = stream_engine.a

libHTSEngine_a_SOURCES = HTS_engine.c  HTS_hidden.h  HTS_misc.c  \
                         HTS_pstream.c  HTS_sstream.c  HTS_model.c  \
                         HTS_gstream.c HTS_label.c

DISTCLEANFILES = *.log *.out *~

MAINTAINERCLEANFILES = Makefile.in
