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
#include "list.h"

#define MAXOBJECTS 43    /* must match mdf-c generated dx.mdf file */

/* extern, but as far as i know not being used anywhere else */
Group _dxfBuildGroup(Object *in);


int
m_Append(Object *in, Object *out)
{
/* was going to have append accept same inputs as List, but
 * the rest of the inputs are (name,object) pairs, or (value,object)
 * pairs.  that doesn't make sense for list, which is just a 
 * simple list of objects.
 */
#if 0
    Class c;

    if (in[0]) {
	
	c = DXGetObjectClass(in[0]);

	if (c == CLASS_GROUP)
	    out[0] = (Object)_dxfBuildGroup(in);
	
	else if (c == CLASS_ARRAY  ||  c == CLASS_STRING)
	    out[0] = (Object)_dxfBuildList(in);

    } else {

	/* shouldn't this depend on whether the first thing
         * is an array or not?  guess i can't change the existing
	 * default which is to make a new empty group and add the
	 * rest of the inputs as members.
	 */
#endif

	out[0] = (Object)_dxfBuildGroup(in);
#if 0
    }
#endif
    
    return (out[0] != NULL) ? OK : ERROR;
}


Group _dxfBuildGroup(Object *in)
{
    int i;
    int seen_null = 0;
    int say_warn = 0;
    int seen_valid = 0;
    Class gclass = (Class) 0;
    Object newo;
    Group newgroup = NULL;
    char *cp;
    float f;
    int cnt;

    /* this will now do two distinct functions:  
     *  if the first input is a group, it makes all subsequent
     *  inputs a member of the group.  
     *  if the first input is an array or string, it concatinates
     *  all subsequent inputs to the end of the array.
     */

    if (in[0]) {
	
	if (DXGetObjectClass(in[0]) != CLASS_GROUP) {
	    DXSetError(ERROR_BAD_CLASS, "#10192", "input");
	    goto error;
	}
	gclass = DXGetGroupClass((Group)in[0]);
	newgroup = (Group)DXCopy(in[0], COPY_STRUCTURE);
	
    } else {
	newgroup = DXNewGroup();
	if (!newgroup)
	    goto error;

	/* DXWarning("#4018");  -* creating a new group */
    }
    
    /* for each pair of inputs, if they are both null, skip them and
     *  continue.  if one is non-null, return an error.  if both are
     *  good, add them to the group.  look for repeated names.
     */
    for (i = 1; i<MAXOBJECTS; i += 2) {

        /* skip null pairs */
        if (!in[i] && !in[i+1]) {
            seen_null = 1;
            continue;
        }

	if (!seen_valid)
	    seen_valid = 1;

        /* check for mismatched pairs of object/names.
         *  make it ok to have an object and no name, but require an
         *  object if there is a name.  for a series, still require
	 *  the series parm.
         */
        if ((gclass == CLASS_SERIES) && in[i] && !in[i+1]) {
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

	    
	switch (gclass) {

	  case CLASS_SERIES:
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
	    
	    if (!DXGetMemberCount(newgroup, &cnt))
		goto error;
	    
	    if (!DXSetSeriesMember((Series)newgroup, cnt, f, newo))
		goto error;
	    
	    break;
	    
	  default:
	    if (!in[i+1])
		cp = NULL;
	    else {
		if (!DXExtractString(in[i+1], &cp)) {
		    DXSetError(ERROR_BAD_PARAMETER, "#11080", i/2);
		    goto error;
		}
	    }
	    
	    if (cp && DXGetMember(newgroup, cp) != NULL) {
		DXSetError(ERROR_BAD_PARAMETER, "#11085", i/2, cp);
		goto error;
	    }

	    if (!DXSetMember(newgroup, cp, in[i]))
		goto error;
	    
	    break;
	}

    }
    
    if (!seen_valid)
	; /* DXWarning("#4016");  -* no inputs specified; nothing appended */
    
    if (say_warn)
        ; /* DXWarning("#4010");  -* valid object following NULL in list */

    return newgroup;

  error:
    DXDelete((Object)newgroup);
    return NULL;
}

