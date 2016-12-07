/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>



#include "hwDeclarations.h"
#include "hwMaterials.h"
#include "hwDebug.h"


static Error _deleteHwMaterial(Pointer arg);

typedef struct materialS {
    float ambient;              /* coefficient of ambient component */
    float diffuse;              /* coefficient of diffuse component */
    float specular;             /* coefficent of specular component */
    int shininess;              /* exponent of specular component */
} materialT, *materialP;

materialO
_dxf_newHwMaterial(float ka, float kd, float ks, float sp)
{
  materialP      ret;
  materialO      reto;

  ENTRY(("_newMaterial"));

  ret = (materialP)DXAllocateZero(sizeof(materialT));
  if (! ret)
    goto error;

  ret->ambient   = ka;
  ret->diffuse   = kd;
  ret->specular  = ks;
  ret->shininess = sp;

  reto = (materialO)_dxf_newHwObject(HW_CLASS_MATERIAL, 
			(Pointer)ret, _deleteHwMaterial);
  if (! reto)
    goto error;

  EXIT(("reto = 0x%x", reto));
  return reto;

 error:
  if(ret)
    DXFree(ret);

  EXIT(("ERROR"));
  DXErrorReturn (ERROR_NO_MEMORY,"");
}

materialO
_dxf_getHwMaterialInfo(materialO mo,
    float *ka, float *kd, float *ks, float *sp)
{
  materialP mp;

  if (! mo)
    return NULL;
  
  mp = (materialP)_dxf_getHwObjectData((dxObject)mo);
  if (! mp)
    return NULL;
  
  if (ka)
    *ka = mp->ambient;
  
  if (kd)
    *kd = mp->diffuse;
  
  if (ks)
    *ks = mp->specular;
  
  if (sp)
    *sp = mp->shininess;
  
  return mo;
}

materialO
_dxf_setHwMaterialAmbient(materialO mo, float ka)
{
  materialP mp;

  if (! mo)
    return NULL;
  
  mp = (materialP)_dxf_getHwObjectData((dxObject)mo);
  if (! mp)
    return NULL;
  
  mp->ambient = ka;

  return mo;
}
  
materialO
_dxf_setHwMaterialDiffuse(materialO mo, float kd)
{
  materialP mp;

  if (! mo)
    return NULL;
  
  mp = (materialP)_dxf_getHwObjectData((dxObject)mo);
  if (! mp)
    return NULL;
  
  mp->diffuse = kd;

  return mo;
}
  
materialO
_dxf_setHwMaterialSpecular(materialO mo, float ks)
{
  materialP mp;

  if (! mo)
    return NULL;
  
  mp = (materialP)_dxf_getHwObjectData((dxObject)mo);
  if (! mp)
    return NULL;
  
  mp->specular = ks;

  return mo;
}
  
materialO
_dxf_setHwMaterialShininess(materialO mo, float sp)
{
  materialP mp;

  if (! mo)
    return NULL;
  
  mp = (materialP)_dxf_getHwObjectData((dxObject)mo);
  if (! mp)
    return NULL;
  
  mp->shininess = sp;

  return mo;
}
  

static Error
_deleteHwMaterial(Pointer arg)
{
  materialP      mp = (materialP) arg;

  ENTRY(("_dxf_deleteMaterial(0x%x)", arg));

  if (!mp) {
    EXIT(("ERROR: mp == NULL"));
    return ERROR;
  }

  DXFree(mp);

  EXIT(("OK"));
  return OK;
}


