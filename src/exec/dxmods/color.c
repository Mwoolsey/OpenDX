/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/*
 * Header: /usr/people/gresh/code/svs/src/dxmods/RCS/color.m,v 5.2 93/02/09 09:18:31 gresh Exp Locker: gresh 
 *
 */

#include <dxconfig.h>
#include "_autocolor.h"

/***
MODULE:
Color   
SHORTDESCRIPTION:
 Colors a field 
CATEGORY:
 Data Transformation
INPUTS:
 input;      field;                           NULL;              field to color
 color;      field or vector value or string; NULL;              DXRGB color
 opacity;    field or scalar;                 data dependent;    opacity 
 component;  string;                          "colors";      component to color
 delayed;    flag;                            0;             delay applying map
OUTPUTS:
 colored;    color field;    		      NULL;    color mapped input field
FLAGS:
BUGS:
AUTHOR:
 dl gresh   
END:
***/

#include <math.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <dx/dx.h>
#include "_autocolor.h"
#include "color.h"

extern Error DXColorNameToRGB (char *, RGBColor *); /* from libdx/color.c */

Error m_Color(Object *in, Object *out)
{
    Object  color, opacity;
    Array   onearray=NULL;
    RGBColor colorvec;
    float  opacityval, minopacity, maxopacity;
    float  minmap, maxmap, mindata, maxdata;
    char   component[30], *tmpcomponent, *colorstr;
    char   newstring[30];
    int    setcolor, setopacity, colorfield, opacityfield, i, count, byteflag;
    int    immediate=0, delayed, one=1;
    Group  g_out;
    
    
    color = NULL;
    g_out = NULL;
    opacity = NULL;
    setcolor = 0;
    setopacity = 0;
    opacityfield = 0;
    colorfield = 0;
    colorvec.r = 0;
    colorvec.g = 0;
    colorvec.b = 0;
    opacityval = 0;
    
    
    out[0] = in[0];  
    
    /*  check for required input */
    if (!in[0]) {
	/* input must be specified */
	DXSetError(ERROR_MISSING_DATA,"#10000", "input");
	goto error;
    }
    
    if (! _dxfFieldWithInformation(in[0]))
	return OK; 
    if (! _dxfByteField((Object)in[0], &byteflag))
	goto error;
    
    
    
    /*  see whether we need to call DXMap on the "color" component */
    setcolor = 1;
    if (!in[1])	
	setcolor = 0;
    
    /*  what have we got for the color map? First see if it's a string */
    else if (DXExtractString((Object)in[1], &colorstr)) {
	/*  convert from string to DXRGB */
	if (!DXColorNameToRGB(colorstr, &colorvec))
	    goto error;
	colorfield = 0;
    }
    
    
    /*      if it's not a string, is it a vector color? */
    else if (DXExtractParameter((Object)in[1],
				TYPE_FLOAT,3,1,(Pointer)&colorvec)) {
	colorfield = 0;
    }

    /* else is it the special kind of group map put out when
       you import a .cm file */
    else if (!(_dxfIsImportedColorMap(in[1], &colorfield, &color, NULL))) {
      if (_dxfIsColorMap(in[1])) {
	colorfield = 1;
	color = in[1];
	/* check once to see if data is within the map */
	/* only check if the data is not byte */
	if (!byteflag) {
	    if (!DXStatistics((Object)in[1], "positions", &minmap,
			      &maxmap, NULL, NULL)) {
		goto error;
	    } 
	    if (!DXStatistics((Object)in[0], "data", &mindata,
			      &maxdata, NULL, NULL)) {
		goto error;
	    } 
	    if ((minmap > mindata)||(maxmap < maxdata)) {
		DXWarning("color map does not fully encompass data");
	    }
	}
      }
      else
	goto error;
    } 
  
  
    /*  now see if we have to map opacity */
    setopacity = 0;
    opacityfield = 0;
    if (in[2]) { 
	/*      is it a simple float number? */
	if (DXExtractFloat((Object)in[2], &opacityval)) {
	    opacityfield = 0;
	    if ((opacityval < 0.0)||(opacityval > 1.0)) {
		/* opacity must be between 0 and 1 */
		DXSetError(ERROR_BAD_PARAMETER,"#10110","opacity", 0, 1);
		goto error;
	    }
	    setopacity = 1;
	}
        /* else is it the special kind of group map put out when
           you import a .cm file */
        else if (!_dxfIsImportedColorMap(in[2], &opacityfield, 
                                         NULL, &opacity)) {
	/*      or is it an opacity map?  */ 
	 if (_dxfIsOpacityMap(in[2])){
	    if (!(DXStatistics((Object)in[2],"data",
			       &minopacity,&maxopacity,NULL,NULL)))
		goto error;
	    if (minopacity < 0.0) {
		/* opacity must be between 0 and 1 */
		DXSetError(ERROR_BAD_PARAMETER,"#10110","opacity", 0, 1);
		goto error;
	    }
	    
	    if (minopacity==maxopacity) {
		if (minopacity == 1.0) {
		    setopacity = 0;
		    opacityfield = 0;
		}
		else {
		    setopacity = 1;
		    opacityfield = 0;
		    opacityval = minopacity;
		}
	    }
	    else {
		opacityfield = 1;
		opacity = (Object)in[2];
		setopacity = 1;
		/* check once to see if data is within the map */
		if (!byteflag) {
		    if (!DXStatistics((Object)in[2], "positions", &minmap,
				      &maxmap, NULL, NULL)) {
			goto error;
		    } 
		    if (!DXStatistics((Object)in[0], "data", &mindata,
				      &maxdata, NULL, NULL)) {
			goto error;
		    } 
		    if ((minmap > mindata)||(maxmap < maxdata)) {
			DXWarning("opacity map does not fully encompass data");
		    }
		}
	    }
	}
	else 
	    goto error; 
        }
    }
  
    /*  now see what component to operate on */
    if (!in[3]) {
	strcpy(component,"colors");
    }
    else {
	if (DXExtractString((Object)in[3], &tmpcomponent)) {
	    count = 0;
	    i = 0;
	    while ( i<29 && tmpcomponent[i] != '\0') {
		if (isalpha(tmpcomponent[i])){
		    if (isupper(tmpcomponent[i]))
			newstring[count]= tolower(tmpcomponent[i]);
		    else
			newstring[count]= tmpcomponent[i];
		    count++;
		}
		i++; 
	    }	
	    newstring[count]='\0';
	    tmpcomponent = newstring;
	    
	    if (strcmp(tmpcomponent,"colors") &&
		strcmp(tmpcomponent,"frontcolors") &&
		strcmp(tmpcomponent,"backcolors")) {
		/* bad component name */
		DXSetError(ERROR_BAD_PARAMETER, "#10210", 
			   tmpcomponent, "component");
		goto error;	
	    }
	    if (!strcmp(tmpcomponent,"frontcolors")) 
		strcpy(component,"front colors");
	    if (!strcmp(tmpcomponent,"backcolors")) 
		strcpy(component,"back colors");
	    if (!strcmp(tmpcomponent,"colors")) 
		strcpy(component,"colors");
	}
	else {
	    /* component must be a string */
	    DXSetError(ERROR_BAD_PARAMETER,"#10200","component");
	    goto error;
	}
    }
    
    if (!(in[4])) {
	delayed = 0;
    }
    else {
	if (!DXExtractInteger(in[4],&delayed)) {
	    DXSetError(ERROR_BAD_PARAMETER, "#10070", "delayed");	 
	    goto error;
	}
	if ((delayed != 0) && (delayed != 1)) { 
	    DXSetError(ERROR_BAD_PARAMETER, "#10070", "delayed");	 
	    goto error;
	}
    }
    if (delayed == 0) immediate = 1;
    if (delayed == 1) immediate = 0;
    
    /*    now we are ready to operate on the structure */
    if ((setcolor)||(setopacity)) {
	
	if (!(g_out = (Group)DXCopy(in[0], COPY_STRUCTURE))) {
	    /* input must be a group or a field */
	    DXAddMessage("#10190","input");
	    goto error;
	}
	
	if (!DXCreateTaskGroup())
	    goto error;

	if (! _dxfColorRecurseToIndividual((Object)g_out, color, opacity, 
					   component, setcolor, setopacity,
					   colorfield, opacityfield, 
					   colorvec, opacityval, immediate))
	    goto error;

	if (!DXExecuteTaskGroup())
	    goto error;

	out[0] = (Object)g_out;
    }

    /* for delayed colors, add the direct color map attribute */
    if (delayed) {
       onearray = DXNewArray(TYPE_INT,CATEGORY_REAL, 0);
       if (!DXAddArrayData(onearray, 0, 1, &one))
           goto error; 
       if (!DXSetAttribute(out[0], "direct color map", (Object) onearray))
           goto error;
       onearray = NULL;
    }

    return OK;
    
  error:
    DXDelete((Object)g_out);
    DXDelete((Object)onearray);
    out[0]= NULL;
    return ERROR;
}


