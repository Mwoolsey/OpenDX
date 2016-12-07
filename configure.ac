#include "aclocal.m4"

AC_PREREQ(2.57)
dnl Start of configuration ----
AC_INIT([dx],[4.4.4])

dnl Process this file with autoconf to produce a configure script.
AC_CONFIG_SRCDIR([include/dx/arch.h])
AC_CONFIG_HEADERS([include/dxconfig.h])
AM_INIT_AUTOMAKE
AC_DISABLE_SHARED
AC_PROG_LIBTOOL

VERSION_STRING="04.4.4000"
MAJOR_VERSION=4
MINOR_VERSION=4
MICRO_VERSION=4
AC_DEFINE(DXD_VERSION_STRING, "04.4.4000", [Version String Number])
AC_DEFINE(DXD_COPYRIGHT_STRING,
  ["Open Visualization Data Explorer\nMore Info at www.research.ibm.com/dx\nand www.opendx.org\nVersion - 4.4.4"], 
  [Copyright string placed on splash screen startup.])
AC_DEFINE_UNQUOTED(DXD_VERSION, $MAJOR_VERSION, [Version Number])
AC_DEFINE_UNQUOTED(DXD_RELEASE, $MINOR_VERSION, [Release Number])
AC_DEFINE_UNQUOTED(DXD_MODIFICATION, $MICRO_VERSION, [Modification Number])

AC_SUBST(MAJOR_VERSION)dnl
AC_SUBST(MINOR_VERSION)dnl
AC_SUBST(MICRO_VERSION)dnl

SHELL=sh
BUILDBASE="`pwd`"

AC_SUBST(YACC)
AC_SUBST(SHELL)
AC_SUBST(ARCH)
AC_SUBST(EXEEXT)
AC_SUBST(OBJEXT)
AC_SUBST(DOT_EXE_EXT)
AC_SUBST(INSTALL_BIN_PROGRAM)
AC_SUBST(INSTALL_BIN_SCRIPT)
AC_SUBST(FMT_LIBS)
AC_SUBST(LIBS)
AC_SUBST(JAVABRANCH)
AC_SUBST(JAVAC)
AC_SUBST(JAR)
AC_SUBST(JAVAH)
AC_SUBST(JINC)
AC_SUBST(DX_JAVA_CLASSPATH)
AC_SUBST(JNI_CFLAGS)
AC_SUBST(BUILDBASE)
AC_SUBST(DXEXEC_EXP)
AC_SUBST(DXEXEC_IMP)
AC_SUBST(DXUI_ALDFLAGS)
AC_SUBST(VERSION_STRING)
AC_SUBST(ANYDX)
AC_SUBST(DXABI)
AC_SUBST(DX_RTL_CFLAGS)
AC_SUBST(DX_RTL_ALDFLAGS)
AC_SUBST(DX_RTL_SYSLIBS)
AC_SUBST(DX_RTL_IMPORTS)
AC_SUBST(DX_RTL_DXENTRY)
AC_SUBST(DX_OUTBOARD_LIBS)
AC_SUBST(DX_X_LINK_LIBS)
AC_SUBST(DX_GL_LINK_LIBS)
AC_SUBST(SHARED_LINK)
AC_SUBST(MKDEP)
AC_SUBST(JNI_CFLAGS)
AC_SUBST(LOCALMAKE)

dnl First set up all the possible configure command line options
dnl

dnl For the time being, require a command-line arg to build with SMP 
dnl support on linux platforms
AC_ARG_ENABLE(smp_linux,
	      [  --enable-smp-linux     build exec, libs with SMP-parallel capability for Linux platforms (default is disabled)],
	      [with_smp_linux=$enableval],
	      [with_smp_linux='no'])

dnl Since building the libs for all the formats and DX stuff takes so long
dnl this allows you to disable it while developing/extending dx.
AC_ARG_ENABLE(ddx,
	      [  --enable-ddx            build exec, libs for ddx compatibility (default is disabled)],
	      [with_ddx=$enableval],
	      [with_ddx='no'])

dnl Some users will prefer a set of accelerators much more like that of the native
dnl applications on Windows and the MacOS. Required for some limitations on some
dnl keyboards.
AC_ARG_ENABLE(new-keylayout,
	      [  --enable-new-keylayout  build user interface with different accelerators (default is disabled)],
	      [with_new_keylayout=$enableval],
	      [with_new_keylayout='no'])


dnl Since building the libs for all the formats and DX stuff takes so long
dnl this allows you to disable it while developing/extending dx.
AC_ARG_ENABLE(buildlibdx,
	      [  --enable-buildlibdx     enable building libDX.a (default is yes)],
	      [with_buildlibdx=$enableval],
	      [with_buildlibdx='yes'])

dnl This allows you to disable installing the dx html documentation.
AC_ARG_ENABLE(installhtml,
	      [  --enable-installhtml    enable install of html docs (default is yes)],
	      [with_installhtml=$enableval],
	      [with_installhtml='yes'])

dnl Get configuration of ImageMagick

# Disable support for largefiles
AC_ARG_WITH(largefiles,
              [  --without-largefiles    disable support for large (64 bit) file offsets],
              [with_largefiles=$withval],
              [with_largefiles='yes'])

# Enable HDF.
AC_ARG_WITH(hdf,
	      [  --with-hdf              enable HDF (default is yes)],
	      [with_hdf=$withval],
	      [with_hdf='yes'])

# Enable TIFF.
AC_ARG_WITH(tiff,
	      [  --with-tiff             enable TIFF (default is yes)],
	      [with_tiff=$withval],
	      [with_tiff='yes'])

AC_ARG_WITH(magick,
	      [  --with-magick           enable ImageMagick (default is yes)],
	      [with_magick=$withval],
	      [with_magick='yes'])

AC_ARG_WITH(statmagick,
	      [  --with-statmagick       enable Link to Static ImageMagick (default is no)],
	      [with_smagick=$withval],
	      [with_smagick='no'])

# Enable CDF.
AC_ARG_WITH(cdf,
	      [  --with-cdf              enable CDF (default is yes)],
	      [with_cdf=$withval],
	      [with_cdf='yes'])

# Enable NetCDF.
AC_ARG_WITH(netcdf,
	      [  --with-netcdf           enable NetCDF (default is yes)],
	      [with_netcdf=$withval],
	      [with_netcdf='yes'])

# Enable 64-bit arenas, if supported
AC_ARG_WITH(large-arenas,
              [  --with-large-arenas     enable 64-bit arenas, when available (default is no)],
              [with_large_arenas=$withval],
              [with_large_arenas='no'])

