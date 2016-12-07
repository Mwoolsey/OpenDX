/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/*
 * (C) COPYRIGHT International Business Machines Corp. 1997
 * All Rights Reserved
 * Licensed Materials - Property of IBM
 * 
 * US Government Users Restricted Rights - Use, duplication or
 * disclosure restricted by GSA ADP Schedule Contract with IBM Corp.
 */

#include <dxconfig.h>

#if defined(HAVE_STRING_H)
#include <string.h>
#endif

#include <dx/dx.h>
#include <ctype.h>
#include "changemember.h"

/* if we made this "changemember" it could be
 *  made to work on lists as well as groups.
 *  would be a good compliment to Select().
 *  but we didn't make Append do lists - i made
 *  a List module because one module didn't fit
 *  the input parms exactly. 
 */

/* operations, relative to an existing member */
#define O_UNKNOWN        0
#define O_INSERT         1
#define O_INSERT_BEFORE  2
#define O_REPLACE        3
#define O_INSERT_AFTER   4
#define O_DELETE         5

static Error do_operation(Group newgrp, char *memname, int id, int nmembers, 
			  Class grouptype, int optype, Object newmember, 
			  float newseriespos, char *newmemname, int byname);


/* inputs: existing group, 
 *          operation (insert before, replace, insert after, delete),
 *          existing member id,
 *          newmember, newtag (seriespos or name)
 * outputs: copy of group with changed member
 */
