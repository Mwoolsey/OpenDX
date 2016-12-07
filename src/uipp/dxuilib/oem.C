/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"
#include "../base/DXStrings.h"
#include "../base/ErrorDialogManager.h"
#include "oem.h"

char *ScrambleString(const char *str, const char *hash)
{
    ErrorMessage("Net file encryption algorithms have been removed from OpenDX");
    abort();
    return NULL;
}

char *ScrambleAndEncrypt(const char *src, const char *hash, char *cryptbuf)
{
    ErrorMessage("Net file encryption algorithms have been removed from OpenDX");
    abort();
    return NULL;
}
