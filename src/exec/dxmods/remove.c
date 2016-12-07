/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


#include <dx/dx.h>
#include "mark.h"

extern Object _dxfDXEmptyObject(Object o); /* from libdx/component.c */


Error
m_Remove(Object *in, Object *out)
{
    char *old;
    int compname = 0;

    out[0] = NULL;

    if (!_dxfParmCheck(1, in[0], "input", NULL, NULL,
		       1, in[1], "component name", 0,
		          NULL,   NULL,            0, 1))
	return ERROR;


    if (_dxfDXEmptyObject(in[0])) {
	DXWarning("#4000", "input");
	out[0] = in[0];
	return OK;
    }


    /* for each component to remove... 
     */
    for (compname = 0; DXExtractNthString(in[1], compname, &old); compname++) {

	if (!DXExists(in[0], old)) {
	    DXWarning("#10252", "input", old);
	    continue;
	}
	
	if (!out[0]) {
	    out[0] = DXCopy(in[0], COPY_STRUCTURE);
	    if (!out[0])
		return ERROR;
	}

	if (!DXRemove(out[0], old))
	    goto error;

    }

    /* nothing to remove; return input unchanged */
    if (!out[0])
	out[0] = in[0];

    return OK;

  error:
    DXDelete(out[0]);
    out[0] = NULL;
    return ERROR;    
}
