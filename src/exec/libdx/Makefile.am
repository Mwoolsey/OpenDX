## Process this file with automake to produce Makefile.in

MAJOR_VERSION=@MAJOR_VERSION@
MINOR_VERSION=@MINOR_VERSION@
MICRO_VERSION=@MICRO_VERSION@

#INCLUDES = -I../../../include  -D@ARCH@
INCLUDES = -I${top_srcdir}/include  -D@ARCH@

dxlibdir = $(prefix)/dx/lib_@ARCH@

noinst_LTLIBRARIES = libLIBDX.la libcallm.la libmem.la
#dxlib_LIBRARIES = libDXlite.a 

noinst_HEADERS = \
	binSort.h triangles.h internals.h diskio.h zclip.h edf.h render.h \
	qsort.c displayutil.h displayx.h displayw.h plock.h lbmsgs.h

XFILES = \
	arrayClass.X cameraClass.X clippedClass.X fieldClass.X groupClass.X \
	lightClass.X objectClass.X privateClass.X screenClass.X stringClass.X \
	xformClass.X interpClass.X fieldinterpClass.X groupinterpClass.X \
	fle2DClass.X linesII1DClass.X linesRR1DClass.X linesRI1DClass.X \
	trisRI2DClass.X quadsRR2DClass.X cubesIIClass.X cubesRRClass.X \
	tetrasClass.X quadsII2DClass.X 

EXTRA_DIST = \
	class class.awk v3.c \
	$(XFILES) \
	buffer.Z lbshade.Z triangle.Z quad.Z plane.Z  \
	local.mk.in

built_sources = \
	buffer.c lbshade.c triangle.c quad.c plane.c \
	arrayClass.c cameraClass.c clippedClass.c fieldClass.c groupClass.c \
	lightClass.c objectClass.c privateClass.c screenClass.c stringClass.c \
	xformClass.c interpClass.c fieldinterpClass.c groupinterpClass.c \
	fle2DClass.c linesII1DClass.c linesRR1DClass.c linesRI1DClass.c \
	trisRI2DClass.c quadsRR2DClass.c cubesIIClass.c cubesRRClass.c \
	tetrasClass.c quadsII2DClass.c  

built_headers = \
	arrayClass.h cameraClass.h clippedClass.h fieldClass.h groupClass.h \
	lightClass.h objectClass.h privateClass.h screenClass.h stringClass.h \
	xformClass.h interpClass.h fieldinterpClass.h groupinterpClass.h \
	fle2DClass.h linesII1DClass.h linesRR1DClass.h linesRI1DClass.h \
	trisRI2DClass.h quadsRR2DClass.h cubesIIClass.h cubesRRClass.h \
	tetrasClass.h quadsII2DClass.h  

dxlite_sources0 = \
        object.c string.c lbcamera.c clipped.c field.c group.c \
        series.c composite.c multigrid.c lblight.c array.c sharedarray.c \
        regulararray.c cstarray.c patharray.c productarray.c \
        mesharray.c xform.c lbscreen.c lbprivate.c lbmessage.c  \
        timing.c lock.c plock.c stringtable.c client.c basic.c lbvector.c

dxlite_sources1 = \
        matrix.c init.c lbgrid.c box.c helper.c neighbors.c \
        lbprint.c hash.c lbcolor.c lbextract.c component.c edflex.c \
        edfparse.c edfobj.c edfdata.c edfdict.c edfio.c edfprint.c \
        fileio.c rwobject.c renderstubs.c memorystubs.c qmessage.c \
        lbtask.c lbcache.c version.c outglue.c invalid.c seglist.c \
        notify.c userinit.c parts.c

dxlite_sources2 = \
		arrayClass.c cameraClass.c clippedClass.c fieldClass.c \
		groupClass.c lightClass.c objectClass.c stringClass.c \
		xformClass.c screenClass.c privateClass.c

#libDXlite_a_SOURCES = $(dxlite_sources0) $(dxlite_sources1)

dxlib_LTLIBRARIES = libDXlite.la

libDXlite_la_SOURCES = $(dxlite_sources0) $(dxlite_sources1) $(dxlite_sources2)
libDXlite_la_LDFLAGS = -version-info $(MAJOR_VERSION):$(MINOR_VERSION)$(MICRO_VERSION):0

