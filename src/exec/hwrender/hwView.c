/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>

#if defined(HAVE_STRING_H)
#include <string.h>
#endif

#include "hwDeclarations.h"
#include "hwView.h"
#include "hwMatrix.h"
#include "hwDebug.h"


typedef struct viewS {
  long int      	flags;
  gatherO       	gather;
  translationO          translation;
  float         	pm[4][4];       /* Projection Matrix */
  float         	vm[4][4];       /* View Matrix */
  RGBColor      	background;     /* Carried on DX camera */

} viewT, *viewP;

static Error _deleteHwView(Pointer arg);

viewO
_dxf_newHwView()
{
  viewP      ret;
  viewO      reto;

  ENTRY(("_newHwView"));

  ret = (viewP)DXAllocateZero(sizeof(viewT));
  if (! ret)
    goto error;

  reto = (viewO)_dxf_newHwObject(HW_CLASS_VIEW, 
			(Pointer)ret, _deleteHwView);
  if (! reto)
    goto error;

  EXIT(("reto = 0x%x", reto));
  return reto;

 error:
  if (ret)
    DXFree(ret);

  EXIT(("ERROR"));
  DXErrorReturn (ERROR_NO_MEMORY,"");
}

viewO
_dxf_getHwViewInfo(viewO vo,
    int *flags, gatherO *gather, translationO *translation,
    float *projection, float *view, RGBColor *background)
{
  viewP vp;

  if (! vo)
    return NULL;
  
  vp = (viewP)_dxf_getHwObjectData((dxObject)vo);
  if (! vp)
    return NULL;
  
  if (flags)
    *flags = vp->flags;
  
  if (gather)
    *gather = vp->gather;
  
  if (translation)
    *translation = vp->translation;
  
  if (projection)
    memcpy(projection, &vp->pm, sizeof(vp->pm));

  if (view)
    memcpy(view, &vp->vm, sizeof(vp->vm));

  if (background)
    *background = vp->background;
  
  return vo;
}

viewO
_dxf_setHwViewFlags(viewO vo, int flags)
{
  viewP vp;

  if (! vo)
    return NULL;
  
  vp = (viewP)_dxf_getHwObjectData((dxObject)vo);
  if (! vp)
    return NULL;

  vp->flags = flags;

  return vo;
}

viewO
_dxf_setHwViewGather(viewO vo, gatherO gather)
{
  viewP vp;

  if (! vo)
    return NULL;
  
  vp = (viewP)_dxf_getHwObjectData((dxObject)vo);
  if (! vp)
    return NULL;

  if (vp->gather)
  {
      DXDelete((dxObject)vp->gather);
      vp->gather = NULL;
  }

  if (gather)
      vp->gather = (gatherO)DXReference((dxObject)gather);

  return vo;
}

viewO
_dxf_setHwViewTranslation(viewO vo, translationO translation)
{
  viewP vp;

  if (! vo)
    return NULL;
  
  vp = (viewP)_dxf_getHwObjectData((dxObject)vo);
  if (! vp)
    return NULL;

  if (vp->translation)
  {
      DXDelete((dxObject)vp->translation);
      vp->translation = NULL;
  }

  if (translation)
      vp->translation = (translationO)DXReference((dxObject)translation);

  return vo;
}

viewO
_dxf_setHwViewProjectionMatrix(viewO vo, float projectionMatrix[4][4])
{
  viewP vp;

  if (! vo)
    return NULL;
  
  vp = (viewP)_dxf_getHwObjectData((dxObject)vo);
  if (! vp)
    return NULL;

  COPYMATRIX(vp->pm, projectionMatrix);

  return vo;
}

viewO
_dxf_setHwViewViewMatrix(viewO vo, float viewMatrix[4][4])
{
  viewP vp;

  if (! vo)
    return NULL;
  
  vp = (viewP)_dxf_getHwObjectData((dxObject)vo);
  if (! vp)
    return NULL;

  COPYMATRIX(vp->vm, viewMatrix);

  return vo;
}

viewO
_dxf_setHwViewBackground(viewO vo, RGBColor background)
{
  viewP vp;

  if (! vo)
    return NULL;
  
  vp = (viewP)_dxf_getHwObjectData((dxObject)vo);
  if (! vp)
    return NULL;

  vp->background = background;

  return vo;
}


static Error
_deleteHwView(Pointer arg)
{
  viewP      vp = (viewP) arg;

  ENTRY(("_dxf_deleteView(0x%x)", arg));

  if (!vp) {
    EXIT(("ERROR: vp == NULL"));
    return ERROR;
  }

  if (vp->gather)
      DXDelete((dxObject)vp->gather);
    
  if (vp->translation)
      DXDelete((dxObject)vp->translation);

  DXFree(vp);

  EXIT(("OK"));
  return OK;
}


