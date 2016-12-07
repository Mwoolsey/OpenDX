/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>

#include <stdio.h>
#include <dx/dx.h>
#include <math.h>
#include <string.h>
#include "_autogray.h"
#include "_autocolor.h"

typedef struct {
    Field a_f;
    Object a_color;
    Object a_opacity;
} arg;

typedef struct {
    Field a_f;
    int a_setopacities;
    float a_min;
    float a_max;
    int a_surface;
    float a_opacity;
    float a_intensitystart;
    float a_intensityrange;
    float a_saturation;
    float a_hue;
    int   a_delayed;
    RGBColor   a_lowerrgb;
    RGBColor   a_upperrgb;
} arg_byte;


#define LN(x) log((double)x)

static Error AutoGrayScaleDelayedObject(Object, int, float, float, int,
  		          float, float, float, float, float, 
                          RGBColor, RGBColor);
static Error RecurseToIndividual(Object, float, float, float, 
     	 float, float, float *, float *, Object *, int, RGBColor,
         RGBColor);
static Error MakeMapAndColorGray(Object, float, float, float, float,
	 float, float *, float *, Object *, int, RGBColor,
         RGBColor);
static Error RemoveOpacities(Object);
static Error AutoGrayScaleDelayedField(Pointer);
static Error GoodField(Field);

#if 0
static Error MakeMapAndColorGrayMGrid(Object, float, float, float, float,
	 float, float *, float *, Object *, int, RGBColor,
         RGBColor);
#endif

/* extern somewhere */
extern Object _dxfDXEmptyObject(Object); /* from libdx/component.c */
extern Object DXMap(Object, Object, char *, char *); /* from libdx/map.c */
extern Array DXScalarConvert(Array); /* from libdx/stats.h */

#define ABS(a)	((a)>0.0 ? (a) : -(a))


static Pointer
AllocateBest(int n)
{
    Pointer p;
    if ((p=DXAllocateLocal(n)))
        return p;
    DXResetError();
    return DXAllocate(n);
}


/*
 * public entry point: 
 *
 *  parameters are an input group containing at least 1 field with a 'data'
 *  component, an opacity, intensity, phase, range, saturation, and
 *  inputmin and inputmax, and delayed (a flag). 
 */

Group _dxfAutoGrayScale(Object g,  float opacity, float hue, 
                        float intensity, float range, float saturation, 
                        float *inputmin, float *inputmax, 
                        Object *outcolorfield, int delayed, 
                        RGBColor colorvecmin, RGBColor colorvecmax) 
{
  Class class; 
  Group g_out;
  
 
  /* quick parm check */
  if(!g) {
    DXSetError(ERROR_BAD_PARAMETER, "NULL input");	
    return ERROR;
  }

  
  class = DXGetObjectClass((Object)g);
  if((class != CLASS_GROUP)  &&  (class != CLASS_FIELD) && 
     (class != CLASS_XFORM) && (class != CLASS_SCREEN) &&
     (class != CLASS_CLIPPED)) {
    DXSetError(ERROR_BAD_PARAMETER, "#10190","data");
    return ERROR;
  }
  
  /* duplicate input group */
  if (!(g_out = (Group)DXCopy((Object)g, COPY_STRUCTURE))) {
    return ERROR;
  }

  if (!(RecurseToIndividual((Object)g_out, opacity, hue, 
        intensity, range, saturation, inputmin, inputmax, 
        outcolorfield, delayed, colorvecmin, colorvecmax)))
     return ERROR;

  return (g_out);

}


static Error RecurseToIndividual(Object g_out, float opacity, 
	  	  float hue, float phase, float range, float saturation,
	    	  float *inputmin, float *inputmax, Object *outcolormap,
                  int delayed, RGBColor colorvecmin,
                  RGBColor colorvecmax)

/* this function will recurse to the level of an "individual"; that is,
   a composite field, a series, or a single field.  Generic groups will thus
   be broken apart, and each of the individuals in the group will have its
   own map created. */

