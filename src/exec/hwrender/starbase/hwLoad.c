/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


/*---------------------------------------------------------------------------*\
 $Source: /src/master/dx/src/exec/hwrender/starbase/hwLoad.c,v $

\*---------------------------------------------------------------------------*/
#include <stdio.h>
#include <math.h>
#include <sys/param.h>

#if hp700
#include <dl.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <errno.h>
#endif

#include "hwDeclarations.h"

#include "hwDebug.h"

tdmPortHandleP
_dxfInitPortHandle(Display *dpy, char *hostname);

/* HWload()
 * This does the real work in demand loading either the DX.o  
 * executable (ibm6000) or the DXhw.sl shared lib (hp700) so
 * we can load the GL or Starbase dependent stuff on the fly
 * and simply report HW render not available if it fails.
 */ 


int
_dxfHWload(tdmPortHandleP  (**initPP)(Display*, char*), Display *dsp)
{
 
#if !defined(DEBUG)

    char HWname[257],HWpath[513];	/* name of HW module for load()*/
    extern main();			/* used to bind HW module     	*/
    int (*HW)();			/* HW entry point from load() call */
    struct stat statbuf;		/* for stat() call       	*/
    static shl_t HW_handle;		/* for shl_load() call		*/

    ENTRY(("_dxfHWload(0x%x)", initPP));
    
    if (getenv("DXHWMOD"))
      strcpy(HWname,getenv("DXHWMOD"));
    else
      strcpy(HWname,"DXhwdd.sl");
    strcpy(HWpath,"");
    

     if (getenv("DXEXECROOT")){
	strcpy(HWpath,getenv("DXEXECROOT"));
	strcat(HWpath,"/bin_hp700/");
	strcat(HWpath,HWname);
        if (!stat(HWpath,&statbuf))
	  goto loadlib;
    }

    if (getenv("DXROOT")){
	strcpy(HWpath,getenv("DXROOT"));
	strcat(HWpath,"/bin_hp700/");
	strcat(HWpath,HWname);
	if (!stat(HWpath,&statbuf))
	  goto loadlib;
    }

    strcpy(HWpath,"/usr/lpp/dx/bin_hp700/");
    strcat(HWpath,HWname);

    if (!stat(HWpath,&statbuf))
      goto loadlib;

    DXSetError (ERROR_BAD_PARAMETER,"Hardware rendering not available: "
		"%s not found.",HWpath);
    goto error;
    
  loadlib:
  
    if ((HW_handle=shl_load(HWpath,BIND_VERBOSE ,0))==NULL) { 
      DXSetError (ERROR_BAD_PARAMETER,"Hardware rendering not available: "
		  "/usr/lib/libsb.sl not found.");
      goto error;
    } 
    if ((shl_findsym(&HW_handle,"_dxfInitPortHandle",TYPE_PROCEDURE,
		(long *)initPP))==-1) {
      DXSetError (ERROR_UNEXPECTED,"shl_findsym failed");
      goto error ;
    }

    EXIT(("OK"));
    return OK ;

  error:

    EXIT(("ERROR"));
    return ERROR ;
#else
    ENTRY(("_dxfHWload(0x%x)", initPP));

    *initPP = _dxfInitPortHandle;

    EXIT(("OK"));
    return OK;
#endif
}
/* work around for running under HPUX 9.01 and 9.05.  we are currently
 * compiling on HPUX 8.05.  the load-time HP starbase library on this
 * version doesn't reference fabsf() since it doesn't exist in the standard
 * math library.  on later versions of the op system, the HP dynamic lib
 * expects to find the fabsf() function already linked into the app.
 *
 * THIS SHOULD BE REMOVED AS SOON AS WE MOVE THE BUILD TO HPUX 9.xx
 */

/* the real math lib routine */
extern double fabs(double x);

/* our fake fabsf() imitation */
float fabsf(float x) {
   return (float)fabs((double)x);
}
