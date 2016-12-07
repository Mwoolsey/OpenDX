/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/*
 */

#include <dxconfig.h>


/* issues: partitioned grids for base, handling 2D and 3D */
/* should do not only data but all components dep positions */
/* what about data dep connections? need to do a set type after */
/* the work is done */

#include <stdio.h>
#include <math.h>
#if defined(HAVE_STRING_H)
#include <string.h>
#endif

#include <dx/dx.h>
#include "_glyph.h"
#include "_connectgrids.h"


Error
m_Regrid(Object *in, Object *out)
{
  char *string;
  Class class;
  Category category;
  Object ino=NULL, base=NULL;
  Array missing=NULL;
  float radius,  *radius_ptr, exponent;
  Type type;
  int numitems, rank, shape[30], numnearest;

  radius_ptr = &radius;
  
  if (!in[0]) {
    DXSetError(ERROR_BAD_PARAMETER, "#10000","input");
    return ERROR;
  }
  
  class = DXGetObjectClass(in[0]);
  if ((class != CLASS_ARRAY)&&
      (class != CLASS_FIELD)&&
      (class != CLASS_GROUP)&&
      (class != CLASS_XFORM)) {
    DXSetError(ERROR_BAD_PARAMETER, "#10630", "input");
    return ERROR;
  }
  
  
  if (!in[1]) {
     DXSetError(ERROR_BAD_PARAMETER,"#10000", "grid");
     goto error;
  }
    
  /* now get numnearest, radius, missing, exponent */

  /* numnearest */
  if (!in[2]) {
     numnearest = 1;
  }
  else {
     if (!DXExtractInteger(in[2], &numnearest)) {
        if (!DXExtractString(in[2], &string)) {
           DXSetError(ERROR_BAD_PARAMETER, "#10370",
                      "nearest", "an integer or the string `infinity`");
           goto error;
        }
        /* XXX lower case */
        if (strcmp(string,"infinity")) {
           DXSetError(ERROR_BAD_PARAMETER, "#10370",
                      "nearest", "an integer or the string `infinity`");
           goto error;
        }
        numnearest = -1;
     }
     else {
        if (numnearest <= 0) {
           DXSetError(ERROR_BAD_PARAMETER,"#10020","nearest");
           goto error; 
        }
     }
  }
 

  /* radius */
  if (!in[3]) {
     *radius_ptr = -1;
  }
  else {
     if (!DXExtractFloat(in[3], radius_ptr)) {
        if (!DXExtractString(in[3], &string)) {
           DXSetError(ERROR_BAD_PARAMETER, "#10370",
                "radius", "a positive scalar value or the string `infinity`");
           goto error;
        }
        /* XXX lower case */
        if (strcmp(string,"infinity")) {
           DXSetError(ERROR_BAD_PARAMETER, "#10370",
                "radius", "a positive scalar value or the string `infinity`");
           goto error;
        }
        *radius_ptr = -1;
     }
     else {
        if (*radius_ptr < 0) {
           DXSetError(ERROR_BAD_PARAMETER, "#10370",
                "radius", "a positive scalar value or the string `infinity`");
           goto error; 
        }
        else if (*radius_ptr == 0){
           DXWarning("Regrid radius set to 0, data values assigned to nearest grid point");
           numnearest=0;
        }
     }
  }



  /* now get exponent */
  if (!in[4]) {
     exponent = 1;
  }
  else {
     if (!DXExtractFloat(in[4], &exponent)) {
        DXSetError(ERROR_BAD_PARAMETER,"#10080",
                   "exponent");
        goto error;
     }
  }

  /* get missing */
  if (in[5]) {
      if (!(DXGetObjectClass(in[5])==CLASS_ARRAY)) {
	DXSetError(ERROR_BAD_PARAMETER, "#10261",
		   "missing");
	goto error;
      }
      missing = (Array)in[5];
  }

/*Moved common data prep steps outside of if/else statement --JAB*/
    class = DXGetObjectClass(in[0]);
    if (class == CLASS_ARRAY) {
      if (!DXGetArrayInfo((Array)in[0], &numitems, &type, &category, 
			  &rank, shape))
        goto error;
      if ((type != TYPE_FLOAT)||(rank != 1)) {
        /* should also handle scalar I guess XXX */
        DXSetError(ERROR_BAD_PARAMETER,"#10630","input");
        goto error;
      }
      if (shape[0]<1 || shape[0]> 3) {
        DXSetError(ERROR_BAD_PARAMETER,"#10370",
                   "input","1-, 2-, or 3-dimensional");
        goto error;
      }
      /* just a list of positions; let's make it a field with positions*/
      ino = (Object)DXNewField();
      if (!ino)
        goto error;
      if (!DXSetComponentValue((Field)ino,"positions",in[0]))
        goto error; 
    }
    else {
      /* so I can safely delete it at the bottom, as well as modify it */
      ino = DXCopy(in[0], COPY_STRUCTURE);
    }

    /* remove connections */
    if (DXExists(ino, "connections"))
     DXRemove(ino,"connections");
   
    /* cull */
    ino = DXCull(ino);


    /* copy the field with positions and connections; we'll add 
     * data using the given input 0 */
    base = DXApplyTransform(in[1],NULL);

    /* copy the attributes of the input scattered points to the output grid*/
    if (!DXCopyAttributes(base, ino)) 
      goto error; 

    if (!DXCreateTaskGroup())
      goto error;

    /*Issue warning when base grid with invalid positions is used*/
    if (DXExists(base, "invalid positions") && missing)
      DXWarning("Regrid base grid contains invalid positions which are not removed when missing is set");

/* if nearest is infinity, we use the "radius" method. 
   if radius is 0, we assign data value to nearest grid point,
   In all other cases we use the "nearest" method" */

  if (numnearest == -1) {
    /* use the "radius" method */

    if (!_dxfConnectRadiusObject((Object)ino, (Object)base, 
				 radius, exponent, missing))
      goto error;
  }
  else if (numnearest == 0) {
    /*assign value to nearest grid point - JAB*/
  
    /* remove invalid postions if missing does not exist*/ 
    if (DXExists(base, "invalid positions") && !missing)
     DXRemove(base,"invalid positions");
   
    if (!_dxfConnectScatterObject((Object)ino, (Object)base, missing))
      goto error;
  } 
  else {
    /* use the "nearest" method */
    /* if radius was infinity... */
    if (*radius_ptr==-1)
       radius_ptr = NULL;
    if (!_dxfConnectNearestObject(ino, base, numnearest, radius_ptr, 
			          exponent, missing))
      goto error;

  }

  if (!DXExecuteTaskGroup())
    goto error;

  DXDelete((Object)ino);
  out[0] = base;
  
  return OK;
  
 error:
  DXDelete((Object)ino);
  DXDelete((Object)base);
  return ERROR;
  
}
