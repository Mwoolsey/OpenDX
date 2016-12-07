# this file is used to record the build configuration 
# so that we can switch architectures and
# so that we do not have to repeat configuration for samples 
DXCC = gcc
DXCFLAGS = -g -O2 -D_GNU_SOURCE
DXEXECLINKLIBS = -lnsl -lXinerama -ldl -lXm -lGLU -lGL -lm -lXext -lXt -lX11 -lSM -lICE -lpthread  
DXEXECLINKFLAGS = 
DXRUNTIMELOADFLAGS = 
DXFMTLIBS = 
DXLDFLAGS =  
OBJEXT = o
EXEEXT = 
DXABI = 
DX_X_LINK_LIBS = -lXm -lGLU -lGL -lm -lXext -lXt -lX11 -lSM -lICE -lpthread  
DX_GL_LINK_LIBS =  -lGL 
DOT_EXE_EXT = 
# the var ARCH may change to DXARCH due to namespace
DXARCH = linux
JINC = 
DX_JAVA_CLASSPATH = 
DX_RTL_CFLAGS =  -D_GNU_SOURCE -Dlinux
DX_RTL_LDFLAGS = -fPIC -shared
DX_RTL_DXENTRY =  -eDXEntry
DX_RTL_IMPORTS = 
DX_RTL_SYSLIBS = 
SHARED_LINK = $(CC)
DX_OUTBOARD_LIBS = 
