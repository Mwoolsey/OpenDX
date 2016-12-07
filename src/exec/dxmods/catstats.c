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

#include "dx/dx.h"
#include <math.h>
#include "cat.h"

#define STAT_COUNT	0
#define STAT_MEAN	1
#define STAT_SD		2
#define STAT_VAR	3
#define STAT_MIN	4
#define STAT_MAX	5
#define STAT_ACCUM	6
#define STAT_UNDEF	99

#define STR_COUNT	"count"
#define STR_MEAN	"mean"
#define STR_SD		"sd"
#define STR_VAR		"var"
#define STR_MIN		"min"
#define STR_MAX		"max"
#define STR_ACCUM	"accum"

#define STR_DATA	"data"

static Error traverse(Object *, Object *);
static Error doLeaf(Object *, Object *);
static Error HasInvalid(Field f, char *component, ICH *ih);

int
CategoryStatistics_worker(float *out_data, int in_knt, int out_knt, Array cat_array,
	Array data_array, Type cat_type, Type data_type, int operation);

Error
m_CategoryStatistics(Object *in, Object *out)
{
  int i;

  out[0] = NULL;

  if (in[0] == NULL)
  {
    DXSetError(ERROR_MISSING_DATA, "\"input\" must be specified");
    return ERROR;
  }

  if (!traverse(in, out))
    goto error;

  return OK;

error:
  for (i = 0; i < 1; i++)
  {
    if (in[i] != out[i])
      DXDelete(out[i]);
    out[i] = NULL;
  }
  return ERROR;
}


static Error
traverse(Object *in, Object *out)
{
  switch(DXGetObjectClass(in[0]))
  {
    case CLASS_FIELD:
/*     case CLASS_ARRAY: */
      if (! doLeaf(in, out))
  	     return ERROR;

      return OK;

#if 0
    case CLASS_GROUP:
    {
      int   i, j;
      int   memknt;
      Class groupClass  = DXGetGroupClass((Group)in[0]);

      DXGetMemberCount((Group)in[0], &memknt);

        for (i = 0; i < memknt; i++)
        {
          Object new_in[4], new_out[1];

          if (in[0])
            new_in[0] = DXGetEnumeratedMember((Group)in[0], i, NULL);
          else
            new_in[0] = NULL;

          new_in[1] = in[1];
          new_in[2] = in[2];
          new_in[3] = in[3];

          new_out[0] = DXGetEnumeratedMember((Group)out[0], i, NULL);

          if (! traverse(new_in, new_out))
            return ERROR;

          DXSetEnumeratedMember((Group)out[0], i, new_out[0]);

        }
      return OK;
    }

    case CLASS_XFORM:
    {
      int    i, j;
      Object new_in[4], new_out[1];

      if (in[0])
        DXGetXformInfo((Xform)in[0], &new_in[0], NULL);
      else
        new_in[0] = NULL;

      new_in[1] = in[1];
      new_in[2] = in[2];
      new_in[3] = in[3];

      DXGetXformInfo((Xform)out[0], &new_out[0], NULL);

      if (! traverse(new_in, new_out))
        return ERROR;

      DXSetXformObject((Xform)out[0], new_out[0]);

      return OK;
    }

    case CLASS_SCREEN:
    {
       int    i, j;
       Object new_in[4], new_out[1];

       if (in[0])
         DXGetScreenInfo((Screen)in[0], &new_in[0], NULL, NULL);
       else
         new_in[0] = NULL;

       new_in[1] = in[1];
       new_in[2] = in[2];
       new_in[3] = in[3];

       DXGetScreenInfo((Screen)out[0], &new_out[0], NULL, NULL);

       if (! traverse(new_in, new_out))
         return ERROR;

       DXSetScreenObject((Screen)out[0], new_out[0]);

       return OK;
     }

     case CLASS_CLIPPED:
     {
       int    i, j;
       Object new_in[4], new_out[1];


       if (in[0])
         DXGetClippedInfo((Clipped)in[0], &new_in[0], NULL);
       else
         new_in[0] = NULL;

       new_in[1] = in[1];
       new_in[2] = in[2];
       new_in[3] = in[3];

       DXGetClippedInfo((Clipped)out[0], &new_out[0], NULL);

       if (! traverse(new_in, new_out))
         return ERROR;

       DXSetClippedObjects((Clipped)out[0], new_out[0], NULL);

       return OK;
     }
#endif

     default:
     {
       DXSetError(ERROR_BAD_CLASS, "input must be Field");
       return ERROR;
     }
  }
}

