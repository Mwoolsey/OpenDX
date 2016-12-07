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
#include "mark.h"


/* specials: these don't call DXEndField */
extern Object _dxf_RenameX(Object o, char *old, char *new); /* from libdx/component.c */
extern Object _dxf_ReplaceX(Object o, Object add, char *from, char *to); /* from libdx/component.c */
extern Object _dxfDXEmptyObject(Object o); /* from libdx/component.c */

Error
m_Mark(Object *in, Object *out)
{
    char *target;

    if (!_dxfParmCheck(1, in[0], "input", NULL, NULL,
		       1, in[1], "component name", 0,
		          NULL, NULL, 0, 0)) {
	out[0] = NULL;
	return ERROR;
    }

    if (_dxfDXEmptyObject(in[0])) {
	DXWarning("#4000", "input");
	out[0] = in[0];
	return OK;
    }

    DXExtractString(in[1], &target);
    if (!DXExists(in[0], target)) {
        DXSetError(ERROR_MISSING_DATA, "#10252", "input", target);
        return ERROR;
    }
    
    if (DXExists(in[0], "data") && DXExists(in[0], "saved data")) {
        DXSetError(ERROR_DATA_INVALID, "#11260", "saved data");
	return ERROR;
    }

    
    out[0] = DXCopy(in[0], COPY_STRUCTURE);
    if (!out[0])
	return ERROR;
    
    
    /* special case - if marking the 'data' component, use replace instead
     *  of rename, because you are just duplicating the data component in
     *  saved data.
     */
    if (!strcmp(target, "data")) {
	if (!_dxf_ReplaceX(out[0], in[0], "data", "saved data"))
	    goto error;
	
        return OK;
    }

     
    /* if 'data' exists, first move it into 'saved data'
     *  --- needs Exists2(out[0], "data", target); here ---
     */
    if (DXExists(out[0], "data") && 
	! _dxf_RenameX(out[0], "data", "saved data"))
	    goto error;

     
    if (!_dxf_ReplaceX(out[0], out[0], target, "data"))
	goto error;
    

    if (! DXSetAttribute(out[0], "marked component", in[1]))
	goto error;


    return OK;

  error:
    DXDelete(out[0]);
    out[0] = NULL;
    return ERROR;
    
}



Error _dxfParmCheck(int nobjs,  Object o1, char *oname1,
		                Object o2, char *oname2,
		    int nparms, Object p1, char *name1, int nullok1,
		                Object p2, char *name2, int nullok2,
		    int parmlist)
{
    /* input object tests.
     */
    if (!o1) {
	DXSetError(ERROR_BAD_PARAMETER, "#10000", oname1);
	return ERROR;
    }
    
    if (nobjs > 1) {
	if (!o2) {
	    DXSetError(ERROR_BAD_PARAMETER, "#10000", oname2);
	    return ERROR;
	}
    }
    
    /* component name tests
     */
    if (!p1 && !nullok1) {
	DXSetError(ERROR_BAD_PARAMETER, "#10200", name1);
	return ERROR;
    }
    if (p1) {
	if (parmlist) {
	    if (!DXExtractNthString(p1, 0, NULL)) {
		DXSetError(ERROR_BAD_PARAMETER, "#10201", name1);
		return ERROR;
	    }
	} else {
	    if (!DXExtractString(p1, NULL)) {
		DXSetError(ERROR_BAD_PARAMETER, "#10200", name1);
		return ERROR;
	    }
	}
    }
    
    if (nparms > 1) {
	if (!p2 && !nullok2) {
	    DXSetError(ERROR_BAD_PARAMETER, "#10200", name2);
	    return ERROR;
	}
	if (p2) {
	    if (parmlist) {
		if (!DXExtractNthString(p2, 0, NULL)) {
		    DXSetError(ERROR_BAD_PARAMETER, "#10201", name2);
		    return ERROR;
		}
	    } else {
		if (!DXExtractString(p2, NULL)) {
		    DXSetError(ERROR_BAD_PARAMETER, "#10200", name2);
		    return ERROR;
		}
	    }
	}
    }
    
    return OK;
}

