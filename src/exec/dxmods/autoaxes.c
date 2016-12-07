/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>

#include <ctype.h>

#if defined(HAVE_STRING_H)
#include <string.h>
#endif

#include <dx/dx.h>
#include "autoaxes.h"
#include "plot.h"

static RGBColor DEFAULTTICSCOLOR = {1.0, 1.0, 0.0};
static RGBColor DEFAULTLABELCOLOR = {1.0, 1.0, 1.0};
static RGBColor DEFAULTGRIDCOLOR = {0.3, 0.3, 0.3};
static RGBColor DEFAULTBACKGROUNDCOLOR = {0.05, 0.05, 0.05};
static Error GetAnnotationColors(Object, Object,
                                 RGBColor, RGBColor, RGBColor, RGBColor, 
                                 int *,
                                 RGBColor *, RGBColor *, RGBColor *,
                                 RGBColor *);
static Error _dxfCopyMostAttributes(Object, Object); 

extern Pointer _dxfNewAxesObject(void); /* from libdx/axes.c */
extern Error _dxfSetAxesCharacteristic(Pointer, char *, Pointer);  /* from libdx/axes.c*/
extern Object _dxfAutoAxes(Pointer); /* from libdx/axes.c*/

int
m_AutoAxes(Object *in, Object *out)
{
    Pointer axeshandle=NULL;
    Object object, labels, corners;
    Camera camera;
    int n = -9999, ns[3], adjust = 1, i;
    char *xlabel = NULL, *ylabel = NULL, *zlabel = NULL, *extra = NULL;
    char *fontname;
    Point cursor, box[8], min, max;
    int frame;
    float labelscale=1.0, fuzzattfloat;
    int cursor_specified = 0;
    int grid, fuzzattint;
    Class class; 
    RGBColor labelcolor, ticcolor, gridcolor, backgroundcolor;
    Array xticklocations=NULL, yticklocations=NULL, zticklocations=NULL;
    int numlist;
    float *list_ptr;


    ns[0] = ns[1] = ns[2] = 0;

    
    /* the object */
    if (!in[0]) {
        /* object must be specified */
	DXSetError(ERROR_BAD_PARAMETER, "#10000", "object");
        return ERROR;
    }
    if (DXGetObjectClass(in[0])==CLASS_FIELD && DXEmptyField((Field)in[0])) {
        out[0] = in[0];
        return OK;
    } 
    class = DXGetObjectClass(in[0]);
    if ((class != CLASS_FIELD)&&(class != CLASS_GROUP)&&
        (class != CLASS_XFORM)&&(class != CLASS_CLIPPED)) {
       /* object must be a field or a group */
       DXSetError(ERROR_BAD_PARAMETER,"#10190", "object");
       return ERROR;
    }   
    object = in[0];

    /* camera */
    if (!in[1]) {
        DXSetError(ERROR_BAD_PARAMETER, "#10000", "camera");
        return ERROR;
    }
    class = DXGetObjectClass(in[1]);
    if (class != CLASS_CAMERA) {
       /* camera must be a camera */
       DXSetError(ERROR_BAD_PARAMETER,"#10660", "camera");
       return ERROR;
    }
    camera = (Camera)in[1];

    /* labels */
    labels = in[2]? in[2] : DXGetAttribute(object, "axis labels");
    if (labels) {
	if (!DXExtractNthString(labels, 0, &xlabel)) {
            /* must be a string list */
	    DXSetError(ERROR_BAD_PARAMETER, "#10201", "labels");
            return ERROR;
        }
	DXExtractNthString(labels, 1, &ylabel);
	DXExtractNthString(labels, 2, &zlabel);
	if (DXExtractNthString(labels, 3, &extra))
	    DXWarning("extra axis label(s) ignored");
    }

    /* number of tick marks */
    n = -9999;
    if (in[3]) {
	if (DXExtractParameter(in[3], TYPE_INT, 1, 3, (Pointer)ns)) {
	    n = 0;
	} else if (DXExtractParameter(in[3], TYPE_INT, 1, 2, (Pointer)ns)) {
	    ns[2] = 0;
	    n = 0;
	} else if (!DXExtractInteger(in[3], &n)) {
            /* ticks must be an integer or integer list */
	    DXSetError(ERROR_BAD_PARAMETER, "#10050", "ticks");
            return ERROR;
	} 
    }

    /* corners is in[4] */
    corners = in[4];

    /* show frame? */
    frame = 0;
    if (in[5] && !DXExtractInteger(in[5], &frame)) {
        /* frame parameter must be either 0 or 1 */
	DXSetError(ERROR_BAD_PARAMETER, "#10070", "frame");
        return ERROR;
    }
    if ((frame<0) || (frame>1)) {
	DXSetError(ERROR_BAD_PARAMETER, "#10070", "frame"); 
        return ERROR;
    }
    /* adjust? */
    adjust = 1;
    if (in[6] && !DXExtractInteger(in[6], &adjust)) {
	DXSetError(ERROR_BAD_PARAMETER, "#10070", "adjust");
        return ERROR;
    }
    if (adjust!=0 && adjust!=1) {
	DXSetError(ERROR_BAD_PARAMETER, "#10070", "adjust");
        return ERROR;
    }

    /* cursor? */
    cursor_specified = 0;
    if (in[7]) {
	if (!DXExtractParameter(in[7], TYPE_FLOAT, 3, 1, (Pointer)&cursor)) {
	    /* "cursor must be a three vector" */
	    DXSetError(ERROR_BAD_PARAMETER, "#10230","cursor", 3);
            return ERROR;
        }
	cursor_specified = 1;
    }

    if (in[8]) {
        if (!DXExtractInteger(in[8], &grid))  {
           /* grid must be 0 or 1 */
           DXSetError(ERROR_BAD_PARAMETER,"#10070", "grid");
           return ERROR;
        }
        if ((grid < 0)||(grid > 1)) {
           DXSetError(ERROR_BAD_PARAMETER,"#10070", "grid");
           return ERROR;
        }
    }
    else {
        grid=0;
    }

    /* in[9] and in[10] are the colors for the annotation objects */
    if (!(GetAnnotationColors(in[9], in[10], 
                              DEFAULTTICSCOLOR, DEFAULTLABELCOLOR,
                              DEFAULTGRIDCOLOR, DEFAULTBACKGROUNDCOLOR,
                              &frame,
                              &ticcolor, &labelcolor, 
                              &gridcolor, &backgroundcolor)))
        return ERROR;

   /* labelscale */
   if (in[11]) {
      if (!DXExtractFloat(in[11],&labelscale)) {
         DXSetError(ERROR_BAD_PARAMETER, "#10090","labelscale");
         return ERROR;
      }
      if (labelscale < 0) {
         DXSetError(ERROR_BAD_PARAMETER, "#10090","labelscale");
         return ERROR;
      }
   }


   /* font */
   if (in[12]) {
      if (!DXExtractString(in[12],&fontname)) {
         DXSetError(ERROR_BAD_PARAMETER, "#10200","font");
         return ERROR;
      }
   }
   else {
      fontname = "standard";
   }

   /* If specified, these should override the corners and the tics params */
   /* user-forced xtic locations */
   if (in[13]) {
     if (!(DXGetObjectClass(in[13])==CLASS_ARRAY)) {
         DXSetError(ERROR_BAD_PARAMETER,"xlocations must be a scalar list");
         return ERROR;
     }
     xticklocations = (Array) in[13];
   }
   /* user-forced ytic locations */
   if (in[14]) {
     if (!(DXGetObjectClass(in[14])==CLASS_ARRAY)) {
         DXSetError(ERROR_BAD_PARAMETER,"ylocations must be a scalar list");
         return ERROR;
     }
     yticklocations = (Array) in[14];
   }
   /* user-forced ztic locations */
   if (in[15]) {
     if (!(DXGetObjectClass(in[15])==CLASS_ARRAY)) {
         DXSetError(ERROR_BAD_PARAMETER,"zlocations must be a scalar list");
         return ERROR;
     }
     zticklocations = (Array) in[15];
   }
   /* user-forced xtic labels; need to set in[13] */
   if (in[16]) {
     if (!(DXGetObjectClass(in[16])==CLASS_ARRAY)) {
         DXSetError(ERROR_BAD_PARAMETER,"xlabels must be a string list");
         return ERROR;
     }
     if (!DXGetArrayInfo((Array)in[16], &numlist, NULL,NULL,NULL,NULL))
        return ERROR;
     if (!in[13]) {
         /* need to make an array to use. It will go from 0 to n-1 */
         xticklocations = DXNewArray(TYPE_FLOAT, CATEGORY_REAL, 0);
         if (!xticklocations) return ERROR;
         list_ptr = DXAllocate(numlist*sizeof(float));
         if (!list_ptr) return ERROR;
         for (i=0; i<numlist; i++)
             list_ptr[i] = (float)i;
         if (!DXAddArrayData(xticklocations, 0, numlist, list_ptr))
             return ERROR;
         DXFree((Pointer)list_ptr);
      }
   }

   /* user-forced ytic labels; need to set in[14] */
   if (in[17]) {
     if (!(DXGetObjectClass(in[17])==CLASS_ARRAY)) {
         DXSetError(ERROR_BAD_PARAMETER,"ylabels must be a string list");
         return ERROR;
     }
     if (!DXGetArrayInfo((Array)in[17], &numlist, NULL,NULL,NULL,NULL))
        return ERROR;
     if (!in[14]) {
         /* need to make an array to use. It will go from 0 to n-1 */
         yticklocations = DXNewArray(TYPE_FLOAT, CATEGORY_REAL, 0);
         if (!yticklocations) return ERROR;
         list_ptr = DXAllocate(numlist*sizeof(float));
         if (!list_ptr) return ERROR;
         for (i=0; i<numlist; i++)
             list_ptr[i] = (float)i;
         if (!DXAddArrayData(yticklocations, 0, numlist, list_ptr))
             return ERROR;
         DXFree((Pointer)list_ptr);
      }
   }
   /* user-forced ztic labels; need to set in[15] */
   if (in[18]) {
     if (!(DXGetObjectClass(in[18])==CLASS_ARRAY)) {
         DXSetError(ERROR_BAD_PARAMETER,"zlabels must be a string list");
         return ERROR;
     }
     if (!DXGetArrayInfo((Array)in[18], &numlist, NULL,NULL,NULL,NULL))
        return ERROR;
     if (!in[15]) {
         /* need to make an array to use. It will go from 0 to n-1 */
         zticklocations = DXNewArray(TYPE_FLOAT, CATEGORY_REAL, 0);
         if (!zticklocations) return ERROR;
         list_ptr = DXAllocate(numlist*sizeof(float));
         if (!list_ptr) return ERROR;
         for (i=0; i<numlist; i++)
             list_ptr[i] = (float)i;
         if (!DXAddArrayData(zticklocations, 0, numlist, list_ptr))
             return ERROR;
         DXFree((Pointer)list_ptr);
      }
   }
 
 
   /*
    * Copy object prior to sticking fuzz onto it
    */
   object = DXCopy(object, COPY_HEADER);
   if (! object)
       return ERROR;

   /* put some fuzz on the object */
   /* if there's already some fuzz, don't munge that */
   if (DXGetFloatAttribute(object, "fuzz", &fuzzattfloat))
     fuzzattfloat = fuzzattfloat + 6;
   else if (DXGetIntegerAttribute(object,"fuzz", &fuzzattint))
     fuzzattfloat = fuzzattint + 6;
   else
     fuzzattfloat = 6;
   object = DXSetFloatAttribute(object, "fuzz", fuzzattfloat);

   axeshandle = _dxfNewAxesObject();
    if (!axeshandle) return ERROR;
    _dxfSetAxesCharacteristic(axeshandle, "OBJECT", (Pointer)&object);
    _dxfSetAxesCharacteristic(axeshandle, "CAMERA", (Pointer)&camera);
    _dxfSetAxesCharacteristic(axeshandle, "XLABEL", (Pointer)xlabel);
    _dxfSetAxesCharacteristic(axeshandle, "YLABEL", (Pointer)ylabel);
    _dxfSetAxesCharacteristic(axeshandle, "FONT", (Pointer)fontname);
    _dxfSetAxesCharacteristic(axeshandle, "LABELSCALE", (Pointer)&labelscale);
    _dxfSetAxesCharacteristic(axeshandle, "ZLABEL", (Pointer)zlabel);
    _dxfSetAxesCharacteristic(axeshandle, "NUMTICS", (Pointer)&n); 
    _dxfSetAxesCharacteristic(axeshandle, "NUMTICSX", (Pointer)&ns[0]); 
    _dxfSetAxesCharacteristic(axeshandle, "NUMTICSY", (Pointer)&ns[1]); 
    _dxfSetAxesCharacteristic(axeshandle, "NUMTICSZ", (Pointer)&ns[2]); 
    _dxfSetAxesCharacteristic(axeshandle, "CORNERS", (Pointer)&corners); 
    _dxfSetAxesCharacteristic(axeshandle, "FRAME", (Pointer)&frame); 
    _dxfSetAxesCharacteristic(axeshandle, "ADJUST", (Pointer)&adjust); 
    if (cursor_specified)
       _dxfSetAxesCharacteristic(axeshandle, "CURSOR", (Pointer)&cursor);
    else
       _dxfSetAxesCharacteristic(axeshandle, "CURSOR", NULL); 
    _dxfSetAxesCharacteristic(axeshandle, "TYPEX", (Pointer)"lin"); 
    _dxfSetAxesCharacteristic(axeshandle, "TYPEY", (Pointer)"lin"); 
    _dxfSetAxesCharacteristic(axeshandle, "TYPEZ", (Pointer)"lin"); 
    _dxfSetAxesCharacteristic(axeshandle, "GRID", (Pointer)&grid); 
    _dxfSetAxesCharacteristic(axeshandle, "LABELCOLOR", (Pointer)&labelcolor); 
    _dxfSetAxesCharacteristic(axeshandle, "TICKCOLOR", (Pointer)&ticcolor); 
    _dxfSetAxesCharacteristic(axeshandle, "AXESCOLOR", (Pointer)&ticcolor); 
    _dxfSetAxesCharacteristic(axeshandle, "GRIDCOLOR", (Pointer)&gridcolor); 
    _dxfSetAxesCharacteristic(axeshandle, "BACKGROUNDCOLOR", 
                              (Pointer)&backgroundcolor); 
    /* &&& */
    _dxfSetAxesCharacteristic(axeshandle, "XLOCS", 
                              (Pointer)xticklocations); 
    _dxfSetAxesCharacteristic(axeshandle, "YLOCS", 
                              (Pointer)yticklocations); 
    _dxfSetAxesCharacteristic(axeshandle, "ZLOCS", 
                              (Pointer)zticklocations); 
    _dxfSetAxesCharacteristic(axeshandle, "XLABELS", 
                              (Pointer)in[16]); 
    _dxfSetAxesCharacteristic(axeshandle, "YLABELS", 
                              (Pointer)in[17]); 
    _dxfSetAxesCharacteristic(axeshandle, "ZLABELS", 
                              (Pointer)in[18]); 

    if (!DXGetError() == ERROR_NONE) {
       DXFree((Pointer)axeshandle); 
       return ERROR;
    }

    out[0] = (Object)_dxfAutoAxes(axeshandle);
 
    if (out[0]) {
       if (!DXBoundingBox(out[0],box)) {
          DXFree((Pointer)axeshandle);
          return ERROR;
       }
       min.x = DXD_MAX_FLOAT; 
       min.y = DXD_MAX_FLOAT; 
       min.z = DXD_MAX_FLOAT; 
       max.x = -DXD_MAX_FLOAT; 
       max.y = -DXD_MAX_FLOAT; 
       max.z = -DXD_MAX_FLOAT; 
       for (i=0; i<8; i++) {
         if (box[i].x < min.x) min.x = box[i].x;
         if (box[i].y < min.y) min.y = box[i].y;
         if (box[i].z < min.z) min.z = box[i].z;
         if (box[i].x > max.x) max.x = box[i].x;
         if (box[i].y > max.y) max.y = box[i].y;
         if (box[i].z > max.z) max.z = box[i].z;
       }
       /* delta.x = max.x - min.x; */
       /* delta.y = max.y - min.y; */
       /* delta.z = max.z - min.z; */

       DXFree((Pointer)axeshandle);

       DXSetIntegerAttribute(out[0], "autoaxes", 1);
       /* copy all the top level attributes. However, we
          don't want to copy the color multiplier and 
          opacity multiplier attributes, because then they'll
          be there twice and will get concatenated together. */
       _dxfCopyMostAttributes(out[0],in[0]); 



       if (!in[13]) DXDelete((Object)xticklocations);
       if (!in[14]) DXDelete((Object)yticklocations);
       if (!in[15]) DXDelete((Object)zticklocations);
       return OK;
    }
    else 
       if (!in[13]) DXDelete((Object)xticklocations);
       if (!in[14]) DXDelete((Object)yticklocations);
       if (!in[15]) DXDelete((Object)zticklocations);
       DXFree((Pointer)axeshandle);
       return ERROR;

}



