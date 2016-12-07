/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


#ifndef tdmView_h
#define tdmView_h

/*---------------------------------------------------------------------------*\
 $Source: /src/master/dx/src/exec/hwrender/hwView.h,v $
  Author: Tim Murphy

  This file contains definitions for the 'view' structure. This structure
  represents a DX  object which has been converted to a form which is more
  pallatable to the render(s).

\*---------------------------------------------------------------------------*/

viewO _dxf_newHwView();
viewO _dxf_getHwViewInfo(viewO vo,
	    int *flags, gatherO *gather, translationO *translation,
	    float *projection, float *view, RGBColor *background);
viewO _dxf_setHwViewFlags(viewO vo, int flags);
viewO _dxf_setHwViewGather(viewO vo, gatherO gather);
viewO _dxf_setHwViewTranslation(viewO vo, translationO translation);
viewO _dxf_setHwViewProjectionMatrix(viewO vo, float projectionMatrix[4][4]);
viewO _dxf_setHwViewViewMatrix(viewO vo, float viewMatrix[4][4]);
viewO _dxf_setHwViewBackground(viewO vo, RGBColor background);

#endif /* tdmView_h */
