## The building of Java Explorer should really be done with
## a subpackage configure from the top. This would allow for
## the jni lib to be built with libtool and give us more 
## control over features. But at this time, it is too much
## work.
##
## Process this file with automake to produce Makefile.in
##
## There are 2 build happening here: client side javadx
## and server side javadx.  These 2 builds live together
## because their sources overlap - the dx.protocol
## package is used by each.  The build strategy
## is to call javac on some source which will force
## all the dependencies to be built.  (Java just
## does that for us.)  This way we don't need
## makefiles in subdirs.  This works fine a nightly
## type of build but doesn't work so well as 
## a developer's build.  The -d arg to javac
## causes all resulting .class file to be output
## to the directory arg following the -d.  The
## result is a place which is easy to jar up.
##

## The current version of this Makefile tries to construct
## Java shared libraries by working some magick on the object
## files for each platform. This needs to be reworked with
## libtool and the -shrext flag to make this much more 
## portable.

javadxdir = $(prefix)/dx/java/htmlpages
javadx_DATA = dx.jar 
javasrvdir = $(prefix)/dx/java/server/class
javasrv_DATA = server.jar 
EXTRA_DIST = $(DX_SRCS) DXServer.java makeall.java makewrl.java exports.awk README
javalibdir = $(prefix)/dx/java/server/lib_@ARCH@

javalib_DATA = @ANYDX@
SUBDIRS = server dx layout

##
## Using -d simplifies things because all .class files
## are automatically copied into a directory structure
## which allows for quick jarring
##

CLASSPATH = @DX_JAVA_CLASSPATH@

export CLASSPATH

dx.jar: makeall.java $(DX_SRCS)
	mkdir -p classes
	if test "$(ARCH)" = "cygwin" -o "$(ARCH)" = "intelnt" ; then \
		tmp=`pwd`/classes ; \
		classdir=`cygpath --windows $$tmp` ; \
	else \
		classdir=`pwd`/classes ; \
	fi ; \
	cd $(srcdir) ; \
	$(JAVAC) -d $$classdir makewrl.java || exit 1 ; \
	$(JAVAC) -d $$classdir makeall.java $(DX_SRCS) || exit 1 ; \
	cd $$classdir ; \
	$(JAR) cf ../dx.jar $(DX_OBJS) dx layout || exit 1

server.jar: dx.jar DXServer.java
	mkdir -p dxsc
	if test "$(ARCH)" = "cygwin" -o "$(ARCH)" = "intelnt" ; then \
	        tmp=`pwd`/dxsc ; \
	        dxscdir=`cygpath --windows $$tmp` ; \
	else \
		dxscdir=`pwd`/dxsc ; \
	fi ; \
	cd $(srcdir) ; \
	$(JAVAC) -d $$dxscdir DXServer.java || exit 1 ; \
	cd $$dxscdir ; \
	$(JAR) cf ../server.jar DXServer.class server dx || exit 1

SHRD_FLAGS = -shared -o libAnyDX.so

libAnyDX.so: DXLink.o
	( $(MAKE) usual_shared_object || $(MAKE) aix_shared_object ) 

AnyDX.dll: DXLink.o
	if test "$(ARCH)" = "intelnt" ; then 		\
		( $(MAKE) windows_shared_object ) 	\
	else						\
		( $(MAKE) cygwin_shared_object ) 	\
	fi

libAnyDX.jnilib: DXLink.o
	jObjs=`ls ../dxl/*.o` ;			\
	if test "z$${jObjs}" = "z" ; then 	\
		jObjs=`ls ../dxl/.libs/*.o` ;  	\
	fi ;					\
	jObjs=`echo $$jObjs |sed -e "s&[^ ]*object\.o&&" -e "s&[^ ]*dylib[^ ]*\.o&&" -e "s&[^ ]*x11\.o&&"` ; \
	cc -bundle -o libAnyDX.jnilib -framework JavaVM $${jObjs} DXLink.o -L/usr/X11R6/lib -lXt

usual_shared_object: DXLink.o
	jObjs=`ls ../dxl/*.o` ;                 \
	if test "z$${jObjs}" = "z" ; then       \
		jObjs=`ls ../dxl/.libs/*.o` ;   \
	fi ;                                    \
	jObjs=`echo $$jObjs |sed -e "s&[^ ]*object\.o&&" -e "s&[^ ]*x11\.o&&"` ; \
	$(SHARED_LINK) $(DXABI) $(DX_RTL_ALDFLAGS) $(SO_FLAGS) $(SHRD_FLAGS) $${jObjs}  DXLink.o

aix_shared_object: DXLink.o
	jObjs=`ls ../dxl/*.o` ;			\
	if test "z$${jObjs}" = "z" ; then	\
		jObjs=`ls ../dxl/.libs/*.o` ; 	\
	fi ;					\
	jObjs=`echo $$jObjs |sed -e "s&[^ ]*object\.o&&" -e "s&[^ ]*x11\.o&&"` ; \
	ar -ruv tmpAnyDX.a $${jObjs} DXLink.o
	nm -epC tmpAnyDX.a | awk -f $(srcdir)/exports.awk > tmpAnyDX.exp
	ld -bnoentry -bM:SRE -o libAnyDX.so -bE:tmpAnyDX.exp tmpAnyDX.a -L /usr/lpp/X11/lib -lXt -lX11 -lc
	rm -f tmpAnyDX.*

