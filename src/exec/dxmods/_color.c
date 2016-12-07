/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


#include <math.h>
#include <ctype.h>
#include <dx/dx.h>
#include <string.h>
#include "_autocolor.h"
#include "color.h"

#define ISGENERICGROUP(x) ((DXGetObjectClass((Object)x) == CLASS_GROUP)&& \
			   (DXGetGroupClass((Group)x) == CLASS_GROUP))
#define LN(x) log((double)x)


struct arg {
  Field f;
  Object color;
  Object opacity;
  char component[30];
  int setcolor;
  int setopacity;
  int colorfield;
  int opacityfield;
  RGBColor colorvec;
  float opacityval;
  int immediate;
};

/* moved static prototypes to make ansi compiler happy.  nsc 21apr92 */
static Error ConvertColors(Object);
static Error ConvertOpacities(Object);
static Error ColorField(Pointer);
static Error ColorIndividual(Object, Object, Object, char *, 
			     int, int, int, int, RGBColor, float, int);
static Error ColorObject(Object, Object, Object, char *, 
			 int, int, int, int, RGBColor, float, int);


extern Array DXScalarConvert(Array); /* from libdx/stats.h */


Error 
  _dxfColorRecurseToIndividual(Object o, Object color, Object opacity, 
                               char *component, int setcolor, int setopacity, 
                               int colorfield, int opacityfield, 
                               RGBColor colorvec, float opacityval, 
                               int immediate)
{
  int i;
  Object subo, subcolor, subopacity;
  Class class, groupclass;

  if (!(class = DXGetObjectClass(o)))
    return ERROR;
  
  switch(class) {
  case CLASS_GROUP:
    if (!(groupclass = DXGetGroupClass((Group)o)))
      return ERROR;
    switch(groupclass) {

    case CLASS_SERIES:
    case CLASS_MULTIGRID:
    case CLASS_COMPOSITEFIELD:
      if (!ColorIndividual(o,color, opacity, component, setcolor,
			   setopacity, colorfield, opacityfield, colorvec,
			   opacityval, immediate))
	return ERROR;
      break;

    default:

      /* generic group; continue recursing */
      for (i=0; (subo=DXGetEnumeratedMember((Group)o,i,NULL)); i++) {
	if ((colorfield)&&(ISGENERICGROUP(color))) {
	  subcolor = DXGetEnumeratedMember((Group)color, i, NULL);
          if (!subcolor) {
	    DXSetError(ERROR_BAD_PARAMETER,"#11841","color map");
	    return ERROR;
          }
	}
	else
	  subcolor = color;
	if ((opacityfield)&&(ISGENERICGROUP(opacity))) {
	  subopacity = DXGetEnumeratedMember((Group)opacity, i, NULL);
          if (!subopacity) {
	    DXSetError(ERROR_BAD_PARAMETER,"#11841","opacity map");
	    return ERROR;
          }
        }
	else 
	  subopacity = opacity;
        if (!_dxfColorRecurseToIndividual(subo,subcolor, subopacity, 
				      component, setcolor, setopacity,
				      colorfield, opacityfield, colorvec,
				      opacityval, immediate))
	  return ERROR;
      }
    }
    break;
  case CLASS_FIELD:
    /* set arguments */
    if ((colorfield)&& (ISGENERICGROUP(color))) {
      DXSetError(ERROR_BAD_PARAMETER, "#11841", "color map");
      return ERROR;
    }
    if ((opacityfield)&& (ISGENERICGROUP(opacity))) {
      DXSetError(ERROR_BAD_PARAMETER,"#11841", "opacity map");
      return ERROR;
    }
    if (!ColorIndividual(o,color, opacity, component, setcolor,
			 setopacity, colorfield, opacityfield, colorvec,
			 opacityval, immediate))
      return ERROR;
    break; 
  case CLASS_SCREEN:
    if (!(DXGetScreenInfo((Screen)o, &subo, NULL, NULL)))
      return ERROR;
    if (!_dxfColorRecurseToIndividual(subo, color, opacity, component, setcolor,
				  setopacity, colorfield, opacityfield,
				  colorvec, opacityval, immediate))
      return ERROR;
    break;
  case CLASS_CLIPPED:
    if (!(DXGetClippedInfo((Clipped)o, &subo, NULL)))
      return ERROR;
    if (!_dxfColorRecurseToIndividual(subo, color, opacity, component, setcolor,
				  setopacity, colorfield, opacityfield,
				  colorvec, opacityval, immediate))
      return ERROR;
    break;
  case CLASS_XFORM:
    if (!(DXGetXformInfo((Xform)o, &subo, NULL)))
      return ERROR;
    if (!_dxfColorRecurseToIndividual(subo, color, opacity, component, setcolor,
				  setopacity, colorfield, opacityfield, 
				  colorvec, opacityval, immediate))
      return ERROR;
    break;
    
	  
  default:
    DXWarning("not a group or field or screen or xform or clipped");
    return OK;
  }	
  return OK;
}

   
static Error ColorObject(Object o, Object color, 
			  Object opacity, char *component, 
			  int setcolor, int setopacity, int colorfield, 
			  int opacityfield, RGBColor colorvec,
			  float opacityval, int immediate)
{
  int i;
  Object subo, subcolor, subopacity;
  struct arg a;


  switch(DXGetObjectClass(o)) {
  case CLASS_GROUP:
    /* get children and call ColorObject(subo); */
    for (i=0; (subo=DXGetEnumeratedMember((Group)o, i, NULL)); i++){
      if ((colorfield)&&(ISGENERICGROUP(color))) {
	subcolor = DXGetEnumeratedMember((Group)color, i, NULL);
        if (!subcolor) {
	  DXSetError(ERROR_BAD_PARAMETER,
		   "color map does not match object hierachy");
	  return ERROR;
        }
      }
      else 
	subcolor = color;
      
      if ((opacityfield)&&(ISGENERICGROUP(opacity))) {
	subopacity = DXGetEnumeratedMember((Group)opacity, i, NULL);
        if (!subopacity) {
	  DXSetError(ERROR_BAD_PARAMETER,
		   "opacity map does not match object hierachy");
	  return ERROR;
        }
      }
      else 
	subopacity = opacity;
      
      if (!ColorObject(subo,subcolor, subopacity, component, setcolor,
		       setopacity, colorfield, opacityfield, colorvec,
		       opacityval, immediate))
	return ERROR;
    }
    break;
  case CLASS_FIELD:
    /* set arguments */
    if ((colorfield)&& (ISGENERICGROUP(color))) {
      DXSetError(ERROR_BAD_PARAMETER, "#11841", "color map");
      return ERROR;
    }
    if ((opacityfield)&& (ISGENERICGROUP(opacity))) {
      DXSetError(ERROR_BAD_PARAMETER, "#11841", "opacity map");
      return ERROR;
    }
    
    /* if component is "colors" then remove any front or back colors
       components which may be hanging around */
    if (setcolor) { 
      if (!(strcmp(component,"colors"))) {
        if (DXGetComponentValue((Field)o,"front colors"))
  	  DXDeleteComponent((Field)o, "front colors");
        if (DXGetComponentValue((Field)o,"back colors"))
	  DXDeleteComponent((Field)o, "back colors");
      }
    }
    /* if user specifically asked for opacity = 1.0 (not defaulting)
       then remove any opacity component */
    
    if (opacityval == 1.0) {
      if (DXGetComponentValue((Field)o,"opacities"))
	DXDeleteComponent((Field)o, "opacities");
      setopacity = 0;
    }
    

    a.f = (Field)o;
    a.color = color;
    a.opacity = opacity;
    strcpy(a.component, component);
    a.setcolor = setcolor;
    a.setopacity = setopacity;       
    a.colorfield = colorfield;
    a.opacityfield = opacityfield;       
    a.colorvec = colorvec;
    a.opacityval = opacityval;
    a.immediate = immediate;



    /* and schedule autocolorfield to be called */
    if (!(DXAddTask(ColorField, (Pointer)&a, sizeof(a), 1.0))) {
      return ERROR;
    }
    break;
      

  case CLASS_SCREEN:
    if (!(DXGetScreenInfo((Screen)o, &subo, NULL, NULL)))
      return ERROR;
    if (!ColorObject(subo, color, opacity, component, setcolor,
		     setopacity, colorfield, opacityfield, colorvec,
		     opacityval, immediate))
      return ERROR;
    break;
  case CLASS_CLIPPED:
    if (!(DXGetClippedInfo((Clipped)o, &subo, NULL)))
      return ERROR;
    if (!ColorObject(subo, color, opacity, component, setcolor,
		     setopacity, colorfield, opacityfield, colorvec,
		     opacityval, immediate))
      return ERROR;
    break;
  case CLASS_XFORM:
    if (!(DXGetXformInfo((Xform)o, &subo, NULL)))
      return ERROR;
    if (!ColorObject(subo, color, opacity, component, setcolor,
		     setopacity, colorfield, opacityfield, colorvec,
		     opacityval, immediate))
      return ERROR;
    break;
    
	  
  default:
    DXWarning("not a group or field or screen or xform or clipped");
    return OK;
  }	
 
  return OK;
}



