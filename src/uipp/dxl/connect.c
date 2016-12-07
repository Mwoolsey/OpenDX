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

#include "dxlP.h"

DXLError
uiDXLStartServer(DXLConnection *conn)
{
    if (!conn->dxuiConnected)
	return ERROR;
    return DXLSend(conn, "StartServer");
}

DXLError
uiDXLConnectToRunningServer(DXLConnection *conn, const int port)
{
    char buffer[100];

    if (!conn->dxuiConnected)
	return ERROR;

    sprintf(buffer, "ConnectToServer %d", port);
    return DXLSend(conn, buffer);
}
