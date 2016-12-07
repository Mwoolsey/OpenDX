## Process this file with automake to produce Makefile.in

INCLUDES = -I${top_srcdir}/include -I${srcdir}/../base -D@ARCH@

dxbindir = $(prefix)/dx/bin_@ARCH@

dxbin_PROGRAMS = tutor

tutor_SOURCES =				\
	Main.C                          \
	NoUndoTutorAppCommand.C         \
	TutorApplication.C              \
	TutorWindow.C			

noinst_HEADERS = \
	NoUndoTutorAppCommand.h \
	TutorApplication.h \
	TutorWindow.h

tutor_LDADD = 				\
	../base/libBASE.a  		\
	../widgets/libWIDGETS.a  	\
	../dxl/libDXL.la ../../exec/libdx/libDXlite.la \
	@DXUI_ALDFLAGS@ 