Error m_ChangeGroupMember(Object *in, Object *out)
{
    int nmembers;
    int id;
    char *cp;
    char *opname = NULL;
    int optype = O_UNKNOWN;
    char *memname = NULL;
    char *newmemname = NULL;
    int byname = 0;
    Class grouptype;
    float newseriespos;
    Group newgrp = NULL;
    Object newmember = NULL;



    /* first input must be an existing group.  required. 
     */
    if (!in[0] || DXGetObjectClass(in[0]) != CLASS_GROUP) {
        /* must be a group */
        DXSetError(ERROR_BAD_PARAMETER, "#10192", "input");  
        return ERROR;
    }
    grouptype = DXGetGroupClass((Group)in[0]);
    DXGetMemberCount((Group)in[0], &nmembers);
    
    
    
    /* what to do - insert/replace/delete.  required.
     */
    if (!in[1] || !DXExtractString(in[1], &cp)) {
        DXSetError(ERROR_BAD_PARAMETER, 
		   "`operation' must be specified and one of `insert', `replace', `delete', or `insert before' and `insert after' for series groups'");
        goto error;
    }
    /* lcase string and squish spaces here */
    opname = dxf_lsquish(cp);
    if (!opname)
        goto error;
    
    if (!strcmp(opname, "insert"))
        optype = O_INSERT;
    else if (!strcmp(opname, "insertbefore"))
        optype = O_INSERT_BEFORE;
    else if (!strcmp(opname, "replace"))
        optype = O_REPLACE;
    else if (!strcmp(opname, "insertafter"))
        optype = O_INSERT_AFTER;
    else if (!strcmp(opname, "delete"))
        optype = O_DELETE; 
    else {
        DXSetError(ERROR_BAD_PARAMETER, 
		   "`operation' must be one of `insert', `replace', `delete' or `insert before' or `insert after'");
	DXFree((Pointer)opname);
        goto error;
    }
    DXFree((Pointer)opname);
    

    /* ok, everything from here on down is in most ways dependent on the
     * type of operation requested.
     */


    /* make sure insert before/after only used w/series */
    if ((optype == O_INSERT_BEFORE || optype == O_INSERT_AFTER) &&
	(grouptype != CLASS_SERIES)) {
	DXSetError(ERROR_BAD_PARAMETER,
		   "`operation' cannot be `insert before' or `insert after' unless input is a series group; use `insert' instead");
	goto error;
    }
 

    /* if operation != delete, user has to specify new member.  otherwise
     * they can't specify new member.  i suppose that for replace it can
     * be interpreted that replace with no new member means change the
     * series position or name of specified existing member.
     */
    if (optype == O_DELETE) {
        if (in[3]) {
            DXSetError(ERROR_BAD_PARAMETER, 
		       "cannot specify new member for `delete' operation");
            goto error;
        }
    } else if (optype == O_REPLACE) {
        ; /* don't care whether new member spec'd or not */
    } else {
        if (!in[3]) {
            DXSetError(ERROR_BAD_PARAMETER, "`newmember' object must be specified");
            goto error;
        }
    }
    newmember = in[3];  /* this might be null here and that's ok */


    /* existing member id - by ordinal or name 
     */
    if (!in[2]) {
	if (optype != O_INSERT) {  /* doesn't require an existing id */
	    DXSetError(ERROR_BAD_PARAMETER, 
		       "`id' must be either integer index or string name");
	    goto error;
	}
	id = nmembers-1;   /* set this in case someone cares - add at end */
	
    } else {
	if (!DXExtractInteger(in[2], &id) && !DXExtractString(in[2], &memname)) {
	    DXSetError(ERROR_BAD_PARAMETER, 
		       "`id' must be either integer index or string name");
	    goto error;
	}
	if (optype == O_INSERT) {
	    DXSetError(ERROR_BAD_PARAMETER, "cannot specify `id' parameter for insert");
	    goto error;
	}
    }
    
    if (memname != NULL)
	byname++;
    
    
    /* if by ordinal - check the value is in range */
    if (!byname && (id < 0 || id >= nmembers)) {
	DXSetError(ERROR_BAD_PARAMETER, 
		   "group has %d members, id (%d) must be between 0 and %d",
		   nmembers, id, nmembers-1);
	goto error;
    }
    
    /* if by name, make sure member exists */
    if (byname && !DXGetMember((Group)in[0], memname)) {
	DXSetError(ERROR_BAD_PARAMETER, "#11320", memname);
	/* group has no members with name `%s' */
	goto error;
    }
    
    /* this is a strange combination - cut this off here */
    if (byname && grouptype == CLASS_SERIES) {
	DXSetError(ERROR_DATA_INVALID, "cannot get/set a series member by name");
	goto error;
    }
    
    /* this seems like it will be needed later on - the id even if the member
     * is specified by name.  if byname, look up the member number anyway
     * so "id" has a valid value.  ditto for id - see if there is a name 
     * and/or seriespos.
     */
    if (byname) {
        for (id=0; DXGetEnumeratedMember((Group)in[0], id, &cp); id++) {
            if (!strcmp(cp, memname))
                break;
        }
        if (id == nmembers) {
            /* shouldn't happen - we looked it up by name before and shouldn't
             * have gotten here if we found it before.
             */
            DXSetError(ERROR_INTERNAL, "cannot find member by name");
            goto error;
        }
    } else {
	if (!DXGetEnumeratedMember((Group)in[0], id, &memname)) {
            DXSetError(ERROR_INTERNAL, "cannot find member by id");
	    goto error;
	}
	if (grouptype == CLASS_SERIES) {
	    if (!DXGetSeriesMember((Series)in[0], id, &newseriespos))
		goto error;
	}
   }



	    
    /* newid optional for replace, not permitted for delete.
     * for insert, required for series and optional for all others.
     */
    if (optype == O_DELETE) {
        if (in[4]) {
            DXSetError(ERROR_BAD_PARAMETER, "cannot specify `newtag' for delete operation");
            goto error;
        }
    } else if (optype == O_REPLACE) {
        /* split this into two separate tests for better error msgs */

        if (grouptype == CLASS_SERIES)
	{
	    if (in[4])
	    {
		if (!DXExtractFloat(in[4], &newseriespos)) 
		{
		    DXSetError(ERROR_BAD_PARAMETER,
			       "`newtag' must be a floating point series position");
		    goto error;
		}
	    }
	    else if (! DXGetSeriesMember((Series)in[0], id, &newseriespos))
	    {
	        DXSetError(ERROR_BAD_PARAMETER, "Input has no member %d", id);
		goto error;
	    }
	}
	else
	{
	    if (in[4])
	    {
		if (!DXExtractString(in[4], &newmemname))
		{
		    DXSetError(ERROR_BAD_PARAMETER, "`newtag' must be a string name");
		    goto error;
		}
            }
	    else
	        newmemname = memname;
	}
    
    } else {
        if (grouptype == CLASS_SERIES) {
            if (!in[4] || !DXExtractFloat(in[4], &newseriespos)) {
                DXSetError(ERROR_BAD_PARAMETER,
                   "`newtag' must be a floating point series position");
                goto error;
            }
        } else {
            if (in[4] && !DXExtractString(in[4], &newmemname)) {
                DXSetError(ERROR_BAD_PARAMETER,
                    "`newtag' must be a string name");
                goto error;
            }
        }
    }

 
    /* now be sure that for replace that at least one of newmember or 
     * newtag was set.
     */
    if (optype == O_REPLACE && newmember == NULL && !in[4]) {
        DXSetError(ERROR_BAD_PARAMETER, 
         "`replace' operation requires at least one of `newmember' or `newtag' be specified");
        goto error;
    }
 
    /* make sure that replace of existing member specified by id
     * doesn't have a name collision.  same with insert - verify
     * there isn't already a member with this name.
     */
    if (optype == O_REPLACE && !byname && newmemname) {
	if (DXGetMember((Group)in[0], newmemname) != NULL) {
	    DXSetError(ERROR_BAD_PARAMETER, "group has another member with name '%s' not at index %d", newmemname, id);
	    goto error;
	}
    }
    if (optype == O_INSERT && newmemname) {
	if (DXGetMember((Group)in[0], newmemname) != NULL) {
	    DXSetError(ERROR_BAD_PARAMETER,
		       "group already has a member with name `%s'", newmemname);
	    goto error;
	}
    }


    /* ok, we have all the input parms parsed at this point. */


    /* make copy of top level object, keeping ptrs to members 
     */
    newgrp = (Group)DXCopy(in[0], COPY_HEADER);
    if (!newgrp)
        goto error;
    
    
    if (do_operation(newgrp, memname, id, nmembers, grouptype, optype,
		     newmember, newseriespos, newmemname, byname) == ERROR)
	goto error;
    
    out[0] = (Object)newgrp;
    return OK;

  error:
    DXDelete((Object)newgrp);
    out[0] = NULL;
    return ERROR;
}



