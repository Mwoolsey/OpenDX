/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>

#if defined(HAVE_STRING_H)
#include <string.h>
#endif

/* SimplifySurface

   is a module that approximates a triangulated surface and resamples data

   this file contains the wrapper routines that extract DX objects,
   call the simplification routine, and build a new object with the
   result.

   It starts with the output of the "module builder" dx -builder.

   However, since the module changes the positions and connections,
   it is not possible to use COPY_STRUCTURE

   the input objects are recursively decomposed in the routine "traverse"

   and a new object with the same structure is gradually built from scratch.

 */

/*********************************************************************/
/*

  Author: Andre GUEZIEC, March/April 1997

 */
/*********************************************************************/


#include <dx/dx.h>
#include "simplesurf.h"

static Error traverse(Object *, Object *);
static Error doLeaf(Object *, Object *);

/*
 * Declare the interface routine.
 */

static int
SimplifySurface_worker(
    Field in_field,
    int p_knt, int p_dim, float *p_positions,
    int c_knt, int c_nv, int *c_connections,
    float *old_positional_error,
    int data_dimension,
    float *original_surface_data,
    float *max_error,
    float *max_data_error,
    int *preserve_volume,
    int *simplify_boundary,
    int *preserve_boundary_length,
    int *preserve_data,
    int *statistics,
    Field *simplified_surface);


/*+--------------------------------------------------------------------------+
  |                                                                          |
  |   m_SimplifySurface                                                      |
  |                                                                          |
  +--------------------------------------------------------------------------+*/


Error
m_SimplifySurface(Object *in, Object *out)
{
  int i;

  Object new_in[8];

  /* initialize all new inputs to NULL */

  for (i=0;i<8;i++)

    new_in[i] = NULL;
    
  /*
   * Initialize all outputs to NULL
   */
  out[0] = NULL;

  /*
   * Error checks: required inputs are verified.
   */

  /* Parameter "original_surface" is required. */
  if (in[0] == NULL)
  {
    DXSetError(ERROR_MISSING_DATA, "\"original_surface\" must be specified");
    return ERROR;
  }


  /* cull the invalid data:

   I have taken inspiration from "regrid.m":
  
   I create a new object
   when calling DXInvalidateConnections and subsequently
   use this new object both as input and output argument of
   obj = DXInvalidateUnreferencedPositions(obj)
   obj = DXCull(obj)
   otherwise I get an error at DXGetObjectClass of the result of
   DXCull*/

  new_in[0] = DXInvalidateConnections(in[0]);
  if (!DXInvalidateUnreferencedPositions(new_in[0])) goto error;
  if (!DXCull(new_in[0]))  goto error;
 
  /* other inputs are values*/

  for (i=1;i<8;i++)

    new_in[i] = in[i];
  
  if (!traverse(new_in, out))
    {
      goto error;
    }
 
  if (new_in[0] != in[0] && new_in[0]!=NULL) DXDelete(new_in[0]);
 
  return OK;

error:
 
  if (new_in[0] != in[0] && new_in[0]!=NULL) DXDelete(new_in[0]);
 
  DXDelete(out[0]);
  out[0] = NULL;
  
  return ERROR;
}


/*+--------------------------------------------------------------------------+
  |                                                                          |
  |   traverse                                                               |
  |                                                                          |
  +--------------------------------------------------------------------------+*/

/* 
   traverse() explores recursively the object hierarchy, runs each object through
   the SimplifySurface routine (doLeaf()) and rebuilds a new hierarchy that is
   a copy of the previous hierarchy except that there are fewer
   positions and connections. 

   Apparently, DXCopy cannot be used for this purpose, even using the DX_COPY_STRUCTURE
   parameter, because the positions and connections could not be changed when using
   DXCopy()
*/

static Error traverse(Object *in, Object *out)
{

  switch(DXGetObjectClass(in[0]))
    { 
    case CLASS_PRIVATE:
    case CLASS_LIGHT:
    case CLASS_CAMERA:
    case CLASS_FIELD:
    case CLASS_ARRAY:
    case CLASS_STRING:

      /*
       * If we have made it to the leaf level, call the leaf handler.
       */
    
      if (!doLeaf(in, out))
	return ERROR;	

      return OK;

    case CLASS_GROUP:/* if we encounter a group or a series, we need to recreate
                        a group with the same members.
                        A series is a particular type of group
	                The difference between a series and a group is 
                        each series member has a floating point value attached to it */
      {
	int   
          is_series = 0,
	  i,
	  inputknt,
	  memknt;

	float
	  position;

	Group 
	  new_group; /* create a new group to avoid type
			problems associated with series groups */

	Object new_in[8], new_out[1];
   
	DXGetMemberCount((Group)in[0], &memknt);
	
	is_series = (DXGetGroupClass((Group)in[0]) == CLASS_SERIES);

	if (is_series)
	  new_group = (Group) DXNewSeries();
	else
	  new_group = DXNewGroup();

	/* other inputs are values: pass references to the other input objects other
           than the first object when recursively calling SimplifySurface */

	for (inputknt=1;inputknt<8;inputknt++)

	  new_in[inputknt] = in[inputknt];
 
	/*
	 * Create new in and out lists for each child
	 * of the first input. 
	 */
        for (i = 0; i < memknt; i++)
	  {
        
	    /*
	     * For all inputs that are Values, pass them to 
	     * child object list.  For all that are Field/Group, get 
	     * the appropriate decendent and place it into the
	     * child input object list.
	     */

	    
	    /* input "original_surface" is Field/Group */
	    if (in[0])
	      {
		/* in the particular case when the group is a series,
		   we retrieve the floating point number associated with the 
		   group member and we will later pass on this floating point value to
		   the new series member
		   */

		if (is_series)
		  new_in[0] = DXGetSeriesMember((Series)in[0], i, &position);
 
		else
		  new_in[0] = DXGetEnumeratedMember((Group)in[0], i, NULL);

	      }
	    else
	      new_in[0] = NULL;

            /* in new_in, the first object is a reference to the ith group member,
                          the next objects are references to the parameters of 
                          SimplifySurface */

	    /*
	     * For all outputs that are Values, pass them to 
	     * child object list.  For all that are Field/Group,  get
	     * the appropriate decendent and place it into the
	     * child output object list.  Note that none should
	     * be NULL (unlike inputs, which can default).
	     */

	    /* output "simplified_surface" is Field/Group */

	    /* in fact, new_out will not be used in SimplifySurface,
	       but it will be overwritten with the "simplified" field */

	    new_out[0] = DXGetEnumeratedMember(new_group, i, NULL); 
	    
	    if (! traverse(new_in, new_out))
	      return ERROR;

	    /*
	     * Now for each output that is not a Value, replace
	     * the updated child into the object in the parent.
	     */

	    /* output "simplified_surface" is Field/Group */

	    if (is_series)
	      DXSetSeriesMember((Series)new_group, i, (double) position, new_out[0]);

	    else
	      DXSetEnumeratedMember(new_group, i, new_out[0]);

	  }

	/* set the output to the group that was newly created */
	out[0] = (Object) new_group;

	return OK;
      }

    /* this next class is the class of rigid transformations. Transformations are
       applied at rendering time, and are not used for changing the object positions
       in the visual program before rendering */

    case CLASS_XFORM:
      {
	int inputknt;

	Object new_in[8], new_out[1];

	Matrix mat[1];
	/*
	 * Create new in and out lists for the decendent of the
	 * first input.  For inputs and outputs that are Values
	 * copy them into the new in and out lists.  Otherwise
	 * get the corresponding decendents.
	 */


	/* input "original_surface" is Field/Group */
	if (in[0])
	  DXGetXformInfo((Xform)in[0], &new_in[0], &mat[0]);
	else
	  new_in[0] = NULL;
	
	/* we also store the parameters of the transformation in the Matrix mat.
	   We will have to call DXNewXform later to create an copy of the transformation
           used in the new field. Unless we prefer the new field to simply point
           to the old transformation rather than have its own copy of the transformation. 

           If the new object does *not* have its own copy, I wonder what happens
           if one object gets transformed and not the other.

                                     Input
                                     |   |
                             Translate   SimplifySurface      
                                 |               |
                               Image1          Image2

              Are we getting the same image 1 and 2, or does 2 use the old transformation
              and 1 the old. In other words, does Translate create a new transformation 
              (adds a translation, that is later composed with all other transformations
               in a combined transformation matrix) or modifies the transformation matrix, or  
               creates a new matrix and unreferences the old matrix.

               Since "simplified" is a new object with its own downstream visual program 
               it seems safer to copy the transform rather than refer to it.
        */
 
	for (inputknt=1;inputknt<8;inputknt++)

	  new_in[inputknt] = in[inputknt];
 
	new_out[0] = NULL;
	
	if (! traverse(new_in, new_out))
	  return ERROR;

	/*
	 * Now for each output that is not a Value replace
	 * the updated child into the object in the parent.
	 */
	
	{
	  Xform transform = DXNewXform((Object) new_out[0],  mat[0]);
	  out[0]   = (Object)transform;
	}

	return OK;
      }

    case CLASS_CLIPPED:
      {
	int inputknt;

	Object new_in[8], new_out[1], clipping[1];

	if (in[0])
	  DXGetClippedInfo((Clipped)in[0], &new_in[0], &clipping[0]);
	else
	  new_in[0] = NULL;
       
	for (inputknt=1;inputknt<8;inputknt++)

	  new_in[inputknt] = in[inputknt];
 
	new_out[0] = NULL;

	if (! traverse(new_in, new_out))
	  return ERROR;
	
	out[0] = (Object) DXNewClipped((Object) new_out[0], 
				       (Object) clipping[0]);

	return OK;
      }

    case CLASS_SCREEN: 
      {
	Object new_in[8], new_out[1];

	int position, z, inputknt;

	if (in[0])
	  DXGetScreenInfo((Screen)in[0], &new_in[0], &position, &z);
	else
	  new_in[0] = NULL;
      
	for (inputknt=1;inputknt<8;inputknt++)

	  new_in[inputknt] = in[inputknt];
 
	new_out[0] = NULL;

	if (! traverse(new_in, new_out))
	  return ERROR;

      
	out[0] = (Object)DXNewScreen((Object) new_out[0], position,  z);
	return OK;
      }

    default:
      {
	DXSetError(ERROR_BAD_CLASS, "encountered in object traversal");
	return ERROR;
      }
    }
}