static int
doLeaf(Object *in, Object *out)
{
  int result=0;
  Field field;
  Category category;
  Category lookup_category;
  int rank, shape[30];
  char *cat_comp;
  char *data_comp;
  char *lookup_comp;
  char name_str[256];
  char *opstr;
  int operation;
  int lookup_knt;
  int lookup_knt_provided = 0;
  Array cat_array = NULL;
  Array data_array = NULL;
  Array out_array = NULL;
  Array array = NULL;
  Array lookup_array = NULL;
  float *out_data;
  int data_knt, cat_knt;
  int out_knt=0;
  Type cat_type, data_type, lookup_type;
  float floatmax;
  ICH invalid;

  if (DXGetObjectClass(in[0]) == CLASS_FIELD)
  {
    field = (Field)in[0];

    if (DXEmptyField(field))
      return OK;
  }

  if (!DXExtractString((Object)in[1], &opstr))
	opstr = STR_COUNT;

  if (!strcmp(opstr, STR_COUNT))
    operation = STAT_COUNT;
  else if (!strcmp(opstr, STR_MEAN))
    operation = STAT_MEAN;
  else if (!strcmp(opstr, STR_SD))
    operation = STAT_SD;
  else if (!strcmp(opstr, STR_VAR))
    operation = STAT_VAR;
  else if (!strcmp(opstr, STR_MIN))
    operation = STAT_MIN;
  else if (!strcmp(opstr, STR_MAX))
    operation = STAT_MAX;
  else if (!strcmp(opstr, STR_ACCUM))
    operation = STAT_ACCUM;
  else
    operation = STAT_UNDEF;

  if (operation == STAT_UNDEF) {
    DXSetError(ERROR_BAD_PARAMETER, "statistics operation must be one of: count, mean, sd, var, min, max");
    goto error;
  }

  if (!DXExtractString((Object)in[2], &cat_comp))
	cat_comp = STR_DATA;
  if (!DXExtractString((Object)in[3], &data_comp))
	data_comp = STR_DATA;

  if (in[0])
  {
      if (DXGetObjectClass(in[0]) != CLASS_FIELD)
      {
        DXSetError(ERROR_BAD_CLASS, "\"input\" should be a field");
        goto error;
      }

      cat_array = (Array)DXGetComponentValue((Field)in[0], cat_comp);
      if (! cat_array)
      {
        DXSetError(ERROR_MISSING_DATA, "\"input\" has no \"%s\" categorical component", cat_comp);
        goto error;
      }

      if (DXGetObjectClass((Object)cat_array) != CLASS_ARRAY)
      {
        DXSetError(ERROR_BAD_CLASS, "categorical component \"%s\" of \"input\" should be an array", cat_comp);
        goto error;
      }

      if (!HasInvalid((Field)in[0], cat_comp, &invalid))
      {
        DXSetError(ERROR_INTERNAL, "Bad invalid component");
        goto error;
      }

      if (invalid)
      {
        DXSetError(ERROR_DATA_INVALID, "categorical component must not contain invalid data");
        goto error;
      }

      DXGetArrayInfo(cat_array, &cat_knt, &cat_type, &category, &rank, shape);
      if ( (cat_type != TYPE_BYTE && cat_type != TYPE_UBYTE && cat_type != TYPE_INT && cat_type != TYPE_UINT)
             || category != CATEGORY_REAL || !((rank == 0) || ((rank == 1)&&(shape[0] == 1))))
      {
        DXSetError(ERROR_DATA_INVALID, "categorical component %s must be scalar non-float", cat_comp);
        goto error;
      }

      if (operation != STAT_COUNT) {
        data_array = (Array)DXGetComponentValue((Field)in[0], data_comp);
        if (! data_array)
        {
          DXSetError(ERROR_MISSING_DATA, "\"input\" has no \"%s\" data component", data_comp);
          goto error;
        }

        if (DXGetObjectClass((Object)data_array) != CLASS_ARRAY)
        {
          DXSetError(ERROR_BAD_CLASS, "data component \"%s\" of \"input\" should be an array", data_comp);
          goto error;
        }

        DXGetArrayInfo(data_array, &data_knt, &data_type, &category, &rank, shape);
        if ( (data_type != TYPE_BYTE && data_type != TYPE_UBYTE && data_type != TYPE_INT && data_type != TYPE_UINT
             && data_type != TYPE_FLOAT && data_type != TYPE_DOUBLE)	
               || category != CATEGORY_REAL || !((rank == 0) || ((rank == 1)&&(shape[0] == 1))))
        {
          DXSetError(ERROR_DATA_INVALID, "data component \"%s\" must be scalar", data_comp);
          goto error;
        }

        if (data_knt != cat_knt)
        {
	  DXSetError(ERROR_DATA_INVALID, "category and data counts must be the same");
	  goto error;
        }
      }
  }

  if (in[4]) {
      if (DXExtractString((Object)in[4], &lookup_comp)) {
	    lookup_array = (Array)DXGetComponentValue((Field)in[0], lookup_comp);
	    if (!lookup_array)
	    {
	      DXSetError(ERROR_MISSING_DATA, "\"input\" has no \"%s\" lookup component", lookup_comp);
	      goto error;
	    }
      } else if (DXExtractInteger((Object)in[4], &lookup_knt)) {
	    lookup_knt_provided = 1;
	    out_knt = lookup_knt;
      } else if (DXGetObjectClass((Object)in[4]) == CLASS_ARRAY) {
	    lookup_array = (Array)in[4];
	    sprintf(name_str, "%s lookup", cat_comp);
	    lookup_comp = name_str;
      } else { 
            DXSetError(ERROR_DATA_INVALID, "lookup component must be string, integer, or array");
	    goto error;
      }
  } else {
      sprintf(name_str, "%s lookup", cat_comp);
      lookup_comp = name_str;
      lookup_array = (Array)DXGetComponentValue((Field)in[0], lookup_comp);
  } 

  if (lookup_array) {
    DXGetArrayInfo(lookup_array, &lookup_knt, &lookup_type, &lookup_category, &rank, shape);
    out_knt = lookup_knt;
  } else if (!lookup_knt_provided){
    if (!DXStatistics((Object)in[0], cat_comp, NULL, &floatmax, NULL, NULL)) {
      DXSetError(ERROR_INTERNAL, "Bad statistics on categorical component");
      goto error;
    }
    out_knt = (int)(floatmax+1.5);
  }

  out_array = DXNewArray(TYPE_FLOAT, CATEGORY_REAL, 0);
  if (! out_array)
    goto error;

  if (! DXSetAttribute((Object)out_array, "dep", (Object)DXNewString("positions")))
    goto error;

  if (! DXAddArrayData(out_array, 0, out_knt, NULL))
    goto error;

  if (out[0])
  {
    if (DXGetObjectClass(out[0]) != CLASS_FIELD)
    {
      DXSetError(ERROR_INTERNAL, "non-field object found in output");
      goto error;
    }

    if (DXGetComponentValue((Field)out[0], "data"))
      DXDeleteComponent((Field)out[0], "data");

    if (! DXSetComponentValue((Field)out[0], "data", (Object)out_array))
      goto error;

    if (lookup_array) {
      if (! DXSetComponentValue((Field)out[0], lookup_comp, (Object)lookup_array))
        goto error;
    }
  }
  else
  {
    out[0] = (Object)DXNewField();
    array = DXMakeGridPositions(1, out_knt, 0.0, 1.0);
    if (!array)
	goto error;
    DXSetComponentValue((Field)out[0], "positions", (Object)array);
    array = DXMakeGridConnections(1, out_knt);
    if (!array)
	goto error;
    DXSetComponentValue((Field)out[0], "connections", (Object)array);
    DXSetComponentValue((Field)out[0], "data", (Object)out_array);
    if (lookup_array) {
      if (! DXSetComponentValue((Field)out[0], lookup_comp, (Object)lookup_array))
        goto error;
    }
  }

  out_data = DXGetArrayData(out_array);
  if (! out_data)
    goto error;

  result = CategoryStatistics_worker(
  		out_data, cat_knt, out_knt, cat_array, data_array,
		cat_type, data_type, operation);

  if (! result) {
     if (DXGetError()==ERROR_NONE)
        DXSetError(ERROR_INTERNAL, "error return from user routine");
     goto error;
  }

  result = (DXEndField((Field)out[0]) != NULL);

error:
  return result;
}

