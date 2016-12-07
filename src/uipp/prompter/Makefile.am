## Process this file with automake to produce Makefile.in

INCLUDES =  -I${srcdir}/../base -I${top_srcdir}/include -D@ARCH@

dxbindir = $(prefix)/dx/bin_@ARCH@

dxbin_PROGRAMS = prompter

noinst_HEADERS = \
	Browser.h \
	BrowserCommand.h \
	CDFChoice.h \
	CommandTextPopup.h \
	CommentDialog.h \
	ConfirmedOpenCommand.h \
	ConfirmedQCommand.h \
	DXChoice.h \
	DataFileDialog.h \
	Field.h \
	FileContents.h \
	FilenameSelectDialog.h \
	GARApplication.h \
	GARChooserWindow.h \
	GARCommand.h \
	GARMainWindow.h \
	GARNewCommand.h \
	GridChoice.h \
	HDFChoice.h \
	ImageChoice.h \
	ImportableChoice.h \
	MsgDialog.h \
	NetCDFChoice.h \
	NoUndoChoiceCommand.h \
	NoUndoChooserCommand.h \
	NoUndoGARAppCommand.h \
	NonimportableChoice.h \
	OpenFileDialog.h \
	QuitCommand.h \
	RecordSeparator.h \
	SADialog.h \
	SearchDialog.h \
	SpreadSheetChoice.h \
	TypeChoice.h 

prompter_SOURCES =  		 	\
	Browser.C			\
	BrowserCommand.C		\
	BuildTCDict.C			\
	CDFChoice.C			\
	CommandTextPopup.C		\
	CommentDialog.C			\
	ConfirmedOpenCommand.C		\
	ConfirmedQCommand.C		\
	DataFileDialog.C		\
	DXChoice.C			\
	Field.C				\
	FileContents.C			\
	FilenameSelectDialog.C		\
	GARApplication.C		\
	GARChooserWindow.C		\
	GARCommand.C			\
	GARMainWindow.C			\
	GARNewCommand.C			\
	GridChoice.C			\
	HDFChoice.C			\
	ImageChoice.C			\
	ImportableChoice.C		\
	MsgDialog.C			\
	NetCDFChoice.C			\
	NonimportableChoice.C		\
	NoUndoChoiceCommand.C		\
	NoUndoChooserCommand.C		\
	NoUndoGARAppCommand.C		\
	OpenFileDialog.C		\
	QuitCommand.C			\
	RecordSeparator.C		\
	SADialog.C			\
	SearchDialog.C			\
	SpreadSheetChoice.C		\
	TypeChoice.C			\
	\
	Main.C

prompter_LDADD = 			\
	../base/libBASE.a  		\
	../widgets/libWIDGETS.a  	\
	../dxl/libDXL.la ../../exec/libdx/libDXlite.la \
	@DXUI_ALDFLAGS@	

EXTRA_DIST = 				\
	col_insens.bm			\
	col_sens.bm			\
	grid1.bm			\
	grid2.bm			\
	grid3.bm			\
	grid4.bm			\
	row_insens.bm			\
	row_sens.bm			\
	series1.bm			\
	series1_insens.bm		\
	series2.bm			\
	series2_insens.bm		\
	vector1.bm			\
	vector1_insens.bm		\
	vector2.bm			\
	vector2_insens.bm

