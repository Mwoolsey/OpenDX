/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>



#include "hwDeclarations.h"
#include "hwClipped.h"
#include "hwDebug.h"

static Error _deleteHwClipped(Pointer arg);

typedef struct clippedS {
    Point         *points;
    Vector        *normals;
    int           count;
    dxObject      object;
} clippedT, *clippedP;


clippedO
_dxf_newHwClipped(int cnt, dxObject obj)
{
  clippedP      ret;
  clippedO      reto;

  ENTRY(("_newClipped(%d)", cnt));

  ret = (clippedP)DXAllocateZero(sizeof(clippedT));
  if (! ret)
    goto error;

  if (obj)
      ret->object = (dxObject)DXReference(obj);
  else
      ret->object = NULL;

  ret->count = cnt;

  ret->points = (Point *)DXAllocateZero(sizeof(Point) * cnt);
  if (! ret->points)
    goto error;

  ret->normals = (Point *)DXAllocateZero(sizeof(Vector) * cnt);
  if (! ret->normals)
    goto error;

  reto = (clippedO)_dxf_newHwObject(HW_CLASS_CLIPPED, 
			(Pointer)ret, _deleteHwClipped);
  if (! reto)
    goto error;

  EXIT(("reto = 0x%x", reto));
  return reto;

 error:
  if(ret) {
    if(ret->points) DXFree(ret->points);
    if(ret->normals) DXFree(ret->normals);
    DXFree(ret);
  }

  EXIT(("ERROR"));
  DXErrorReturn (ERROR_NO_MEMORY,"");
}

clippedO
_dxf_getHwClippedInfo(clippedO co,
    Point **points, Vector **normals, int *count, dxObject *object)
{
  clippedP cp;

  if (! co)
    return NULL;
  
  cp = (clippedP)_dxf_getHwObjectData((dxObject)co);
  if (! cp)
    return NULL;
  
  if (points)
    *points = cp->points;
  
  if (normals)
    *normals = cp->normals;
  
  if (count)
    *count = cp->count;
  
  if (object)
    *object = cp->object;
  
  return co;
}

static Error
_deleteHwClipped(Pointer arg)
{
  clippedP      cp = (clippedP) arg;

  ENTRY(("_dxf_deleteClipped(0x%x)", arg));

  if (!cp) {
    EXIT(("ERROR: cp == NULL"));
    return ERROR;
  }

  DXDelete(cp->object);
  DXFree(cp->points);
  DXFree(cp->normals);
  DXFree(cp);

  EXIT(("OK"));
  return OK;
}