/*+--------------------------------------------------------------------------+
  |                                                                          |
  |   doLeaf                                                                 |
  |                                                                          |
  +--------------------------------------------------------------------------+*/

static int doLeaf(Object *in, Object *out)
{
 
  int result=0, the_input;
  Array array, data_array = NULL;
  Field field;
  Pointer *in_data[8];
  int in_knt[8];
  Type      type;
  Category  category;
  int       rank, shape, data_dimension = 0;
  Object    attr;
  Object    element_type_attr;
  char      the_message[256];

  static char *in_name[8] = {"\"original_surface\"",  "\"max_error\"",
                              "\"max_data_error\"",    "\"preserve_volume\"",
                              "\"simplify_boundary\"", "\"preserve_boundary_length\"",
                              "\"preserve_data\"",     "\"statistics\""};
  /*
   * Irregular positions info
   */

  int p_knt, p_dim;
  float *p_positions;

  /*
   * Irregular connections info
   */

  int c_knt, c_nv;
  float *c_connections;


  /* positional error info:
     the positional error provides the width of the error volume for each vertex
     of the surface */

  float *old_positional_error = NULL;

  /*
   Process the first input,"original surface"  

   positions and/or connections are required, so the first input must
   be a field.
   */
 
  if (DXGetObjectClass(in[0]) != CLASS_FIELD)
  {
    /* do not process the input but return it copied */
    out[0] = DXCopy(in[0], COPY_HEADER);

    return OK;
  }
  else {
    field = (Field)in[0];

    if (DXEmptyField(field))
      return OK;

    array = (Array)DXGetComponentValue(field, "positions");
    if (! array)
    {
      DXSetError(ERROR_BAD_CLASS, 
		 "\"original_surface\" contains no positions component");
      goto error;
    }

    /* 
     * The user requested irregular positions.  So we
     * get the count, the dimensionality and a pointer to the
     * explicitly enumerated positions.  If the positions
     * are in fact regular, this will expand them.
     */
    DXGetArrayInfo(array, &p_knt, NULL, NULL, NULL, &p_dim);
 
    if (p_dim != 3) {
      DXSetError(ERROR_DATA_INVALID, "positions must be three-dimensional");
      goto error;
    } 


    p_positions = (float *)DXGetArrayData(array);
    if (! p_positions)
      goto error;

    array = (Array)DXGetComponentValue(field, "connections");
    if (! array)
    {
      DXSetError(ERROR_BAD_CLASS, 
		 "\"original_surface\" contains no connections component");
      goto error;
    }

    /*
     * Check that the field's element type matches that requested
     */
    element_type_attr = DXGetAttribute((Object)array, "element type");
    if (! element_type_attr)
    {
        DXSetError(ERROR_DATA_INVALID,
            "input \"original_surface\" has no element type attribute");
        goto error;
    }

    if (DXGetObjectClass(element_type_attr) != CLASS_STRING)
    {
        DXSetError(ERROR_DATA_INVALID,
	  "input \"original_surface\" element type attribute is not a string");
        goto error;
    }

    /* we accept triangles as well as quads, that we convert directly to triangles */

    if ((strcmp(DXGetString((String)element_type_attr), "triangles")) && 
        (strcmp(DXGetString((String)element_type_attr), "quads")))
    {
      /* then we cannot process the data.
	 it can be a field associated with a string object for instance 
         the solution is to copy the object */
	   
      out[0] = DXCopy(in[0], COPY_HEADER);

      /* we copy the header only so that the copy can be deleted if we want */

	
      return OK;
	   
      
    }

    /* 
     * The user requested irregular connections.  So we
     * get the count, the dimensionality and a pointer to the
     * explicitly enumerated elements.  If the positions
     * are in fact regular, this will expand them.
     */
    DXGetArrayInfo(array, &c_knt, NULL, NULL, NULL, &c_nv);

    c_connections = (float *)DXGetArrayData(array);
    if (! c_connections)
      goto error;

  }
  /*
   * If the input argument is not NULL then we get the 
   * data array: either the object itself, if its an 
   * array, or the data component if the argument is a field
   */
  if (! in[0])
  {
    array = NULL;
    in_data[0] = NULL;
    in_knt[0] = 0;
  }
  else { if (DXGetObjectClass(in[0]) == CLASS_ARRAY)
    {
      array = (Array)in[0];
    }
    else if (DXGetObjectClass(in[0]) == CLASS_STRING)
    {
      in_data[0] = (Pointer)DXGetString((String)in[0]);
      in_knt[0] = 1;
    }
    else { if (DXGetObjectClass(in[0]) != CLASS_FIELD)
      {
        DXSetError(ERROR_BAD_CLASS, "\"original_surface\" should be a field");
        goto error;
      }

      array = (Array)DXGetComponentValue((Field)in[0], "data");
      if (! array)
      {
        in_data[0] = NULL;
        in_knt[0]  = 0; 
	goto GET_POSITIONAL_ERROR;
      }
      
      else if (DXGetObjectClass((Object)array) != CLASS_ARRAY)
      {
          in_data[0] = NULL;
	  in_knt [0] = 0;
	  goto GET_POSITIONAL_ERROR;
      }
    }

    /* 
     * get the dependency of the data component
     */
    attr = DXGetAttribute((Object)array, "dep");
    if (! attr)
    {
      DXSetError(ERROR_MISSING_DATA, 
		 "data component of \"original_surface\" has no dependency");
      goto error;
    }

    if (DXGetObjectClass(attr) != CLASS_STRING)
    {
      DXSetError(ERROR_BAD_CLASS, 
	      "dependency attribute of data component of \"original_surface\"");
      goto error;
    }

  /*
   * The dependency of this arg should be positions
   */
    if (strcmp("positions", DXGetString((String)attr)))
    {
      in_data[0] = NULL;
    }


    /* At this point, we only want to deal with data that is 
	scalar and dependent on position */
	
    /* any type gets promoted to floats */

    else if (DXGetObjectClass(in[0]) != CLASS_STRING)    {

      /* I am putting NULL into the "shape" slot since I don't know what the rank is 
	 beforehand. If the rank is not zero or one, I am not interested 
	 in this data, and I won't try to obtain the shape information */

      DXGetArrayInfo(array, &in_knt[0], &type, &category, &rank, NULL);

      if (category != CATEGORY_REAL || (rank > 1))
	{


	  in_data[0] = NULL;
	  
	  /* then don't use this data for simplification, but still
	     resample the data afterwards */

	  in_knt[0]  = 0;
	  goto GET_POSITIONAL_ERROR;
	}

      /* if the rank is zero then the data dimension is one */

      if (rank == 0) data_dimension = 1;

      /* otherwise the rank is one and I am fetching the data dimension now */
      
      else DXGetArrayInfo(array, &in_knt[0], &type, &category, &rank, &data_dimension);

      /* we are not interested in this data for simplification if the dimension is
	 larger than three. This is for memory management purposes.
	 the algorithm would work all the same, only a bit slower. */

      if (data_dimension > 3)
	{
	  in_data[0] = NULL;
	
	  in_knt[0]  = 0;
	  
	  goto GET_POSITIONAL_ERROR;
	}
      
      else if (DXQueryConstantArray(array, NULL, NULL))
	{
	  /* do not use the data of a constant array for simplification purposes */
	  in_data[0] = NULL;
	  in_knt[0]  = 0;

	  goto GET_POSITIONAL_ERROR;
	}

      else if (type == TYPE_FLOAT) 
	/* then get the data directly. No need to convert it to floats since it is already converted */
	in_data[0] = DXGetArrayData(array);

      else
	{
	  /* otherwise take that data and promote the array to floats */

	  /* I spent an *entire* day debugging to realize that
	     DXArrayConvert() sets an error code, and screws up the rest of
	     the module if array is of TYPE_FLOAT. Apparently, it is not
	     clever enough to realize this.

	     This is why I separately test if (type == TYPE_FLOAT) 
	     and in that case, avoid to call DXArrayConvert() */
	
	  data_array = DXArrayConvert(array, TYPE_FLOAT, CATEGORY_REAL, rank);

	  /* again, if this fails, we can't use the data */

	  if (!data_array) 
	    {
	      in_data[0] = NULL;
	
	      in_knt[0]  = 0;
	  
	      goto GET_POSITIONAL_ERROR;
	    }

	  else in_data[0] = DXGetArrayData(data_array);
	}

    }

GET_POSITIONAL_ERROR:

    /* we try to extract the "positional error" component from the input field
       if it exists, in which case we extract the *old_positional_error data 
       and pass it on to the SimplifySurface driver */

    array = (Array)DXGetComponentValue(field, "positional error");
    
    if (array)
      {
	/* verify that it is dependent on positions */
	attr = DXGetAttribute((Object)array, "dep");
 
	if (!strcmp("positions", DXGetString((String)attr)))
	  {
	    int knt;

	    /* verify that the array info makes sense */
	     DXGetArrayInfo(array, &knt, &type, &category, &rank, NULL);
	     
	     if (type == TYPE_FLOAT && category == CATEGORY_REAL && rank < 2 && knt == p_knt)

	       old_positional_error = (float *) DXGetArrayData(array);

	  }
      }

  }

  /*
   Then, process Input 1: Input 1 specifies the maximum simplification error.
   In case Input 1 is not specified, we compute a data dependent default value
   */

  if (! in[1])
  {
    array = NULL;
    in_data[1] = NULL;
    in_knt [1] = 0;
  }

  else
  {
    if (DXGetObjectClass(in[1]) == CLASS_ARRAY)
    {
      array = (Array)in[1];
    }
    else if (DXGetObjectClass(in[1]) == CLASS_STRING)
    {
      in_data[1] = (Pointer)DXGetString((String)in[1]);
      in_knt [1] = 1;
    }
    else
    {
      if (DXGetObjectClass(in[1]) != CLASS_FIELD)
      {
        DXSetError(ERROR_BAD_CLASS, "\"max_error\" should be a field");
        goto error;
      }

      array = (Array)DXGetComponentValue((Field)in[1], "data");
      if (! array)
      {
        DXSetError(ERROR_MISSING_DATA, "\"max_error\" has no data component");
        goto error;
      }

      if (DXGetObjectClass((Object)array) != CLASS_ARRAY)
      {
        DXSetError(ERROR_BAD_CLASS, 
		   "data component of \"max_error\" should be an array");
        goto error;
      }
    }


    if (DXGetObjectClass(in[1]) != CLASS_STRING)    {
       DXGetArrayInfo(array, &in_knt[1], &type, &category, &rank, &shape);
       if (type != TYPE_FLOAT || category != CATEGORY_REAL ||
             !((rank == 0) || ((rank == 1)&&(shape == 1))))
       {
         DXSetError(ERROR_DATA_INVALID, "input \"max_error\"");
         goto error;
       }

       in_data[1] = DXGetArrayData(array);
       if (! in_data[1])
          goto error;

    }
  }

  /*
   Then, process Input 2: Input 2 specifies the maximum simplification error
   on the data. Input 2 is only relevant if the data attached to the surface
   (Input 0) will be preserved during simplification.
   */

  if (! in[2])
  {
    /* the maximum error on the data was not provided,
       so we will later use a default value */

    array = NULL;
    in_data[2] = NULL;
    in_knt[2]  = 0;
  
  }
  else
  {
    if (DXGetObjectClass(in[2]) == CLASS_ARRAY)
    {
      array = (Array)in[2];
    }
    else if (DXGetObjectClass(in[2]) == CLASS_STRING)
    {
      in_data[2] = (Pointer)DXGetString((String)in[2]);
      in_knt[2] = 1;
    }
    else
    {
      if (DXGetObjectClass(in[2]) != CLASS_FIELD)
      {
        DXSetError(ERROR_BAD_CLASS, "\"max_data_error\" should be a field");
        goto error;
      }

      array = (Array)DXGetComponentValue((Field)in[2], "data");
      if (! array)
      {
        DXSetError(ERROR_MISSING_DATA, "\"max_data_error\" has no data component");
        goto error;
      }

      if (DXGetObjectClass((Object)array) != CLASS_ARRAY)
      {
        DXSetError(ERROR_BAD_CLASS, "data component of \"max_data_error\" should be an array");
        goto error;
      }
    }


    if (DXGetObjectClass(in[2]) != CLASS_STRING)    {
       DXGetArrayInfo(array, &in_knt[2], &type, &category, &rank, &shape);
       if (type != TYPE_FLOAT || category != CATEGORY_REAL ||
             !((rank == 0) || ((rank == 1) && (shape == 1))))
       {
         DXSetError(ERROR_DATA_INVALID, "input \"max_data_error\"");
         goto error;
       }

       in_data[2] = DXGetArrayData(array);
       if (! in_data[2])
          goto error;

    }
  }

  /*
   Process Input 3, which is a flag specifying whether the object volume should
   be preserved.

   Input 3,4,5,6,7 are all flags. To reduce the size of this file, 
   I treat them in a loop. Originally, I have
   kept the structure provided by the Module Builder, that consists of
   rewriting similar code for each input (except for the input name);
   */

 for(the_input=3;the_input<8;the_input++) {

  if (! in[the_input])
  {
    array = NULL;
    /* the input value was not specified. We'll pick default values later */
    in_data[the_input] = NULL;
    in_knt[the_input]  = 0;
  }
  else
  {
    if (DXGetObjectClass(in[the_input]) == CLASS_ARRAY)
    {
      array = (Array)in[the_input];
    }
    else if (DXGetObjectClass(in[the_input]) == CLASS_STRING)
    {
      in_data[the_input] = (Pointer)DXGetString((String)in[the_input]);
      in_knt[the_input] = 1;
    }
    else
    {
      if (DXGetObjectClass(in[the_input]) != CLASS_FIELD)
      {
        sprintf(the_message," %s should be a field", in_name[the_input]);
        DXSetError(ERROR_BAD_CLASS, the_message);
        goto error;
      }

      array = (Array)DXGetComponentValue((Field)in[the_input], "data");
      if (! array)
      {
        
        sprintf(the_message," %s has no data component", in_name[the_input]);
        DXSetError(ERROR_MISSING_DATA, the_message);
        goto error;
      }

      if (DXGetObjectClass((Object)array) != CLASS_ARRAY)
      {
        sprintf(the_message,"data component of %s should be an array", in_name[the_input]);
        DXSetError(ERROR_BAD_CLASS, the_message);
        goto error;
      }
    }


    if (DXGetObjectClass(in[the_input]) != CLASS_STRING)    {
       DXGetArrayInfo(array, &in_knt[the_input], &type, &category, &rank, &shape);

       if (type != TYPE_INT || category != CATEGORY_REAL || !((rank == 0) || ((rank == 1) && (shape == 1))))
	 {
	   sprintf(the_message, "input %s", in_name[the_input]);
           DXSetError(ERROR_DATA_INVALID, the_message);
           goto error;
	 }

       in_data[the_input] = DXGetArrayData(array);
       if (! in_data[the_input])
          goto error;

    }
  }
}
  /*
   Process Input 4, which is a flag specifying whether the surface boundary should
   be simplified ----> done within the loop
   */


/* START_WORKER */
 
  {

    float 
        max_error,
        max_error_data;

    /* default values for the options "volume", "boundary" "length" and "data" */

    int 
        preserve_volume          = 1,
        simplify_boundary        = 0,
        preserve_boundary_length = 1,
        preserve_data            = 1,
        statistics               = 0;

    Field f = DXNewField(); /* create a new field to contain the simplified surface */

    if (!f) goto error;

    /* verify that the input parameters have all been specified.
       if this is not the case, use the defaults */

    if (in_data[1] == NULL) /* this is the \"maximum_simplification_error\" parameter */
      {
	 max_error =  _dxfFloatDataBoundingBoxDiagonal(
						       (float *)p_positions,
						       p_knt, 3);

	/* use one percent of the bounding box diagonal */
	
	max_error /= 100.;

	DXMessage ( "SimplifySurface: 'max_error' defaults to 1 percent of bbox diagonal: %g .", max_error);

	in_knt [1]  = 1  ;
	in_data[1]  = (Pointer) &max_error;
      }

     /* process in priority the "preserve_data" parameter
	to determine whether the user decided to ignore the data
	component before computing a default max_error_data parameter */

     if (in_data[6] == NULL)
       {
	 in_knt [6] = 1;
	 in_data[6] = (Pointer) &preserve_data;
       }

   
    
    if (in_data[2] == NULL)
      {
	 /* the maximum error on the data was not provided,
	    so we are using a default value */

	if (in_data[0] ) /* if the data is an array of floats */
         {
          /* did we set the "preserve_data" flag to zero, meaning
	     that we want to ignore the data ? */
           int *preserve_data_flag = (int *)in_data[6];
           
           if (preserve_data_flag[0] == 1) 

	  /* then, in that case, the bounding box diagonal for the data still should
	     be larger than zero, otherwise, the data does not need preserving */

	  if ((max_error_data = _dxfFloatDataBoundingBoxDiagonal((float *) in_data[0], 
								 in_knt[0], data_dimension)) > 0.0)
	    {
	      /* by default, we allow ten percent error on the data bounding box diagonal
		 more than on the positions */
	     
	      max_error_data /= 10.;
	      
	      DXMessage ( 
		    "SimplifySurface: 'max_error_data' defaults to 10 percent of data bbox diagonal: %g .", 
		    max_error_data);

	      in_knt [2] = 1;

	      in_data[2] = (Pointer) &max_error_data;
	    }
         }
      }

     if (in_data[3] == NULL) /* parameter instructing DX whether to preserve volume
                                or not preserve volume */
       {
	 in_knt [3] = 1;

	 in_data[3] = (Pointer) &preserve_volume;

       }
   
     if (in_data[4] == NULL) /* parameter instructing DX whether to simplify the boundary
                                edges in addition to the interior edges */
       {
	 in_knt [4] = 1;
	 in_data[4] = (Pointer) &simplify_boundary;
       }

     if (in_data[5] == NULL) /* in case the boundary edges are simplified, parameter specifying
                                whether the length of the boundary should be preserved */
       {
	 in_knt [5] = 1;
	 in_data[5] = (Pointer) &preserve_boundary_length;
       }
   
     if (in_data[7] == NULL) /* instructs SimplifySurface to provide limited statistics
                                on the simplification: original and final numbers of
                                vertices and triangles, and percentage of original
                                vertices and triangles. */
       {
	 in_knt [7] = 1;
	 in_data[7] = (Pointer) &statistics;
       }


   
   
    /*
     Call the worker routine. Check the error code.
     */
    
    result = SimplifySurface_worker(
				      (Field) in[0],
				      p_knt, p_dim, (float *) p_positions,
				      c_knt, c_nv,  (int *)   c_connections,
				      old_positional_error,
				      data_dimension,
				      (float *)in_data[0],
				      (float *)in_data[1],
				      (float *)in_data[2],
				      (int *)in_data[3],
				      (int *)in_data[4],
				      (int *)in_data[5],
				      (int *)in_data[6],
				      (int *)in_data[7],
				      &f
				      );
   if (! result)
    {
       if (DXGetError()==ERROR_NONE)
          DXSetError(ERROR_INTERNAL, "error return from user routine"); 
       goto error;
    }

    /* creates boxes and neighbors */
    DXEndField(f);

    out[0] = (Object) f;
  }

  
  
  /*
   * In either event: success or failure of the simplification, clean up allocated memory
   */

  if (data_array) DXFree((Pointer)data_array);

  return result;

error:
   
  if (data_array) DXFree((Pointer)data_array);

  return 0;
}

