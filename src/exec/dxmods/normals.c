/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


#include <stdio.h>
#include <math.h>
#include <dx/dx.h>
#include "_normals.h"

int
m_Normals (Object *in, Object *out)
{
    char		*method;

    out[0] = NULL;

    if (in[0] == NULL)
    {
	DXSetError(ERROR_BAD_PARAMETER, "#10000", "input");
	goto error;
    }

    if (in[1])
    {
	if (! DXExtractString(in[1], &method))
	{
	    DXSetError(ERROR_BAD_PARAMETER, "#10200", "method");
	    goto error;
	}
    }
    else
	method = "positions";

    out[0] = _dxfNormals(in[0], method);

    if (out[0] == NULL || DXGetError() != ERROR_NONE)
	goto error;
    
    return OK;

error:
    DXDelete(out[0]);
    return ERROR;
}