AC_ARG_WITH(javadx,
	      [  --with-javadx           enable JAVADX (default is yes)],
	      [with_javadx=$withval],
	      [with_javadx='yes'])

AC_ARG_WITH(jni-path,
	      [  --with-jni-path         java jni include search path (default none)],
	      [with_jni_path=$withval],
	      [with_jni_path=''])
if test "$with_jni_path" = "yes"
then
  with_jni_path=''
fi

AC_ARG_WITH(java40jar-path,
	      [  --with-java40jar-path   set path to Netscape's java40.jar (default none)],
	      [with_java40jar_path=$withval],
	      [with_java40jar_path=''])
if test "$with_java40jar_path" != "yes" && test -z "$with_java40jar_path"
then
  with_java40jar_path=''
fi

AC_ARG_WITH(cosmojar-path,
	      [  --with-cosmojar-path    set path to Cosmo's npcosmop211.jar (default none)],
	      [with_cosmojar_path=$withval],
	      [with_cosmojar_path=''])
if test "$with_cosmojar_path" != "yes" && test -z "$with_cosmojar_path"
then
  with_cosmojar_path=''
fi

# Looking for rsh and path
AC_ARG_WITH(rsh,
        [  --with-rsh=PATH         Specify path to remote shell program ],
        [
                rsh_path=$withval
        ],
        [
if test "$ARCH" = "intelnt" ; then
	rsh_path="rsh"
else
                AC_PATH_PROG(rsh_path, rsh)
fi
        ]
)
AC_DEFINE_UNQUOTED(RSH, "$rsh_path", [Path to the system's remote shell.])

AC_ARG_WITH(bsh,
        [  --with-bsh=PATH         Specify path to system bourne shell program ],
        [
                if test "x$withval" != "$no" ; then
                        bsh_path=$withval
                fi
        ],
        [
                AC_PATH_PROG(bsh_path, sh)
        ]
)
AC_DEFINE_UNQUOTED(BSH, "$bsh_path", [Path to the system's Bourne shell.])

dnl End of command line options configuration

dnl Start DX configuration
AC_LANG([C++])
AC_CHECK_HEADERS(stdlib.h)
AC_LANG([C])

#
# Check for large file support
#
LIB_LF=''
if test "$with_largefiles" = 'yes'
then
   AC_MSG_CHECKING(whether large file support needs explicit enabling)
   result='no'
   getconf='getconf LFS_CFLAGS'
   if ($getconf) >/dev/null 2>&1
   then
      CPPFLAGS="$CPPFLAGS "`$getconf`
      result='yes'
   fi
   getconf='getconf LFS_LDFLAGS'
   if ($getconf) >/dev/null 2>&1
   then
      LDFLAGS="$LDFLAGS "`$getconf`
      result='yes'
   fi
   getconf='getconf LFS_LIBS'
   if ($getconf) >/dev/null 2>&1
   then
      LIB_LF=`$getconf`
      LIBS="$LIB_LF $LIBS"
      result='yes'
   fi
   AC_MSG_RESULT($result)
fi
AC_SUBST(LIB_LF)

DX_ARCHITECTURE
DX_ARCH_SPECIFIC

if test "$MKDEP" = "" ; then
    MKDEP='$(CC) -M -MG $(DEFS) $(INCLUDES) $(CPPFLAGS) $(CFLAGS)'
fi
	
if test "$ARCH" = "intelnt" ; then
    INSTALL_BIN_PROGRAM="dx"
    INSTALL_BIN_SCRIPT=""
else
    INSTALL_BIN_PROGRAM=""
    INSTALL_BIN_SCRIPT="dx"
fi

AC_PROG_INSTALL

if test -z "$LN_S" ; then
    AC_PROG_LN_S
fi

dnl Checks for programs.
dnl AC_PROG_RANLIB
AC_PROG_CXX
if test -n "$CXXCPP" ; then
    ac_cv_prog_CXXCPP=$CXXCPP
fi

AC_PROG_CXXCPP
AC_PROG_CC
AC_PROG_CPP
DX_CYGWIN
DX_EXEEXT
if test -z "$EXEEXT" ; then
	DOT_EXE_EXT="";
	else
	DOT_EXE_EXT=".$EXEEXT";
fi
DX_OBJEXT

if test "$ARCH" = "intelnt" ; then
	AC_CHECK_HEADERS(windows.h)
else
	ac_cv_header_windows_h="no"
fi

# On SGI, detect the ABI we're compiling for (handles all methods that
#   can be used to set the ABI target).
if test "$ARCH" = "sgi" ; then
    AC_MSG_CHECKING(for the target SGI ABI ...)
AC_COMPILE_IFELSE(
  AC_LANG_PROGRAM([[]], [[
   #if defined(sgi) && _MIPS_SIM == _ABI64
   #else
   xyzz
   #endif
   ]]
  ),
  [AC_MSG_RESULT("-64")
   AC_DEFINE(SGI_ABI, -64, [SGI specific ABI])
  ],
  [ AC_COMPILE_IFELSE(
     AC_LANG_PROGRAM([[]], [[
      #if defined(sgi) && _MIPS_SIM == _ABIO32
      #else
      xyzz
      #endif
      ]]
     ), 
   [AC_MSG_RESULT("-32")
    AC_DEFINE(SGI_ABI,-32)], 
   [AC_MSG_RESULT("-n32")
    AC_DEFINE(SGI_ABI,-n32)
   ])
  ]
)
fi

# For SGI, set DXABI based on SGI_ABI
if test "$ARCH" = "sgi" ; then
    case "$SGI_ABI" in
      -64)  DXABI=" $SGI_ABI -L/usr/lib64";;
      -32)  DXABI=" $SGI_ABI -L/usr/lib";;
      -n32) DXABI=" $SGI_ABI -L/usr/lib32";;
    esac
fi

#
# Enable 64-bit arenas if requested and supported
#
AC_MSG_CHECKING(whether to enable large arenas ...)
if test "$with_large_arenas" != 'no' -a \
   "$ARCH" = "sgi" -a "$SGI_ABI" = "-64"; then
  AC_DEFINE(ENABLE_LARGE_ARENAS, 1, [Define to 1 if you need large arenas on SGI])
  AC_MSG_RESULT("yes")
else
  AC_MSG_RESULT("no")
fi

# Code to enable JAVADX to be built

if test -r C:\ ; then
        DIRSEP=";"
else
        DIRSEP=":"
fi

if test "$with_javadx" != 'no'; then
  failed=0;
  passed=0;
  DX_PROG_JAVAC(passed=`expr $passed + 1`,failed=`expr $failed + 1`)
  DX_PROG_JAR(passed=`expr $passed + 1`,failed=`expr $failed + 1`)
  DX_PROG_JAVAH(passed=`expr $passed + 1`,failed=`expr $failed + 1`)
  DX_PATH_JAVA

dnl The following determines if we have the necessary compilers, etc. then look to
dnl see if we can find the jni headers. First look to see if a configure flag sent
dnl this in. If not, look to see if an environment varaible was set. If not, 
dnl use the javac -verbose code to try and determine where they may be.

  if test $failed -eq 0 ; then
    if test -z "$with_jni_path" ; then
      if test -z "$JNIPATH" ; then
        DX_FIND_JNI(passed=`expr $passed + 1`,failed=`expr $failed + 1`)
      else
        DX_FIND_JNI_WITH_PATH($JNIPATH,passed=`expr $passed + 1`,failed=`expr $failed + 1`)
      fi
    else
      DX_FIND_JNI_WITH_PATH($with_jni_path,passed=`expr $passed + 1`,failed=`expr $failed + 1`)
    fi
  fi

  if test $failed -gt 0; then
    with_javadx='no'
    AC_MSG_WARN(JavaDX will be skipped during compilation due to limitations.)
  else
    with_javadx='yes'
    JAVABRANCH=java
    DX_FIND_JDK_CLASSES
    DX_JAVA_CLASSPATH="$ac_cv_jdk_classes"
    if test -z "$with_java40jar_path" ; then
      if test -z "$JAVA40JAR" ; then
        DX_FIND_DEFAULT_NETSCAPE
      else
        DX_FIND_NETSCAPE($JAVA40JAR)
      fi
    else
      DX_FIND_NETSCAPE($with_java40jar_path)
    fi
    if test -z "$with_cosmojar_path" ; then
      if test -z "$COSMOJAR" ; then
        DX_FIND_DEFAULT_COSMO
      else
        DX_FIND_COSMO($COSMOJAR)
      fi
    else
      DX_FIND_COSMO($with_cosmojar_path)
    fi
    WRL_CLASSPATH="$ac_cv_class_netscape"
    if test -n "$ac_cv_class_cosmo" ; then
      WRL_CLASSPATH="$ac_cv_class_netscape$DIRSEP$ac_cv_class_cosmo"
    fi

dnl Fix up the full CLASSPATH to be used with the JAVA compiles.
    TMP_CLASSPATH=./
    if test ! "$CLASSPATH" = "" ; then
	TMP_CLASSPATH=$CLASSPATH$DIRSEP$TMP_CLASSPATH
    fi
    if test ! "$COSMOJAR" = "" ; then
	TMP_CLASSPATH=$COSMOJAR$DIRSEP$TMP_CLASSPATH
    fi
    if test ! "$WRL_CLASSPATH" = "" ; then
	TMP_CLASSPATH=$WRL_CLASSPATH$DIRSEP$TMP_CLASSPATH
    fi
    if test ! "$DX_JAVA_CLASSPATH" = "" ; then
	TMP_CLASSPATH=$DX_JAVA_CLASSPATH$DIRSEP$TMP_CLASSPATH
    fi
    DX_JAVA_CLASSPATH=$TMP_CLASSPATH
    AC_MSG_NOTICE([ DX_JAVA_CLASSPATH :: $DX_JAVA_CLASSPATH ])
  fi

fi
# end of JAVADX configure

AC_LANG([C])

dnl Checks for header files.
AC_HEADER_DIRENT
AC_HEADER_STDC
AC_HEADER_SYS_WAIT

dnl Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_MODE_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_HEADER_TIME
AC_STRUCT_TM

if test "$ARCH" = "intelnt" -o "$ARCH" = "cygwin" ; then
    ANYDX=AnyDX.dll
elif test "$ARCH" = "macos" ; then
    ANYDX=libAnyDX.jnilib
else
    ANYDX=libAnyDX.so
fi

dnl AC_LANG([C])

dnl DX_CYGWIN_MOUNTS
DX_SET_RTL_FLAGS
AC_CHECK_LIB(pthread, pthread_getspecific)

dnl Check of X related files and set up include directories

AC_PATH_X
if test "$x_includes" != "" ; then
    CFLAGS="$CFLAGS -I$x_includes"
    CXXFLAGS="$CXXFLAGS -I$x_includes"
    CPPFLAGS="$CPPFLAGS -I$x_includes"
fi
if test "$x_libraries" != "" ; then
    LIBS="-L$x_libraries $LIBS"
fi

AC_CHECK_LIB(ICE, IceConnectionNumber)
AC_CHECK_LIB(SM, SmcSetProperties)
AC_CHECK_LIB(xlibcon, printf)
AC_CHECK_LIB(xlib, XCreateWindow)
AC_CHECK_LIB(xt, XtVaGetValues)
AC_CHECK_LIB(xmstatxt, XtVaGetValues)
AC_CHECK_LIB(xmstatic, XmStringFree)
AC_CHECK_LIB(xm, XmStringFree)
AC_CHECK_LIB(hclglx, glXGetConfig)
AC_CHECK_LIB(hclglu, gluBuild2DMipmaps)
AC_CHECK_LIB(glwstati, glAccum)

AC_CHECK_LIB(X11, XCreateGC)
AC_CHECK_LIB(Xt, XtMalloc)
AC_CHECK_LIB(Xext, XextAddDisplay)
AC_CHECK_LIB(Xmu, XmuAddInitializer)
AC_CHECK_LIB(m, sqrt)

DX_PATH_XM
if test "$xm_includes" != "" ; then
    CFLAGS="$CFLAGS -I$xm_includes"
    CXXFLAGS="$CXXFLAGS -I$xm_includes"
    CPPFLAGS="$CPPFLAGS -I$xm_includes"
fi
if test "$xm_libraries" != "" ; then
    LIBS="-L$xm_libraries $LIBS"
fi

AC_CHECK_LIB(GL, glXGetConfig)
AC_CHECK_LIB(GLU, gluBuild2DMipmaps)

AC_CHECK_LIB(Xp, XpSetImageResolution)
AC_CHECK_LIB(Xm, XmGetDestination)
AC_TRY_LINK_FUNC(_XmMapKeyEvents, 
	AC_DEFINE(HAVE_XMMAPKEYEVENTS, 1,
		[Define to 1 if your Motif can use _XmMapKeyEvents]))
AC_TRY_LINK_FUNC(HCLXmInit, AC_DEFINE(HAVE_HCLXMINIT, 1, 
	[Define to 1 if Windows has XmInit fnct]))
AC_TRY_LINK_FUNC(HCLXtInit, AC_DEFINE(HAVE_HCLXTINIT, 1, 
	[Define to 1 if Windows has XtInit fnct]))

dnl want to put the Xmu tests in before here!!
DX_X_LINK_LIBS="$LIBS"

dnl The following is set up to check for the old LessTif libraries
dnl
AC_LINK_IFELSE([AC_LANG_PROGRAM([[
#include <Xm/FormP.h>
]], [[
XmFormConstraintPart *p;
p->att[0].type;
]])],
  [],
  [
    AC_DEFINE_UNQUOTED(OLD_LESSTIF, 1, [Define to 1 if Using old version of lesstif])
    AC_MSG_NOTICE([using old version of  LESSTIF])
])

if test "$ARCH" = "solaris" ; then
    OLDLIBS=$LIBS
    LIBS="-L/usr/openwin/lib $LIBS"
    AC_CHECK_LIB(dga, dga_draw_devfd, 
    [
      LIBS="$LIBS -ldga"
      AC_CHECK_LIB(Xmu, XmuAddInitializer, LIBS="$LIBS -lXmu")
    ], LIBS=$OLDLIBS, -lX11)
fi

dnl A couple libs for cygwin
AC_CHECK_LIB(REXP, regcomp)
DX_CHECK_TGMATH

dnl If buildlibdx off then pass this down to the Makefile
AM_CONDITIONAL(WITH_BUILDLIBDX, test $with_buildlibdx = 'yes') 
dnl If install html off then pass this to its Makefile
AM_CONDITIONAL(WITH_INSTALLHTML, test $with_installhtml = 'yes') 

dnl Check for ImageMagick Libraries

dnl Checks for libraries.
AC_CHECK_LIB(g++, main)
AC_CHECK_LIB(dl, main)

dnl Set the CXXFLAG for new keylayout
if test "$with_new_keylayout" != 'no'
then
    AC_DEFINE_UNQUOTED(DX_NEW_KEYLAYOUT, 1, 
	[Define to 1 if you want the new accelerator keys.])
fi

dnl Leave the TIFF check for now incase we want to rewrite the ReadImage
dnl module to use these routines at a later date. 

#
# Check for TIFF
#
have_tiff='no'
LIB_TIFF=''
if test "$with_tiff" != 'no'
then
    AS_MESSAGE([checking for TIFF support ......])
    failed=0;
    passed=0;
    AC_CHECK_HEADER(tiff.h,passed=`expr $passed + 1`,failed=`expr $failed + 1`)
    AC_CHECK_HEADER(tiffio.h,passed=`expr $passed + 1`,failed=`expr $failed + 1`)
    AC_CHECK_LIB(tiff,TIFFOpen,passed=`expr $passed + 1`,failed=`expr $failed + 1`, [ -lm ] )
    AC_MSG_CHECKING(if TIFF package is complete)
    if test $passed -gt 0
    then
    if test $failed -gt 0
    then
	AC_MSG_RESULT(no -- some components failed test)
	have_tiff='no (failed tests)'
    else
	LIB_TIFF='-ltiff'
	LIBS="$LIB_TIFF $LIBS"
	FMT_LIBS="$FMT_LIBS libtiff.a"
	AC_DEFINE(HAVE_LIBTIFF, 1, [Define to 1 if you have the TIFF libraries])
	AC_MSG_RESULT(yes)
	have_tiff='yes'
    fi
    else
    AC_MSG_RESULT(no)
    fi
fi

#
# Check for Xpm
#
have_xpm='no'
LIB_XPM=''
AS_MESSAGE([checking for XPM support ......])
failed=0;
passed=0;
AC_CHECK_HEADERS(xpm.h X11/xpm.h)
if test "$ac_cv_header_xpm_h" = 'yes' -o "$ac_cv_header_X11_xpm_h" = 'yes'
then
    passed=`expr $passed + 1`
else
    failed=`expr $failed + 1`
fi
AC_CHECK_LIB(Xpm,XpmReadFileToPixmap,passed=`expr $passed + 1`,failed=`expr $failed + 1`, )
AC_MSG_CHECKING(if XPM package is complete)
if test $passed -gt 0
then
    if test $failed -gt 0
    then
	AC_MSG_RESULT(no -- some components failed test)
	have_xpm='no (failed tests)'
    else
	LIB_XPM='-lXpm'
	LIBS="$LIB_XPM $LIBS"
	AC_DEFINE(HAVE_LIBXPM, 1, [Define to 1 if you have the xpm libraries])
	AC_MSG_RESULT(yes)
	have_xpm='yes'
    fi
else
AC_MSG_RESULT(no)
fi


#
# Check for HDF
#
have_hdf='no'
LIB_HDF=''
if test "$with_hdf" != 'no'
then
    AS_MESSAGE([checking for HDF components ......])
    failed=0;
    passed=0;
    AC_CHECK_HEADERS(dfsd.h hdf/dfsd.h)
    if test "$ac_cv_header_dfsd_h" = 'yes' -o "$ac_cv_header_hdf_dfsd_h" = 'yes'
    then
    	passed=`expr $passed + 1`
    else
    	failed=`expr $failed + 1`
    fi
if test "$ARCH" = "intelnt" ; then
    AC_CHECK_LIB(hd421m,Hopen,passed=`expr $passed + 1`,failed=`expr $failed + 1`,[ hm421m.lib ])
    AC_MSG_CHECKING(if HDF package is complete)
    if test $passed -gt 0
    then
    if test $failed -gt 0
    then
	AC_MSG_RESULT(no -- some components failed test)
        have_hdf='no (failed tests)'
    else
	LIB_HDF='-lhm421m'
	LIBS="$LIB_HDF $LIBS"
	FMT_LIBS="$FMT_LIBS hm421m.lib"
	AC_DEFINE(HAVE_LIBDF, 1, [Define to 1 if you have the HDF libraries])
	AC_MSG_RESULT(yes)
	have_hdf='yes'
    fi
    else
    AC_MSG_RESULT(no)
    fi
dnl -- check for unix
else
    AC_CHECK_LIB(df,Hopen,passed=`expr $passed + 1`,failed=`expr $failed + 1`,[ -lz -ljpeg ])
    AC_MSG_CHECKING(if HDF package is complete)
    if test $passed -gt 0
    then
    if test $failed -gt 0
    then
	AC_MSG_RESULT(no -- some components failed test)
        have_hdf='no (failed tests)'
    else
	LIB_HDF='-ldf'
	LIBS="$LIB_HDF $LIBS"
	FMT_LIBS="$FMT_LIBS libdf.a"
	AC_DEFINE(HAVE_LIBDF, 1, [Define to 1 if you have the HDF libraries])
	AC_MSG_RESULT(yes)
	have_hdf='yes'
    fi
    else
    AC_MSG_RESULT(no)
    fi
fi
fi


#
# Check for CDF
#
have_cdf='no'
LIB_CDF=''
if test "$with_cdf" != 'no'
then
    AS_MESSAGE([checking for CDF support ......])
    failed=0;
    passed=0;
    AC_CHECK_HEADER(cdf.h,passed=`expr $passed + 1`,failed=`expr $failed + 1`)
if test "$ARCH" = "intelnt" ; then
    AC_CHECK_LIB(dllcdf,Hopen,passed=`expr $passed + 1`,failed=`expr $failed + 1`,)
    AC_MSG_CHECKING(if CDF package is complete)
    if test $passed -gt 0
    then
    if test $failed -gt 0
    then
	AC_MSG_RESULT(no -- some components failed test)
        have_cdf='no (failed tests)'
    else
	LIB_HDF='-ldllcdf'
	LIBS="$LIB_HDF $LIBS"
	FMT_LIBS="$FMT_LIBS dllcdf.lib"
	AC_DEFINE(HAVE_LIBCDF, 1, [Define to 1 if you have the CDF libraries])
	AC_MSG_RESULT(yes)
	have_cdf='yes'
    fi
    else
    AC_MSG_RESULT(no)
    fi
dnl -- check for unix
else
    AC_CHECK_LIB(cdf,CDFlib,passed=`expr $passed + 1`,failed=`expr $failed + 1`,)
    AC_MSG_CHECKING(if CDF package is complete)
    if test $passed -gt 0
    then
    if test $failed -gt 0
    then
	AC_MSG_RESULT(no -- some components failed test)
        have_cdf='no (failed tests)'
    else
	LIB_CDF='-lcdf'
	LIBS="$LIB_CDF $LIBS"
	FMT_LIBS="$FMT_LIBS libcdf.a"
	AC_DEFINE(HAVE_LIBCDF, 1, [Define to 1 if you have the CDF libraries])
	AC_MSG_RESULT(yes)
        have_cdf='yes'
    fi
    else
        AC_MSG_RESULT(no)
    fi
fi
fi


#
# Check for NETCDF
#
have_netcdf='no'
LIB_NETCDF=''
if test "$with_netcdf" != 'no'
then
    AS_MESSAGE([checking for NETCDF support ......])
    failed=0;
    passed=0;
    AC_CHECK_HEADER(netcdf.h,passed=`expr $passed + 1`,failed=`expr $failed + 1`)
    AC_CHECK_LIB(netcdf,nc_copy_att,passed=`expr $passed + 1`,failed=`expr $failed + 1`,)
    AC_MSG_CHECKING(if NETCDF package is complete)
    if test $passed -gt 0
    then
    if test $failed -gt 0
    then
	AC_MSG_RESULT(no -- some components failed test)
        have_netcdf='no (failed tests)'
    else
	LIB_NETCDF='-lnetcdf'
	LIBS="$LIB_NETCDF $LIBS"
	FMT_LIBS="$FMT_LIBS libnetcdf.a"
	AC_DEFINE(HAVE_LIBNETCDF, 1, [Define to 1 if you have the NetCDF libraries])
	AC_MSG_RESULT(yes)
        have_netcdf='yes'
    fi
    else
        AC_MSG_RESULT(no)
    fi
fi


#
# Check for IMAGEMAGICK
#
have_magick='no'
LIB_MAGICK=''
if test "$with_magick" != 'no'
then
    AS_MESSAGE([checking for ImageMagick support ......])
    AC_CHECK_PROG(HAVE_MC, Magick-config, yes, no)

    failed=0;
    passed=0;
    AC_CHECK_HEADER([magick/api.h],[passed=`expr $passed + 1`],[failed=`expr $failed + 1`])
if test "$ARCH" = "intelnt" ; then
    AC_CHECK_LIB(CORE_RL_magick_,Hopen,passed=`expr $passed + 1`,failed=`expr $failed + 1`,)
    AC_MSG_CHECKING(if ImageMagick package is complete)
    if test $passed -gt 0
    then
    if test $failed -gt 0
    then
	AC_MSG_RESULT(no -- some components failed test)
        have_magick='no (failed tests)'
    else
	if test "$with_smagick" != 'no'
	then
	    LIB_MAGICK='-lCORE_RL_magick_ -lCORE_RL_bzlib_ -lCORE_RL_coders_ -lCORE_RL_filters_ -lCORE_RL_jbig_ -lCORE_RL_jbig_ -lCORE_RL_jp2_ -lCORE_RL_jpeg_ -lCORE_RL_lcms_ -lCORE_RL_libxml_ -lCORE_RL_png_ -lCORE_RL_tiff_ -lCORE_RL_ttf_ -lCORE_RL_wand_ -lCORE_RL_wmf_ -lCORE_RL_zlib_'
	else
	    LIB_MAGICK='-lCORE_RL_magick_'
	fi
	LIBS="$LIB_MAGICK $LIBS"
	FMT_LIBS="$FMT_LIBS CORE_RL_magick_.lib"
	AC_DEFINE(HAVE_LIBMAGICK, 1, [Define to 1 if you have the ImageMagick libraries])
	AC_MSG_RESULT(yes)
	have_magick='yes'
    fi
    else
    AC_MSG_RESULT(no)
    fi
dnl -- check for unix
else

dnl The ImageMagick configuration can be collected with an ImageMagick
dnl routine. This allows us to simplify the configuration of all the
dnl format configuration libraries.

 if test "$HAVE_MC" = "yes" ; then
    LIB_MAGICK="`Magick-config --ldflags` `Magick-config --libs`"
    AC_CHECK_LIB( Magick,GetImageInfo, passed=`expr $passed + 1`, failed=`expr $failed + 1`, [ $LIB_MAGICK ])
    AC_MSG_CHECKING(if ImageMagick package is complete)
    if test $passed -gt 0
    then
    if test $failed -gt 0
    then
	AC_MSG_RESULT(no -- some components failed test)
        have_magick='no (failed tests)'
    else
dnl	CFLAGS="`Magick-config --cflags` $CFLAGS"
	CPPFLAGS="`Magick-config --cppflags` $CPPFLAGS"
	LIB_MAGICK="$LIBS `Magick-config --ldflags` `Magick-config --libs`"
dnl UNIQUIFY will remove duplicate library tokens leaving the last instance.
	AC_UTILS_UNIQUIFY($LIB_MAGICK, LIBS)
	FMT_LIBS="$FMT_LIBS libMagick.a"
	AC_DEFINE(HAVE_LIBMAGICK, 1, [Define to 1 if you have the ImageMagick libraries])
	AC_MSG_RESULT(yes)
        have_magick='yes'
    fi
    else
        AC_MSG_RESULT(no)
    fi
  else
	AC_MSG_RESULT(no - Magick-config not found)
  fi
fi
fi

AC_LANG([C++])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[]], [[int a = true;]])],
  [AC_DEFINE(CXX_HAS_TRUE, 1, [Define to 1 if c++ compiler defines true])],
  [AC_DEFINE(CXX_HAS_TRUE, 0)])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[]], [[int a = false;]])],
  [AC_DEFINE(CXX_HAS_FALSE, 1, [Define to 1 if c++ compiler defines false])],
  [AC_DEFINE(CXX_HAS_FALSE, 0)])
