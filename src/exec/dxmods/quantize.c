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

#include <dx/dx.h>
#include "quant.h"

static Error traverse(Object *, Object *, int nColors);
static Error doLeaf(Object *, Object *, int nColors);

/*
 * Declare the interface routine.
 */
int
QuantizeColors_worker(Object image, int float_pixels,
    int, ubyte *,
    int, ubyte *,
    int);

static TreeNode traverse_series_pass1(TreeNode, Object *, int nColors);
static int      traverse_series_pass2(TreeNode, Object *, Object *);

static TreeNode doLeaf_series_pass1(TreeNode, Object *, int);
static int      doLeaf_series_pass2(TreeNode, Object *, Object *);

TreeNode
QuantizeColors_series_pass1(TreeNode, int float_pixels,
    int, ubyte *, int);

int
QuantizeColors_series_pass2(TreeNode, int float_pixels,
    int, ubyte *,
    int, ubyte *);


static TreeNodeBuf buflist   = NULL;
static TreeNode    freenodes = NULL;

static int nLeaves;

struct treeNode depthlist[8] = {{0, 0, 0, 0, 0}};

static int reduce_max = 1;
static int find_me = -1;

static int skip = 11;


Error
m_QuantizeImage(Object *in, Object *out)
{
  int i;
  int nColors = 256;

  /*
   * Initialize all outputs to NULL
   */
  out[0] = NULL;

  /*
   * Error checks: required inputs are verified.
   */

  /* Parameter "rgb_image" is required. */
  if (in[0] == NULL)
  {
    DXSetError(ERROR_MISSING_DATA, "\"rgb_image\" must be specified");
    return ERROR;
  }

  if (in[1])
  {
      if (! DXExtractInteger(in[1], &nColors) || nColors < 8 || nColors > 256)
      {
	  DXSetError(ERROR_BAD_PARAMETER, "nColors must be an integer between 8 and 256");
	  return ERROR;
      }
  }


  /*
   * Since output "quantized" is structure Field/Group, it initially
   * is a copy of input "rgb_image".
   */
  out[0] = DXCopy(in[0], COPY_STRUCTURE);
  if (! out[0])
    goto error;

  /*
   * If in[0] was an array, then no copy is actually made - Copy 
   * returns a pointer to the input object.  Since this can't be written to
   * we postpone explicitly copying it until the leaf level, when we'll need
   * to be creating writable arrays anyway.
   */
  if (out[0] == in[0])
    out[0] = NULL;

  /*
   * Call the hierarchical object traversal routine
   */
  if (!traverse(in, out, nColors))
    goto error;

  return OK;

error:
  /*
   * On error, any successfully-created outputs are deleted.
   */
  for (i = 0; i < 1; i++)
  {
    if (in[i] != out[i])
      DXDelete(out[i]);
    out[i] = NULL;
  }
  return ERROR;
}


