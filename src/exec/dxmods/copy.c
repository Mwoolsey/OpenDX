/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/*
 * (C) COPYRIGHT International Business Machines Corp. 1997
 * All Rights Reserved
 * Licensed Materials - Property of IBM
 * 
 * US Government Users Restricted Rights - Use, duplication or
 * disclosure restricted by GSA ADP Schedule Contract with IBM Corp.
 */

#include <dxconfig.h>


#include <dx/dx.h>
#include "changemember.h"

/* inputs:  object to copy
 */
Error m_CopyContainer(Object *in, Object *out)
{
    enum _dxd_copy how = COPY_ATTRIBUTES;

    if (!in[0]) {
        DXSetError(ERROR_BAD_PARAMETER, "#10000", "input");
        return ERROR;
    }

    out[0] = DXCopy(in[0], how);
    return (out[0] != NULL ? OK : ERROR);
}


#if 0

    /* the code above only allows the user to make a copy of
     * the top level container object, with attributes and
     * type info, but without refs to either components or 
     * members.  the following code is ifdef'ed out but would
     * support the other options at the libdx programming level
     * for the copy function.
     */
    char *cp;
    char *reply = NULL;

    if (in[1]) {
        if (!DXExtractString(in[1], &cp)) {
            DXSetError(ERROR_BAD_PARAMETER, "#xxx", 
              "how must be a string and one of `x' or `y' ");
            return ERROR;
        }

        /* lcase & compress spaces */
        reply = dxf_lsquish(cp);
        if (!reply)
            return ERROR;

        if (!strcmp(reply, "header"))
            how = COPY_HEADER;
        else if (!strcmp(reply, "structure"))
            how = COPY_STRUCTURE;
        else if (!strcmp(reply, "attributes"))
            how = COPY_ATTRIBUTES;
        else {
            DXFree((Pointer)reply);
            DXSetError(ERROR_BAD_PARAMETER, "#xxx",
                "how must be one of `x', `y' ");
            return ERROR;
        }
        DXFree((Pointer)reply);
    }

    out[0] = DXCopy(in[0], how);
    return (out[0] != NULL ? OK : ERROR);
#endif


