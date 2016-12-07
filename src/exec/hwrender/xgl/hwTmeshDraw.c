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
 $Source: /src/master/dx/src/exec/hwrender/xgl/hwTmeshDraw.c,v $

 $Log: hwTmeshDraw.c,v $
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

 Revision 10.1  1999/02/23 21:03:10  gda
 OpenDX Baseline

 Revision 9.1  1997/05/22 22:34:23  svs
 Copy of release 3.1.4 code

 * Revision 8.0  1995/10/03  22:15:56  nsc
 * Copy of release 3.1 code
 *
 * Revision 7.2  1994/03/31  15:55:26  tjm
 * added versioning
 *
 * Revision 7.1  94/01/18  19:00:16  svs
 * changes since release 2.0.1
 * 
 * Revision 6.1  93/11/16  10:26:20  svs
 * ship level code, release 2.0
 * 
 * Revision 1.2  93/07/21  21:37:34  mjh
 * #include "hwXfield.h"
 * 
 * Revision 1.1  93/06/29  10:01:47  tjm
 * Initial revision
 * 
 * Revision 5.2  93/04/30  23:43:38  mjh
 * Implement wireframe and dot approximation styles.  The HOLLOW fill style
 * seems to have lots of problems in xgl.
 * 
 * Revision 5.1  93/03/30  14:41:34  ellen
 * Moved these files from the 5.0.2 Branch
 *
 * Revision 5.0.2.10  93/03/29  15:46:26  ellen
 * Added code to check for "out of memory".
 *
 * Revision 5.0.2.9  93/02/24  17:57:49  owens
 * *** empty log message ***
 *
 * Revision 5.0.2.8  93/01/17  16:44:56  owens
 * added caching of xgl data
 *
 * Revision 5.0.2.7  93/01/13  19:17:20  mjh
 * more XGL-specific
 *
 * Revision 5.0.2.6  93/01/13  17:40:14  mjh
 * add XGL_ILLUM_PER_VERTEX to color-per-field code
 *
 * Revision 5.0.2.5  93/01/13  16:56:43  mjh
 * hacked from Starbase mesh code.
 *
\*---------------------------------------------------------------------------*/

#include "hwDeclarations.h"
#include "hwPortXGL.h"
#include "hwXfield.h"
#include "hwPortLayer.h"
#include "hwMemory.h"
#include "hwTmesh.h"
#include "hwCacheUtilXGL.h"

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

#include "hwMeshDraw.c.h"