static Error do_operation(Group newgrp, char *memname, int id, int nmembers, 
			  Class grouptype, int optype, Object newmember, 
			  float newseriespos, char *newmemname, int byname)
{
    int i;
    char *oldname;
    float seriespos;
    Object subo;

 

    /* do operation specific code */
    switch (optype) {
	
      case O_DELETE:
        if (byname) {
            if (!DXSetMember(newgrp, memname, NULL))  /* does this del? */
                goto error;
        } else {
            if (!DXSetEnumeratedMember(newgrp, id, NULL))  /* ditto */
                goto error;
        }
	break;
	

      case O_INSERT:
	if (grouptype != CLASS_SERIES) {
	    if (!DXSetMember(newgrp, newmemname, newmember))
		goto error;
        } else {   /* series - should this be ok? do an append. */
	    if (!DXSetSeriesMember((Series)newgrp, nmembers, 
				   newseriespos, newmember))
		goto error;
	}
        break;
	
	
      case O_INSERT_BEFORE:   /* already verified input is series */
	for (i=nmembers-1; i >= id; --i) {
	    subo = DXGetSeriesMember((Series)newgrp, i, &seriespos);
	    if (!subo || !DXSetSeriesMember((Series)newgrp, i+1, seriespos, subo))
		goto error;
	}
	if (!DXSetSeriesMember((Series)newgrp, id, newseriespos, newmember))
	    goto error;

        break;


      case O_INSERT_AFTER:   /* already verified input is series */
	for (i=nmembers-1; i > id; --i) {
	    subo = DXGetSeriesMember((Series)newgrp, i, &seriespos);
	    if (!subo || !DXSetSeriesMember((Series)newgrp, i+1, seriespos, subo))
		goto error;
	}
	if (!DXSetSeriesMember((Series)newgrp, id+1, newseriespos, newmember))
	    goto error;

        break;


      case O_REPLACE:
        /* two main cases - a new member is specified (the usual use),
	 * and the new member is not specified (meaning you want to change
	 * either the name or series position tag of an existing member).
         */
 
        if (newmember != NULL) {  /* the "normal" case */
	    
	    /* 4 cases here - byname & byid (!byname) into a series & !series
	     *  except that byname into series doesn't make sense and has
	     *  already been checked for and excluded.
	     */
	    if (!byname && grouptype == CLASS_SERIES) {
		/* replace by id into a series */
		if (!DXSetSeriesMember((Series)newgrp, id, newseriespos, newmember))
		    goto error;
	    } else if (!byname && grouptype != CLASS_SERIES) {
		/* replace by id into non-series */
		/* add new member by name, then del old by id */
		if (!DXSetMember(newgrp, newmemname, newmember))
		    goto error;
		if (!DXSetEnumeratedMember(newgrp, id, NULL))
		    goto error;
	    } else if (byname && grouptype != CLASS_SERIES) {
		/* replace by name into non-series */
		/* add new member by name, then del old by name if !same */
		if (!DXSetMember(newgrp, newmemname, newmember))
		    goto error;
		if (strcmp(memname, newmemname))
		    if (!DXSetMember(newgrp, memname, NULL))
			goto error;
	    } else {
		; /* byname & type == series.  "can't happen" */
	    }
	    
	} else {  /* newmember == NULL; just rename or set new seriespos */

	    /* 4 cases here - byname & byid (!byname) into a series & !series
	     *  except that byname into series doesn't make sense and has
	     *  already been checked for and excluded.
	     */
	    if (!byname && grouptype == CLASS_SERIES) {
		/* change seriespos by id */
		if (!(newmember = DXGetSeriesMember((Series)newgrp, id, &seriespos)))
		    goto error;
		if (!DXSetSeriesMember((Series)newgrp, id, newseriespos, newmember))
		    goto error;
		
	    } else if (!byname && grouptype != CLASS_SERIES) {
		/* change name by id into non-series */
		/* get old member by id, add new member by name, 
		 *  then del old by id 
		 */
		if (!(newmember = DXGetEnumeratedMember(newgrp, id, &oldname)))
		    goto error;
		if (!DXSetMember(newgrp, newmemname, newmember))
		    goto error;
		if (!DXSetEnumeratedMember(newgrp, id, NULL))
		    goto error;

	    } else if (byname && grouptype != CLASS_SERIES) {
		/* change by name into non-series */
		/* get the member, set it under the new name, and then
		 * del it with the old name.  don't do it in a different
		 * order or you might end up losing it if the ref count
		 * goes to 0.
		 */
                if (!(newmember = DXGetMember(newgrp, memname)))
		    goto error;
		if (!DXSetMember(newgrp, newmemname, newmember))
		    goto error;
		if (!DXSetMember(newgrp, memname, NULL))
		    goto error;
		
	    } else {
		; /* byname & type == series.  "can't happen" */
	    }
	}
        break;
    }
    
    return OK;

  error:
    return ERROR;
}


/* this routines allocates new space and makes a copy of the
 * input string.  the caller has to free the space when done.
 * all chars are converted to lower case and blanks are removed.
 */
char *dxf_lsquish(char *instr)
{
    char *outstr = NULL;
    char *cpi, *cpo;

    outstr = (char *)DXAllocate(strlen(instr) + 1);
    if (!outstr)
        return NULL;

    for (cpi=instr, cpo=outstr; *cpi; cpi++) {

	if (isspace(*cpi))
	    continue;

	if (isupper(*cpi))
	    *cpo++ = tolower(*cpi);
	else
	    *cpo++ = *cpi;
    }
    
    *cpo = *cpi;
    return outstr;
}
