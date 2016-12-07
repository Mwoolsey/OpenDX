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
#include <string.h>
#include <dx/dx.h>
#include "_normals.h"

int
m_FaceNormals (Object *in, Object *out)
{
    out[0] = NULL;

    if (in[0] == NULL)
    {
	DXSetError(ERROR_BAD_PARAMETER, "#10000", "input");
	goto error;
    }

    out[0] = _dxfNormals(in[0], "connections");

    if (out[0] == NULL || DXGetError() != ERROR_NONE)
	goto error;
    
    return OK;

error:
    DXDelete(out[0]);
    return ERROR;
}
