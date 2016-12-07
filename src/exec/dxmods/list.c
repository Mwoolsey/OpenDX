/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/*
 * $Header: /src/master/dx/src/exec/dxmods/list.c,v 1.7 2002/03/21 02:57:36 rhh Exp $
 */

#include <dxconfig.h>



#include <dx/dx.h>
#include "list.h"

#define MAXALLOC    100
#define MAXOBJECTS  21
#define MAXRANK     16


#define CHECK_LIMIT()         \
    if (strcnt >= maxalloc) { \
        maxalloc *= 1.5;      \
        strlist = (char **)DXReAllocate((Pointer)strlist, \
                                       sizeof(char *) * maxalloc); \
        if (!strlist)         \
            goto error;       \
    }

/* if you want to also operate on the data array from a field, set this to 1 */
#define DO_FIELDS 0




int
m_List(Object *in, Object *out)
{
    out[0] = (Object)_dxfBuildList(in);
    
    return out[0] ? OK : ERROR;
}


/*
 * internal entry point; take a list of arrays and coalesce into one array.
 */
Array _dxfBuildList(Object *in)
{
    int i, onum;
    int seen_null = 0 /*, warn_null = 0 */;
    Array new = NULL;
    Object next = NULL;
    Type type;
    Category cat;
    int rank, shape[MAXRANK];
    int items = 0, nextitems;
    int isstring = 0;
    int strcnt = 0;
    int maxalloc = MAXALLOC;
    char **strlist = NULL;
    char *src;

    int ncount = 0;
    Array alist[MAXOBJECTS];
    Array na;


    /* make room for list of strings to collect (this isn't used if the
     *  input is a list of regular arrays)
     */
    strlist = (char **)DXAllocate(sizeof(char *) * maxalloc);
    if (!strlist)
        return NULL;


    /* for each input, if it is null, skip it and continue;
     *  otherwise, make sure the type matches the first parm, and add
     *  the data to the list.
     */
    for (onum = 0; onum < MAXOBJECTS; ++onum) {

	next = in[onum];

        /* skip nulls */
        if (!next) {
            seen_null = 1;
            continue;
        }

	if (seen_null) {
      /* warn_null = 1; */
	    seen_null = 0;
	}

	/* input.  if field, get data.  if array, use bare.
	 */
	switch(DXGetObjectClass(next)) {
	  case CLASS_STRING:
	    /* must be a string */
	    if (!isstring)
		isstring = 1;
	    else if (isstring < 0) {
		DXSetError(ERROR_BAD_PARAMETER, "#10680", "inputs", 0, onum);
		goto error;
	    }
	    /* accumulate strings into list - _dxfNewStringListV() will be 
	     *  called later.
	     */
	    strlist[strcnt] = DXGetString((String)next);
	    if (!strlist[strcnt])
		goto error;

	    strcnt++;
            CHECK_LIMIT();
	    continue;

#if DO_FIELDS
	  case CLASS_FIELD:
	    next = (Object)DXGetComponentValue((Field)next, "data");
	    if (!next) {
		DXSetError(ERROR_DATA_INVALID, 
			 "no data component found in field to add to list");
		goto error;
	    }
            
	    /* FALL THRU */
#endif
            
	  case CLASS_ARRAY:
	    if (!DXExtractNthString(next, 0, &src)) {
		/* can't be a string */
		if (!isstring)
		    isstring = -1;
		else if (isstring > 0) {
		    DXSetError(ERROR_BAD_PARAMETER, "#10680", 
			       "inputs", 0, onum);
		    goto error;
		}

		alist[ncount++] = (Array)next;
		
	    } else {
		/* must be a string or stringlist */
		for (i=0; DXExtractNthString(next, i, &src); i++) {
		    if (!isstring)
			isstring = 1;
		    else if (isstring < 1) {
			DXSetError(ERROR_BAD_PARAMETER, "#10680", 
				   "inputs", 0, onum);
			goto error;
		    }
		    strlist[strcnt] = src;
		    if (!strlist[strcnt])
			goto error;
		    
		    strcnt++;
                    CHECK_LIMIT();
		}
	    }

	    break;

	  default:
	    DXSetError(ERROR_BAD_PARAMETER, "input %d must be %s", onum,
#if FIELD_SUPPORT
		     "field or array"
#else
                     "an array"
#endif
                     );
            goto error;
	}

    }
    
    
    /* if input was string instead of data array, delete any byte data
     *  arrays which might have been accumulated and make a stringlist instead.
     */
    if (isstring > 0) {

	new = DXMakeStringListV(strcnt, strlist);
	if (!new)
	    goto error;

    } else {
	
        /* find a common format and convert the arrays to it */
        if (DXQueryArrayCommonV(&type, &cat, &rank, shape, ncount, alist) != OK)
            goto error;
        
        new = DXNewArrayV(type, cat, rank, shape);
        if (!new)
            goto error;
        
        
        items = 0;
        for (i=0; i<ncount; i++) {
            
            na = DXArrayConvertV(alist[i], type, cat, rank, shape);
            if (!na)
                goto error;
            
            DXGetArrayInfo(na, &nextitems, NULL, NULL, NULL, NULL);
            src = (char *)DXGetArrayData((Array)na);
            if (!src)
                goto error;
            
            if (!DXAddArrayData(new, items, nextitems, (Pointer)src))
                goto error;
            
            items += nextitems;
            DXDelete((Object)na);
        }
        

    }

    if (!new)
        DXErrorReturn(ERROR_BAD_PARAMETER, "all inputs null");

#if 0
    /* warn if there were null objects in the middle of the list.
     */
    if (warn_null)
	DXWarning("valid object following null object in list");
#endif


    DXFree((Pointer)strlist);
    return new;

  error:
    DXFree((Pointer)strlist);
    DXDelete((Object)new);
    return ERROR;
}
