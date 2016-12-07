/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/*
 * $Header: /src/master/dx/src/exec/dxmods/shade.c,v 1.6 2006/01/03 17:02:25 davidt Exp $
 */

#include <dxconfig.h>

#if defined(HAVE_STRING_H)
#include <string.h>
#endif

#include <stdio.h>
#include <math.h>
#include <dx/dx.h>
#include "_normals.h"

static Error ShadeField(Field, float, int, float, float);
static Error DoNormals(Object, int, char *, int);
static Error ShadeIt(Object, int, char *, float, int, float, float, int);
static Error CheckNormalsDirection(Object, int);
static Error CheckNormalsDirectionField(Field, int);

Error
  m_Shade(Object *in, Object *out)
{
  int shade, shininess, flipfront;
  char *how;
  float specular, ambient, diffuse;
  Object outo=NULL;
  

  
  if (!in[0]) {
    DXSetError(ERROR_BAD_PARAMETER, "object must be specified");
    return ERROR;
  }
  
  
  /* shade param */ 
  if (!in[1]) {
    shade = 1;
  }
  else {
    if (!DXExtractInteger(in[1], &shade)) {
      DXSetError(ERROR_BAD_PARAMETER,"shade must be either 0 or 1");
      goto error;
    }
  }
  if ((shade < 0)||(shade > 1)) {
    DXSetError(ERROR_BAD_PARAMETER,"shade must be either 0 or 1");
    goto error;
  }
  
  
  /* how param */
  if (!in[2]) {
    how = "default";
  }
  else {
    if (!DXExtractString(in[2], &how)) {
      DXSetError(ERROR_BAD_PARAMETER,
		 "how must be either `faceted' or `smooth'");
      goto error;
    }
    /* XXX convert to lower case */
    if (strcmp(how,"faceted")&&(strcmp(how,"smooth"))) {
      DXSetError(ERROR_BAD_PARAMETER,
		 "how must be either `faceted' or `smooth'");
      goto error;
    }
  }
  if (in[2] && (shade==0)) {
    DXWarning("how is ignored because shade is equal to 0");
  }
  
  
  /* specular param */
  if (!in[3]) {
    specular = -1.0;
  }
  else {
    if (!DXExtractFloat(in[3], &specular)) {
      DXSetError(ERROR_BAD_PARAMETER,
		 "specular must be a non-negative scalar value");
      goto error;
    }
    if (specular < 0) {
      DXSetError(ERROR_BAD_PARAMETER,
		 "specular must be a non-negative scalar value");
      goto error;
    }
  }
  

  /* shininess */
  if (!in[4]) {
    shininess = -1;
  }
  else {
    if (!DXExtractInteger(in[4], &shininess)) {
      DXSetError(ERROR_BAD_PARAMETER,
		 "shininess must be a non-negative integer");
      goto error;
    }
    if (shininess < 0) {
      DXSetError(ERROR_BAD_PARAMETER,
		 "shininess must be a non-negative integer");
      goto error;
    }
  }


  /* diffuse param */
  if (!in[5]) {
    diffuse = -1.0;
  }
  else {
    if (!DXExtractFloat(in[5], &diffuse)) {
      DXSetError(ERROR_BAD_PARAMETER,
		 "diffuse must be a non-negative scalar value");
      goto error;
    }
    if (diffuse < 0) {
      DXSetError(ERROR_BAD_PARAMETER,
		 "diffuse must be a non-negative scalar value");
      goto error;
    }
  }
  
  /* ambient param */
  if (!in[6]) {
    ambient = -1.0;
  }
  else {
    if (!DXExtractFloat(in[6], &ambient)) {
      DXSetError(ERROR_BAD_PARAMETER,
		 "ambient must be a non-negative scalar value");
      goto error;
    }
    if (ambient < 0) {
      DXSetError(ERROR_BAD_PARAMETER,
		 "ambient must be a non-negative scalar value");
      goto error;
    }
  }

  /* whether or not to flip front and back */
  flipfront = 0;
  
  if (!(outo = DXCopy(in[0], COPY_STRUCTURE)))
    goto error;
  
  /* do the stuff with normals (must be done at the field level) */

  if (!ShadeIt(outo, shade, how, specular, shininess, diffuse, ambient,
               flipfront))
    goto error;
  
  
  out[0] = outo;
  return OK;
  
 error:
  DXDelete((Object)outo);
  return ERROR;
}


