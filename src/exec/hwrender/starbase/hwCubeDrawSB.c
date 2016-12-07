/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


/*---------------------------------------------------------------------------*\
 $Source: /src/master/dx/src/exec/hwrender/starbase/hwCubeDrawSB.c,v $
  Author: Mark Hood

\*---------------------------------------------------------------------------*/

#define _HP_FAST_MACROS 1
#include <starbase.c.h>
#include "hwDeclarations.h"
#include "hwPortSB.h"
#include "hwMemory.h"
#include "hwCacheUtilSB.h"

#include "hwDebug.h"

#define Polyhedra                 Cube
#define CpfPolyhedra             "CpfCube"
#define CpcPolyhedra             "CpcCube"
#define CpvPolyhedra             "CpvCube"
#define tdmPolyhedraDraw          _dxfCubeDraw

#define PolyhedraSize             8
#define FacetsPerPolyhedron       6
#define VerticesPerFacet          4

#include "hwPolyhedraDrawSB.c.h"
