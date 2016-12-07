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
 $Source: /src/master/dx/src/exec/hwrender/xgl/hwBoundingBoxDraw.c,v $
 $Log: hwBoundingBoxDraw.c,v $
 Revision 1.3  1999/05/10 15:45:39  gda
 Copyright message

 Revision 1.3  1999/05/10 15:45:39  gda
 Copyright message

 Revision 1.2  1999/05/03 14:06:42  gda
 moved to using dxconfig.h rather than command-line defines

 Revision 1.1.1.1  1999/03/24 15:18:35  gda
 Initial CVS Version

 Revision 1.1.1.1  1999/03/19 20:59:49  gda
 Initial CVS

 Revision 10.1  1999/02/23 21:01:39  gda
 OpenDX Baseline

 Revision 9.1  1997/05/22 22:32:52  svs
 Copy of release 3.1.4 code

 * Revision 8.0  1995/10/03  22:14:44  nsc
 * Copy of release 3.1 code
 *
 * Revision 7.3  1994/03/31  15:55:00  tjm
 * added versioning
 *
 * Revision 7.2  94/02/25  21:48:53  mjh
 * xgl/sun4 maintenance: part 4, use xfield instead of DX field for 
 * bounding box.
 * 
 * Revision 7.1  94/01/18  18:59:51  svs
 * changes since release 2.0.1
 * 
 * Revision 6.1  93/11/16  10:25:56  svs
 * ship level code, release 2.0
 * 
 * Revision 1.1  93/06/29  10:01:01  tjm
 * Initial revision
 * 
 * Revision 5.2  93/05/06  18:40:52  ellen
 * Changed DXWarning, DXError*, and DXSetError with code numbers.
 * 
 * Revision 5.1  93/03/30  14:41:06  ellen
 * Moved these files from the 5.0.2 Branch
 * 
 * Revision 5.0.2.1  93/02/24  18:59:43  mjh
 * initial revision
 * 
\*---------------------------------------------------------------------------*/

#include <xgl/xgl.h>
#include "hwDeclarations.h"
#include "hwPortXGL.h"
#include "hwXfield.h"

int
_dxfBoundingBoxDraw (tdmPortHandleP portHandle, xfieldP xf, int clip_status)
{
  static int faceverts[] = {0, 1, 3, 2, 0, 4, 5, 7, 6, 4} ;
  Xgl_color color ;
  Xgl_pt_list line ;
  Xgl_pt_f3d pos[24] ;
  register Xgl_pt_f3d *p ;
  register int i ;
  DEFPORT(portHandle) ;
  
  DPRINT("\n(_dxfBoundingBoxDraw") ;
  if (clip_status)
    {
      DPRINT("\nclipping unimplemented)") ;
      return 0 ;
    }
  
  color.rgb.r = color.rgb.g = color.rgb.b = 1.0 ;
  xgl_object_set (XGLCTX, XGL_CTX_LINE_COLOR, &color, NULL) ;

  line.pt_type = XGL_PT_F3D ;
  line.bbox = NULL ;
  line.pts.f3d = p = pos ;
  
#define ENDLINE {if (p != pos) xgl_multipolyline(XGLCTX,NULL,1,&line); p=pos;}
#define MOVE(vertex) {ENDLINE; *p=*(Xgl_pt_f3d *)vertex; line.num_pts=1;}
#define DRAW(vertex) {*++p=*(Xgl_pt_f3d *)vertex; line.num_pts++;}

  MOVE(&xf->box[faceverts[0]]) ;
  for (i=1 ; i<10 ; i++)
      DRAW(&xf->box[faceverts[i]]) ;

  MOVE(&xf->box[2]) ; DRAW(&xf->box[6]) ;
  MOVE(&xf->box[3]) ; DRAW(&xf->box[7]) ;
  MOVE(&xf->box[1]) ; DRAW(&xf->box[5]) ;
  ENDLINE ;
  
  DPRINT(")") ;
  return 1 ;
}
