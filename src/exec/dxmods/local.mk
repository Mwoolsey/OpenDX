SUFFIXES = .c .y .c .s .h .o 


.l.c:
	lex $(LFLAGS) $*.l

usercm.c:  dxcm.mdf
	@echo usercm.c rule
	awk -f ${top_srcdir}/lib/mdf2c.awk < dxcm.mdf > usercm.c

user.c:  dx.mdf
	@echo user.c rule
	awk -f ${top_srcdir}/lib/mdf2c.awk < dx.mdf > user.c

dxcm.mdf:  ${srcdir}/dxmdf.src
	-rm -f dxcm.mdf
	cp ${srcdir}/dxmdf.src tmp.c
	$(CPP) $(INCLUDES) $(MDF) $(DEFS) tmp.c > dxcm.mdf
	-rm -f tmp.c

dx.mdf:  ${srcdir}/dxmdf.src
	echo MYINC: $(MYINC)
	-rm -f dx.mdf
	cp ${srcdir}/dxmdf.src tmp.c
	$(CPP) $(INCLUDES) $(MYINC) $(MDF) $(DEFS) -DCPLUSPLUS_UI -DEXECUTIVE_MODS tmp.c > dx.mdf
	@echo '/^ *$$/{' > stripl.sed
	@echo 'N' >> stripl.sed
	@echo '/^ *\\n *$$/D' >> stripl.sed
	@echo '}' >> stripl.sed
	cat dx.mdf |sed '/^#/d' > tmp.mdf
	cat tmp.mdf |sed -f stripl.sed > dx.mdf
	@rm -f stripl.sed tmp.mdf
	-rm -f tmp.c

.y.h:
	$(YACC) $(YFLAGS) -d ${srcdir}/$*.y
	@mv y.tab.h $*.h
	@mv y.tab.c $*.c
	@echo h rule

.y.c:
	$(YACC) $(YFLAGS) -d ${srcdir}/$*.y
	@mv y.tab.h $*.h
	@mv y.tab.c $*.c
	@echo c rule

#y.tab.c: ${srcdir}/_compparse.y
#	$(YACC) -vd ${srcdir}/_compparse.y

#_compparse.c: y.tab.c
#	sed -e "s/yy/_dxfcc/g" -e "s/YY/_CC/g" y.tab.c > _compparse.c

#_compparse.h: y.tab.c
#	sed -e "s/yy/_dxfcc/g" -e "s/YY/_CC/g" y.tab.h > _compparse.h


