/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


#include "hwDeclarations.h"
#include "hwList.h"
#include "hwClipped.h"
#include "hwScreen.h"
#include "hwWindow.h"
#include "hwPortLayer.h"
#include "hwGather.h"
#include "hwView.h"
#include "hwMatrix.h"
#include "hwXfield.h"
#include "hwTmesh.h"
#include "hwMaterials.h"
#include "hwSort.h"

#include "hwDebug.h"

extern void _dxfGetViewMatrixWithFuzz(void *stack, double fuzz, 
	register float m[4][4]);

static Error 
_paintRecurse(void* globals, dxObject object,
	      gatherO gather, int buttonUp,
	      screenO parent, screenO grandparent);


struct processImageArg {
  void*	win;
  void* translation;
};

/*
 * Load lighting into the hardware. Provide surface properties to allow
 * the lights to be biased to represent surface properties.
 *
 * Some ports do not have a way to define a material properties that are
 * independent of the surface color (GL OpenGL). However, we can apply the
 * coeficients to the lights instead to achieve the same result
 *
 * Light at each vertex has three parts, an ambient, a diffuse and a specular
 * component. The vertex color is:
 * vert_color = amb_light_color * vert_color * k_amb +
 *	        for_each_light( light_color * vert_color * k_diff * misc_stuff +
 *                              light_color * k_spec * other_misc_stuff)
 *
 * if we wrap the k_amb and k_diff into the light color the equ becomes:
 *
 * vert_color = new_amb_light_color * vert_color +
 *	        for_each_light( new_light_color * vert_color *  misc_stuff +
 *                              new_light_color * k_spec/k_diff * other_misc_stuff)
 *
 * What is significant here is that we only need to send a single color to GL
 * for ambient,diffuse and specular, and that this color does not need to be
 * multiplied before being sent down. This is a big optimization.
 */
static Error
loadLights(void* globals, gatherO gather, materialAttrP newMaterial)
{
  DEFGLOBALDATA(globals);
  DEFPORT(PORT_HANDLE);
  int	i,j;
  Light	newLight;
  static materialAttrT	identMaterial = {1.0, 1.0, 1.0, 1};
  RGBColor color;
  listO lights;
  materialO omo, nmo;
  float gka, gkd, gks, gsp;

  ENTRY(("loadLights(0x%x, 0x%x, 0x%x)",globals, gather, newMaterial));

  /* 
   * if no material coeficients were specified or this port does not
   * want the coeficients pre-multiplied into the lights, use all
   * unit valued coeficients.
   */
  if(!newMaterial || 
     !_dxf_isFlagsSet(_dxf_SERVICES_FLAGS(),SF_CONST_COEF_HELP))
    newMaterial = &identMaterial;

  /*
   * Only re-load the lights if the material coeficients have changed
   */
  if (! _dxf_getHwGatherMaterial(gather, &omo))
      goto error;

  if (omo)
      if (! _dxf_getHwMaterialInfo(omo, &gka, &gkd, &gks, &gsp))
	  goto error;

  if(!omo ||
     gka != newMaterial->ambient ||
     gkd != newMaterial->diffuse) {

    nmo = _dxf_newHwMaterial(
		newMaterial->ambient,
		newMaterial->diffuse,
		newMaterial->specular,
		newMaterial->shininess);

    if (! nmo)
	goto error;
       
    if (! _dxf_setHwGatherMaterial(gather, nmo))
      goto error;

    if (! _dxf_getHwGatherAmbientColor(gather, &color))
      goto error;
    
    /* Load (or disable) the Ambient light */
    if(color.r == 0.0 && color.g == 0.0 && color.b == 0.0) 
    {

      if (!_dxf_DEFINE_LIGHT (LWIN, 0, NULL))
        goto error;

    }
    else
    {
	float		ambient = 1.0;

	/* 
	 * For ports with SF_CONST_COEF_HELP we put the ambient/diffuse coefs
	 * in the ambient light color. This is for GL, this allows us to
	 * use LMC_AD color for surfaces. This reduces the number of calls
	 * we need by 2 per vertex.
	 *
	 * The ambient is divided by the diffuse so that we can multiply the
	 * diffuse into the surface color and use LMC_AD for the color.
	 */

	if(newMaterial->diffuse)
	  ambient = newMaterial->ambient / newMaterial->diffuse;
	else
	  ambient = newMaterial->ambient / (1./10000.);

	color.r *= ambient;
	color.g *= ambient;
	color.b *= ambient;

	if(!(newLight = DXNewAmbientLight(color)))
	  goto error;

	if (!_dxf_DEFINE_LIGHT (LWIN, 0, newLight))
	  goto error;
	
	DXDelete((dxObject)newLight);
    }

    if (! _dxf_getHwGatherLights(gather, &lights))
	goto error;

    /* Load all directional lights */
    for(i=1,j=_dxf_listSize(lights)-1;j>=0 && i<=8;j--,i++) {
      if (!_dxf_DEFINE_LIGHT (LWIN, i, 
			      _dxf_listGetItem(lights,j)))
	goto error;
    }
    
    /* remove any previously defined lights, that have not been redefined */
    for(;i<=8;i++) {
      if (!_dxf_DEFINE_LIGHT (LWIN, i, NULL))
	goto error;
    }	
  }

  EXIT(("OK"));
  return OK;

 error:

  EXIT(("ERROR"));
  return ERROR;
}   

