/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


#include <dx/dx.h>
#include "echo.h"

/*
 * This module sends a message across the connection that is intended for
 * a DXLink application (i.e one that has linked with libdxl.a).
 */

#define TAG in[0]
#define VALUE in[1]

Error m_DXLOutput(Object in[], Object out[])
{
    char *tag, *value;

    if (!TAG) {
        DXSetError(ERROR_BAD_PARAMETER,"#10000","tag");
        goto error; 
    } else if (!DXExtractString(TAG,&tag)) {
        DXSetError(ERROR_BAD_PARAMETER,"#10200","tag");
        goto error; 
    }

    if (!VALUE) {
        value = "";
    } else if (!_dxf_ConvertObjectsToStringValues(&VALUE,1,&value,1)) {
        DXSetError(ERROR_BAD_PARAMETER,"#10200","value");
        goto error; 
    }
    

    DXUIMessage("LINK","DXLOutput VALUE %s %s",tag,value);
    if (VALUE)
	DXFree((Pointer)value);

    return OK;

error:
    return ERROR;

}

Error m_DXLOutputNamed(Object in[],  Object out[])
{
    return m_DXLOutput(in, out);
}
