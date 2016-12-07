/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dx/dx.h>
#include <dxconfig.h>

static Matrix Identity = {
  {{ 1.0, 0.0, 0.0 },
   { 0.0, 1.0, 0.0 },
   { 0.0, 0.0, 1.0 }}
};


static Error DoPick(Object, Field, int, int, int, int, Object *);
static Error SetValidity(Object, int);

Error m_PickInvalidate(Object *in, Object *out)
{
  Object o = NULL, leaf=NULL;
  Field pickfield;
  int poke, pick, depth, exclude;
  
  /* 
   * copy the structure of in[0], which is the object in which
   * picking took place 
   */

  if (!in[0]) {
    DXSetError(ERROR_BAD_PARAMETER, "missing input");
    goto error;
  }
  o = (Object)DXCopy(in[0], COPY_STRUCTURE);
  if (!o)
    goto error;
  

  /* 
   * pickfield is expected to be a field
   */

  if (!(DXGetObjectClass(in[1])==CLASS_FIELD)) {
    DXSetError(ERROR_DATA_INVALID,"pickfield must be a field");
    goto error;
  }
 
 
  /* 
   * in[1] is the pick field. 
   * If the pick field is null or an empty field, just return 
   * the copy of the object 
   */

  if (!in[1] || DXEmptyField((Field)in[1])) {
    out[0] = o;
    return OK;
  }
  pickfield = (Field)in[1];
  
  /* 
   * get the exclude param which will be used 
   * to set picked objects, which is in[2] 
   */

  if (in[2]) {
    if (!DXExtractInteger((Object)in[2], &exclude)) {
      DXSetError(ERROR_BAD_PARAMETER,"exclude must be either 0 or 1");
      goto error;
    }
    if ((exclude !=0) &&(exclude != 1)) {
      DXSetError(ERROR_BAD_PARAMETER,"exclude must be either 0 or 1");
      goto error;
    }
  } 
  else {
    exclude = 0;
  }
  
  
  /* 
   * first set all to invalid, to initialize 
   */
  if (exclude==0) {
    if (!SetValidity(o, 0))
      goto error;
  }
  else {
    if (!SetValidity(o, 1))
      goto error;
  }
  
  
  

  /* 
   * are we to selecting a particular poke, or all of them?
   */

  if (!in[3]) {
    poke = -1;
  }
  else {
    if (!DXExtractInteger(in[3], &poke)) {
      DXSetError(ERROR_BAD_PARAMETER,"poke must be a non-negative integer");
      goto error;
    }
    if (poke < 0) {
      DXSetError(ERROR_BAD_PARAMETER,"poke must be a non-negative integer");
      goto error;
    }
  }
  
  
  /* 
   * are we to selecting a particular pick, or all of them?
   */

  if (!in[4]) {
    pick = -1;
  }
  else {
    if (!DXExtractInteger(in[4], &pick)) {
      DXSetError(ERROR_BAD_PARAMETER,"depth must be a non-negative integer");
      goto error;
    }
    if (pick < 0) {
      DXSetError(ERROR_BAD_PARAMETER,"depth must be a non-negative integer");
      goto error;
    }
  }
  
  /* 
   * are we to selecting a depth?
   */

  if (!in[5]) {
    depth = -1;
  }
  else {
    if (!DXExtractInteger(in[5], &depth)) {
      DXSetError(ERROR_BAD_PARAMETER,"depth must be a non-negative integer");
      goto error;
    }
    if (depth < 0) {
      DXSetError(ERROR_BAD_PARAMETER,"depth must be a non-negative integer");
      goto error;
    }
  }
  
 
  /* 
   * traverse the pick object, using the pick structure, 
   * passing the given parameters 
   */ 

  if (!DoPick(o, pickfield, exclude, poke, pick, depth, &leaf))
    goto error;
  
  
  
  /* 
   * successful return 
   */

  out[0] = o;
  out[1] = leaf;
  return OK;
  
  /* 
   * return on error 
   */  

 error:
  DXDelete(o);
  return ERROR;
}




/* 
 * The DoPick routine traverses  the picked object 
 */

