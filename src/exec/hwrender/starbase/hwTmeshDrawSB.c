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
 $Source: /src/master/dx/src/exec/hwrender/starbase/hwTmeshDrawSB.c,v $
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
#include "hwTmesh.h"

#include "hwDebug.h"

#define tdmMeshDraw      _dxfTmeshDraw
#define tdmPolygonDraw   _dxfTriDraw
#define tdm_get_mesh     _dxf_get_tmesh
#define Mesh             Tmesh
#define mesh             tmesh
#define CpfMsh          "CpfTmsh"
#define CpcMsh          "CpcTmsh"
#define CpvMsh          "CpvTmsh"
#define Strip            Tstrip
#define strip            tstrip
#define strips           tstrips

#include "hwMeshDrawSB.c.h"