/*+--------------------------------------------------------------------------+
  |                                                                          |
  |   SimplifySurface_worker                                                 |
  |                                                                          |
  +--------------------------------------------------------------------------+*/


static int
SimplifySurface_worker(
    Field in_field,
    int p_knt, int p_dim, float *p_positions,
    int c_knt, int c_nv, int *c_connections,
    float *old_positional_error,
    int data_dimension,
    float *original_surface_data,
    float *max_error,
    float *max_data_error,
    int *preserve_volume,
    int *simplify_boundary,
    int *preserve_boundary_length,
    int *preserve_data,
    int *statistics,
    Field *simplified_surface)
{
  /*
   * The arguments to this routine are:
   *
   *  p_knt:           total count of input positions
   *  p_dim:           dimensionality of input positions
   *  p_positions:     pointer to positions list
   *  c_knt:           total count of input connections elements
   *  c_nv:            number of vertices per element
   *  c_connections:   pointer to connections list


   old_positional_error: the values of the component "positional error"
   if any are available

   */



/*+--------------------------------------------------------------------------+
  |                                                                          |
  |   _dxfSimplifySurfaceDriver                                              |
  |                                                                          |
  +--------------------------------------------------------------------------+*/

/* call the driver routine that simplifies the surface */

{
  
  Array pos = DXNewArray(TYPE_FLOAT, CATEGORY_REAL, 1, 3);

  if (!pos)
    goto error;
  
  else
    {
      int 
        nT                               = c_knt,
	new_nV                           = 0, 
	new_nT                           = 0,
	old_nV_after_manifold_conversion = 0,
	old_nT_after_manifold_conversion = 0,
        *connections                     = (int *) c_connections,
	*new_connections                 = NULL,
	*vertex_parents                  = NULL,
	*face_parents                    = NULL,
	*vertex_lut                      = NULL,
	*face_lut                        = NULL;

      float 
	*new_positions        = NULL,
	*vertex_data          = NULL,
	vertex_data_max_error = 0,
	*new_vertex_data      = NULL,
	*new_positional_error = NULL,
	*face_normals         = NULL,
	*old_face_areas       = NULL,
	*new_face_areas       = NULL;

      static char *element_type[5] = {"","","", "triangles", "quads"};
  
      /* since there isn't always data attached to the vertices,
	 original_surface_data may be NULL
	 and accessing original_surface_data[0] would cause a segmentation fault
	 this is why I created the intermediate pointers "vertex_data"
	 and "vertex_data_max_error"
       */

      if (original_surface_data /* if there is data at all */
          && 
          max_data_error        /* and if we have a valid error bound*/ 
	  && 
	  /* also, the user may explicitely want to ignore the data: */
	  preserve_data[0])
	{

	  vertex_data             = (float *) original_surface_data;
	  vertex_data_max_error   =           max_data_error[0];

	}
	/* if simple statistics are requested, we print the number of vertices and triangles
	   before and after simplification, as well as the percentage of the original number
           of triangles that they represent */

	if (statistics[0])
	   DXMessage ( "SimplifySurface: 'original_surface' has %d vertices and %d %s.", p_knt, c_knt,
		       element_type[c_nv]);

      /* since we don't know beforehand how many new positions there
	 will be nor how many new connections,

	 _dxfSimplifySurfaceDriver()
	 uses DXAllocate() to allocate new_positions, new_vertex_data,
	 new_connections, vertex_parents, face_parents, vertex_lut,
	 new_positional_error, face_normals, old_face_areas, new_face_areas.

	 Accordingly, I run DXFree() on each array after completion of the routine */

      /* convert the quads to triangles */

      if ((c_nv == 4) && !(_dxfQuads2Triangles(&nT, &connections, &face_lut))) 
        goto failed_creating_surface;

      if (_dxfSimplifySurfaceDriver(p_knt, p_positions,
				    nT, connections,
				    data_dimension,
				    vertex_data,
				    max_error[0],
				    vertex_data_max_error,
				    &old_nV_after_manifold_conversion,
				    &old_nT_after_manifold_conversion,
				    &new_nV, &new_positions, &new_vertex_data,
				    &new_nT, &new_connections, 
				    &vertex_parents, &face_parents, &vertex_lut, &face_lut,
				    old_positional_error,
				    &new_positional_error, &face_normals, 
				    &old_face_areas, &new_face_areas,    
				    preserve_volume[0],
				    simplify_boundary[0],
				    preserve_boundary_length[0]))
	
	{
	  Array con  = DXNewArray(TYPE_INT, CATEGORY_REAL, 1, 3);
	    
	  if (!con)
	    goto failed_creating_surface;
	    
	  else
	    {
	      Array pos_error = DXNewArray(TYPE_FLOAT, CATEGORY_REAL, 1, 1); 
		
	      if (!pos_error)
		goto failed_creating_surface;

	      else
		{ 
		  Array nor  = DXNewArray(TYPE_FLOAT, CATEGORY_REAL, 1, 3);
		  
		  if (!nor)
		    goto failed_creating_surface;
		  else
		    {
		     /* provide simple statistics, if explicitely requested by the user */

	             if (statistics[0])
                        {
	                  DXMessage ( "SimplifySurface: 'simplified' has %d vertices (%2.1f percent of original)...", 
                                       new_nV, 100. * (double)new_nV/(double)p_knt);
	                  DXMessage ( "SimplifySurface: ...and %d triangles (%2.1f percent of original).", 
                                       new_nT, 100. * (double)new_nT/(double)c_knt);
                        }
		      
                      /* copy the new positions and new connections to the newly created
                         arrays */
		      DXAddArrayData(pos,  0, new_nV, new_positions);
		      DXFree((Pointer)new_positions);

		      DXAddArrayData(con,  0, new_nT, new_connections);
			  
                      /* "positional error" is a component created by SimplifySurface() */
		      DXAddArrayData(pos_error, 0, new_nV, new_positional_error);
		      DXFree((Pointer)new_positional_error);

                      /* convert the triangle normals that were computed by the
                         SimplifySurfaceDriver() routine to vertex normals,
                         using averaging weighted with triangle areas */

		      _dxfVertexNormalsfromTriangleNormals(nor, new_nV, new_nT, new_connections,
							   face_normals, new_face_areas);

		      DXFree((Pointer)new_connections);

		      DXFree((Pointer)new_face_areas);
			
		      DXFree((Pointer)face_normals);
			
                      /* set the positions, connections and other components in the
                         new field "*simplified_surface */

		      DXSetComponentValue(*simplified_surface, "positions", (Object)pos);
		      DXSetComponentValue(*simplified_surface, "positional error", (Object)pos_error);
		      DXSetComponentValue(*simplified_surface, "normals", (Object)nor);

		      DXSetConnections(*simplified_surface, "triangles", con);
		   

                      /* define the dependencies and other relations between field 
                         components */

		      /* set positions as dependent on positions */

		      if (!DXSetComponentAttribute(
						   *simplified_surface, "positions", 
						   "dep", 
						   (Object)DXNewString("positions")))
			goto failed_creating_surface;

		      /* set positional error as dependent on positions */
		    
		      if (!DXSetComponentAttribute(
						   *simplified_surface, "positional error", 
						   "dep", 
						   (Object)DXNewString("positions")))
			goto failed_creating_surface;
			
		      /* set normals as dependent on positions */
			  
		      if (!DXSetComponentAttribute(
						   *simplified_surface, "normals", 
						   "dep", 
						   (Object)DXNewString("positions")))
			goto failed_creating_surface;

		      /* create the data component only if it was carried along the simplification */
			  
		      if (new_vertex_data)
			{ 
			  Array data = DXNewArray(TYPE_FLOAT, CATEGORY_REAL, 1, data_dimension);

			  if (!data)
			    goto failed_creating_surface;
			      
			  DXAddArrayData(data, 0, new_nV, new_vertex_data);
			  DXFree((Pointer)new_vertex_data);
			  DXSetComponentValue(*simplified_surface, "data", (Object)data);

			  /* set data as dependent on positions */
		    
			  if (!DXSetComponentAttribute(
						       *simplified_surface, "data", 
						       "dep", 
						       (Object)DXNewString("positions")))
			    goto failed_creating_surface;
			}
			
		      /* resample the components dep on positions and dep on connections
			 that can be resampled. By "resampling", I mean using
                         the old values (vectors) attached to the original surface vertices
                         and triangles and the vertex_parents and face_parents arrays,
                         that indicate to which old elements the new elements correspond
                         to compute new values (vectors) attached to the new vertices and
                         triangles */
		      
		      _dxfResampleComponentValuesAfterSimplification(in_field, simplified_surface,
						    old_nV_after_manifold_conversion, 
						    old_nT_after_manifold_conversion, new_nV, new_nT,
						    vertex_parents, face_parents, vertex_lut, face_lut,
						    old_face_areas);
					
		      
		      /* copy the surface attributes such as "name"
			 or "fuzz" or "Tube diameter", things like that */
		      /*                      destination,      source */
		      if (!DXCopyAttributes((Object)*simplified_surface,
					    (Object)in_field))
			goto error;
		      
		      DXFree((Pointer)vertex_parents);

		      DXFree((Pointer)face_parents);

		      DXFree((Pointer)vertex_lut);

		      DXFree((Pointer)face_lut);

		      DXFree((Pointer)old_face_areas); 

                      if (connections && connections != c_connections) 
                        DXFree((Pointer)connections);
		    }
		}
	    }
	}   
	
      else 
	{ 

	failed_creating_surface:

          if (connections && connections != c_connections) DXFree((Pointer)connections);
	  if (new_positions       )                        DXFree((Pointer)new_positions);
	  if (new_connections     )                        DXFree((Pointer)new_connections);
	  if (new_positional_error)                        DXFree((Pointer)new_positional_error);
	  if (face_normals        )                        DXFree((Pointer)face_normals);
	  if (vertex_parents      )                        DXFree((Pointer)vertex_parents);
	  if (face_parents        )                        DXFree((Pointer)face_parents);
	  if (vertex_lut          )                        DXFree((Pointer)vertex_lut);
	  if (face_lut            )                        DXFree((Pointer)face_lut);
	  if (old_face_areas      )                        DXFree((Pointer)old_face_areas); 
	  if (new_face_areas      )                        DXFree((Pointer)new_face_areas);
	  if (new_vertex_data     )                        DXFree((Pointer)new_vertex_data);

	  goto error;
	}
    }
}

     
  /*
   * successful completion
   */
   return 1;
     
  /*
   * unsuccessful completion
   */
error:  
 

   return 0;
  
}

