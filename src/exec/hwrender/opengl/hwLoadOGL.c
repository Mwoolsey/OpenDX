/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../hwDeclarations.h"

/*---------------------------------------------------------------------------*\
$Header: /src/master/dx/src/exec/hwrender/opengl/hwLoadOGL.c,v 1.8 2003/07/11 05:50:39 davidt Exp $
\*---------------------------------------------------------------------------*/
#include <stdio.h>
#include <math.h>

#if defined(HAVE_SYS_PARAM_H)
#include <sys/param.h>
#endif

#if defined(HAVE_SYS_STAT_H)
#include <sys/stat.h>
#endif

#if defined(HAVE_DLFCN_H)
#include <dlfcn.h>
#endif

#if defined(HAVE_SYS_LDR_H)
#include <sys/ldr.h>
#endif

#if defined(HAVE_STRING_H)
#include <string.h>
#endif

#if defined(HAVE_GET_H)
#include <get.h>
#endif
#if defined(HAVE_STDLIB_H)
#include <stdlib.h>
#endif

#include "../hwDebug.h"

#define NAMELEN 257
#define PATHLEN 513

#ifndef STANDALONE

extern int isOpenGL;

#if defined(DX_NATIVE_WINDOWS)
typedef tdmPortHandleP (*PF_PORTHANDLE)(char *);
#else
typedef tdmPortHandleP (*PF_PORTHANDLE)(Display *, char *);
#endif

#if !defined(DEBUG) && defined(RTLOAD)
/* We do not do demand loading if we are in DEBUG mode */

#if defined (alphax) || defined(sgi) || defined(solaris)

static PF_PORTHANDLE
_tryLoad(char* path, char* file, char** error)
{
  char buff[NAMELEN+PATHLEN];	/* tmp file path/name buffer */
  struct stat statbuf;		/* for stat() call           */
  extern main();		/* used to bind HW module    */
  void *HW_handle;
  void *entry_point;
  
  ENTRY(("_tryLoad(\"%s\", \"%s\", 0x%x)",path, file, error));
  
  /* Try to avoid automounter problems by stat of load file */
  if (file[0] == '/')
  {
      strcpy(buff, file);
  }
  else
  {
      strcpy(buff,path);
      strcat(buff,"/");
      strcat(buff,file);
  }

  if(stat(buff,&statbuf))
  {
     /* Hardware unavailable: load module '%s/%s' not found */
     *error = "#5180";
     EXIT(("load module not found: %s/%s",path,file));
     return NULL;
  }
  
  if((HW_handle=dlopen(buff,RTLD_LAZY))==NULL)
  { 
     /* Hardware unavailable: unable to load module '%s/%s' */
     *error = "#5182";
     EXIT(("unable to load module: %s/%s",path,file));
     return NULL;
  }
  
  if(strstr(buff, "OGL")) /* SGI Port Shares Originally AlphaX code */
  {                       /* See which Lib We Actually Want         */
     if(!(entry_point = dlsym(HW_handle,"_dxfInitPortHandleOGL")))
     {
       /* Hardware unavailable: could not bind symbols for '%s/%s' */
       *error = "#5184";
       EXIT(("unable to bind symbols for: %s/%s",path,file));
       return NULL;
     }
  }
  else
  {
     if(!(entry_point = dlsym(HW_handle,"_dxfInitPortHandle")))
     {
       /* Hardware unavailable: could not bind symbols for '%s/%s' */
       *error = "#5184";
       EXIT(("unable to bind symbols for: %s/%s",path,file));
       return NULL;
     }
  }

  EXIT(("entry_point: 0x%x)",entry_point));
#if defined(DX_NATIVE_WINDOWS)
  return  (tdmPortHandleP(*)(char *)) entry_point;
#else
  return  (tdmPortHandleP(*)(Display *, char *)) entry_point;
#endif
}

#elif defined(ibm6000)