{  
  int i, empty;
  Object subo;
  Group outcolorgroup;
  Class class, groupclass;
  
  
  if (!(class = DXGetObjectClass(g_out)))
    return ERROR;

  if (_dxfDXEmptyObject((Object)g_out)) 
    empty = 1;
  else
    empty = 0;

  if (empty) {
     *outcolormap = (Object)DXNewField();
     return OK;
  }
  
  switch(class) {
  case CLASS_GROUP:
    if (!(groupclass = DXGetGroupClass((Group)g_out)))
      return ERROR;
    switch(groupclass) {

    case CLASS_SERIES:
    case CLASS_MULTIGRID:
    case CLASS_COMPOSITEFIELD:
      if (!(MakeMapAndColorGray(g_out, opacity, hue, phase,
				    range, saturation, inputmin, 
				    inputmax, outcolormap, delayed, 
                                    colorvecmin, colorvecmax)))
	return ERROR;
      break;  

    /* this was here when I thought that multigrids could have mixed
     * volumes and surfaces. They can't.
    case CLASS_MULTIGRID:
      if (!(MakeMapAndColorGrayMGrid(g_out, opacity, hue, phase,
				    range, saturation, inputmin, 
				    inputmax, outcolormap, delayed, 
                                    colorvecmin, colorvecmax)))
	return ERROR;
      break;  
     */

    default:
      /* generic group: continue recursing  */
      if (!(outcolorgroup = (Group)DXNewGroup())) 
	return ERROR;
      for (i=0; (subo=
                (Object)DXGetEnumeratedMember((Group)g_out, i, NULL)); 
                      i++){ 
	if (!RecurseToIndividual(subo, opacity, hue, phase, range,
				 saturation, inputmin, inputmax, 
				 outcolormap,delayed, 
                                 colorvecmin, colorvecmax))
          return ERROR;
        if (!(DXSetMember(outcolorgroup, NULL, *outcolormap)))
	  return ERROR;
      }
      *outcolormap = (Object)outcolorgroup;
      break;
    }
    break;
  case CLASS_FIELD:     
    if (!(MakeMapAndColorGray(g_out, opacity, hue, phase,
			  range, saturation, inputmin, inputmax, 
			  outcolormap, delayed, colorvecmin,
                          colorvecmax)))
      return ERROR;
    break;
    
  case CLASS_SCREEN:
    if (!(DXGetScreenInfo((Screen)g_out, &subo, NULL, NULL)))
      return ERROR;
    if (!RecurseToIndividual(subo, opacity, hue, phase, range,
			     saturation, inputmin, inputmax,
			     outcolormap, delayed, 
                             colorvecmin, colorvecmax))
      return ERROR;
    break;
  case CLASS_CLIPPED:
    if (!(DXGetClippedInfo((Clipped)g_out, &subo, NULL)))
      return ERROR;
    if (!RecurseToIndividual(subo, opacity, hue, phase, range,
			     saturation, inputmin, inputmax,
			     outcolormap, delayed, 
                             colorvecmin, colorvecmax))
      return ERROR;
    break;
  case CLASS_XFORM:
    if (!(DXGetXformInfo((Xform)g_out, &subo, NULL)))
      return ERROR;
    if (!RecurseToIndividual(subo, opacity, hue, phase, range,
			     saturation, inputmin, inputmax,
			     outcolormap, delayed, 
                             colorvecmin, colorvecmax))
      return ERROR;
    break;
    
    
  default:
    /* set error here */
    DXSetError(ERROR_BAD_CLASS,"not a group, field, xform or screen object");
    return ERROR;
  }	 
  return OK;
}



#if 0
static Error MakeMapAndColorGrayMGrid(Object g_out, float opacity,float hue,
				      float phase, float range, 
				      float saturation,
				      float *inputmin,float *inputmax,
				      Object *outcolorfield, int delayed,
				      RGBColor colorvecmin, 
				      RGBColor colorvecmax)
{
  float minvalue, maxvalue, min, max;
  int i;
  Object subo;
  
  /* multigrids may have mixed volumes and surfaces. The purpose of this
     routine is to use the stats of the entire field to set the colors,
     but then do the pieces of the multigrid separately */
  
  
  if ((!delayed)||(!inputmin)||(!inputmax)) {
    /* get data statistices */
    if (!DXStatistics((Object)g_out, "data", &minvalue,
                      &maxvalue, NULL, NULL)) {
      return ERROR;
    }
  }
  
  /* if not provided, set the min and max based on the statistics */
  /* else use the ones given */
  if (!inputmin) {
    min = minvalue;
  }
  else {
    min = *inputmin;
  }
  
  
  if (!inputmax) {
    max = maxvalue;
  }
  else {
    max = *inputmax;
  }
  /* work on the members */
  for (i=0; subo = DXGetEnumeratedMember((Group)g_out, i, NULL); i++){
    if (!MakeMapAndColorGray(subo, opacity, hue, phase, range,
			     saturation, &min, &max,
			     outcolorfield, delayed, colorvecmin, colorvecmax))
      return ERROR;
  }
  return OK;
}
#endif




static Error MakeMapAndColorGray(Object g_out, float opacity,float hue,
                                 float phase, float range, float saturation,
                                 float *inputmin,float *inputmax,
                                 Object *outcolorfield, int delayed,
                                 RGBColor colorvecmin, RGBColor colorvecmax)
     