cygwin_shared_object: DXLink.o
	jObjs=`ls ../dxl/*.o` ;                 \
	if test "z$${jObjs}" = "z" ; then       \
		jObjs=`ls ../dxl/.libs/*.o` ;   \
	fi ;                                    \
	jObjs=`echo $$jObjs |sed -e "s&[^ ]*object\.o&&" -e "s&[^ ]*x11\.o&&"` ; \
	ar -ruv tmpAnyDX.a $${jObjs} DXLink.o
	echo EXPORTS > tmpAnyDX.exp
	nm --demangle --defined-only tmpAnyDX.a | sed -n 's/^.* T //p' >> tmpAnyDX.exp
	dllwrap -s --export-all-symbols --image-base 0x6620000 --def tmpAnyDX.exp --driver-name gcc -o libAnyDX.dll DXLink.o -L../dxl/.libs -lDXL -L/usr/X11R6/lib -lXt -lSM -lICE -lX11
	rm -f tmpAnyDX.*

windows_shared_object: DXLink.o
	jObjs=`ls ../dxl/*.obj` ;                 \
	if test "z$${jObjs}" = "z" ; then       \
		jObjs=`ls ../dxl/.libs/*.obj` ;   \
	fi ;                                    \
	jObjs=`echo $$jObjs |sed -e "s&[^ ]*object\.obj&&" -e "s&[^ ]*x11\.obj&&"` ; \
	ar -ruv tmpAnyDX.a $${jObjs} DXLink.obj
	cc /LD -o AnyDX.dll DXLink.obj tmpAnyDX.a $(LIBS)
#	cc /LD -o AnyDX.dll DXLink.obj -L../dxl/.libs libDXL.lib $(LIBS)
	rm -f tmpAnyDX.*

## JINC set by configure via acinclude.m4

DXLink.o:  server.jar
	javaobj=`pwd` ; \
	cd $(top_srcdir) ; \
	srcincs=`pwd`/include ; \
	cd $$javaobj ; \
	cd $(srcdir) ; \
	javasrc=`pwd` ; \
	cd $$javaobj ; \
	if test "$(ARCH)" = "cygwin" -o "$(ARCH)" = "intelnt" ; then \
	        tmp=`pwd`/server ; \
	        srvrdir=`cygpath --windows $$tmp` ; \
	        tmp=`pwd`/dxsc ; \
	        dxscdir=`cygpath --windows $$tmp` ; \
	else \
	        dxscdir=`pwd`/dxsc ; \
	        srvrdir=`pwd`/server ; \
	fi ; \
	cd $$dxscdir ; \
	CLASSPATH="." $(JAVAH) -d \
	        $$srvrdir server.DXServer server.DXServerThread ; \
	cd $$javaobj ; \
	if test "$(ARCH)" = "cygwin" -o "$(ARCH)" = "intelnet" ; then \
		$(CC) $(DXABI) -o $$javaobj/DXLink.o -I$$srcincs -Iserver \
	        -I$(BUILDBASE)/include -I$$javaobj/server \
	        -c -Dintelnt $(JNI_CFLAGS) @JINC@ $(srcdir)/server/DXLink.c ; \
	else \
		$(CC) $(DXABI) -o $$javaobj/DXLink.o -I$$srcincs -Iserver \
		-I$(BUILDBASE)/include -c -D@ARCH@ $(JNI_CFLAGS) @JINC@ $(srcdir)/server/DXLink.c ; \
	fi ;

##SAMPLE_OBJS = AS400VSMonth.class AutoInsurance.class CCNet2.class \
##	GroceryList.class HomeOwn.class MRI_2.class MappedIso.class \
##	MutualFund.class ScatterData.class SimplifySurface.class \
##	Streamline.class Topo.class TwoWells.class ddi.class \
##	gordon.class hedge.class ibmqw.class mutFun.class npvcall.class \
##	optval.class tgsheet.class wacc.class

##SAMPLE_SRCS = AS400VSMonth.java AutoInsurance.java CCNet2.java \
##	GroceryList.java HomeOwn.java MRI_2.java MappedIso.java \
##	MutualFund.java ScatterData.java SimplifySurface.java Streamline.java \
##	Topo.java TwoWells.java ddi.java gordon.java hedge.java ibmqw.java \
##	mutFun.java npvcall.java optval.java tgsheet.java wacc.java

DX_OBJS = imageWindow.class CaptionLabels.class Status.class HTMLDisplay.class
DX_SRCS = imageWindow.java CaptionLabels.java Status.java HTMLDisplay.java

CLEANFILES = dx.jar server.jar javainfo libAnyDX.dll libAnyDX.so libAnyDX.dylib

distclean-local:
	rm -rf classes dxsc
	find . -name '*.class' | xargs rm -f

install-data-hook:
	if test "$(ARCH)" = "intelnt" ; then \
		$(INSTALL_DATA) AnyDX.lib $(DESTDIR)$(javalibdir)/AnyDX.lib ; \
		$(INSTALL_DATA) AnyDX.exp $(DESTDIR)$(javalibdir)/AnyDX.exp ; \
	fi
	mkdir -p $(prefix)/dx/java/server/nets