static Error
traverse(Object *in, Object *out, int nColors)
{
  switch(DXGetObjectClass(in[0]))
  {
    case CLASS_FIELD:
    case CLASS_ARRAY:
    case CLASS_STRING:
      /*
       * If we have made it to the leaf level, call the leaf handler.
       */
      if (! doLeaf(in, out, nColors))
  	     return ERROR;

      return OK;

    case CLASS_GROUP:
	if (DXGetGroupClass((Group)in[0]) == CLASS_SERIES)
	{
	  int   i;
	  int   memknt;
	  TreeNode tree = NULL;
	  struct color ubyte_map[256];
	  int tablesz = 0;
	  Array cmap = NULL;
	  float *f;

	  for (i = 0; i < 8; i++)
	  {
	    depthlist[i].next = NULL;
	    depthlist[i].knt = 0;
	  }

	  nLeaves = 0;

	  DXGetMemberCount((Group)in[0], &memknt);

	  for (i = 0; i < memknt; i++)
	  {
	    Object o = DXGetEnumeratedMember((Group)in[0], i, NULL);
	    if (o)
	    {
	      tree = traverse_series_pass1(tree, &o, nColors);
	      if (! tree)
	      {
		CleanupNodes();
		return ERROR;
	      }
	    }
	  }

	  if (!tree)
	  {
	    CleanupNodes();
	    return OK;
	  }

	  if (! CreateTable(tree, ubyte_map, nColors, &tablesz))
	  {
	    CleanupNodes();
	    return ERROR;
	  }

	  if (tablesz > 256)
	  {
	    DXSetError(ERROR_INTERNAL,
		"too many leaves in tree: %d\n", tablesz);
	    CleanupNodes();
	    return ERROR;
	  }

	  for (i = 0; i < memknt; i++)
	  {
	    Object new_in[1], new_out[1];

	   /*
	    * For all inputs that are Values, pass them to 
	    * child object list.  For all that are Field/Group, get 
	    * the appropriate decendent and place it into the
	    * child input object list.
	    */

	    /* input "rgb_image" is Field/Group */
	    if (in[0])
	      new_in[0] = DXGetEnumeratedMember((Group)in[0], i, NULL);
	    else
	      new_in[0] = NULL;

	   /*
	    * For all outputs that are Values, pass them to 
	    * child object list.  For all that are Field/Group,  get
	    * the appropriate decendent and place it into the
	    * child output object list.  Note that none should
	    * be NULL (unlike inputs, which can default).
	    */

	    /* output "quantized" is Field/Group */
	    new_out[0] = DXGetEnumeratedMember((Group)out[0], i, NULL);

	    if (! traverse_series_pass2(tree, new_in, new_out))
	    {
	      CleanupNodes();
	      return ERROR;
	    }

	   /*
	    * Now for each output that is not a Value, replace
	    * the updated child into the object in the parent.
	    */

	    /* output "quantized" is Field/Group */
	    DXSetEnumeratedMember((Group)out[0], i, new_out[0]);

	  }

	  CleanupNodes();

	  cmap = DXNewArray(TYPE_FLOAT, CATEGORY_REAL, 1, 3);
	  if (! cmap)
	    return ERROR;

	  if (! DXAddArrayData(cmap, 0, 256, NULL))
	  {
	    DXDelete((Object)cmap);
	    return ERROR;
	  }

	  f = (float *)DXGetArrayData(cmap);

	  for (i = 0; i < tablesz; i++)
	  {
	    *f++ = ubyte_map[i].r / 255.0;
	    *f++ = ubyte_map[i].g / 255.0;
	    *f++ = ubyte_map[i].b / 255.0;
	  }

	  for ( ; i < 256; i++)
	  {
	    *f++ = 0.0;
	    *f++ = 0.0;
	    *f++ = 0.0;
	  }

	  DXSetAttribute(out[0], "color map", (Object)cmap);
	  DXSetIntegerAttribute(out[0], "direct color map", 1);

	  CleanupNodes();

	  for (i = 0; i < memknt; i++)
	  {
	    Object o = DXGetEnumeratedMember((Group)out[0], i, NULL);

	    DXSetComponentValue((Field)o, "color map", (Object)cmap);
	  }
	      
	  return OK;
	}
	else
	{
	  int   i;
	  int   memknt;

	  DXGetMemberCount((Group)in[0], &memknt);


	 /*
	  * Create new in and out lists for each child
	  * of the first input. 
	  */
	  for (i = 0; i < memknt; i++)
	  {
	    Object new_in[1], new_out[1];

	   /*
	    * For all inputs that are Values, pass them to 
	    * child object list.  For all that are Field/Group, get 
	    * the appropriate decendent and place it into the
	    * child input object list.
	    */

		/* input "rgb_image" is Field/Group */
	      if (in[0])
	      new_in[0] = DXGetEnumeratedMember((Group)in[0], i, NULL);
	    else
	      new_in[0] = NULL;

	   /*
	    * For all outputs that are Values, pass them to 
	    * child object list.  For all that are Field/Group,  get
	    * the appropriate decendent and place it into the
	    * child output object list.  Note that none should
	    * be NULL (unlike inputs, which can default).
	    */

	    /* output "quantized" is Field/Group */
	    new_out[0] = DXGetEnumeratedMember((Group)out[0], i, NULL);

	    if (! traverse(new_in, new_out, nColors))
	      return ERROR;

	   /*
	    * Now for each output that is not a Value, replace
	    * the updated child into the object in the parent.
	    */

	    /* output "quantized" is Field/Group */
	    DXSetEnumeratedMember((Group)out[0], i, new_out[0]);

	  }
	  return OK;
	}

    case CLASS_XFORM:
    {
      Object new_in[1], new_out[1];


      /*
       * Create new in and out lists for the decendent of the
       * first input.  For inputs and outputs that are Values
       * copy them into the new in and out lists.  Otherwise
       * get the corresponding decendents.
       */

      /* input "rgb_image" is Field/Group */
      if (in[0])
        DXGetXformInfo((Xform)in[0], &new_in[0], NULL);
      else
        new_in[0] = NULL;

      /*
       * For all outputs that are Values, copy them to 
       * child object list.  For all that are Field/Group,  get
       * the appropriate decendent and place it into the
       * child output object list.  Note that none should
       * be NULL (unlike inputs, which can default).
       */

      /* output "quantized" is Field/Group */
      DXGetXformInfo((Xform)out[0], &new_out[0], NULL);

      if (! traverse(new_in, new_out, nColors))
        return ERROR;

      /*
       * Now for each output that is not a Value replace
       * the updated child into the object in the parent.
       */

      /* output "quantized" is Field/Group */
      DXSetXformObject((Xform)out[0], new_out[0]);

      return OK;
    }

    case CLASS_SCREEN:
    {
      Object new_in[1], new_out[1];


      /*
       * Create new in and out lists for the decendent of the
       * first input.  For inputs and outputs that are Values
       * copy them into the new in and out lists.  Otherwise
       * get the corresponding decendents.
       */

      /* input "rgb_image" is Field/Group */
      if (in[0])
        DXGetScreenInfo((Screen)in[0], &new_in[0], NULL, NULL);
      else
        new_in[0] = NULL;


      /*
       * For all outputs that are Values, copy them to 
       * child object list.  For all that are Field/Group,  get
       * the appropriate decendent and place it into the
       * child output object list.  Note that none should
       * be NULL (unlike inputs, which can default).
       */

       /* output "quantized" is Field/Group */
       DXGetScreenInfo((Screen)out[0], &new_out[0], NULL, NULL);

       if (! traverse(new_in, new_out, nColors))
         return ERROR;

      /*
       * Now for each output that is not a Value, replace
       * the updated child into the object in the parent.
       */

      /* output "quantized" is Field/Group */
       DXSetScreenObject((Screen)out[0], new_out[0]);

       return OK;
     }

     case CLASS_CLIPPED:
     {
       Object new_in[1], new_out[1];


       /* input "rgb_image" is Field/Group */
       if (in[0])
         DXGetClippedInfo((Clipped)in[0], &new_in[0], NULL);
       else
         new_in[0] = NULL;


      /*
       * For all outputs that are Values, copy them to 
       * child object list.  For all that are Field/Group,  get
       * the appropriate decendent and place it into the
       * child output object list.  Note that none should
       * be NULL (unlike inputs, which can default).
       */

       /* output "quantized" is Field/Group */
       DXGetClippedInfo((Clipped)out[0], &new_out[0], NULL);

       if (! traverse(new_in, new_out, nColors))
         return ERROR;

      /*
       * Now for each output that is not a Value, replace
       * the updated child into the object in the parent.
       */

       /* output "quantized" is Field/Group */
       DXSetClippedObjects((Clipped)out[0], new_out[0], NULL);

       return OK;
     }

     default:
     {
       DXSetError(ERROR_BAD_CLASS, "encountered in object traversal");
       return ERROR;
     }
  }
}

