/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>




#include "hwDeclarations.h"
#include "hwScreen.h"
#include "hwMatrix.h"
#include "hwMemory.h"

#include "hwDebug.h"

static Error _dxf_deleteHwScreen(Pointer ptr);

typedef struct screenS {
  double         mvm[4][4];     /* Stores the concatentated view and modeling
                                 * matrix after updateView.
                                 */
  double         mm[4][4];      /* Stores the modeling matrix after gather.
                                 * (This is overladed, it also stores the
                                 *  projection matrix for the very top level
                                 *  screen object created from the camera)
                                 */
  int            width;
  int            height;
  dxScreen       screen;
  dxObject       subObject;
} screenT;

screenO
_dxf_newHwScreen(dxScreen s, dxObject subObject,
		      float mvm[4][4], float mm[4][4],
		      int width, int height)
{
  screenO	ret;
  screenT	*str;

  ENTRY(("_dxf_newHwScreen(0x%x, 0x%x, 0x%x, 0x%x, %d, %d)",
	 s, subObject, mvm, mm, width, height));
  
  if (!(str = (screenT *)DXAllocateZero(sizeof(screenT)))) {
    EXIT(("No memory"));
    DXErrorReturn(ERROR_NO_MEMORY,"");
  }
  
  str->subObject = NULL;

  ret = (screenO)_dxf_newHwObject(HW_CLASS_SCREEN,
		(Pointer)str, _dxf_deleteHwScreen);

  EXIT(("calling _dxf_setScreenView"));
  return _dxf_setHwScreenView(ret,s,subObject,mvm,mm,width,height);
}

screenO
_dxf_setHwScreenView(screenO ob, dxScreen s, dxObject subObject,
		   float mvm[4][4], float mm[4][4],
		   int width, int height)
{
  screenT *sp;

  if (! ob)
      return NULL;

  sp = (screenT *)_dxf_getHwObjectData(ob);

  ENTRY(("_dxf_setScreenView(0x%x, 0x%x, 0x%x, 0x%x ,0x%x, %d, %d)",
	 sp, s, subObject, mvm, mm, width, height));
  
  if (sp->screen != s)
  {
      if (sp->screen)
      {
	  DXDelete((dxObject)sp->screen);
	  sp->screen = NULL;
      }

      if (s)
      {
	  s = (dxScreen)DXReference((dxObject)s);
	  sp->screen = s;
      }
  }

  sp->width = width;
  sp->height = height;

  if(sp->subObject != subObject)
  {
    if(sp->subObject)
    {
      DXDelete(sp->subObject);
      sp->subObject = NULL;
    }

    if (subObject)
    {
	DXReference(subObject);
	sp->subObject = subObject;
    }
  }

  if (mvm)
    COPYMATRIX((sp->mvm),mvm);

  if (mm)
    COPYMATRIX((sp->mm),mm);

  EXIT(("sp = 0x%x", sp));
  return ob;
}

screenO
_dxf_getHwScreenView(screenO ob, dxScreen* s, dxObject* subObject,
		   float mvm[4][4], float mm[4][4],
		   int* width, int* height)
{
  screenT *sp;

  ENTRY(("_dxf_getScreenView(0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x)",
	 sp, s, subObject, mvm, mm, width, height));
  
  if (! ob)
      return NULL;

  sp = (screenT *)_dxf_getHwObjectData(ob);

  if(s)
    *s = sp->screen;

  if(width)
    *width = sp->width;

  if(height)
    *height = sp->height;

  if(subObject)
    *subObject = sp->subObject;

  if(mvm)
    COPYMATRIX(mvm,(sp->mvm));

  if(mm)
    COPYMATRIX(mm,(sp->mm));

  EXIT(("sp = 0x%x",sp));
  return ob;
}


static Error
_dxf_deleteHwScreen(Pointer ptr)
{
  screenT *sp = (screenT *)ptr;

  ENTRY(("_dxf_deleteHwScreen(0x%x)", sp));

  if(sp)
  {
    if (sp->screen)
      DXDelete((dxObject)sp->screen);

    if(sp->subObject)
      DXDelete((dxObject)sp->subObject);

    DXFree (sp);
  }

  EXIT((""));
  return OK;
}