Error _dxfIsColorMap(Object ob)
{
    Class cl;
    Object dp, pp;
    float *pos;
    Field part;
    int count;
    
    if (!(cl = DXGetObjectClass(ob))) {
	return ERROR;
    }
    if ( !(cl == CLASS_FIELD) && !(cl == CLASS_GROUP)) {
	DXSetError(ERROR_BAD_PARAMETER,"#10570","color");
	return ERROR;
    }
    
    if (cl == CLASS_FIELD) {
	/* should have data and positions */
	if ((!(dp=DXGetComponentValue((Field)ob,"data")))||
	    (!(pp=DXGetComponentValue((Field)ob,"positions")))) {
	    DXSetError(ERROR_BAD_PARAMETER, "#10250",
		       "color map", "data or positions");
	    return ERROR;
	}
	
	/* for a color map, should have shape 3 */
	if (!DXTypeCheck((Array)dp, TYPE_FLOAT, CATEGORY_REAL, 1, 3)) {
	    if (!DXQueryArrayConvert((Array)dp, TYPE_FLOAT, 
				     CATEGORY_REAL, 1, 3)) {
		DXSetError(ERROR_BAD_PARAMETER, "#10331","colormap data");
		return ERROR;
	    }
	}
	/* positions should be 1D */
	if (!((DXTypeCheck((Array)pp, TYPE_FLOAT, CATEGORY_REAL, 1, 1)) || 
	      (DXTypeCheck((Array)pp, TYPE_FLOAT, CATEGORY_REAL, 0)))) {
	    DXSetError(ERROR_BAD_PARAMETER,"#11130","color", "color");
	    return ERROR;
	}
	/* max should be greater than min */
	pos = (float *)DXGetArrayData((Array)pp);
	if (!DXGetArrayInfo((Array)dp,&count,NULL,NULL,NULL,NULL))
	    return ERROR;
	if (pos[count-1] < pos[0]) {
	    DXSetError(ERROR_BAD_PARAMETER, "#11220", 
		       "color map min", "color map max");
	    return ERROR;
	} 
	
	
    }
    else {
	int i;
	
	for (i = 0; NULL != (part = DXGetPart(ob, i)); i++)
	    if (! DXEmptyField(part))
		break;
	
	if (part == NULL) {
	    DXSetError(ERROR_BAD_PARAMETER, "#11819","color map");
	    return ERROR;
	}
	
	if ((!(dp=DXGetComponentValue((Field)part,"data")))||
	    (!(pp=DXGetComponentValue((Field)part,"positions")))) {
	    DXSetError(ERROR_BAD_PARAMETER, "#10250", 
		       "color map", "data or positions");
	    return ERROR;
	}
	
	/* for a color map, should have shape 3 */
	if (!DXTypeCheck((Array)dp, TYPE_FLOAT, CATEGORY_REAL, 1, 3)) {
	    DXSetError(ERROR_BAD_PARAMETER, "#10331",
		       "color map data component");
	    return ERROR;
	}
	/* positions should be 1D */
	if (!((DXTypeCheck((Array)pp, TYPE_FLOAT, CATEGORY_REAL, 1, 1)) || 
	      (DXTypeCheck((Array)pp, TYPE_FLOAT, CATEGORY_REAL, 0)))) {
	    DXSetError(ERROR_BAD_PARAMETER,"#11130","color","color");
	    return ERROR;
	}
    }
    
    return OK;
}

