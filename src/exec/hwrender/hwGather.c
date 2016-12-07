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
#include "hwList.h"
#include "hwClipped.h"
#include "hwMatrix.h"
#include "hwMemory.h"
#include "hwPortLayer.h"

#include "hwGather.h"
#include "hwXfield.h"
#include "hwWindow.h"
#include "hwObjectHash.h"
#include "hwSort.h"

#include "hwDebug.h"

extern  Error _dxf_deleteAttribute(attributeP att);
extern screenO _dxf_newHwScreen(dxScreen s, dxObject subObject,
	float mvm[4][4], float mm[4][4], int width, int height);

static Error _gatherRecurse(dxObject object, dxObject *newObject, 
				gatherO gather, attributeP attributes,
	       			void *globals);

typedef struct clipPlanesS *clipPlaneP;

typedef struct clipPlanesS
{
  int		n;
  Point		*p;
  Vector	*v;
  clipPlaneP 	next;
} clipPlanesT;

typedef struct gatherS
{
  long int	flags;
  listO		lights;
  RGBColor	ambientColor;
  materialO 	lastUsedMaterial;
  dxObject	object;     /* modified copy of original object */
  dxObject     	clipObject; /* modified copy of object doing tranp clip */
  clipPlaneP	clipStack;
} gatherT, *gatherP;

static gatherP
_getHwGatherData(gatherO go)
{
    return (gatherP)_dxf_getHwObjectData((dxObject)go);
}

Error
_dxf_pushHwGatherClipPlanes(gatherO go, int n, Point *p, Vector *v)
{
    int i;
    gatherP gop;
    clipPlaneP clips = NULL;

    if (! go)
	return ERROR;

    gop = _getHwGatherData(go);

    clips = (clipPlaneP)DXAllocateZero(sizeof(clipPlanesT));
    if (! clips)
	goto error;
    
    clips->n = n;

    if (n > 0)
    {
	clips->p = (Point *)DXAllocateZero(n*sizeof(Point));
	if (! clips->p)
	    goto error;

	clips->v = (Point *)DXAllocateZero(n*sizeof(Vector));
	if (! clips->v)
	    goto error;

	for (i = 0; i < n; i++)
	{
	    clips->p[i] = p[i];
	    clips->v[i] = v[i];
	}
    }

    clips->next = gop->clipStack;
    gop->clipStack = clips;

    return OK;

error:
    if (clips)
    {
	DXFree((Pointer)clips->p);
	DXFree((Pointer)clips->v);
	DXFree((Pointer)clips);
    }

    return ERROR;
}

		
Error
_dxf_getHwGatherClipPlanes(gatherO go, int *nClips, Point **p, Vector **v)
{
    gatherP gop;
    clipPlaneP clips;

    *p = NULL;
    *v = NULL;

    if (! go)
	return ERROR;

    gop = _getHwGatherData(go);
    
    *nClips = 0;
    for (clips = gop->clipStack; clips; clips = clips->next)
	*nClips += clips->n;


    if (*nClips)
    {
	int i, j;

	*p = (Point *)DXAllocate((*nClips)*sizeof(Point));
	*v = (Point *)DXAllocate((*nClips)*sizeof(Point));
	if (! *p || ! *v)
	    goto error;

	for (j = 0, clips = gop->clipStack; clips; clips = clips->next)
	    for (i = 0; i < clips->n; i++, j++)
	    {
		(*p)[j] = clips->p[i];
		(*v)[j] = clips->v[i];
	    }
    }

    return OK;

error:

    DXFree((Pointer)*p);
    DXFree((Pointer)*v);
    return ERROR;
}



static Error
__popHwGatherClipPlanes(gatherP gop)
{
    clipPlaneP top;

    if (! gop)
	 return OK;
    
    if (NULL == (top = gop->clipStack))
	return ERROR;

    gop->clipStack = top->next;

    DXFree((Pointer)top->p);
    DXFree((Pointer)top->v);
    DXFree((Pointer)top);

    return OK;
}

		
Error
_dxf_popHwGatherClipPlanes(gatherO go)
{
    gatherP gop;

    if (! go)
	return ERROR;

    gop = _getHwGatherData(go);

    return __popHwGatherClipPlanes(gop);
    

    return OK;
}

		
Error
_dxf_getHwGatherFlags(gatherO go, int *flags)
{
    gatherP gop;

    if (! go)
	return ERROR;

    gop = _getHwGatherData(go);
    
    if (!gop)
	return ERROR;

    if (flags)
        *flags = gop->flags;

    return OK;
}

