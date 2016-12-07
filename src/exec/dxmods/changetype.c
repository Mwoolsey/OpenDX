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
#include "changemember.h"


/* inputs: existing group, new group type (series, multigrid, etc) and
 *          a list of group ids -- series positions if series, names (optional)
 *          if nonseries group.
 * outputs: new group
 */
Error m_ChangeGroupType(Object *in, Object *out)
{
    int i, nmembers;
    int islist = 0;
    char *grpname;
    Class oldgrouptype, newgrouptype;
    float seriespos, *seriesposlist = NULL;
    char *cp, **namelist = NULL;
    Group newgrp = NULL;
    Object member;
 
    /* existing group */
    if (!in[0] || DXGetObjectClass(in[0]) != CLASS_GROUP) {
        DXSetError(ERROR_BAD_PARAMETER, "#10192", "input");
        return ERROR;
    }
    oldgrouptype = DXGetGroupClass((Group)in[0]);
    DXGetMemberCount((Group)in[0], &nmembers);
    
    /* new group type */
    if (!in[1] || !DXExtractString(in[1], &cp)) {
        DXSetError(ERROR_BAD_PARAMETER, 
	  "`type' must be specified and one of `series', `multigrid' or `generic'");

        return ERROR;
    }
    /* lcase string and squish spaces here */
    grpname = dxf_lsquish(cp);
    if (!grpname)
        return ERROR;

    if (!strcmp(grpname, "series"))
        newgrouptype = CLASS_SERIES;
    else if (!strcmp(grpname, "multigrid"))
        newgrouptype = CLASS_MULTIGRID;
    else if (!strcmp(grpname, "compositefield"))
        newgrouptype = CLASS_COMPOSITEFIELD;
    else if (!strcmp(grpname, "generic"))
        newgrouptype = CLASS_GROUP;
    else {
        DXFree((Pointer)grpname);
        DXSetError(ERROR_BAD_PARAMETER, 
	 "`type' must be a string and one of `series', `multigrid' or `generic'");
        return ERROR;
    }
    DXFree((Pointer)grpname);

    /* optional; ids of new group members.  seriespos for series, 
     *   names for all others.
     */
    if (in[2]) {
        if (nmembers <= 0)
           /* warn here no ids will be handled? */ 
            ;

        switch (newgrouptype) {
        case CLASS_SERIES:
            /* verify id list is float (or can be coerced to float) */
            seriesposlist = (float *)DXAllocate(sizeof(float) * nmembers);
            if (!seriesposlist)
                goto error;
           
            if (!DXExtractParameter(in[2], TYPE_FLOAT, 0, nmembers, seriesposlist)) {
                DXSetError(ERROR_BAD_PARAMETER,
                  "`newtags' must be a list of %d scalars for a series group",
                    nmembers);
                goto error;
            }
            break;
        
        default: 
            /* verify id list is string */
            namelist = (char **)DXAllocate(sizeof(char *) * nmembers);
            if (!namelist)
                goto error;
 
	    for (i=0; DXExtractNthString(in[2], i, &namelist[i]); i++)
		;
	    
	    if (i != nmembers) {
                DXSetError(ERROR_BAD_PARAMETER,
			   "`newtags' must be a list of %d string names", nmembers);
                goto error;
            }
            break;
        } 
        islist++;
    } 

    /* if type is already OK, and no ids were given, we have nothing to do.
     * return the input unchanged.
     */
    if ((oldgrouptype == newgrouptype) && !in[2]) {
	newgrp = (Group)in[0];
	goto done;
    }


    /* make new group header or else make copy of header, inc attrs */
    if (oldgrouptype == newgrouptype) {
        newgrp = (Group)DXCopy(in[0], COPY_ATTRIBUTES);
        if (!newgrp)
            goto error;

     } else {
        switch (newgrouptype) {
        case CLASS_SERIES:
           newgrp = (Group)DXNewSeries();
           break;
        case CLASS_MULTIGRID:
           newgrp = (Group)DXNewMultiGrid();
           break;
        case CLASS_COMPOSITEFIELD:
           newgrp = (Group)DXNewCompositeField();
           break;
        case CLASS_GROUP:
           newgrp = (Group)DXNewGroup();
           break;
        default: 
          DXSetError(ERROR_DATA_INVALID, "Unrecognized group type");
          goto error;
          
        } 
        if (!newgrp)
            goto error;

        /* copy all attributes from old group to new group */
        if (!DXCopyAttributes((Object)newgrp, in[0]))
           goto error;
     } 

     /* have new header.  copy members from old to new */
     switch (newgrouptype) {
     case CLASS_SERIES:
        for (i=0; (member = DXGetSeriesMember((Series)in[0], i, &seriespos)); i++ ) {
           if (islist) {
              if (!DXSetSeriesMember((Series)newgrp, i, seriesposlist[i], member))
		  goto error;
           } else {
              if (!DXSetSeriesMember((Series)newgrp, i, seriespos, member))
		  goto error;
	  }
        }
        break;
     default: 
        for (i=0; (member = DXGetEnumeratedMember((Group)in[0], i, &cp)); i++ ) {
           if (islist) {
              if (!DXSetMember((Group)newgrp, namelist[i], member))
		  goto error;
           } else {
              if (!DXSetMember((Group)newgrp, cp, member))
		  goto error;
	  }
        }
        break;
     }

  done:
    out[0] = (Object)newgrp;
    return OK;

  error:
    DXDelete((Object)newgrp);
    DXFree((Pointer)seriesposlist);
    DXFree((Pointer)namelist);
    out[0] = NULL;
    return ERROR;
}
