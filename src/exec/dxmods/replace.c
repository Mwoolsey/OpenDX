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
m_Replace(Object *in, Object *out)
{
    char *src, *dst;

    out[0] = NULL;

    if (!_dxfParmCheck(2, in[0], "source field", in[1], "destination field",
		       2, in[2], "source component name", 1,
		          in[3], "destination component name", 1, 0))
	return ERROR;


    if (_dxfDXEmptyObject(in[1])) {
	DXWarning("#4000", "destination field");
	out[0] = in[0];
	return OK;
    }


    if (in[2])
	DXExtractString(in[2], &src);
    else
        src = "data";
    
    if (in[3])
        DXExtractString(in[3], &dst);
    else
        dst = "data";


    if (in[0] == in[1] && !strcmp(src, dst)) {
	DXWarning("#4100", "source", "destination");
	out[0] = in[0];
	return OK;
    }

    out[0] = DXCopy(in[1], COPY_STRUCTURE);
    if(!out[0])
	return ERROR;

    /* look out here - the libsvs call parms are: destination, source,
     *  but this module interface is: source, destination.  make sure
     *  you don't get the order mixed up in the following call.
     */
    if (!DXReplace(out[0], in[0], src, dst))
	goto error;
    
    return OK;

  error:
    DXDelete(out[0]);
    out[0] = NULL;
    return ERROR;
}


