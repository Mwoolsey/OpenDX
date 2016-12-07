/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


#include "hwDeclarations.h"
#include "hwPortLayer.h"
#include "hwXfield.h"
#include "hwPortSB.h"

#if 1
#define Object dxObject
#define Matrix dxMatrix
#include "internals.h"
#undef Object
#undef Matrix
#endif

#include "hwDebug.h"

Error
_dxf_DrawOpaque (tdmPortHandleP portHandle, xfieldP xf,
		 RGBColor ambientColor, int buttonUp)
{
  DEFPORT(portHandle) ;
  enum approxE approx;
  int  density;
  int opaque_pass, contains_transparent, ret ;

  ENTRY(("_dxf_DrawOpaque(0x%x, 0x%x, 0x%x, %d)",
	 portHandle, xf, ambientColor, buttonUp));
  TIMER("> _dxf_DrawOpaque") ;

  /* HACK ALERT */
  if (buttonUp > 1)
    {
      /*
       *  For now interpret this as indicating that the port layer
       *  supports sorting transparent polygons across a single field.
       *  (This does not produce a correct rendering if more than one
       *  transparent field is projected onto the same screen area, but it
       *  is adequate for many visualizations).  The buttonUp parameter is
       *  kludged to flag the transparent rendering pass, which must occur
       *  after the opaque pass.  (see hwPaint.c)
       */
      buttonUp -= 2 ;
      opaque_pass = 0 ;
    }
  else
      opaque_pass = 1 ;

#if 0
  /* this doesn't seem to work */
  contains_transparent = _dxf_isAttributeFlag (_dxf_xfieldAttributes(xf),
					       CONTAINS_TRANSPARENT) ;
#else
  contains_transparent = (int) xf->opacities_array ;
#endif

  PRINT(("%s", opaque_pass ? "opaque pass" : "transparent pass"));
  PRINT(("field contains transparent: %s", contains_transparent ? "yes" : "no"));

  if(buttonUp) {
      density = xf->attributes.buttonUp.density ;
      approx = xf->attributes.buttonUp.approx ;
  } else {
      density = xf->attributes.buttonDown.density ;
      approx = xf->attributes.buttonDown.approx ;
    }

  if (xf->normalsDep != ct_none && approx == approx_none)
    {
      _dxf_SET_GLOBAL_LIGHT_ATTRIBUTES (PORT_CTX ,1) ;
      
      if (!(xf->attributes.flags & CORRECTED_COLORS))
	{
	  xf->attributes.front.ambient = 
	      pow((double)xf->attributes.front.ambient,
		  1./SBTRANSLATION->gamma) ;
	  xf->attributes.front.specular = 
	      pow((double)xf->attributes.front.specular,
		  1./SBTRANSLATION->gamma) ;
	  xf->attributes.flags |= CORRECTED_COLORS ;
	}
      
      _dxf_SET_MATERIAL_SPECULAR (PORT_CTX, 1.0 /*R*/, 1.0 /*G*/, 1.0 /*B*/,
				  (float) xf->attributes.front.shininess) ;
    }

  _dxf_PUSH_MULTIPLY_MATRIX (PORT_CTX, xf->attributes.mm) ;

  /* return OK if we end up drawing nothing */
  ret = OK ;

  if (opaque_pass)
      switch (approx)
	{
	case approx_box:
	  ret = _dxfBoundingBoxDraw (portHandle, xf, 0) ;
	  break;
	default:
	  switch (xf->connectionType)
	    {
	    case ct_none:
	      TIMER("> dots:");
	      ret = _dxfUnconnectedPointDraw (portHandle, xf, buttonUp) ;
	      TIMER("< dots:");
	      break;
	    case ct_lines:
	      TIMER("> lines:");
	      ret = _dxfLineDraw (portHandle, xf, buttonUp) ;
	      TIMER("< lines:");
	      break;
	    case ct_polylines:
	      TIMER("> lines:");
	      ret = _dxfPolylineDraw (portHandle, xf, buttonUp) ;
	      TIMER("< lines:");
	      break;
	    case ct_triangles:
	      if (!contains_transparent) {
		TIMER("> triangle:");
		ret = _dxfTriDraw (portHandle, xf, buttonUp) ;
		TIMER("< triangle:");
	      }
	      break;
	    case ct_quads:
	      if (!contains_transparent) {
		TIMER("> quads:");
		ret = _dxfQuadDraw (portHandle, xf, buttonUp) ;
		TIMER("< quads:");
	      }
	      break;
	    case ct_pline:
	      if (density == 1) {
		TIMER("> plines:");
		ret = _dxfPlineDraw (portHandle, xf, buttonUp) ;
		TIMER("< plines:");
	      } else {
		TIMER("> lines:");
		ret = _dxfLineDraw (portHandle, xf, buttonUp) ;
		TIMER("< lines:");
	      }
	      break;
	    case ct_tmesh:
	      if (!contains_transparent)
		if (density == 1) {
		  TIMER("> tmesh:");
		  ret = _dxfTmeshDraw (portHandle, xf, buttonUp) ;
		  TIMER("< tmesh:");
	        } else {
		  TIMER("> triangle:");
		  ret = _dxfTriDraw (portHandle, xf, buttonUp) ;
		  TIMER("< triangle:");
		}
	      break;
	    case ct_qmesh:
	      if (!contains_transparent)
		if (density == 1) {
		  TIMER("> qmesh:");
		  ret = _dxfQmeshDraw (portHandle, xf, buttonUp) ;
		  TIMER("< qmesh:");
	        } else {
		  TIMER("> quads:");
		  ret = _dxfQuadDraw (portHandle, xf, buttonUp) ;
		  TIMER("< quads:");
		}
	      break;
	    }
	}
  else
      /* transparent pass */
      switch (approx)
	{
	case approx_box:
	  break ;
	default:
	  switch (xf->connectionType)
	    {
	    case ct_tetrahedra:
	      TIMER("> tetras:");
	      ret = _dxfTetraDraw (portHandle, xf, buttonUp) ;
	      TIMER("< tetras:");
	      break;
	    case ct_cubes:
	      TIMER("> cubes:");
	      ret = _dxfCubeDraw (portHandle, xf, buttonUp) ;
	      TIMER("< cubes:");
	      break;
	    case ct_triangles:
	      if (contains_transparent) {
		TIMER("> triangle:");
		ret = _dxfTriDraw (portHandle, xf, buttonUp) ;
		TIMER("< triangle:");
	      }
	      break;
	    case ct_quads:
	      if (contains_transparent) {
		TIMER("> quads:");
		ret = _dxfQuadDraw (portHandle, xf, buttonUp) ;
		TIMER("< quads:");
	      }
	      break;
	    case ct_tmesh:
	      if (contains_transparent) 
		if (density == 1) {
		    /* allow fast dot and line approximations */
		  TIMER("> tmesh:");
		  ret = _dxfTmeshDraw (portHandle, xf, buttonUp) ;
		  TIMER("< tmesh:");
	        } else {
		  TIMER("> triangle:");
		  ret = _dxfTriDraw (portHandle, xf, buttonUp) ;
		  TIMER("< triangle:");
		}
	      break;
	    case ct_qmesh:
	      if (contains_transparent)
		if (density == 1) {
		    /* allow fast dot and line approximations */
		  TIMER("> qmesh:");
		  ret = _dxfQmeshDraw (portHandle, xf, buttonUp) ;
		  TIMER("< qmesh:");
		} else {
		  TIMER("> quads:");
		  ret = _dxfQuadDraw (portHandle, xf, buttonUp) ;
		  TIMER("< quads:");
		}
	      break;
	    }
	}

 done:
  _dxf_POP_MATRIX(PORT_CTX) ;
  TIMER("< _dxfDrawOpaque");
  EXIT(("ret = 0x%x", ret));
  return ret ;
}
