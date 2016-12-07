/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/*
 * (C) COPYRIGHT International Business Machines Corp. 1997
 * All Rights Reserved
 * Licensed Materials - Property of IBM
 *
 * US Government Users Restricted Rights - Use, duplication or
 * disclosure restricted by GSA ADP Schedule Contract with IBM Corp.
 */

#include <dxconfig.h>

#if defined(HAVE_STRING_H)
#include <string.h>
#endif

#include <dx/dx.h>
#include <stdlib.h>
#include "color.h"
#include "plot.h"
#include "_construct.h"

#define MINTICPIX 5;

static RGBColor DEFAULTTICCOLOR = {0.3, 0.3, 0.3};
static RGBColor DEFAULTLABELCOLOR = {1.0, 1.0, 1.0};


Error
m_Legend(Object *in, Object *out)
{
  char *namestring=NULL;
  int numstrings, numcolors, rank, shapearray[32], adjust, horizontal;
  int frame, axeslines=1, i, *col_ptr_i=NULL, intzero=0;
  int ticsx=0, ticsy=0;
  float *actualcorners = NULL, labelscale, *col_ptr_f=NULL;
  float scaleheight, scalewidth, minticsize; 
  float *locs_ptr=NULL; 
  Type type, colortype;
  Category category;
  Pointer axeshandle=NULL;
  char extralabel[80];
  char *nullstring="", *fontname, *colorstring, *label; 
  Object outo, o, oo, ooo, ob, newo, outscreen;
  Array a_newpositions=NULL, a_newcolors=NULL, a_newconnections=NULL; 
  Array stringlist=NULL;
  Array locs=NULL, color_list=NULL, index_list=NULL; 
  float *index_list_ptr;
  Class class;
  Point translation, box[8], min, max;
  RGBColor ticcolor, labelcolor, framecolor, color;

  typedef struct {
    float height, width, thickness;
  }  Shape;
  Shape shape;

 
  strcpy(extralabel,nullstring);
  adjust = 0; 
  outo = NULL;
  o = NULL;
  oo = NULL;


  if (!in[0])
    DXErrorGoto(ERROR_BAD_PARAMETER, "missing string_list");
  class = DXGetObjectClass(in[0]); 
  if (!DXExtractNthString(in[0],0,&namestring))
    DXErrorGoto(ERROR_BAD_PARAMETER,"string_list must be a string list");
  if (DXGetObjectClass(in[0])==CLASS_STRING) {
    numstrings = 1;
    stringlist = DXNewArray(TYPE_STRING, CATEGORY_REAL, 1, 
                            strlen(DXGetString((String)in[0]))+1);
    if (!DXAddArrayData(stringlist, 0, 1, (Pointer)DXGetString((String)in[0])))
       goto error;
  }
  else {
    if (!DXGetArrayInfo((Array)in[0], &numstrings, &type, &category, 
  		        &rank, shapearray)){
      goto error;
    }
    stringlist = _dxfReallyCopyArray((Array)in[0]);
    if (!stringlist) goto error; 
  }
  
  if (!in[1])
    DXErrorGoto(ERROR_BAD_PARAMETER, "missing color_list");
  class = DXGetObjectClass(in[1]); 
  if (class==CLASS_FIELD) {
    /* it must be a colormap. Get the list of colors */
    index_list = DXNewArray(TYPE_FLOAT, CATEGORY_REAL, 0);
    if (!DXAddArrayData(index_list, 0, numstrings, NULL))
      goto error;
    index_list_ptr = (float *)DXGetArrayData(index_list);
    if (!index_list_ptr) goto error;
    for (i=0; i<numstrings; i++) {
      index_list_ptr[i] = i;
    }
    color_list = (Array)DXMap((Object)index_list, in[1], NULL, NULL); 
    if (!DXGetArrayInfo((Array)color_list, &numcolors, &colortype, &category, 
			&rank, shapearray)){
      goto error;
    }
  }
  else {
    if (class == CLASS_STRING) {
      color_list = DXNewArray(TYPE_STRING, CATEGORY_REAL, 1, 
                              strlen(DXGetString((String)in[1])+1));
      if (!color_list) goto error;
      if (!DXAddArrayData(color_list, 0, 1, DXGetString((String)in[1]))) 
        goto error;
    }
    else if (class == CLASS_ARRAY) {
      color_list = _dxfReallyCopyArray((Array)in[1]);
      if (!color_list) goto error;
    }
    else {
      DXSetError(ERROR_BAD_PARAMETER, 
		  "color_list must be a list of rgb colors or strings, or a field");
      goto error;
    }
    if (!DXGetArrayInfo(color_list, &numcolors, &colortype, &category, 
			&rank, shapearray)){
      goto error;
    }
    if (numcolors != numstrings) {
      DXErrorGoto(ERROR_BAD_PARAMETER, 
		  "color_list and string_list must have same number of items");
    }
  }
  
  if (!in[4]) {
    horizontal = 0;
  }
  else {
    if (!(DXExtractInteger(in[4], &horizontal))) {
      DXSetError(ERROR_BAD_PARAMETER,"#10070","horizontal");
      goto error;
    }
    if ((horizontal != 0)&&(horizontal != 1)) {
      DXSetError(ERROR_BAD_PARAMETER,"#10070","horizontal");
      goto error;
    }
  }
  
  if (!in[2]) {
    if (horizontal==0) {
      translation.x = 0.95;
      translation.y = 0.95;
      translation.z = 0.0;
    }
    else {
      translation.x = 0.5;
      translation.y = 0.95;
      translation.z = 0.0;
    }
  }
  else {
    if (!DXExtractParameter(in[2], TYPE_FLOAT, 3, 1, (Pointer)&translation)) {
      if (!DXExtractParameter(in[2], TYPE_FLOAT, 2, 1, 
			      (Pointer)&translation)) {
	DXSetError(ERROR_BAD_PARAMETER,"#10231","position", 2, 3);
	goto error;
      }
    }
    translation.z = 0.0;
  }
  
  if ((translation.x < 0)||(translation.x > 1)||(translation.y<0.0)||
      (translation.y>1.0)) {
    DXSetError(ERROR_BAD_PARAMETER, "#10110", "position", 0, 1);      
    goto error;
  }
  
  if (!in[3]) {
    shape.height = 300.0;
    shape.width = 25.0;
    shape.thickness = 1.0;
  }
  else {
    if (!DXExtractParameter(in[3], TYPE_FLOAT, 3, 1, (Pointer)&shape)) {
      if (!DXExtractParameter(in[3], TYPE_FLOAT, 2, 1, (Pointer)&shape)) {
        DXSetError(ERROR_BAD_PARAMETER,"#10231","shape", 2, 3);
        goto error;
      }
      shape.thickness = 1;
    }
    
    if (shape.height <= 0.0) {
      DXSetError(ERROR_BAD_PARAMETER,"#10531","shape");
      goto error;
    }
    if (shape.width <= 0.0) {
      DXSetError(ERROR_BAD_PARAMETER,"#10531","shape");
      goto error;
    }
  }
  
  
  if (!_dxfGetColorBarAnnotationColors(in[6], in[7], DEFAULTTICCOLOR,
				       DEFAULTLABELCOLOR, &ticcolor, 
				       &labelcolor, 
				       &framecolor, &frame))
    goto error;
  
  if (frame==1) axeslines=1;
  else axeslines=0;
  
  if (in[8]) {
    if (!DXExtractFloat(in[8],&labelscale)) {
      DXSetError(ERROR_BAD_PARAMETER,"#10090","labelscale");
      goto error;
    }
    if (labelscale < 0) {
      DXSetError(ERROR_BAD_PARAMETER,"#10090","labelscale");
      goto error;
    }
  }
  else labelscale = 1;
  
  if (in[9]) {
    if (!DXExtractString(in[9],&fontname)) {
      DXSetError(ERROR_BAD_PARAMETER,"#10200","font");
      goto error;
    }
  }
  else {
    fontname = "standard";
  }
  
  a_newcolors = DXNewArray(TYPE_FLOAT, CATEGORY_REAL, 1, 3);
  if (!a_newcolors) goto error;
  
  if (colortype==TYPE_FLOAT)
    col_ptr_f = DXGetArrayData((Array)color_list);
  else if (colortype==TYPE_INT)
    col_ptr_i = DXGetArrayData((Array)color_list);
  else if (colortype!=TYPE_STRING) {
    DXSetError(ERROR_DATA_INVALID,
	       "color_list must be a list of 3-vectors or colorname strings");
    goto error;
  }
  
  locs = DXNewArray(TYPE_FLOAT, CATEGORY_REAL, 0);
  if (!DXAddArrayData(locs, 0, numstrings, NULL))
    goto error;
  locs_ptr= DXGetArrayData(locs);
  for (i=0; i<numstrings; i++)
    locs_ptr[i] = i+1; 
  
  if (!horizontal) { 
    a_newpositions = DXMakeGridPositions(3, 2, numstrings+1, 1, 
					 0.0, 0.5, 0.0, 
                                         1.0, 0.0, 0.0, 
                                         0.0, 1.0, 0.0,
                                         0.0, 0.0, 1.0);
    if (!a_newpositions)
      goto error;
  }
  else {
    a_newpositions = DXMakeGridPositions(3, numstrings+1, 2, 1, 
					 0.5, 0.0, 0.0, 
                                         1.0, 0.0, 0.0, 
                                         0.0, 1.0, 0.0,
                                         0.0, 0.0, 1.0);
    if (!a_newpositions)
      goto error;
  }
  for (i=0; i<numstrings; i++) { 
    switch (colortype) {
    case (TYPE_FLOAT):
      color = DXRGB(col_ptr_f[3*i], 
		    col_ptr_f[3*i+1], 
		    col_ptr_f[3*i+2]);
      break;
    case (TYPE_INT):
      color = DXRGB(col_ptr_i[3*i], 
		    col_ptr_i[3*i+1], 
		    col_ptr_i[3*i+2]);
      break;
    case (TYPE_STRING): 
      if (!DXExtractNthString(in[1], i, &colorstring))
	goto error;
      if (!DXColorNameToRGB(colorstring, &color))
	break;
    default:
        break;
    }
    if (!(DXAddArrayData(a_newcolors, i, 
			 1, (Pointer)&color))) {
      goto error;
    }
  }
  
  outo = (Object)DXNewField();
  
  if (!(DXSetComponentValue((Field)outo,"positions", 
			    (Object)a_newpositions))) {
    goto error;
  }
  a_newpositions=NULL;

  if (!(DXSetComponentValue((Field)outo,"colors", (Object)a_newcolors))) {
    goto error;
  }
  a_newcolors=NULL;
  if (!(DXSetComponentAttribute((Field)outo,"colors","dep",
				(Object)DXNewString("connections")))) {
    goto error;
  }
  
  if (horizontal) { 
    if (!(a_newconnections = DXMakeGridConnections(2, numstrings+1, 2))) {
      goto error;
    }
  }
  else {
    if (!(a_newconnections = DXMakeGridConnections(2, 2, numstrings+1))) {
      goto error;
    }
  }
  if (!(DXSetComponentValue((Field)outo,"connections",
			    (Object)a_newconnections))) {
    goto error;
  }
  a_newconnections=NULL;
  if (!(DXSetComponentAttribute((Field)outo, "connections", "ref", 
				(Object)DXNewString("positions")))) {
    goto error;
  }
  if (!(DXSetComponentAttribute((Field)outo, "connections", "element type", 
				(Object)DXNewString("quads")))) {
    goto error;
  }
  
  /* first get the map to start at 0.0 */ 
  /* now I know that the color map is monotonic */
  scaleheight = shape.height/numstrings; 
  scalewidth = shape.width;
  /* figure out the minimum tic size */
  minticsize = MINTICPIX;
  
  
  
  /* check the label param */
  if (!in[5]) {
    label = nullstring;
  }
  else {
    if (!DXExtractString(in[5], &label)) {
      DXSetError(ERROR_DATA_INVALID,"#10200", "label");
      goto error;
    }
  }
  
  /* get the thing into pixel units now */
  if (horizontal == 0) {
    axeshandle = _dxfNew2DAxesObject();
    if (!axeshandle)
      goto error;
    ob = (Object)DXNewXform((Object)outo, 
			    DXScale(scalewidth, scaleheight, 1.0));
    
    /* put a little negative fuzz on the color bar object */
    DXSetFloatAttribute((Object)ob, "fuzz", -2.0);
    _dxfSet2DAxesCharacteristic(axeshandle, "OBJECT", (Pointer)&ob);
    _dxfSet2DAxesCharacteristic(axeshandle, "XLABEL", (Pointer)extralabel);
    _dxfSet2DAxesCharacteristic(axeshandle, "YLABEL",(Pointer)label);
    _dxfSet2DAxesCharacteristic(axeshandle, "FONT",(Pointer)fontname);
    _dxfSet2DAxesCharacteristic(axeshandle, "LABELSCALE",
				(Pointer)&labelscale);
    _dxfSet2DAxesCharacteristic(axeshandle,"TICKS", (Pointer)&intzero);
    _dxfSet2DAxesCharacteristic(axeshandle,"TICKSX", (Pointer)&ticsx);
    _dxfSet2DAxesCharacteristic(axeshandle,"TICKSY", (Pointer)&ticsy);
    _dxfSet2DAxesCharacteristic(axeshandle,"FRAME", (Pointer)&frame);
    _dxfSet2DAxesCharacteristic(axeshandle,"ADJUST", (Pointer)&adjust);
    _dxfSet2DAxesCharacteristic(axeshandle,"GRID", (Pointer)&intzero);
    _dxfSet2DAxesCharacteristic(axeshandle,"ISLOGX", (Pointer)&intzero);
    _dxfSet2DAxesCharacteristic(axeshandle,"ISLOGY", (Pointer)&intzero);
    _dxfSet2DAxesCharacteristic(axeshandle,"MINTICKSIZE", 
				(Pointer)&minticsize);
    _dxfSet2DAxesCharacteristic(axeshandle,"AXESLINES",(Pointer)&axeslines);
    _dxfSet2DAxesCharacteristic(axeshandle,"JUSTRIGHT",(Pointer)&intzero);
    _dxfSet2DAxesCharacteristic(axeshandle,"RETURNCORNERS", 
				(Pointer)actualcorners);
    _dxfSet2DAxesCharacteristic(axeshandle,"LABELCOLOR", 
				(Pointer)&labelcolor);
    _dxfSet2DAxesCharacteristic(axeshandle,"TICKCOLOR", (Pointer)&ticcolor);
    _dxfSet2DAxesCharacteristic(axeshandle,"AXESCOLOR", (Pointer)&framecolor);
    _dxfSet2DAxesCharacteristic(axeshandle,"XLOCS", (Pointer)NULL);
    _dxfSet2DAxesCharacteristic(axeshandle,"YLOCS", (Pointer)locs);
    _dxfSet2DAxesCharacteristic(axeshandle,"XLABELS", (Pointer)NULL);
    _dxfSet2DAxesCharacteristic(axeshandle,"YLABELS", (Pointer)stringlist);
    
    if (DXGetError() != ERROR_NONE) goto error;
    
    o = (Object)_dxfAxes2D(axeshandle);
    if (!o) goto error;
    /* get the new bounding box */
    if (!DXBoundingBox(o,box)) goto error;
    min=box[0];
    max=box[0];
    for (i=0; i<7; i++) {
      min = DXMin(min,box[i]);
    }
    for (i=0; i<7; i++) {
      max = DXMax(max,box[i]);
    }
    shape.width = max.x - min.x;
    shape.height = max.y - min.y;
    newo = (Object)DXNewXform(o, DXTranslate(
					     DXPt(shape.width*(-translation.x
							       +
							       (min.x/(min.x-max.x))), 
						  shape.height*(-translation.y + 
								(min.y/(min.y-max.y))),
						  0.0))); 
    if (!newo) goto error;
  }
  /* else horizontal */
  else {
    axeshandle = _dxfNew2DAxesObject();
    if (!axeshandle)
	goto error;
    ob = (Object)DXNewXform((Object)outo, 
			    DXScale(scaleheight, scalewidth, 1.0));
    /* put a little negative fuzz on the color bar object */
    DXSetFloatAttribute((Object)ob, "fuzz", -2);
    _dxfSet2DAxesCharacteristic(axeshandle, "OBJECT", (Pointer)&ob);
    _dxfSet2DAxesCharacteristic(axeshandle, "XLABEL",(Pointer)label);
    _dxfSet2DAxesCharacteristic(axeshandle, "YLABEL", (Pointer)extralabel);
    _dxfSet2DAxesCharacteristic(axeshandle, "FONT",(Pointer)fontname);
    _dxfSet2DAxesCharacteristic(axeshandle, "LABELSCALE", 
				(Pointer)&labelscale);
    _dxfSet2DAxesCharacteristic(axeshandle,"TICKS", (Pointer)&intzero);
    _dxfSet2DAxesCharacteristic(axeshandle,"TICKSX", (Pointer)&ticsx);
    _dxfSet2DAxesCharacteristic(axeshandle,"TICKSY", (Pointer)&ticsy);
    _dxfSet2DAxesCharacteristic(axeshandle,"FRAME", (Pointer)&frame);
    _dxfSet2DAxesCharacteristic(axeshandle,"ADJUST", (Pointer)&adjust);
    _dxfSet2DAxesCharacteristic(axeshandle,"GRID", (Pointer)&intzero);
    _dxfSet2DAxesCharacteristic(axeshandle,"ISLOGX", (Pointer)&intzero);
    _dxfSet2DAxesCharacteristic(axeshandle,"ISLOGY", (Pointer)&intzero);
    _dxfSet2DAxesCharacteristic(axeshandle,"MINTICKSIZE", 
				(Pointer)&minticsize);
    _dxfSet2DAxesCharacteristic(axeshandle,"AXESLINES",(Pointer)&axeslines);
    _dxfSet2DAxesCharacteristic(axeshandle,"JUSTRIGHT",(Pointer)&intzero);
    _dxfSet2DAxesCharacteristic(axeshandle,"RETURNCORNERS", 
				(Pointer)actualcorners);
    _dxfSet2DAxesCharacteristic(axeshandle,"LABELCOLOR", 
				(Pointer)&labelcolor);
    _dxfSet2DAxesCharacteristic(axeshandle,"TICKCOLOR", (Pointer)&ticcolor);
    _dxfSet2DAxesCharacteristic(axeshandle,"AXESCOLOR", (Pointer)&framecolor);
    _dxfSet2DAxesCharacteristic(axeshandle,"XLOCS", (Pointer)locs);
    _dxfSet2DAxesCharacteristic(axeshandle,"YLOCS", (Pointer)NULL);
    _dxfSet2DAxesCharacteristic(axeshandle,"XLABELS", (Pointer)stringlist);
    _dxfSet2DAxesCharacteristic(axeshandle,"YLABELS", (Pointer)NULL);
    
    if (DXGetError() != ERROR_NONE) goto error;
    
    /* get the new bounding box */
    o = (Object)_dxfAxes2D(axeshandle);
    if (!o) goto error;
    if (!DXBoundingBox(o,box)) goto error;
    min=box[0];
    max=box[0];
    for (i=0; i<7; i++) {
      min = DXMin(min,box[i]);
    }
    for (i=0; i<7; i++) {
      max = DXMax(max,box[i]);
    }
    shape.width = max.y - min.y;
    shape.height = max.x - min.x;
    newo = (Object)DXNewXform(o,
			      DXTranslate(DXPt(shape.height*(-translation.x +
							     (min.x/(min.x-max.x))),
					       shape.width*(-translation.y +
							    (min.y/(min.y-max.y))),
					       0.0)));
    if (!newo) goto error;
  }
  
  if (!(outscreen =(Object)DXNewScreen((Object)newo, SCREEN_VIEWPORT, 1))) {
    goto error;
  }
  
  oo = (Object)DXNewXform((Object)outscreen, DXTranslate(translation));
  if (!oo) goto error;
  
  /* make it immovable */
  ooo = (Object)DXNewScreen(oo, SCREEN_STATIONARY, 0);
  if (!ooo) goto error;
  
  
  
  out[0]=ooo;
  DXDelete((Object)a_newpositions);
  DXDelete((Object)a_newconnections);
  DXDelete((Object)a_newcolors);
  _dxfFreeAxesHandle((Pointer)axeshandle);
  DXDelete((Object)index_list);
  DXDelete((Object)stringlist);
  DXDelete((Object)color_list);
  return OK;
  
 error:
  DXDelete((Object)oo);
  DXDelete((Object)a_newpositions);
  DXDelete((Object)a_newconnections);
  DXDelete((Object)a_newcolors);
  DXDelete((Object)color_list);
  DXDelete((Object)stringlist);
  if (axeshandle) 
    _dxfFreeAxesHandle((Pointer)axeshandle);
  DXDelete((Object)index_list);
  return ERROR;
}


