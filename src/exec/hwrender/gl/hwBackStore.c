/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


/*---------------------------------------------------------------------------*\
 $Source: /src/master/dx/src/exec/hwrender/gl/hwBackStore.c,v $

\*---------------------------------------------------------------------------*/
/********************************************\
 * Isolates an IBM RS/6000 Optimization Bug *
\********************************************/

#include <gl/gl.h>

#if defined(sgi) || defined(indigo)
#include <get.h>
#endif

#define STRUCTURES_ONLY
#include "hwDeclarations.h"

#include "hwPortGL.h"
#include "hwWindow.h"
#include "hwMemory.h"
#include "hwPortLayer.h"
#include "hwDebug.h"

extern hwFlags _dxf_SERVICES_FLAGS();
extern void _dxf_FREE_PIXEL_ARRAY (void *, void *);
extern void * _dxf_ALLOCATE_PIXEL_ARRAY (void *, int, int);

int _dxf_READ_APPROX_BACKSTORE_GL(void *win, int camw, int camh)
{
  DEFWINDATA(win) ;
  DEFPORT(PORT_HANDLE);
  int status=OK ;

  ENTRY(("_dxf_READ_APPROX_BACKSTORE(0x%x, %d, %d)", win, camw, camh));

  if (SAVE_BUF_SIZE != camw * camh)
    {
      if (SAVE_BUF)
        {
          _dxf_FREE_PIXEL_ARRAY (PORT_CTX, SAVE_BUF) ;
          SAVE_BUF = (void *) NULL ;
        }

      /* allocate a buffer */
      SAVE_BUF_SIZE = camw * camh ;
      if (!(SAVE_BUF = _dxf_ALLOCATE_PIXEL_ARRAY (PORT_CTX, camw, camh)))
        {
          EXIT(("ERROR: no memory"));
          DXErrorReturn (ERROR_NO_MEMORY, "#13790") ;
        }

      /*
       * If we are running against an adapter that does not do
       * readpixels well (i.e. E&S) then simply fill
       * the buffer with black pixels.  Return OK so
       * common layer uses the buffer for the interactors.
       */
      if (_dxf_isFlagsSet(_dxf_SERVICES_FLAGS(),
                          SF_INVALIDATE_BACKSTORE)) {
        bzero(SAVE_BUF,SAVE_BUF_SIZE*sizeof(int));
        EXIT(("OK"));
        return OK ;
      }
    }

  if (!_dxf_isFlagsSet(_dxf_SERVICES_FLAGS(),
                       SF_INVALIDATE_BACKSTORE)) {
    /* resynchronize */
    finish() ;
    XSync (DPY, False) ;
    readsource(SRC_FRONT) ;

    lrectread (0, 0, camw-1, camh-1, SAVE_BUF);
    finish() ;
    XSync (DPY, False) ;
  }

  EXIT(("status = %d",status));
  return status ;
}
