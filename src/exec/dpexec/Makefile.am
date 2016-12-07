## Process this file with automake to produce Makefile.in

INCLUDES = -I${top_srcdir}/include -I${srcdir}/../libdx -D@ARCH@

noinst_LTLIBRARIES = libDPEXEC.la libDPEXECclm.la

SUFFIXES = .m .y .c .s .h .o .a 

BUILT_SOURCES = compile.h

noinst_HEADERS = \
	_eval.h _macro.h _variable.h attribute.h background.h cache.h \
	cachegraph.h ccm.h command.h compile.h config.h context.h crc.h \
	d.h distp.h dxmain.h dxpfsmgr.h evalgraph.h exobject.h function.h graph.h \
	graphIntr.h graphqueue.h help.h instrument.h lex.h license.h \
	loader.h log.h nodeb.h obmodule.h packet.h parse.h parsemdf.h \
	path.h pendingcmds.h pmodflags.h queue.h rq.h remote.h rih.h rq.h \
	sfile.h socket.h status.h swap.h sysvars.h task.h userinter.h \
	utils.h vcr.h version.h yuiif.h

libDPEXEC_la_SOURCES =  \
	dpattribute.c background.c cache.c cachegraph.c ccm.c \
	command.c context.c crc.c d.c distconnect.c distpacket.c \
	distqueue.c dxmain.c dxpfsmgr.c evalgraph.c exobject.c function.c \
	graph.c graph2.c graphqueue.c help.c instrument.c lex.c \
	license.c loader.c log.c macro.c nodereadb.c \
	nodewriteb.c packet.c dpparse.c parsemdf.c path.c pendingcmds.c \
	queue.c remote.c rih.c rq.c socket.c status.c \
	swap.c task.c userinter.c utils.c variable.c vcr.c \
	optarg.c yuiif.c sfile.c ddx.c

libDPEXECclm_la_SOURCES = \
	crc.c d.c exobject.c function.c license.c loader.c macro.c \
	dpparse.c userinter.c utils.c optarg.c sfile.c ddx.c

EXTRA_DIST = local.mk dxThreadMain.cpp dxThreadMain.h tmainUtil.cpp tmainUtil.h

$(OBJECTS): yuiif.c yuiif.h

DISTCLEANFILES = compile.h

@LOCALMAKE@

command.o : compile.h