int
CategoryStatistics_worker(float *out_data, int in_knt, int out_knt, Array cat_array,
	Array data_array, Type cat_type, Type data_type, int operation)
{
    float *sumsq = NULL;
    int *count = NULL;
    float *fmin = NULL;
    float *fmax = NULL;
    int i;
    Pointer last=NULL;
    Pointer ldat=NULL;
    uint c=0;
    float d=0;
    double tmp;
    int knt;
    float mean;
    ArrayHandle cat_array_handle = NULL;
    ArrayHandle data_array_handle = NULL;

    switch (operation) {
	case STAT_COUNT:
		count = (int *)DXAllocate(out_knt*sizeof(int));
		for (i=0; i< out_knt; i++) {
		    count[i] = 0;
		}
		break;
	case STAT_MEAN:
		count = (int *)DXAllocate(out_knt*sizeof(int));
		for (i=0; i< out_knt; i++) {
		    count[i] = 0;
		    out_data[i] = 0;
		}
		break;
	case STAT_SD:
	case STAT_VAR:
		count = (int *)DXAllocate(out_knt*sizeof(int));
		sumsq = (float *)DXAllocate(out_knt*sizeof(float));
		fmin = (float *)DXAllocate(out_knt*sizeof(float));
		fmax = (float *)DXAllocate(out_knt*sizeof(float));
		for (i=0; i< out_knt; i++) {
		    count[i] = 0;
		    sumsq[i] = 0;
		    out_data[i] = 0;
		    fmin[i] =  DXD_MAX_FLOAT;
		    fmax[i] = -DXD_MAX_FLOAT;
		}
		break;
	case STAT_MIN:
		for (i=0; i< out_knt; i++)
		    out_data[i] = DXD_MAX_FLOAT;
		break;
	case STAT_MAX:
		for (i=0; i< out_knt; i++)
		    out_data[i] = -DXD_MAX_FLOAT;
		break;
	case STAT_ACCUM:
		for (i=0; i< out_knt; i++) {
		    out_data[i] = 0;
		}
		break;
    }

    if (!(cat_array_handle = DXCreateArrayHandle((Array) cat_array))) {
	goto error;
    }

    if (operation != STAT_COUNT) {
	if (!(data_array_handle = DXCreateArrayHandle((Array) data_array))) {
	    goto error;
	}
    }

    for (i=0; i<in_knt; i++)
    {
	last = DXIterateArray(cat_array_handle, i, last, &tmp);  
	if (!last)
	    goto error;

	switch (cat_type) {
	    case TYPE_UBYTE:	c = (uint)(*((ubyte *)last)); 	break;
	    case TYPE_BYTE: 	c = (uint)(*((char *)last)); 	break;
	    case TYPE_USHORT: 	c = (uint)(*((ushort *)last)); 	break;
	    case TYPE_SHORT: 	c = (uint)(*((short *)last)); 	break;
	    case TYPE_UINT: 	c = (uint)(*((uint *)last)); 	break;
	    case TYPE_INT: 	c = (uint)(*((int *)last)); 	break;
	    default: break;
	}

	if (operation != STAT_COUNT) {
	    ldat = DXIterateArray(data_array_handle, i, ldat, &tmp);  
	    if (!ldat)
		goto error;

	    switch (data_type) {
		case TYPE_FLOAT: 	d = (float)(*((float *)ldat)); 	break;
		case TYPE_DOUBLE: 	d = (float)(*((double *)ldat));	break;
		case TYPE_UBYTE:	d = (float)(*((ubyte *)ldat)); 	break;
		case TYPE_BYTE: 	d = (float)(*((char *)ldat));	break;
		case TYPE_USHORT: 	d = (float)(*((ushort *)ldat));	break;
		case TYPE_SHORT: 	d = (float)(*((short *)ldat)); 	break;
		case TYPE_UINT: 	d = (float)(*((uint *)ldat)); 	break;
		case TYPE_INT:          d = (float)(*((int *)ldat)); 	break;
	        default: break;
	    }
	}

	/* here should give warning if c is out of range */
	if (c < out_knt) {
	    switch (operation) {
		case STAT_COUNT:
			count[c]++; 
			break;
		case STAT_MEAN:
			count[c]++; 
			out_data[c] += d;
			break;
		case STAT_SD:
		case STAT_VAR:
			count[c]++;
			out_data[c] += d;
			sumsq[c] += d*d;
			if (fmin[c] > d)
			    fmin[c] = d;
			if (fmax[c] < d)
			    fmax[c] = d;
			break;
		case STAT_MIN:
			if (out_data[c] > d)
			    out_data[c] = d;
			break;
		case STAT_MAX:
			if (out_data[c] < d)
			    out_data[c] = d;
			break;
		case STAT_ACCUM:
			out_data[c] += d;
			break;
	    }
	}
    }

    if (operation != STAT_COUNT)
	DXFreeArrayHandle(data_array_handle);
    DXFreeArrayHandle(cat_array_handle);
    data_array_handle = NULL;
    cat_array_handle = NULL;

    switch (operation) {
	case STAT_COUNT:
		for (i=0; i<out_knt; i++)
		    out_data[i] = (float)count[i]; 
		DXFree(count);
		break;
	case STAT_MEAN:
		for (i=0; i<out_knt; i++) {
		    if (count[i] == 0)
			out_data[i] = 0;
		    else
			out_data[i] /= (float)count[i];
		}
		DXFree(count);
		break;
	case STAT_SD:
		for (i=0; i<out_knt; i++) {
		    if (!(knt = count[i]) || (knt == 1) || (fmin[i] == fmax[i])) {
			out_data[i] = 0;
		    } else {
			mean = out_data[i]/knt;
			out_data[i] = sqrt( (sumsq[i] - out_data[i]*out_data[i]/knt)/(knt-1) );
		    }
		}
		DXFree(fmax);
		DXFree(fmin);
		DXFree(sumsq);
		DXFree(count);
		break;
	case STAT_VAR:
		for (i=0; i<out_knt; i++) {
		    if (!(knt = count[i]) || (knt == 1) || (fmin[i] == fmax[i])) {
			out_data[i] = 0;
		    } else {
			mean = out_data[i]/knt;
			out_data[i] = (sumsq[i] - out_data[i]*out_data[i]/knt)/(knt-1);
		    }
		}
		DXFree(fmax);
		DXFree(fmin);
		DXFree(sumsq);
		DXFree(count);
		break;
    }

   return 1;
     
error:
    if (data_array_handle)
	DXFreeArrayHandle(data_array_handle);
    if (cat_array_handle)
	DXFreeArrayHandle(cat_array_handle);
    if (fmax)
	DXFree(fmax);
    if (fmin)
	DXFree(fmin);
    if (sumsq)
	DXFree(sumsq);
    if (count)
	DXFree(count);
    
   return 0;
  
}

static Error HasInvalid(Field f, char *component, ICH *ih)
{
    Error rc = OK;
    char *dep = NULL;
    char *invalid = NULL;

    *ih = NULL;

    if (!DXGetStringAttribute(DXGetComponentValue(f, component), "dep", &dep))
	return OK;
    
#define INVLEN 10     /* strlen("invalid "); */    
    
    if (!(invalid = (char *)DXAllocate(strlen(dep) + INVLEN)))
	return ERROR;

    strcpy(invalid, "invalid ");
    strcat(invalid, dep);

    /* if no component, return NULL */
    if (!DXGetComponentValue(f, invalid))
	goto done;

    *ih = DXCreateInvalidComponentHandle((Object)f, NULL, dep);
    if (!*ih) {
	rc = ERROR;
	goto done;
    }
    
  done:
    DXFree((Pointer)invalid);
    return rc;
}

