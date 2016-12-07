/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>

#include <ctype.h>
#include <dx/dx.h>
#include <math.h>
#include <string.h>
#include "eigen.h"
#include "_glyph.h"
#include "_autocolor.h"
#define TRUE                    1
#define FALSE                   0
#define DIMENSION3D             3
#define DIMENSION2D             2


typedef struct Point2D
{
   float x; 
   float y;
} Point2D;

typedef struct arg 
{
      Object outobject;
      char type[30];
      Object typefield;
      int type_is_field;
      float shape;
      float scale;
      int scale_set;
      float ratio;
      int ratio_set;
      float given_min;
      int min_set;
      float given_max;
      int max_set;
      float quality;
      float base_size;
      int AutoSize;
      Object parent;
      int parentindex;
      char font[100];
} Arg;
     
typedef struct textarg 
{
      Object inobject;
      Object outobject;
      int colored;
      float scale;
      char font[100];
} Textarg;
     

int
m_AutoGlyph(Object *in, Object *out)
{
  Object ino=NULL, outo=NULL, incopy=NULL;
  Class class, classglyph=CLASS_ARRAY;
  char type_string[100], *type;
  float quality, shape, scale, ratio=0, given_min=0, given_max=0;
  int ratio_set, min_set, max_set, scale_set, AutoSize, count;
  Object typefield, glyphcopy=NULL;
  int type_is_field, i, stats_done=0;
  struct textarg textarg;
  int index=0;
  float base_size;
  Array data;
  struct arg arg;
  Type datatype;
  char font[100];

  strcpy(font,"fixed");

 
  out[0] = NULL;


  
  if (in[0]) {
    class = DXGetObjectClass((Object)in[0]);
    if (class != CLASS_FIELD && class != CLASS_GROUP && class != CLASS_XFORM
        && class != CLASS_CLIPPED) {
      DXSetError(ERROR_BAD_PARAMETER, "#10190", "data");
      goto error;
    }
  } else { 
    DXSetError(ERROR_MISSING_DATA, "#10000", "data");
    goto error;
  }
  ino = in[0];
  
  
  if (!(_dxfFieldWithInformation((Object)in[0]))) {
    out[0] = in[0];
    return OK;
  }
  
  strcpy(type_string,"standard");

  /* type */
  quality = -1;
  type_is_field = 0; 
  typefield = NULL;
  if (in[1]) {
    if (!DXExtractString(in[1], &type)) {
      if (!DXExtractFloat(in[1],&quality)) { 
        classglyph = DXGetObjectClass(in[1]);
        if ((classglyph != CLASS_FIELD)&&(classglyph != CLASS_XFORM)&&
            (classglyph != CLASS_GROUP)) {
          /* type must be a field, a scalar value between 0 and 1 or a 
             string */
	  DXSetError(ERROR_BAD_PARAMETER,"#10202","type", 0.0, 1.0); 
	  goto error;
        }
        glyphcopy = DXApplyTransform(in[1],NULL);
        typefield = (Object)glyphcopy;
        classglyph = DXGetObjectClass(glyphcopy);
        type_is_field = 1; 
      }
      else {
        if ((quality < 0)||(quality > 1.0)) {
  	  DXSetError(ERROR_BAD_PARAMETER, "#10202", "type", 0.0, 1.0);
	  goto error;
        }
      }
    }
    if ((quality==-1) &&(!type_is_field)) {
      strcpy(type_string,type);
      /* turn type string to lower case */
      i = 0;	
      count = 0;	
      while (i < 29 && type_string[i] != '\0') {
	if (isalpha(type_string[i])){
	  if (isupper(type_string[i]))
	    type_string[count]= tolower(type_string[i]);
	  else
	    type_string[count]= type_string[i];
	  count++;
	}
	else {
	  if (type_string[i] != ' ') {
	    type_string[count]= type_string[i];
	    count++;
	  }
	}
	i++;
      } 
      type_string[count]='\0';
    }
  } else {
    strcpy(type_string,"standard");
  }

  /* scale */
  if(in[3]) {
    if (!DXExtractFloat(in[3], &scale)) {
      DXSetError(ERROR_BAD_PARAMETER, "#10090", "scale");
      goto error;
    }
    if (scale <= 0.0) {
      DXSetError(ERROR_BAD_PARAMETER, "#10090", "scale");
      goto error;
    }
    scale_set = 1;
    AutoSize = 1;
  } else {
    scale = 1.0;
    scale_set = 0;
    AutoSize = 1;
  }


  /* the index type of glyph.  */
   if (classglyph == CLASS_GROUP) {
      /* check the group type */
      if ((DXGetGroupClass((Group)in[1]) != CLASS_GROUP)&&
          (DXGetGroupClass((Group)in[1]) != CLASS_SERIES)) {
        DXSetError(ERROR_DATA_INVALID,
                   "for annotation glyphs, type parameter must be a series or generic group");
        goto error;
      }
      /* instead of calling copy, just call applytransform */
      /* out[0] = DXCopy(in[0], COPY_STRUCTURE); */
      out[0] = DXApplyTransform(in[0], NULL); 
      if (!out[0]) goto error;
      if (!_dxfIndexGlyphObject(out[0], glyphcopy, scale))
         goto error;
      DXDelete((Object)glyphcopy);
      return OK;
   }


  
  /* shape */
  if (in[2]) { 
    if (!DXExtractFloat(in[2], &shape)) {
      DXSetError(ERROR_BAD_PARAMETER, "#10090", "shape");
      goto error;
    }
    if (shape <= 0.0) {
      DXSetError(ERROR_BAD_PARAMETER, "#10090", "shape");
      goto error;
    }
  } else {
    shape = 1;
  }
  
  
  /* ratio */
  if (in[4]) { 
    if (!DXExtractFloat(in[4], &ratio)) {
      DXSetError(ERROR_BAD_PARAMETER, "#10100", "ratio");
      goto error;
    }
    if (ratio < 0.0) {
      DXSetError(ERROR_BAD_PARAMETER, "#10100", "ratio");
      goto error;
    }
    ratio_set = 1;
  } else {
    ratio_set = 0;
  }
  
  min_set = 0; 
  max_set = 0; 
  /* given_min */
  if (in[5]) { 
    if (!DXExtractFloat(in[5], &given_min)) {
      if (_dxfIsFieldorGroup((Object)in[5])) {
        if ((!strcmp(type_string,"tensor"))||(_dxfIsTensor(in[5]))) {
          if (!_dxfGetTensorStats((Object)in[5], &given_min, &given_max)) {
            goto error;
          }
        }
        else {
          if (!DXStatistics((Object)in[5],"data",&given_min,&given_max,
                            NULL,NULL)) {
            goto error;
          }
          max_set = 1;
        }
      }
      else {
        DXSetError(ERROR_BAD_PARAMETER,"#10520","min");
        goto error;
      }
    }
    min_set = 1;
  }

  /* given_max */
  if (in[6]) {
    if (!DXExtractFloat(in[6], &given_max)) {
      if (_dxfIsFieldorGroup((Object)in[6])) {
        if ((!strcmp(type_string,"tensor"))||(_dxfIsTensor(in[6]))) {
          if (!_dxfGetTensorStats((Object)in[6], NULL, &given_max)) {
            goto error;
          }
        }
        else {
          if (!DXStatistics((Object)in[6],"data",NULL,&given_max,
                            NULL,NULL)) {
            goto error;
          }
        }
      }
      else {
        DXSetError(ERROR_BAD_PARAMETER,"#10520","max");
        goto error;
      }
    }
    max_set = 1;
  }

  if (!_dxfGetFontName(type_string, font)) 
     goto error;


  DXCreateTaskGroup();

  /* call ApplyTransform on the input. We'll delete it later. */
  incopy = DXApplyTransform(ino, NULL);


  /* text glyphs are special, because we have to return a group. Thus if
   * the user specifically specifies that it's text, or if it's a field
   * and the data is of type string, we have to turn fields into groups */

  if (DXGetObjectClass(incopy) == CLASS_FIELD) {
     data = (Array)DXGetComponentValue((Field)incopy,"data");
     if (!data &&((!strcmp(type_string,"text"))||
                  (!strcmp(type_string,"coloredtext")))) {
        DXSetError(ERROR_DATA_INVALID,
                   "text glyphs require a data component");
        goto error;
     }
     if (data) {
       if (!DXGetArrayInfo((Array)data, NULL, &datatype, NULL, NULL, NULL))
           goto error;
     }
     else
       datatype=TYPE_FLOAT;
     if ((datatype == TYPE_STRING) ||
         (!strcmp(type_string,"text")) || 
         (!strcmp(type_string,"coloredtext"))) {
         outo = (Object)DXNewGroup();
         if (!outo) goto error; 
         /* Call text processing stuff giving outo, incopy, and params */
         textarg.inobject = incopy;
         textarg.outobject = outo;
         if (!strcmp(type_string,"coloredtext"))
            textarg.colored = 1;
         else
            textarg.colored = 0;
         textarg.scale = scale;
         strcpy(textarg.font,font); 
         if (!_dxfTextField(&textarg)) {
            /* incopy was deleted in TextField */
            incopy = NULL;
            goto error;
         }
     }
     /* else it's a field, but not text data */
     else {
       outo = incopy;
       incopy = NULL;
       arg.outobject = outo;
       strcpy(arg.type,type_string);
       arg.shape = shape;
       arg.scale = scale;
       arg.scale_set = scale_set;
       arg.ratio = ratio;
       arg.ratio_set = ratio_set;
       arg.quality = quality;
       if (!_dxfGetStatisticsAndSize(outo, type_string, min_set, 
  		                     &given_min,  max_set, &given_max,
		                     &base_size, &stats_done))
          goto error;
       arg.given_min = given_min;
       arg.min_set = min_set;
       arg.given_max = given_max;
       arg.max_set = max_set;
       arg.base_size = base_size;
       arg.AutoSize = AutoSize;
       arg.typefield = typefield;
       arg.type_is_field = type_is_field;
       arg.parent=NULL;
       arg.parentindex = -1;
       strcpy(arg.font,font);
       if (!(DXAddTask(_dxfGlyphTask, (Pointer)&arg, sizeof(arg), 1.0)))
             goto error;
     }
  }
  /* else it's a group */
  else {
     /* Call generic processing stuff on outo, giving params */
     outo = incopy;
     incopy = NULL;
     if (!_dxfProcessGlyphObject(NULL, outo, type_string,  shape,  scale, 
                                 scale_set, ratio, ratio_set, given_min, 
                                 min_set, given_max, max_set, quality, 
                                 AutoSize, typefield, type_is_field, index, 
                                 font, &stats_done, &base_size))
        goto error;
  }

  
  if (!DXExecuteTaskGroup()) 
    goto error;


  /* apply fuzz to the object */
  out[0] = DXSetFloatAttribute(outo, "fuzz", 4);
  if (!out[0])
    goto error;

  DXDelete((Object)glyphcopy);
  return OK;
  
 error:
  out[0]=NULL;
  DXDelete((Object)incopy); 
  DXDelete((Object)outo); 
  DXDelete((Object)glyphcopy);
  return ERROR;
}