static Error ShadeIt(Object obj, int shade, char *how, float specular, 
                     int shininess, float diffuse, float ambient, 
                     int flipfront)
{
  Object child;
  int i;
  
  switch (DXGetObjectClass(obj)) {
  case (CLASS_FIELD):
    /* do the normals stuff here */
    if (!DoNormals((Object)obj, shade, how, flipfront))
      goto error;
    if (!ShadeField((Field)obj, specular, shininess,
		    diffuse, ambient))
      goto error;
    break;
  case (CLASS_GROUP):
    switch (DXGetGroupClass((Group)obj)) {
    case (CLASS_COMPOSITEFIELD):
      /* do the normals stuff here, then continue through the
	 composite field */
      if (!DoNormals((Object)obj, shade, how, flipfront))
	goto error;
      for (i=0; (child=DXGetEnumeratedMember((Group)obj,i,NULL));i++){
	if (!ShadeField((Field)child, specular, shininess, diffuse,
			ambient))
	  goto error; 
      }
      break;
    default:
      for (i=0; (child=DXGetEnumeratedMember((Group)obj,i,NULL)); i++) {
	if (!ShadeIt(child, shade, how, specular, shininess,
		     diffuse, ambient, flipfront))
	  goto error;
      }
      break;
    }
    break;
  case (CLASS_XFORM):
    if (!DXGetXformInfo((Xform)obj, &child, NULL))
      goto error;
    if (!ShadeIt(child, shade, how, specular, shininess, diffuse, 
		 ambient, flipfront))
      goto error;
    break;
  case (CLASS_CLIPPED):
    if (!DXGetClippedInfo((Clipped)obj, &child, NULL))
      goto error;
    if (!ShadeIt(child, shade, how, specular, shininess, diffuse,
		 ambient, flipfront))
      goto error;
    break;
  case (CLASS_SCREEN):
    if (!DXGetScreenInfo((Screen)obj, &child, NULL, NULL))
      goto error;
    if (!ShadeIt(child, shade, how, specular, shininess, diffuse,
		 ambient, flipfront))
      goto error;
    break;
  default:
    DXSetError(ERROR_DATA_INVALID,
	       "object must be a field, group, xform, or clipped object");
    goto error;
  }
  
  return OK;
  
 error:
  return ERROR;
  
}

static Error ShadeField(Field field, float specular,
                        int shininess, float diffuse, float ambient)
{
  /* set all the rendering attributes */
  if (specular != -1.0)
    if (!DXSetFloatAttribute((Object)field, "specular", specular))
      goto error;
  if (shininess != -1)
    if (!DXSetIntegerAttribute((Object)field, "shininess", shininess))
      goto error;
  if (diffuse != -1)
    if (!DXSetFloatAttribute((Object)field, "diffuse", diffuse))
      goto error;
  if (ambient != -1)
    if (!DXSetFloatAttribute((Object)field, "ambient", ambient))
      goto error;
  
  if (!DXEndField(field))
    goto error;
  return OK;
  
 error:
  return ERROR;
  
}

