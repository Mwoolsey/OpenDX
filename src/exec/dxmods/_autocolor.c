/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/*
 * Header: /usr/people/gresh/code/svs/src/libdx/RCS/autocolor.c,v 5.0 92/11/12
09:07:29 svs Exp Locker: gresh 
 */

#include <dxconfig.h>

#include <stdio.h>
#include <dx/dx.h>
#include <math.h>
#include <string.h>
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
    float a_huestart;
    float a_huerange;
    float a_saturation;
    float a_intensity;
    RGBColor   a_lowerrgb;
    RGBColor   a_upperrgb;
} arg_byte;

#define LN(x) log((double)x)
#define ABS(a)	((a)>0.0 ? (a) : -(a))

static float ValueFcn(float, float, float);
static Error AutoColorDelayedObject(Object, int, float, float, int, float,
                   float, float, float, float, RGBColor,
                   RGBColor);
static Error RecurseToIndividual(Object, float, float, float, 
     	           float, float, float *, float *, Object *, int, RGBColor,
                   RGBColor);
static Error MakeMapAndColor(Object, float, float, float, float,
	           float, float *, float *, Object *, int, RGBColor, RGBColor);

static Error RemoveOpacities(Object);
static Error AutoColorField(Pointer);
static Error AutoColorDelayedField(Pointer);
static Error GoodField(Field);
static int   direction(int);
static int   sign(int);

#if 0
static Error MakeMapAndColorMGrid(Object, float, float, float, float,
	           float, float *, float *, Object *, int, RGBColor, RGBColor);
#endif


/* extern somewhere */
extern Object _dxfDXEmptyObject(Object); /* from libdx/component.c */
extern Object DXMap(Object, Object, char *, char *); /* from libdx/map.c */
extern Array DXScalarConvert(Array); /* from libdx/stats.h */


/*
 * public entry point: 
 *
 *  parameters are an input group containing at least 1 field with a 'data'
 *  component, an opacity, intensity, phase, range, saturation, and
 *  inputmin and inputmax, and delayed (a flag). 
 */

Group _dxfAutoColor(Object g,  float opacity, float intensity, float phase, 
	            float range, float saturation, float *inputmin, 
                    float *inputmax, Object *outcolorfield, int delayed,
                    RGBColor colormin, RGBColor colormax) 
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
     (class !=CLASS_CLIPPED)) {
    /* data is not a group or a field */
    DXSetError(ERROR_BAD_PARAMETER,"#10190", "data");
    return ERROR;
  }
  
  /* duplicate input group */
  if (!(g_out = (Group)DXCopy((Object)g, COPY_STRUCTURE))) {
    return ERROR;
  }

  if (!(RecurseToIndividual((Object)g_out, opacity, intensity, 
        phase, range, saturation, inputmin, inputmax, outcolorfield, 
        delayed, colormin, colormax)))
     return ERROR;

  return (g_out);

}


static Error RecurseToIndividual(Object g_out, float opacity, 
	  	  float intensity, float phase, float range, float saturation,
	    	  float *inputmin, float *inputmax, Object *outcolormap,
                  int delayed, RGBColor colorvecmin,
                  RGBColor colorvecmax)

/* this function will recurse to the level of an "individual"; that is,
   a composite field, a series, a multigrid, 
   or a single field.  Generic groups will thus
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
      if (!(MakeMapAndColor(g_out, opacity, intensity, phase, range, 
                            saturation, inputmin, inputmax, outcolormap, 
                            delayed, colorvecmin, colorvecmax)))
	return ERROR;
      break;  


    /* 
     * this was here when I thought that multigrids could be mixed
     * volumes and surfaces. They can't.
    case CLASS_MULTIGRID:
      if (!(MakeMapAndColorMGrid(g_out, opacity, intensity, phase, range, 
                                 saturation, inputmin, inputmax, outcolormap, 
                                 delayed, colorvecmin, colorvecmax)))
	return ERROR;
      break;  
    */

    default:

      /* generic group;  continue recursing  */
      if (!(outcolorgroup = DXNewGroup())) 
	return ERROR;
      for (i=0; (subo=DXGetEnumeratedMember((Group)g_out, i, NULL)); i++){ 
	if (!RecurseToIndividual(subo, opacity, intensity, phase, range,
				 saturation, inputmin, inputmax, 
				 outcolormap,delayed,colorvecmin,
                                 colorvecmax))
          return ERROR;
        if (!(DXSetMember(outcolorgroup, NULL, *outcolormap)))
	  return ERROR;
      }
      *outcolormap = (Object)outcolorgroup;
      break;
    }
    break;
  case CLASS_FIELD:     
    if (!(MakeMapAndColor(g_out, opacity, intensity, phase,
			  range, saturation, inputmin, inputmax, 
			  outcolormap, delayed, colorvecmin,
                          colorvecmax)))
      return ERROR;
    break;
    
  case CLASS_SCREEN:
    if (!(DXGetScreenInfo((Screen)g_out, &subo, NULL, NULL)))
      return ERROR;
    if (!RecurseToIndividual(subo, opacity, intensity, phase, range,
			     saturation, inputmin, inputmax,
			     outcolormap, delayed, colorvecmin,
                             colorvecmax))
      return ERROR;
    break;
  case CLASS_CLIPPED:
    if (!(DXGetClippedInfo((Clipped)g_out, &subo, NULL)))
      return ERROR;
    if (!RecurseToIndividual(subo, opacity, intensity, phase, range,
			     saturation, inputmin, inputmax,
			     outcolormap, delayed, colorvecmin,
                             colorvecmax))
      return ERROR;
    break;
  case CLASS_XFORM:
    if (!(DXGetXformInfo((Xform)g_out, &subo, NULL)))
      return ERROR;
    if (!RecurseToIndividual(subo, opacity, intensity, phase, range,
			     saturation, inputmin, inputmax,
			     outcolormap, delayed, colorvecmin,
                             colorvecmax))
      return ERROR;
    break;
    
    
  default:
    /* set error here */
    DXSetError(ERROR_BAD_CLASS,
               "not a group, field, xform, clipped, or screen object");
    return ERROR;
  }	 
  return OK;
}

#if 0
static Error MakeMapAndColorMGrid(Object g_out, float opacity_i,
                             float intensity_i,
			     float phase, float range, float saturation, 
			     float *inputmin, float *inputmax, 
			     Object *outcolorfield, int delayed, 
                             RGBColor colorvecmin, RGBColor colorvecmax)
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
     if (!MakeMapAndColor(subo, opacity_i, intensity_i, phase, range,
                          saturation, &min, &max,
                          outcolorfield, delayed, colorvecmin, colorvecmax))
        return ERROR;
  }
  return OK;
}
#endif


static Error MakeMapAndColor(Object g_out, float opacity_i, 
                             float intensity_i,
			     float phase, float range, float saturation, 
			     float *inputmin,float *inputmax, 
			     Object *outcolorfield, int delayed, 
                             RGBColor colorvecmin, RGBColor colorvecmax)
     
