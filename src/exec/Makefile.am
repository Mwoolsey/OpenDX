SUBDIRS = libdx dxmods hwrender dpexec dxexec

dxlibdir = $(prefix)/dx/lib_@ARCH@

MAJOR_VERSION=@MAJOR_VERSION@
MINOR_VERSION=@MINOR_VERSION@
MICRO_VERSION=@MICRO_VERSION@

DELFILES = \
        xxmain.o \
        mduser.o \
        lbmemory.o \
        lbmem.o    \
        exattribute.o \
        exbackground.o \
        excache.o \
        excachegraph.o \
        exccm.o \
        excommand.o \
        excontext.o \
        exdistconnect.o \
        exdistpacket.o \
        exdistqueue.o \
        exdxmain.o \
        exdxpfsmgr.o \
        exevalgraph.o \
        exgraph.o \
        exgraph2.o \
        exgraphqueue.o \
        exhelp.o \
        exinstrument.o \
        exlex.o \
        exlog.o \
        exnodereadb.o \
        exnodewriteb.o \
        expacket.o \
        exparsemdf.o \
        expath.o \
        expendingcmds.o \
        exqueue.o \
        exremote.o \
        exrih.o \
        exrq.o \
        exsocket.o \
        exstatus.o \
        exswap.o \
        extask.o \
        exvariable.o \
        exvcr.o \
        exyuiif.o

dxlib_LTLIBRARIES = libDX.la libDXcallm.la
libDX_la_SOURCES = 
libDX_la_CFLAGS = $(AM_CFLAGS)
libDX_la_LDFLAGS = -version-info $(MAJOR_VERSION):$(MINOR_VERSION)$(MICRO_VERSION):0
libDX_la_LIBADD = dxexec/libDXEXEC.la \
	dxmods/libDXMODS.la dxmods/libuser.la dxmods/libDXMODSN.la \
	libdx/libLIBDX.la libdx/libmem.la \
	dpexec/libDPEXEC.la \
	hwrender/libHW.la \
	hwrender/opengl/libOPENGL.la



libDXcallm_la_SOURCES =
libDXcallm_la_LDFLAGS = -version-info $(MAJOR_VERSION):$(MINOR_VERSION)$(MICRO_VERSION):0
libDXcallm_la_LIBADD = dxmods/libDXMODS.la dxmods/libusercm.la \
	libdx/libLIBDX.la libdx/libcallm.la \
	dpexec/libDPEXECclm.la \
	hwrender/libHW.la \
	hwrender/opengl/libOPENGL.la


