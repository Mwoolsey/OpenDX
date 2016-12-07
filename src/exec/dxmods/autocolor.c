/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/*
 * Header: /usr/people/gresh/code/svs/src/dxmods/RCS/autocolor.m,v 5.3 93/01/25 14:50:45 gresh Exp Locker: gresh 
 */

#include <dxconfig.h>


/***
MODULE:
    AutoColor
SHORTDESCRIPTION:
    Adds color and opacity information to data, to prepare it for rendering.
CATEGORY:
 Data Transformation
INPUTS:
   data;   scalar field;  NULL; field to color 
   opacity;   scalar;     data dependent;            opacity
   intensity; scalar;     data dependent;            color intensity 
   start;     scalar;     0.6666;          starting color (default blue)
   range;     scalar;     0.6666;          range of color (default blue -> red)
   saturation; scalar;    1.00;          saturation
   min;       scalar or scalar field; data dependent; minimum of data to map 
   max;       scalar or scalar field; data dependent; maximum of data to map 
   delayed;   flag; 0; delay application of color and opacity maps 
OUTPUTS:
   mapped;     color field;        NULL; color mapped input field 
   colormap; field;       NULL; rgb color map used
FLAGS:
BUGS:
AUTHOR:
    dl gresh
END:
***/

#include <string.h>
#include <ctype.h>
#include <dx/dx.h>
#include "_autocolor.h"

static int _dxfIsFieldorGroup(Object ob);