/*+--------------------------------------------------------------------------+
  |                                                                          |
  |   _dxfResampleComponentValuesAfterSimplification                         |
  |                                                                          |
  +--------------------------------------------------------------------------+*/

int _dxfResampleComponentValuesAfterSimplification(Field original_surface, 
						   Field *simplified_surface,
						   int old_nV_after_conversion,
						   int old_nT_after_conversion,
						   int new_nV,
						   int new_nT,
						   int *vertex_parents, int *face_parents, 
						   int *vertex_lut, int *face_lut, float *old_face_areas)
{
  /* get each enumerated component after each other, and create a
     resampled version of it in the simplified surface whenever
     applicable, except for the positions, connections, and data, if
     there is already a data component in the simplified surface */

  register int num_component = 0;

  char *component_name = NULL;

  Array 
    array     = NULL,
    new_array = NULL;

  while ((array = (Array)DXGetEnumeratedComponentValue(original_surface, num_component++, &component_name)))
    {

      Object attr = DXGetAttribute((Object)array, "dep");

      /* reject "positions", "connections",  "normals", "neighbors", "positional error", components
	"invalid positions", "invalid_connections" because the surface was DXCulled so
	they are irrelevant.
	are there other components that I should reject?

	yes! "box" */

       if (strcmp("positions", component_name) != 0           && strcmp("connections", component_name) != 0 &&
	   strcmp("normals", component_name) != 0             && strcmp("neighbors", component_name) != 0 &&  
	   strcmp("invalid positions", component_name)   != 0 &&  
	   strcmp("invalid connections", component_name) != 0 &&  
	   strcmp("positional error", component_name)    != 0 &&  
	   strcmp("box", component_name)    != 0 )
	 {
	   
	   /* reject "data" component if already present in the
	      "*simplified_surface" field */

	   if (strcmp("data", component_name) != 0 || /* or if it *is* the data component
							 I dont want to see it in the simplified surface
							 already */
	       DXGetComponentValue(*simplified_surface, "data") == NULL)

	     /* there must be an attribute */

	     if (attr && DXGetObjectClass(attr) == CLASS_STRING)
	       {

		 /* it must be either dependent on positions...
		  but not of type string*/

		 if (!strcmp("positions", DXGetString((String)attr)))
		   {
		     /* in which case we resample according to rule 1
			and create the same attribute for the "*simplified_surface" Field */
		     
		     new_array = _dxfResampleComponentDepPosAfterSimplification(array,
										      old_nV_after_conversion,
										      new_nV,
										      vertex_parents,
										      vertex_lut);
										     

		     if (new_array)
		       {
			 DXSetComponentValue(*simplified_surface, component_name, (Object)new_array);

			 /* set component as dependent on positions */
		    
			 if (!DXSetComponentAttribute(*simplified_surface, component_name, 
						      "dep", 
						      (Object)DXNewString("positions")))
			   goto error;
		       }

		   }

		 /* or on connections... */

		 else if (!strcmp("connections", DXGetString((String)attr)))
		   {
		     /* dep on connections component, resample according to rule 2
			and create the same attribute for the "*simplified_surface" Field */
		     
		     new_array = _dxfResampleComponentDepConAfterSimplification(array, 
										      old_nT_after_conversion,
										      new_nT,
										      face_parents,
										      face_lut,
										      old_face_areas);
		     

		     if (new_array)
		       {
			 DXSetComponentValue(*simplified_surface, component_name, (Object)new_array);

			 /* set component as dependent on positions */
		    
			 if (!DXSetComponentAttribute(*simplified_surface, component_name, 
						      "dep", 
						      (Object)DXNewString("connections")))
			   goto error;
		       }
		   }
	       }
	 }
       
       /* DXFree((Pointer)component_name);*/
    }

  return 1;

error:

  if (new_array) DXDelete ((Object) new_array);

  return 0;
}