{
  int surface, setopacities, compactopacities, ii;
  int allblue, count, icount;
  float intensitystart, intensityend, intensity ;
  float red, blue, green, r, g, b, h, s, v; 
  float givenmin, givenmax, tmp;
  float minvalue, maxvalue;
  float unitopacity=0, unitvalue=0, thickness;
  float avg, standdev;
  float *opos_ptr, *cpos_ptr, *odata_ptr;
  RGBColor *cdata_ptr;
  Field  OpacityField, ColorField, newcolorfield;
  Array a_colordata=NULL, a_opacitydata=NULL; 
  Array a_colorpositions=NULL, a_opacitypositions=NULL;
  Array a_colorconnections=NULL, a_opacityconnections=NULL;
  Array colorfield_positions=NULL; 
  Array colorfield_connections=NULL; 
  Array colorfield_data=NULL;
  RGBColor corigin, lowerhsv, upperhsv, lowerrgb, upperrgb, colorval;
  float oorigin, opval, dataval, fuzz;
  float *colorfield_posptr, dmin, dmax;
  int datamin, datamax, numentries;
  RGBColor *colorfield_dataptr;
  Object bytecolorfield;

  ColorField = NULL;
  OpacityField = NULL;
  newcolorfield = NULL;


  if (!(_dxfIsVolume((Object)g_out, &surface)))  return ERROR;
  
  if (surface) {
    if (!_dxfRGBtoHSV(colorvecmin.r, colorvecmin.g, colorvecmin.b, &h, &s, &v))
      return ERROR;
    lowerhsv = DXRGB(h,s,v);
    if (!_dxfRGBtoHSV(colorvecmax.r, colorvecmax.g, colorvecmax.b, &h, &s, &v))
      return ERROR;
    upperhsv = DXRGB(h,s,v);
    lowerrgb = colorvecmin;
    upperrgb = colorvecmax;
  }
  else {
    lowerrgb = DXRGB(0.0, 0.0, 0.0);
    lowerhsv = DXRGB(0.0, 0.0, 0.0);
    upperrgb = DXRGB(0.0, 0.0, 0.0);
    upperhsv = DXRGB(0.0, 0.0, 0.0);
  }
  
  
  /*  determine the object's maximum thickness, for volume rendering only */
  if (!(surface))  {
    if (!(_dxfBoundingBoxDiagonal((Object)g_out,&thickness))) return ERROR; 
  }
  
  if (delayed) {
    /* Check if it's a valid field for delayed colors */
    if (!_dxfDelayedOK((Object)g_out))
      return ERROR;
  }
  
  
  intensitystart = phase;
  intensityend = phase + range;
  
  /* for surfaces, opacity is equal to the user specified value, and
     the value is set to the user-given "intensity" parameter */
  if (surface) {
    if (opacity == 1.0) {
      /* we need to recursively remove any opacity components */
      if (!(RemoveOpacities((Object)g_out)))
	return ERROR;
    }
    if (opacity == -1.0) opacity = 1.0; 
    /* XXX what was this for ? */
    /* if (intensity == -1.0) intensity = 1.0;  */
  }
  
  
  
  /* for volumes, the opacity per unit length is set using the
     user-given value for the opacity of light from the 
     back of the object. The value per unit length is set using 
     the user-given maximum brightness (intensity). */
  else {
    if (opacity == -1.0) {
      unitopacity = -LN(0.5)/thickness;
      unitvalue = unitopacity/0.5;
      opacity = 1.0;
      /* if (intensity == -1.0) intensity = 1.0; */
    }
    else if (opacity >= 0.99) {
      unitopacity = -LN(0.01)/thickness;
      unitvalue = unitopacity;
      /* if (intensity == -1.0) intensity = 1.0; */
    }
    else if (opacity == 0.0) {
      unitopacity = 0.0;
      unitvalue = 1.0/thickness;
      /* if (intensity == -1.0) intensity = 1.0; */
    }
    else {
      unitopacity = -LN(1.0 - opacity)/(thickness * opacity);
      /* if (intensity == -1.0) intensity = 1.0; */
      unitvalue = unitopacity;
    }
  }
  
  if ((!delayed)||(!inputmin)||(!inputmax)) { 
    /* get data statistices */  
    if (!DXStatistics((Object)g_out, "data", &minvalue, 
		      &maxvalue, &avg, &standdev)) {
      return ERROR;  
    }
  } 
  /* if not provided, set the min and max based on the statistics */
  /* else use the ones given */
  if (!inputmin)
    givenmin = minvalue;
  else {
    givenmin = *inputmin;
    if (delayed) minvalue = givenmin;
  }
  
  if (!inputmax) 
    givenmax = maxvalue;
  else {
    givenmax = *inputmax;
    if (delayed) maxvalue = givenmax;
  }

  /* check that if delayed, the min and max lie in 0 - 255 */
  if (delayed) {
     if (givenmin < 0) {
         givenmin = 0;
         DXWarning("AutoColor min clamped to 0 for delayed colors");
     }
     if (givenmin > 255) {
         givenmin = 255;
         DXWarning("AutoColor min clamped to 255 for delayed colors");
     }
     if (givenmax < 0) {
         givenmax = 0;
         DXWarning("AutoColor max clamped to 0 for delayed colors");
     }
     if (givenmax > 255) {
         givenmax = 255;
         DXWarning("AutoColor max clamped to 255 for delayed colors");
     }
  }




  if (givenmin > givenmax) {
    DXWarning("min is greater than max--exchanging min and max");
    tmp = givenmin;
    givenmin = givenmax;
    givenmax = tmp;
  }  
  
  /* remove this once greg fixes the interpolator library */
  /*fuzz = (givenmax - givenmin)/1000.0; */
  fuzz = 0.0;
  
  if (opacity == 1.0)
    setopacities = 0;
  else
    setopacities = 1;
  

  /* if it's a surface, we can always make a compact opacities array */
  /* if it's a volume, we can make a compact opacities array only if
     the min to max of the data are within the min to max set by the
     user. Otherwise, there will be opacities of 0.0 in there and 
     a compact array is impossible */
  if ((!surface)&&((givenmin > minvalue)||(givenmax < maxvalue))) 
    compactopacities = 0;
  else
    compactopacities = 1;

  
  /* if all the points are outside the range, we can make quick work
     of this */
  if ((givenmin > maxvalue)||(givenmax < minvalue)) {
    count = 1;
    /* make simple maps of opacity and color */
    if (surface) {
      if (setopacities) {
	oorigin = opacity;
	/*odelta = 0.0;*/
	if (!(a_opacitydata =
	      (Array)DXNewConstantArray(count, (Pointer)&oorigin, TYPE_FLOAT,
					CATEGORY_REAL, 0, NULL))) {
	  goto cleanup;
	}
      }
      corigin = lowerrgb;
      /*cdelta = DXRGB(0.0, 0.0, 0.0);*/
      if (!(a_colordata =
	    (Array)DXNewConstantArray(count, (Pointer)&corigin,
				      TYPE_FLOAT, CATEGORY_REAL, 1, 3))) {
	goto cleanup;
      }
    }
    /* else it is a volume */
    else {
      if (setopacities) {
	oorigin = 0.0;
	/*odelta = 0.0;*/
        if (!(a_opacitydata =
	      (Array)DXNewConstantArray(count, (Pointer)&oorigin,
					TYPE_FLOAT, CATEGORY_REAL, 0, NULL))) 
	  goto cleanup;
      }
      /* for volumes, out of range points are invisible */
      corigin = DXRGB(0.0, 0.0, 0.0);
      /*cdelta = DXRGB(0.0, 0.0, 0.0);*/
      if (!(a_colordata =
	    (Array)DXNewConstantArray(count, (Pointer)&corigin,
				      TYPE_FLOAT, CATEGORY_REAL, 1, 3))) 
	goto cleanup;
    }
    if (!(DXCreateTaskGroup())) 
      goto cleanup;
    
    if (!delayed) {
      /* ok to call autocolor here */
      if (!(_dxfAutoColorObject((Object)g_out, (Object)a_colordata,
				(Object)a_opacitydata))) {
	goto cleanup;
      }
    }
    else {
      /* not ok to call autocolor here */
      if (!(AutoGrayScaleDelayedObject((Object)g_out, setopacities, 
				       givenmin, givenmax, surface,
				       opacity, intensitystart, range, 
				       saturation, hue, 
				       lowerrgb, upperrgb))) {
	goto cleanup;
      }
    }
    if (!(DXExecuteTaskGroup())) {
      goto cleanup;
    }
    *outcolorfield = (Object)a_colordata;
    DXDelete((Object)ColorField);
    DXDelete((Object)OpacityField);
    if (!surface) {
      if (!(_dxfSetMultipliers(g_out, unitvalue, unitopacity))) {
	goto cleanup;
      }
    }
    return OK;
  }
  else if ((minvalue==givenmin)&&(minvalue==maxvalue)&&(maxvalue==givenmax)) { 
    count = 1;
    /* make simple maps of opacity and color */
    if (surface) {
      if (setopacities) {
	oorigin = opacity;
	/*odelta = 0.0;*/
        if (!(a_opacitydata =
	      (Array)DXNewConstantArray(count, (Pointer)&oorigin,
					TYPE_FLOAT, CATEGORY_REAL, 0, NULL))) 
	  goto cleanup;
      }
      
      if (!(_dxfHSVtoRGB(hue, saturation, intensitystart,&red,&green,&blue))) 
	goto cleanup;
      
      
      corigin = DXRGB(red, green, blue);
      /*cdelta = DXRGB(0.0, 0.0, 0.0);*/
      if (!(a_colordata =
	    (Array)DXNewConstantArray(count, (Pointer)&corigin,
				      TYPE_FLOAT, CATEGORY_REAL,1, 3))) {
	goto cleanup;
      }
    }
    /* else it is a volume */
    else {
      if (setopacities) {
	oorigin = opacity;
	/*odelta = 0.0;*/
	if (!(a_opacitydata =
	      (Array)DXNewConstantArray(count, (Pointer)&oorigin,
					TYPE_FLOAT, CATEGORY_REAL,0, NULL))) 
	  goto cleanup;
      }
      if (!(_dxfHSVtoRGB(hue, saturation, intensitystart,
			 &red,&green,&blue))) 
	goto cleanup;
      
      corigin = DXRGB(red, green, blue);
      /*cdelta = DXRGB(0.0, 0.0, 0.0);*/
      if (!(a_colordata =
	    (Array)DXNewConstantArray(count, (Pointer)&corigin,
				      TYPE_FLOAT, CATEGORY_REAL, 1, 3))) 
	goto cleanup;
    }
    if (!(DXCreateTaskGroup())) {
      goto cleanup;
    }
    if (!delayed) {
      /* ok to call autocolor here */
      if (!(_dxfAutoColorObject((Object)g_out, (Object)a_colordata,
				(Object)a_opacitydata))) {
	goto cleanup;
      }
    }
    else {
      if (!(AutoGrayScaleDelayedObject((Object)g_out, setopacities, 
				       givenmin, givenmax, surface,
				       opacity, intensitystart, range, 
				       saturation, hue, 
				       lowerrgb, upperrgb))) 
	goto cleanup;
    }
    if (!(DXExecuteTaskGroup())) {
      goto cleanup;
    }
    *outcolorfield = (Object)a_colordata;
    DXDelete((Object)ColorField);
    DXDelete((Object)OpacityField);
    if (!surface) {
      if (!(_dxfSetMultipliers(g_out, unitvalue, unitopacity))) {
	goto cleanup;
      }
    }
    return OK;
  }
  /* else not all points are out of range */
  
  else {
    if (!delayed) {
      /* make the maps */
      if ((setopacities)&&(!(compactopacities))) {
	if (!(OpacityField = DXNewField())) {
	  goto cleanup;
	}
      }
      if (!(ColorField = DXNewField())) {
	goto cleanup;
      }
      
      /* first make opacity map, if necessary */
      if (setopacities) {
	if (compactopacities) {
	  count = 1;
	  oorigin = opacity;
	  /*odelta = 0.0;*/
	  if (!(a_opacitydata =
		(Array)DXNewConstantArray(count, (Pointer)&oorigin,TYPE_FLOAT,
					  CATEGORY_REAL, 0, NULL))) 
	    goto cleanup;
	}
	/* else opacity array is not compact, because some volume points
	   are out of range */
	else {
	  /* figure out the number of items in the array */
	  if ((givenmin > minvalue)&&(givenmax < maxvalue)) 
	    count = 6;
	  else if (givenmin > minvalue) 
	    count = 4;
	  else if (givenmax < maxvalue) 
	    count = 4;
	  else 
	    count = 2;
	  
	  /* special case */
	  if ((minvalue == givenmin)&&(givenmin == givenmax)&&
	      (maxvalue == givenmax)){
	    count = 2;
	    allblue = 1;
	  }
	  else
	    allblue = 0;
	  
	  /* create the array */
	  if (!(a_opacitydata = DXNewArray(TYPE_FLOAT, CATEGORY_REAL, 1, 1))) 
	    goto cleanup;
	  if (!DXAddArrayData(a_opacitydata, 0, count, NULL))
	    goto cleanup;
	  odata_ptr=(float *)DXGetArrayData(a_opacitydata);
	  
	  if (!(a_opacitypositions =
		DXNewArray(TYPE_FLOAT, CATEGORY_REAL, 1, 1))) 
	    goto cleanup;
	  if (!DXAddArrayData(a_opacitypositions,0, count, NULL))
	    goto cleanup;
	  opos_ptr=(float *)DXGetArrayData(a_opacitypositions);
	  
	  if (!(a_opacityconnections =  DXMakeGridConnectionsV(1, &count))) 
	    goto cleanup;
	  
	  /* now put the constructed arrays in the maps */
	  if (!(DXSetComponentValue((Field)OpacityField, "data", 
				    (Object)a_opacitydata))) 
	    goto cleanup;
	  a_opacitydata=NULL;
	  
	  if (!(DXSetComponentValue((Field)OpacityField, "positions", 
				    (Object)a_opacitypositions))) 
	    goto cleanup;
	  a_opacitypositions=NULL;
	  
	  if (!(DXSetComponentValue((Field)OpacityField, "connections", 
				    (Object)a_opacityconnections))) 
	    goto cleanup;
	  a_opacityconnections=NULL;
	  
	  
	  if (!(DXSetComponentAttribute((Field)OpacityField, "data", "dep",
					(Object)DXNewString("positions")))) 
	    goto cleanup;
	  
	  if (!(DXSetComponentAttribute((Field)OpacityField,"connections",
					"ref",
					(Object)DXNewString("positions")))) 
	    goto cleanup;
	  
	  if (!(DXSetComponentAttribute((Field)OpacityField,"connections",
					"element type",
					(Object)DXNewString("lines")) ))  
	    goto cleanup;
	  
	    
	  /* now add the data to the arrays */
	  icount = 0;
	  /* first deal with the first two invisible points, if necessary */
	  if (givenmin > minvalue) {
	    opval = 0.0;
	    dataval = minvalue - fuzz;
	    opos_ptr[icount]=dataval;
	    odata_ptr[icount]=opval;
	    
	    icount++;
	    dataval = givenmin - fuzz;
	    
	    opos_ptr[icount]=dataval;
	    odata_ptr[icount]=opval;
	    
	    icount++;
	  }
	  
	  /* now do the part between givenmin and givenmax*/
	  opval = opacity;
	  dataval = givenmin-fuzz; 
	  
	  opos_ptr[icount]=dataval;
	  odata_ptr[icount]=opval;
	  
	  icount++;
	  dataval = givenmax+fuzz; 
	  
	  opos_ptr[icount]=dataval;
	  odata_ptr[icount]=opval;
	  
	  icount++;
	  

	  /* now deal with the last two invisible points, if necessary */
	  if (givenmax < maxvalue) {
	    opval = 0.0;
	    dataval = givenmax + 2*fuzz;
	    
	    opos_ptr[icount]=dataval;
	    odata_ptr[icount]=opval;
	    
	    icount ++;
	    dataval = maxvalue + fuzz;
	    
	    opos_ptr[icount]=dataval;
	    odata_ptr[icount]=opval;
	    
	    icount++;
	  }
	}
      }
      

      /* now do colors */
      /* figure out the number of items in the array */
      if ((givenmin > minvalue)&&(givenmax < maxvalue)) 
	count = 6;
      else if (givenmin > minvalue) 
	count = 4;
      else if (givenmax < maxvalue) 
	count = 4;
      else 
	count = 2;
      
      /* special case */
      if ((minvalue==givenmin)&&(givenmin == givenmax)&&(maxvalue==givenmax)) {
	count = 2;
	allblue = 1;
      }
      else
	allblue = 0;
      
      
      /* create the arrays */
      if (!(a_colordata = DXNewArray(TYPE_FLOAT, CATEGORY_REAL, 1, 3))) {
	goto cleanup;
      }
      if (!DXAddArrayData(a_colordata,0,count,NULL))
	goto cleanup;
      cdata_ptr=(RGBColor *)DXGetArrayData(a_colordata);
      
	
      if (!(a_colorpositions =
	    DXNewArray(TYPE_FLOAT, CATEGORY_REAL, 1, 1))) 
	goto cleanup;
      if (!DXAddArrayData(a_colorpositions,0,count,NULL))
	goto cleanup;
      cpos_ptr=(float *)DXGetArrayData(a_colorpositions);
      
      
      if (!(a_colorconnections =  DXMakeGridConnectionsV(1, &count))) 
	goto cleanup;
      
      /* add to the field */
      if (!(DXSetComponentValue((Field)ColorField, "data", 
				(Object)a_colordata))) 
	goto cleanup;
      a_colordata=NULL;
      
      if (!(DXSetComponentValue((Field)ColorField, "positions", 
				(Object)a_colorpositions))) 
	goto cleanup;
      a_colorpositions=NULL;
      
      if (!(DXSetComponentValue((Field)ColorField, "connections", 
				(Object)a_colorconnections))) 
	goto cleanup;
      a_colorconnections=NULL;
      
      
      if (!(DXSetComponentAttribute((Field)ColorField,"data", "dep",
				    (Object)DXNewString("positions")) )) 
	goto cleanup;
      
      
      if (!(DXSetComponentAttribute((Field)ColorField,"connections","ref",
				    (Object)DXNewString("positions")) )) 
	goto cleanup;
      
      if (!(DXSetComponentAttribute((Field)ColorField,"connections",
				    "element type",
				    (Object)DXNewString("lines")) )) 
	goto cleanup;
      
      
      /* now add the data to the arrays */
      icount = 0;
      /* first deal with the first two invisible points, if necessary */
      if (givenmin > minvalue) {
	dataval = minvalue - fuzz;
	
	cdata_ptr[icount]=lowerhsv;
	cpos_ptr[icount]=dataval;
	
	icount++;
	dataval = givenmin - 2*fuzz;
	
	cdata_ptr[icount]=lowerhsv;
	cpos_ptr[icount]=dataval;
	
	icount++;
      }
      
      
      /* now do the part between givenmin and givenmax*/
      intensity = intensitystart;
      dataval = givenmin - fuzz;
      colorval = DXRGB(hue, saturation, intensity);
      
      cdata_ptr[icount]=colorval;
      cpos_ptr[icount]=dataval;
      
      icount++;
      
      if (!(allblue)) intensity = intensityend;
      else intensity = intensitystart;
      
      dataval = givenmax + fuzz;
      colorval = DXRGB(hue, saturation, intensity);
      
      cdata_ptr[icount]=colorval;
      cpos_ptr[icount]=dataval;
      
      icount++;
      
      /* now deal with the last two invisible points, if necessary */
      if (givenmax < maxvalue) {
	dataval = givenmax + 2*fuzz;
	
	cpos_ptr[icount]=dataval;
	cdata_ptr[icount]=upperhsv;
	
	icount ++;
	dataval = maxvalue + fuzz;
	
	cpos_ptr[icount]=dataval;
	cdata_ptr[icount]=upperhsv;
	
	icount ++;
      }
	
	
      if (ColorField) {
	if (!(DXEndField(ColorField))) {
	  goto cleanup;
	}
      }
      if (OpacityField) {
	if (!(DXEndField(OpacityField))) {
	  goto cleanup;
	}
      } 
      
      /* now convert the created color map to an rgb map */
      if (!(newcolorfield = _dxfMakeRGBColorMap(ColorField))) {
	goto cleanup;
      }
      
      if (!(DXCreateTaskGroup())) {
	goto cleanup;
      }
      /* if compact opacities, just pass the array */
      if (compactopacities) {
	/* ok to call DXAutoColor */
	if (!(_dxfAutoColorObject((Object)g_out, 
				  (Object)newcolorfield,
				  (Object)a_opacitydata))) {
	  goto cleanup;
	}
      }
      /* else, pass the opacity field */
      else {
	/* ok to call DXAutoColor */
	if (!(_dxfAutoColorObject((Object)g_out, 
				  (Object)newcolorfield,
				  (Object)OpacityField))) {
	  goto cleanup;
	}
      }
      if (!(DXExecuteTaskGroup())) {
	goto cleanup;
      }
      
      DXDelete((Object)ColorField);
      DXDelete((Object)OpacityField);
      *outcolorfield = (Object)newcolorfield;
      if (!surface) {
	if (!(_dxfSetMultipliers(g_out, unitvalue, unitopacity))) {
	  goto cleanup;
	}
      }
      return OK;
    }
    /* else delayed */
    else {
      /* create outcolorfield */
      bytecolorfield = (Object)DXNewField();
      colorfield_positions = (Array)DXNewArray(TYPE_FLOAT,CATEGORY_REAL,1,1);
      /* need the actual min and max of the field */
      if (!DXStatistics((Object)g_out, "data", &dmin, &dmax, NULL, NULL))
        goto cleanup;
      datamin=dmin;
      datamax=dmax;
      /* for now, set to 0-255 */
      datamin=0;
      datamax=255;
      numentries = datamax - datamin + 1;

      colorfield_connections = (Array)DXNewPathArray(numentries);
      colorfield_data = (Array)DXNewArray(TYPE_FLOAT,CATEGORY_REAL,1,3);
      if (!(DXAddArrayData(colorfield_positions, 0, numentries, NULL))) 
	goto cleanup;

      if (!(DXAddArrayData(colorfield_data, 0, numentries, NULL))) 
	goto cleanup;

      colorfield_posptr = (float *)DXGetArrayData(colorfield_positions);
      colorfield_dataptr = (RGBColor *)DXGetArrayData(colorfield_data);

      if (!(DXSetComponentValue((Field)bytecolorfield,"positions",
				(Object)colorfield_positions))) 
	goto cleanup;
      colorfield_positions=NULL;

      if (!(DXSetComponentValue((Field)bytecolorfield,"connections",
				(Object)colorfield_connections))) 
	goto cleanup;
      colorfield_connections=NULL;
      
      if (!(DXSetComponentValue((Field)bytecolorfield,"data",
				(Object)colorfield_data))) 
	goto cleanup;
      

      if (!(DXSetComponentAttribute((Field)bytecolorfield,"data",
				    "dep",(Object)DXNewString("positions")))) 
	goto cleanup;
      
      if (!(DXSetComponentAttribute((Field)bytecolorfield, "connections",
				    "ref",
				    (Object)DXNewString("positions")))) 
	goto cleanup;
      
      if (!(DXSetComponentAttribute((Field)bytecolorfield, "connections",
				    "element type",
				    (Object)DXNewString("lines")))) 
          goto cleanup;
      
      
      for (ii=datamin; ii<givenmin; ii++) {
	colorfield_posptr[ii-datamin] = (float)ii;
	colorfield_dataptr[ii-datamin] =  lowerrgb;
      }
      for (ii=givenmin; ii<=givenmax; ii++) {
	colorfield_posptr[ii-datamin] = (float)ii;
	intensity = intensitystart +
	  ((ii-givenmin)/(givenmax-givenmin))*range;
	if (!_dxfHSVtoRGB(hue,saturation,intensity,&r,&g,&b))
	  goto cleanup;
	colorfield_dataptr[ii-datamin] =  DXRGB(r, g, b);
      }
      for (ii=givenmax+1; ii<=datamax; ii++) {
	colorfield_posptr[ii-datamin] = (float)ii;
	colorfield_dataptr[ii-datamin] =  upperrgb;
      }
      
      *outcolorfield=bytecolorfield;
      
      if (!(DXCreateTaskGroup())) {
	goto cleanup;
      }
      if (!(AutoGrayScaleDelayedObject((Object)g_out,
				       setopacities, givenmin, givenmax,
				       surface, opacity, intensitystart,
				       range, saturation,
				       hue, lowerrgb, upperrgb))) {
	goto cleanup;
      }
      if (!(DXExecuteTaskGroup())) {
	goto cleanup;
      }
      if (!surface) {
	if (!(_dxfSetMultipliers(g_out, unitvalue, unitopacity))) {
	  goto cleanup;
	}
      }
      return OK;
    }
  }
 cleanup: {
   DXDelete((Object)ColorField);
   DXDelete((Object)OpacityField);
   DXDelete((Object)a_opacitydata);
   DXDelete((Object)a_opacitypositions);
   DXDelete((Object)a_opacityconnections);
   DXDelete((Object)a_opacitydata);
   DXDelete((Object)a_colorpositions);
   DXDelete((Object)a_colorconnections);
   DXDelete((Object)a_colordata);
   return ERROR;
 }
}






