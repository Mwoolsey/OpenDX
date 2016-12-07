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

static Error checkdim(Object, int *);

int
m_CollectMultiGrid(Object *in, Object *out)
{
    int i;
    int seen_null = 0;
    int say_warn = 0;
    Class class;
    int prev_d = -1;
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

        /* check for mismatched pairs of object/names.
	 * make it ok to leave off the name, but require the object
	 * if a name is given.
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
	    out[0] = (Object)DXNewMultiGrid();
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
	
	/* check name to see if duplicate */
	if (cp && DXGetMember((Group)out[0], cp) != NULL) {
	    DXSetError(ERROR_BAD_PARAMETER, "#11085", i/2, cp);
            goto error;
	}

	/* check member to see if it's field or partitioned field */
	class = DXGetObjectClass(in[i]);
	if (class != CLASS_FIELD && class != CLASS_COMPOSITEFIELD) {
	    if (!cp)
		DXSetError(ERROR_BAD_PARAMETER, "#11087", i/2);
	    else
		DXSetError(ERROR_BAD_PARAMETER, "#11088", i/2, cp);
            goto error;
	}
	/* now check the connections to see if they are the same
         * dimensionality (like cubes & tets, or quads & tris, or lines)
	 */
	if (checkdim(in[i], &prev_d) == ERROR)
	    goto error;

	/* make it part of the new composite field */
	if (!DXSetMember((Group)out[0], cp, in[i]))
            goto error;
	
    }
    
    /* this used to complain about no parms (at least 1 member was
     *  required), but it's been changed to allow a user to create
     *  an empty multigrid.
     */
    if (!out[0]) {
	out[0] = (Object)DXNewMultiGrid();
	/* DXWarning("#4014", "multigrid");   -* empty group */
    }	
    
    if (say_warn)
        ; /* DXWarning("#4010");   -* skipping null objects */
    
    return OK;

  error:
    DXDelete(out[0]);
    out[0] = NULL;
    return ERROR;
}


static Error checkdim(Object in, int *prev_d)
{
    int i = 0;
    int this_d;
    Object newo;
    Array conn;
    char *cp;


    /* if not field, must be partitioned field (already checked it
     * it is one or the other back in the calling routine).
     * if partitioned, get a non-empty member to test.
     */
    if (DXGetObjectClass(in) != CLASS_FIELD) {
	/* loop until we get a non-empty field member.  if all members
	 * are empty, it can't be the wrong dimensionality - return ok
	 */
	for (i=0; ; i++) {
	    newo = DXGetEnumeratedMember((Group)in, i, NULL);
	    if (!newo)
		return OK;

	    if (DXGetObjectClass(newo) != CLASS_FIELD) {
		DXSetError(ERROR_DATA_INVALID, 
			   "composite field members must be fields");
		return ERROR;
	    }

	    if (!DXEmptyField((Field)newo)) {
		in = newo;
		break;
	    }
	}
    }

    
    conn = (Array)DXGetComponentValue((Field)in, "connections");
    
    /* points */
    if (!conn)
	this_d = 0;   
    else {
	
	if (!DXGetStringAttribute((Object)conn, "element type", &cp) || !cp) {
	    DXSetError(ERROR_DATA_INVALID, 
		       "#10255", "connections", "element type");
	    return ERROR;
	}
	
	
	if (!strcmp(cp, "points"))
	    this_d = 0;
	else if (!strcmp(cp, "lines"))
	    this_d = 1;
	else if (!strcmp(cp, "quads"))
	    this_d = 2;
	else if (!strcmp(cp, "triangles"))
	    this_d = 2;	     
	else if (!strcmp(cp, "cubes"))
	    this_d = 3;
	else if (!strcmp(cp, "tetrahedra"))
	    this_d = 3;
	else if (!strncmp(cp, "cubes", 5)) {
	    if(sscanf(cp, "cubes%dD", &this_d) != 1) {
                this_d = -1; /* unknown type */
            }
        }
	
	else
	    this_d = -1;   /* unknown type */
    }

    if (*prev_d == -1)   /* no connections seen yet */
	*prev_d = this_d;
    
    if (*prev_d != this_d) {
	DXSetError(ERROR_DATA_INVALID, "#10420");
	return ERROR;
    }
    
    return OK;
}
