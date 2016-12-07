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
uiDXLOpenVPE(DXLConnection *conn)
{
    if (!conn->dxuiConnected)
        return ERROR;
    return DXLSend(conn, "open VPE");
}

DXLError
uiDXLOpenAllColorMapEditors(DXLConnection *conn)
{
    if (!conn->dxuiConnected)
        return ERROR;
    return DXLSend(conn, "open colormapEditor");
}

DXLError
uiDXLOpenColorMapEditorByLabel(DXLConnection *conn, char *label)
{
    char buf[1024];
    if (!conn->dxuiConnected)
        return ERROR;
    sprintf(buf, "open colormapEditor label=%s", label);
    return DXLSend(conn, buf);
}

DXLError
uiDXLOpenColorMapEditorByTitle(DXLConnection *conn, char *title)
{
    char buf[1024];
    if (!conn->dxuiConnected)
        return ERROR;
    sprintf(buf, "open colormapEditor title=%s", title);
    return DXLSend(conn, buf);
}

DXLError
uiDXLOpenControlPanelByName(DXLConnection *conn, char *name)
{
    char buf[1024];
    if (!conn->dxuiConnected)
        return ERROR;
    sprintf(buf, "open controlpanel title=%s", name);
    return DXLSend(conn, buf);
}

DXLError
uiDXLOpenSequencer(DXLConnection *conn)
{
    if (!conn->dxuiConnected)
        return ERROR;
    return DXLSend(conn, "open sequencer");
}

DXLError
uiDXLOpenAllImages(DXLConnection *conn)
{
    if (!conn->dxuiConnected)
        return ERROR;
    return DXLSend(conn, "open image");
}

DXLError
uiDXLOpenImageByLabel(DXLConnection *conn, char *label)
{
    char buf[1024];
    if (!conn->dxuiConnected)
        return ERROR;
    sprintf(buf, "open image label=%s", label);
    return DXLSend(conn, buf);
}

DXLError
uiDXLOpenImageByTitle(DXLConnection *conn, char *title)
{
    char buf[1024];
    sprintf(buf, "open image title=%s", title);
    if (!conn->dxuiConnected)
        return ERROR;
    return DXLSend(conn, buf);
}

DXLError
uiDXLSetRenderMode(DXLConnection *conn, char *title, int rendermode)
{
    char buf[1024];
    char mode[3];

    if (!conn->dxuiConnected)
        return ERROR;

    if (rendermode==0) {
      strcpy(mode, "sw");
    } else {
      strcpy(mode, "hw");
    } /* endif */

    sprintf(buf, "render-mode %s title=%s",mode,title);
    printf("%s\n",buf);
    return DXLSend(conn, buf);
}