/*+--------------------------------------------------------------------------+
  |                                                                          |
  |   _dxfResampleComponentDepPosAfterSimplification                         |
  |                                                                          |
  +--------------------------------------------------------------------------+*/

Array _dxfResampleComponentDepPosAfterSimplification(Array array, 
						     int old_nV_after_conversion,
						     int new_nV, int *vertex_parents, 
						     int *vertex_lut)
{
  /* returns NULL in case the resampling is not possible */

  Array  new_array = NULL;


  Pointer new_array_data = NULL;

  Type type;
  Category category;

  int 
    *vertex_weights = NULL,
    rank, 
    shape[3],
    size, 
    tensor_size = 1, 
    old_nV, 
    i;

  float *vertex_data_averages = NULL;

  DXGetArrayInfo(array, &old_nV, &type, &category, &rank, NULL);

  /* I do not average strings (of course) or higher order tensors  */
  
  if (type == TYPE_STRING || rank > 3) goto error;

  /* now that I am sure that the rank is 3 or less I can call the
     same routine with 3 allocated slots for shape */

  DXGetArrayInfo(array, &old_nV, &type, &category, &rank, shape);


  /* particular treatment if the array is a constant array */

  if (DXQueryConstantArray(array, &old_nV, NULL))
    {
      if (rank == 0)
	new_array = (Array) DXNewConstantArray(new_nV, DXGetConstantArrayData(array), type, category, rank, 1);
      
      else
	new_array = (Array) DXNewConstantArrayV(new_nV, DXGetConstantArrayData(array), type, category, rank, shape);
      
      if (!new_array) goto error;
    }
  
  else
 
    {
      /* the array was not a constant array: we need to resample */

      if (rank == 0)
	new_array = DXNewArray(type, category, rank, 1);
      else
	new_array = DXNewArrayV(type, category, rank, shape);

      if (!new_array) goto error;

      size = DXTypeSize(type);
      
      /* multiply the element size by the element shape */
      
      for(i=0;i<rank;i++)
	tensor_size *= shape[i];

      new_array_data = DXAllocateZero(size * tensor_size * new_nV);

      /* DXAllocate sets an error code if the allocation actually fails 
	 but does not free anything if the error code is set;
	 this is why we need to free */

      if (!new_array_data) goto error;

      vertex_weights = (int *) DXAllocateZero(new_nV * sizeof (int));
  
      if (!vertex_weights) goto error;

      vertex_data_averages = (float *) DXAllocateZero(new_nV * tensor_size * sizeof (float));

      if (!vertex_data_averages) goto error;
    
      switch(type)
	{
	case TYPE_BYTE:
	  _dxfRESAMPLE_DEP_POS_AFTER_SIMPLIFICATION(byte); break;

	case TYPE_UBYTE:
	  _dxfRESAMPLE_DEP_POS_AFTER_SIMPLIFICATION(ubyte); break;
	  
	case TYPE_DOUBLE:
	  _dxfRESAMPLE_DEP_POS_AFTER_SIMPLIFICATION(double); break;
	  
	case TYPE_FLOAT:
	  _dxfRESAMPLE_DEP_POS_AFTER_SIMPLIFICATION(float); break;
	  
	case TYPE_INT:
	  _dxfRESAMPLE_DEP_POS_AFTER_SIMPLIFICATION(int); break;
	  
	case TYPE_UINT:
	  _dxfRESAMPLE_DEP_POS_AFTER_SIMPLIFICATION(uint); break;
	  
	case TYPE_SHORT:
	  _dxfRESAMPLE_DEP_POS_AFTER_SIMPLIFICATION(short); break;
	  
	case TYPE_USHORT:
	  _dxfRESAMPLE_DEP_POS_AFTER_SIMPLIFICATION(ushort); break;
	  
	default:
	  goto error; break;
	  
	}
  
      DXAddArrayData(new_array, 0, new_nV, new_array_data);
      DXFree((Pointer) new_array_data);
      DXFree((Pointer) vertex_weights);
      DXFree((Pointer) vertex_data_averages);
    }

  return new_array;

error:

  if (new_array)            DXDelete ((Object) new_array);
  if (new_array_data)       DXFree(new_array_data);
  if (vertex_weights)       DXFree(vertex_weights);
  if (vertex_data_averages) DXFree(vertex_data_averages);

 return (Array) NULL;
}