/* IBM port */
static PF_PORTHANDLE
_tryLoad(char* path, char* file, char** error)
{
  char buff[NAMELEN+PATHLEN];	/* tmp file path/name buffer */
  struct stat statbuf;		/* for stat() call           */
  extern main();		/* used to bind HW module    */
  void *HW_handle;
  int (*entry_point)();
  
  ENTRY(("_tryLoad(\"%s\", \"%s\", 0x%x)",path, file, error));

  /* Try to avoid automounter problems by stat of load file */
  strcpy(buff,path);
  strcat(buff,"/");
  strcat(buff,file);

  if(stat(buff,&statbuf))
  {
     /* Hardware unavailable: load module '%s/%s' not found */
     *error = "#5180";
     EXIT(("load module not found: %s/%s",path,file));
     return NULL;
  }
  
  if((entry_point=load(file,L_NOAUTODEFER,path))==NULL)
  {
     /* Hardware unavailable: unable to load module '%s/%s' */
     *error = "#5182";
     EXIT(("unable to load module: %s/%s",path,file));
     return NULL;
  }
  
  if(loadbind(0,main,entry_point))
  {
    /* Hardware unavailable: could not bind symbols for '%s/%s' */
    *error = "#5184";
    EXIT(("unable to bind symbols for: %s/%s",path,file));
    return NULL;
  }

  EXIT(("entry_point: 0x%x)",entry_point));
  return  (tdmPortHandleP(*)(Display *, char *)) entry_point;
}

#else

/* Windows */

#include <sys/types.h>
#include <sys/stat.h>

static PF_PORTHANDLE
_tryLoad(char* path, char* file, char** error)
{
  char buff[NAMELEN+PATHLEN];	/* tmp file path/name buffer */
  char buf1[NAMELEN+PATHLEN];	/* tmp file path/name buffer */
  HINSTANCE handle;
  FARPROC entry;
  int i;
  struct _stat statbuf;
  
  ENTRY(("_tryLoad(\"%s\", \"%s\", 0x%x)",path, file, error));
  
  /* Try to avoid automounter problems by stat of load file */
  if (file[0] == '/' || (file[1] == ':' && file[2] == '/'))
      strcpy(buff, file);
  else
  {
      strcpy(buff,path);
      strcat(buff,"/");
      strcat(buff,file);
  }
  
  for (i = 0; i < strlen(buff); i++)
	  if (buff[i] == '/')
	      buff[i] = '\\';

  if (stricmp(buff+(strlen(buff)-4), ".dll"))
	  strcat(buff, ".dll");

  strcpy(buf1, buff);
 
  if (_stat(buf1, &statbuf))
  {
	DXMessage("unable to find: %s\n", buff);
	*error = "unable to find loadable hardware library";
	return NULL;
  }

  handle = LoadLibrary(buff);
  if(! handle)
  {
	 char *errbuf;	 
	DXMessage("unable to load: %s\n", buff);
     /* Hardware unavailable: unable to load  odule '%s/%s' */
	 FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		 NULL, GetLastError(), MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
		 (LPTSTR)&errbuf, 0, NULL);

     *error = errbuf;
     EXIT(("unable to load module: %s", buff));
     return NULL;
  }
  
  entry = GetProcAddress(handle, "_dxfInitPortHandleOGL");
  if (! entry)
  {
    /* Hardware unavailable: could not bind symbols for '%s/%s' */
    *error = "#5184";
    EXIT(("unable to bind symbols for: %s/%s",buff));
    return NULL;
  }
#if defined(DX_NATIVE_WINDOWS)
  return  (tdmPortHandleP(*)(char *)) entry;
#else
  return  (tdmPortHandleP(*)(Display *, char *)) entry;
#endif
}
#endif


/* _HWload()
 * This does the real work in demand loading either the DX.o  
 * executable (ibm6000) or the DXhw.sl shared lib (hp700) so
 * we can load the GL, OpenGL or Starbase dependent stuff on the fly
 * and simply report HW render not available if it fails.
 */ 
