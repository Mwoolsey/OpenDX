all: sequence.h imagemac.net gifmac.h dxmac.h netlex.c netyacc.c

EditorWindow.o: sequence.h
ImageNodeUtils.o: imagemac.h dxmac.h vrmlmac.h gifmac.h


sequence.h: ${srcdir}/sequence.net 
	sh ${srcdir}/seq2c ${srcdir}/sequence.net sequence.h

imagemac.h: ${srcdir}/imagemac.net
	sh ${srcdir}/net2c ${srcdir}/imagemac.net imagemac.h

gifmac.h:  
	sh ${srcdir}/net2c ${srcdir}/../java/server/macros/gifmac.net gifmac.h

vrmlmac.h:
	sh ${srcdir}/net2c ${srcdir}/../java/server/macros/vrmlmac.net vrmlmac.h

dxmac.h:
	sh ${srcdir}/net2c ${srcdir}/../java/server/macros/dxmac.net dxmac.h

netlex.c: ${srcdir}/net.lex
	$(LEX) ${srcdir}/net.lex
	sed "/#line/d" lex.yy.c > netlex.c

netyacc.c: ${srcdir}/net.y netlex.c ${srcdir}/netyacc.awk
	$(YACC) ${srcdir}/net.y
	awk -f ${srcdir}/netyacc.awk y.tab.c > netyacc.c
	rm -f y.tab.c
