/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"


#if defined(HAVE_MALLOC_H)
#include <malloc.h>
#endif
#include <string.h>

#include "dxlP.h"

DXLError
uiDXLSaveVisualProgram(DXLConnection *conn, const char *file)
{
    int l = strlen(file);
    int sts;
    char *buffer = MALLOC(l + 32);

    if (conn->dxuiConnected)
    {
	sprintf(buffer, "save network %s", file);
	sts = DXLSend(conn, buffer);
    }
    else
    {
	_DXLError(conn, "saving visual programs  requires a UI connection");
	sts = ERROR;
    }
	
    FREE(buffer);
    return sts;
}
