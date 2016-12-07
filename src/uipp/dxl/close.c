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

int 
uiDXLCloseVPE(DXLConnection *conn)
{
    return DXLSend(conn, "close VPE");
}

int 
uiDXLCloseSequencer(DXLConnection *conn)
{
    return DXLSend(conn, "close sequencer");
}

int 
uiDXLCloseAllColorMapEditors(DXLConnection *conn)
{
    return DXLSend(conn, "close colormapEditor");
}

int 
uiDXLCloseColorMapEditorByLabel(DXLConnection *conn, char *label)
{
    char buf[1024];
    sprintf(buf, "close colormapEditor label=%s", label);
    return DXLSend(conn, buf);
}

int 
uiDXLCloseColorMapEditorByTitle(DXLConnection *conn, char *title)
{
    char buf[1024];
    sprintf(buf, "close colormapEditor title=%s", title);
    return DXLSend(conn, buf);
}

int 
uiDXLCloseAllImages(DXLConnection *conn)
{
    return DXLSend(conn, "close image");
}

int 
uiDXLCloseImageByLabel(DXLConnection *conn, char *label)
{
    char buf[1024];
    sprintf(buf, "close image label=%s", label);
    return DXLSend(conn, buf);
}

int 
uiDXLCloseImageByTitle(DXLConnection *conn, char *title)
{
    char buf[1024];
    sprintf(buf, "close image title=%s", title);
    return DXLSend(conn, buf);
}

