/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/*********************************************************************/
/*                     I.B.M. CONFIENTIAL                           */
/*********************************************************************/

#include <dxconfig.h>


/*---------------------------------------------------------------------------*\
 $Source: /src/master/dx/src/exec/hwrender/xgl/hwLoad.c,v $
\*---------------------------------------------------------------------------*/
#include <stdio.h>
#include <math.h>
#include <sys/param.h>

#include <dlfcn.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <errno.h>
#include <sys/utsname.h>

#include "hwDeclarations.h"


tdmPortHandleP
_dxfInitPortHandle(Display *dpy, char *hostname);


			
/* HWload()
 * This does the real work in demand loading either the DX.o  
 * executable (ibm6000) or the DXhw.sl shared lib (hp700) so
 * we can load the GL or Starbase dependent stuff on the fly
 * and simply report HW render not available if it fails.
 */ 


int
_dxfHWload(tdmPortHandleP  (**initPP)(Display*, char* ), Display *dsp)
{
 
#if !defined(DEBUG)

    char HWname[257],HWpath[513];	/* name of HW module for load()*/
    extern main();			/* used to bind HW module     	*/
    int (*HW)();			/* HW entry point from load() call */
    struct stat statbuf;		/* for stat() call       	*/
    static void *HW_handle;		/* for shl_load() call		*/
    volatile double dummy;		/* for demand load hack see below */
    struct utsname unamebuf;   		/* for uname() call */


#if defined (sun4)
    
    /*
     * XXX demand load hack.  This call is just to insure that infinity is
     * linked into the exec because XGL calls in DXhwdd.so will need it 
     * latter.  Because libm is archive only (no .so), only referenced
     * symbols from it are bound into the exec.  We also can't simply link
     * DXhwdd.so with -lm because it is not PIC and the assert pure-text
     * fails.  If SUN gives us a better solution to prob #1366834 we will
     * fix this.
     */

     dummy=infinity();


     /*
     * If this is the sun4 build of DX, but user is running solaris
     * then fail.
     */
    uname(&unamebuf);

    if (unamebuf.release[0] == '5'){
      DXSetError (ERROR_BAD_PARAMETER,"Hardware rendering not available. ");
      goto error;
    }	 

#endif


    
    if (getenv("DXHWMOD"))
      strcpy(HWname,getenv("DXHWMOD"));
    else
      strcpy(HWname,"DXhwdd.so");
    

     if (getenv("DXEXECROOT")){
	strcpy(HWpath,getenv("DXEXECROOT"));
	strcat(HWpath,"/bin_");
	strcat(HWpath,DXD_ARCHNAME);
	strcat(HWpath,"/");
	strcat(HWpath,HWname);
        if (!stat(HWpath,&statbuf))
	  goto loadlib;
    }

    if (getenv("DXROOT")){
	strcpy(HWpath,getenv("DXROOT"));
	strcat(HWpath,"/bin_");
	strcat(HWpath,DXD_ARCHNAME);
	strcat(HWpath,"/");
	strcat(HWpath,HWname);
	if (!stat(HWpath,&statbuf))
	  goto loadlib;
    }

    strcpy(HWpath,"/usr/lpp/dx/bin_");
    strcat(HWpath,DXD_ARCHNAME);
    strcat(HWpath,"/");
    strcat(HWpath,HWname);
    if (!stat(HWpath,&statbuf))
      goto loadlib;

    DXSetError (ERROR_BAD_PARAMETER,"Hardware rendering not available: "
		"%s not found.",HWpath);
    goto error;
    
  loadlib:
  
    if ((HW_handle=dlopen(HWpath,RTLD_LAZY))==NULL) { 
      DXSetError (ERROR_BAD_PARAMETER,dlerror()); /* FIX THIS */
      /*DXSetError (ERROR_BAD_PARAMETER,"Hardware rendering not available: "
		  "/usr/lib/libxgl.a not found.");  */
      goto error;
    } 
    if ((*initPP=(tdmPortHandleP  (*)())dlsym(HW_handle,"_dxfInitPortHandle"))==(tdmPortHandleP (*)())NULL){
      DXSetError (ERROR_UNEXPECTED,dlerror());
      goto error ;
    }
    return OK ;

  error:

    return ERROR ;
#else

    *initPP = _dxfInitPortHandle;
    return OK;

#endif

}