static Error DoNormals(Object obj, int shade, char *how, int flipfront)
{
  int i;
  char *attr;
  Field first, child;
  int shadeattribute;
  
  /* this is called at the Field or Composite field level */
  

  switch (DXGetObjectClass(obj)) {
  case CLASS_FIELD:
    

    /* if we don't want the object shaded... */
    /* regardless of what how is, make sure either there aren't any normals,
     * or if there are, that the "no shade" attribute is set */
    if (shade == 0) {
      /* if there are normals, we need to disable them */
      if (DXGetComponentValue((Field)obj, "normals")) {
        if (!DXSetIntegerAttribute((Object)obj, "shade", 0))
	  goto error;
      }
    }
    
    /* we want the object shaded */
    else if (shade==1) {
      /* if there's a shade=0 attribute set, set it to 1 */
      if (DXGetAttribute((Object)obj,"shade")) {
         if (!DXGetIntegerAttribute((Object)obj, "shade", &shadeattribute))
           goto error;
         if (shadeattribute==0) {
           /* set it to 1 */
           if (!DXSetIntegerAttribute((Object)obj, "shade", 1)) 
             goto error;
         }
         else if (shadeattribute != 1) {
           DXSetError(ERROR_DATA_INVALID,
                      "invalid shade attribute, must be 0 or 1");
           goto error;
         }
      }
      if (!strcmp(how, "default")) {
	/* only do something if there aren't any normals. otherwise
	 * do nothing */
	if (!DXGetComponentValue((Field)obj, "normals")) {
	  
          /* follow data if present */
          if (DXGetComponentValue((Field)obj,"data")) {
            attr = DXGetString((String)DXGetComponentAttribute((Field)obj,
                                                               "data","dep"));
            if (!attr) {
	      DXSetError(ERROR_MISSING_DATA,
			 "missing data dependent attribute");
	      goto error;
            }
          }
          else {
            attr = "positions";
          }
	  
          if (!_dxfNormalsObject((Object)obj, attr))
	    goto error;
	}
        else {
          /* check whether the normals are consistent with the connections */
          if (!CheckNormalsDirection(obj, flipfront))
             goto error;
        }
      }
      else if (!strcmp(how,"faceted")) {
	/* call normals dep connections (make sure this routine does the
	 * check whether they are there already) */
        if (!_dxfNormalsObject((Object)obj,"connections"))
	  goto error;
      }
      else if (!strcmp(how,"smooth")) {
	/* call normals dep positions (make sure this routine does the
	 * check whether they are there already) */
        if (!_dxfNormalsObject((Object)obj,"positions"))
	  goto error;
      }
      break;
    case (CLASS_GROUP):
      /* it's a composite field */
      if (shade == 0) {
	/* if there are normals, we need to disable them */
	first = (Field)DXGetEnumeratedMember((Group)obj, 0, NULL);
	/* if there are normals, we need to disable them */
	if (DXGetComponentValue(first, "normals")) {
	  
	  for (i=0; (child=(Field)DXGetEnumeratedMember((Group)obj,i,NULL));i++){
	    if (!DXSetIntegerAttribute((Object)child, "shade", 0))
	      goto error;
	  }
	}
      }
      /* we want the object shaded */
      else if (shade==1) {
	if (!strcmp(how, "default")) {
	  /* only do something if there aren't any normals. otherwise
	   * do nothing */
	  first = (Field)DXGetEnumeratedMember((Group)obj, 0, NULL);
	  if (!DXGetComponentValue(first, "normals")) {
	    
	    /* follow data if present */
	    if (DXGetComponentValue(first,"data")) {
	      attr = DXGetString((String)DXGetComponentAttribute(first,
								 "data",
								 "dep"));
	      if (!attr) {
		DXSetError(ERROR_MISSING_DATA,
			   "missing data dependent attribute");
		goto error;
	      }
	    }
	    else {
	      attr = "positions";
	    }
	    
	    /* call normals on the composite field */
	    if (!_dxfNormalsObject(obj, attr))
	      goto error;
	  }
          else {
            /* check whether the normals are consistent with the connections */
            if (!CheckNormalsDirection(obj, flipfront))
               goto error;
          }
	}
	else if (!strcmp(how,"faceted")) {
	  /* call normals dep connections (make sure this routine does the
	   * check whether they are there already) */
          if (!_dxfNormalsObject((Object)obj,"connections"))
	    goto error;
	}
	else if (!strcmp(how,"smooth")) {
	  /* call normals dep positions (make sure this routine does the
	   * check whether they are there already) */
          if (!_dxfNormalsObject((Object)obj,"positions"))
	    goto error;
	}
      }
    }
    break;
  default:
    break;
  }
  
  return OK;
 error:
  return ERROR;
  
}


/* this routine checks the normals against the direction of the
   connections. If they disagree, it fixes the normals. This can
   be called on either a field or a composite field. */
