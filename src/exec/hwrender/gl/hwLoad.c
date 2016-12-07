/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


/*---------------------------------------------------------------------------*\
$Header: /src/master/dx/src/exec/hwrender/gl/hwLoad.c,v 1.3 1999/05/10 15:45:36 gda Exp $

\*---------------------------------------------------------------------------*/
#include <stdio.h>
#include <math.h>
#include <sys/param.h>

#if ibm6000
#include <sys/stat.h> 
#include <sys/ldr.h>
#include <string.h>
#endif

#if defined(sgi) 
#include <get.h>
#include <stdlib.h>
#endif

#include "hwDeclarations.h"

#include "hwDebug.h"

#define NAMELEN 257
#define PATHLEN 513

#ifndef STANDALONE

#if defined(ibm6000) && !defined(DEBUG)

static void *
_tryLoad(char* path, char* file, char** error)
{
  char buff[NAMELEN+PATHLEN];	/* tmp file path/name buffer */
  struct stat statbuf;		/* for stat() call       	*/
  int (*HW)();			/* HW entry point from load() call */
  extern main();		/* used to bind HW module     	*/
  
  ENTRY(("_tryLoad(\"%s\", \"%s\", 0x%x)", path, file, error));

  /* Try to avoid automounter problems by stat of load file */
  strcpy(buff,path);
  strcat(buff,"/");
  strcat(buff,file);

  if(stat(buff,&statbuf)) {
    /* Hardware unavailable: load module '%s/%s' not found */
    *error = "#5180";
    EXIT(("can't find load module"));
    return NULL;
  }
  
  if ((HW=load(file,L_NOAUTODEFER,path))==NULL) { 
    /*  Hardware unavailable: unable to load module '%s/%s' */
    *error = "#5182";
    EXIT(("can't load load module"));
    return NULL;
    
  }
  
  if(loadbind(0,main,HW)) {
    /* Hardware unavailable: could not bind symbols for '%s/%s' */
    *error = "#5184";
  }

  EXIT(("HW = 0x%x",HW));
  return HW;
}

/* _HWload()
 * This does the real work in demand loading either the DX.o  
 * executable (ibm6000) or the DXhw.sl shared lib (hp700) so
 * we can load the GL, OpenGL or Starbase dependent stuff on the fly
 * and simply report HW render not available if it fails.
 */ 

int
_dxfHWload(tdmPortHandleP  (**initPP)(Display*, char*), Display *dsp)
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
  char buff[NAMELEN+PATHLEN];	/* tmp file path/name buffer */
  char *error,*error2;
  static void*	save_init = NULL;
  
  ENTRY(("_dxfHWload(0x%x)", initPP));
  
  if(save_init) {
    *initPP = save_init;
    EXIT(("Using save_init"));
    return OK;
  }
  
  if (getenv("DXEXECROOT")){
    
    strcpy(HWpath,getenv("DXEXECROOT"));
    strcat(HWpath,"/bin_ibm6000");
    
  } else if (getenv("DXROOT")){
    
    strcpy(HWpath,getenv("DXROOT"));
    strcat(HWpath,"/bin_ibm6000");
    
  } else  {
    
    strcpy(HWpath,"/usr/lpp/dx/bin_ibm6000");
    
  }
  
  if (getenv("DXHWMOD")) {
    strcpy(HWname,getenv("DXHWMOD"));
    if(!(*initPP =  (tdmPortHandleP  (*)(Display*, char*))
	 _tryLoad(HWpath, HWname, &error))) {
      DXSetError(ERROR_BAD_PARAMETER, error, HWpath, HWname);
      goto error;
    }
  } else {
    if(!(*initPP =  (tdmPortHandleP  (*)(Display*, char*))
	 _tryLoad(HWpath, "DXhwdd.o", &error)))
      if(!(*initPP =  (tdmPortHandleP  (*)(Display*, char*))
	   _tryLoad(HWpath, "DXhwddOGL.o", &error2))) {
        if(error == error2) {
          strcpy(buff, "DXhwdd.o or ");
          strcat(buff,HWpath);
          strcat(buff,"/");
          strcat(buff,"DXhwddOGL.o");
	  DXSetError(ERROR_BAD_PARAMETER, error, HWpath, buff);
        }
        else {
   	  DXSetError(ERROR_BAD_PARAMETER, error, HWpath, "DXhwdd.o");
	  DXAddMessage(error2, HWpath, "DXhwddOGL.o");
        }
	goto error;
      }
  }

  save_init = *initPP;

  EXIT(("OK"));
  return OK;

 error:

  EXIT(("ERROR"));
  return ERROR;
}


#else

#ifdef OGL_AVAILABLE
tdmPortHandleP
_dxfInitPortHandleOGL(Display *dpy, char *hostname) ;
#endif

tdmPortHandleP
_dxfInitPortHandle(Display *dpy, char *hostname) ;


int
_dxfHWload(tdmPortHandleP  (**initPP)(Display*, char*), Display *dsp)
{

  ENTRY(("_dxfHWload(0x%x)", initPP));

   /* 
    * Changed this logic. This applies to debug only.
    * If DXHWMOD is set, use OpenGL otherwise use GL.
    */

#ifdef OGL_AVAILABLE
   if (getenv("DXHWMOD")) {
     PRINT(("DXHWMOD = %s, using OpenGL.",getenv("DXHWMOD")));
     *initPP = _dxfInitPortHandleOGL ;
   } else {
     PRINT(("DXHWMOD is unset, using GL."));
     *initPP = _dxfInitPortHandle ;
   }
#else
  PRINT(("OpenGL not available, using GL."));
  *initPP = _dxfInitPortHandle ;
#endif

  EXIT(("OK"));
  return OK ;
}

#endif
#endif