Error _dxfIsOpacityMap(Object ob)
{
    Class cl;
    Object dp, pp;
    Field part;
    float *pos;
    int count;
    
    if (!(cl = DXGetObjectClass(ob))) {
	return ERROR;
    }
    if (!(cl == CLASS_FIELD) && !(cl == CLASS_GROUP)) {
	DXSetError(ERROR_BAD_PARAMETER,"#10190","opacity map");
	return ERROR;
    }
    
    if (cl == CLASS_FIELD) {
	/* should have data and positions */
	if ((!(dp=DXGetComponentValue((Field)ob,"data")))||
	    (!(pp=DXGetComponentValue((Field)ob,"positions")))) {
	    DXSetError(ERROR_BAD_PARAMETER, "#10250", 
		       "opacity map", "data or positions");
	    return ERROR;
	}
	
	/* for an opacity map, should have shape 1 */
	if (!((DXTypeCheck((Array)dp, TYPE_FLOAT, CATEGORY_REAL, 1, 1)) || 
	      (DXTypeCheck((Array)dp, TYPE_FLOAT, CATEGORY_REAL, 0)))) 
	    if (!DXQueryArrayConvert((Array)dp, TYPE_FLOAT, 
				     CATEGORY_REAL, 1, 1)) {
		DXSetError(ERROR_BAD_PARAMETER,"#10332","opacity map");
		return ERROR;
	    }
	/* the positions should have shape 1 */
	if (!((DXTypeCheck((Array)pp, TYPE_FLOAT, CATEGORY_REAL, 1, 1)) || 
	      (DXTypeCheck((Array)pp, TYPE_FLOAT, CATEGORY_REAL, 0)))) {
	    DXSetError(ERROR_BAD_PARAMETER,"#11130","opacity","opacity");
	    return ERROR;
	}
	/* max should be greater than min */
	pos = (float *)DXGetArrayData((Array)pp);
	if (!DXGetArrayInfo((Array)dp,&count,NULL,NULL,NULL,NULL))
	    return ERROR;
	if (pos[count-1] < pos[0]) {
	    DXSetError(ERROR_BAD_PARAMETER, "#11220","opacity map min",
		       "opacity map max");
	    return ERROR;
	} 
    }
    else {
	int i;
	
	for (i = 0; NULL != (part = DXGetPart(ob, i)); i++)
	    if (! DXEmptyField(part))
		break;
	
	if (part == NULL) {
	    DXSetError(ERROR_BAD_PARAMETER,"#11819","opacity map");
	    return ERROR;
	}
	
	if ((!(dp=DXGetComponentValue((Field)part,"data")))||
	    (!(pp=DXGetComponentValue((Field)part,"positions")))) {
	    DXSetError(ERROR_BAD_PARAMETER, "#10250", 
		       "opacity map", "data or positions");
	    return ERROR;
	}
	
	/* for an opacity map, should have shape 1 */
	if (!((DXTypeCheck((Array)dp, TYPE_FLOAT, CATEGORY_REAL, 1, 1)) || 
	      (DXTypeCheck((Array)dp, TYPE_FLOAT, CATEGORY_REAL, 0)))) {
	    DXSetError(ERROR_BAD_PARAMETER, "#10332",
		       "opacity map data component");
	    return ERROR;
	}
	/* the positions should have shape 1 */
	if (!((DXTypeCheck((Array)pp, TYPE_FLOAT, CATEGORY_REAL, 1, 1)) || 
	      (DXTypeCheck((Array)pp, TYPE_FLOAT, CATEGORY_REAL, 0)))) {
	    DXSetError(ERROR_BAD_PARAMETER, "#11130","opacity","opacity");
	    return ERROR;
	}
    }
    
    return OK;
}


int
_dxfIsImportedColorMap(Object obj, int *isfield, Object *cmap, Object *omap)

{
   Class class;
   Object member1=NULL, member2=NULL;
   int returnval;

   class = DXGetObjectClass(obj);
   if (class != CLASS_GROUP)
      return 0;

   member1 = DXGetMember((Group)obj,"colormap");
   member2 = DXGetMember((Group)obj,"opacitymap");

   if (member1 && member2)
      returnval = 1;
   else
      returnval = 0;

   if (cmap) {
     if (member1) {
       if (isfield) *isfield = 1;
       *cmap = member1;
     }
   }
   if (omap) {
     if (member2) {
       if (isfield) *isfield = 1;
       *omap = member2;
     }
   }
   return returnval;
}   