static Error RemoveOpacities(Object o)
{
  int i;
  Object subo;
  
  
  switch(DXGetObjectClass(o)) {
  case CLASS_GROUP:
    /* get children and call RemoveOpacities(subo); */
    for (i=0; (subo=DXGetEnumeratedMember((Group)o, i, NULL)); i++){ 
      if (!RemoveOpacities(subo))
	return ERROR;
    }
    break;
  case CLASS_FIELD:
    if (DXGetComponentValue((Field)o,"opacities"))
       DXDeleteComponent((Field)o,"opacities");
    break;
  default:
    /* set error here */
    DXSetError(ERROR_BAD_CLASS,"not a group or field");
    return ERROR;
  }	 
  return OK;
}




static Error 
  AutoGrayScaleDelayedObject(Object o, int setopacities,
                          float givenmin, float givenmax, int surface,
  		          float opacity, float intensitystart, float range,
                          float saturation, float hue, 
                          RGBColor lowerrgb, RGBColor upperrgb)
{
  int i;
  Object subo;
  arg_byte a;
  
  
  switch(DXGetObjectClass(o)) {
  case CLASS_GROUP:
    /* get children and call AutoColorObject(subo); */
    for (i=0; (subo=DXGetEnumeratedMember((Group)o, i, NULL)); i++){ 
      if (!AutoGrayScaleDelayedObject(subo, setopacities, givenmin, 
                                      givenmax, surface, opacity, 
                                      intensitystart, range, saturation,
                                      hue, lowerrgb, upperrgb))
	return ERROR;
    }
    break;
  case CLASS_FIELD:
    a.a_f = (Field)o;
    a.a_setopacities = setopacities;
    a.a_min = givenmin;
    a.a_max = givenmax;
    a.a_surface = surface;
    a.a_opacity = opacity;
    a.a_intensitystart = intensitystart;
    a.a_intensityrange = range;
    a.a_saturation = saturation;
    a.a_hue = hue;
    a.a_lowerrgb = lowerrgb;
    a.a_upperrgb = upperrgb;
    
    /* and schedule autocolorfield to be called */
    if (!(DXAddTask(AutoGrayScaleDelayedField, (Pointer)&a, sizeof(a), 1.0))) {
       return ERROR;
    }
    break;
  default:
    /* set error here */
    DXSetError(ERROR_BAD_CLASS,"not a group or field");
    return ERROR;
  }	 
  return OK;
}