static Error ColorField(Pointer ptr)
{
  Field f, savedfield;
  Object color, opacity, copycolor=NULL, copyopacity=NULL;
  Interpolator i;
  char *component, *el_type, *att;
  int setcolor, setopacity, colorfield, opacityfield, count;
  int scalar, mapcounts, j, k;
  int immediate, floatflag, textflag; 
  int floatcolors, floatopacities;
  float *dp_m, *dp_md, minmap, maxmap;
  RGBColor *colortable=NULL;
  float *opacitytable=NULL;
  float fraction;
  float red, green, blue;
  int numentries;
  Array corigin=NULL, oorigin=NULL;
  Array a_d, a_p, a_o, a_c, olddata, newdata, dataarray;
  Array mapposarray, mapdataarray;
  Array deferredcolormap=NULL, deferredopacitymap=NULL;
  struct arg *a;
  RGBColor c;
  float o;


  /* unpack arg */
  a = (struct arg *)ptr;
  
  f = a->f;
  color = a->color;
  opacity = a->opacity;
  component = a->component;
  setcolor = a->setcolor;
  setopacity = a->setopacity;
  colorfield = a->colorfield;
  opacityfield = a->opacityfield;
  c = a->colorvec;
  o = a->opacityval;
  if (!colorfield)
    corigin = (Array)DXNewConstantArray(1, &c,TYPE_FLOAT, CATEGORY_REAL, 1, 3);
  if (!opacityfield)
     oorigin = (Array)DXNewConstantArray(1, &o,TYPE_FLOAT,CATEGORY_REAL, 0, 
                                         NULL);
  newdata = NULL;
  savedfield = NULL;
  immediate = a->immediate;


  if (DXEmptyField((Field)f)) {
    DXDelete((Object)corigin);
    DXDelete((Object)oorigin);
    return OK;
  }
  
  if (!immediate) 
    if (!_dxfDelayedOK((Object)f))
      goto error;
  
  /*    if it's a volume, then we NEED to set opacities.....*/
  /*    Determine if it's volume or surface data   */
  el_type  = NULL;
  if ((el_type= DXGetString((String)DXGetComponentAttribute((Field)f,
							    "connections", 
							    "element type"))) 
      != NULL) {
    if ( !strcmp(el_type,"triangles") || !strcmp(el_type,"quads") ||
	 !strcmp(el_type,"faces") || !strcmp(el_type,"lines") ) 
    {
      /* surface = 1; */
    }
    else if ( !strcmp(el_type,"cubes") || !strcmp(el_type,"tetrahedra") )
    {
      /* surface = 0; */
    }
    else {
      DXSetError(ERROR_UNEXPECTED,"unrecognized connections");
      goto error;
    } 
  } 
  else  {
    /* surface = 1; */
  }
  
  /*   now check what the data (if any) is dependent on. If there is
       a data component, then will make the colors dep on whatever 
       the data is dep on. If there is no data component, then we'll
       make the colors dep on positions. Unless there are faces, in
       which case we ought to make the colors dep on faces. Special
       case. */
  
  /*   first, if there is no data component */
  if (!(a_d = (Array)DXGetComponentValue(f,"data"))) {
    if (!(a_p = (Array)DXGetComponentValue(f,"positions"))) {
      DXSetError(ERROR_BAD_PARAMETER, "no positions or data");
      goto error;
    }
    else {
      /* if colors exist, follow the color dependency attribute */
      a_c = (Array)DXGetComponentValue(f,"colors"); 
      a_o = (Array)DXGetComponentValue(f,"opacities"); 
      if (a_c) {
	if (!(att = 
	      DXGetString((String)DXGetComponentAttribute(f,"colors",
							  "dep")))) {
	  DXSetError(ERROR_DATA_INVALID,"bad or missing color dep attribute");
          goto error;
	}
	if (!DXGetArrayInfo(a_c, &count, NULL, NULL, NULL, NULL))   {
	  DXSetError(ERROR_BAD_PARAMETER, "bad colors component");
          goto error;
	}
      }
      /* else if opacities exist, follow the opacity dependency attribute */
      else if (a_o) {
	if (!(att = 
	      DXGetString((String)DXGetComponentAttribute(f,"opacities",
							  "dep")))) {
	  DXSetError(ERROR_DATA_INVALID,
		     "bad or missing opacity dep attribute");
          goto error;
	}
	if (!DXGetArrayInfo(a_o, &count, NULL, NULL, NULL, NULL))   {
	  DXSetError(ERROR_BAD_PARAMETER, "bad opacities component");
          goto error;
	}
      }
      /* if neither data, colors, nor opacites exist, 
         make the colors dep on positions */
      else {
	att = "positions";
	if (!DXGetArrayInfo(a_p, &count, NULL, NULL, NULL, NULL))   {
	  DXSetError(ERROR_BAD_PARAMETER, "bad positions component");
          goto error;
	}
      }
    }
  }
  /* there is a data component */
  else  {
    if (!(DXGetArrayInfo((Array)a_d, &count, NULL, NULL, NULL, NULL))) {
      DXSetError(ERROR_BAD_PARAMETER, "bad data component");
      goto error;
    }
    if (!(att = DXGetString((String)DXGetComponentAttribute(f,"data",
							    "dep")))) {
      DXSetError(ERROR_BAD_PARAMETER,"missing data dependent attribute");
      goto error;
    }
  }
  
  
  /*    if we need to change the colors.... */
  if (setcolor) {
    /* if it's a color map, let DXMap do the work*/
    if (!(_dxfScalarField(f,&scalar)))
      goto error;
    if (!(_dxfFloatField((Object)f,&floatflag)))
      goto error;
    if (!(_dxfIsTextField(f,&textflag)))
      goto error;
    if (((!scalar)||(!floatflag))&&(!textflag)) {
      if (immediate) {
	/* copy the field and make a new data component for map to work on */
	savedfield = (Field)DXCopy((Object)f,COPY_STRUCTURE);
	olddata = (Array)DXGetComponentValue((Field)f, "data");
	if (!(newdata = DXScalarConvert(olddata)))
	  goto error;
	if (!(DXSetComponentValue((Field)f, "data", (Object)newdata)))
	  goto error;
      }
    }
    
    if (immediate) { 
      if (color) {
	/* if the map has non-float data, then convert to float */
        if (!(_dxfFloatField((Object)color, &floatcolors)))
	  goto error;
        if ((!floatcolors)&&(color)) {
          /* copy the field and make a new data component */
          copycolor = (Object)DXCopy((Object)color, COPY_STRUCTURE);
          if (!ConvertColors((Object)copycolor))
	    goto error;
        }
	
        if (!floatcolors) {
	  if (! DXMapCheck((Object)f, (Object)copycolor, 
			   "data",  NULL, NULL, NULL, NULL))
	    goto error;
	  
	  i = DXNewInterpolator((Object)copycolor, INTERP_INIT_DELAY, 0.0);
	  if (! i)
	    goto error;
	  
	  f = (Field)DXMap((Object)f, (Object)i, "data", component);
	  if (DXGetError() != ERROR_NONE && !f) {
	    goto error;
	  }
	  else if (! f) {
	    goto error;
	  }
	  DXDelete((Object)i);
        }
        else {
	  if (! DXMapCheck((Object)f, color, "data",  NULL, NULL, NULL, NULL))
	    goto error;
	  
	  i = DXNewInterpolator((Object)color, INTERP_INIT_DELAY, 0.0);
	  if (! i)
	    goto error;
	  
	  f = (Field)DXMap((Object)f, (Object)i, "data", component);
	  if (DXGetError() != ERROR_NONE && !f) {
	    goto error;
	  }
	  else if (! f) {
	    goto error;
	  }
	  DXDelete((Object)i);
        }
      }
      else {
	/* follow the chosen component */
	f = (Field)DXMap((Object)f, (Object)corigin, att, component);
      }
      DXDeleteComponent((Field)f, "color map");
      if (setopacity) DXDeleteComponent((Field)f, "opacity map");
      if (!(DXEndField(f))) {
	goto error;
      }
      if (!(DXChangedComponentValues(f,component)))
	goto error;
    }
    /* else it is NOT immediate application of the colormap */
    else {  
      if (color) {
	if (!(mapposarray =
	      (Array)DXGetComponentValue((Field)color,"positions"))) {
	  DXSetError(ERROR_BAD_PARAMETER,"bad map positions");
          goto error;
	}
	if (!(dp_m = (float *)DXGetArrayData(mapposarray))) {
          goto error;
	}
	if (!(mapdataarray =
	      (Array)DXGetComponentValue((Field)color,"data"))) {
	  DXSetError(ERROR_BAD_PARAMETER,"bad map data");
          goto error;
	}
	if (!(dp_md = (float *)DXGetArrayData(mapdataarray))) {
          goto error;
	}
	if (!(DXGetArrayInfo((Array)mapposarray,&mapcounts,
			     NULL,NULL,NULL,NULL))) {
          goto error;
	}
	
	/* check out the map */
	if (dp_m[mapcounts-1] < dp_m[0]) {
	  DXSetError(ERROR_BAD_PARAMETER,
		     "color map max must be greater than or equal to min");
          goto error;
	}
        
	for (j=0; j<mapcounts-1; j++) {
	  if ((dp_m[j] > dp_m[j+1])) {
	    DXSetError(ERROR_BAD_PARAMETER,
		       "non-monotonic color map");
	    goto error;
	  }
	}

        minmap=0;
        maxmap=255;
	numentries=maxmap-minmap+1;
	colortable = DXAllocateLocal(numentries*sizeof(RGBColor));
	if (!colortable) {
	  DXAddMessage("cannot allocate delayed color table with %d entries",
		       numentries);
	  goto error;
	}
        if (dp_m[0] == dp_m[mapcounts-1]) {
           for (k=0; k<=255; k++) {
             if (k<dp_m[0] || k > dp_m[mapcounts-1]) {
                colortable[k]=DXRGB(0.0, 0.0, 0.0);
             }
             else { 
              colortable[k]=DXRGB(dp_md[0],
                                  dp_md[1], 
                                  dp_md[2]);
             }
          }  
        }
        else {
	for (k=minmap; k<=maxmap; k++) {
          if (k<dp_m[0] || k > dp_m[mapcounts-1]) {
            colortable[k]=DXRGB(0.0, 0.0, 0.0);
          }
          else {
            /* now find the last dp_m[] which matches k */
            j=0;
	    while (j<mapcounts && (dp_m[j]<=k)) {
              j++;
            }
            if (dp_m[j-1] == k)
              colortable[k]=DXRGB(dp_md[3*(j-1)],
                                  dp_md[3*(j-1)+1], 
                                  dp_md[3*(j-1)+2]);
            else {
  	      fraction = (k-dp_m[j-1])/(dp_m[j]-dp_m[j-1]);
	      red  = fraction*(dp_md[3*j]-dp_md[3*(j-1)])+dp_md[3*(j-1)];
	      green= fraction*
	        (dp_md[3*j+1]-dp_md[3*(j-1)+1])+dp_md[3*(j-1)+1];
	      blue = fraction*
	        (dp_md[3*j+2]-dp_md[3*(j-1)+2])+dp_md[3*(j-1)+2];
	      colortable[k] = DXRGB(red, green, blue);
            }
	  }
	}
       }
      }
      else {
        /* color map is a single color */
        if (!DXStatistics((Object)f,"data",&minmap,&maxmap,NULL,NULL)) {
          DXAddMessage("delayed option requires a data component");
	  goto error;
        }
        /* for now, make this 0:256 */
        minmap=0;
        maxmap=255;
	numentries=maxmap-minmap+1;
	colortable = DXAllocateLocal(numentries*sizeof(RGBColor));
	if (!colortable) {
	  DXAddMessage("cannot allocate delayed color table with %d entries",
		       numentries);
	  goto error;
	}
	for (k=minmap; k<=maxmap; k++) {
	  colortable[k] = c;
	}
      } 
      
      if (!(deferredcolormap = DXNewArray(TYPE_FLOAT,
					  CATEGORY_REAL, 1, 3))) 
	goto error;
      if (!(DXAddArrayData(deferredcolormap, 0, numentries, 
			   (Pointer)colortable))) {
	goto error;
      }
      if (!(DXSetComponentValue((Field)f,"color map",  
				(Object)deferredcolormap))) {
	goto error;
      }
      deferredcolormap=NULL;
      if (!(dataarray=(Array)DXGetComponentValue((Field)f,"data"))) {
	DXSetError(ERROR_BAD_PARAMETER,
		   "delayed=1 requires data component");
	goto error;
      }
      /* need to delete the old colors component first.  This is because
       * it may have an incorrect attribute which will screw up (believe
       * me it will) both the new colors component AND the data component, since
       * they are now the same */
      if (!(DXSetComponentValue((Field)f, component, NULL)))
         goto error;
      if (!(DXSetComponentValue((Field)f,component, (Object)dataarray))) 
	goto error;
      
      if (!(DXEndField(f))) {
	goto error;
      }
    }
  }
  /*    if we need to change the opacities.... */
  if (setopacity) {
    if (!(_dxfScalarField(f,&scalar)))
      goto error;
    if (!(_dxfFloatField((Object)f,&floatflag)))
      goto error;
    if (!(_dxfIsTextField(f,&textflag)))
      goto error;
    if (((!scalar)||(!floatflag))&&(!textflag)) {
      if (immediate) {
	/* copy the field and make a new data component for map to work on */
	/* but only if we haven't already done this */
	if (!savedfield) {
	  savedfield = (Field)DXCopy((Object)f,COPY_STRUCTURE);
	  olddata = (Array)DXGetComponentValue((Field)f, "data");
	  if (!(newdata = DXScalarConvert(olddata)))
            goto error;
	  if (!(DXSetComponentValue((Field)f, "data", (Object)newdata)))
	    goto error;
	}
      }
    }

    if (immediate) { 
      if (opacity) {
        /* if the map has non-float data, then convert to float */
        if (!(_dxfFloatField((Object)opacity, &floatopacities)))
	  goto error;
        if ((!floatopacities)&&(opacity)) {
          /* copy the field and make a new data component */
          copyopacity = (Object)DXCopy((Object)opacity, COPY_STRUCTURE);
          if (!ConvertOpacities(copyopacity))
	    goto error;
        }
        if (!floatopacities) {
	  if (! DXMapCheck((Object)f, (Object)copyopacity, 
			   "data",  NULL, NULL, NULL, NULL))
	    goto error;
	  
	  i = DXNewInterpolator((Object)copyopacity, INTERP_INIT_DELAY, 0.0);
	  if (! i)
	    goto error;
	  
	  f = (Field)DXMap((Object)f, (Object)i, "data", "opacities");
	  if (DXGetError() != ERROR_NONE && !f) {
	    goto error;
	  }
	  else if (! f) {
	    goto error;
	  }
	  DXDelete((Object)i);
        }
        else {
	  if (! DXMapCheck((Object)f, opacity, 
			   "data",  NULL, NULL, NULL, NULL))
	    goto error;
	  
	  i = DXNewInterpolator((Object)opacity, INTERP_INIT_DELAY, 0.0);
	  if (! i)
	    goto error;
	  
	  f = (Field)DXMap((Object)f, (Object)i, "data", "opacities");
	  if (DXGetError() != ERROR_NONE && !f) {
	    goto error;
	  }
	  else if (! f) {
	    goto error;
	  }
	  DXDelete((Object)i);
        }
      }
      else {
	/* follow the chosen component */
	f = (Field)DXMap((Object)f, (Object)oorigin, att, "opacities");
      }
      if (!(DXEndField(f))) 
	goto error;
      if (!(DXChangedComponentValues(f,"opacities"))) {
	goto error;
      }
    }
    /* else it's delayed data */
    else {
      if (opacity) {
	if (!(mapposarray =
	      (Array)DXGetComponentValue((Field)opacity,"positions"))) {
	  DXSetError(ERROR_BAD_PARAMETER,"bad map positions");
          goto error;
	}
	if (!(dp_m = (float *)DXGetArrayData(mapposarray))) {
          goto error;
	}
	if (!(mapdataarray =
	      (Array)DXGetComponentValue((Field)opacity,"data"))) {
	  DXSetError(ERROR_BAD_PARAMETER,"bad map data");
          goto error;
	}
	if (!(dp_md = (float *)DXGetArrayData(mapdataarray))) {
          goto error;
	}
	if (!(DXGetArrayInfo((Array)mapposarray,&mapcounts,
			     NULL,NULL,NULL,NULL))) {
          goto error;
	}
	
	/* check out the map */
	if (dp_m[mapcounts-1] <= dp_m[0]) {
	  DXSetError(ERROR_BAD_PARAMETER,
		     "opacity map max must be greater than min");
          goto error;
	}
	for (j=0; j<mapcounts-1; j++) {
	  if ((dp_m[j] > dp_m[j+1])) {
	    DXSetError(ERROR_BAD_PARAMETER,
		       "non-monotonic opacity map");
            goto error;
          }
	}
        minmap = dp_m[0];
        maxmap = dp_m[mapcounts-1];
        /* for now, make this 0:256 */
        minmap = 0;
        maxmap = 255;
        numentries=maxmap-minmap+1;
        opacitytable = DXAllocateLocal(numentries*sizeof(float));
        if (!opacitytable) {
          DXAddMessage("cannot allocate delayed opacity table with %d entries",
                       numentries);
          goto error;
        }
	
	for (k=minmap; k<=maxmap; k++) { 
	  if (k<dp_m[0] || k > dp_m[mapcounts-1]) {
            opacitytable[k]=0.0;
          }
          else {
            /* now find the last dp_m[] which matches k */
            j=0;
            while (j<mapcounts && (dp_m[j]<=k)) {
              j++;
            }
            if (dp_m[j-1] == k)
              opacitytable[k]=dp_md[j-1];
            else {
              fraction = (k-dp_m[j-1])/(dp_m[j]-dp_m[j-1]);
	      opacitytable[k] = fraction*(dp_md[j]-dp_md[j-1]) + dp_md[j-1];
            }
	  } 
	}
      }
      else {
        /* opacity map is a single color */
        if (!DXStatistics((Object)f,"data",&minmap,&maxmap, NULL, NULL)) {
          DXAddMessage("delayed option requires a data component");
	  goto error;
        }
        /* for now, make this 0 to 255 */
        minmap = 0;
        maxmap = 255;
	numentries=maxmap-minmap+1;
	opacitytable = DXAllocateLocal(numentries*sizeof(float));
	if (!opacitytable) {
	  DXAddMessage("cannot allocate delayed opacity table with %d entries",
		       numentries);
	  goto error;
	}
	for (k=minmap; k<=maxmap; k++) {
	  opacitytable[k] = o;
	}
      }
      

      
      if (!(deferredopacitymap = 
	    DXNewArray(TYPE_FLOAT,CATEGORY_REAL,0,NULL))) {
	goto error;
      }
      if (!(DXAddArrayData(deferredopacitymap, 0, numentries,
			   (Pointer)opacitytable))) {
	goto error;
      }
      if (!(DXSetComponentValue((Field)f,"opacity map",
				(Object)deferredopacitymap))) {
	goto error;
      }
      deferredopacitymap=NULL;
      if (!(dataarray=(Array)DXGetComponentValue((Field)f, "data"))) {
	DXSetError(ERROR_BAD_PARAMETER,
		   "delayed=1 requires data component");
	goto error;
      }
      if (!(DXSetComponentValue((Field)f,"opacities",
				(Object)dataarray))) {
	goto error;
      }
      if (immediate) DXDeleteComponent((Field)f, "color map");
      if ((immediate)&&(setopacity)) 
	DXDeleteComponent((Field)f, "opacity map");
      if (!(DXEndField(f))) {
	goto error;
      }
    }
  }
  
  if (savedfield) {
    /* we need to get the data from savedfield and put it in f, then
     * delete savedfield */
    if (!DXReplace((Object)f, (Object)savedfield, "data", "data")) {
      DXSetError(ERROR_INTERNAL,"can't replace data in field");
      goto error;
    }
  }
  
  DXDelete((Object)corigin);
  DXDelete((Object)oorigin);
  DXDelete((Object)deferredcolormap);
  DXDelete((Object)deferredopacitymap);
  DXFree((Pointer)colortable);
  DXFree((Pointer)opacitytable);
  DXDelete((Object)savedfield);
  DXDelete((Object)copycolor);
  DXDelete((Object)copyopacity);
  return OK;
  
 error:
  DXDelete((Object)corigin);
  DXDelete((Object)oorigin);
  DXFree((Pointer)colortable);
  DXFree((Pointer)opacitytable);
  DXDelete((Object)deferredcolormap);
  DXDelete((Object)deferredopacitymap);
  DXDelete((Object)savedfield);
  DXDelete((Object)copycolor);
  DXDelete((Object)copyopacity);
  return ERROR;
  
}