Error
_dxf_setHwGatherFlags(gatherO go, int flags)
{
    gatherP gop;

    if (! go)
	return ERROR;

    gop = _getHwGatherData(go);
    
    if (!gop)
	return ERROR;

    gop->flags = flags;

    return OK;
}

Error
_dxf_getHwGatherLights(gatherO go, listO *list)
{
    gatherP gop;

    if (! go)
	return ERROR;

    gop = _getHwGatherData(go);
    
    if (!gop)
	return ERROR;

    if (list)
        *list = gop->lights;

    return OK;
}

Error
_dxf_setHwGatherLights(gatherO go, listO list)
{
    gatherP gop;

    if (! go)
	return ERROR;

    gop = _getHwGatherData(go);
    
    if (!gop)
	return ERROR;

    if (gop->lights)
    {
	DXDelete((dxObject)gop->lights);
	gop->lights = NULL;
    }
    
    if (list)
	gop->lights = (listO)DXReference((dxObject)list);

    return OK;
}

Error
_dxf_getHwGatherAmbientColor(gatherO go, RGBColor *color)
{
    gatherP gop;

    if (! go)
	return ERROR;

    gop = _getHwGatherData(go);
    
    if (!gop)
	return ERROR;

    if (color)
        *color = gop->ambientColor;

    return OK;
}

Error
_dxf_setHwGatherAmbientColor(gatherO go, RGBColor color)
{
    gatherP gop;

    if (! go)
	return ERROR;

    gop = _getHwGatherData(go);
    
    if (!gop)
	return ERROR;

    gop->ambientColor = color;

    return OK;
}

Error
_dxf_getHwGatherMaterial(gatherO go, materialO *mat)
{
    gatherP gop;

    if (! go)
	return ERROR;

    gop = _getHwGatherData(go);
    
    if (!gop)
	return ERROR;

    if (mat)
        *mat = gop->lastUsedMaterial;

    return OK;
}

Error
_dxf_setHwGatherMaterial(gatherO go, materialO mat)
{
    gatherP gop;

    if (! go)
	return ERROR;

    gop = _getHwGatherData(go);
    
    if (!gop)
	return ERROR;

    if (gop->lastUsedMaterial)
    {
	DXDelete((dxObject)gop->lastUsedMaterial);
	gop->lastUsedMaterial = NULL;
    }

    DXReference((dxObject)mat);
    gop->lastUsedMaterial = mat;

    return OK;
}

Error
_dxf_getHwGatherObject(gatherO go, dxObject *object)
{
    gatherP gop;

    if (! go)
	return ERROR;

    gop = _getHwGatherData(go);
    
    if (!gop)
	return ERROR;

    if (object)
        *object = gop->object;

    return OK;
}

Error
_dxf_setHwGatherObject(gatherO go, dxObject object)
{
    gatherP gop;

    if (! go)
	return ERROR;

    gop = _getHwGatherData(go);
    
    if (!gop)
	return ERROR;

    if (gop->object)
    {
	DXDelete((dxObject)gop->object);
	gop->object = NULL;
    }

    if (object)
	gop->object = (dxObject)DXReference(object);

    return OK;
}

Error
_dxf_getHwGatherClipObject(gatherO go, dxObject *clipObject)
{
    gatherP gop;

    if (! go)
	return ERROR;

    gop = _getHwGatherData(go);
    
    if (!gop)
	return ERROR;

    if (clipObject)
        *clipObject = gop->clipObject;

    return OK;
}

Error
_dxf_setHwGatherClipObject(gatherO go, dxObject clipObject)
{
    gatherP gop;

    if (! go)
	return ERROR;

    gop = _getHwGatherData(go);
    
    if (!gop)
	return ERROR;

    if (gop->clipObject)
    {
	DXDelete((dxObject)gop->clipObject);
	gop->clipObject = NULL;
    }

    if (clipObject)
	gop->clipObject = (dxObject)DXReference(clipObject);

    return OK;
}

