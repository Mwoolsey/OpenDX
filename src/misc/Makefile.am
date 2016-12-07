## Process this file with automake to produce Makefile.in

INCLUDES = -I${top_srcdir}/include -D@ARCH@

dxbindir = $(prefix)/bin
noinst_PROGRAMS = dxlocal
archmakdir = $(prefix)/dx/lib_$(ARCH)
archmak_DATA = arch.mak

dxlocal_SOURCES = dx.c utils.c

noinst_HEADERS = dx.h utils.h

install-exec-local: 
	if test ! -z "@INSTALL_BIN_PROGRAM@" ; then		         \
		$(mkinstalldirs) $(prefix)/bin ;                	         \
		$(INSTALL) dxlocal@EXEEXT@ 				         \
		$(prefix)/dx/bin_$(ARCH)/@INSTALL_BIN_PROGRAM@@EXEEXT@ ; \
	fi