static Error GetAnnotationColors(Object colors, Object which,
                                     RGBColor defaultticcolor,
                                     RGBColor defaultlabelcolor,
                                     RGBColor defaultgridcolor,
                                     RGBColor defaultbackgroundcolor,
                                     int *frame,
                                     RGBColor *ticcolor, 
                                     RGBColor *labelcolor,
                                     RGBColor *gridcolor,
                                     RGBColor *backgroundcolor)
{
  RGBColor *colorlist =NULL;
  int i, numcolors, numcolorobjects;
  char *colorstring, *newcolorstring;


  /* in[9] is the colors list */
  /* first set up the default colors */
  *ticcolor = defaultticcolor;
  *labelcolor = defaultlabelcolor;
  *gridcolor = defaultgridcolor;
  *backgroundcolor = defaultbackgroundcolor;
  if (colors) {
    if (DXExtractNthString(colors,0,&colorstring)) {
      /* it's a list of strings */
      /* first figure out how many there are */
      if (!_dxfHowManyStrings(colors, &numcolors))
         goto error;
      /* allocate space for the 3-vectors */
      colorlist = DXAllocateLocal(numcolors*sizeof(RGBColor));
      if (!colorlist) goto error;
      for (i=0; i<numcolors;i++) {
        DXExtractNthString(colors,i,&colorstring);
        if (!DXColorNameToRGB(colorstring, &colorlist[i])) {
          _dxfLowerCase(colorstring, &newcolorstring);
          if (!strcmp(newcolorstring,"clear")) {
              DXResetError();
              colorlist[i]=DXRGB(-1.0, -1.0, -1.0);
          }
          else {
            goto error;
          }
          DXFree((Pointer)newcolorstring);
      }
    }
    }
    else {
      if (!DXQueryParameter(colors, TYPE_FLOAT, 3, &numcolors)) {
         DXSetError(ERROR_BAD_PARAMETER, "#10052", "colors");  
         goto error;
      }
      /* it's a list of 3-vectors */
      colorlist = DXAllocateLocal(numcolors*sizeof(RGBColor));
      if (!colorlist) goto error;
      if (!DXExtractParameter(colors, TYPE_FLOAT, 3, numcolors, 
                            (Pointer)colorlist)) {
         DXSetError(ERROR_BAD_PARAMETER,"#10052", "colors");
         goto error;
      }
    }
  }
  if (!which) {
    if (colors) {
      /* all objects get whatever color was there */
      if (numcolors != 1) {
        /* "more than one color specified for colors parameter; this requires 
            a list of objects to color" */
         DXSetError(ERROR_BAD_PARAMETER, "#11839");
         goto error;
      }
      if (colorlist[0].r != -1 || colorlist[0].g != -1 || 
          colorlist[0].b != -1) {
      *ticcolor = colorlist[0];
      *labelcolor = colorlist[0];
      *gridcolor = colorlist[0];
      *backgroundcolor = colorlist[0];
      }
      else {
         /* clear color is appropriate only for background */
         DXSetError(ERROR_BAD_PARAMETER, "#11811");
         goto error;
      }
    }
  }
  else {
    /* a list of strings was specified */
    /* figure out how many */
    if (!_dxfHowManyStrings(which, &numcolorobjects)) {
      /* annotation must be a string list */
      DXSetError(ERROR_BAD_PARAMETER,"#10201", "annotation");
      goto error;
    }
    if (numcolors==1) {
       for (i=0; i< numcolorobjects; i++) {
         DXExtractNthString(which, i, &colorstring);
         _dxfLowerCase(colorstring, &newcolorstring);
         /* XXX lower case */
         if ((colorlist[0].r==-1 && colorlist[0].g==-1 && 
               colorlist[0].b==-1) && strcmp(newcolorstring,"background")) {
            DXSetError(ERROR_BAD_PARAMETER, "#11811");
            goto error;
         }
         if (!strcmp(newcolorstring, "ticks"))
           *ticcolor = colorlist[0];
         else if (!strcmp(newcolorstring, "labels"))
           *labelcolor = colorlist[0];
         else if (!strcmp(newcolorstring,"grid"))
           *gridcolor=colorlist[0];
         else if (!strcmp(newcolorstring,"background")) {
           if (colorlist[0].r==-1 && 
               colorlist[0].g==-1 && 
               colorlist[0].b==-1) *frame = *frame+2;
           else
             *backgroundcolor=colorlist[0];
         }
         else if (!strcmp(newcolorstring,"all")) {
           *ticcolor = colorlist[0];
           *labelcolor = colorlist[0];
           *gridcolor=colorlist[0];
           *backgroundcolor=colorlist[0];
         }
         else {
           /* annotation objects must be one of "ticks", "labels", 
              "background" or "grid" */
           DXSetError(ERROR_BAD_PARAMETER, "#10203", "annotation strings", 
                      "ticks, labels, background, grid");
           goto error;
         }
         DXFree((Pointer)newcolorstring);
       }
    }
    else {
      if (numcolors != numcolorobjects) {
        /* number of colors must match number of annotation objects if 
           number of colors is greater than 1 */
        DXSetError(ERROR_BAD_PARAMETER,"#11813");
        goto error;
      }
       for (i=0; i< numcolorobjects; i++) {
         DXExtractNthString(which, i, &colorstring);
         _dxfLowerCase(colorstring, &newcolorstring);
         if ((colorlist[i].r==-1 && colorlist[i].g==-1 && 
               colorlist[i].b==-1) && strcmp(newcolorstring,"background")) {
            /* clear is appropriate only for background */
            DXSetError(ERROR_BAD_PARAMETER, "#11811");
            goto error;
         }
         /* XXX lower case */
         if (!strcmp(newcolorstring, "ticks"))
           *ticcolor = colorlist[i];
         else if (!strcmp(newcolorstring, "labels"))
           *labelcolor = colorlist[i];
         else if (!strcmp(newcolorstring,"grid"))
           *gridcolor=colorlist[i];
         else if (!strcmp(newcolorstring,"background")) {
           if (colorlist[i].r==-1 && 
               colorlist[i].g==-1 && 
               colorlist[i].b==-1) *frame = *frame+2;
           else
             *backgroundcolor=colorlist[i];
         }
         else {
           /* annotation objects must be one of "ticks", "labels", 
              "background" or "grid" */
           DXSetError(ERROR_BAD_PARAMETER,"#10203", "annotation strings", 
                      "ticks, labels, background, grid");
           goto error;
         }
         DXFree((Pointer)newcolorstring);
       }
    }
 }
  DXFree((Pointer)colorlist);
  return OK;
error:
  DXFree((Pointer)colorlist);
  return ERROR;
}

Error _dxfLowerCase(char *stringin, char **stringout)
{
   int length, i;

   length = strlen(stringin);
   *stringout = DXAllocateLocal((length+1)*sizeof(char));
   for (i=0; i<length; i++) {
     (*stringout)[i] = tolower(stringin[i]);
   }
   (*stringout)[length]= '\0'; 
   return OK;

}

static Error _dxfCopyMostAttributes(Object out, Object in)
{

  char *name;
  int i, count=0;
  Object attribute;

  for (i=0; (attribute = DXGetEnumeratedAttribute(in, i, &name)); i++) {
      if (strcmp(name,"color multiplier")&&strcmp(name,"opacity multiplier")) {
         DXSetAttribute(out, name, attribute);
         count++;
      }
  }
  return OK;

}