int
m_AutoColor(Object *in, Object *out)
{
    Group g_in;
    Array onearray=NULL;
    float opacity, intensity, phase, range, saturation;
    float  inputmin,   inputmax, r, g, b;
    float *inputminp=(&inputmin), *inputmaxp=(&inputmax);
    int    used_object, delayed,  i, count, one=1;
    Object colormap;
    char *colorstr, newstring[21];
    RGBColor colorvecmin, colorvecmax;
    RGBColor cv[2];



/* at this point it is assumed that the input group g_in is a composite
   field (i.e. that all of the subfields are of a uniform type). */

    if(!(g_in = (Group)in[0])) {
	out[0] = NULL;
        /* missing first parameter */
	DXSetError(ERROR_MISSING_DATA, "#10000", "data");
	return ERROR;
    }

    if (!(_dxfFieldWithInformation((Object)in[0]))) {
	out[0] = in[0];
        out[1] = (Object)DXNewField();
	return OK;
    }

    if (!in[1]) 
	opacity = -1.0;
    else if (!DXExtractFloat((Object)in[1],&opacity)) {
        /* bad opacity input; must be a float between 0 and 1 */
	DXSetError(ERROR_BAD_PARAMETER,"#10110", "opacity", 0, 1);
	return ERROR;
    }
    else if ((opacity < 0.0)||(opacity > 1.0)) {
        /* bad opacity input; must be a float between 0 and 1 */
        DXSetError(ERROR_BAD_PARAMETER,"#10110", "opacity", 0, 1);
        return ERROR;
    }
    if (!in[2]) 
	intensity = -1.0;
    else if (!DXExtractFloat((Object)in[2],&intensity)) {
        /* bad intensity input; must be a positive float */
	DXSetError(ERROR_BAD_PARAMETER,"#10100", "intensity");
	return ERROR;
    }
    else if (intensity < 0.0) {
        /* bad intensity input; must be a positive float */
	DXSetError(ERROR_BAD_PARAMETER,"#10100", "intensity");
        return ERROR;
    }
    if (!in[3]) 
	phase = 0.6666;
    else if (!DXExtractFloat((Object)in[3],&phase)) {
        /* bad phase, must be a float */
	DXSetError(ERROR_BAD_PARAMETER,"#10080", "start");
	return ERROR;
    }
    if (!in[4]) 
	range =  0.6666;
    else if (!DXExtractFloat((Object)in[4],&range)) {
        /* bad range, must be a float */
	DXSetError(ERROR_BAD_PARAMETER,"#10080","range");
	return ERROR;
    }
    range = -range;
    if (!in[5]) 
	saturation = 1.0;
    else if (!DXExtractFloat((Object)in[5],&saturation)) {
        /* bad saturation, must be a float between 0 and 1 */
	DXSetError(ERROR_BAD_PARAMETER,"#10110","saturation", 0, 1);
	return ERROR;
    }
    else if ((saturation < 0.0)||(saturation > 1.0)) {
        /* bad saturation input; must be a float between 0 and 1 */
        DXSetError(ERROR_BAD_PARAMETER,"#10110", "saturation", 0, 1);
        return ERROR;
    }

    /* if in[6] is an object, and in[7] is null, then we want to 
       make inputmax be the max of the object, rather than the normal
       default */

    used_object = 0;


    if (!in[6]) 
	inputminp = NULL;
    else if (!DXExtractFloat((Object)in[6],inputminp)) {
	if (_dxfIsFieldorGroup((Object)in[6])) {
	    /* get max and min */
	    if (!DXStatistics((Object)in[6],"data",inputminp,inputmaxp,
			    NULL,NULL)) 
	    {	
                /* min must be a field or a scalar value */ 
		DXAddMessage("min parameter");
		return ERROR;
	    }
	    else
		used_object++;
	}
	else {
            /* min must be a field or a scalar value */ 
	    DXSetError(ERROR_BAD_PARAMETER,"#10520","min");
	    return ERROR;
	}
    }

    if (!in[7]) {
	if (!used_object) 
	    inputmaxp = NULL;
/*         implicit else, max was set above  */
       }
    else if (!DXExtractFloat((Object)in[7],inputmaxp)) {
	if (_dxfIsFieldorGroup((Object)in[7])) {
	    if (!DXStatistics((Object)in[7],"data",NULL,inputmaxp,NULL,
			    NULL)) {
                /* max must be a field or a scalar value */ 
		DXAddMessage("max parameter");
		return ERROR;
	    }
	}
	else {
            /* max must be a field or a scalar value */ 
	    DXSetError(ERROR_BAD_PARAMETER,"#10520", "max");
	    return ERROR;
	}
    }
  
    if (!in[8])
	delayed = 0;
    else {
        if (!(DXExtractInteger(in[8],&delayed))) {
	    DXSetError(ERROR_BAD_PARAMETER, "#10070", "delayed");
            return ERROR;
        }
        if ((delayed != 0)&&(delayed != 1)) {
	    DXSetError(ERROR_BAD_PARAMETER, "#10070", "delayed");
            return ERROR;
        }
    } 
    
    if (!in[9]) {
      colorvecmin = DXRGB(0.3, 0.3, 0.3);
      colorvecmax = DXRGB(0.3, 0.3, 0.3);
    }
    /* first try a string */
    else if (DXExtractNthString(in[9], 0, &colorstr)) {
      /* is it a valid color name? */
      if (!(DXColorNameToRGB(colorstr, &colorvecmin))) {
	/* if it's not a valid colorname, is it "min" or "max"? */
        count = 0;
        i=0;
        while (i<20 && colorstr[i] != '\0') {
	  for (i=0; i< 20; i++) {
	    if (isalpha(colorstr[i])) {
	      newstring[count]=tolower(colorstr[i]);
	      count++;
	    }
	    else {
	      if (colorstr[i] != ' ') {
		newstring[count] = colorstr[i];
		count++;
	      }
	    } 
	  }
        }
        if (!strcmp(newstring, "colorofmin")) {
          DXResetError();
          if (intensity != -1)
	    _dxfHSVtoRGB(phase, saturation, intensity, &r, &g, &b);
          else
	    _dxfHSVtoRGB(phase, saturation, 1.0, &r, &g, &b);
	  colorvecmin = DXRGB(r,g,b);
	}
	else if (!strcmp(newstring,"colorofmax")) {
          DXResetError();
          if (intensity != -1)
	    _dxfHSVtoRGB(phase+range, saturation, intensity, &r, &g, &b);
          else
	    _dxfHSVtoRGB(phase+range, saturation, 1.0, &r, &g, &b);
	  colorvecmin = DXRGB(r,g,b);
	}
	else goto error;
      }
      /* start by setting max = min */
      colorvecmax=colorvecmin;
      /* now look and see if there's a second string hanging around */
      if (DXExtractNthString(in[9], 1, &colorstr)) {
	/* is it a valid color name? */
	if (!(DXColorNameToRGB(colorstr, &colorvecmax))) {
	  /* if it's not a valid colorname, is it "min" or "max"? */
          count = 0;
          i=0;
	  while (i<20 && colorstr[i] != '\0') {
	    for (i=0; i< 20; i++) {
	      if (isalpha(colorstr[i])) {
		newstring[count]=tolower(colorstr[i]);
		count++;
	      }
	      else {
		if (colorstr[i] != ' ') {
		  newstring[count] = colorstr[i];
		  count++;
		}
	      } 
	    }
	  }
	  if (!strcmp(newstring, "colorofmin")) {
            DXResetError();
            if (intensity != -1)
	      _dxfHSVtoRGB(phase, saturation, intensity, &r, &g, &b);
            else
	      _dxfHSVtoRGB(phase, saturation, 1.0, &r, &g, &b);
	    colorvecmax = DXRGB(r,g,b);
	  }
	  else if (!strcmp(newstring,"colorofmax")) {
            DXResetError();
            if (intensity != -1) 
	      _dxfHSVtoRGB(phase+range, saturation, intensity, &r, &g, &b);
            else
	      _dxfHSVtoRGB(phase+range, saturation, 1.0, &r, &g, &b);
	    colorvecmax = DXRGB(r,g,b);
	  }
	  else goto error;
	}
      }
    }
    else if ((DXExtractParameter(in[9], TYPE_FLOAT, 3, 1, 
                                 (Pointer)&colorvecmin))) {
	colorvecmax = colorvecmin;
    }
    else if ((DXExtractParameter(in[9], TYPE_FLOAT, 3, 2, (Pointer)&cv))) { 
	colorvecmin = cv[0];
	colorvecmax = cv[1];
    }
    else {
      DXSetError(ERROR_BAD_PARAMETER,
                 "outofrange must be a color string or pair of color strings, \"color of min\" or \"color of max\", or a 3-vector or pair of 3 vectors");
         goto error;
    }



    out[0] = (Object)_dxfAutoColor((Object)g_in, opacity, 
	    intensity, phase, range, saturation, inputminp, inputmaxp,
	    &colormap, delayed, colorvecmin, colorvecmax);
    if (out[0]) out[1] = (Object)colormap;

    /* for delayed colors, add the direct color map attribute */
    if (delayed && out[0]) {
       onearray = DXNewArray(TYPE_INT,CATEGORY_REAL, 0);
       if (!DXAddArrayData(onearray, 0, 1, &one))
           goto error;
       if (!DXSetAttribute(out[0], "direct color map", (Object) onearray))
           goto error;
       onearray = NULL;
    }



    if ((!out[0])||(!(out[1]))) 
       return ERROR;
    else
       return OK;
error:
    DXDelete((Object)onearray);
    return ERROR;
}



static int _dxfIsFieldorGroup(Object ob)
{
    Class cl;


    if ( ((cl = DXGetObjectClass(ob)) == CLASS_FIELD) || 
	  (cl == CLASS_GROUP) )
	return OK;
    else
	return ERROR;
}