static Error 
_deleteGather(Pointer arg)
{
  gatherP gp = (gatherP)arg;

  ENTRY(("_dxf_deleteGather(0x%x)", arg));
  
  if (gp)
  {
      if (gp->lights)
      {
	  DXDelete((dxObject)gp->lights);
	  gp->lights = NULL;
      }

      if (gp->lastUsedMaterial)
      {
	  DXDelete((dxObject)gp->lastUsedMaterial);
	  gp->lastUsedMaterial = NULL;
      }

      if(gp->object)
      {
	  DXDelete(gp->object);
	  gp->object = NULL;
      }

      if(gp->clipObject)
      {
	  DXDelete(gp->clipObject);
	  gp->clipObject = NULL;
      }

      while(__popHwGatherClipPlanes(gp));

  }
    
  DXFree(gp);
  gp = NULL;

  tdmCheckAllocTable(3);

  EXIT(("OK"));
  return OK;
}

gatherO
_dxf_newHwGather(void)
{
  gatherP	ret;
  gatherO	go;

  ENTRY(("_dxf_newGather()"));
  
  ret = (gatherP)DXAllocateZero(sizeof(gatherT));
  if(!ret)
    goto error;

  ret->lights = _dxf_newList(8, (int(*)(void*))DXDelete);
  if(!ret->lights)
    goto error;

  go = (gatherO)_dxf_newHwObject(HW_CLASS_GATHER,
				(Pointer)ret, _deleteGather);
  if (!go)
    goto error;

  EXIT(("ret = 0x%x", ret));
  return go;

 error:
  if(ret) {
    DXFree(ret);
    ret = NULL;
  }

  EXIT(("ERROR"));
  DXErrorReturn(ERROR_NO_MEMORY,"");
}

/*
 * Gather
 *
 * NOTE: This function should only do View Independent and Medium Indepenant
 * functions. (No camera or medium is available)
 *
 * Gathers together camera independent lights 
 * Gathers a single clip object for transparent fields.
 * Creates a new object heirarchy with:
 *	fields replaced with xfields (which contain accumulated model xforms)
 *	SCREEN_WORLD screen objects replaced with SCREEN_VIEWPORT screen objects
 *	modeling transforms carried down through clip and render objects
 * 	XForms objects removed
 * count the total number of connections in all transparent fields.
 *
 * Sets global flags indicating:
 *	if transparent fields exist in the object
 *	if Volume fields exist in the object
 *	if a single regular volume is the only transparent object ?????
 *	if Clip objects exist in the object
 *
*/

Error
_dxf_gather(dxObject object, gatherO gather, void *globals)
{
  DEFGLOBALDATA(globals);
  DEFPORT(PORT_HANDLE);
  attributeP 	attributes = NULL;
  dxObject	newObject, savenew;
  int		flags;

  ENTRY(("_dxf_gather(0x%x, 0x%x, 0x%x)", object, gather, globals));

  attributes = _dxf_newAttribute(NULL);

  if (!object) {
    EXIT(("object == NULL"));
    return OK;
  }

  /*
   * Initialize (or create) the mesh and texture hash tables.
   * During the gather traversal, whenever we hit a triangle- or
   * quad-strippable field, look in the mesh hash table to see if 
   * there already is one.  Same for textures.  In the initialization
   * process, the hit flag for every object (of either kind) is
   * set to 0.  When an object is accessed on this traversal, its
   * hit flag is set to 1.  At the end of the traversal, the hash
   * tables are traversed, and any object with a hit flag of 0 
   * (indicating that it wasn't needed in this rendering) is discarded.
   */
  if (! MESHHASH) MESHHASH = _dxf_InitObjectHash();
  else _dxf_ResetObjectHash(MESHHASH);
    
  if (! TEXTUREHASH) TEXTUREHASH = _dxf_InitObjectHash();
  else _dxf_ResetObjectHash(TEXTUREHASH);

  _dxf_clearSortList(SORTLIST);

  if(!(savenew = newObject = DXCopy(object,COPY_ATTRIBUTES))) goto error;
  if(!(_gatherRecurse(object, &newObject, gather, attributes, globals))) 
    goto error;

  if(savenew != newObject)
    DXDelete(savenew);

  if (! _dxf_getHwGatherFlags(gather, &flags))
    goto error;

  if(!(flags & CONTAINS_LIGHT)) 
  {
    listO lo;
    Vector direction = {-1.0, 0.0, 1.0};
    RGBColor color = {1.0, 1.0, 1.0};
    Light	newLight;

    if(!(newLight = DXNewCameraDistantLight(direction,color)))
       goto error;

    DXReference((dxObject)newLight);

    if (! _dxf_getHwGatherLights(gather, &lo))
	goto error;

    if (! _dxf_listAppendItem(lo, (Pointer)newLight))
	goto error;

    color.r = color.g = color.b = 0.2;
    if (! _dxf_setHwGatherAmbientColor(gather, color))
	goto error;
  } 

  if (! _dxf_setHwGatherObject(gather, newObject))
      goto error;

  if (attributes)
    _dxf_deleteAttribute(attributes);

  /*
   * Now delete all the un-hit objects from the mesh and
   * texture hashes
   */
  MESHHASH = _dxf_CullObjectHash(MESHHASH);
  TEXTUREHASH = _dxf_CullObjectHash(TEXTUREHASH);

  EXIT(("OK"));
  return OK;

 error:

  if(attributes)
    _dxf_deleteAttribute(attributes);

  EXIT(("ERROR"));
  return ERROR;
}

