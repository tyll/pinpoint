SUBDIRS=transitions
bin_PROGRAMS=pinpoint

AM_CFLAGS = $(DEPS_CFLAGS) $(MAINTAINER_FLAGS) -D_GNU_SOURCE -DPKGDATADIR=\"$(pkgdatadir)/\"
ACLOCAL_AMFLAGS = -I m4

if USE_DAX
DAX_SOURCES = pp-super-aa.c pp-super-aa.h
endif

pinpoint_LDADD  = $(DEPS_LIBS)
pinpoint_SOURCES = \
  pinpoint-main.c \
  pinpoint-main.h \
  pp-cairo.c \
  pp-clutter.c \
  gst-video-thumbnailer.c \
  $(DAX_SOURCES)

if BUILD_LIBRARY
lib_LTLIBRARIES = libpinpoint-@PP_API_VERSION@.la

libpinpoint_@PP_API_VERSION@_la_SOURCES = \
  pinpoint.c

libpinpoint_@PP_API_VERSION@_includedir = $(includedir)/pinpoint-@PP_API_VERSION@/pinpoint
libpinpoint_@PP_API_VERSION@_include_HEADERS = \
  pinpoint.h
endif

EXTRA_DIST=sample.txt wth.jpg bg.jpg pp-super-aa.c pp-super-aa.h

MAINTAINERCLEANFILES = aclocal.m4 compile config.guess config.sub configure depcomp install-sh ltmain.sh Makefile.in missing

snapshot:
	$(MAKE) dist distdir=$(PACKAGE)-snap`date +"%Y%m%d"`

