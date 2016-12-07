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

extern Object _dxfDXEmptyObject(Object o); /* from libdx/component.c */


int
m_Unmark(Object *in, Object *out)
{
    char   *target;
    Object attr;

    if (!_dxfParmCheck(1, in[0], "input", NULL, NULL,
		       1, in[1], "component name", 1,
		          NULL, NULL, 0, 0)) {
	out[0] = NULL;
	return ERROR;
    }

    if (_dxfDXEmptyObject(in[0])) {
	DXWarning("#4000", "input");
	out[0] = in[0];
	return OK;
    }

    if (in[1])
	attr = in[1];
    else 
	attr = DXGetAttribute(in[0], "marked component");
    
    if (!attr || !DXExtractString(attr, &target))
    {
	DXSetError(ERROR_BAD_PARAMETER, "component name");
	return ERROR;
    }

    /* the only way it makes sense to unmark a field which doesn't have
     *  a data component is if the component you are unmarking is 'data'.
     *  (that is interpreted as a request to move the 'saved data' component
     *  back into 'data'.)
     */
    if (!DXExists(in[0], "data") && strcmp(target, "data")) {
        DXSetError(ERROR_MISSING_DATA, "#10252", "input", "data");
	return ERROR;
    }


    out[0] = DXCopy(in[0], COPY_STRUCTURE);
    if (!out[0])
	return ERROR;


    /* special case unmarking the 'data' component - ignore the existing
     *  'data' component and overwrite it with the 'saved data' component.
     *  if no saved data, just leave the data component alone.
     */
    if (!strcmp(target, "data")) {
        if (!DXExists(out[0], "saved data")) {
            DXWarning("#10250", "input", "`saved data'");
            return OK;
        }

        if (!DXRename(out[0], "saved data", "data")) {
	    DXAddMessage("while trying to create output field");
	    goto error;
	}

        return OK;
    }

     
    /* normal case.  put data back to its original name, and then check for
     *  saved data and put it back to data.
     */
    if (!DXRename(out[0], "data", target)) {
	DXAddMessage("while trying to create output field");
	goto error;
    }


    /* if 'saved data' exists, move it back into 'data'
     */
    if (DXExists(out[0], "saved data") &&
	!DXRename(out[0], "saved data", "data")) {
	    DXAddMessage("while trying to create output field");
	    goto error;
	}
    
    if (DXGetAttribute(out[0], "marked component"))
	DXSetAttribute(out[0], "marked component", NULL);

    return OK;

  error:
    DXDelete(out[0]);
    out[0] = NULL;
    return ERROR;
}
