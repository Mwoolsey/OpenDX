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

Error
m_Rename(Object *in, Object *out)
{
    char *old, *new;

    if (!_dxfParmCheck(1, in[0], "input", NULL, NULL,
		       2, in[1], "old component name", 0,
		          in[2], "new component name", 1, 0)) {
	out[0] = NULL;
	return ERROR;
    }

    if (_dxfDXEmptyObject(in[0])) {
	DXWarning("#4000", "input");
	out[0] = in[0];
	return OK;
    }


    DXExtractString(in[1], &old);

    if (!DXExists(in[0], old)) {
        DXSetError(ERROR_MISSING_DATA, "#10252", "input", old);
	out[0] = NULL;
        return ERROR;
    }

    if (in[2])
        DXExtractString(in[2], &new);
    else
        new = "data";

    if (!strcmp(old, new)) {
	DXWarning("#4100", "old", "new component names");
	out[0] = in[0];
	return OK;
    }

    out[0] = DXCopy(in[0], COPY_STRUCTURE);
    if (!out[0])
	return ERROR;


    if (!DXRename(out[0], old, new))
	goto error;
    
    return OK;

  error:
    DXDelete(out[0]);
    out[0] = NULL;
    return ERROR;
}