/*+--------------------------------------------------------------------------+
  |                                                                          |
  |   _dxfResampleComponentDepConAfterSimplification                         |
  |                                                                          |
  +--------------------------------------------------------------------------+*/

Array _dxfResampleComponentDepConAfterSimplification(Array array, 
						     int old_nT_after_conversion,
						     int new_nT, int *face_parents, int *face_lut,
						     float *old_face_areas)
{
  /* returns NULL in case an error occured and the resampling will not be done */

  Array   new_array = NULL;

  Pointer new_array_data = NULL;

  Type type;
  Category category;

  int 
    rank, 
    shape[3],
    size, 
    tensor_size = 1, 
    old_nT, 
    i;

  float 
    *face_data_weights  = NULL,
    *face_data_averages = NULL;

  DXGetArrayInfo(array, &old_nT, &type, &category, &rank, NULL);

  /* I do not average strings (of course) or higher order tensors  */
  
  if (type == TYPE_STRING || rank > 3) goto error;

  DXGetArrayInfo(array, &old_nT, &type, &category, &rank, shape);


  /* particular treatment if the array is a constant array */

  if (DXQueryConstantArray(array, &old_nT, NULL))
    {
      if (rank == 0)
	new_array = (Array) DXNewConstantArray(new_nT, DXGetConstantArrayData(array), type, category, rank, 1);
      
      else
	new_array = (Array) DXNewConstantArrayV(new_nT, DXGetConstantArrayData(array), type, category, rank, shape);
      
      if (!new_array) goto error;
    }
  
  else
 
    {
      /* the array was not a constant array: we need to resample */

      if (rank == 0)
	new_array = DXNewArray(type, category, rank, 1);
      else
	new_array = DXNewArrayV(type, category, rank, shape);

      if (!new_array) goto error;

      size = DXTypeSize(type);

      /* multiply the element size by the element shape */

      for(i=0;i<rank;i++)
	tensor_size *= shape[i];

      new_array_data = DXAllocateZero(size * tensor_size * new_nT);

      if (!new_array_data) goto error;

      face_data_averages = (float *) DXAllocateZero(new_nT * tensor_size * sizeof (float));

      if (!face_data_averages) goto error;
  
      face_data_weights = (float *) DXAllocateZero(new_nT * tensor_size * sizeof (float));

      if (!face_data_weights) goto error;
     
      switch(type)
	{
	case TYPE_BYTE:
	  _dxfRESAMPLE_DEP_CON_AFTER_SIMPLIFICATION(byte); break;
      
	case TYPE_UBYTE:
	  _dxfRESAMPLE_DEP_CON_AFTER_SIMPLIFICATION(ubyte); break;
      
	case TYPE_DOUBLE:
	  _dxfRESAMPLE_DEP_CON_AFTER_SIMPLIFICATION(double); break;
      
	case TYPE_FLOAT:
	  _dxfRESAMPLE_DEP_CON_AFTER_SIMPLIFICATION(float); break;
      
	case TYPE_INT:
	  _dxfRESAMPLE_DEP_CON_AFTER_SIMPLIFICATION(int); break;
      
	case TYPE_UINT:
	  _dxfRESAMPLE_DEP_CON_AFTER_SIMPLIFICATION(uint); break;

	case TYPE_SHORT:
	  _dxfRESAMPLE_DEP_CON_AFTER_SIMPLIFICATION(short); break;

	case TYPE_USHORT:
	  _dxfRESAMPLE_DEP_CON_AFTER_SIMPLIFICATION(ushort); break;

	default:
	  goto error; break;
	
	}
  
      DXAddArrayData(new_array, 0, new_nT, new_array_data);
      DXFree((Pointer) new_array_data);
      DXFree((Pointer) face_data_averages);
      DXFree((Pointer) face_data_weights);
    }

  return new_array;

error:

  if (new_array)            DXDelete ((Object) new_array);
  
  if (new_array_data)       DXFree(new_array_data);
  if (face_data_averages)   DXFree(face_data_averages);
  if (face_data_weights)    DXFree(face_data_weights);

 return (Array) NULL;


}

		   
/*+--------------------------------------------------------------------------+
  |                                                                          |
  |   _dxfVertexNormalsfromTriangleNormals                                   |
  |                                                                          |
  +--------------------------------------------------------------------------+*/

