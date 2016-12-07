
SUFFIXES = .m .y .c .s .h .o .a 

.y.h:
	$(YACC) $(YFLAGS) -d ${srcdir}/$*.y
	mv y.tab.h $*.h
	mv y.tab.c $*.c
	echo h rule

.y.c:
	$(YACC) $(YFLAGS) -d ${srcdir}/$*.y
	mv y.tab.h $*.h
	mv y.tab.c $*.c
	echo c rule

compile.h: 
	echo '/***********************************************************************/' >> compile.h
	echo '/* Open Visualization Data Explorer                                    */' >> compile.h
	echo '/* (C) Copyright IBM Corp. 1989,1999                                   */' >> compile.h
	echo '/* ALL RIGHTS RESERVED                                                 */' >> compile.h
	echo '/* This code licensed under the                                        */' >> compile.h
	echo '/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */' >> compile.h
	echo '/***********************************************************************/' >> compile.h
	echo '#define' EX_COM_DATE '"'`date`'"' >> compile.h
	echo '#define' EX_COM_HOST '"'`uname -a`'"' >> compile.h
	
