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
 $Source: /src/master/dx/src/exec/hwrender/xgl/hwCacheUtilXGL.h,v $
  Author: Mark Hood

  Data structures and prototypes for caching Starbase primitives.

 $Log: hwCacheUtilXGL.h,v $
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

 Revision 10.1  1999/02/23 21:01:44  gda
 OpenDX Baseline

 Revision 9.1  1997/05/22 22:32:23  svs
 Copy of release 3.1.4 code

 * Revision 8.0  1995/10/03  22:14:22  nsc
 * Copy of release 3.1 code
 *
 * Revision 7.3  1994/03/31  15:55:09  tjm
 * added versioning
 *
 * Revision 7.2  94/02/24  18:31:33  mjh
 * sun4/xgl maintenance: part 3, remove references to original DX field in
 * the xfield data structure.  XGL objects are now cached on the address of
 * the xfield.  XGL port now no longer requires STUBBED_CALLS preprocessor
 * definition.
 * 
 * Revision 7.1  94/01/18  18:59:45  svs
 * changes since release 2.0.1
 * 
 * Revision 6.1  93/11/16  10:25:50  svs
 * ship level code, release 2.0
 * 
 * Revision 1.2  93/07/28  19:34:21  mjh
 * apply 2D position hacks, install invalid connection/postion support
 * 
 * Revision 1.1  93/06/29  10:01:19  tjm
 * Initial revision
 * 
 * Revision 5.4  93/05/07  19:57:04  mjh
 * add polygon cache
 * 
 * Revision 5.3  93/05/03  18:25:09  mjh
 * add polygon cache, update names tdm -> dxf
 * 
 * Revision 5.2  93/04/30  23:42:29  mjh
 * Revise some confusing names taken from the Starbase implementation.
 * 
 * Revision 5.1  93/03/30  14:41:56  ellen
 * Moved these files from the 5.0.2 Branch
 * 
 * Revision 5.0.2.2  93/01/18  11:10:36  john
 * initial XGL revision
 * 
 * Revision 5.0  93/01/17  16:45:59  owens
 * added caching of tmesh xgl data
\*---------------------------------------------------------------------------*/

#include "hwTmesh.h"
#include "hwXfield.h"
#include "xgl/xgl.h"

typedef struct
{
  int num_strips ;
  Xgl_pt_list *pt_lists ;
  Xgl_facet_list *facet_lists ;
} tdmStripDataXGL ;

tdmStripDataXGL *
_dxf_GetTmeshCacheXGL (char *fun, xfieldT *f) ;

void
_dxf_PutTmeshCacheXGL (char *fun, xfieldT *f, tdmStripDataXGL *stripsXGL) ;

Error
_dxf_FreeTmeshCacheXGL (Pointer p) ;


char *
_dxf_GetPolyCacheXGL (char *fun, int mod, xfieldT *f) ;

void
_dxf_PutPolyCacheXGL (char *fun, int mod, xfieldT *f, char *polysXGL) ;

Error
_dxf_FreePolyCacheXGL (Pointer p) ;


char *
_dxf_GetLineCacheXGL (char *fun, int mod, xfieldT *f) ;

void
_dxf_PutLineCacheXGL (char *fun, int mod, xfieldT *f, char *linesXGL) ;

Error
_dxf_FreeLineCacheXGL (Pointer p) ;


Xgl_pt_list *
_dxf_GetPointCacheXGL (char *fun, int mod, xfieldT *f) ;

void
_dxf_PutPointCacheXGL (char *fun, int mod, xfieldT *f, Xgl_pt_list *pt_list) ;

Error
_dxf_FreePointCacheXGL (Pointer p) ;