static Error CheckNormalsDirection(Object obj, int flipfront)
{
   int i;
   Object child;

   switch (DXGetObjectClass(obj)) {
     case (CLASS_FIELD):
       if (!CheckNormalsDirectionField((Field)obj, flipfront))
          goto error;
       break;
     case (CLASS_GROUP):
       if (DXGetGroupClass((Group)obj) != CLASS_COMPOSITEFIELD) {
	  DXSetError(ERROR_UNEXPECTED,"unexpected class in CheckNormalsDirection");
	  goto error;
       }
       for (i=0; (child = DXGetEnumeratedMember((Group)obj,i,NULL));i++){
          if (!CheckNormalsDirectionField((Field)child, flipfront))
            goto error;
       }
       break;
     default:
       DXSetError(ERROR_UNEXPECTED,"unexpected class in CheckNormalsDirection");
       goto error;

   }

   return OK;
error:
   return ERROR;
}

static Error CheckNormalsDirectionField(Field obj, int flipfront)
{
  Array connections, normals, positions, newnormals=NULL, newconnections=NULL;
  Point *pos1, *pos2, *pos3, *norm, vec1, vec2, crossprod, newnormal;
  Point position1, position2, position3;
  Vector zerovec={0.0, 0.0, 0.0};
  char *str1, *str2;
  Object dep, eType;
  ArrayHandle positionshandle=NULL, normalshandle=NULL, connectionshandle=NULL;
  int rank, shape[10], *connections_ptr, conn1, conn2, conn3;
  int numitems, i=0, *cptr1, numcon, scratchtri[3];
  int scratchquad[4];
  float dotprod, scratch[3];
  Triangle newtri, *triptr=NULL;
  Quadrilateral newquad, *quadptr=NULL;
  
  /* get the connections component */
  connections = (Array)DXGetComponentValue(obj, "connections");
  if (!connections) {
     goto done;
  }
  if (!DXGetArrayInfo(connections, &numcon, NULL, NULL, NULL, NULL))
    goto error;
  /* get the element type attribute */
  eType = DXGetAttribute((Object)connections, "element type");
  if (! eType || DXGetObjectClass(eType) != CLASS_STRING) {
    DXSetError(ERROR_DATA_INVALID,
	       "invalid or missing element type attribute");
    goto error;
  }
  str1 = DXGetString((String)eType);
  /* what about faces XXX ? */
  if (strcmp(str1, "quads") && strcmp(str1, "triangles"))
    goto done;


  connectionshandle = DXCreateArrayHandle(connections);
  if (!connectionshandle)
    goto error;

  /* first do the flipping on the connections. That way we only
     flip the normals once */
  if (flipfront) {
    if (!strcmp(str1,"triangles")) {
      newconnections = DXNewArray(TYPE_INT, CATEGORY_REAL, 1, 3);
      if (!newconnections)
	goto error;
      if (!DXAddArrayData(newconnections, 0, numcon, NULL))
	goto error;
      for (i=0; i<numcon; i++) {
	if (NULL == 
	    (triptr = DXIterateArray(connectionshandle, i, 
				     triptr, (Pointer)scratchtri)))
	  goto error;
	newtri.p = triptr->p;
	newtri.q = triptr->r;
	newtri.r = triptr->q;
	if (!DXAddArrayData(newconnections, i, 1, &newtri))
	  goto error;
      }
    }
    else { /* quads */
	/* else I'll treat it as irregular XXX */
	newconnections = DXNewArray(TYPE_INT, CATEGORY_REAL, 1, 4);
	if (!newconnections)
	  goto error;
	if (!DXAddArrayData(newconnections, 0, numcon, NULL))
	  goto error;
	for (i=0; i<numcon; i++) {
	  if (NULL == 
	      (quadptr = DXIterateArray(connectionshandle, i, 
					quadptr, (Pointer)scratchquad)))
	    goto error;
	  /* XXX this is irregular */
	  newquad.p = quadptr->r;
	  newquad.q = quadptr->s;
	  newquad.r = quadptr->p;
	  newquad.s = quadptr->q;
	  if (!DXAddArrayData(newconnections, i, 1, &newquad))
	    goto error;
	}
    }
    if (!DXSetComponentValue(obj, "connections", (Object)newconnections))
      goto error;
    newconnections = NULL;
    DXChangedComponentValues(obj,"connections");
    connections = (Array)DXGetComponentValue(obj, "connections");
    connectionshandle = DXCreateArrayHandle(connections);
    if (!connectionshandle)
       goto error;
  }
  
  /* get the dependency attribute of the normals */
  normals = (Array)DXGetComponentValue(obj, "normals");
  if (!normals) {
    /* I don't think this should happen, but if it does, it's ok */
    goto done;
  }
  
  dep = DXGetAttribute((Object)normals, "dep");
  if (! dep || DXGetObjectClass(dep) != CLASS_STRING) {
    DXSetError(ERROR_DATA_INVALID,
	       "invalid or missing normals dep attribute");
    goto error;
  }
  str2 = DXGetString((String)dep);
  if (strcmp(str2, "positions") && strcmp(str2, "connections")) {
    DXSetError(ERROR_DATA_INVALID,"unrecognized normals dep attribute");
    goto error;
  }
  
  positions = (Array)DXGetComponentValue(obj, "positions");
  if (!positions) {
    goto done;
  }
  
  
  positionshandle = DXCreateArrayHandle(positions);
  if (!positionshandle)
    goto error;
  if (!DXGetArrayInfo(positions, NULL, NULL, NULL, &rank, shape))
    goto error;
  if (rank != 1) {
    DXSetError(ERROR_DATA_INVALID,"rank %d positions not supported", rank);
    goto error;
  }
  
  normalshandle = DXCreateArrayHandle(normals);
  if (!normalshandle)
    goto error;
  if (!DXGetArrayInfo(normals, &numitems, NULL, NULL, NULL, NULL))
    goto error;
  
  if ((shape[0] != 2)&&(shape[0] != 3)) {
    DXSetError(ERROR_DATA_INVALID,
	       "only 2D and 3D positions supported for quads and triangles");
    goto error;
  }
  
  if (!strcmp(str1, "triangles")) {
    /* check the first triangle */
    connections_ptr = (int *)DXGetArrayData(connections);
    conn1 = connections_ptr[0];
    conn2 = connections_ptr[1];
    conn3 = connections_ptr[2];
    
    if (NULL == (pos1 = DXGetArrayEntry(positionshandle, conn1, scratch)))
      goto error;
    if (shape[0]==2) 
      position1 = DXPt(pos1->x, pos1->y, 0.0);
    else
      position1 = DXPt(pos1->x, pos1->y, pos1->z);
    
    if (NULL == (pos2 = DXGetArrayEntry(positionshandle, conn2, scratch)))
      goto error;
    if (shape[0]==2) 
      position2 = DXPt(pos2->x, pos2->y, 0.0);
    else
      position2 = DXPt(pos2->x, pos2->y, pos2->z);
    
    if (NULL == (pos3 = DXGetArrayEntry(positionshandle, conn3, scratch)))
      goto error;
    if (shape[0]==2) 
      position3 = DXPt(pos3->x, pos3->y, 0.0);
    else
      position3 = DXPt(pos3->x, pos3->y, pos3->z);
    
    vec1 = DXSub(position2, position1);
    vec2 = DXSub(position3, position2);
    crossprod = DXCross(vec1, vec2); 
    
    /* now grab an appropriate normal */
    if (!strcmp(str2,"positions")) {
      if (NULL == 
	  (norm = DXGetArrayEntry(normalshandle, conn1, scratch)))
	goto error;
    }
    else {
      if (NULL == 
	  (norm = DXGetArrayEntry(normalshandle, 0, (Pointer)scratch)))
	goto error;
    }
    
    /* check the dot product of the normal with crossprod. Should be + */
    dotprod = DXDot(crossprod, *norm);
    if (dotprod > 0) 
      goto done;
    else { 
      /* flip the normals */
      switch (DXGetArrayClass(normals)) {
      case (CLASS_CONSTANTARRAY):
	if (NULL == 
	    (norm = DXIterateArray(normalshandle, i, 
				   norm, (Pointer)scratch)))
	  goto error;
	newnormal = DXSub(zerovec, *norm);
	newnormals = (Array)DXNewConstantArray(numitems, &newnormal, TYPE_FLOAT,
					CATEGORY_REAL, 1, 3);
	break;
      default:
	newnormals = DXNewArray(TYPE_FLOAT,CATEGORY_REAL, 1, 3);
	if (!DXAddArrayData(newnormals, 0, numitems, NULL))
	  goto error; 
	for (i=0; i<numitems; i++) {
	  if (NULL == 
              (norm = DXIterateArray(normalshandle, i, 
                                     norm, (Pointer)scratch)))
	    goto error;
	  newnormal = DXSub(zerovec, *norm);
	  if (!DXAddArrayData(newnormals, i, 1, &newnormal))
	    goto error;
	}
	break;
      }
      if (!DXSetComponentValue(obj, "normals", (Object)newnormals))
	goto error;
      newnormals = NULL;
      DXChangedComponentValues(obj,"normals");
    }
  }
  else { 
    /* check the first quad */
    
    if (NULL==(cptr1 = DXGetArrayEntry(connectionshandle, 0, scratchquad)))
      goto error;
    if (NULL == (pos1 = DXGetArrayEntry(positionshandle, cptr1[0], scratch)))
      goto error;
    if (shape[0]==2) 
      position1 = DXPt(pos1->x, pos1->y, 0.0);
    else
      position1 = DXPt(pos1->x, pos1->y, pos1->z);
    
    if (NULL == (pos2 = DXGetArrayEntry(positionshandle, cptr1[1], scratch)))
      goto error;
    if (shape[0]==2) 
      position2 = DXPt(pos2->x, pos2->y, 0.0);
    else
      position2 = DXPt(pos2->x, pos2->y, pos2->z);
    
    if (NULL == (pos3 = DXGetArrayEntry(positionshandle, cptr1[2], scratch)))
      goto error;
    if (shape[0]==2) 
      position3 = DXPt(pos3->x, pos3->y, 0.0);
    else
      position3 = DXPt(pos3->x, pos3->y, pos3->z);
    

    vec1 = DXSub(position3, position1);
    vec2 = DXSub(position2, position1);
    crossprod = DXCross(vec1, vec2); 
    
    /* now grab an appropriate normal */
    if (!strcmp(str2,"positions")) {
      if (NULL == 
	  (norm = DXGetArrayEntry(normalshandle, *cptr1, scratch)))
	goto error;
    }
    else {
      if (NULL == 
	  (norm = DXGetArrayEntry(normalshandle, 0, (Pointer)scratch)))
	goto error;
    }
    
    /* check the dot product of the normal with crossprod. Should be + */
    dotprod = DXDot(crossprod, *norm);
    if (dotprod > 0) 
      goto done;
    else { 
      /* flip the normals */
      switch (DXGetArrayClass(normals)) {
      case (CLASS_CONSTANTARRAY):
	if (NULL == 
	    (norm = DXIterateArray(normalshandle, i, 
				   norm, (Pointer)scratch)))
	  goto error;
	newnormal = DXSub(zerovec, *norm);
	newnormals = (Array)DXNewConstantArray(numitems, &newnormal, TYPE_FLOAT,
					CATEGORY_REAL, 1, 3);
	break;
      default:
	newnormals = DXNewArray(TYPE_FLOAT,CATEGORY_REAL, 1, 3);
	if (!DXAddArrayData(newnormals, 0, numitems, NULL))
	  goto error; 
	for (i=0; i<numitems; i++) {
	  if (NULL == 
              (norm = DXIterateArray(normalshandle, i, 
                                     norm, (Pointer)scratch)))
	    goto error;
	  newnormal = DXSub(zerovec, *norm);
	  if (!DXAddArrayData(newnormals, i, 1, &newnormal))
	    goto error;
	}
	break;
      }
      if (!DXSetComponentValue(obj, "normals", (Object)newnormals))
	goto error;
      newnormals = NULL;
      DXChangedComponentValues(obj,"normals");
    }
  }   

   
 done:
  DXFreeArrayHandle(positionshandle); 
  DXFreeArrayHandle(normalshandle); 
  DXFreeArrayHandle(connectionshandle); 
  return OK;
 error:
  DXFreeArrayHandle(positionshandle); 
  DXFreeArrayHandle(normalshandle); 
  DXFreeArrayHandle(connectionshandle); 
  DXDelete((Object)newnormals);
  DXDelete((Object)newconnections);
  return ERROR;

}
