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

#define MAXOBJECTS 42    /* must match mdf-c generated dx.mdf file */


int
m_CollectNamed(Object *in, Object *out)
{
    int i;
    int seen_null = 0;
    int say_warn = 0;
    char *cp;

    out[0] = NULL;

    /* for each pair of inputs, if they are both null, skip them and
     *  continue.  if one is non-null, return an error.  if both are
     *  good, add them to the group.  look for repeated names.
     */
    for (i = 0; i<MAXOBJECTS; i += 2) {

        /* skip null pairs */
        if (!in[i] && !in[i+1]) {
            seen_null = 1;
            continue;
        }

        /* check for mismatched pairs of object/names.  allow the name
	 *  to be null but if the name is given, require the object.
         */

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
	    out[0] = (Object)DXNewGroup();
            if (!out[0])
                return ERROR;
        }

	if (!in[i+1])
	    cp = NULL;
	else {
	    if (!DXExtractString(in[i+1], &cp)) {
		DXSetError(ERROR_BAD_PARAMETER, "#11080", i/2);
		goto error;
	    }
	}

	if (cp && DXGetMember((Group)out[0], cp) != NULL) {
	    DXSetError(ERROR_BAD_PARAMETER, "#11085", i/2, cp);
            goto error;
	}
	
	if (!DXSetMember((Group)out[0], cp, in[i]))
            goto error;
	
    }
    
    /* allow no parms, and make a generic empty group.  this is different
     *  from before when at least one object was required.
     */
    if (!out[0]) {
	out[0] = (Object)DXNewGroup();
	/* DXWarning("#4014", "group");   -* making empty group */
    }

    if (say_warn)
        ;  /* DXWarning("#4010");    -* skipping invalid object */

    return OK;

  error:
    DXDelete(out[0]);
    out[0] = NULL;
    return ERROR;
}

