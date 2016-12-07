## Process this file with automake to produce Makefile.in

INCLUDES = -I${srcdir}/../dpexec -D@ARCH@

EXP=${top_srcdir}/lib/dxexec.exp
WEXP=${top_srcdir}/lib/dxexec.def

noinst_LTLIBRARIES = libDXEXEC.la
libDXEXEC_la_SOURCES = main.c
libDXEXEC_la_CFLAGS = $(AM_CFLAGS)

bindir = $(prefix)/dx/bin_@ARCH@
bin_PROGRAMS = dxexec

dxexec_LDADD = ../dpexec/libDPEXEC.la ../dxmods/libDXMODS.la ../dxmods/user.lo \
	../dxmods/libDXMODSN.la ../libdx/libLIBDX.la ../libdx/mem.lo \
	../libdx/memory.lo ../hwrender/libHW.la ../hwrender/opengl/libOPENGL.la 
dxexec_LDFLAGS = @DXEXEC_EXP@
dxexec_SOURCES = main.c

EXTRA_DIST = tmain.cpp

install-data-hook:
	if test "$(ARCH)" = "intelnt" ; then \
	$(INSTALL_DATA) dxexec.exp $(DESTDIR)$(prefix)/dx/lib_intelnt/dxexec.exp ; \
	$(INSTALL_DATA) dxexec.lib $(DESTDIR)$(prefix)/dx/lib_intelnt/dxexec.lib ; \
	fi
	if test "$(ARCH)" = "cygwin" ; then \
	$(INSTALL_DATA) dxexec.a $(DESTDIR)$(prefix)/dx/lib_cygwin/dxexec.a; \
	fi
