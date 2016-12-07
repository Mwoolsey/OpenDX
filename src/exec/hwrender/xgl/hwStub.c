/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/*********************************************************************/
/*                     I.B.M. CONFIENTIAL                           */
/*********************************************************************/

#include <dxconfig.h>



/*---------------------------------------------------------------------------*\
 $Source: /src/master/dx/src/exec/hwrender/xgl/hwStub.c,v $

 $Log: hwStub.c,v $
 Revision 1.3  1999/05/10 15:45:40  gda
 Copyright message

 Revision 1.3  1999/05/10 15:45:40  gda
 Copyright message

 Revision 1.2  1999/05/03 14:06:43  gda
 moved to using dxconfig.h rather than command-line defines

 Revision 1.1.1.1  1999/03/24 15:18:36  gda
 Initial CVS Version

 Revision 1.1.1.1  1999/03/19 20:59:49  gda
 Initial CVS

 Revision 10.1  1999/02/23 21:03:04  gda
 OpenDX Baseline

 Revision 9.1  1997/05/22 22:34:12  svs
 Copy of release 3.1.4 code

 * Revision 8.0  1995/10/03  22:15:48  nsc
 * Copy of release 3.1 code
 *
 * Revision 7.7  1995/04/03  15:12:35  gda
 * added polylines
 *
 * Revision 7.6  1994/04/21  15:35:31  tjm
 * added polylines to sun and hp, fixed mem leak in xf->origConnections and
 * added performance timing marks to sun and hp
 *
 * Revision 7.5  94/03/31  15:55:24  tjm
 * added versioning
 * 
 * Revision 7.4  94/03/09  17:16:55  tjm
 * restored DrawOpaque interface, use TRANSLATION->gamma for gamma correction
 * 
 * 
\*---------------------------------------------------------------------------*/
#include "hwDeclarations.h"
#include "hwPortLayer.h"
#include "hwXfield.h"
#include "hwMemory.h"
#include "hwPortXGL.h"

#define String dxString
#define Object dxObject
#define Angle dxAngle
#define Matrix dxMatrix
#define Screen dxScreen
#define Boolean dxBoolean
#include "internals.h"
#undef String
#undef Object
#undef Angle
#undef Matrix
#undef Screen
#undef Boolean

#define DXMARK(x)	DXMarkTime(x)
Error
_dxf_DrawOpaque (tdmPortHandleP portHandle, xfieldP xf,
		 RGBColor ambientColor, int buttonUp)
{
  DEFPORT(portHandle);
  enum approxE approx ;
  int density ;
  Error ret ;

  DPRINT("\n(_dxf_DrawOpaque") ;
  
  if (buttonUp)
    {
      approx = xf->attributes.buttonUp.approx ;
      density = xf->attributes.buttonUp.density ;
    }
  else
    {
      approx = xf->attributes.buttonDown.approx ;
      density = xf->attributes.buttonDown.density ;
    }

  if (xf->normalsDep != ct_none && approx == approx_none)
    {
      _dxf_SET_GLOBAL_LIGHT_ATTRIBUTES (PORT_CTX, 1) ;

      if ( !(xf->attributes.flags & CORRECTED_COLORS))
	{
	  xf->attributes.front.ambient = 
	      pow((double)xf->attributes.front.ambient,
		  1./XGLTRANSLATION->gamma) ;
	  xf->attributes.front.specular = 
	      pow((double)xf->attributes.front.specular,
		  1./XGLTRANSLATION->gamma) ;
	  xf->attributes.flags |= CORRECTED_COLORS ;
	}

      _dxf_SET_MATERIAL_SPECULAR (PORT_CTX, 1.0 /*R*/, 1.0 /*G*/, 1.0 /*B*/,
				  (float) xf->attributes.front.shininess
				  /* specular power */) ;
    }

  _dxf_PUSH_MULTIPLY_MATRIX (PORT_CTX, xf->attributes.mm) ;

  if (approx == approx_box)
      ret = _dxfBoundingBoxDraw (portHandle, xf, 0) ;

  else if (approx == approx_dots && xf->colorsDep != dep_connections)
      ret = _dxfUnconnectedPointDraw (portHandle, xf, buttonUp) ;
  
  else
      /*
       *  All primitives must handle line and connection-dependent dot
       *  approximations themselves.
       */
      switch (xf->connectionType)
	{
	case ct_none:
	  DXMARK("> dots:");
	  ret = _dxfUnconnectedPointDraw (portHandle, xf, buttonUp) ;
	  DXMARK("< dots:");
	  break;
	case ct_polylines:
	  DXMARK("> polylines:");
	  ret = _dxfPolylineDraw (portHandle, xf, buttonUp) ;
	  DXMARK("< polylines:");
	  break;
	case ct_lines:
	  DXMARK("> lines:");
	  ret = _dxfLineDraw (portHandle, xf, buttonUp) ;
	  DXMARK("< lines:");
	  break;
	case ct_tetrahedra:
	  DXMARK("> tetras:");
	  ret = _dxfTetraDraw (portHandle, xf, buttonUp) ;
	  DXMARK("< tetras:");
	  break;
	case ct_cubes:
	  DXMARK("> cubes:");
	  ret = _dxfCubeDraw (portHandle, xf, buttonUp) ;
	  DXMARK("< cubes:");
	  break;
	case ct_triangles:
	  DXMARK("> triangles:");
	  ret = _dxfTriDraw (portHandle, xf, buttonUp) ;
	  DXMARK("< triangles:");
	  break;
	case ct_quads:
	  DXMARK("> quads:");
	  ret = _dxfQuadDraw (portHandle, xf, buttonUp) ;
	  DXMARK("< quads:");
	  break;
	case ct_pline:
	  if (density == 1 &&
	      !(xf->colorsDep == dep_connections)) {
	    DXMARK("> plines:");
	    ret = _dxfPlineDraw (portHandle, xf, buttonUp) ;
	    DXMARK("< plines:");
	  } else {
	    DXMARK("> lines:");
	    ret = _dxfLineDraw (portHandle, xf, buttonUp) ;
	    DXMARK("< lines:");
	  }
	  break;
	case ct_tmesh:
	  if (density == 1 &&
	      approx != approx_dots &&
	      !(approx == approx_lines && xf->colorsDep == dep_connections)) {
	    DXMARK("> tmesh:");
	    ret = _dxfTmeshDraw (portHandle, xf, buttonUp) ;
	    DXMARK("< tmesh:");
	  } else {
	    DXMARK("> triangles:");
	    ret = _dxfTriDraw (portHandle, xf, buttonUp) ;
	    DXMARK("< triangles:");
	  }
	  break;
	case ct_qmesh:
	  if (density == 1 &&
	      approx != approx_dots &&
	      !(approx == approx_lines && xf->colorsDep == dep_connections)) {
	    DXMARK("> qmesh:");
	    ret = _dxfQmeshDraw (portHandle, xf, buttonUp) ;
	    DXMARK("< qmesh:");
	  } else {
	    DXMARK("> quads:");
	    ret = _dxfQuadDraw (portHandle, xf, buttonUp) ;
	    DXMARK("< quads:");
	  }
	  break;
	default:
	  DPRINT("\nunknown connection type") ;
	  DXSetError (ERROR_INTERNAL, "#13170") ;
	  ret = 0 ;
	  break ;
	}
 done:
  _dxf_POP_MATRIX(PORT_CTX) ;
  DPRINT(")") ;
  return ret ;
}