AC_CHECK_FUNCS(getdtablesize)

AC_LANG([C])

AC_CHECK_FUNCS(getcwd gethostname gettimeofday mkdir mkfifo mktime \
	putenv re_comp regcmp select socket strcspn strdup strerror \
	strspn strstr strtod strtol strtoul uname trunc \
        _Errno spawnvp _spawnvp regcomp sysmp sysconf \
        pipe _pipe _popen popen _pclose pclose random rand srandom \
	shmat getopt srand opendir _findfirst strrstr strcasecmp \
	stricmp getpid _getpid unlink _unlink getlogin isatty _isatty \
	setsockopt isnan finite )

AC_MSG_CHECKING(if RAND_MAX defined)
AC_COMPILE_IFELSE(
   AC_LANG_PROGRAM([[#include <stdlib.h>]], 
                  [[ int a = RAND_MAX; ]]), 
  [AC_DEFINE(HAVE_RAND_MAX, 1, 
                  [Define to 1 if RAND_MAX is in stdlib.h])
   AC_MSG_RESULT(yes)],
  [ AC_MSG_RESULT(no) ]
)

AC_MSG_CHECKING(if signal.h defines SIGBUS)
AC_EGREP_CPP(xyzzy, 
[
#include <signal.h>
#if defined(SIGBUS)
xyzzy
#endif
], 
  [AC_DEFINE(HAVE_SIGBUS, 1, [Define to 1 if signal defines SIGBUS])
   AC_MSG_RESULT(yes)],
  [AC_MSG_RESULT(no)]
)

AC_MSG_CHECKING(if signal.h defines SIGKILL)
AC_EGREP_CPP(xyzzy, 
[
#include <signal.h>
#if defined(SIGKILL)
xyzzy
#endif
], 
  [AC_DEFINE(HAVE_SIGKILL, 1, [Define to 1 if signal defines SIGKILL])
   AC_MSG_RESULT(yes)],
  [AC_MSG_RESULT(no)])

AC_MSG_CHECKING(if signal.h defines SIGDANGER)
AC_EGREP_CPP(xyzzy, 
[
#include <signal.h>
#if defined(SIGDANGER)
xyzzy
#endif
], 
  [AC_DEFINE(HAVE_SIGDANGER, 1, [Define to 1 if signal defines SIGDANGER])
   AC_MSG_RESULT(yes)],
  [AC_MSG_RESULT(no)])

AC_MSG_CHECKING(if signal.h defines SIGPIPE)
AC_EGREP_CPP(xyzzy, 
[
#include <signal.h>
#if defined(SIGPIPE)
xyzzy
#endif
], 
  [AC_DEFINE(HAVE_SIGPIPE, 1, [Define to 1 if signal defines SIGPIPE])
   AC_MSG_RESULT(yes)],
  [AC_MSG_RESULT(no)])

AC_MSG_CHECKING(if signal.h defines SIGQUIT)
AC_EGREP_CPP(xyzzy, 
[
#include <signal.h>
#if defined(SIGQUIT)
xyzzy
#endif
], 
  [AC_DEFINE(HAVE_SIGQUIT, 1, [Define to 1 if signal defines SIGQUIT])
   AC_MSG_RESULT(yes)],
  [AC_MSG_RESULT(no)])

AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[]], [[extern char *getenv(const char *);]])],
  [AC_DEFINE_UNQUOTED(GETENV_ARG, const char *, getenv argument requires const)],
  [AC_DEFINE_UNQUOTED(GETENV_ARG, char *)])

