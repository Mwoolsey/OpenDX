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
 $Source: /src/master/dx/src/exec/hwrender/starbase/hwQuadDrawSB.c,v $
  Author: Mark Hood

\*---------------------------------------------------------------------------*/

#define _HP_FAST_MACROS 1
#include <starbase.c.h>
#include "hwDeclarations.h"
#include "hwPortSB.h"
#include "hwXfield.h"
#include "hwMemory.h"
#include "hwCacheUtilSB.h"

#include "hwDebug.h"

#define Polygon                 Quad
#define PolygonSize             4
#define CpfPolygon             "CpfQuad"
#define CpcPolygon             "CpcQuad"
#define CpvPolygon             "CpvQuad"
#define tdmPolygonDraw          _dxfQuadDraw

#include "hwPolygonDrawSB.c.h"