static
  Error
  DoPick(Object o, Field pickfield, int exclude, int pokes, int picks, 
         int depth, Object *out)
{
  int pokecount, pickcount, poke, pick, i, pathlength;
  int vertexid, elementid, *path, validity;
  Object current;
  Matrix matrix;
  Array newcolors=NULL;
  int pokemin, pokemax;
  int pickmin, pickmax;
  int thisdepth;





  /* 
   * find out how many pokes there are 
   */

  DXQueryPokeCount(pickfield, &pokecount);
 
  /* 
   * The user has specified to mark all pokes 
   */

  if (pokes < 0) {
      pokemin = 0, pokemax = pokecount-1;
  }

  /* 
   * The user has specified a poke larger than the number present 
   */

  else if (pokes > pokecount-1) {
      DXSetError(ERROR_BAD_PARAMETER,
                 "only %d pokes are present", pokecount);
      return OK;
  }

  /*
   * Consider only the given poke
   */

  else
      pokemin = pokemax = pokes;



  
  /* 
   * for each poke... 
   */

  for (poke=pokemin; poke<=pokemax; poke++) {
    
    /* 
     * find out how many picks there are in this poke
     */

    if (!DXQueryPickCount(pickfield, poke, &pickcount))
      goto error;

    /*
     * The user has specified that we are to consider all of the picks
     */

    if (picks < 0) {
	pickmin = 0, pickmax = pickcount-1;
    }
   
    /* 
     * Warning if this particular poke does not contain that many picks
     */

    else if (picks > pickcount-1) {
	DXWarning("poke %d contains only %d picks", poke, pickcount);
    }

    else {
	pickmin = pickmax = picks;
  
	/* 
	 * for each pick ... 
	 */

	for (pick=pickmin; pick<=pickmax; pick++) {
      
	  /* 
	   * for the given pickfield, the current poke number, and the 
	   * current pick number,  get the traversal path "path", the length
	   * of the traversal path "pathlength", and the id's of the 
	   * picked element and the picked vertex
	   */ 

	  DXQueryPickPath(pickfield, poke, pick, &pathlength, &path,
						&elementid, &vertexid);
      
	  /* 
	   * initialize current to the picked object, and matrix to the
	   * identity matrix
	   */ 

	  current = o;
	  matrix = Identity;
	  if (depth != -1 && pathlength > depth)
	    thisdepth = depth;
	  else
	    thisdepth = pathlength;
	  
	  /* 
	   * iterate through the pick path 
	   */

	  for (i=0; i<thisdepth; i++) {
	    current = DXTraversePickPath(current, path[i], &matrix);
	    if (!current)
	      goto error;
	  }

	  /* 
	   * current is now the field level of the picked object, and we have
	   * the element and vertex id's of the picked object 
	   */
	  

	    /* 
	     * we are simply to invalidate the entire field
	     */ 

            if (exclude == 0)
              validity =  1;
            else 
              validity =  0;
	    if (!SetValidity(current, validity))
	      goto error;
 
            /* return the object */
            *out = current;
	  }
      }
  }
  
  return OK;
  
 error:
  DXDelete((Object)newcolors);
  return ERROR;
}





/* 
 * This routine simply sets all colors to the given color in object o 
 */

static Error SetValidity(Object o, int validity)
{
  Object subo;
  int i;
  InvalidComponentHandle handle=NULL;


  switch (DXGetObjectClass(o)) {
    
    
  case (CLASS_GROUP):

    /* 
     * if o is a group, call SetValidity recursively on its children 
     */

    for (i=0; (subo = DXGetEnumeratedMember((Group)o, i, NULL)); i++) 
      if (!SetValidity(subo, validity))
         goto error;
    break;
    
    
  case (CLASS_XFORM):

    /* 
     * if o is an xform, call SetValidity on its child
     */

    DXGetXformInfo((Xform)o, &subo, NULL);
    if (!SetValidity(subo, validity))
       goto error;
    break;
    
    
  case (CLASS_CLIPPED):

    /* 
     * if o is an clipped object, call SetValidity on its child 
     */

    DXGetClippedInfo((Clipped)o, &subo, NULL);
    if (!SetValidity(subo, validity))
       goto error;
    break;
    

  case (CLASS_FIELD):

    /* 
     * if o is a field, set the validity 
     */
    
    if (DXEmptyField((Field)o))
      return OK;
    

    
    if (DXGetComponentValue((Field)o, "connections")) {
      handle = DXCreateInvalidComponentHandle(o, NULL, "connections");
      if (!handle)
         goto error;
      if (validity==0) {
        if (!DXSetAllInvalid(handle))
          goto error;
      }
      else {
        if (!DXSetAllValid(handle))
          goto error;
      }
    }
    else {
      handle = DXCreateInvalidComponentHandle(o, NULL, "positions");
      if (!handle)
         goto error;
      if (validity==0) {
        if (!DXSetAllInvalid(handle))
          goto error;
      }
      else {
        if (!DXSetAllValid(handle))
          goto error;
      }
    }
    if (!DXSaveInvalidComponent((Field)o, handle))
       goto error;
    
    break;
    default:
      break;
  }
  
  
  /* 
   * successful return, or return on error 
   */


  DXFreeInvalidComponentHandle(handle);
  return OK;
 error:
  DXFreeInvalidComponentHandle(handle);
  return ERROR;
}
