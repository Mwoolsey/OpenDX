## Process this file with automake to produce Makefile.in

MAJOR_VERSION=@MAJOR_VERSION@
MINOR_VERSION=@MINOR_VERSION@
MICRO_VERSION=@MICRO_VERSION@


INCLUDES = -I${top_srcdir}/include  -D@ARCH@

dxlibdir = $(prefix)/dx/lib_@ARCH@

noinst_HEADERS = \
	dxlP.h \
	dict.h

dxlib_LTLIBRARIES = libDXL.la

libDXL_la_SOURCES = close.c	conn.c connect.c dict.c	execute.c exit.c	\
	handlers.c load.c os2.c open.c query.c save.c send.c sequencer.c	\
	socket.c value.c windows.c x11.c object.c

libDXL_la_LDFLAGS = -version-info $(MAJOR_VERSION):$(MINOR_VERSION)$(MICRO_VERSION):0