static TreeNode
traverse_series_pass1(TreeNode tree, Object *in, int nColors)
{
  switch(DXGetObjectClass(in[0]))
  {
    case CLASS_FIELD:
	return doLeaf_series_pass1(tree, in, nColors);

    default:
       DXSetError(ERROR_BAD_CLASS, "encountered in series traversal");
       DXPrintV(in[0], "o", NULL);
       return ERROR;
  }
}

static int
traverse_series_pass2(TreeNode tree, Object *in, Object *out)
{
  switch(DXGetObjectClass(in[0]))
  {
    case CLASS_FIELD:
	return doLeaf_series_pass2(tree, in, out);

    default:
       DXSetError(ERROR_BAD_CLASS, "encountered in series traversal");
       return ERROR;
  }
}

static TreeNode
doLeaf_series_pass1(TreeNode tree, Object *in, int nColors)
{
  Array array;
  Field field;
  Pointer *in_data[1];
  int in_knt[1];
  Type type;
  Category category;
  int rank, shape;
  Object attr, src_dependency_attr = NULL;
  int p_knt = -1;
  int float_pixels;

  if (DXGetObjectClass(in[0]) != CLASS_FIELD)
  {
    DXSetError(ERROR_BAD_CLASS, "\"rgb_image\" should be a field");
    goto error;
  }

  field = (Field)in[0];

  if (DXEmptyField(field))
    return tree;

  /* 
   * Determine the dependency of the source object's data
   * component.
   */
  src_dependency_attr = DXGetComponentAttribute(field, "colors", "dep");
  if (! src_dependency_attr)
  {
    DXSetError(ERROR_MISSING_DATA,
	"\"rgb_image\" data component is missing a dependency attribute");
    goto error;
  }

  if (DXGetObjectClass(src_dependency_attr) != CLASS_STRING)
  {
    DXSetError(ERROR_BAD_CLASS, "\"rgb_image\" dependency attribute");
    goto error;
  }

  /*src_dependency = DXGetString((String)src_dependency_attr);*/

  array = (Array)DXGetComponentValue(field, "positions");
  if (! array)
  {
    DXSetError(ERROR_BAD_CLASS,
	"\"rgb_image\" contains no positions component");
    goto error;
  }

  DXGetArrayInfo(array, &p_knt, NULL, NULL, NULL, NULL);

  array = (Array)DXGetComponentValue((Field)in[0], "colors");
  if (! array)
  {
    DXSetError(ERROR_MISSING_DATA, "\"rgb_image\" has no colors component");
    goto error;
  }

  if (DXGetObjectClass((Object)array) != CLASS_ARRAY)
  {
    DXSetError(ERROR_BAD_CLASS,
	"colors component of \"rgb_image\" should be an array");
    goto error;
  }

  /* 
   * get the dependency of the data component
   */
  attr = DXGetAttribute((Object)array, "dep");
  if (! attr)
  {
    DXSetError(ERROR_MISSING_DATA,
	"colors component of \"rgb_image\" has no dependency");
    goto error;
  }

  if (DXGetObjectClass(attr) != CLASS_STRING)
  {
    DXSetError(ERROR_BAD_CLASS,
	"dependency attribute of data component of \"rgb_image\"");
    goto error;
  }

  /*
   * The dependency of this arg should be positions
   */
  if (strcmp("positions", DXGetString((String)attr)))
  {
    DXSetError(ERROR_DATA_INVALID,
	"colors dependency of \"rgb_image\" must be positions");
    goto error;
  }

  DXGetArrayInfo(array, &in_knt[0], &type, &category, &rank, &shape);

  if ((type != TYPE_UBYTE && type != TYPE_FLOAT)
	 || category != CATEGORY_REAL ||
	 rank != 1 || shape != 3)
  {
    DXSetError(ERROR_DATA_INVALID, "input \"rgb_image\"");
    goto error;
  }

  in_data[0] = DXGetArrayData(array);
  if (! in_data[0])
     goto error;

  float_pixels = (type == TYPE_FLOAT);
  
  /*
   * Call the user's routine.  Check the return code.
   */
  tree = QuantizeColors_series_pass1(tree, float_pixels,
    in_knt[0], (ubyte *)in_data[0], nColors);

  if (! tree)
     if (DXGetError()==ERROR_NONE)
        DXSetError(ERROR_INTERNAL, "error return from user routine");

  /*
   * In either event, clean up allocated memory
   */

error:
  return tree;
}

