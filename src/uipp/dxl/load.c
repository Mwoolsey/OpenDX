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

int
DXLLoadMacroFile(DXLConnection *conn, const char *file)
{
    int l = strlen(file);
    char *buffer = MALLOC(l + 50);
    int sts;

    sprintf(buffer, "load macroFile %s", file);
    sts = DXLSend(conn, buffer);
    FREE(buffer);
    return sts;
}

int
DXLLoadMacroDirectory(DXLConnection *conn, const char *dir)
{
    int l = strlen(dir);
    char *buffer = MALLOC(l + 50);
    int sts;

    sprintf(buffer, "load macroDirectory %s", dir);
    sts = DXLSend(conn, buffer);
    FREE(buffer);
    return sts;
}