#if defined(DX_NATIVE_WINDOWS)
int _dxfHWload(tdmPortHandleP (**initPP)(char*))
#else
int _dxfHWload(tdmPortHandleP (**initPP)(Display*, char*), Display *dpy)
#endif
{
/*
 * NOTE!!! XXX
 * If this code is compiled DEBUG and linked using the optimized flags to
 * makedx, you will seg fault. This is because if we are in debug we assume the
 * device dependent code to be staticly linked with the dxexec image, however,
 * the optimized flags expects the code below to demand load the device 
 * dependent stuff.
 */
  char HWname[NAMELEN];		/* name of HW module for load()*/
  char HWpath[PATHLEN];
  char buff[NAMELEN+PATHLEN];   /* tmp file path/name buffer */
  char *error,*error2;

  
  ENTRY(("_dxfHWload(0x%x)",initPP));

  *initPP = NULL;

  if(getenv("DXEXECROOT"))
  {
     strcpy(HWpath,getenv("DXEXECROOT"));
     strcat(HWpath,"/bin_");
     strcat(HWpath,DXD_ARCHNAME);
  }
  else
  if(getenv("DXROOT"))
  {
     strcpy(HWpath,getenv("DXROOT"));
     strcat(HWpath,"/bin_");
     strcat(HWpath,DXD_ARCHNAME);
  }
  else
  {
     strcpy(HWpath,"/usr/lpp/dx/bin_");
     strcat(HWpath,DXD_ARCHNAME);
  }

  if(getenv("DXHWMOD")) /* Force Lib */
  {
     strcpy(HWname,getenv("DXHWMOD"));

     *initPP = _tryLoad(HWpath, HWname, &error);

     if (strstr(HWname, "OGL"))
	 isOpenGL = 1;
     else
	 isOpenGL = 0;

     if(! *initPP)
     {
        DXSetError(ERROR_BAD_PARAMETER, error, HWpath, HWname);
        goto error;
     }
  }
  else 
  {
#if !defined(DX_NATIVE_WINDOWS)
     int glx,  Extra, Major, valid;

     valid = XQueryExtension(dpy, "GLX", &Major, &Extra, &Extra);

     glx = valid && Major > 0;
#else
     int glx = 1; /* Extension not needed for native windows */
#endif

     if(glx)
     { 
	 isOpenGL = 1;
#if defined(solaris)
	 *initPP = _tryLoad(HWpath, "DXhwddOGL.so", &error);
#else
	 *initPP = _tryLoad(HWpath, "DXhwddOGL.o", &error);
#endif
     }

#if defined(sgi) || defined(ibm6000) || defined(solaris)
     if (! *initPP)
     {
         isOpenGL = 0;
#if defined(solaris)
	 *initPP = _tryLoad(HWpath, "DXhwdd.so", &error2);
#else
	 *initPP = _tryLoad(HWpath, "DXhwdd.o", &error2);
#endif
     }
#endif

     if (! *initPP)
     {
#if defined(sgi) || defined(ibm6000)
 	DXSetError(ERROR_BAD_PARAMETER,
		"unable to load either %s/DXhwddOGL.o or %s/DXhwdd.o",
		HWpath, HWpath);
#elif defined (solaris)
 	DXSetError(ERROR_BAD_PARAMETER,
		"unable to load either %s/DXhwddOGL.so or %s/DXhwdd.so",
		HWpath, HWpath);

#else
 	DXSetError(ERROR_BAD_PARAMETER,
		"unable to load %s/DXhwddOGL.o",
		HWpath);
#endif
       goto error;
    }
  }

  EXIT(("OK"));
  return OK;

 error:

  EXIT(("ERROR"));
  return ERROR;
}


#else

#if defined(DX_NATIVE_WINDOWS)
tdmPortHandleP _dxfInitPortHandleOGL(char *hostname) ;
tdmPortHandleP _dxfInitPortHandle(char *hostname) ;
#else
tdmPortHandleP _dxfInitPortHandleOGL(Display *dpy, char *hostname) ;
tdmPortHandleP _dxfInitPortHandle(Display *dpy, char *hostname) ;
#endif

#if defined(DX_NATIVE_WINDOWS)
int _dxfHWload(tdmPortHandleP  (**initPP)(char*))
#else
int _dxfHWload(tdmPortHandleP  (**initPP)(Display*, char*), Display *dpy)
#endif
{
  ENTRY(("_dxfHWload(0x%x)",initPP));

  *initPP = _dxfInitPortHandleOGL;
  isOpenGL = 1;
   
  EXIT(("OK"));
  return OK ;
}
#endif
#endif
