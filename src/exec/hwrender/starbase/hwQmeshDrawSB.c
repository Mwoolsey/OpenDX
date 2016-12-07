/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


/*---------------------------------------------------------------------------*\
 $Source: /src/master/dx/src/exec/hwrender/starbase/hwQmeshDrawSB.c,v $
  Author: Mark Hood

\*---------------------------------------------------------------------------*/

#define _HP_FAST_MACROS 1
#include <starbase.c.h>
#include "hwDeclarations.h"
#include "hwPortSB.h"
#include "hwXfield.h"
#include "hwCacheUtilSB.h"
#include "hwPortLayer.h"
#include "hwMemory.h"
#include "hwQmesh.h"

#include "hwDebug.h"

#define tdmMeshDraw      _dxfQmeshDraw
#define tdmPolygonDraw   _dxfQuadDraw
#define tdm_get_mesh     _dxf_get_qmesh
#define Mesh             Qmesh
#define mesh             qmesh
#define CpfMsh          "CpfQmsh"
#define CpcMsh          "CpcQmsh"
#define CpvMsh          "CpvQmsh"
#define Strip            Qstrip
#define strip            qstrip
#define strips           qstrips

Qmesh *
_dxf_get_qmesh (Field f, 
		int ntriangles, int *triangles,
		int npoints, float *points) ;

#include "hwMeshDrawSB.c.h"