AC_LINK_IFELSE([AC_LANG_PROGRAM([[
#include <sys/systemcfg.h>
]], [[
int n = _system_configuration.ncpus;
]])],[
AC_DEFINE_UNQUOTED(HAVE_SYSCONFIG_NCPUS, 1, 
	[Define to 1 if you have systemcfg to check for  ncpus])
],[])

dnl Check lexer. Look for flex first--if using lex, may be problem
dnl with duplicate yylineno linking errors.
AC_PROG_LEX
if test "$LEX" = "flex" ; then
    AC_DEFINE(USING_FLEX, 1, [Define to 1 if using flex])
    LEX="flex -l"
fi
dnl Check if lexer adds a global variable yylineno
DX_LEX_YYLINENO

dnl Check yaccer.  Don't use standard AQC_PROG_YACC... We're going
dnl to create an -D as well as the make variable
AC_CHECK_PROG(yakker, bison, "bison", "yacc")
if test "$yakker" = "bison" ; then
    AC_DEFINE(USING_BISON, 1, [Define to 1 if using bison])
    YACC="bison -y"
else
    YACC=yacc
fi
dnl We are including the bison'd files; thus always using bison
AC_DEFINE(USING_BISON, 1, [Define to 1 if using bison])


dnl AC_LANG([C])