{
  int surface, setopacities, compactopacities, ii;
  int allblue, count, icount;
  float huestart, hueend, hue ;
  float red, blue, green, r, g, b; 
  float givenmin, givenmax, tmp;
  float dmin, dmax;
  int datamin, datamax, numentries;
  float minvalue, maxvalue;
  float unitopacity=0, unitvalue=0, thickness;
  float avg, standdev;
  Field  OpacityField, ColorField, newcolorfield;
  float *odata_ptr, *opos_ptr, *cpos_ptr;
  RGBColor *cdata_ptr;
  Array a_colordata=NULL; 
  Array a_opacitydata=NULL; 
  Array a_colorpositions=NULL;
  Array a_opacitypositions=NULL;
  Array a_colorconnections=NULL; 
  Array a_opacityconnections=NULL;
  Array colorfield_positions=NULL; 
  Array colorfield_connections=NULL; 
  Array colorfield_data=NULL;
  RGBColor corigin, colorval;
  float oorigin, opval, dataval, fuzz, opacity, intensity;
  float *colorfield_posptr, h, s, v;
  RGBColor *colorfield_dataptr, lowerhsv, upperhsv, lowerrgb, upperrgb;
  Object bytecolorfield;

  ColorField = NULL;
  OpacityField = NULL;
  newcolorfield = NULL;

  if (!(_dxfIsVolume((Object)g_out, &surface)))  return ERROR;


  opacity = opacity_i;
  intensity = intensity_i;
  /*  determine the object's maximum thickness, for volume rendering only */
  if (!(surface))  {
    if (!(_dxfBoundingBoxDiagonal((Object)g_out,&thickness))) return ERROR; 
  }
  
  /* for surfaces, opacity is equal to the user specified value, and
     the value is set to the user-given "intensity" parameter */
  if (surface) {
    if (opacity == 1.0) {
      /* we need to recursively remove any opacity components */
      if (!(RemoveOpacities((Object)g_out)))
	return ERROR;
    }
    if (opacity == -1.0) opacity = 1.0; 
    if (intensity == -1.0) intensity = 1.0; 
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
      if (intensity == -1.0) intensity = 1.0;
    }
    else if (opacity >= 0.99) {
      unitopacity = -LN(0.01)/thickness;
      unitvalue = unitopacity;
      if (intensity == -1.0) intensity = 1.0;
    }
    else if (opacity == 0.0) {
      unitopacity = 0.0;
      unitvalue = 1.0/thickness;
      if (intensity == -1.0) intensity = 1.0;
    }
    else {
      unitopacity = -LN(1.0 - opacity)/(thickness * opacity);
      if (intensity == -1.0) intensity = 1.0;
      unitvalue = unitopacity;
    }
  }
 


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
  
  if (delayed) {
    /* Check if it's a valid field for delayed colors */
    if (!_dxfDelayedOK((Object)g_out))
      return ERROR;
  } 
  
  huestart = phase;
  hueend = phase + range;
  
 
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
	      (Array)DXNewConstantArray(count, (Pointer)&oorigin,
					TYPE_FLOAT, CATEGORY_REAL,
					0, NULL))) {
	  goto cleanup;
	}
      }
      corigin = lowerrgb;
      /*cdelta = DXRGB(0.0, 0.0, 0.0);*/
      if (!(a_colordata = 
	    (Array)DXNewConstantArray(count, (Pointer)&corigin,
				      TYPE_FLOAT, CATEGORY_REAL,
				      1, 3))) {
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
					TYPE_FLOAT, CATEGORY_REAL,
					0, NULL))) {
	  goto cleanup;
	}
      }
      /* for volumes, out of range points are invisible */
      corigin = DXRGB(0.0, 0.0, 0.0);
      /*cdelta = DXRGB(0.0, 0.0, 0.0);*/
      if (!(a_colordata = 
	    (Array)DXNewConstantArray(count, (Pointer)&corigin,
				      TYPE_FLOAT, CATEGORY_REAL,
				      1, 3))) {
	goto cleanup;
      }
    }
    if (!(DXCreateTaskGroup())) {
      goto cleanup;
    }
    if (!delayed) {
      if (!(_dxfAutoColorObject((Object)g_out, (Object)a_colordata,
				(Object)a_opacitydata))) {
	goto cleanup;
      }
    }
    else {
      if (!(AutoColorDelayedObject((Object)g_out, 
				   setopacities, givenmin, givenmax, surface,
				   opacity, huestart, range, saturation,
				   intensity, lowerrgb, upperrgb))) {
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
    goto returnok;
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
					TYPE_FLOAT, CATEGORY_REAL,
					0, NULL))) {
	  goto cleanup;
	}
      }
      
      if (!(_dxfHSVtoRGB(huestart, saturation, intensity,&red,&green,&blue))) {
	goto cleanup;
      }
      
      corigin = DXRGB(red, green, blue);
      /*cdelta = DXRGB(0.0, 0.0, 0.0);*/
      if (!(a_colordata = 
	    (Array)DXNewConstantArray(count, (Pointer)&corigin,
				      TYPE_FLOAT, CATEGORY_REAL,
				      1, 3))) {
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
					TYPE_FLOAT, CATEGORY_REAL,
					0, NULL))) {
	  goto cleanup;
	}
      }
      if (!(_dxfHSVtoRGB(huestart, saturation, intensity,&red,&green,&blue))) {
	goto cleanup;
      }
      
      corigin = DXRGB(red, green, blue);
      /*cdelta = DXRGB(0.0, 0.0, 0.0);*/
      if (!(a_colordata = 
	    (Array)DXNewConstantArray(count, (Pointer)&corigin,
				      TYPE_FLOAT, CATEGORY_REAL,
				      1, 3))) {
	goto cleanup;
      }
    }
    if (!(DXCreateTaskGroup())) {
      goto cleanup;
    }
    if (!delayed) {
      if (!(_dxfAutoColorObject((Object)g_out, (Object)a_colordata,
				(Object)a_opacitydata))) {
	goto cleanup;
      }
    }
    else {
      if (!(AutoColorDelayedObject((Object)g_out, 
				   setopacities, givenmin, givenmax, surface,
				   opacity, huestart, range, saturation,
				   intensity, lowerrgb, upperrgb))) {
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
    goto returnok;
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
                (Array)DXNewConstantArray(count, (Pointer)&oorigin,
					  TYPE_FLOAT, CATEGORY_REAL,
					  0, NULL))) {
	    goto cleanup;
	  }
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
          if (!DXAddArrayData(a_opacitydata,0,count,NULL))
            goto cleanup;
          odata_ptr=(float *)DXGetArrayData(a_opacitydata); 
	  
	  if (!(a_opacitypositions =
		DXNewArray(TYPE_FLOAT, CATEGORY_REAL, 1, 1))) 
	    goto cleanup;
          if (!DXAddArrayData(a_opacitypositions,0,count,NULL))
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
	    
            odata_ptr[icount]=opval;
            opos_ptr[icount]=dataval;

	    icount++;
	    dataval = givenmin - fuzz;

            odata_ptr[icount]=opval;
            opos_ptr[icount]=dataval;

	    icount++;
	  }
	  
	  
	  /* now do the part between givenmin and givenmax*/
	  opval = opacity;
	  dataval = givenmin-fuzz; 
	  
          odata_ptr[icount]=opval;
          opos_ptr[icount]=dataval;

          icount++;
	  dataval = givenmax+fuzz; 

          odata_ptr[icount]=opval;
          opos_ptr[icount]=dataval;

          icount++;


	  /* now deal with the last two invisible points, if necessary */
	  if (givenmax < maxvalue) {
	    opval = 0.0;
	    dataval = givenmax + 2*fuzz;
	    
            odata_ptr[icount]=opval;
            opos_ptr[icount]=dataval;

	    icount ++;
	    dataval = maxvalue + fuzz;

            odata_ptr[icount]=opval;
            opos_ptr[icount]=dataval;

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
      if (!(a_colordata = DXNewArray(TYPE_FLOAT, CATEGORY_REAL, 1, 3))) 
	goto cleanup;
      if (!DXAddArrayData(a_colordata,0,count,NULL))
        goto cleanup;
      cdata_ptr=(RGBColor *)DXGetArrayData(a_colordata);
      
    
      if (!(a_colorpositions =
	    DXNewArray(TYPE_FLOAT, CATEGORY_REAL, 1, 1))) 
	goto cleanup;
      if (!DXAddArrayData(a_colorpositions,0,count,NULL))
        goto cleanup;
      cpos_ptr = (float *)DXGetArrayData(a_colorpositions);
    
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
      
      if (!(DXSetComponentAttribute((Field)ColorField,"connections",
				    "ref",
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
      hue = huestart;
      dataval = givenmin - fuzz;
      colorval = DXRGB(hue, saturation, intensity);

      cdata_ptr[icount]=colorval;
      cpos_ptr[icount]=dataval;

      icount++;
      if (!(allblue)) hue = hueend;
      else hue = huestart;
      dataval = givenmax + fuzz;
      colorval = DXRGB(hue, saturation, intensity);

     
      cdata_ptr[icount]=colorval;
      cpos_ptr[icount]=dataval;

      icount++;

      /* now deal with the last two invisible points, if necessary */
      if (givenmax < maxvalue) {
	dataval = givenmax + 2*fuzz;
	
        cdata_ptr[icount]=upperhsv;
        cpos_ptr[icount]=dataval;
        
	icount ++;
	dataval = maxvalue + fuzz;

        cdata_ptr[icount]=upperhsv;
        cpos_ptr[icount]=dataval;

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
	if (!(_dxfAutoColorObject((Object)g_out, 
				  (Object)newcolorfield,
				  (Object)a_opacitydata))) {
	  goto cleanup;
	}
      }
      /* else, pass the opacity field */
      else {
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
      if (compactopacities) {
         DXDelete((Object)a_opacitydata);
         a_opacitydata = NULL;
      } 
      goto returnok;
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
      colorfield_data=NULL;
      
      
      if (!(DXSetComponentAttribute((Field)bytecolorfield, "data","dep",
				    (Object)DXNewString("positions")))) 
	goto cleanup;
      
      
      if (!(DXSetComponentAttribute((Field)bytecolorfield, "connections",
				    "ref",
				    (Object)DXNewString("positions")))) 
	goto cleanup;


      if (!(DXSetComponentAttribute((Field)bytecolorfield,
					"connections",
					"element type",
					(Object)DXNewString("lines")))) 
	goto cleanup;


      for (ii=datamin; ii<givenmin; ii++) {
	colorfield_posptr[ii-datamin] = (float)ii;
        colorfield_dataptr[ii-datamin] = lowerrgb;
      }
      for (ii=givenmin; ii<=givenmax; ii++) {
	colorfield_posptr[ii-datamin] = (float)ii;
	hue = huestart + ((ii-givenmin)/(givenmax-givenmin))*range;
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
      if (!(AutoColorDelayedObject((Object)g_out, 
			 	setopacities, givenmin, givenmax, surface,
				opacity, huestart, range, saturation,
                                intensity, lowerrgb, upperrgb))) 
	goto cleanup;

      if (!(DXExecuteTaskGroup())) 
	goto cleanup;
      
      if (!surface) {
	if (!(_dxfSetMultipliers(g_out, unitvalue, unitopacity))) 
	  goto cleanup;
      } 
      goto returnok;
    }
  }

 returnok:
   DXDelete((Object)a_opacitydata);
   return OK;

 cleanup: {
   DXDelete((Object)ColorField);
   DXDelete((Object)OpacityField);
   DXDelete((Object)a_colordata);
   DXDelete((Object)a_colorpositions);
   DXDelete((Object)a_colorconnections);
   DXDelete((Object)a_opacitydata);
   DXDelete((Object)a_opacitypositions);
   DXDelete((Object)a_opacityconnections);
   DXDelete((Object)colorfield_data);
   DXDelete((Object)colorfield_positions);
   DXDelete((Object)colorfield_connections);
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


Error _dxfAutoColorObject(Object o, Object Color, Object Opacity)
{
  int i;
  Object subo;
  arg a;
  
  
  switch(DXGetObjectClass(o)) {
  case CLASS_XFORM:
    if (!DXGetXformInfo((Xform)o, &subo, NULL))
	return ERROR;
    if (!_dxfAutoColorObject(subo, Color, Opacity))
	return ERROR;
    break;
  case CLASS_CLIPPED:
    if (!DXGetClippedInfo((Clipped)o, &subo, NULL))
	return ERROR;
    if (!_dxfAutoColorObject(subo, Color, Opacity))
	return ERROR;
    break;
  case CLASS_SCREEN:
    if (!DXGetScreenInfo((Screen)o, &subo, NULL, NULL))
	return ERROR;
    if (!_dxfAutoColorObject(subo, Color, Opacity))
	return ERROR;
    break;
  case CLASS_GROUP:
    /* get children and call AutoColorObject(subo); */
    for (i=0; (subo=DXGetEnumeratedMember((Group)o, i, NULL)); i++){ 
      if (!_dxfAutoColorObject(subo, Color, Opacity))
	return ERROR;
    }
    break;
  case CLASS_FIELD:
    a.a_f = (Field)o;
    a.a_color = Color;
    a.a_opacity = Opacity;
    
    /* and schedule autocolorfield to be called */
    if (!(DXAddTask(AutoColorField, (Pointer)&a, sizeof(a), 1.0))) {
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



static Error AutoColorField(Pointer ptr)


{
  Field f, savedfield;
  Object ColorMap, OpacityMap;
  Interpolator i;
  int scalar, floatflag;
  Array newdata, olddata;
  arg *a;
  
  a = (arg *)ptr;
  f = a->a_f;
  ColorMap = a->a_color;
  OpacityMap = a->a_opacity;
  newdata = NULL;
  olddata = NULL;
  savedfield = NULL;

  /* if it's an empty field, or a field without data, ignore it */
  if (!(GoodField(f))) return OK;

  /* if it's not a good data field, return on ERROR */
  if (!(_dxfScalarField(f, &scalar))) return ERROR;

  /* because a map's positions represent the data values, and because
     positions are float, I need to convert the data array to float to
     use map to do the color mapping */
  if (!(_dxfFloatField((Object)f,&floatflag))) return ERROR;

  if ((!scalar)||(!floatflag)) {
     /* only bother to convert the data if you need to, that is, either
      * opacity map or color map is a real map, not just a vector */
     if (((OpacityMap)&&(DXGetObjectClass(OpacityMap)!=CLASS_ARRAY)) ||
           ((ColorMap)&&(DXGetObjectClass(ColorMap)!=CLASS_ARRAY))) {
        /* copy the field and make a new data component for map to work on */
        savedfield = (Field)DXCopy((Object)f,COPY_STRUCTURE);
        if (!(olddata = (Array)DXGetComponentValue((Field)f, "data"))) {
            return ERROR;
        }
        if (!(newdata = DXScalarConvert(olddata))) {
           return ERROR;
        }
        if (!(DXSetComponentValue((Field)f, "data", (Object)newdata)))
            return ERROR;
     }
  }

  if (OpacityMap) {

    /* don't make the interpolator if it's a simple vector for the map */
    if (DXGetObjectClass((Object)OpacityMap) == CLASS_ARRAY) 
    {
      /*f = (Field)DXMap((Object)f, (Object)OpacityMap, "data", "opacities");*/
      if (!DXMap((Object)f, (Object)OpacityMap, "data", "opacities"))
      {
	goto error;
      }
    }
    else
    {
      if (! DXMapCheck((Object)f, OpacityMap, "data", NULL, NULL, NULL, NULL))
      {
	goto error;
      }

      i = DXNewInterpolator((Object)OpacityMap, INTERP_INIT_IMMEDIATE, 0.0);
      if (! i)
         goto error;

      /*f = (Field)DXMap((Object)f, (Object)i, "data", "opacities");*/
      if (!DXMap((Object)f, (Object)i, "data", "opacities"))
      {
	goto error;
      }

      DXDelete((Object)i);
    }
  }

  if (ColorMap) {

    /* don't make the interpolator if it's a simple vector for the map */
    if (DXGetObjectClass((Object)ColorMap) == CLASS_ARRAY) 
    {
      /*f = (Field)DXMap((Object)f, (Object)ColorMap, "data", "colors");*/
      if (!DXMap((Object)f, (Object)ColorMap, "data", "colors"))
      {
	goto error;
      }
    }
    else
    {
      if (! DXMapCheck((Object)f, ColorMap, "data",  NULL, NULL, NULL, NULL))
      {
	goto error;
      }

      i = DXNewInterpolator((Object)ColorMap, INTERP_INIT_IMMEDIATE, 0.0);
      if (! i)
	goto error;

      /*f = (Field)DXMap((Object)f, (Object)i, "data", "colors"); */
      if (!DXMap((Object)f, (Object)i, "data", "colors"))
      {
	goto error;
      }

      DXDelete((Object)i);
    }
  }

  /* now remove any "front colors" or "back colors" components which
     may be hanging around */
     if (DXGetComponentValue((Field)f,"front colors"))
         if( !(DXDeleteComponent((Field)f, "front colors"))) goto error;
     if (DXGetComponentValue((Field)f,"back colors"))
         if (!(DXDeleteComponent((Field)f, "back colors"))) goto error;

  if (savedfield) {
     /* we need to get the data from savedfield and put it in f, then
      * delete savedfield */
      f = 
	  (Field)DXReplace((Object)f, (Object)savedfield, "data", "data");
      if (!f) {
	     DXSetError(ERROR_INTERNAL,"can't replace data in field");
	     goto error;
      }
      DXDelete((Object)savedfield);
  }

  DXDeleteComponent(f,"color map");  
  if (OpacityMap) DXDeleteComponent(f,"opacity map");  
  if (!DXEndField(f))
     goto error;
  return OK;

  error:
    return ERROR;
}


static Error AutoColorDelayedObject(Object o, int setopacities,
                                 float givenmin, float givenmax, int surface,
				 float opacity, float huestart, float range,
                                 float saturation, float intensity, 
                                 RGBColor lowerrgb, RGBColor upperrgb)
{
  int i;
  Object subo;
  arg_byte a;
  
  
  switch(DXGetObjectClass(o)) {
  case CLASS_GROUP:
    /* get children and call AutoColorObject(subo); */
    for (i=0; (subo=DXGetEnumeratedMember((Group)o, i, NULL)); i++){ 
      if (!AutoColorDelayedObject(subo, setopacities, givenmin, givenmax, 
                                  surface, opacity, huestart, range, 
                                  saturation, intensity, lowerrgb, upperrgb))
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
    a.a_huestart = huestart;
    a.a_huerange = range;
    a.a_saturation = saturation;
    a.a_intensity = intensity;
    a.a_lowerrgb = lowerrgb;
    a.a_upperrgb = upperrgb;
    
    /* and schedule autocolorfield to be called */
    if (!(DXAddTask(AutoColorDelayedField, (Pointer)&a, sizeof(a), 1.0))) {
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



static Error AutoColorDelayedField(Pointer ptr)


{
  Field f;
  int scalar, i, setopacities, surface,numentries;
  float givenmin, givenmax,opacity,r,g,b;
  float *opacityarray=NULL, huestart, range,saturation,intensity,hue;
  float dmin, dmax;
  int datamin, datamax;
  RGBColor *colorarray=NULL;
  RGBColor lowerrgb, upperrgb;
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
  huestart = a->a_huestart;
  range = a->a_huerange;
  saturation = a->a_saturation;
  intensity = a->a_intensity;
  lowerrgb = a->a_lowerrgb;
  upperrgb = a->a_upperrgb;
  deferredcolors = NULL;
  deferredopacities = NULL;

  /* if it's an empty field, or a field without data, ignore it */
  if (!(GoodField(f))) return OK;
  
  a_data = (Array)DXGetComponentValue((Field)f,"data"); 
  if (!a_data) {
    /* "missing data component" */
    DXSetError(ERROR_MISSING_DATA,"#10240","data");
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
    /* need to figure out how many entries I need */
    opacityarray = DXAllocateLocal(numentries*sizeof(float));
    if (!opacityarray) {
      DXAddMessage("cannot allocate delayed opacity map with %d entries",
		   numentries);
      goto error;
    }
    if (surface) {
      for(i=0; i<=numentries; i++) {
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
  colorarray = DXAllocateLocal(numentries*sizeof(RGBColor));
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
    if (givenmin==givenmax) hue = huestart;
    else hue =huestart + (i-givenmin)/(givenmax-givenmin)*range;
    if (!_dxfHSVtoRGB(hue,saturation,intensity,&r,&g,&b)) 
      goto error;
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

  if (!(DXAddArrayData(deferredcolors,0,numentries,(Pointer)colorarray))) 
    goto error;
  
  if (!(DXSetComponentValue((Field)f, "color map", 
			    (Object)deferredcolors))) 
    goto error;

  /* need to delete the old colors component first. This is because
   * it may have an incorrect attribute which will screw up (believe me
   * it will) both the new colors component AND the data component, since
   * they are now the same */
  if (!(DXSetComponentValue((Field)f, "colors", NULL)))
    goto error; 
  if (!(DXSetComponentValue((Field)f, "colors", (Object)a_data))) 
    goto error;

  if (setopacities) {
    if (!(deferredopacities = (Array)DXNewArray(TYPE_FLOAT, 
                                                CATEGORY_REAL,0,NULL))) 
      goto error;

    if (!(DXAddArrayData(deferredopacities,0,numentries,
			 (Pointer)opacityarray))) 
	goto error;

    if (!(DXSetComponentValue((Field)f, "opacity map",
			      (Object)deferredopacities))) 
      goto error;
    
    if (!(DXSetComponentValue((Field)f, "opacities", (Object)a_data))) 
	goto error;
    
  }
  
  
  DXEndField(f); 
  DXFree((Pointer)colorarray);
  return OK;
  
 error:
  DXDelete((Object)deferredopacities);
  DXDelete((Object)deferredcolors);
  DXFree((Pointer)opacityarray);
  DXFree((Pointer)colorarray);
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



Error _dxfIsTextField(Field f, int *text)
{
  Array a;
  Type type;

  if (!(a = (Array)DXGetComponentValue(f,"data"))) {
    *text = 0;
    return OK;
  }
  if (!DXGetArrayInfo((Array)a, NULL, &type, NULL, NULL, NULL)) 
    return ERROR;
  if (type == TYPE_STRING)
    *text=1;
  else 
    *text=0;
  return OK;
}


Error _dxfScalarField(Field f, int *scalar) 
{
  Array a;
  Type type;
  Category category;
  int rank, shape;


  if (!(a = (Array)(DXGetComponentValue(f, "data")))) {
     *scalar=1;
     return OK;
  }
  if (!(DXGetArrayInfo((Array)a, NULL, &type, &category, &rank, &shape))) {
     DXSetError(ERROR_BAD_PARAMETER,"bad data component");
     return ERROR;
  }
  if (!(category == CATEGORY_REAL)) {
     DXSetError(ERROR_BAD_PARAMETER,"non-real data field");
     return ERROR;
  }
  if ((rank == 0) || ((rank == 1)&&(shape == 1))) {
     *scalar = 1;
     return OK;
  }
  if (rank>=1) {
     *scalar = 0;
     return OK;
  }
  /*
  DXSetError(ERROR_BAD_PARAMETER,"data field not scalar or vector");
  return ERROR;
  */
  return OK;
}



Error _dxfFloatField(Object f, int *floatflag)
{
  Array a;
  Type type;
  int i;
  Object sub;

  *floatflag = 1;

  switch (DXGetObjectClass(f)) {
    case (CLASS_FIELD):
     if (DXEmptyField((Field)f)) return OK;

     if (!(a = (Array)(DXGetComponentValue((Field)f, "data")))) {
       return OK;
     }
     if (!(DXGetArrayInfo((Array)a, NULL, &type, NULL, NULL, NULL))) {
       DXSetError(ERROR_BAD_PARAMETER,"bad data component");
       return ERROR;
     }
     if (type != TYPE_FLOAT) {
       *floatflag = 0;
     }
     break;
    case (CLASS_GROUP):
     /* if any member of the group has a non-float data component, set
        the flag to 0 */
     i = 0;
     while ((sub=(Object)DXGetPart((Object)f, i))) {
       i++;
       if (!(a = (Array)(DXGetComponentValue((Field)sub, "data")))) {
         continue; 
       }
       if (!(DXGetArrayInfo((Array)a, NULL, &type, NULL, NULL, NULL))) {
         DXSetError(ERROR_BAD_PARAMETER,"bad data component");
         return ERROR;
       }
       if (type != TYPE_FLOAT) {
         *floatflag = 0;
         goto done;
       }
     }
     break;
     default:
     break;
   }
done:
   return OK;
}



Error _dxfByteField(Object g_out, int *byteflag)
{
  int foundone, i, rank, shape;
  Field f;
  Class class;
  Array a;
  Category category;
  Type type;

  *byteflag = 0;
  if (!(class=DXGetObjectClass((Object)g_out)))
     return ERROR;
  if (class == CLASS_FIELD) {
     if (DXEmptyField((Field)g_out)) {
        return OK;
     }
  }
  
  /*  I'm just getting the first part that I find.  */
  foundone = 0;
  i = 0;
  while ((f=DXGetPart((Object)g_out, i))) {
    i++;
    if (DXEmptyField(f))
      continue; 
    else {
      foundone = 1; 
      break; 
    } 
  }
  /* presumably all empty */
  if (!(foundone)) {
   *byteflag = 0;
   return OK; 
  }

  if (!(a = (Array)(DXGetComponentValue(f, "data")))) {
     return OK;
  }
  if (!(DXGetArrayInfo((Array)a, NULL, &type, &category, &rank, &shape))) {
     DXSetError(ERROR_BAD_PARAMETER,"bad data component");
     return ERROR;
  }
  if (type == TYPE_UBYTE) {
     if (!((rank==0)||((rank==1)&&(shape==1)))) {
         /* byte vectors are to be treated at floats (no lookup) */
         *byteflag=0;
         return OK;
     }	
     *byteflag=1;
     if (!(category==CATEGORY_REAL)) {
	  DXSetError(ERROR_BAD_PARAMETER,"must be category real");
          return ERROR;
     }	
  }
  return OK;
}


Error _dxfDelayedOK(Object g_out)
{
  int foundone, i, rank, shape;
  Field f;
  Class class;
  Array a;
  Category category;
  Type type;

  if (!(class=DXGetObjectClass((Object)g_out)))
     return ERROR;
  if (class == CLASS_FIELD) {
     if (DXEmptyField((Field)g_out)) {
        return OK;
     }
  }

  /*  I'm just getting the first part that I find.  */
  foundone = 0;
  i = 0;
  while ((f=DXGetPart((Object)g_out, i))) {
    i++;
    if (DXEmptyField(f))
      continue;
    else {
      foundone = 1;
      break;
    }
  }
  /* presumably all empty */
  if (!(foundone)) {
   return OK;
  }

  if (!(a = (Array)(DXGetComponentValue(f, "data")))) {
     return OK;
  }
  if (!(DXGetArrayInfo((Array)a, NULL, &type, &category, &rank, &shape))) {
     DXSetError(ERROR_BAD_PARAMETER,"bad data component");
     return ERROR;
  }
  /*
  if ((type != TYPE_UBYTE)&&(type != TYPE_BYTE)&&(type != TYPE_INT)&&
      (type != TYPE_UINT)&&(type != TYPE_SHORT)&&(type != TYPE_USHORT)) { 
         DXSetError(ERROR_BAD_PARAMETER,
                    "delayed option only valid for byte, integer, and short scalar data");
         return ERROR;
  }
  */
  if (type != TYPE_UBYTE) { 
         DXSetError(ERROR_BAD_PARAMETER,"#11821");
         return ERROR;
  }
  if (!((rank==0)||((rank==1)&&(shape==1)))) {
         DXSetError(ERROR_BAD_PARAMETER,
                    "delayed option only valid for scalar data");
         return ERROR;
  } 
  return OK;
}

int _dxfIsVolume(Object g_out, int *surface)
{
  int foundone, i;
  Field f;
  char *el_type;
  Class class;

  if (!(class=DXGetObjectClass((Object)g_out)))
     return ERROR;
  if (class == CLASS_FIELD) {
     if (DXEmptyField((Field)g_out)) {
        *surface = 1;
        return OK;
     }
  }
  
  /*  I'm just getting the first part that I find.  */
  *surface = 0;
  foundone = 0;
  i = 0;
  while ((f=DXGetPart((Object)g_out, i))) {
    i++;
    if (DXEmptyField(f))
      continue; 
    else {
      foundone = 1; 
      break; 
    } 
  }
  if (!(foundone)) {
    *surface = 1;
    return OK; 
  }

  el_type  = NULL;
  if ((el_type= DXGetString((String)DXGetComponentAttribute((Field)f,
		       "connections", "element type"))) != NULL) {
    if ( !strcmp(el_type,"triangles") || !strcmp(el_type,"quads") ||
	!strcmp(el_type,"faces") || !strcmp(el_type,"lines") ) 
      *surface = 1;
    else if ( !strcmp(el_type,"cubes") || !strcmp(el_type,"tetrahedra") )
      *surface = 0;
    else {
      DXSetError(ERROR_DATA_INVALID,"unrecognized connections");
      return ERROR; 
    } 
  } 
  else  {
    /*    if no connections, assume surface */
    *surface = 1;
  }
  return OK;
}


Error
_dxfHSVtoRGB(float h, float s, float v, float *red, float *green, float *blue)
/*   algorithm is from Fundamentals of Interactive Computer Graphics
     by Foley and Van Dam, 1984   */
{
  
  int i;
  float f,p,q,t,valfactor;
 
  /* first convert from 0 -> 1 to 0 -> 360 */
  h = h*360.0;

  if (h < 0.0)   h = h + ((int)(-h/360)+1)*  360.0;
  if (h > 360.0) h = h - ((int)( h/360))  *  360.0;

  /* do the checking */
  if ((s < 0.0)||(s > 1.0)) {
     DXSetError(ERROR_BAD_PARAMETER,"bad SATURATION value");
     return ERROR;
  }
  if (v < 0.0) {
     DXSetError(ERROR_BAD_PARAMETER,"bad VALUE value");
     return ERROR;
  }
  valfactor = 1.0;
  if (v > 1.0) {
     valfactor = v;
     v = 1.0;
  }
  
  if (s==0.0)
    {
      *red = v*valfactor;
      *green = v*valfactor;
      *blue = v*valfactor;
    }
  else   
    { 
      if (h==360.0) h=0.0;
      h=h/60.0;
      i=h;
      f=h-i;
      p=v*(1.0-s);
      q=v*(1.0-(s*f));
      t=v*(1.0-(s*(1.0-f)));
      switch(i)
        {	
	case 0:
	  *red=v*valfactor; *green=t*valfactor,*blue=p*valfactor;
	  break;
	case 1:
	  *red=q*valfactor; *green=v*valfactor,*blue=p*valfactor;
	  break;
	case 2:
	  *red=p*valfactor; *green=v*valfactor,*blue=t*valfactor;
	  break;
	case 3:
	  *red=p*valfactor; *green=q*valfactor,*blue=v*valfactor;
	  break;
	case 4:
	  *red=t*valfactor; *green=p*valfactor,*blue=v*valfactor;
	  break;
	case 5:
	  *red=v*valfactor; *green=p*valfactor,*blue=q*valfactor;
	  break;
	};
      
    }
    return OK; 
}  







Error  
RGBtoHLS(float r, float g, float b, float *h, float *l, float *s)
/*   algorithm is from Fundamentals of Interactive Computer Graphics
     by Foley and Van Dam, 1984   */
{
  float min, max, delta;

  min = r;
  if (g < min) min = g;
  if (b < min) min = b;

  max = r;
  if (g > max) max = g;
  if (b > max) max = b;

  if ((min < 0.0)||(max > 1.0)) {
     DXSetError (ERROR_BAD_PARAMETER,"bad rgb value");
     return ERROR;
  }

  *l = (max + min)/2.0;

  if (max == min) {
     *s = 0.0;
     *h = 0.0;
  }

  else {
     delta = max - min;
     if (*l <= 0.5) 
	*s = delta/(max + min);
     else
	*s = delta/(2.0 - max - min);
     if (r == max)
	*h = (g - b)/delta;
     else if (g == max)
	*h = 2.0 + (b - r)/delta;
     else if (b == max)
	*h = 4.0 + (r - g)/delta;
     *h = *h * 60.0;
     if (*h < 0.0) *h = *h + 360;
  }
  return OK;
} 

Error  
HLStoRGB(float h, float l, float s, float *r, float *g, float *b)
/*   algorithm is from Fundamentals of Interactive Computer Graphics
     by Foley and Van Dam, 1984   */
{
  float m2, m1;
 
  if (h < 0.0)   h = h + ((int)(-h/360)+1)*  360.0;
  if (h > 360.0) h = h - ((int)( h/360))  *  360.0;

  if (l <= 0.5)
     m2 = l*(1.0 + s);
  else
     m2 = l + s - l*s;
  m1 = 2.0*l - m2;
  if (s == 0.0) {
     *r = l;
     *g = l;
     *b = l;
  }
  else {
     *r = ValueFcn(m1,m2, h + 120.0);
     *g = ValueFcn(m1, m2, h);
     *b = ValueFcn(m1, m2, h - 120.0);
  }
  return OK;
} 

static float ValueFcn(float n1, float n2, float hue)
{
   if (hue > 360.0)
      hue = hue - 360.0;
   else if (hue < 0.0)
      hue = hue + 360.0;
   if (hue < 60.0)
      return (n1 + (n2 - n1)*(240.0 - hue)/60.0);
   else if (hue < 180.0)
      return n2;
   else if (hue < 240.0)
      return (n1 + (n2 - n1)*(240.0 - hue)/60.0);
   else
      return n1;
}





Error  
_dxfRGBtoHSV(float r, float g, float b, float *h, float *s, float *v)
/*   algorithm is from Fundamentals of Interactive Computer Graphics
     by Foley and Van Dam, 1984   */
{
 
  float max, min, delta; 

  max = r;
  if (g > max) max = g;
  if (b > max) max = b;

  min = r;
  if (g < min) min = g;
  if (b < min) min = b;

  if ((max > 1.0)||(min < 0.0)) {     
     DXSetError(ERROR_BAD_PARAMETER,"#10110", "r g b values", 0, 1);
     return ERROR;
  }

  *v = max;

  if (max != 0.0) 
     *s = (max - min)/max;
  else
     *s = 0.0;

  if (max == 0.0) 
     *h = 0.0;
  else {
     delta = max - min;
     if (delta != 0.0) {
       if (r == max)
  	  *h = (g - b)/delta;
       else if (g == max) 
	  *h = 2.0 + (b - r)/delta;
       else if (b == max)
	  *h = 4.0 + (r - g)/delta;
       *h = *h*60.0;

       if (*h < 0.0)
  	  *h = *h + 360.0;
       }
       else
	  *h = 0.0;
    } 
    /* convert from 0 to 360 to 0 to 1 */
    *h = *h/ 360.0;

    return OK; 
}  

Error _dxfBoundingBoxDiagonal(Object ob, float *thickness)
{
  Point boxpoint[8];

  if (!DXBoundingBox((Object)ob, boxpoint)) {
    return ERROR;
  }

  *thickness = DXLength(DXSub(boxpoint[7], boxpoint[0])); 
  if (*thickness == 0.0) {
    DXSetError(ERROR_DATA_INVALID,
	     "bounding box for volume has zero thickness");
    return ERROR;
  }
  return OK;
}


static int direction(int value)
{
  if (value >= 0)
      return 1;
  else
      return 0;
}

static int sign(int value)
{
  if (value >= 0)
      return 1;
  else
      return -1;
}


int _dxfFieldWithInformation(Object ob)
{
  Object subo;
  int i;


  switch(DXGetObjectClass(ob)) {
  case CLASS_GROUP:
    /* get children; */
    for (i=0; (subo=DXGetEnumeratedMember((Group)ob, i, NULL)); i++)
      if (_dxfFieldWithInformation(subo))
         return 1;
      break;

  case CLASS_FIELD: 
    if (!(DXEmptyField((Field)ob)))
      return 1;
    break;

  case CLASS_XFORM:
    if (!(DXGetXformInfo((Xform)ob, &subo, NULL)))
      return 0;
    if (_dxfFieldWithInformation(subo))
      return 1;
    break;

  case CLASS_CLIPPED:
    if (!(DXGetClippedInfo((Clipped)ob, &subo, NULL)))
      return 0;
    if (_dxfFieldWithInformation(subo))
      return 1;
    break; 
  default:
    return 1;
  }
  return 0;
}





Field _dxfMakeRGBColorMap(Field mapfield)
     /* converts an hsv color map to an rgb color map */
{
  int icountnew, icountoriginal, i, countpos, countdata, rank, shape, j;
  int numnew, numnew1, numnew2, numnew3;
  int huestep0, huestep1, huestepdiff, deppos;
  float *dp_f=NULL, *pp;
  int *dp_i=NULL;
  uint *dp_ui=NULL;
  short *dp_s=NULL;
  ushort *dp_us=NULL;
  byte *dp_b=NULL;
  ubyte *dp_ub=NULL;
  Type type, datatype;
  char *datadep;
  Category category;
  float lasthue=0, lastsat=0, lastval=0;
  float tmphue, tmpsat, tmpval, tmppos, lasttmppos;
  float lastpos, pos, dpos;
  float red, green, blue;
  float abshue, abssat, absval;
  float hue=0, sat=0, val=0;
  float dhue, dsat, dval;
  float maxhuediff, maxsatdiff, maxvaldiff, fuzz;
  RGBColor newcolor;
  Array a_positions, a_data, a_newpositions, a_newdata, a_newconnections;
  Field newmapfield;

  fuzz = 0.0;
  datadep = NULL;
  
  if (!(datadep=DXGetString((String)
			  DXGetComponentAttribute(mapfield,"data","dep")))) {
    DXSetError(ERROR_DATA_INVALID,"missing data dependent attribute");
    return ERROR;
  }
  if (strcmp(datadep,"positions")) 
    deppos = 0;
  else
    deppos = 1;
  
  if (!(newmapfield = DXNewField())) {
    return ERROR;
  }
  
  a_newdata = NULL;
  a_newpositions = NULL;
  a_newconnections = NULL;


  /* set the criteria for adding new points to the map */
  /* this should result in max error of 0.001 in r, g, and b 
     from an exact transformation from a straight line in hsv 
     maxhuediff = 0.01306;
     maxsatdiff = 0.013;
     maxvaldiff = 0.013; 
     */

  /* set the criteria for adding new points to the map */
  /* this should result in max error of 0.01 in r, g, and b 
     from an exact transformation from a straight line in hsv */
  if (deppos) {
    maxhuediff = 0.0389;
    maxsatdiff = 0.04;
    maxvaldiff = 0.04; 
  }
  else {
    maxhuediff = DXD_MAX_FLOAT;
    maxsatdiff = DXD_MAX_FLOAT;
    maxvaldiff = DXD_MAX_FLOAT; 
  }



  if (!(a_positions =
	(Array)DXGetComponentValue((Field)mapfield,"positions"))) {
    DXSetError(ERROR_BAD_PARAMETER,"map has no positions component");
    goto cleanup;
  }
  if (!DXGetComponentValue((Field)mapfield,"connections")) {
    DXSetError(ERROR_BAD_PARAMETER,"map has no connections component");
    goto cleanup;
  }
  if (!(a_data = (Array)DXGetComponentValue((Field)mapfield,"data"))) {
    DXSetError(ERROR_BAD_PARAMETER,"map has no data component");
    goto cleanup;
  }
  if (DXGetArrayClass(a_data) != CLASS_ARRAY) {
    DXSetError(ERROR_BAD_PARAMETER,"map is bad class");
    goto cleanup;
  }
  /* get size and shape */
  if (!(DXGetArrayInfo(a_data, &countdata, &datatype, &category, 
		     &rank, &shape)))  goto cleanup;
  if((rank != 1) || (category != CATEGORY_REAL) || (shape != 3)) {
    DXSetError(ERROR_BAD_PARAMETER,
	    "data (rank != 1) or (non-real) or (vec len != 3)");
    goto cleanup;
  }
  
  /* make the new arrays*/
  if (!(a_newdata = DXNewArray(TYPE_FLOAT, CATEGORY_REAL, 1, 3))) goto cleanup;
  if (!(a_newpositions = DXNewArray(TYPE_FLOAT,
				  CATEGORY_REAL, 1, 1)))  {
    DXDelete((Object)a_newdata);				  
    return ERROR;
  }
  
  if (DXGetArrayClass(a_positions) != CLASS_ARRAY) {
    DXSetError(ERROR_BAD_PARAMETER,"map positions are bad class");
    goto cleanupnofield;
  }
  /* get size and shape */
  if (!(DXGetArrayInfo(a_positions,
         &countpos, &type, &category, &rank, &shape))) 
     goto cleanup;
  if ((category != CATEGORY_REAL) || (type != TYPE_FLOAT)) {
    DXSetError(ERROR_BAD_PARAMETER, "positions (non-real) or (not float)");
    goto cleanupnofield;
  }
  if (!( (rank == 0) || ((rank == 1)&&(shape == 1)))) {
    DXSetError(ERROR_BAD_PARAMETER, "non-scalar positions");
    goto cleanupnofield;
  }
 
  switch (datatype) {
    case (TYPE_FLOAT):
       dp_f = (float *)DXGetArrayData(a_data);
       if (!dp_f) goto cleanupnofield;
       break;
    case (TYPE_INT):
       dp_i = (int *)DXGetArrayData(a_data);
       if (!dp_i) goto cleanupnofield;
       break;
    case (TYPE_UINT):
       dp_ui = (uint *)DXGetArrayData(a_data);
       if (!dp_ui) goto cleanupnofield;
       break;
    case (TYPE_SHORT):
       dp_s = (short *)DXGetArrayData(a_data);
       if (!dp_s) goto cleanupnofield;
       break;
    case (TYPE_USHORT):
       dp_us = (ushort *)DXGetArrayData(a_data);
       if (!dp_us) goto cleanupnofield;
       break;
    case (TYPE_BYTE):
       dp_b = (byte *)DXGetArrayData(a_data);
       if (!dp_b) goto cleanupnofield;
       break;
    case (TYPE_UBYTE):
       dp_ub = (ubyte *)DXGetArrayData(a_data);
       if (!dp_ub) goto cleanupnofield;
       break;
    default:
       DXSetError(ERROR_BAD_PARAMETER,"unknown data type");
       goto cleanupnofield;
    }
  if (!(pp= (float *)DXGetArrayData(a_positions))) { 
    goto cleanupnofield;
  }
 

  if (deppos) { 
    icountnew = 0;   
    icountoriginal = 0;   
    switch (datatype) {
      case (TYPE_FLOAT):
        lasthue = dp_f[0];
        lastsat = dp_f[1];
        lastval = dp_f[2];
        break; 
      case (TYPE_INT):
        lasthue = (float)dp_i[0];
        lastsat = (float)dp_i[1];
        lastval = (float)dp_i[2];
        break; 
      case (TYPE_UINT):
        lasthue = (float)dp_ui[0];
        lastsat = (float)dp_ui[1];
        lastval = (float)dp_ui[2];
        break; 
      case (TYPE_SHORT):
        lasthue = (float)dp_s[0];
        lastsat = (float)dp_s[1];
        lastval = (float)dp_s[2];
        break; 
      case (TYPE_USHORT):
        lasthue = (float)dp_us[0];
        lastsat = (float)dp_us[1];
        lastval = (float)dp_us[2];
        break; 
      case (TYPE_BYTE):
        lasthue = (float)dp_b[0];
        lastsat = (float)dp_b[1];
        lastval = (float)dp_b[2];
        break; 
      case (TYPE_UBYTE):
        lasthue = (float)dp_ub[0];
        lastsat = (float)dp_ub[1];
        lastval = (float)dp_ub[2];
        break; 
      default:
        break;
    }
    lastpos = pp[0];
    if (!(_dxfHSVtoRGB(lasthue, lastsat, lastval,
		   &red, &green, &blue))) goto cleanupnofield;
    
    newcolor = DXRGB(red,green,blue);
    if (!(DXAddArrayData(a_newdata,icountnew,1,(Pointer)&newcolor))) 
      goto cleanupnofield;
    if (!(DXAddArrayData(a_newpositions,icountnew,
		       1,(Pointer)&lastpos))) goto cleanupnofield;
    icountnew++;
    icountoriginal++;
    
    
    /* run through the original map */
    for (i = 3; i < 3*countpos; i=i+3) {
    switch (datatype) {
      case (TYPE_FLOAT):
        hue = dp_f[i];
        sat = dp_f[i+1];
        val = dp_f[i+2];
        break; 
      case (TYPE_INT):
        hue = (float)dp_i[i];
        sat = (float)dp_i[i+1];
        val = (float)dp_i[i+2];
        break; 
      case (TYPE_UINT):
        hue = (float)dp_ui[i];
        sat = (float)dp_ui[i+1];
        val = (float)dp_ui[i+2];
        break; 
      case (TYPE_SHORT):
        hue = (float)dp_s[i];
        sat = (float)dp_s[i+1];
        val = (float)dp_s[i+2];
        break; 
      case (TYPE_USHORT):
        hue = (float)dp_us[i];
        sat = (float)dp_us[i+1];
        val = (float)dp_us[i+2];
        break; 
      case (TYPE_BYTE):
        hue = (float)dp_b[i];
        sat = (float)dp_b[i+1];
        val = (float)dp_b[i+2];
        break; 
      case (TYPE_UBYTE):
        hue = (float)dp_ub[i];
        sat = (float)dp_ub[i+1];
        val = (float)dp_ub[i+2];
        break; 
      default:
        break;
    }
      pos = pp[icountoriginal];
      dhue = hue - lasthue;
      abshue = ABS(dhue);
      dsat = sat - lastsat;
      abssat = ABS(dsat);
      dval = val - lastval;
      absval = ABS(dval);
      dpos = pos - lastpos;

      /* see if we are about to cross discontinuity */
      huestep1 = floor(hue/60.0);
      huestep0 = floor(lasthue/60.0);
      huestepdiff = (huestep1 - huestep0);
      
      if ((abshue < maxhuediff)&&(abssat < maxsatdiff)&&
  	  (absval < maxvaldiff) && (huestepdiff == 0)) {
	if (!(_dxfHSVtoRGB(hue, sat, val, &red, &green, &blue))) 
	  goto cleanupnofield;
	newcolor = DXRGB(red,green,blue);
	if (!(DXAddArrayData(a_newdata,icountnew,
			   1,(Pointer)&newcolor))) goto cleanupnofield;
        if (!(DXAddArrayData(a_newpositions,icountnew,
			   1,(Pointer)&pos))) goto cleanupnofield;
        icountnew++;
        icountoriginal++;
        lasthue = hue;
        lastsat = sat;
        lastval = val;
        lastpos = pos;
      }
      /* we need to do some interpolation */
      else {
        numnew1 = 0;
        numnew2 = 0;
        numnew3 = 0;
        if (abshue > maxhuediff) numnew1 = abshue/maxhuediff;
        if (abssat > maxsatdiff) numnew2 = abssat/maxsatdiff;
        if (abshue > maxvaldiff) numnew3 = absval/maxvaldiff;
        numnew = 1;
        if (numnew1 > numnew) numnew = numnew1;
        if (numnew2 > numnew) numnew = numnew2;
        if (numnew3 > numnew) numnew = numnew3;
        numnew++;        
        tmppos = lastpos;
        for (j = 1; j < numnew; j++) {
   	  tmphue = lasthue + (((float)j)/((float)numnew))*dhue;
	  huestep1 = floor(tmphue/60.0);
          huestepdiff = huestep1 - huestep0; 
	  /* first do the discontinuity if necessary */
	  if (huestepdiff != 0) {
	    tmphue = (huestep0 + direction(huestepdiff))*60.0;
            huestep0 = huestep0 + sign(huestepdiff);
            lasttmppos = tmppos;
            tmpsat = lastsat + ((tmphue-lasthue)/(dhue))*dsat;
            tmpval = lastval + ((tmphue-lasthue)/(dhue))*dval;
            tmppos = lastpos + ((tmphue-lasthue)/(dhue))*dpos;
	    if (!(_dxfHSVtoRGB(tmphue, tmpsat, tmpval, &red, 
	  	 	   &green, &blue)))  goto cleanupnofield;
	    newcolor = DXRGB(red,green,blue);
            /* don't add it if it is the same as the previous one or 
             * one of the two end points */
            if ((ABS(((double)(tmppos - lastpos))) > fuzz) &&
                (ABS(((double)(pos - tmppos))) > fuzz) &&
                (ABS(((double)(tmppos - lasttmppos))) > fuzz)) {
	      if (!(DXAddArrayData(a_newdata,icountnew,
				 1,(Pointer)&newcolor))) goto cleanupnofield;
	      if (!(DXAddArrayData(a_newpositions,icountnew,
				 1, (Pointer)&tmppos)))  goto cleanupnofield;
	      icountnew++;
            }
	  }
	  /* now do the others */
	  tmphue = lasthue + (((float)j)/((float)numnew))*dhue;
	  tmpsat = lastsat + (((float)j)/((float)numnew))*dsat;
	  tmpval = lastval + (((float)j)/((float)numnew))*dval;
          lasttmppos = tmppos;
	  tmppos = lastpos + (((float)j)/((float)numnew))*dpos;
	  if (!(_dxfHSVtoRGB(tmphue, tmpsat, tmpval, &red, 
	  	         &green, &blue)))  goto cleanupnofield;
	  newcolor = DXRGB(red,green,blue);
          if ((ABS(tmppos - lastpos) > fuzz)&&(ABS(pos - tmppos) > fuzz)&&
	      (ABS(tmppos - lasttmppos) > fuzz)) {
	    if (!(DXAddArrayData(a_newdata,icountnew,
			       1,(Pointer)&newcolor))) goto cleanupnofield;
	    if (!(DXAddArrayData(a_newpositions,icountnew,
			       1,(Pointer)&tmppos)))  goto cleanupnofield;
	    icountnew++;
          }
        }
        /* now do the one I was originally going to do */
        if (!(_dxfHSVtoRGB(hue, sat, val, &red, &green, &blue))) 
	  goto cleanupnofield;
        newcolor = DXRGB(red,green,blue);
        if (!(DXAddArrayData(a_newdata,icountnew,
			   1,(Pointer)&newcolor))) goto cleanupnofield;
        if (!(DXAddArrayData(a_newpositions,icountnew,
			   1,(Pointer)&pos))) goto cleanupnofield;
        icountnew++;
        icountoriginal++;
        lasthue = hue;
        lastsat = sat;
        lastval = val; 
        lastpos = pos; 
      }
    }
    if (!(a_newconnections = DXMakeGridConnectionsV(1, &icountnew))) goto cleanup;
  }
  else {
    /* run through the original map */
    icountnew=0;
    for (i = 0; i < 3*countdata; i=i+3) {
      switch (datatype) {
        case (TYPE_FLOAT):
         lasthue = dp_f[i];
         lastsat = dp_f[i+1];
         lastval = dp_f[i+2];
         break;
        case (TYPE_INT):
         lasthue = (float)dp_i[i];
         lastsat = (float)dp_i[i+1];
         lastval = (float)dp_i[i+2];
         break;
        case (TYPE_UINT):
         lasthue = (float)dp_ui[i];
         lastsat = (float)dp_ui[i+1];
         lastval = (float)dp_ui[i+2];
         break;
        case (TYPE_SHORT):
         lasthue = (float)dp_s[i];
         lastsat = (float)dp_s[i+1];
         lastval = (float)dp_s[i+2];
         break;
        case (TYPE_USHORT):
         lasthue = (float)dp_us[i];
         lastsat = (float)dp_us[i+1];
         lastval = (float)dp_us[i+2];
         break;
        case (TYPE_BYTE):
         lasthue = (float)dp_b[i];
         lastsat = (float)dp_b[i+1];
         lastval = (float)dp_b[i+2];
         break;
        case (TYPE_UBYTE):
         lasthue = (float)dp_ub[i];
         lastsat = (float)dp_ub[i+1];
         lastval = (float)dp_ub[i+2];
         break;
        default:
	 break;
      }
      if (!(_dxfHSVtoRGB(lasthue, lastsat, lastval,
		     &red, &green, &blue))) goto cleanupnofield;
      newcolor = DXRGB(red,green,blue);
      if (!(DXAddArrayData(a_newdata,icountnew,1,(Pointer)&newcolor))) 
	goto cleanupnofield;
      icountnew++;
    }
    for (i=0; i< countpos; i++) {
      if (!(DXAddArrayData(a_newpositions,i,1,(Pointer)&pp[i]))) 
	goto cleanupnofield;
    }    
    if (!(a_newconnections = DXMakeGridConnectionsV(1, &countpos))) goto cleanup;
  }
  if (!(DXSetComponentValue((Field)newmapfield, "data",
			  (Object)a_newdata))) {
    DXDelete((Object)a_newpositions);
    goto cleanup;
  }
  if (!(DXSetComponentValue((Field)newmapfield,
			  "positions",(Object)a_newpositions))) {
    goto cleanup;
  }
  if (!(DXSetComponentValue((Field)newmapfield, "connections",
			  (Object)a_newconnections))) {
    DXDelete((Object)a_newconnections);			      
    goto cleanup;
  }
  if (deppos) {
    if (!(DXSetComponentAttribute((Field)newmapfield,"data", "dep",
				(Object)DXNewString("positions"))))  
      goto cleanup;
  }
  else {
    if (!(DXSetComponentAttribute((Field)newmapfield,"data", "dep",
				(Object)DXNewString(datadep))))  goto cleanup;
  }
  
  if (!(DXSetComponentAttribute((Field)newmapfield,"connections","ref",
			      (Object)DXNewString("positions"))))   goto cleanup;
  if (!(DXSetComponentAttribute((Field)newmapfield,"connections","element type",
			      (Object)DXNewString("lines"))))    goto cleanup;

  if (!DXEndField((Field)newmapfield))
     goto cleanup;
  
  
  return newmapfield;
  
 cleanupnofield:
  DXDelete ((Object)newmapfield); 
  DXDelete ((Object)a_newdata); 
  DXDelete ((Object)a_newpositions); 
  DXDelete ((Object)a_newconnections); 
  return ERROR;

 cleanup:
  DXDelete ((Object)newmapfield); 
  return ERROR;
}




Field _dxfMakeHSVfromRGB(Field mapfield)
/* converts an hsv color map to an rgb color map */
{
  Type type;
  Category category;
  Array a_positions, a_data, a_newpositions, a_newdata, a_newconnections;
  float *dp, *pp, maxsteprgb, newr, newg, newb, biggest;
  float hue, saturation, value, newpos, fuzz;
  float lastr, lastg, lastb, lastp;
  RGBColor newcolor;
  Field newmapfield;
  int count, rank, shape, i, j, icountnew, numnewsteps;

  fuzz = 0.0;

  if (!(newmapfield = DXNewField())) {
    return ERROR;
  }
  
  a_newdata = NULL;
  a_newpositions = NULL;
  a_newconnections = NULL;

  /* set the criteria for adding new points to the map */

  /* I changed this on 10/18/94 to a larger number. Seems ok from
   * tests I've run */
  maxsteprgb = 0.08;



  if (!(a_positions =
	(Array)DXGetComponentValue((Field)mapfield,"positions"))) {
    DXSetError(ERROR_BAD_PARAMETER,"map has no positions component");
    goto cleanupnofield;
  }
  if (!(a_data = (Array)DXGetComponentValue((Field)mapfield,"data"))) {
    DXSetError(ERROR_BAD_PARAMETER,"map has no data component");
    goto cleanupnofield;
  }
  if (DXGetArrayClass(a_data) != CLASS_ARRAY) {
    DXSetError(ERROR_BAD_PARAMETER,"map is bad class");
    goto cleanupnofield;
  }
  /* get size and shape */
  if (!(DXGetArrayInfo(a_data, &count, &type, &category, 
		     &rank, &shape)))  goto cleanupnofield;
  if((rank != 1) || (category != CATEGORY_REAL) || (type != TYPE_FLOAT) ||
     (shape != 3)) {
    DXSetError(ERROR_BAD_PARAMETER,
	    "data (rank != 1) or (non-real) or (not float) or (vec len != 3)");
    goto cleanupnofield;
  }
      
  /* make the new arrays*/
  if (!(a_newdata = DXNewArray(TYPE_FLOAT, CATEGORY_REAL, 1, 3))) goto cleanup;
  if (!(a_newpositions = DXNewArray(TYPE_FLOAT,
				  CATEGORY_REAL, 1, 1)))  
    goto cleanupnofield; 
  
  /* get size and shape */
  if (!(DXGetArrayInfo(a_positions,
		     &count, &type, &category, &rank, &shape))) goto cleanup;
  if ((category != CATEGORY_REAL) || (type != TYPE_FLOAT)) {
    DXSetError(ERROR_BAD_PARAMETER, "positions (non-real) or (not float)");
    goto cleanupnofield;
  }
  if (!( (rank == 0) || ((rank == 1)&&(shape == 1)))) {
    DXSetError(ERROR_BAD_PARAMETER, "non-scalar positions");
    goto cleanupnofield;
  }
  if (!(dp = (float *)DXGetArrayData(a_data))) {
    goto cleanupnofield;
  }
  if (!(pp= (float *)DXGetArrayData(a_positions))) { 
    goto cleanupnofield;
  }
  

  /* run through the original map */
  icountnew = 0;
  /* do the first point */
  if (!(_dxfRGBtoHSV(dp[0], dp[1], dp[2], &hue, &saturation, &value)))
    goto cleanupnofield;
  newcolor = DXRGB(hue,saturation,value);
  if (!(DXAddArrayData(a_newdata,icountnew,1,(Pointer)&newcolor)))
    goto cleanupnofield;
  if (!(DXAddArrayData(a_newpositions,icountnew,
		     1,(Pointer)&pp[0]))) goto cleanupnofield;
  icountnew++;
  lastr = dp[0];
  lastg = dp[1];
  lastb = dp[2];
  lastp = pp[0];
  for (i = 3; i < 3*count; i=i+3) {
    if ((ABS(dp[i]-lastr) > maxsteprgb)||
	(ABS(dp[i+1]-lastg) > maxsteprgb) ||
	(ABS(dp[i+2]-lastb) > maxsteprgb)) {
	  biggest = ABS(dp[i]-lastr);
	  if (ABS(dp[i+1]-lastg) > biggest) biggest = ABS(dp[i+1] - lastg);
	  if (ABS(dp[i+2]-lastb) > biggest) biggest = ABS(dp[i+2] - lastb);
	  numnewsteps = biggest/maxsteprgb + 1;
	  for (j=1; j<numnewsteps; j++) {
	    newr = j*(dp[i]-lastr)/numnewsteps + lastr; 
	    newg = j*(dp[i+1]-lastg)/numnewsteps + lastg; 
	    newb = j*(dp[i+2]-lastb)/numnewsteps + lastb; 
	    newpos = j*(pp[i/3]-lastp)/numnewsteps + lastp;
	    if (ABS(newpos - lastp)>fuzz) {
	    if (!(_dxfRGBtoHSV(newr, newg, newb, 
			   &hue, &saturation, &value))) goto cleanupnofield;
	    newcolor = DXRGB(hue,saturation,value);
	    if (!(DXAddArrayData(a_newdata,icountnew,1,(Pointer)&newcolor)))
	      goto cleanupnofield;
	    if (!(DXAddArrayData(a_newpositions,icountnew,
			       1,(Pointer)&newpos))) goto cleanupnofield;
	    icountnew++;
	    }
	  }
          if (!(_dxfRGBtoHSV(dp[i], dp[i+1], dp[i+2], &hue, &saturation, &value)))
	      goto cleanupnofield;
          newcolor = DXRGB(hue,saturation,value);
          newpos = pp[i/3];
          if (!(DXAddArrayData(a_newdata,icountnew,1,(Pointer)&newcolor)))
	     goto cleanupnofield;
          if (!(DXAddArrayData(a_newpositions,icountnew, 1,(Pointer)&newpos)))
	     goto cleanupnofield;
          icountnew++;
	}
    else {
      if (!(_dxfRGBtoHSV(dp[i], dp[i+1], dp[i+2], &hue, &saturation, &value)))
	goto cleanupnofield;
      newcolor = DXRGB(hue,saturation,value);
      newpos = pp[i/3];
      if (!(DXAddArrayData(a_newdata,icountnew,1,(Pointer)&newcolor)))
	goto cleanupnofield;
      if (!(DXAddArrayData(a_newpositions,icountnew, 1,(Pointer)&newpos)))
	goto cleanupnofield;
      icountnew++;
    }
    lastr = dp[i];
    lastg = dp[i+1];
    lastb = dp[i+2];
    lastp = pp[i/3];
  }


  if (!(DXSetComponentValue((Field)newmapfield, "data",
			  (Object)a_newdata))) 
    goto cleanupnofield;
  if (!(DXSetComponentValue((Field)newmapfield,
			  "positions",(Object)a_newpositions))) {
    DXDelete((Object)a_newpositions);
    DXDelete((Object)a_newconnections);
    goto cleanup;
  }
  if (!(a_newconnections = DXMakeGridConnectionsV(1, &icountnew)))  {
    DXDelete((Object)a_newconnections);
    goto cleanup;
  }
  if (!(DXSetComponentValue((Field)newmapfield, "connections",
			  (Object)a_newconnections))) {
    DXDelete((Object)a_newconnections);			      
    goto cleanup;
  }
  if (!(DXSetComponentAttribute((Field)newmapfield,"data", "dep",
			      (Object)DXNewString("positions"))))  goto cleanup;
  if (!(DXSetComponentAttribute((Field)newmapfield,"connections","ref",
			      (Object)DXNewString("positions"))))   goto cleanup;
  if (!(DXSetComponentAttribute((Field)newmapfield,"connections","element type",
			      (Object)DXNewString("lines"))))    goto cleanup;
  
  
  return newmapfield;
  
 cleanupnofield:
  DXDelete ((Object)newmapfield); 
  DXDelete ((Object)a_newdata); 
  DXDelete ((Object)a_newpositions); 
  DXDelete ((Object)a_newconnections); 
  return ERROR;

 cleanup:
  DXDelete ((Object)newmapfield); 
  return ERROR;
}







Error _dxfSetMultipliers(Object g_out, float unitvalue, float unitopacity)
{ 
   Class class, groupclass;
   int i;
   Object subo; 

   

   if (!(class = DXGetObjectClass((Object)g_out))) {
      return ERROR;
   }
   if (class == CLASS_GROUP) {
      if (!(groupclass = DXGetGroupClass((Group)g_out))) {
          return ERROR;
      }

      /* put the multiplier attribute on each member of the series, 'cause
         someone might extract them down the road */
      if (groupclass == CLASS_SERIES) {
         for (i=0; (subo=DXGetEnumeratedMember((Group)g_out, i, NULL)); i++){ 
             if (!(DXSetAttribute((Object)subo, "color multiplier", 
                 (Object)DXAddArrayData(DXNewArray(TYPE_FLOAT, 
                                                   CATEGORY_REAL, 0),
                                      0, 1, (Pointer)&unitvalue)))) 
                   return ERROR;
             if (!(DXSetAttribute((Object)subo, "opacity multiplier", 
                 (Object)DXAddArrayData(DXNewArray(TYPE_FLOAT, 
                                                   CATEGORY_REAL, 0),
                                      0, 1, (Pointer)&unitopacity)))) 
                   return ERROR;
         }
      }


      else {
             if (!(DXSetAttribute((Object)g_out, "color multiplier", 
                 (Object)DXAddArrayData(DXNewArray(TYPE_FLOAT, 
                                                   CATEGORY_REAL, 0),
                                      0, 1, (Pointer)&unitvalue)))) 
                  return ERROR;
             if (!(DXSetAttribute((Object)g_out, "opacity multiplier", 
                 (Object)DXAddArrayData(DXNewArray(TYPE_FLOAT, 
                                                   CATEGORY_REAL, 0),
                                      0, 1, (Pointer)&unitopacity)))) 
                  return ERROR;
      }
   }
   else {
      if (!(DXSetAttribute((Object)g_out, "color multiplier", 
           (Object)DXAddArrayData(DXNewArray(TYPE_FLOAT, CATEGORY_REAL, 0),
                                  0, 1, (Pointer)&unitvalue)))) 
                  return ERROR;
      if (!(DXSetAttribute((Object)g_out, "opacity multiplier", 
           (Object)DXAddArrayData(DXNewArray(TYPE_FLOAT, CATEGORY_REAL, 0),
                                  0, 1, (Pointer)&unitopacity)))) 
                  return ERROR;
   }
   return OK;
}  