static Field
_processImage(Field f, char* args, int size)
{
  struct processImageArg* argI = (struct processImageArg*)args;
  DEFWINDATA((argI->win));
  DEFPORT(PORT_HANDLE);

  ENTRY(("_processImage(0x%x, \"%s\", %d)",f, args, size));

  /* XXX Will this work in parallel ? */
  _dxf_DRAW_IMAGE(argI->win, (dxObject)f, argI->translation);
  
  EXIT(("f = 0x%x",f));
  return f;
}

Error
_dxf_paint(void* globals, viewO view, int buttonUp, screenO camScreen)
{
  DEFGLOBALDATA(globals);
  DEFPORT(PORT_HANDLE);
  gatherO	gather;
  translationO  translation;
  int flags;
  RGBColor color;
  RGBColor background;
  float projectionMat[4][4];
  float viewMat[4][4];
  SortListP slp;


  if (! _dxf_getHwViewInfo(view, NULL, &gather, &translation,
		(float *)&projectionMat, (float *)viewMat, &background))
      goto error;

  ENTRY(("_dxf_paint(0x%x, 0x%x, %d)",globals, view, buttonUp));

  /* Turn off trace macros from this level down */
  /* Turn it on by exporting this variable */
  DEBUG_CALL ( if (!getenv("DXHW_DEBUG_DETAIL")) DEBUG_OFF(); );

  if (! _dxf_getHwGatherFlags(gather, &flags))
      goto error;

  if(flags & CONTAINS_IMAGE_ONLY) {
    dxObject object;
    static RGBColor black = {0.0,0.0,0.0};

    _dxf_SET_BACKGROUND_COLOR(PORT_CTX,black);
    _dxf_INIT_RENDER_PASS (LWIN,SingleBufferMode,ZBufferDisabled);

    if (! _dxf_getHwGatherObject(gather, &object))
	goto error;


    _dxf_DRAW_IMAGE(LWIN, object, translation);

  }
  else
  {
    dxObject object;

    _dxf_REPLACE_VIEW_MATRIX(PORT_CTX,projectionMat);
    _dxf_PUSH_REPLACE_MATRIX(PORT_CTX,viewMat);
    _dxf_SET_BACKGROUND_COLOR(PORT_CTX,background);
    _dxf_INIT_RENDER_PASS (LWIN,DoubleBufferMode,ZBufferEnabled);
    
    /* force a reload of the lights with each new object */
    if (! _dxf_setHwGatherMaterial(gather, NULL))
	goto error;

    if (! _dxf_getHwGatherObject(gather, &object))
	goto error;

    if(!_paintRecurse(globals, object, gather, buttonUp, camScreen, NULL))
      goto error;

    slp = (SortListP)SORTLIST;
    if (slp->itemCount)
    {
	if (! _dxf_Sort_Translucent(SORTLIST))
	  goto error;
		
	if (! _dxf_getHwGatherAmbientColor(gather, &color))
	  goto error;

        if(!_dxf_DRAW_TRANSPARENT(globals, &color, buttonUp))
	  goto error;
    }

    _dxf_POP_MATRIX(PORT_CTX);
  }

  DEBUG_ON();
  EXIT(("OK"));
  return OK;

 error:

  DEBUG_ON();
  EXIT(("ERROR"));
  return ERROR;
}