#define OPACITY(offset)							\
  *(float *)((xf->omap) ?                                               \
      DXGetArrayEntry(xf->omap,                                         \
          *(char *) DXGetArrayEntry(xf->opacities, offset, &iscratch),  \
          fscratch) :                                                   \
      DXGetArrayEntry(xf->opacities,                                    \
          offset,                                                       \
          fscratch))

static Error
_gatherField(Field f, 
	     dxObject *newObject,
	     gatherO gather, 
	     attributeP attributes,
	     void *globals)
{
  DEFGLOBALDATA(globals);
  DEFPORT(PORT_HANDLE);
  xfieldP	xf;
  hwFlags	attFlags;
  hwFlags	servicesFlags;
  int           alphaTexture;

  ENTRY(("_gatherField(0x%x, 0x%x, 0x%x, 0x%x, 0x%x)",
	 f, newObject, gather, attributes, globals));
  
  if (DXEmptyField(f)) {
    *newObject = NULL;
    EXIT(("OK: f is empty"));
    return OK;
  }
	  
  servicesFlags = _dxf_SERVICES_FLAGS();

  if(_dxf_isFlagsSet(servicesFlags,SF_VOLUME_BOUNDARY))
    {
      dxObject	connections;
      char	*elementType = NULL;
      int	one = 1;
      
      if( (connections = DXGetComponentValue(f,"connections")) ) {
	if(!DXGetStringAttribute(connections,"element type",&elementType))
	  DXErrorGoto(ERROR_BAD_PARAMETER, "#13070");
	
	if(!strcmp(elementType,"tetrahedra") || !strcmp(elementType,"cubes")) {
	  Array validity;
	  dxObject newField;
	  ModuleInput inputs[2];
	  ModuleOutput outputs[1];
	  
	  if (!(validity = (Array)DXNewArray(TYPE_INT, CATEGORY_REAL, 0)))
	    goto error;

	  if (!DXAddArrayData((Array)validity, 0, 1, (Pointer)&one))
	    goto error;
	  
	  DXReference((dxObject)validity);

	  DXSetModuleInput(inputs[0],"input",(dxObject)f);
	  DXSetModuleInput(inputs[1],"validity",(dxObject)validity);
	  DXSetModuleOutput(outputs[0],"output",&newField);

	  if(!DXCallModule("ShowBoundary",2,inputs,1,outputs))
	  {
	      if (DXGetError() == ERROR_NONE)
		    DXSetError(ERROR_INTERNAL,
		    "ShowBoundary or DXCallModule failed, no error code set");
	      DXDelete((dxObject)validity);
	      goto error;
	  } 

	  DXDelete((dxObject)validity);
	  DXReference((dxObject)newField);
		    
	  if (!_gatherRecurse(newField,newObject, gather, 
			      attributes, globals))
	  {
	    DXDelete(newField);
	    goto error;
	  }
	  
	  DXDelete(newField);
	  
	  EXIT(("OK"));
	  return OK;
	}
	
      }
    }

  /* allocate xfield */
  *newObject = (dxObject)_dxf_newXfieldO(f,attributes,globals);
  if (! *newObject)
    goto error;

  xf = _dxf_getXfieldData((xfieldO)*newObject);
  if (! xf)
    goto error;

  attFlags = _dxf_attributeFlags(_dxf_xfieldAttributes(xf));

  if (!(_dxf_isFlagsSet(attFlags,(CONTAINS_TRANSPARENT | CONTAINS_VOLUME |
                                  CONTAINS_TRANSPARENT_TEXTURE ))))
  {
    EXIT(("OK: neither transparent nor volume field"));
    return OK;
  }
  
  /*
   * OK we're into the transparent fields (and volumes) now
   */

  if(_dxf_isFlagsSet(attFlags,IN_SCREEN_OBJECT)) {
    EXIT(("ERROR"));
    DXErrorReturn(ERROR_BAD_PARAMETER,
		  "Transparent fields not allowed in screen object");
  }
  
  /*
  _dxf_setFlags(attFlags,CONTAINS_TRANSPARENT);
  */

  alphaTexture = _dxf_isFlagsSet(attFlags,CONTAINS_TRANSPARENT_TEXTURE);
  if ( alphaTexture ) {
      int	 i;
      for (i = 0; i < xf->nconnections; i++)
          if (!xf->invCntns || !DXIsElementInvalid(xf->invCntns,i))
              _dxf_Insert_Translucent(SORTLIST, (void *)xf, i);
  }

  else /* opacities */
      switch (xf->opacitiesDep)
      {
	  case dep_field:
	  {
	      int	 i;
	      float  fscratch[1];
	      int    iscratch;

	      if (OPACITY(0))
		  for (i = 0; i < xf->nconnections; i++)
		      if (!xf->invCntns || !DXIsElementInvalid(xf->invCntns,i))
			  _dxf_Insert_Translucent(SORTLIST, (void *)xf, i);
	      break;
	  }

	  case dep_connections:
	  {
	      int	 i;
	      float  fscratch[1];
	      int    iscratch;

	      for (i = 0; i < xf->nconnections; i++)
		  if (!xf->invCntns || !DXIsElementInvalid(xf->invCntns,i))
		      if (OPACITY(i))
			  _dxf_Insert_Translucent(SORTLIST, (void *)xf, i);
	      break;
	  }

	  case dep_positions:
	  {
	      int	 i, j;
	      float  fscratch[1];
	      int    iscratch;
	      int	 *c, cscr[8];

	      for (i = 0; i < xf->nconnections; i++)
	      {
		  if (!xf->invCntns || !DXIsElementInvalid(xf->invCntns,i))
		  {
		      c = (int*)DXGetArrayEntry(xf->connections,i,cscr);
		      for (j = 0; j < xf->posPerConn; j++)
			  if (OPACITY(c[j]))
			  {
			      _dxf_Insert_Translucent(SORTLIST, (void *)xf, i);
			      break;
			  }
		  }
	      }
	      break;
	  }

	  case dep_none:
	  default:
	      break;
      }

  EXIT(("OK"));
  return OK;

 error:

  EXIT(("ERROR"));
  return ERROR;
}

