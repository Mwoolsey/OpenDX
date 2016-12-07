/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/*
 * Header: /usr/people/gresh/code/svs/src/dxmods/RCS/convert.m,v 5.0 92/11/12
 10:05:29 svs Exp Locker: gresh 
 *
 * 
 */

#include <dxconfig.h>


/***
MODULE:
Convert   
SHORTDESCRIPTION:
 Converts between DXRGB and HSV  
CATEGORY:
 Data Transformation
INPUTS:
 data;            vector, list of vectors, or field;   NULL ;   input color or color map 
 incolorspace;    string;                 "hsv";    color space of input
 outcolorspace;   string;                 "rgb";    color space of output 
OUTPUTS:
 output;          vector, list of vectors, or field;   NULL;  output colors or color map

BUGS:
AUTHOR:
 dl gresh   
END:
***/


#include <math.h>
#include <ctype.h>
#include <string.h>
#include <dx/dx.h>
#include "_autocolor.h"
#include "color.h"

static Error ConvertObject(Object, char *, Object *);
static Error ConvertFieldObject(Object, char *);

int
m_Convert(Object *in, Object *out)
{
  Array    incolorvec, outcolorvec;
  int      ismap, isfield, count, i, ii, numitems, addpoints;
  char     *colorstrin, *colorstrout; 
  char     newstrin[30], newstrout[30];
  Object   gout;
  float    *dpin=NULL, *dpout=NULL;

  gout=NULL;
  incolorvec = NULL;
  outcolorvec = NULL;
   
  if (!in[0]) {
    DXSetError(ERROR_MISSING_DATA,"#10000","data");
    return ERROR;
  }
  out[0] = in[0];
  ismap = 0; 
  isfield = 0; 
  

  /*
   *  if it's an array (vector or list of vectors)
   */

  if (DXGetObjectClass(in[0]) == CLASS_ARRAY) {
    if (!(DXQueryParameter((Object)in[0], TYPE_FLOAT, 3, &numitems))) {
      /* must be a list of 3-vectors or a field */
      DXSetError(ERROR_BAD_PARAMETER,"#10550","data");
      out[0] = NULL;
      return ERROR;
    }
    if (!(incolorvec = DXNewArray(TYPE_FLOAT,CATEGORY_REAL, 1, 3))){
      out[0] = NULL;
      return ERROR;
    }
    if (!(DXAddArrayData(incolorvec,0,numitems,NULL)))
      goto error;
    if (!(dpin = (float *)DXGetArrayData(incolorvec)))
      goto error;
    if (!(DXExtractParameter((Object)in[0], TYPE_FLOAT, 3, numitems,
			   (Pointer)dpin))) {
      /* must be a list of 3-vectors or a field */
      DXSetError(ERROR_BAD_PARAMETER,"#10550","data");
      goto error;
    }

    /*    
     *  also make the output array
     */
    if (!(outcolorvec = DXNewArray(TYPE_FLOAT,CATEGORY_REAL, 1, 3)))
      goto error;
    if (!(DXAddArrayData(outcolorvec,0,numitems,NULL)))
      goto error;
    if (!(dpout = (float *)DXGetArrayData(outcolorvec)))
      goto error;

  }
  else { 
    if (_dxfIsColorMap(in[0])) 
      ismap = 1;
    else {
      DXResetError();
      isfield = 1;
    }
  }




  
  /* now extract the to and from space names */
  /*  get color name  */
  if (!(in[1]))
    colorstrin = "hsv";
  else 
    if (!DXExtractString((Object)in[1], &colorstrin)) {
      /* invalid input string */
      DXSetError(ERROR_BAD_PARAMETER,"#10200","incolorspace");
      goto error;
    }
  /*  convert color name to all lower case and remove spaces */
  count = 0;
  i = 0;
  while ( i<29 && colorstrin[i] != '\0') {
    if (isalpha(colorstrin[i])){
      if (isupper(colorstrin[i]))
	newstrin[count]= tolower(colorstrin[i]);
      else
	newstrin[count]= colorstrin[i];
      count++;
    }
    i++;
  }

  newstrin[count]='\0';
  if (strcmp(newstrin,"rgb")&&strcmp(newstrin,"hsv")) {
    DXSetError(ERROR_BAD_PARAMETER,"#10210", newstrin, "incolorspace");
    goto error;
  }
  
  
  if (!in[2]) 
    colorstrout = "rgb";
  else if (!DXExtractString((Object)in[2], &colorstrout)) {
      /* invalid outcolorspace string */
    DXSetError(ERROR_BAD_PARAMETER,"#10200","outcolorspace");
    goto error;
  }
  
  /*  convert color name to all lower case and remove spaces */
  count = 0;
  i = 0;
  while ( i<29 && colorstrout[i] != '\0') {
    if (isalpha(colorstrout[i])){
      if (isupper(colorstrout[i]))
	newstrout[count]= tolower(colorstrout[i]);
      else
	newstrout[count]= colorstrout[i];
      count++;
    }
    i++;
  }
  newstrout[count]='\0';
  if (strcmp(newstrout,"rgb")&&strcmp(newstrout,"hsv")){
    /* invalid outcolorspace string */
    DXSetError(ERROR_BAD_PARAMETER,"#10210", newstrout, "outcolorspace");
    goto error;
  }
  
  if (!strcmp(newstrin,newstrout)) {
    DXWarning("incolorspace and outcolorspace are the same");
    return OK;
  }

  if (!in[3]) {
     /* default behavior is to treat maps like maps (adding points)
        and to treat fields like fields (not adding points) */
  }
  else {
     if (!DXExtractInteger(in[3], &addpoints)) {
        DXSetError(ERROR_BAD_PARAMETER, "#10070",
                   "addpoints");
        goto error;
     }
     if ((addpoints < 0) || (addpoints > 1)) {
        DXSetError(ERROR_BAD_PARAMETER, "#10070",
                   "addpoints");
        goto error;
     }
     if (isfield && addpoints) {
        DXSetError(ERROR_BAD_PARAMETER, "#10370",
         "for a field with greater than 1D positions, addpoints", "0");
        goto error;
     }
     if (ismap && (!addpoints)) {
        /* treat the map like a field */
        ismap = 0;
        isfield = 1;
     }
  }

  /* we have been given a vector value or list of values, not a map */
  if ((!ismap)&&(!isfield)) {
    if (!strcmp(newstrin,"hsv")) {
      for (ii = 0; ii < numitems*3; ii = ii+3) {
	if (!(_dxfHSVtoRGB(dpin[ii], dpin[ii+1], dpin[ii+2],
		       &dpout[ii], &dpout[ii+1], &dpout[ii+2]))) 
	  goto error;
      }
    }
    else {
      for (ii = 0; ii < numitems*3; ii = ii+3) {
	if (!(_dxfRGBtoHSV(dpin[ii], dpin[ii+1], dpin[ii+2],
		       &dpout[ii], &dpout[ii+1], &dpout[ii+2]))) 
	  goto error;
      }
    }
    out[0] = (Object)outcolorvec;
    DXDelete((Object)incolorvec);
    return OK;
  }

  /* we have a map or field */
  else {
    if (ismap) {
      if (!(ConvertObject(in[0], newstrin, &gout)))
        goto error;
    }
    else {
      gout = DXCopy(in[0], COPY_STRUCTURE);
      if (!gout) goto error;
      if (!(ConvertFieldObject(gout, newstrin)))
         goto error;
    }
    out[0] = gout;
    return OK;
  }

  error:
    out[0] = NULL;
    DXDelete((Object)gout);
    DXDelete((Object)incolorvec);
    DXDelete((Object)outcolorvec);
    return ERROR;
}