#if 0
static Error ColorIndividualMGrid(Object o, Object color,
			      Object opacity, char *component, 
			      int setcolor, int setopacity, int colorfield, 
			      int opacityfield, RGBColor colorvec, 
			      float opacityval, int immediate)
{
  /* this is for multigrids, which might be mixed volumes and surfaces */

}
#endif




static Error ColorIndividual(Object o, Object color, 
			      Object opacity, char *component, 
			      int setcolor, int setopacity, int colorfield, 
			      int opacityfield, RGBColor colorvec, 
			      float opacityval, int immediate)

{

  int surface;
  float thickness, opacitymult=0, intensitymult=0;

  
  if (!_dxfIsVolume((Object)o, &surface))
    return ERROR;
  if (!surface) {
    if (!_dxfBoundingBoxDiagonal((Object)o, &thickness))
      return ERROR;
    if (opacityfield) {
      opacitymult = (float)(-LN(0.5)/thickness);
      intensitymult = 2*opacitymult; 
    }
    else {
      if (setopacity) {
	if (opacityval >= 0.99) {
	  opacitymult = (float)(-LN(0.01)/thickness);
	  intensitymult = opacitymult;
	}
	else if (opacityval == 0.0) {
	  opacitymult = 0.0;
	  intensitymult = 1.0/thickness;
	}
	else {
	  opacitymult =(float)(-LN(1.0-opacityval)/
			       (thickness*opacityval));
	  intensitymult = opacitymult;
	}
      }
      else  {
	opacitymult = (float)(-LN(0.5)/thickness);
	intensitymult = opacitymult/0.5;
      }
    }
  }
  if (!ColorObject(o,color, opacity, component, setcolor,
		   setopacity, colorfield, opacityfield, colorvec,
		   opacityval, immediate))
    return ERROR;
  
  if (!surface) {
    if (!(_dxfSetMultipliers(o, intensitymult, opacitymult))) {
       return ERROR;
    }
  }
  return OK;
}