dnl Just need to check if we have them.

DX_CHECK_HEADERS( \
  DPS/XDPSlib.h DPS/dpsXclient.h DPS/psops.h GL/gl.h GL/glx.h Mrm/MrmPublic.h \
  X11/Composite.h X11/CompositeP.h X11/Constraint.h X11/CoreP.h X11/Intrinsic.h \
  X11/IntrinsicI.h X11/IntrinsicP.h X11/Protocols.h X11/Shell.h X11/StringDefs.h \
  X11/X.h X11/XHPlib.h X11/Xatom.h X11/Xlib.h X11/XlibXtra.h \
  X11/Xos.h X11/Xutil.h X11/cursorfont.h X11/keysym.h X11/keysymdef.h Xm/ArrowB.h \
  Xm/ArrowBG.h Xm/AtomMgr.h Xm/BulletinB.h Xm/BulletinBP.h Xm/CascadeB.h \
  Xm/CascadeBG.h Xm/Command.h Xm/CutPaste.h Xm/DialogS.h Xm/DragC.h Xm/DragDrop.h \
  Xm/DrawingA.h Xm/DrawingAP.h Xm/DrawnB.h Xm/FileSB.h Xm/Form.h Xm/FormP.h \
  Xm/Frame.h Xm/FrameP.h Xm/GadgetP.h Xm/Label.h Xm/LabelG.h Xm/LabelGP.h \
  Xm/LabelP.h Xm/List.h Xm/MainW.h Xm/ManagerP.h Xm/MenuShell.h Xm/MessageB.h \
  Xm/MwmUtil.h Xm/PanedW.h Xm/PrimitiveP.h Xm/Protocols.h Xm/PushB.h Xm/PushBG.h \
  Xm/PushBP.h Xm/RepType.h Xm/RowColumn.h Xm/Scale.h Xm/ScrollBar.h Xm/ScrolledW.h \
  Xm/ScrolledWP.h Xm/SelectioB.h Xm/SeparatoG.h Xm/SeparatoGP.h Xm/Separator.h \
  Xm/Text.h Xm/TextF.h Xm/ToggleB.h Xm/ToggleBG.h Xm/Xm.h Xm/XmP.h Xm/XmStrDefs.h \
  gl.h gl/device.h gl/gl.h invent.h iop/afb.h iop/mov.h iop/pfs.h license.h \
  linux/kernel.h linux/sys.h math.h mingw32/dir.h mon.h net/if_arp.h net/route.h \
  os2.h setjmp.h starbase.c.h stddef.h stdio.h synch.h sys/access.h \
  sys/ipc.h sys/m88kbcs.h sys/mman.h sys/mode.h sys/pstat.h sys/resource.h \
  sys/svs.h sys/sysconfig.h sys/syslimits.h sys/sysmacros.h ulocks.h x11/xlibxtra.h \
  xgl/xgl.h)

