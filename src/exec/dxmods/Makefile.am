## Process this file with automake to produce Makefile.in

INCLUDES = -I${top_srcdir}/include -I${srcdir}/../dpexec -I$(BUILDBASE)/include -D@ARCH@

dxmdfdir = $(prefix)/dx/lib
noinst_LTLIBRARIES = libDXMODS.la libusercm.la libuser.la libDXMODSN.la
SUFFIXES = .c .y .c .s .h .o 

BUILT_SOURCES = user.c usercm.c
EXTRA_DIST = local.mk

DISTCLEANFILES = dx.mdf dxcm.mdf user.c usercm.c tmp.c

dxmdf_DATA = dx.mdf

_complex.o: _compparse.h

HEADERS1 = \
	_autocolor.h _autogray.h _colormap.h _compoper.h _compparse.h \
	_compputils.h  _compute.h _connectgrids.h _connectvor.h _construct.h \
	_divcurl.h _getfield.h _glyph.h _gradient.h _grid.h _helper_jea.h \
	_isosurface.h _maptoplane.h _newtri.h _normals.h _partnbrs.h \
	_plot.h _post.h _refine.h _rubbersheet.h _rw_image.h _sample.h \
	autoaxes.h bounds.h cases.h cat.h changemember.h color.h echo.h

HEADERS2 = \
	eigen.h exp_gai.h genimp.h glyph_ARRW.h glyph_ARRW2D.h glyph_BOX.h \
	glyph_CIRCLE10.h glyph_CIRCLE20.h glyph_CIRCLE4.h glyph_CIRCLE40.h \
	glyph_CIRCLE6.h glyph_CIRCLE8.h glyph_DMND.h glyph_NDDL.h glyph_NDDL2D.h \
	glyph_PNT.h glyph_RCKT12.h glyph_RCKT20.h glyph_RCKT2D.h glyph_RCKT3.h \
	glyph_RCKT4.h glyph_RCKT6.h glyph_RCKT8.h glyph_SPHR114.h glyph_SPHR12.h

HEADERS3 = \
	glyph_SPHR14.h glyph_SPHR146.h glyph_SPHR26.h glyph_SPHR266.h \
	glyph_SPHR42.h glyph_SPHR62.h glyph_SQUARE.h histogram.h impCDF.h \
	import.h integer.h interact.h list.h macroutil.h mark.h measure.h \
	plot.h quant.h scalar.h  separate.h showboundary.h simplesurf.h \
	stream.h superwin.h trace.h unpart.h userInteractors.h vectors.h \
	verify.h vrml.h vsincos.h

noinst_HEADERS = $(HEADERS1) $(HEADERS2) $(HEADERS3)

SOURCES1 = \
	ambientlight.c append.c arrange.c attribute.c \
	autocolor.c autoaxes.c autoglyph.c autogray.c \
	autoregrid.c band.c bspline.c cachescene.c camera.c \
	caption.c categorize.c catstats.c changetype.c \
	changemember.c clipbox.c clipplane.c collect.c \
	collectmulti.c collectnamed.c collectser.c color.c \
	colorbar.c colormap.c compute.c compute2.c connect.c 
SOURCES2 = \
	construct.c convert.c copy.c describe.c direction.c \
	display.c divcurl.c dxlmessage.c \
	dxloutvalue.c echo.c enumerate.c equalize.c \
	export.c extract.c facenormals.c filter.c format.c \
	fourier.c getscene.c glyph.c gradient.c \
	grid.c histogram.c imagemessage.c import.c import_ss.c \
	include.c inquire.c isolate.c isosurface.c integer.c 
SOURCES3 = \
	integerlist.c keyin.c light.c list.c lookup.c \
	macroutil.c map.c maptoplane.c mark.c \
	measure.c message.c normals.c options.c overlay.c \
	parse.c partition.c pick.c pickinv.c pie.c plot.c \
	post.c print.c programctl.c quantize.c rdimagewin.c \
	readimage.c reduce.c refine.c regrid.c remove.c \
	rename.c render.c reorient.c replace.c ribbon.c 
SOURCES4 = \
	rotate.c route.c rubbersheet.c sample.c scalar.c \
	scalarlist.c scale.c scalescreen.c screen.c select.c \
	selector.c shade.c showboundary.c showbox.c \
	showconnect.c showposition.c slab.c slice.c socketconnect.c \
	sort.c stack.c statistics.c streakline.c streamline.c \
	stringlegend.c superstate.c superwin.c switch.c system.c \
	text.c toggle.c trace.c transform.c translate.c 
SOURCES5 = \
	transpose.c tube.c unmark.c usage.c vector.c \
	vectorlist.c verify.c visualobject.c writeimage.c \
	simplesurf.c dxmdf.src \
	_autocolor.c _autogray.c \
	_cat_util.c _color.c _colormap.c _complex.c \
	_compoper.c _compoper1.c _compoper2.c _compcmplx.c \
	_compinput.c _compexec.c _compparse.c _compputils.c 
SOURCES6 = \
	_connectgrids.c _connectvor.c _construct.c _divcurl.c \
	_getfield.c _gif.c _glyph.c _gradient.c \
	_grid.c _helper_jea.c _im_image.c _irregstream.c _isosurface.c \
	_maptoplane.c _newtri.c _normals.c _partnbrs.c \
	_plot.c _post.c _postscript.c _refine.c \
	_refineirr.c _refinereg.c _refinetopo.c _regstream.c \
	_rgb_image.c _rubbersheet.c _rw_image.c _sample.c 
SOURCES7 =  \
	_simplesurf.c _tiff.c _tube.c _unpart.c \
	bounds.c definter.c exp_gai.c genimp.c \
	genimp_io.c genimp_parse.c importtable.c import_cdf.c \
	import_cm.c import_hdf.c import_ncdf.c private.c \
	separate.c superwinX.c superwinW.c tracevisual.c vectors.c \
	vrml.c eigen.c vsincos.c

libusercm_la_SOURCES = usercm.c 

libuser_la_SOURCES = user.c

libDXMODSN_la_SOURCES = executive.c getset.c loop.c dxlinnamed.c sequencer.c

libDXMODS_la_SOURCES =   \
	$(SOURCES1) \
	$(SOURCES2) \
	$(SOURCES3) \
	$(SOURCES4) \
	$(SOURCES5) \
	$(SOURCES6) \
	$(SOURCES7)

@LOCALMAKE@