static Error AutoGrayScaleDelayedField(Pointer ptr)


{
  Field f;
  int scalar, i, setopacities, surface;
  float givenmin, givenmax,opacity,r,g,b;
  float dmin, dmax;
  int datamin, datamax, numentries;
  float *opacityarray=NULL, intensitystart, range,saturation,intensity,hue;
  RGBColor *colorarray=NULL, lowerrgb,upperrgb;
  Array a_data;
  Array deferredcolors, deferredopacities;
  arg_byte *a;

  a = (arg_byte *)ptr;
  f = a->a_f;
  setopacities = a->a_setopacities;
  surface = a->a_surface;
  opacity = a->a_opacity;
  givenmin = a->a_min;
  givenmax = a->a_max;
  intensitystart = a->a_intensitystart;
  range = a->a_intensityrange;
  saturation = a->a_saturation;
  hue = a->a_hue;
  lowerrgb = a->a_lowerrgb;
  upperrgb = a->a_upperrgb;
  deferredcolors = NULL;
  deferredopacities = NULL;

  /* if it's an empty field, or a field without data, ignore it */
  if (!(GoodField(f))) return OK;


 a_data = (Array)DXGetComponentValue((Field)f,"data");
  if (!a_data) {
     DXSetError(ERROR_MISSING_DATA,"missing data component");
     return ERROR;
  }
  /* need the actual min and max of the field */
  if (!DXStatistics((Object)f, "data", &dmin, &dmax, NULL, NULL)) {
     return ERROR;
  }
  datamin=dmin;
  datamax=dmax;
  /* for now, set to 0-255 */
  datamin=0;
  datamax=255;
  numentries = datamax - datamin + 1;



  /* if it's not a good data field, return on ERROR */
  if (!(_dxfScalarField(f, &scalar))) return ERROR;

  /* make the maps */
  /* first make opacity map, if necessary */
  if (setopacities) {
    opacityarray=AllocateBest(numentries*sizeof(float));
    if (!opacityarray) {
      DXAddMessage("cannot allocate delayed opacity map with %d entries",
                    numentries);
      goto error;
    }
    if (surface) {
      for(i=0; i<=255; i++) {
	opacityarray[i]=opacity;
      }
    }
    else {
      for (i=datamin; i< givenmin; i++) {
	opacityarray[i-datamin]=0;
      }
      for (i=givenmin; i<=givenmax; i++) {
	opacityarray[i-datamin]=opacity;
      }
      for (i=givenmax+1; i<= datamax; i++) {
	opacityarray[i-datamin]=0;
      }
    }
  } 
  /* now do colors */
  colorarray = AllocateBest(numentries*sizeof(RGBColor));
  if (!colorarray) {
    DXAddMessage("cannot allocate delayed color map with %d entries",
                  numentries);
    goto error;
  }
  for (i=datamin; i< givenmin; i++) {
    if (surface)
      colorarray[i-datamin]=lowerrgb;
    else
      colorarray[i-datamin]=DXRGB(0.0,0.0,0.0);
  }
  /* now do the part between givenmin and givenmax*/
  for (i=givenmin; i<= givenmax; i++) {
    if (givenmin==givenmax) intensity = intensitystart;
    else  intensity =intensitystart + 
           (i-givenmin)/(givenmax-givenmin)*range;
    if (!_dxfHSVtoRGB(hue,saturation,intensity,&r,&g,&b)) 
      return ERROR;
    colorarray[i-datamin]=DXRGB(r,g,b);
  }
  for (i=givenmax+1; i<= datamax; i++) {
    if (surface)
      colorarray[i-datamin]=upperrgb;
    else
      colorarray[i-datamin]=DXRGB(0.0,0.0,0.0);
  }
  
  
  
  
    if (!(deferredcolors = (Array)DXNewArray(TYPE_FLOAT, 
					   CATEGORY_REAL,  1, 3))) 
      goto error;

    if (!(DXAddArrayData(deferredcolors,0,256,(Pointer)colorarray))) 
      goto error;

    if (!(DXSetComponentValue((Field)f, "color map", (Object)deferredcolors))) 
      goto error;
    deferredcolors=NULL;

    if (!(DXSetComponentValue((Field)f, "colors", (Object)a_data))) 
      goto error;
     
    if (setopacities) {
      if (!(deferredopacities = (Array)DXNewArray(TYPE_FLOAT, 
                                                CATEGORY_REAL,0,NULL))) 
	goto error;
       
      if (!(DXAddArrayData(deferredopacities,0,256,(Pointer)opacityarray))) 
	goto error;
       
      if (!(DXSetComponentValue((Field)f,
                              "opacity map",(Object)deferredopacities)))  
        goto error;
      deferredopacities=NULL;

 
      if (!(DXSetComponentValue((Field)f, "opacities", (Object)a_data)))  
	goto error;
       
    }
  
  DXEndField(f); 
  DXFree((Pointer)colorarray);
  DXFree((Pointer)opacityarray);
  return OK;
  
 error:
  DXDelete((Object)deferredcolors);
  DXDelete((Object)deferredopacities);
  DXFree((Pointer)colorarray);
  DXFree((Pointer)opacityarray);
  return ERROR;
}


static Error GoodField(Field f)
{
  if (DXEmptyField(f))
     return ERROR;
  if (!(DXGetComponentValue(f, "data")))
     return ERROR;
  return OK;
}

