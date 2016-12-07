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


/*
 */

#define _HP_FAST_MACROS 1
#include <starbase.c.h>
#include "hwDeclarations.h"
#include "hwPortSB.h"
#include "hwXfield.h"
#include "hwMemory.h"

#include "hwDebug.h"

void
_dxfUnconnectedPointDraw (tdmPortHandleP portHandle, xfieldT *xf, int buttonUp)
{
  register Point *points ;
  register RGBColor *fcolors, *color_map ;
  register int i, mod, num ;
  int type, rank, shape, is_2d ;
  struct p2d {float x, y ;} *pnts2d ;

  DEFPORT(portHandle) ;

  ENTRY(("_dxfUnconnectedPointDraw(0x%x, 0x%x, %d)", portHandle, xf, buttonUp));
  PRINT(("%d invalid positions",
	 xf->invPositions? DXGetInvalidCount(xf->invPositions): 0));
  
  /*
   *  Extract required data from the xfield.
   */
 
  if (is_2d = IS_2D (xf->positions_array, type, rank, shape))
      pnts2d = (struct p2d *) DXGetArrayData(xf->positions_array) ;
  else
      points = (Point *) DXGetArrayData(xf->positions_array) ;

  color_map = (RGBColor *) DXGetArrayData(xf->cmap_array) ;
  
  if (DXGetArrayClass(xf->fcolors_array) == CLASS_CONSTANTARRAY)
      fcolors = (Pointer) DXGetArrayEntry(xf->fcolors, 0, NULL) ;
  else
      fcolors = (Pointer) DXGetArrayData(xf->fcolors_array) ;
  
  mod = buttonUp ? xf->attributes.buttonUp.density :
	           xf->attributes.buttonDown.density ;

  marker_type (FILDES, 0) ;
  hidden_surface (FILDES, TRUE, FALSE) ;

  if (xf->colorsDep == dep_field && mod == 1 && !xf->invPositions)
    {
      /* fast path */
      SET_COLOR (marker_color, 0) ;
      if (is_2d)
	  polymarker2d (FILDES, (float *)pnts2d, xf->npositions, 0) ;
      else
	  polymarker3d (FILDES, (float *)points, xf->npositions, 0) ;
    }
  else
    {
      num = xf->npositions ;
      if (xf->colorsDep == dep_field)
	{
	  SET_COLOR (marker_color, 0) ;
	  if (xf->invPositions)
	      if (is_2d)
		  for (i=0 ; i<num ; i+=mod)
		    {
		      if (DXIsElementValid (xf->invPositions, i))
			  polymarker2d (FILDES, (float *)(pnts2d+i), 1, 0) ;
		    }
	      else
		  for (i=0 ; i<num ; i+=mod)
		    {
		      if (DXIsElementValid (xf->invPositions, i))
			  polymarker3d (FILDES, (float *)(points+i), 1, 0) ;
		    }
	  else
	      if (is_2d)
		  for (i=0 ; i<num ; i+=mod)
		      polymarker2d (FILDES, (float *)(pnts2d+i), 1, 0) ;
	      else
		  for (i=0 ; i<num ; i+=mod)
		      polymarker3d (FILDES, (float *)(points+i), 1, 0) ;
	}
      else if (color_map)
	  if (xf->invPositions)
	      if (is_2d)
		  for (i=0 ; i<num ; i+=mod)
		    {
		      if (DXIsElementValid (xf->invPositions, i))
			{
			  SET_CMAP_COLOR (marker_color, i) ;
			  polymarker2d (FILDES, (float *)(pnts2d+i), 1, 0) ;
			}
		    }
	      else
		  for (i=0 ; i<num ; i+=mod)
		    {
		      if (DXIsElementValid (xf->invPositions, i))
			{
			  SET_CMAP_COLOR (marker_color, i) ;
			  polymarker3d (FILDES, (float *)(points+i), 1, 0) ;
			}
		    }
	  else
	      if (is_2d)
		  for (i=0 ; i<num ; i+=mod)
		    {
		      SET_CMAP_COLOR (marker_color, i) ;
		      polymarker2d (FILDES, (float *)(pnts2d+i), 1, 0) ;
		    }
	      else
		  for (i=0 ; i<num ; i+=mod)
		    {
		      SET_CMAP_COLOR (marker_color, i) ;
		      polymarker3d (FILDES, (float *)(points+i), 1, 0) ;
		    }
      else
	  if (xf->invPositions)
	      if (is_2d)
		  for (i=0 ; i<num ; i+=mod)
		    {
		      if (DXIsElementValid (xf->invPositions, i))
			{
			  SET_CDIR_COLOR (marker_color, i) ;
			  polymarker2d (FILDES, (float *)(pnts2d+i), 1, 0) ;
			}
		    }
	      else
		  for (i=0 ; i<num ; i+=mod)
		    {
		      if (DXIsElementValid (xf->invPositions, i))
			{
			  SET_CDIR_COLOR (marker_color, i) ;
			  polymarker3d (FILDES, (float *)(points+i), 1, 0) ;
			}
		    }
	  else
	      if (is_2d)
		  for (i=0 ; i<num ; i+=mod)
		    {
		      SET_CDIR_COLOR (marker_color, i) ;
		      polymarker2d (FILDES, (float *)(pnts2d+i), 1, 0) ;
		    }
	      else
		  for (i=0 ; i<num ; i+=mod)
		    {
		      SET_CDIR_COLOR (marker_color, i) ;
		      polymarker3d (FILDES, (float *)(points+i), 1, 0) ;
		    }
    }
  hidden_surface(FILDES, FALSE, FALSE) ;

  EXIT((""));
  return ;
}
