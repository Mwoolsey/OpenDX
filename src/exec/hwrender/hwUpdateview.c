/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


#include "hwDeclarations.h"
#include "hwWindow.h"
#include "hwView.h"
#include "hwGather.h"
#include "hwMatrix.h"
#include "hwMemory.h"
#include "hwScreen.h"

#include "hwDebug.h"

#ifndef ABS
#define ABS(x) (x<0.0?-x:x)
#endif

extern clippedO _dxf_getHwClippedInfo(clippedO co, Point **points, 
	Vector **normals, int *count, dxObject *object);

static Error _updateViewRecurse(dxObject object, viewO view, 
				screenO parent);

/*
 * updateView
 *
 * NOTE: This function should do View Dependent and Medium Independent
 * functions. (No medium is available)
 *
 * Add camera dependent lights to view struct
 * Add camera Transform (viewing transform) to all xfields
 *	(Screen objects modify the camera transform)
 *
 * XXXX If no lights found, add default lights to view structure
 * 
*/
Error
  _dxf_updateView(viewO view, void *globals, screenO camScreen)
{
  DEFGLOBALDATA(globals);
  dxObject		object,clipObject;
  int 			flags;
  gatherO 		gather;

  ENTRY(("_dxf_updateView(0x%x, 0x%x, 0x%x)", view, globals, camScreen));
  
  if (! _dxf_getHwViewInfo(view, &flags, &gather, NULL, NULL, NULL, NULL))
      goto error;
    
  if (! _dxf_getHwGatherObject(gather, &object))
      goto error;

  if (! _dxf_getHwGatherClipObject(gather, &clipObject))
      goto error;

  if(!(flags & (CONTAINS_SCREEN))) {
    EXIT(("OK: !(view->flags & (CONTAINS_SCREEN))"));
    return OK;
  }

  if(object)
    if(!( _updateViewRecurse(object,view, camScreen))) 
      goto error;
 
  if(clipObject)
    if(!( _updateViewRecurse(clipObject,view, camScreen))) 
      goto error;

  EXIT(("OK"));
  return OK;

error:

  EXIT(("ERROR"));
  return ERROR;
}

static Error
_updateViewRecurse(dxObject object, viewO view, screenO parent)
{
  dxObject 	subObject;
  int 		i;
  Class		class;

  ENTRY(("_updateViewRecurse(0x%x, 0x%x, 0x%x)",
	 object, view, parent));

     
  if (!(class = DXGetObjectClass(object))) goto error;
  /*
   * XXX check for empty dxObject
   */
  
  switch (class) {
  case CLASS_GROUP:
    for (i=0; (subObject=DXGetEnumeratedMember((Group)object, i, NULL)); i++) {
      /* DXDebug("g", "member %d", i); */
      if (!_updateViewRecurse(subObject, view, parent)) goto error;
    }
    break;
  case CLASS_PRIVATE: /* created from CLASS_FIELD in gather */
    {
      switch(_dxf_getHwClass(object)) {
      case HW_CLASS_CLIPPED:
	{
	    dxObject clipObject;

	    if (! _dxf_getHwClippedInfo((clippedO)object, 
			NULL, NULL, NULL, &clipObject))
		goto error;

	    if (!_updateViewRecurse(clipObject, view, parent))
		goto error;
	    
	}
	break;
      case HW_CLASS_XFIELD:
	break;
      case HW_CLASS_SCREEN:
	{
	  screenO sp = (screenO) object;
	  int	positionUnits,z;
	  int	width,height;
	  dxScreen	s;
	  float	mm[4][4],vm[4][4],pm[4][4],vpm[4][4],tmp[4][4];

	  if(!(_dxf_getHwScreenView(sp,&s,&subObject,NULL,mm,
				  NULL,NULL)))
	    goto error;

	  if(!(_dxf_getHwScreenView(parent,NULL,NULL,vm,pm,
				  &width,&height)))
	    goto error;
	  
	  if(!(DXGetScreenInfo(s,NULL,&positionUnits,&z)))
	    goto error;


	  /*
	   * take 'SCREEN_WORLD' origin from world to view space
	   */
	  COPYMATRIX(tmp,identity);
	  MULTMATRIX(vpm,vm,pm);
	  switch (positionUnits) {
	  case SCREEN_WORLD:
	    {
	      float	matrix[4][4];

	      MULTMATRIX(matrix,mm,vpm);
	      tmp[3][0] = matrix[3][0];
	      tmp[3][1] = matrix[3][1];
	      /*
	       * Adjust for perspective 
	       */
	      if(vpm[3][2]) {
		tmp[3][0] /= matrix[3][3];
		tmp[3][1] /= matrix[3][3];
	      }
	    }
	    break;
	  case SCREEN_VIEWPORT:
	    {
	      /* 
	       * HW viewport is -1 to +1, viewport here is 0 +1, so
	       * scale by 2. and translate by -1.
	       */
	      tmp[3][0] = mm[3][0] * 2. - 1.0;
	      tmp[3][1] = mm[3][1] * 2. - 1.0;
	    }
	    break;
	  case SCREEN_PIXEL:
	    {
	      /* 
	       * HW viewport is -1 to +1, viewport here is 0 to 1 
	       * width (or height),
	       * so adjust the translation by 2./ width (or height) -1.
	       */
	      tmp[3][0] = mm[3][0] * 2. / width  - 1.0;
	      tmp[3][1] = mm[3][1] * 2. / height - 1.0;
	    }
	    break;
	  case SCREEN_STATIONARY:
	    {
	      tmp[3][0] = 0.0;
	      tmp[3][1] = 0.0;
	    }
	    break;
	  }
	  /*
	   * Scale the object from pixel to viewport size pixel is 0, width
	   * HW viewport is -1, +1
	   */
	  tmp[0][0] = tmp[0][0] * 2. / (float)width;
	  tmp[1][1] = tmp[1][1] * 2. / (float)height;
	  /*
	   * Set z scale and translation, we reserve the front 5% and back 5%
	   * of the z depth for screen objects, dividing by 100k  brings a
	   * unit sized object to about the resolution of the z buffer.
	   * The translation brings it to the front or back 5%.
	   */
	  tmp[2][2] = 1./100000.;
	  tmp[3][2] =  z > 0 ? -.99 : (float)z * -.91;

	  /*COPYMATRIX(ident,identity);*/
	  _dxf_setHwScreenView(sp,s,subObject,
			       tmp,NULL,width,height);

	  if (!_updateViewRecurse(subObject, view, sp)) goto error;
	  break;
	}
      }
    }
    break;
  case CLASS_SCREEN:
  case CLASS_FIELD:
      /*
       * should not be any screen or field objects at this point.
       * They were converted to CLASS_PRIVATE objects during the gather step.
       */
    DXErrorGoto (ERROR_INTERNAL, "Invalid Object during update view");
    break;
  case CLASS_XFORM:
    {
      /*
       * should not be any xform objects at this point.
       * They were removed during the gather step.
       */
      goto error;
    }
    break;
  case CLASS_LIGHT:
    break;
  default:
    break;
  }
  
  EXIT(("OK"));
  return OK;

error:

  EXIT(("ERROR"));
  return ERROR;
}


