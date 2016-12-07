/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


#define _HP_FAST_MACROS 1
#include <starbase.c.h>
#include "hwDeclarations.h"
#include "hwTmesh.h"
#include "hwPortSB.h"
#include "hwXfield.h"
#include "hwCacheUtilSB.h"
#include "hwMemory.h"

#include "hwDebug.h"

#ifdef DEBUG
#define PrintBounds()                                                         \
{									      \
    int i;								      \
    float outx, outy, outz;						      \
    float minx, maxx, miny, maxy, minz, maxz;				      \
									      \
    minx = miny = minz = +MAXFLOAT;					      \
    maxx = maxy = maxz = -MAXFLOAT;					      \
									      \
    if (is_2d)								      \
    {									      \
        minz = maxz = 0;						      \
        fprintf (stderr, "\n2-dimensional positions");                        \
        for (i=0; i<xf->npositions; i++)    				      \
	{								      \
	    if (pnts2d[i].x < minx) minx = pnts2d[i].x;			      \
	    if (pnts2d[i].y < miny) miny = pnts2d[i].y;			      \
	  								      \
	    if (pnts2d[i].x > maxx) maxx = pnts2d[i].x;			      \
	    if (pnts2d[i].y > maxy) maxy = pnts2d[i].y;			      \
	}								      \
    }									      \
    else								      \
        for (i=0; i<xf->npositions; i++)    				      \
	{								      \
	    if (points[i].x < minx) minx = points[i].x;			      \
	    if (points[i].y < miny) miny = points[i].y;			      \
	    if (points[i].z < minz) minz = points[i].z;			      \
								              \
	    if (points[i].x > maxx) maxx = points[i].x;			      \
	    if (points[i].y > maxy) maxy = points[i].y;			      \
	    if (points[i].z > maxz) maxz = points[i].z;			      \
	}								      \
									      \
    transform_point(FILDES, MC_TO_VDC, minx, miny, minz, &outx, &outy, &outz);\
    fprintf(stderr, "\nmin MC->VDC %9f %9f %9f -> %9f %9f %9f",		      \
	  minx, miny, minz, outx, outy, outz);				      \
									      \
    transform_point(FILDES, MC_TO_VDC, maxx, maxy, maxz, &outx, &outy, &outz);\
    fprintf(stderr, "\nmax MC->VDC %9f %9f %9f -> %9f %9f %9f",		      \
	  maxx, maxy, maxz, outx, outy, outz);				      \
}
#else
#define PrintBounds() {}
#endif



int
_dxfPolylineDraw (tdmPortHandleP portHandle, xfieldT *xf, int buttonUp)
{
    register Point *points;
    register RGBColor *fcolors, *color_map;
    register float *clist = NULL;
    register int i, k, vsize, dV, *polylines, *edges;
    int num_coords, mod, prev_point, start, cOffs, vertex_flags;
    int type, rank, shape, is_2d;
    struct p2d {float x, y;} *pnts2d;
    enum approxE approx;
    int	nshapes, maxsize;
  
    DEFPORT(portHandle);

    ENTRY(("_dxfPolylineDraw(0x%x, 0x%x, %d)", portHandle, xf, buttonUp));
    
    PRINT(("%d invalid connections",
	 xf->invCntns? DXGetInvalidCount(xf->invCntns): 0));
  
    /*
     *  Extract required data from the xfield.
     */
   
    if (is_2d = IS_2D (xf->positions_array, type, rank, shape))
        pnts2d = (struct p2d *) DXGetArrayData(xf->positions_array);
    else
        points = (Point *) DXGetArrayData(xf->positions_array);

    PrintBounds();

    color_map = (RGBColor *) DXGetArrayData(xf->cmap_array);
  
    polylines = xf->polylines;
    edges     = xf->edges;
    nshapes   = xf->npolylines;
    
    if (DXGetArrayClass(xf->fcolors_array) == CLASS_CONSTANTARRAY)
        fcolors = (Pointer) DXGetArrayEntry(xf->fcolors, 0, NULL);
    else
        fcolors = (Pointer) DXGetArrayData(xf->fcolors_array);
  
    if (buttonUp)
    {
        mod = xf->attributes.buttonUp.density;
        approx = xf->attributes.buttonUp.approx;
    }
    else
    {
      mod = xf->attributes.buttonDown.density;
      approx = xf->attributes.buttonDown.approx;
    }
  
    vertex_flags = 0;
    hidden_surface (FILDES, TRUE, FALSE);
  
    switch (approx)
    {
	case approx_dots:
	case approx_none:
	case approx_lines:
	default:
            if (xf->colorsDep == dep_field)
	    {
	        num_coords = 0;
	        vsize = 3;
	        SET_COLOR(line_color, 0);
	    }
            else
	    {
	        num_coords = 3;
	        vsize = 6;
	        cOffs = 3;
	        vertex_flags = VERTEX_COLOR;
	    }

	    /*
	     * determine length of longest polyline
	     */
	    maxsize = 0;
            for (k = 0; k < nshapes; k += mod)
	    {
	        int start = polylines[k];
	        int end   = (k == nshapes-1) ? xf->nedges : polylines[k+1];
	        int knt = end - start;

	        if (xf->invCntns && !DXIsElementValid (xf->invCntns, k))
	            continue;

	        if (knt > maxsize) maxsize = knt;
	    }

	    clist = (float *)tdmAllocate(maxsize*vsize*sizeof(float));
            if (!clist) DXErrorGoto(ERROR_INTERNAL, "#13000");

      
            for (k = 0; k < nshapes; k += mod)
	    {
	        int start = polylines[k];
	        int end   = (k == nshapes-1) ? xf->nedges : polylines[k+1];
	        int e, knt = end - start;

	        /* skip invalid connections */
	        if (xf->invCntns && !DXIsElementValid (xf->invCntns, k))
	            continue;

		/* copy vertex coordinates into clist */
		if (is_2d)
		    for (i = start, dV = 0; i < end; i++, dV += vsize)
		    {
			*(struct p2d *)(clist+dV) = pnts2d[edges[i]];
			((Point *)(clist+dV))->z = 0;
		    }
		else
		    for (i = start, dV = 0; i < end; i++, dV += vsize)
			*(Point *)(clist+dV) = points[edges[i]];
	  
		if (xf->colorsDep == dep_positions)
		{
		    if (color_map)
			for (i = start, dV = cOffs; i < end; i++, dV += vsize)
			    *(RGBColor *)(clist+dV) =
				color_map[((char *)fcolors)[edges[i]]];
		    else
			for (i = start, dV = cOffs; i < end; i++, dV += vsize)
			    *(RGBColor *)(clist+dV) = fcolors[edges[i]];
		}
		else if (xf->colorsDep == dep_polylines)
		{
		    if (color_map)
			for (i = start, dV = cOffs; i < end; i++, dV += vsize)
			    *(RGBColor *)(clist+dV) =
				color_map[((char *)fcolors)[k]];
		    else
			for (i = start, dV = cOffs; i < end; i++, dV += vsize)
			    *(RGBColor *)(clist+dV) = fcolors[k];
		}

		polyline_with_data3d(FILDES, clist, knt,
				num_coords, vertex_flags, NULL);
	    }
            break;
    }

    if (clist) tdmFree(clist);
    hidden_surface (FILDES, FALSE, FALSE);
    EXIT(("OK"));
    return OK;

error:
    if (clist) tdmFree(clist);
    hidden_surface (FILDES, FALSE, FALSE);
    EXIT(("ERROR"));
    return ERROR;
}
