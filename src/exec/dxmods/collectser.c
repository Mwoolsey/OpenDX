/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


#include <string.h>
#include <dx/dx.h>
#include "_construct.h"

#define MAXOBJECTS 44    /* must match mdf-c generated dx.mdf file */

int
m_CollectSeries(Object *in, Object *out)
{
    int i, m;
    int seen_null = 0;
    int say_warn = 0;
    float f, *fp;
    int num = 0;
    Object newo = NULL;


    out[0] = NULL;

    /* check for a special case.  if the first input pair contains a group
     * and a scalar list, and if the number of group members is the
     * same as the number of items in the list, then create a series from
     * the group members using each item in the list as the series position.
     */

    if (in[0] && in[1]) {
        if (DXGetObjectClass(in[0]) != CLASS_GROUP)
            goto notspecial;

        if (!DXQueryParameter(in[1], TYPE_FLOAT, 1, &i))
            goto notspecial;

        if (i == 1)
            goto notspecial;
        
        DXGetMemberCount((Group)in[0], &m);
        if (i != m)
            goto notspecial;

        /* ok, does seem to be special */
        fp = (float *)DXAllocate(m * sizeof(float));
        if (!fp)
            return ERROR;

        if (!DXExtractParameter(in[1], TYPE_FLOAT, 1, m, (Pointer)fp)) {
            DXFree((Pointer)fp);
            return ERROR;
        }

        out[0] = (Object)DXNewSeries();
        if (!out[0]) {
            DXFree((Pointer)fp);
            return ERROR;
        }

        for (i=0; (newo = DXGetEnumeratedMember((Group)in[0], i, NULL)); i++) {
            if (!DXSetSeriesMember((Series)out[0], i, fp[i], newo)) {
                DXDelete(out[0]);
                out[0] = NULL;
                DXFree((Pointer)fp);
                return ERROR;
            }
        }

        DXFree((Pointer)fp);
        return OK;
    }
    

    /* ok, not that case.  continue with the original code */
  notspecial:

    /* for each pair of inputs, if they are both null, skip them and
     *  continue.  if one is non-null, return an error.  if both are
     *  good, add them to the series.
     */
    for (i = 0; i<MAXOBJECTS; i += 2) {

        /* skip null pairs */
        if (!in[i] && !in[i+1]) {
            seen_null = 1;
            continue;
        }

        /* check for mismatched pairs of object/names
         */
        if (in[i] && !in[i+1]) {
            DXSetError(ERROR_BAD_PARAMETER, "#11070", i/2);
            goto error;
        }

        if (!in[i] && in[i+1]) {
            DXSetError(ERROR_BAD_PARAMETER, "#11065", i/2);
            goto error;
        }


        /* warn if you see valid pairs after null pairs, but it's not
         *  an error.
         */
        if (seen_null) {
            say_warn = 1;
            seen_null = 0;
        }

        /* first non-null object you see, create output group.
         */
        if (!out[0]) {
	    out[0] = (Object)DXNewSeries();
	    if (!out[0])
		return ERROR;
        }


	if (!DXExtractFloat(in[i+1], &f)) {
	    DXSetError(ERROR_BAD_PARAMETER, "#11070", i/2);
            goto error;
	}

        /* this has to be a copy because DXSetSeriesMember adds a series
         *  position attribute, so series members can't be shared between
         *  series groups if they have different positions.
         */
	if (!(newo = DXCopy(in[i], COPY_STRUCTURE)))
	    goto error;
	
	/* if it didn't copy and it's an array, try harder to copy it */
	if ((newo == in[i]) && (DXGetObjectClass(in[i]) == CLASS_ARRAY))
	    newo = (Object)_dxfReallyCopyArray((Array)in[i]);
	
	if (!DXSetSeriesMember((Series)out[0], num, f, newo))
            goto error;
	
	num++;
    }
    
    /* this used to complain about no parms (at least 1 member was
     *  required), but it's been changed to allow a user to create
     *  an empty series.
     */
    if (!out[0]) {
	out[0] = (Object)DXNewSeries();
	/* DXWarning("#4014", "series");    -* creating empty series */
    }
    
    if (say_warn)
	;  /* DXWarning("#4010");  -* skipping nulls in list */
    
    return OK;

  error:
    DXDelete(out[0]);
    out[0] = NULL;
    return ERROR;
}