static Error ConvertColors(Object copycolor)
{
   Array oldcolors=NULL, newcolors=NULL;
   Object subo;
   int i;

   switch (DXGetObjectClass(copycolor)) {
   case (CLASS_FIELD):
          oldcolors = (Array)DXGetComponentValue((Field)copycolor, "data");  
          if (!(newcolors = DXArrayConvert(oldcolors, TYPE_FLOAT,
                                           CATEGORY_REAL, 1, 3)))
	    goto error;
          if (!(DXSetComponentValue((Field)copycolor,
                                    "data", (Object)newcolors)))
	    goto error; 
          break;
   case (CLASS_GROUP):
          for (i=0; (subo=
	  	DXGetEnumeratedMember((Group)copycolor,i, NULL)); i++) {
              if (!ConvertColors(subo))
               goto error;
          }
          break;
   default:
          DXSetError(ERROR_DATA_INVALID,"colormap must be a field or group");
          goto error;
   }
  error:
     DXDelete((Object)newcolors);
     return ERROR;
}

static Error ConvertOpacities(Object copyopacity)
{
   Array oldopacities=NULL, newopacities=NULL;
   Object subo;
   int i;

   switch (DXGetObjectClass(copyopacity)) {
   case (CLASS_FIELD):
          oldopacities = (Array)DXGetComponentValue((Field)copyopacity, "data");
          if (!(newopacities = DXArrayConvert(oldopacities, TYPE_FLOAT,
					      CATEGORY_REAL, 0, NULL)))
	    goto error;
          if (!(DXSetComponentValue((Field)copyopacity,
                                    "data", (Object)newopacities)))
	    goto error;
          break;
   case (CLASS_GROUP):
          for (i=0; (subo=
	  	DXGetEnumeratedMember((Group)copyopacity,i, NULL)); i++) {
              if (!ConvertOpacities(subo))
               goto error;
          }
          break;
   default:
          DXSetError(ERROR_DATA_INVALID,"opacitymap must be a field or group");
          goto error;
   }
  return OK;
  error:
     DXDelete((Object)newopacities);
     return ERROR;
}