libcallm_la_SOURCES =  \
    qmessage.c outglue.c lbtask.c lbcache.c memorystubs.c \
	callm_xinit.c callm_init.c callm_winit.c


libLIBDX_la_SOURCES =  \
	arrayClass.c cameraClass.c clippedClass.c fieldClass.c groupClass.c \
	lightClass.c objectClass.c privateClass.c screenClass.c stringClass.c \
	xformClass.c interpClass.c fieldinterpClass.c groupinterpClass.c \
	fle2DClass.c linesII1DClass.c linesRR1DClass.c linesRI1DClass.c \
	trisRI2DClass.c quadsRR2DClass.c cubesIIClass.c cubesRRClass.c \
	tetrasClass.c quadsII2DClass.c \
	\
	array.c basic.c box.c lbcamera.c clipped.c composite.c \
	cstarray.c field.c lbgrid.c group.c helper.c init.c lblight.c lock.c \
	matrix.c mesharray.c lbmessage.c multigrid.c \
	neighbors.c object.c patharray.c lbprivate.c productarray.c sharedarray.c \
	regulararray.c lbscreen.c series.c string.c stringtable.c \
	timing.c lbvector.c version.c xform.c userinit.c \
	axes.c buffer.c client.c lbcolor.c component.c displayfb.c displayx.c \
	displayutil.c displayw.c windows.c \
	edfdata.c edfdict.c edfio.c edflex.c edfobj.c edfparse.c edfprint.c \
	lbextract.c fileio.c grow.c hash.c image.c invalid.c irreggrow.c lbmap.c \
	gather.c line.c lbpartition.c partreg.c parts.c permute.c \
	paint.c plock.c lbprint.c reggrow.c rwobject.c seglist.c stats.c lbtext.c \
	lbtransform.c binSort.c interp.c fieldinterp.c groupinterp.c fle2D.c \
	linesII1D.c linesRR1D.c linesRI1D.c trisRI2D.c quadsRR2D.c \
	cubesII.c cubesRR.c tetras.c quadsII2D.c plane.c \
	point.c quad.c lbshade.c tile.c triangle.c volume.c xfield.c \
	xwindow.c zclipT.c zclipQ.c  notify.c lbpick.c lbmsgs.c

# mem.c memory.c not in libDXcallm
libmem_la_SOURCES = mem.c memory.c

CLEANFILES = \
	buffer.c lbshade.c triangle.c quad.c plane.c \
	\
	arrayClass.c cameraClass.c clippedClass.c fieldClass.c groupClass.c \
	lightClass.c objectClass.c privateClass.c screenClass.c stringClass.c \
	xformClass.c interpClass.c fieldinterpClass.c groupinterpClass.c \
	fle2DClass.c linesII1DClass.c linesRR1DClass.c linesRI1DClass.c \
	trisRI2DClass.c quadsRR2DClass.c cubesIIClass.c cubesRRClass.c \
	tetrasClass.c quadsII2DClass.c \
	\
	arrayClass.h cameraClass.h clippedClass.h fieldClass.h groupClass.h \
	lightClass.h objectClass.h privateClass.h screenClass.h stringClass.h \
	xformClass.h interpClass.h fieldinterpClass.h groupinterpClass.h \
	fle2DClass.h linesII1DClass.h linesRR1DClass.h linesRI1DClass.h \
	trisRI2DClass.h quadsRR2DClass.h cubesIIClass.h cubesRRClass.h \
	tetrasClass.h quadsII2DClass.h 

SUFFIXES = .X .Z .h .m .c .o .s .i

@LOCALMAKE@

$(built_headers) : $(XFILES)
$(built_sources) : $(built_headers)

BUILT_SOURCES =  $(built_sources) $(built_headers)

$(OBJECTS): $(built_headers)

zclipQ.lo: ${srcdir}/zclipQ.c
	$(LTCOMPILE) -O -c ${srcdir}/zclipQ.c

zclipT.lo: ${srcdir}/zclipT.c
	$(LTCOMPILE) -O -c ${srcdir}/zclipT.c