static Error
_paintRecurse(void* globals, dxObject object,
	      gatherO gather, int buttonUp, 
	      screenO parent, screenO grandparent)
{
  DEFGLOBALDATA(globals);
  DEFPORT(PORT_HANDLE);
  Class		class;
  int		i;
  dxObject	subObject;

  ENTRY(("_paintRecurse(0x%x, 0x%x, 0x%x, %d)",
	 globals, object, gather, buttonUp));
  
  if(!object)
  {
    EXIT(("object = NULL"));
    return OK;
  }

  if(!(class = DXGetObjectClass(object)))
     goto error;
  
  switch (class)
  {
  case CLASS_GROUP:
    PRINT(("CLASS_GROUP"));
    for (i=0; (subObject=DXGetEnumeratedMember((Group)object, i, NULL)); i++) {
      /* DXDebug("g", "member %d", i); */
      if (!_paintRecurse(globals, subObject, gather, buttonUp,
					parent, grandparent))
	goto error;
    }
    break;
  case CLASS_PRIVATE: /* created from CLASS_FIELD in gather */
    {
      switch(_dxf_getHwClass(object)) {
      case HW_CLASS_XFIELD:
      {
	  xfieldP	xf = _dxf_getXfieldData((xfieldO)object);
	  float		fuzz;
	  
	  PRINT(("HW_CLASS_XFIELD"));
	  if(_dxf_isFlagsSet(_dxf_attributeFlags(_dxf_xfieldAttributes(xf)),
			     IN_CLIP_OBJECT))
          {
	    /*
	     * The current implementation allows any object to be clip object,
	     * this line allows the beginClip to call back into this function
	     * to get the clip object recursively drawn
	     */
	    if (!_dxf_DRAW_CLIP(PORT_HANDLE, xf, buttonUp))
	      goto error;
	  }
	  else
	  {
	    int nClips;
	    Point *clipPts = NULL;
	    Vector *clipVecs = NULL;

	    if (! _dxf_getHwGatherClipPlanes(gather, &nClips, &clipPts, &clipVecs))
	      goto error;

	    if (nClips)
	    {
		if (! _dxf_setHwXfieldClipPlanes(object, nClips, clipPts, clipVecs))
		    goto error;
		
		DXFree((Pointer)clipPts);
		DXFree((Pointer)clipVecs);
	    }

	    if(!loadLights(globals, gather,
			   _dxf_attributeFrontMaterial(_dxf_xfieldAttributes(xf))))
	      goto error;


	    
	    /* XXXX
	       I hate this. Fuzz forces us to have the camera available when
	       we are drawing each field !
	       */
	    if(!_dxf_isFlagsSet(_dxf_attributeFlags(_dxf_xfieldAttributes(xf)),
				IN_SCREEN_OBJECT)
	       && (fuzz = (float)*_dxf_attributeFuzz(_dxf_xfieldAttributes(xf),
						     NULL)))
	    {
	      float	vm[4][4];
	      RGBColor color;

	      PRINT(("dealing with fuzz..."));
	      _dxfGetViewMatrixWithFuzz(MATRIX_STACK, (double)fuzz, vm);
	      _dxf_PUSH_REPLACE_MATRIX(PORT_CTX,vm);

	      if (! _dxf_getHwGatherAmbientColor(gather, &color))
		  goto error;

	      if(!_dxf_DRAW_OPAQUE(PORT_HANDLE, xf,
				      color, buttonUp))
		   goto error;

	      _dxf_POP_MATRIX(PORT_CTX);
	    }
	    else 
            {
	      RGBColor color;
	      PRINT(("! Fuzzy Screen Object"));

	      if (! _dxf_getHwGatherAmbientColor(gather, &color))
		  goto error;

	      if(!_dxf_DRAW_OPAQUE(PORT_HANDLE, xf,
				 color, buttonUp))
	      goto error;
	    }
	  }
	}
	break;
      case HW_CLASS_SCREEN:
	{
	  dxScreen		s;
	  float		       	vm[4][4],pm[4][4];

	  PRINT(("HW_CLASS_SCREEN:"));

	  if(!(_dxf_getHwScreenView((screenO)object, &s,
			&subObject,vm,NULL, NULL,NULL)))
	    goto error;

	  if (_dxf_getHwClass(parent) != HW_CLASS_SCREEN)
	  {
	    DXSetError(ERROR_INTERNAL, "parent of screen not screen?");
	    goto error;
	  }

	  if(!(_dxf_getHwScreenView((screenO)parent, NULL,
			NULL,NULL,pm, NULL,NULL)))
	    goto error;

	  /*
	   * If this is a top level screen object (its parent was created from
	   * the camera and has no parent), then set the projection matix to
	   * identity. Reset the the camera projection matrix on exit.
	   */
	  if (!grandparent)
	    _dxf_REPLACE_VIEW_MATRIX(PORT_CTX,(float (*)[4])identity);

	  _dxf_PUSH_REPLACE_MATRIX(PORT_CTX,vm);

	  if (!_paintRecurse(globals, subObject, gather,
					buttonUp, object, parent))
	    goto error;
	  
	  _dxf_POP_MATRIX(PORT_CTX);

	  if(!grandparent)
	    _dxf_REPLACE_VIEW_MATRIX(PORT_CTX,pm);
	}
	break;
      case HW_CLASS_CLIPPED:
	{
	  int		knt;
	  Point         *pts;
	  Vector        *nrms;
	  dxObject      subObject;
	  
	  PRINT(("HW_CLASS_CLIPPED:"));
	  
	  /*
	   * pt and normal are in world coordinates. _dxf_ADD_CLIP_PLANE
	   * requires view coordinates.
	   *
	   * So we need to apply the inverse of the clipping transform to the 
	   * pt and normal to correct for this.
	   */

	  if (!_dxf_getHwClippedInfo((clippedO)object,
					&pts, &nrms, &knt, &subObject))
	    goto error;
	
	  if (!_dxf_pushHwGatherClipPlanes(gather, knt, pts, nrms))
	    goto error;
	
	  if (!_dxf_ADD_CLIP_PLANES(LWIN, pts, nrms, knt))
	    goto error;

	  if(!_paintRecurse(globals, subObject, gather, buttonUp,
			parent, grandparent))
	    goto error;
	  
	  if (!_dxf_popHwGatherClipPlanes(gather))
	      goto error;

	  if (!_dxf_REMOVE_CLIP_PLANES(LWIN, knt))
	    goto error;
	}
	break;
      default:
	DXErrorGoto (ERROR_BAD_PARAMETER,
		     "Unrecognized private Object during paint");
	break;
      }
    }
    break;
  case CLASS_CLIPPED:
  case CLASS_XFORM:
  case CLASS_SCREEN:
  case CLASS_FIELD:
  case CLASS_LIGHT:
  default:
    DXSetError (ERROR_BAD_PARAMETER,
		"Unrecognized Object (%d) during paint",class);
    goto error;
    break;
  }
  
  EXIT(("OK"));
  return OK;

error:

  EXIT(("ERROR"));
  return ERROR;
}