int _dxfVertexNormalsfromTriangleNormals(Array nor, int nV, int nT, int *triangles,
					 float *face_normals, float *face_areas)

     /* this routine uses readily available face areas and face normals to compute
	vertex normals */

{
  float *vertex_normals = DXAllocateZero(nV * 3 * sizeof (float));

  if (!vertex_normals) goto error;

  else
    {
      /* actually compute the normals by averaging the face normals weighted
	 with the face areas */

      register int i,j,k, the_vertex;

      register float *the_vertex_normal;

      for(i=0;i<nT;i++,face_areas++,face_normals+=3)

	for(j=0;j<3;j++,triangles++)
	  {
	    the_vertex = triangles[0];
	    
	    /* add to the vertex normal of the_vertex the contribution of
	       the current normal weighted by the current area */

	    the_vertex_normal = vertex_normals + 3 * the_vertex;

	    for (k=0;k<3;k++)
	      the_vertex_normal[k] += face_areas[0] * face_normals[k];
	    
	  }
      
      /* normalize the vertex normals */

	  
      the_vertex_normal = vertex_normals;

      for(i=0;i<nV;i++,the_vertex_normal+=3)
	{
	  double norm = 
	    the_vertex_normal[0]*the_vertex_normal[0]+
	    the_vertex_normal[1]*the_vertex_normal[1]+
	    the_vertex_normal[2]*the_vertex_normal[2];
	      
	  if (norm > 0.0) 
	    {
	      norm = sqrt(norm);
	      for (j=0;j<3;j++)
		the_vertex_normal[j] /= norm;
	    }
	}
	    

      
      DXAddArrayData(nor, 0, nV, vertex_normals);
			
      DXFree((Pointer)vertex_normals);
	 
    } 
  
  
  
  return 1;

error:

  return 0;
 
}

