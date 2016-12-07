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
 $Source: /src/master/dx/src/exec/hwrender/xgl/hwTetraDraw.c,v $
 $Log: hwTetraDraw.c,v $
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

 Revision 10.1  1999/02/23 21:03:09  gda
 OpenDX Baseline

 Revision 9.1  1997/05/22 22:34:18  svs
 Copy of release 3.1.4 code

 * Revision 8.0  1995/10/03  22:15:52  nsc
 * Copy of release 3.1 code
 *
 * Revision 7.2  1994/03/31  15:55:25  tjm
 * added versioning
 *
 * Revision 7.1  94/01/18  19:00:14  svs
 * changes since release 2.0.1
 * 
 * Revision 6.1  93/11/16  10:26:19  svs
 * ship level code, release 2.0
 * 
 * Revision 1.1  93/06/29  10:01:45  tjm
 * Initial revision
 * 
 * Revision 5.4  93/05/13  22:51:39  mjh
 * reimplement to fix wireframe and merge cube and tetra
 * 
\*---------------------------------------------------------------------------*/

#include <stdio.h>
#include <math.h>
#include <xgl/xgl.h>

#include "hwDeclarations.h"
#include "hwPortXGL.h"
#include "hwMemory.h"
#include "hwCacheUtilXGL.h"

#define Polyhedra                 Tetra
#define CpfPolyhedra             "CpfTetra"
#define CpcPolyhedra             "CpcTetra"
#define CpvPolyhedra             "CpvTetra"
#define _dxfPolyhedraDraw        _dxfTetraDraw

#define VerticesPerPolyhedron     4
#define FacetsPerPolyhedron       4
#define VerticesPerFacet          3

#define IGNORE 0
#define FacetFlags XGL_FACET_FLAG_SIDES_ARE_3 | XGL_FACET_FLAG_SHAPE_CONVEX
    
#define LoadPolyhedronPoints()         \
{                                      \
  LoadFacetPoints(IGNORE,0,1,2); ds++; \
  LoadFacetPoints(IGNORE,0,2,3); ds++; \
  LoadFacetPoints(IGNORE,0,3,1); ds++; \
  LoadFacetPoints(IGNORE,1,3,2); ds++; \
}

#define LoadPolyhedronPointsAndCmapColors()    	     \
  LoadFacetPointsAndCmapColors(IGNORE,0,1,2);  ds++; \
  LoadFacetPointsAndCmapColors(IGNORE,0,2,3);  ds++; \
  LoadFacetPointsAndCmapColors(IGNORE,0,3,1);  ds++; \
  LoadFacetPointsAndCmapColors(IGNORE,1,3,2);  ds++;

#define LoadPolyhedronPointsAndCdirColors()          \
  LoadFacetPointsAndCdirColors(IGNORE,0,1,2);  ds++; \
  LoadFacetPointsAndCdirColors(IGNORE,0,2,3);  ds++; \
  LoadFacetPointsAndCdirColors(IGNORE,0,3,1);  ds++; \
  LoadFacetPointsAndCdirColors(IGNORE,1,3,2);  ds++;

#include "hwPolyhedraDrawXGL.c.h"
