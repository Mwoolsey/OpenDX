## Process this file with automake to produce Makefile.in

INCLUDES =  -I${top_srcdir}/include -I${srcdir}/.. -I${srcdir}/../../libdx -D@ARCH@

noinst_LTLIBRARIES = libOPENGL.la

SUFFIXES = .m .y .c .s .h .o .a 

noinst_HEADERS = \
	hwPortOGL.h hwPortUtilOGL.help


libOPENGL_la_SOURCES = \
	hwPortUtilOGL.c			\
	hwInteractorEchoOGL.c		\
	hwDisplayListsOGL.c		\
	hwPortOGL.c			\
	hwLoadOGL.c			