dnl Need these to define themselves in dxconfig.h

AC_CHECK_HEADERS(arpa/inet.h CC/osfcn.h conio.h crypt.h ctype.h cygwin/socket.h)
AC_CHECK_HEADERS(sys/socket.h dlfcn.h errno.h fcntl.h get.h io.h limits.h malloc.h)
AC_CHECK_HEADERS(netdb.h netinet/in.h process.h pwd.h signal.h string.h)
AC_CHECK_HEADERS(strings.h sys/bsd_types.h sys/errno.h sys/file.h sys/filio.h)
AC_CHECK_HEADERS(sys/ioctl.h sys/ldr.h sys/m_wait.h sys/param.h sys/select.h)
AC_CHECK_HEADERS(sys/shm.h sys/signal.h sys/stat.h sys/sysmp.h)
AC_CHECK_HEADERS(sys/systemcfg.h sys/systeminfo.h sys/timeb.h sys/times.h sys/time.h)
AC_CHECK_HEADERS(sys/types.h sys/un.h sys/utsname.h time.h)
AC_CHECK_HEADERS(types.h unistd.h values.h wait.h)
AC_CHECK_HEADERS([X11/Xmu/Editres.h], [], [], 
[#if HAVE_X11_INTRINSIC_H
   #include <X11/Intrinsic.h>
#endif 
])


