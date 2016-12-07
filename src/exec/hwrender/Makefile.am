## Process this file with automake to produce Makefile.in

SUBDIRS = opengl gl xgl starbase

INCLUDES = -I${top_srcdir}/include -I${srcdir}/../libdx -D@ARCH@

noinst_LTLIBRARIES = libHW.la

SUFFIXES = .m .y .c .s .h .o .a 

noinst_HEADERS = \
	hwClientMessage.h hwClipped.h hwCursorInteractor.h hwDebug.h \
	hwDeclarations.h hwFlags.h hwGather.h hwGlobeEchoDef.h hwInteractor.h \
	hwInteractorEcho.h hwList.h hwMaterials.h hwMatrix.h hwMemory.h \
	hwObject.h hwObjectHash.h hwPortLayer.h hwQmesh.h hwRotateInteractor.h \
	hwScreen.h hwSort.h hwStereo.h hwTmesh.h hwUserInteractor.h \
	hwView.h hwWindow.h hwXfield.h hwZoomInteractor.h

libHW_la_SOURCES = \
	hwClientMessage.c hwClipped.c hwCursorInteractor.c hwDraw.c \
	hwGroupInteractor.c hwInitScreen.c hwInteractor.c hwList.c \
	hwMatrix.c hwMaterials.c hwMemory.c hwNavigateInteractor.c \
	hwObject.c hwObjectHash.c hwPolyline.c hwPortHandle.c \
	hwQmesh.c hwRender.c hwRotateInteractor.c hwScreen.c \
	hwSort.c hwStereo.c hwStereoSys.c hwStereoCams.c  \
	hwTexture.c hwTmesh.c hwWindow.c hwZoomInteractor.c \
	hwGather.c hwPaint.c hwUpdateview.c hwUserInteractor.c \
	hwView.c hwXfield.c