static Error ConvertFieldObject(Object out, char *strin)
{

Class cl;
int i, rank, shape[30], numitems;
Type type;
Category category;
Array data, new_data;
RGBColor *dp_old, *dp_new;
Object subo;
float red, green, blue, hue, sat, val;
    
    if (!(cl = DXGetObjectClass(out))) 
      return ERROR;
    switch (cl) {
       case CLASS_GROUP:
	  for (i=0; (subo = DXGetEnumeratedMember((Group)out, i, NULL)); i++) {
            if (!ConvertFieldObject((Object)subo, strin))
	        return ERROR;
            }
	  break;
       case CLASS_FIELD:
          if (DXEmptyField((Field)out))
             return OK;
          data = (Array)DXGetComponentValue((Field)out,"data");
          if (!data) { 
             DXSetError(ERROR_MISSING_DATA,"#10240", "data");
             return ERROR;
          }
          DXGetArrayInfo(data,&numitems,&type,&category,&rank,shape);
          if ((type != TYPE_FLOAT)||(category != CATEGORY_REAL)) {
             DXSetError(ERROR_DATA_INVALID,"#10331", "data");
             return ERROR;
          }
          if ((rank != 1)||(shape[0] != 3)) {
             DXSetError(ERROR_DATA_INVALID,"#10331", "data");
             return ERROR;
          }
          new_data = DXNewArray(TYPE_FLOAT,CATEGORY_REAL,1,3);
          new_data = DXAddArrayData(new_data, 0, numitems, NULL);
          dp_old = (RGBColor *)DXGetArrayData(data);
          dp_new = (RGBColor *)DXGetArrayData(new_data);
          if (!strcmp(strin,"hsv"))  {
              for (i=0; i<numitems; i++) {
                 if (!_dxfHSVtoRGB(dp_old[i].r, dp_old[i].g, dp_old[i].b,
                               &red, &green, &blue))
                     return ERROR;
                 dp_new[i] = DXRGB(red,green,blue);
              }
          }
          else {
              for (i=0; i<numitems; i++) {
                 if (!_dxfRGBtoHSV(dp_old[i].r, dp_old[i].g, dp_old[i].b,
                               &hue, &sat, &val))
                     return ERROR;
                 dp_new[i] = DXRGB(hue, sat, val);
              }
          }
          DXSetComponentValue((Field)out, "data", (Object)new_data);
          DXChangedComponentValues((Field)out,"data");
          DXEndField((Field)out);
	  break;
       default:
          break;
    }

    return OK; 
    
  }

static Error ConvertObject(Object in, char *strin, Object *out)
{

Class cl;
    
    if (!(cl = DXGetObjectClass(in))) 
      return ERROR;
    switch (cl) {
       case CLASS_GROUP:
          DXSetError(ERROR_DATA_INVALID,"colormaps must be a single field");
          return ERROR;
       case CLASS_FIELD:
          if (!strcmp(strin,"hsv"))  {
            if (!(*out = (Object)_dxfMakeRGBColorMap((Field)in))) 
	        return ERROR;
          }
          else {
            if (!(*out = (Object)_dxfMakeHSVfromRGB((Field)in))) 
	        return ERROR;
          }
	  break;
       default:
          break;
    }

    return OK; 
    
  }


