/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
#include <dx/dx.h>

#include <dxconfig.h>


#define WHERE in[0]

extern Field _dxfSaveSoftwareWindow(char *); /* from libdx/displayx.c */
extern Field _dxfSaveHardwareWindow(char *); /* from hwrender/hwRender.c */

Error
m_ReadImageWindow(Object *in, Object *out)
{
    Object image;
    char *where;

    out[0] = NULL;

    if (! WHERE)
    {
	DXSetError(ERROR_MISSING_DATA, "where");
	return ERROR;
    }

    if (! DXExtractString(WHERE, &where))
    {
	DXSetError(ERROR_DATA_INVALID, "where");
	return ERROR;
    }

    image = (Object)_dxfSaveSoftwareWindow(where);

#if DXD_CAN_HAVE_HW_RENDERING
    if (! image)
    {
	DXResetError();
	image = (Object)_dxfSaveHardwareWindow(where);
    }
#endif
    
    if (! image)
	return ERROR;
    
    out[0] = (Object)image;
    return OK;
}

