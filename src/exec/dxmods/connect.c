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


#include <stdio.h>
#include <math.h>
#include <string.h>
#include <ctype.h>
#include <dx/dx.h>
#include "_connectvor.h"

	
int	
  m_Connect(Object *in, Object *out)
{	
  char *method;
  char newstring[30];
  Class class;
  Category category;
  Object obj=NULL;
  Vector normal;
  Array newarray=NULL;
  float *newarrayptr; 
  double vec_len;
  Type type;
  int numitems, rank, shape[30], count, i;
  
  if (!in[0]) {
    DXSetError(ERROR_BAD_PARAMETER, "#10000", "input");
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
      
  if (in[1]) {
    if (!DXExtractString(in[1],&method)) {
      DXSetError(ERROR_BAD_PARAMETER,"#10200", "method");
      return ERROR;
    }
  }
  else {
    method = "triangulation";
  }
  
  /* convert to lower case */
  count = 0;
  i = 0;
  while (i<29 && method[i] != '\0') {
    if (isalpha(method[i])) { 
      if (isupper(method[i]))
	newstring[count]=tolower(method[i]);
      else
	newstring[count]=method[i];
      count++;
    }
    else if (isdigit(method[i])) { 
      newstring[count]=method[i];
      count++;
    }
    i++;
  }
  newstring[count]='\0';
  method = newstring;
  
  if (strcmp(method,"triangulation")) {
    DXSetError(ERROR_BAD_PARAMETER,"#10370","method","triangulation");
    goto error;
  }
  
  /* get the normal */
  if (!in[2]) {
    /* may want a better default */
    normal = DXVec(0.0, 0.0, 1.0);
  }
  else {
    if (!DXExtractParameter((Object)in[2], TYPE_FLOAT, 3, 1, 
			    (Pointer)&normal)) {
      DXSetError(ERROR_BAD_PARAMETER, "#10230", "normal", 3);
      goto error; 
    }
  }
  vec_len = DXLength(normal);
  if (vec_len == 0.0) {
     DXSetError(ERROR_BAD_PARAMETER,
                "normal parameter must be non-zero length"); 
     goto error;
  }
  /* normalize the normal */
  normal = DXNormalize(normal);

  class = DXGetObjectClass(in[0]);
  if (class == CLASS_ARRAY) {
    if (!DXGetArrayInfo((Array)in[0], &numitems, &type, &category, 
			&rank, shape)) {
      goto error;
    } 
    /* if it's not float or not rank 1, need to allocate space for
       a new array */
    if ((type != TYPE_FLOAT)||(rank != 1)) {
      if ((rank != 0)&&(rank != 1)) {
        DXSetError(ERROR_DATA_INVALID,"rank of input must be 0 or 1");
        goto error;
      }
      if (rank==0) {
         rank = 1;
         shape[0] = 1;
      }
      newarray = DXNewArray(TYPE_FLOAT,CATEGORY_REAL, 1, shape[0]);
      if (!newarray) 
        goto error;
      if (!DXAddArrayData(newarray, 0, numitems, NULL))
        goto error;
      newarrayptr = (float *)DXGetArrayData(newarray);
      if (!DXExtractParameter(in[0], TYPE_FLOAT, shape[0], numitems, 
                              (Pointer)newarrayptr))  {
        DXSetError(ERROR_BAD_PARAMETER,"input must be a field or vector list");
        goto error;
      }
    }
    if (shape[0]<1 || shape[0]> 3) {
      DXSetError(ERROR_BAD_PARAMETER,"#10370",
                 "input","1-, 2-, or 3-dimensional");
      goto error;
    }
    /* just a list of positions; let's make it a field with positions*/
    obj = (Object)DXNewField();
    if (!obj)
      goto error;
    if (!newarray) {
      if (!DXSetComponentValue((Field)obj,"positions",in[0]))
        goto error; 
    }
    else {
      if (!DXSetComponentValue((Field)obj,"positions",(Object)newarray))
        goto error; 
      newarray=NULL;
    }
  }
  else {
    /* copy the positions; we'll add connections */
    obj = DXCopy(in[0], COPY_STRUCTURE);
  }
  if (!_dxfConnectVoronoiObject((Object)obj, normal)) {
    goto error;
  }
  out[0] = obj;
  
  return OK;
  
 error:
  DXDelete((Object)obj);
  DXDelete((Object)newarray);
  return ERROR;
  
}