/*+--------------------------------------------------------------------------+
  |                                                                          |
  |   _dxfFloatDataBoundingBoxDiagonal                                       |
  |                                                                          |
  +--------------------------------------------------------------------------+*/

float _dxfFloatDataBoundingBoxDiagonal(float *data, int n_data, int data_dim)
{
  float diagonal = 0.0;

  float 
    *cur_data = data,
    *bbox     = NULL;
  
  register int i,j;
  
  bbox = (float *) DXAllocateZero (2 * data_dim * sizeof (float));
 
  if (!bbox) goto error;

  /* initialize the bounding box to the first array values */

  /* the bbox is organized as follows

     bbox[0]               MININUM
     bbox[1]                DATA 
                            VALUES 
			    FOR 
			    EACH
     bbox[data_dim-1]       COMPONENT


     bbox[data_dim]        MAXIMUM
     bbox[data_dim+1]       DATA 
                            VALUES 
			    FOR 
			    EACH
     bbox[2*data_dim-1]     COMPONENT
     */

  for(j=0;j<data_dim;j++)
    {
      bbox[j] = bbox[j+data_dim] = data[j];
    }

  /* update the bounding box if data values are smaller than the minimum or larger than the maximum */

  cur_data = data + data_dim;

  for (i=1;i<n_data;i++,cur_data+=data_dim)

    for(j=0;j<data_dim;j++)
      {
	if (cur_data[j] < bbox[j]) bbox[j] = cur_data[j];

	else if (cur_data[j] > bbox[j+data_dim]) bbox[j+data_dim] = cur_data[j];
      }

  /* once the bounding box is complete, compute its diameter. If the dimension of the data is
     one, we just compute max-min and thus avoid the computation of squares and of a square root */

  if (data_dim == 1) diagonal = bbox[1] - bbox[0];

  else
    {
      double 
	one_coordinate,
	double_diagonal = 0.0;
	
      for (j=0;j<data_dim;j++)
	{
	  one_coordinate   = bbox[j+data_dim] - bbox[j];
	  double_diagonal += one_coordinate * one_coordinate;
	}

      if (double_diagonal>0.0) double_diagonal = sqrt(double_diagonal);

      diagonal = (float) double_diagonal;
    }

  DXFree((Pointer)bbox); 
  return diagonal;

error:

  DXFree((Pointer)bbox);

  return 0.0;
}

/*+--------------------------------------------------------------------------+
  |                                                                          |
  |   _dxfQuads2Triangles                                                    |
  |                                                                          |
  +--------------------------------------------------------------------------+*/

int _dxfQuads2Triangles(int *nQ, int **connections, int **face_lut)
{

  /* create a new array for connections, as well as look-up table */

  int
    i, 
    new_nT = 2 * *nQ,
    *quads = *connections,
    *new_t = NULL,
    *t,
    *lut;

  new_t = (int *) DXAllocateZero(new_nT * 3 * sizeof (int));

  if (!new_t) goto error;

  *face_lut = (int *) DXAllocateZero(new_nT * sizeof (int));

  if (!(*face_lut)) goto error;

  /* the triangles are created according to the following simple rule:

         (v0,v1,v2,v3)        ---->  (v0,v2,v1) + (v1,v2,v3) see page 3-6 of the user's guide

           ___                                 v1 _v3
       v1 |   | v3                    v1|\       \ |
          |   |                ---->    |_\   +   \|
       v0 |___| v2                    v0   v2      v2

       */

  t   = new_t;

  lut = *face_lut;

  for(i=0;i< *nQ;i++,quads+=4,lut+=2)
    {
      /* write the first triangle (v0,v2,v1) */
      t[0] = quads[0];
      t[1] = quads[2];
      t[2] = quads[1];

      /* set the triangle pointer to point to the next triangle */
      t+=3;

      /* write the second triangle (v1,v2,v3) */
      t[0] = quads[1];
      t[1] = quads[2];
      t[2] = quads[3];

      t+=3;

      /* both triangles have a face look up table entry of i, which is the index
         of the original quad to which they correspond */
       
      lut[0] = i;
      lut[1] = i;
    }
   

  /* "export" the new connections array outside this routine; They will
     be freed after the simplification is completed */     

  *connections = new_t;

  /* the number of triangles after conversion of "quads" to triangles is: */

  *nQ = new_nT;

  return 1;

error:
  if (new_t)      DXFree((Pointer)new_t);
  if (*face_lut) {DXFree((Pointer)(*face_lut)); *face_lut = NULL;}
  return 0;
}

/*****************************************************************************/
