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
 $Source: /src/master/dx/src/exec/hwrender/xgl/hwQmeshDraw.c,v $

 $Log: hwQmeshDraw.c,v $
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

 Revision 10.1  1999/02/23 21:03:03  gda
 OpenDX Baseline

 Revision 9.1  1997/05/22 22:34:06  svs
 Copy of release 3.1.4 code

 * Revision 8.0  1995/10/03  22:15:43  nsc
 * Copy of release 3.1 code
 *
 * Revision 7.2  1994/03/31  15:55:23  tjm
 * added versioning
 *
 * Revision 7.1  94/01/18  19:00:10  svs
 * changes since release 2.0.1
 * 
 * Revision 6.1  93/11/16  10:26:14  svs
 * ship level code, release 2.0
 * 
 * Revision 1.2  93/07/21  21:37:43  mjh
 * #include "hwXfield.h"
 * 
 * Revision 1.1  93/06/29  10:01:41  tjm
 * Initial revision
 * 
 * Revision 5.2  93/04/30  23:44:33  mjh
 * use same code as hwTmeshDraw.c
 * 
 *
\*---------------------------------------------------------------------------*/
  
#include "hwDeclarations.h"
#include "hwPortXGL.h"
#include "hwPortLayer.h"
#include "hwXfield.h"
#include "hwMemory.h"
#include "hwQmesh.h"
#include "hwCacheUtilXGL.h"

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
_dxf_get_qmesh (Field f, int ntriangles, int *triangles,
		int npoints, float *points) ;

#include "hwMeshDraw.c.h"