static Error
_gatherRecurse(dxObject object,
	       dxObject *newObject,
	       gatherO gather,
	       attributeP attributes,
	       void *globals)
{
  DEFGLOBALDATA(globals);
  DEFPORT(PORT_HANDLE);
  dxObject 	subObject,tmpObject,attr;
  int 		i;
  attributeP 	new = NULL;
  Class		class,groupClass;
  hwFlags	newFlags;

  ENTRY(("_gatherRecurse(0x%x, 0x%x, 0x%x, 0x%x, 0x%x)",
	 object, newObject, gather, attributes, globals));

  attr = DXGetAttribute(object, "visible");
  if (attr)
  {
      int flag;
      DXExtractInteger(attr, &flag);
      if (flag == 0)
      {
	  *newObject = NULL;
	  return OK;
      }
  }
  
  if (!(new = _dxf_parameters((dxObject)object, attributes))) goto error;
  if (!(class = DXGetObjectClass(object))) goto error;
  newFlags = _dxf_attributeFlags(new);
  /*
   * XXX check for empty dxObject
   */

  switch (class) {
  case CLASS_GROUP:
  {
    *newObject = DXCopy(object,COPY_ATTRIBUTES);
    if(!(groupClass = DXGetGroupClass((Group)object))) goto error;
    switch(groupClass)
    {
	case CLASS_COMPOSITEFIELD:
	{
	    _dxf_setFlags(newFlags,IGNORE_PARAMS);
	    i = 0;
	    while (NULL != 
		(subObject = DXGetEnumeratedMember((Group)object, i, NULL)))
	    {
	        dxObject tmpObject;

	        if (!_gatherRecurse(subObject, &tmpObject, 
					gather, new, globals))
		    goto error;

		if (tmpObject)
		{
		    if (!DXSetMember((Group)*newObject,NULL,tmpObject))
			goto error;
	        }

		i++;
	    }
	    break;
	}

	case CLASS_SERIES:
	{
	    float p;
	    int j;
	    _dxf_clearFlags(newFlags, IGNORE_PARAMS);
	    i = j = 0;
	    while (NULL != 
		(subObject = DXGetSeriesMember((Series)object, i, &p)))
	    {
	        dxObject tmpObject;

	        if (!_gatherRecurse(subObject, &tmpObject, 
					gather, new, globals))
		    goto error;

		if (tmpObject)
		{
		    if (!DXSetSeriesMember((Series)*newObject, j++, p, tmpObject))
			goto error;
	        }

		i++;
	    }
	    break;
	}

	default:
	{
	    char *name;
	    _dxf_clearFlags(newFlags, IGNORE_PARAMS);
	    i = 0;
	    while (NULL != 
		(subObject = DXGetEnumeratedMember((Group)object, i, &name)))
	    {
	        dxObject tmpObject;

	        if (!_gatherRecurse(subObject, &tmpObject, 
					gather, new, globals))
		    goto error;

		if (tmpObject)
		{
		    if (!DXSetMember((Group)*newObject, name, tmpObject))
			goto error;
	        }

		i++;
	    }
	    break;
	}
    }

    break;
  }
  case CLASS_FIELD:
    if(!_gatherField((Field)object,newObject,gather,new,globals))
      goto error;
    break;
  case CLASS_SCREEN:
    {
      int		positionUnits, z;
      float		matrix[4][4];
      float		ident[4][4];
      int		flags;
      

      COPYMATRIX(ident,identity);

      if(!(DXGetScreenInfo((dxScreen)object,&subObject,&positionUnits,&z)))
	goto error;

      /* 
       * Set the modeling matrix to identity for recursion
       */
      _dxf_attributeMatrix(attributes,matrix);
      _dxf_setAttributeMatrix(new, ident);
      _dxf_setFlags(newFlags,IN_SCREEN_OBJECT);

      if (positionUnits != SCREEN_STATIONARY)
      {
	if (z)
	  new->ff = 1.0 / 1000000;
	
	new->fuzz *= new->ff;
      }

      if (!_gatherRecurse(subObject, &tmpObject, gather, new, globals))
	goto error;

      if(tmpObject) {

	*newObject = (dxObject)_dxf_newHwScreen((dxScreen)object,
						(dxObject) tmpObject,
						NULL,matrix,
						0,0);
	if (! *newObject)
	   goto error;
 
	if (! _dxf_getHwGatherFlags(gather, &flags))
	  goto error;

	flags |= CONTAINS_SCREEN;

        if (! _dxf_setHwGatherFlags(gather, flags))
	  goto error;

      }
      else
      {
	*newObject = NULL;
      }
    }
    break;
  case CLASS_CLIPPED:
    {
      dxObject 		clipObject;
      float		m[4][4];
      double		dm[4][4],datm[4][4];
      static Vector 	X={1.0,0.0,0.0},Y={0.0,1.0,0.0},Z={0.0,0.0,1.0};
      Array		attr;
      Pointer		attP;
      Point		pt,min,max,newMin,newMax;
      Vector		normal;
      clippedO		cpo;


      /*
       * validity checks 
       */
      if(_dxf_isFlagsSet(newFlags,IN_SCREEN_OBJECT)) {
	EXIT(("ERROR"));
	DXErrorReturn(ERROR_BAD_PARAMETER,
		      "Clipped objects not allowed in screen object");
      }

      if(_dxf_isFlagsSet(newFlags,BEING_CLIPPED)) {
	EXIT(("ERROR"));
	DXErrorReturn(ERROR_BAD_PARAMETER,
		      "Clipped objects cannot be clipped.");
      }

      if(_dxf_isFlagsSet(newFlags,IN_CLIP_OBJECT)) {
	EXIT(("ERROR"));
	DXErrorReturn(ERROR_BAD_PARAMETER,
		      "Nested clipped objects not allowed.");
      }

      DXGetClippedInfo((Clipped)object, &subObject, &clipObject);

      _dxf_setFlags(newFlags,BEING_CLIPPED);
      if (!_gatherRecurse(subObject, &tmpObject, gather, new, globals))
	goto error;
      _dxf_clearFlags(newFlags,BEING_CLIPPED);

      if(tmpObject) {

	/* Get the current modeling transform and adjointTranspose */
	_dxf_attributeMatrix(new,m);
	COPYMATRIX(dm,m);
	_dxfAdjointTranspose(datm,dm);
	
	if((attr = (Array)DXGetAttribute(clipObject,"clipplane normal")))
	{
	  Point  *cp_pt;
	  Vector *cp_nrm;

	  attP = (Vector *)DXGetConstantArrayData(attr);
	  if(!attP)
	    DXErrorGoto(ERROR_BAD_PARAMETER,
		"clip plane has invalid normal");

	  normal = *(Vector*)attP;

	  attr = (Array)DXGetAttribute(clipObject,"clipplane point");
	  if (!attr)
	      DXErrorGoto(ERROR_BAD_PARAMETER,
	  	  "clip plane contains no point");

	  attP = (Point *)DXGetConstantArrayData(attr);
	  if(!attP)
	      DXErrorGoto(ERROR_BAD_PARAMETER,
		  "clip plane has invalid point");

	  pt = *(Vector*)attP;
	  
	  cpo = _dxf_newHwClipped(1, tmpObject);
	  if (! cpo)
	      goto error;
	   
	  APPLY3((float *)&pt,dm);
	  APPLY3((float *)&normal,datm);

	  if (! _dxf_getHwClippedInfo(cpo, &cp_pt, &cp_nrm, NULL, NULL))
	      goto error;

	  /* Negate the sense of the normal (undoes same in DXClipPlane)
	     and normalize */

	  *cp_pt  = pt;
	  *cp_nrm = DXNormalize(DXNeg(normal));
	  
	  *newObject = (dxObject)cpo;

	}
	else if ( (attr = (Array)DXGetAttribute(clipObject,"clipbox min")) )
	{
	  Point  *cp_pts;
	  Vector *cp_nrms;

	  attP = (Point *)DXGetConstantArrayData(attr);
	  if(!attP)
	      DXErrorGoto(ERROR_BAD_PARAMETER,
		  "clip box has invalid min");

	  min = *(Point *)attP;

	  attr = (Array)DXGetAttribute(clipObject,"clipbox max");
	  if(!attr)
	      DXErrorGoto(ERROR_BAD_PARAMETER,
		  "clip box contains no max");

	  attP = (Point *)DXGetConstantArrayData(attr);
	  if(!attP)
	      DXErrorGoto(ERROR_BAD_PARAMETER,
		  "clip box has invalid max");

	  max = *(Point *)attP;
	  
	  newMin.x = min.x < max.x ? min.x : max.x;
	  newMin.y = min.y < max.y ? min.y : max.y;
	  newMin.z = min.z < max.z ? min.z : max.z;
	  
	  newMax.x = min.x > max.x ? min.x : max.x;
	  newMax.y = min.y > max.y ? min.y : max.y;
	  newMax.z = min.z > max.z ? min.z : max.z;
	  
	  APPLY3((float*)&newMin,dm);
	  APPLY3((float*)&newMax,dm);
	  APPLY3((float*)&X,datm);
	  APPLY3((float*)&Y,datm);
	  APPLY3((float*)&Z,datm);

	  cpo = _dxf_newHwClipped(6, tmpObject);
	  if (! cpo)
	      goto error;

	  if (! _dxf_getHwClippedInfo(cpo, &cp_pts, &cp_nrms, NULL, NULL))
	      goto error;

	  cp_pts[0] = newMin;
	  cp_nrms[0] = X;
	  
	  cp_pts[1] = newMin;
	  cp_nrms[1] = Y;
	  
	  cp_pts[2] = newMin;
	  cp_nrms[2] = Z;
	  
	  cp_pts[3] = newMax;
	  cp_nrms[3] = DXNeg(X);
	  
	  cp_pts[4] = newMax;
	  cp_nrms[4] = DXNeg(Y);
	  
	  cp_pts[5] = newMax;
	  cp_nrms[5] = DXNeg(Z);
	  
	  *newObject = (dxObject)cpo;
	}
	else 
	{
	  *newObject = NULL;
	  DXErrorGoto(ERROR_BAD_PARAMETER,"unrecognized clip object");
	}
	
	
      _dxf_setFlags(newFlags,CONTAINS_CLIP_OBJECT);

      } else
	*newObject = NULL;
    }
    break;
  case CLASS_XFORM:
    {
      dxMatrix	matrix;
      float	tmp[4][4],tmp1[4][4],tmp2[4][4];
      /*
       * remove XForms from tree, propagate Xforms to Fields
       */

      if (!DXGetXformInfo((Xform)object, &subObject, &matrix)) {
	EXIT(("DXGetXformInfo returns NULL"));
	return ERROR;
      }
      
      CONVERTMATRIX(tmp,matrix);
      _dxf_attributeMatrix(new,tmp1);
      MULTMATRIX(tmp2,tmp,tmp1);
      _dxf_setAttributeMatrix(new,tmp2);
      if (!_gatherRecurse(subObject, newObject, gather, new, globals))
	goto error;
    }
    break;
  case CLASS_LIGHT:
    {
      float	direction[3];
      RGBColor	color;
      int       flags;

      *newObject = NULL;

      if(_dxf_isFlagsSet(newFlags,IN_SCREEN_OBJECT)) {
	EXIT(("ERROR"));
	DXErrorReturn(ERROR_BAD_PARAMETER,
		      "Light objects not allowed in Screen object");
      }

      if(DXQueryCameraDistantLight((Light)object,(Vector*)direction,&color))
      {
	listO lights;

	if (! _dxf_getHwGatherLights(gather, &lights))
	    goto error;

	 /* only first 8 lights are used */
	if(_dxf_listSize(lights) >= 8) {
	  DXWarning("#5195") ;
	  break;
	}

	DXReference(object);

	if(! _dxf_listAppendItem(lights,(Pointer) object))
	  goto error;

      }
      else if (DXQueryDistantLight((Light)object,(Vector*)direction,&color))
      {
	listO lights;

	if (! _dxf_getHwGatherLights(gather, &lights))
	    goto error;

	if(_dxf_listSize(lights) >= 8) {
	  DXWarning("#5195") ;
	  break;
	}

	DXReference(object);

	if(! _dxf_listAppendItem(lights,(Pointer) object))
	  goto error;
      }
      else if(DXQueryAmbientLight((Light)object,&color)) 
      {
	RGBColor gcolor;

	if (! _dxf_getHwGatherAmbientColor(gather, &gcolor))
	    goto error;

	gcolor.r += color.r;
	gcolor.g += color.g;
	gcolor.b += color.b;

	if (! _dxf_setHwGatherAmbientColor(gather, gcolor))
	    goto error;

      } 
      else
      {
	PRINT(("invalid light"));
	DXErrorGoto(ERROR_DATA_INVALID, "#13750");
      }

      if (! _dxf_getHwGatherFlags(gather, &flags))
	  goto error;

      flags |= CONTAINS_LIGHT;

      if (! _dxf_setHwGatherFlags(gather, flags))
	  goto error;

    }
    break;
  default:
    break;
  }

  if(new)
    _dxf_deleteAttribute(new);

  EXIT(("OK"));
  return OK;

 error:

  if(new)
    _dxf_deleteAttribute(new);

  EXIT(("OK"));
  return ERROR;
}