#
# Check for XINERAMA
#
have_xinerama='no'
LIB_XINERAMA=''
AS_MESSAGE([checking for XINERAMA support ......])
failed=0;
passed=0;
AC_CHECK_HEADERS([X11/extensions/Xinerama.h], [passed=`expr $passed + 1`],
	[failed=`expr $failed + 1`],
[#if HAVE_X11_INTRINSIC_H
   #include <X11/Intrinsic.h>
#endif 
])
AC_CHECK_LIB([Xinerama],[XineramaQueryExtension],[passed=`expr $passed + 1`],
	[failed=`expr $failed + 1`], [-lXt])
    	
AC_MSG_CHECKING(if Xinerama package is complete)
if test $passed -gt 0
	then
	if test $failed -gt 0
		then
			AC_MSG_RESULT(no -- some components failed test)
        		have_xinerama='no (failed tests)'
    	else
			LIB_XINERAMA='-lXinerama'
			LIBS="$LIB_XINERAMA $LIBS"
			AC_DEFINE(HAVE_XINERAMA, 1, [Define to 1 if you have the Xinerama libraries])
			AC_MSG_RESULT(yes)
        	have_xinerama='yes'
    fi
else
    AC_MSG_RESULT(no)
fi


dnl The following is needed to add sys/socket.h to the default headers typically
dnl included for checking net/if.h. Right now, this is the way it has to be done
dnl in Autoconf 2.57, but they are working on some kind of fix to be able to 
dnl include the default headers as well as whatever else is needed in a future 
dnl version of Autoconf.
AC_CHECK_HEADERS([net/if.h],[],[], 
 [#include <stdio.h>
  #if HAVE_SYS_TYPES_H
  # include <sys/types.h>
  #endif
  #if HAVE_SYS_STAT_H
  # include <sys/stat.h>
  #endif
  #if STDC_HEADERS
  # include <stdlib.h>
  # include <stddef.h>
  #else
  # if HAVE_STDLIB_H
  #  include <stdlib.h>
  # endif
  #endif
  #if HAVE_STRING_H
  # if !STDC_HEADERS && HAVE_MEMORY_H
  #  include <memory.h>
  # endif
  # include <string.h>
  #endif
  #if HAVE_STRINGS_H
  # include <strings.h>
  #endif
  #if HAVE_INTTYPES_H
  # include <inttypes.h>
  #else
  # if HAVE_STDINT_H
  #  include <stdint.h>
  # endif
  #endif
  #if HAVE_UNISTD_H
  # include <unistd.h>
  #endif
  #if HAVE_SYS_SOCKET_H
  # include <sys/socket.h>
  #endif
 ])

AC_MSG_CHECKING(for O_RDONLY)
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#if defined(HAVE_FCNTL_H)
#include <fcntl.h>
#endif
]], [[
int i = O_RDONLY;
]])],[
AC_MSG_RESULT([yes])
],[
AC_MSG_RESULT([no])
AC_DEFINE_UNQUOTED(O_RDONLY, _O_RDONLY, [O_RDONLY is defined somewhere])
])

if test "$ARCH" = "intelnt" ; then
    AC_CHECK_HEADERS([winbase.h wingdi.h winioctl.h winsock.h], [], [], 
	[ #if HAVE_WINDOWS_H
	  #include <windows.h>
	  #endif
])
fi

DX_CHECK_STAT

AC_LANG([C++])

dnl Need these to define themselves for C++

AC_CHECK_HEADERS(CC/libc.h direct.h dirent.h iostream iostream.h regex.h)
AC_CHECK_HEADERS(stream stream.h)
AC_CHECK_HEADERS(sstream strstream.h strstrea.h sysent.h)
AC_CHECK_HEADERS(fstream fstream.h)
AC_CHECK_HEADERS([regexp.h], [], [], 
[  #define INIT      const char *sp = "";
   #define GETC()    (*sp++)
   #define PEEKC()   (*sp)
   #define UNGETC(c)  (--sp)
   #define RETURN(pointer) return(pointer);
   #define ERROR(val)  /**/
])

