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
 $Source: /src/master/dx/src/exec/hwrender/xgl/hwCubeDraw.c,v $
 $Log: hwCubeDraw.c,v $
 Revision 1.3  1999/05/10 15:45:39  gda
 Copyright message

 Revision 1.3  1999/05/10 15:45:39  gda
 Copyright message

 Revision 1.2  1999/05/03 14:06:42  gda
 moved to using dxconfig.h rather than command-line defines

 Revision 1.1.1.1  1999/03/24 15:18:36  gda
 Initial CVS Version

 Revision 1.1.1.1  1999/03/19 20:59:49  gda
 Initial CVS

 Revision 10.1  1999/02/23 21:01:46  gda
 OpenDX Baseline

 Revision 9.1  1997/05/22 22:33:04  svs
 Copy of release 3.1.4 code

 * Revision 8.0  1995/10/03  22:14:52  nsc
 * Copy of release 3.1 code
 *
 * Revision 7.2  1994/03/31  15:55:10  tjm
 * added versioning
 *
 * Revision 7.1  94/01/18  18:59:54  svs
 * changes since release 2.0.1
 * 
 * Revision 6.1  93/11/16  10:25:59  svs
 * ship level code, release 2.0
 * 
 * Revision 1.1  93/06/29  10:01:22  tjm
 * Initial revision
 * 
 * Revision 5.3  93/05/13  22:51:12  mjh
 * reimplement to fix wireframe and merge cube and tetra
 * 
\*---------------------------------------------------------------------------*/

#include <stdio.h>
#include <math.h>
#include <xgl/xgl.h>

#include "hwDeclarations.h"
#include "hwWindow.h"
#include "hwPortXGL.h"
#include "hwMemory.h"
#include "hwCacheUtilXGL.h"

#define Polyhedra                 Cube
#define CpfPolyhedra             "CpfCube"
#define CpcPolyhedra             "CpcCube"
#define CpvPolyhedra             "CpvCube"
#define _dxfPolyhedraDraw        _dxfCubeDraw

#define VerticesPerPolyhedron     8
#define FacetsPerPolyhedron       6
#define VerticesPerFacet          4

#define FacetFlags XGL_FACET_FLAG_SIDES_ARE_4 | XGL_FACET_FLAG_SHAPE_CONVEX

#define LoadPolyhedronPoints()      \
{                                   \
  LoadFacetPoints(0,1,3,2); ds++;   \
  LoadFacetPoints(1,5,7,3); ds++;   \
  LoadFacetPoints(5,4,6,7); ds++;   \
  LoadFacetPoints(6,2,0,4); ds++;   \
  LoadFacetPoints(0,4,5,1); ds++;   \
  LoadFacetPoints(3,2,6,7); ds++;   \
}

#define LoadPolyhedronPointsAndCmapColors()    	 \
  LoadFacetPointsAndCmapColors(0,1,3,2);  ds++;	 \
  LoadFacetPointsAndCmapColors(1,5,7,3);  ds++;	 \
  LoadFacetPointsAndCmapColors(5,4,6,7);  ds++;	 \
  LoadFacetPointsAndCmapColors(6,2,0,4);  ds++;	 \
  LoadFacetPointsAndCmapColors(0,4,5,1);  ds++;	 \
  LoadFacetPointsAndCmapColors(3,2,6,7);  ds++;

#define LoadPolyhedronPointsAndCdirColors()      \
  LoadFacetPointsAndCdirColors(0,1,3,2);  ds++;	 \
  LoadFacetPointsAndCdirColors(1,5,7,3);  ds++;	 \
  LoadFacetPointsAndCdirColors(5,4,6,7);  ds++;	 \
  LoadFacetPointsAndCdirColors(6,2,0,4);  ds++;	 \
  LoadFacetPointsAndCdirColors(0,4,5,1);  ds++;	 \
  LoadFacetPointsAndCdirColors(3,2,6,7);  ds++;

#include "hwPolyhedraDrawXGL.c.h"
