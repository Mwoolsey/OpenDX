/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


#include <dx/dx.h>

Error
m_Render(Object *in, Object *out)
{
    char *format = NULL;
    out[0] = NULL;

    if (!in[0]) {
	DXSetError(ERROR_BAD_PARAMETER, "#10000", "object");
	return ERROR;
    }
    
    if (!in[1]) {
	DXSetError(ERROR_BAD_PARAMETER, "#10000", "camera");
	return ERROR;
    }
    
    if (in[2])
	if (!DXExtractString(in[2], &format)) {
	    DXSetError(ERROR_BAD_PARAMETER, "#10200", "format");
	    return ERROR;
	}
    
    out[0] = (Object)DXRender((Object)in[0], (Camera)in[1], format);
	
    if(!out[0])
	    return ERROR;

    return OK;
}