AC_CHECK_LIB(socket, socket)
AC_CHECK_LIB(nsl, inet_addr)
AC_CHECK_LIB(gen, regex)
AC_CHECK_LIB(regex, regcomp)
AC_CHECK_LIB(REXP, regcomp)

DX_STREAM_O2

dnl AC_LANG([C++])

AC_LINK_IFELSE([AC_LANG_PROGRAM([[
#include <unistd.h>
]], [[
char buf[32];
gethostname(buf, 32);
]])],[AC_DEFINE_UNQUOTED(REQUIRES_GETHOSTNAME_DECLARATION, 0, 
	Define to 1 if gethostname needs declared)],[AC_DEFINE_UNQUOTED(REQUIRES_GETHOSTNAME_DECLARATION, 1)])

AC_LINK_IFELSE([AC_LANG_PROGRAM([[
#include <strings.h>
]], [[
char buf[32];
bzero(buf, (size_t)32);
]])],
  [AC_DEFINE_UNQUOTED(REQUIRES_BZERO_DECLARATION, 0, Define to 1 if bzero needs declared)],
  [AC_DEFINE_UNQUOTED(REQUIRES_BZERO_DECLARATION, 1)])

dnl The following DX_HEADER... check changes the ext to .C so must be C++
dnl AC_LANG([C++])

DX_HEADER_HAS_SYMBOL(math.h, M_PI)
DX_HEADER_HAS_SYMBOL(math.h, M_SQRT2)
DX_HEADER_HAS_SYMBOL(sys/stat.h, S_ISDIR)
DX_HEADER_HAS_SYMBOL(sys/mode.h, S_ISDIR)
DX_HEADER_HAS_SYMBOL(sys/sysmp.h, sysmp)

AC_LANG([C])

dnl Checks for library functions.
AC_FUNC_MEMCMP
AC_FUNC_SETVBUF_REVERSED
AC_TYPE_SIGNAL
AC_FUNC_FORK([])
AC_FUNC_VPRINTF

DX_CHECK_TYPE(uint, unsigned int)
DX_CHECK_TYPE(byte, signed char)
DX_CHECK_TYPE(ubyte, unsigned char)
DX_CHECK_TYPE(short, short)
DX_CHECK_TYPE(ushort, unsigned short)
DX_CHECK_TYPE(ulong, unsigned long)
DX_CHECK_TYPE(int8, char)
DX_CHECK_TYPE(uint8, unsigned char)
DX_CHECK_TYPE(int16, short)
DX_CHECK_TYPE(uint16, unsigned short)
DX_CHECK_TYPE(int32, int)
DX_CHECK_TYPE(uint32, unsigned int)
DX_CHECK_TYPE(int64, long)
DX_CHECK_TYPE(float64, double)
DX_CHECK_TYPE(uint64, unsigned long)

if test "$ARCH" != "intelnt" ; then
    DX_CHECK_TYPE(__int64, long long)
    DX_CHECK_TYPE(float32, float)
fi

AC_C_BIGENDIAN

AC_LANG([C++])

AC_MSG_CHECKING([for signal argument list in C++])
AC_LINK_IFELSE([AC_LANG_PROGRAM([[
#include <signal.h>
extern "C" void foo(int bar, ...){}
]], [[
signal(SIGBUS, foo);
]])],[
AC_MSG_RESULT([alternate])
AC_DEFINE_UNQUOTED(ALTERNATE_CXX_SIGNAL, 1, 
	[Define to 1 to use alternate signal argument list in C++])
],[
AC_MSG_RESULT([standard])
])

AC_LANG([C])

DX_CHECK_SELECT_ARG
DX_CHECK_SOCK_LENGTH_TYPE

DX_NEEDS_STDCXX

dnl since we statically link the hw-dependent renderer dxexec links need GL
DX_GL_LINK_LIBS=" -lGL "

LOCALMAKE="include local.mk"

if test "$with_ddx" = 'yes' ; then
    AC_DEFINE(DDX, 1, [Define to 1 to enable distributed DX])
fi

if test "$with_smp_linux" = 'yes' ; then
    AC_DEFINE(ENABLE_SMP_LINUX, 1, [Define to 1 to enable SMP Linux version])
else
    AC_DEFINE(ENABLE_SMP_LINUX, 0)
fi

AC_CONFIG_FILES([ Makefile \
	bin/Makefile \
	bin/dx
	bin/mdf2c \
	doc/Makefile \
	fonts/Makefile \
	help/Makefile \
	html/Makefile \
	html/images/Makefile \
	html/pages/Makefile \
	include/Makefile \
	include/dx/Makefile \
	lib/Makefile \
	man/Makefile \
	man/catl/Makefile \
	man/manl/Makefile \
	src/Makefile \
	src/exec/Makefile \
	src/exec/dpexec/Makefile \
	src/exec/dpexec/local.mk \
	src/exec/dxexec/Makefile \
	src/exec/dxmods/Makefile \
	src/exec/dxmods/local.mk \
	src/exec/hwrender/Makefile \
	src/exec/hwrender/gl/Makefile \
	src/exec/hwrender/opengl/Makefile \
	src/exec/hwrender/starbase/Makefile \
	src/exec/hwrender/xgl/Makefile  \
	src/exec/libdx/Makefile \
	src/exec/libdx/local.mk \
	src/misc/Makefile \
	src/misc/arch.mak \
	src/uipp/Makefile \
	src/uipp/base/Makefile \
	src/uipp/dxl/Makefile \
	src/uipp/dxui/Makefile \
	src/uipp/dxuilib/Makefile \
	src/uipp/dxuilib/local.mk \
	src/uipp/java/Makefile \
	src/uipp/java/dx/Makefile \
	src/uipp/java/dx/protocol/Makefile \
	src/uipp/java/dx/protocol/server/Makefile \
	src/uipp/java/dx/runtime/Makefile \
	src/uipp/java/layout/Makefile \
	src/uipp/java/server/Makefile \
	src/uipp/java/server/macros/Makefile \
	src/uipp/java/server/dxserver.paths \
	src/uipp/java/server/startserver \
	src/uipp/java/dx/client/Makefile \
	src/uipp/java/dx/net/Makefile \
	src/uipp/mb/Makefile \
	src/uipp/prompter/Makefile \
	src/uipp/startup/Makefile \
	src/uipp/tutor/Makefile \
	src/uipp/ui/Makefile \
	src/uipp/widgets/Makefile 
	windows/Makefile ])
AC_OUTPUT
