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
#include "mark.h"

extern Object _dxfDXEmptyObject(Object o); /* from libdx/component.c */


int
m_Extract(Object *in, Object *out)
{
    char *name;

    if (!_dxfParmCheck(1, in[0], "input", NULL, NULL,
		       1, in[1], "component name", 1,
		          NULL,   NULL,            0, 0)) {
	out[0] = NULL;
	return ERROR;
    }

    if (_dxfDXEmptyObject(in[0])) {
	DXWarning("#4000", "input");
	out[0] = in[0];
	return OK;
    }

    
    if(in[1]) 
	DXExtractString(in[1], &name);
    else
	name = "data";
    

    if (!DXExists(in[0], name)) {
	out[0] = NULL;
        DXSetError(ERROR_MISSING_DATA, "#10252", "input", name);
        return ERROR;
    }


    /* extract may return a new object (if a simple field is a passed
     * in, an array is returned.)  the extract routine takes care of
     * deleting the original if necessary.
     */
    out[0] = DXCopy(in[0], COPY_STRUCTURE);
    if(!out[0])
	return ERROR;
    
    if (!(out[0] = DXExtract(out[0], name))) {
	DXDelete(out[0]);
	out[0] = NULL;
	return ERROR;
    }

    return OK;
}
