/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/*********************************************************************/
/*                     I.B.M. CONFIENTIAL                            */
/*********************************************************************/

#include <dxconfig.h>


/*---------------------------------------------------------------------------*\
 $Source: /src/master/dx/src/exec/hwrender/starbase/hwTetraDrawSB.c,v $
  Author: Mark Hood
\*---------------------------------------------------------------------------*/

#define _HP_FAST_MACROS 1
#include <starbase.c.h>
#include "hwDeclarations.h"
#include "hwPortSB.h"
#include "hwMemory.h"
#include "hwCacheUtilSB.h"

#include "hwDebug.h"

#define Polyhedra                 Tetra
#define CpfPolyhedra             "CpfTetra"
#define CpcPolyhedra             "CpcTetra"
#define CpvPolyhedra             "CpvTetra"
#define tdmPolyhedraDraw          _dxfTetraDraw

#define PolyhedraSize             4
#define FacetsPerPolyhedron       4
#define VerticesPerFacet          3

#include "hwPolyhedraDrawSB.c.h"
