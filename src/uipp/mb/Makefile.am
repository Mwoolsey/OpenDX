## Process this file with automake to produce Makefile.in

INCLUDES = -I${top_srcdir}/include -I${srcdir}/../base -I${srcdir}/../widgets -D@ARCH@

dxbindir = $(prefix)/dx/bin_@ARCH@

dxbin_PROGRAMS = builder

builder_SOURCES =			\
	MBApplication.C			\
	MBMainWindow.C			\
	MBCommand.C			\
	MBNewCommand.C			\
	OpenFileDialog.C		\
	SADialog.C			\
	QuitCommand.C			\
	ConfirmedQCommand.C		\
	CommentDialog.C			\
	OptionsDialog.C			\
	MBParameter.C			\
	MBGenerate.C			\
	\
	Main.C

noinst_HEADERS = \
	CommentDialog.h \
	ConfirmedQCommand.h \
	MBApplication.h \
	MBCommand.h \
	MBGenerate.h \
	MBMainWindow.h \
	MBNewCommand.h \
	MBParameter.h \
	OpenFileDialog.h \
	OptionsDialog.h \
	QuitCommand.h \
	SADialog.h

builder_LDADD = 			\
	../base/libBASE.a  		\
	../widgets/libWIDGETS.a 	\
	@DXUI_ALDFLAGS@