static int
doLeaf_series_pass2(TreeNode tree, Object *in, Object *out)
{
  int result=0;
  Array array=NULL;
  Field field;
  Pointer *in_data[1] = {NULL}, *out_data[1] = {NULL};
  int in_knt[1], out_knt[1];
  Type type;
  Category category;
  int rank, shape;
  Object attr, src_dependency_attr = NULL;
  int p_knt = -1;
  int c_knt = -1;
  int float_pixels=0;

  if (DXGetObjectClass(in[0]) == CLASS_FIELD)
  {

    field = (Field)in[0];

    if (DXEmptyField(field))
      return OK;

    /* 
     * Determine the dependency of the source object's data
     * component.
     */
    src_dependency_attr = DXGetComponentAttribute(field, "colors", "dep");
    if (! src_dependency_attr)
    {
      DXSetError(ERROR_MISSING_DATA, "\"rgb_image\" data component is missing a dependency attribute");
      goto error;
    }

    if (DXGetObjectClass(src_dependency_attr) != CLASS_STRING)
    {
      DXSetError(ERROR_BAD_CLASS, "\"rgb_image\" dependency attribute");
      goto error;
    }

    /*src_dependency = DXGetString((String)src_dependency_attr);*/

    array = (Array)DXGetComponentValue(field, "positions");
    if (! array)
    {
      DXSetError(ERROR_BAD_CLASS, "\"rgb_image\" contains no positions component");
      goto error;
    }

    DXGetArrayInfo(array, &p_knt, NULL, NULL, NULL, NULL);
    /* 
     * If there are connections, get their count so that
     * connections-dependent result arrays can be sized.
     */
    array = (Array)DXGetComponentValue(field, "connections");
    if (array)
        DXGetArrayInfo(array, &c_knt, NULL, NULL, NULL, NULL);
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
  else
  {
    if (DXGetObjectClass(in[0]) == CLASS_ARRAY)
    {
      array = (Array)in[0];
    }
    else if (DXGetObjectClass(in[0]) == CLASS_STRING)
    {
      in_data[0] = (Pointer)DXGetString((String)in[0]);
      in_knt[0] = 1;
    }
    else
    {
      if (DXGetObjectClass(in[0]) != CLASS_FIELD)
      {
        DXSetError(ERROR_BAD_CLASS, "\"rgb_image\" should be a field");
        goto error;
      }

      array = (Array)DXGetComponentValue((Field)in[0], "colors");
      if (! array)
      {
        DXSetError(ERROR_MISSING_DATA, "\"rgb_image\" has no data component");
        goto error;
      }

      if (DXGetObjectClass((Object)array) != CLASS_ARRAY)
      {
        DXSetError(ERROR_BAD_CLASS, "colors component of \"rgb_image\" should be an array");
        goto error;
      }
    }

    /* 
     * get the dependency of the data component
     */
    attr = DXGetAttribute((Object)array, "dep");
    if (! attr)
    {
      DXSetError(ERROR_MISSING_DATA, "colors component of \"rgb_image\" has no dependency");
      goto error;
    }

    if (DXGetObjectClass(attr) != CLASS_STRING)
    {
      DXSetError(ERROR_BAD_CLASS, "dependency attribute of data component of \"rgb_image\"");
      goto error;
    }

  /*
   * The dependency of this arg should be positions
   */
    if (strcmp("positions", DXGetString((String)attr)))
    {
      DXSetError(ERROR_DATA_INVALID, "colors dependency of \"rgb_image\" must be positions");
      goto error;
    }

    if (DXGetObjectClass(in[0]) != CLASS_STRING)    {
       DXGetArrayInfo(array, &in_knt[0], &type, &category, &rank, &shape);
       if ((type != TYPE_UBYTE && type != TYPE_FLOAT)
	   || category != CATEGORY_REAL ||
           rank != 1 || shape != 3)
       {
         DXSetError(ERROR_DATA_INVALID, "input \"rgb_image\"");
         goto error;
       }

       in_data[0] = DXGetArrayData(array);
       if (! in_data[0])
          goto error;

    }

    float_pixels = (type == TYPE_FLOAT);
  }
  /*
   * Create an output data array typed according to the
   * specification given
   */
  array = DXNewArray(TYPE_UBYTE, CATEGORY_REAL, 0, 0);
  if (! array)
    goto error;

  /*
   * This output data array will be dep positions - and sized
   * appropriately - if the appropriate size is known
   */
  if (p_knt == -1)
  {
    DXSetError(ERROR_DATA_INVALID,
      "cannot make output \"quantized\" dep on positions because no positions were found in input[0]");
    goto error;
  }

  out_knt[0] = p_knt;

  if (! DXSetAttribute((Object)array, "dep", (Object)DXNewString("positions")))
    goto error;
  /*
   * Actually allocate the array data space
   */
  if (! DXAddArrayData(array, 0, out_knt[0], NULL))
    goto error;

  /*
   * If the output vector slot is not NULL, then it better be a field, and
   * we'll add the new array to it as its data component (delete any prior
   * data component so that its attributes won't overwrite the new component's)
   * Otherwise, place the new array into the out vector.
   */
  if (out[0])
  {
    if (DXGetObjectClass(out[0]) != CLASS_FIELD)
    {
      DXSetError(ERROR_INTERNAL, "non-field object found in output vector");
      goto error;
    }

    if (DXGetComponentValue((Field)out[0], "colors"))
      DXDeleteComponent((Field)out[0], "colors");

    if (! DXSetComponentValue((Field)out[0], "colors", (Object)array))
      goto error;

  }
  else
    out[0] = (Object)array;
  /*
   * Now get the pointer to the contents of the array
   */
  out_data[0] = DXGetArrayData(array);
  if (! out_data[0])
    goto error;

  /*
   * Call the user's routine.  Check the return code.
   */
  result = QuantizeColors_series_pass2(tree, float_pixels,
    in_knt[0], (ubyte *)in_data[0],
    out_knt[0], (ubyte *)out_data[0]);

  if (! result)
     if (DXGetError()==ERROR_NONE)
        DXSetError(ERROR_INTERNAL, "error return from user routine");

  /*
   * In either event, clean up allocated memory
   */

error:
  return result;
}

static int
doLeaf(Object *in, Object *out, int nColors)
{
  int result=0;
  Array array=NULL;
  Field field;
  Pointer *in_data[1] = {NULL}, *out_data[1] = {NULL};
  int in_knt[1], out_knt[1];
  Type type;
  Category category;
  int rank, shape;
  Object attr, src_dependency_attr = NULL;
  int p_knt = -1;
  int c_knt = -1;
  int float_pixels=0;

  if (DXGetObjectClass(in[0]) == CLASS_FIELD)
  {

    field = (Field)in[0];

    if (DXEmptyField(field))
      return OK;

    /* 
     * Determine the dependency of the source object's data
     * component.
     */
    src_dependency_attr = DXGetComponentAttribute(field, "colors", "dep");
    if (! src_dependency_attr)
    {
      DXSetError(ERROR_MISSING_DATA, "\"rgb_image\" data component is missing a dependency attribute");
      goto error;
    }

    if (DXGetObjectClass(src_dependency_attr) != CLASS_STRING)
    {
      DXSetError(ERROR_BAD_CLASS, "\"rgb_image\" dependency attribute");
      goto error;
    }

    /*src_dependency = DXGetString((String)src_dependency_attr);*/

    array = (Array)DXGetComponentValue(field, "positions");
    if (! array)
    {
      DXSetError(ERROR_BAD_CLASS, "\"rgb_image\" contains no positions component");
      goto error;
    }

    DXGetArrayInfo(array, &p_knt, NULL, NULL, NULL, NULL);
    /* 
     * If there are connections, get their count so that
     * connections-dependent result arrays can be sized.
     */
    array = (Array)DXGetComponentValue(field, "connections");
    if (array)
        DXGetArrayInfo(array, &c_knt, NULL, NULL, NULL, NULL);
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
  else
  {
    if (DXGetObjectClass(in[0]) == CLASS_ARRAY)
    {
      array = (Array)in[0];
    }
    else if (DXGetObjectClass(in[0]) == CLASS_STRING)
    {
      in_data[0] = (Pointer)DXGetString((String)in[0]);
      in_knt[0] = 1;
    }
    else
    {
      if (DXGetObjectClass(in[0]) != CLASS_FIELD)
      {
        DXSetError(ERROR_BAD_CLASS, "\"rgb_image\" should be a field");
        goto error;
      }

      array = (Array)DXGetComponentValue((Field)in[0], "colors");
      if (! array)
      {
        DXSetError(ERROR_MISSING_DATA, "\"rgb_image\" has no data component");
        goto error;
      }

      if (DXGetObjectClass((Object)array) != CLASS_ARRAY)
      {
        DXSetError(ERROR_BAD_CLASS, "colors component of \"rgb_image\" should be an array");
        goto error;
      }
    }

    /* 
     * get the dependency of the data component
     */
    attr = DXGetAttribute((Object)array, "dep");
    if (! attr)
    {
      DXSetError(ERROR_MISSING_DATA, "colors component of \"rgb_image\" has no dependency");
      goto error;
    }

    if (DXGetObjectClass(attr) != CLASS_STRING)
    {
      DXSetError(ERROR_BAD_CLASS, "dependency attribute of data component of \"rgb_image\"");
      goto error;
    }

  /*
   * The dependency of this arg should be positions
   */
    if (strcmp("positions", DXGetString((String)attr)))
    {
      DXSetError(ERROR_DATA_INVALID, "colors dependency of \"rgb_image\" must be positions");
      goto error;
    }

    if (DXGetObjectClass(in[0]) != CLASS_STRING)    {
       DXGetArrayInfo(array, &in_knt[0], &type, &category, &rank, &shape);
       if ((type != TYPE_UBYTE && type != TYPE_FLOAT)
	   || category != CATEGORY_REAL ||
           rank != 1 || shape != 3)
       {
         DXSetError(ERROR_DATA_INVALID, "input \"rgb_image\"");
         goto error;
       }

       in_data[0] = DXGetArrayData(array);
       if (! in_data[0])
          goto error;

    }

    float_pixels = (type == TYPE_FLOAT);
  }
  /*
   * Create an output data array typed according to the
   * specification given
   */
  array = DXNewArray(TYPE_UBYTE, CATEGORY_REAL, 0, 0);
  if (! array)
    goto error;

  /*
   * This output data array will be dep positions - and sized
   * appropriately - if the appropriate size is known
   */
  if (p_knt == -1)
  {
    DXSetError(ERROR_DATA_INVALID,
      "cannot make output \"quantized\" dep on positions because no positions were found in input[0]");
    goto error;
  }

  out_knt[0] = p_knt;

  if (! DXSetAttribute((Object)array, "dep", (Object)DXNewString("positions")))
    goto error;
  /*
   * Actually allocate the array data space
   */
  if (! DXAddArrayData(array, 0, out_knt[0], NULL))
    goto error;

  /*
   * If the output vector slot is not NULL, then it better be a field, and
   * we'll add the new array to it as its data component (delete any prior
   * data component so that its attributes won't overwrite the new component's)
   * Otherwise, place the new array into the out vector.
   */
  if (out[0])
  {
    if (DXGetObjectClass(out[0]) != CLASS_FIELD)
    {
      DXSetError(ERROR_INTERNAL, "non-field object found in output vector");
      goto error;
    }

    if (DXGetComponentValue((Field)out[0], "colors"))
      DXDeleteComponent((Field)out[0], "colors");

    if (! DXSetComponentValue((Field)out[0], "colors", (Object)array))
      goto error;

  }
  else
    out[0] = (Object)array;
  /*
   * Now get the pointer to the contents of the array
   */
  out_data[0] = DXGetArrayData(array);
  if (! out_data[0])
    goto error;

  /*
   * Call the user's routine.  Check the return code.
   */
  result = QuantizeColors_worker(out[0], float_pixels,
    in_knt[0], (ubyte *)in_data[0],
    out_knt[0], (ubyte *)out_data[0], nColors);

  if (! result)
     if (DXGetError()==ERROR_NONE)
        DXSetError(ERROR_INTERNAL, "error return from user routine");

  /*
   * In either event, clean up allocated memory
   */

error:
  return result;
}

int
QuantizeColors_worker(Object image, int float_pixels,
    int rgb_image_knt, ubyte *rgb_image_data,
    int quantized_knt, ubyte *quantized_data, int nColors)
{
  /*
   * The arguments to this routine are:
   *
   *
   * The following are inputs and therefore are read-only.  The default
   * values are given and should be used if the knt is 0.
   *
   * rgb_image_knt, rgb_image_data:  count and pointer for input "rgb_image"
   *                   no default value given.
   *
   *  The output data buffer(s) are writable.
   *  The output buffer(s) are preallocated based on
   *     the dependency (positions or connections),
   *     the size of the corresponding positions or
   *     connections component in the first input, and
   *     the data type.
   *
   * quantized_knt, quantized_data:  count and pointer for output "quantized"
   */

  /*
   * User's code goes here
   */
  Array cmap = NULL;
  struct color ubyte_map[256];
  TreeNode tree = NULL;
  int i, tablesz = 0;
  float *f;

  tree = CreateTree(rgb_image_data, rgb_image_knt, nColors, float_pixels);
  if (! tree)
    goto error;

  if (! CreateTable(tree, ubyte_map, nColors, &tablesz))
      goto error;

  if (tablesz > 256)
  {
    DXSetError(ERROR_INTERNAL, "too many leaves in tree: %d\n", tablesz);
    goto error;
  }

  if (! CreateQuantPixels(tree,
		rgb_image_data, quantized_data, rgb_image_knt, float_pixels))
      goto error;

  cmap = DXNewArray(TYPE_FLOAT, CATEGORY_REAL, 1, 3);
  if (! cmap)
    goto error;

  if (! DXAddArrayData(cmap, 0, 256, NULL))
    goto error;

  f = (float *)DXGetArrayData(cmap);

  for (i = 0; i < tablesz; i++)
  {
    *f++ = ubyte_map[i].r / 255.0;
    *f++ = ubyte_map[i].g / 255.0;
    *f++ = ubyte_map[i].b / 255.0;
  }

  for (i = i; i < 256; i++)
  {
      *f++ = 0.0;
      *f++ = 0.0;
      *f++ = 0.0;
  }

  DXSetComponentValue((Field) image, "color map", (Object)cmap);
  DXSetIntegerAttribute(image, "direct color map", 1);

  /*
   * successful completion
   */
   CleanupNodes();
   return 1;
     
  /*
   * unsuccessful completion
   */
error:
   CleanupNodes();
   DXDelete((Object)cmap);
   return 0;
  
}

TreeNode
QuantizeColors_series_pass1(TreeNode tree, int float_pixels,
    int rgb_image_knt, ubyte *rgb_image_data, int nColors)
{
  tree = AddFieldToTree(tree, rgb_image_data,
			rgb_image_knt, float_pixels, nColors);
  return tree;

}

int
QuantizeColors_series_pass2(TreeNode tree, int float_pixels,
    int rgb_image_knt, ubyte *rgb_image_data,
    int quantized_knt, ubyte *quantized_data)
{
  if (! CreateQuantPixels(tree,
		rgb_image_data, quantized_data, rgb_image_knt, float_pixels))
      goto error;

   return 1;
     
  /*
   * unsuccessful completion
   */
error:
   CleanupNodes();
   return 0;
  
}

TreeNode
GetTreeNode()
{
    TreeNode new = NULL;

    if (freenodes)
    {
        new = freenodes;
        freenodes = freenodes->next;
    }
    else
    {
        if (!buflist || buflist->nextfree == ALLOC)
        {
            TreeNodeBuf newbuf =
                (TreeNodeBuf)DXAllocateZero(sizeof(struct treeNodeBuf));
            if (! newbuf)
                return NULL;

            newbuf->nextfree = 0;

            newbuf->nextbuf = buflist;
            buflist = newbuf;
        }

        new = buflist->nodes + buflist->nextfree;
        buflist->nextfree ++;
    }

    return new;
}

void
FreeNode(TreeNode node)
{
    memset(node, 0, sizeof(struct treeNode));
    node->next = freenodes;
    freenodes = node;
}

void
CleanupNodes()
{
    while (buflist)
    {
        TreeNodeBuf this = buflist->nextbuf;
        DXFree((Pointer)buflist);
        buflist = this;
    }

    freenodes = NULL;
}

TreeNode
InsertTree(TreeNode node, Color color, int depth)
{
    int index;

    if (! node)
    {
        node = GetTreeNode();
        if (! node)
            goto error;

        node->r    = 0;
        node->g    = 0;
        node->b    = 0;
        node->knt  = 0;
        node->leaf = depth == 7;

        if (! node->leaf)
        {
            node->next = depthlist[depth].next;
            depthlist[depth].next = node;
            depthlist[depth].knt ++;
        }

        if (node->leaf)
            nLeaves ++;
    }

    if (node->leaf)
    {
        node->r += color->r;
        node->g += color->g;
        node->b += color->b;
    }
    else
    {
        BRANCH(color, depth, index);
        node->kids[index] =
                InsertTree(node->kids[index], color, depth+1);
        if (! node->kids[index])
            goto error;
    }

    node->knt ++;

    return node;

error:
    return NULL;
}

int
SearchTree(TreeNode node, Color color, int depth)
{
    int index;

    if (! node)
    {
        DXSetError(ERROR_INTERNAL, "null node in SearchTree");
        return -1;
    }

    if (node->leaf)
        return node->knt;
    else
    {
        BRANCH(color, depth, index);
        return SearchTree(node->kids[index], color, depth+1);
    }
}

int
ReduceTree()
{
    int i, j, bestknt;
    TreeNode last, next, best=NULL, node;

    for (i = 6; i >= 0; i--)
    {
        if (depthlist[i].next)
        {
            if (reduce_max)
                bestknt = 0;
            else
                bestknt = 99999999;

            last = depthlist + i;
            next = last->next;
            while(next)
            {
                if (reduce_max)
                {
                    if (next->knt > bestknt)
                        best = last;
                }
                else
                {
                    if (next->knt < bestknt)
                        best = last;
                }

                last = next;
                next = next->next;
            }

            node = best->next;
            best->next = node->next;
            depthlist[i].knt --;

            for (j = 0; j < 8; j++)
            {
                TreeNode child = node->kids[j];
                if (child)
                {
                    node->r += child->r;
                    node->g += child->g;
                    node->b += child->b;
                    FreeNode(child);
                    nLeaves --;
                }
            }

            node->leaf = 1;
            nLeaves ++;

            if (i > 0)
            {
#if 0
                node->next = depthlist[i-1].next;
                depthlist[i-1].next = node;
                depthlist[i-1].knt ++;
#endif
            }
            else
            {
                DXSetError(ERROR_INTERNAL, "reduce at level 0?");
                return ERROR;
            }

            return OK;
        }
    }

    return ERROR;
}

int
CountLeaves(TreeNode tree, int *n)
{
    if (tree->leaf)
    {
        *n = *n + 1;
    }
    else
    {
        int i;
        TreeNode child;

        for (i = 0; i < 8; i++)
            if (NULL != (child = tree->kids[i]))
                if (! CountLeaves(child, n))
                    return ERROR;
    }

    return OK;
}

void
found(){}

TreeNode
CreateTree(Pointer pixdata, int knt, int nColors, int float_pixels)
{
    TreeNode tree = NULL;
    struct color pbuf;
    int skipsize;
    ubyte *u_pixdata = (ubyte *)pixdata;
    int i, j;

    if (float_pixels)
        skipsize = skip*3*sizeof(float);
    else
        skipsize = skip*sizeof(struct color);

    for (i = 0; i < 8; i++)
    {
        depthlist[i].next = NULL;
        depthlist[i].knt = 0;
    }

    nLeaves = 0;

    for (j = 0; j < skip; j++)
    {
        ubyte *p;

        if (float_pixels)
            p = u_pixdata + j*3*sizeof(float);
        else
            p = u_pixdata + j*sizeof(struct color);

        for (i = j; i < knt; i += skip, p += skipsize)
        {
            if (i == find_me)
                found();

            if (float_pixels)
            {
                float fr = ((float *)p)[0];
                float fg = ((float *)p)[1];
                float fb = ((float *)p)[2];
                pbuf.r = fr >= 1.0 ? 255 : fr <= 0.0 ? 0 :
                                        (unsigned char)(255 * fr);
                pbuf.g = fg >= 1.0 ? 255 : fg <= 0.0 ? 0 :
                                        (unsigned char)(255 * fg);
                pbuf.b = fb >= 1.0 ? 255 : fb <= 0.0 ? 0 :
                                        (unsigned char)(255 * fb);

                if (NULL == (tree = InsertTree(tree, &pbuf, 0)))
                    goto error;
            }
            else
                if (NULL == (tree = InsertTree(tree, (Color)p, 0)))
                    goto error;

            while (nLeaves > nColors)
                if (! ReduceTree())
                    goto error;
        }
    }

    return tree;

error:
    return NULL;
}

TreeNode
AddFieldToTree(TreeNode tree, Pointer pixdata, int knt,
                                int float_pixels, int nColors)
{
    struct color pbuf;
    int skipsize;
    ubyte *u_pixdata = (ubyte *)pixdata;
    int i, j;

    if (float_pixels)
        skipsize = skip*3*sizeof(float);
    else
        skipsize = skip*sizeof(struct color);

    for (j = 0; j < skip; j++)
    {
        ubyte *p;

        if (float_pixels)
            p = u_pixdata + j*3*sizeof(float);
        else
            p = u_pixdata + j*sizeof(struct color);

        for (i = j; i < knt; i += skip, p += skipsize)
        {
            if (i == find_me)
                found();

            if (float_pixels)
            {
                float fr = ((float *)p)[0];
                float fg = ((float *)p)[1];
                float fb = ((float *)p)[2];
                pbuf.r = fr >= 1.0 ? 255 : fr <= 0.0 ? 0 :
                                        (unsigned char)(255 * fr);
                pbuf.g = fg >= 1.0 ? 255 : fg <= 0.0 ? 0 :
                                        (unsigned char)(255 * fg);
                pbuf.b = fb >= 1.0 ? 255 : fb <= 0.0 ? 0 :
                                        (unsigned char)(255 * fb);

                if (NULL == (tree = InsertTree(tree, &pbuf, 0)))
                    goto error;
            }
            else
                if (NULL == (tree = InsertTree(tree, (Color)p, 0)))
                    goto error;


            while (nLeaves > nColors)
                if (! ReduceTree())
                    goto error;
        }
    }

    return tree;

error:
    return NULL;
}

Error
CreateTable(TreeNode node, Color table, int nColors, int *next)
{
    if (node->leaf)
    {
        float recip = 1.0 / node->knt;
        Color c = table + (*next);

        if ((*next) < nColors)
        {
            c->r = node->r * recip;
            c->g = node->g * recip;
            c->b = node->b * recip;

            node->knt = (*next);
        }

        *next = *next + 1;
    }
    else
    {
        int i;
        TreeNode child;

        for (i = 0; i < 8; i++)
            if (NULL != (child = node->kids[i]))
                if (! CreateTable(child, table, nColors, next))
                    return ERROR;
    }

    return OK;
}

Error
CreateQuantPixels(TreeNode tree,
        Pointer input, unsigned char *output, int knt, int float_pixels)
{
    int i;
    unsigned char *s = (unsigned char *)input;
    unsigned char *d = output;
    int skipsize;
    struct color pbuf;

    skipsize = float_pixels ? 3*sizeof(float) : sizeof(struct color);

    for (i = 0; i < knt; i++, s += skipsize, d++)
    {
        int q;

        if (i == find_me)
            found();

            if (float_pixels)
            {
                float fr = ((float *)s)[0];
                float fg = ((float *)s)[1];
                float fb = ((float *)s)[2];
                pbuf.r = fr >= 1.0 ? 255 : fr <= 0.0 ? 0 :
                                        (unsigned char)(255 * fr);
                pbuf.g = fg >= 1.0 ? 255 : fg <= 0.0 ? 0 :
                                        (unsigned char)(255 * fg);
                pbuf.b = fb >= 1.0 ? 255 : fb <= 0.0 ? 0 :
                                        (unsigned char)(255 * fb);

                q = SearchTree(tree, (Color)&pbuf, 0);
            }
            else
                q = SearchTree(tree, (Color)s, 0);

        if (q < 0)
            return ERROR;

        *d = q;
    }

    return 1;
}


