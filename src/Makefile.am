AUTOMAKE_OPTIONS = foreign

lib_LTLIBRARIES = libsqface.la

libsqface_la_SOURCES = sqface.cpp

libsqface_la_LIBADD = @LTLIBOBJS@
libsqface_la_LDFLAGS = -release @VERSION@

include_HEADERS = sqface.h sqface_version.h
EXTRA_DIST = sqface.h sqface_version.h
noinst_HEADERS = sqface_config.h
