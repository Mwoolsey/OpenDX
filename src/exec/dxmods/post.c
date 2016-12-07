/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


/*
 * $Header: /src/master/dx/src/exec/dxmods/post.c,v 1.6 2000/12/21 15:21:31 davidt Exp $:
 */

#include <stdio.h>
#include <dx/dx.h>
#include "_post.h"

Error
m_Post(Object *in, Object *out)
{
    Object object;
    char   *str = "positions";

    out[0] = NULL;
    object = NULL;

    if (! in[0])
    {
	DXSetError(ERROR_BAD_PARAMETER, "#10000", "field");
	goto error;
    }

    if (in[1])
    {
	if (! DXExtractString(in[1], &str))
	{
	    DXSetError(ERROR_BAD_PARAMETER, "#10200", "dependency");
	    goto error;
	}
    }

    object = DXCopy(in[0], COPY_STRUCTURE);
    if (! object)
	goto error;

    if (! _dxfPost(object, str, NULL))
	goto error;
    
    out[0] = object;
    return OK;

error:
    if (in[0] != object)
	DXDelete(object);
    out[0] = NULL;
    return ERROR;
}

