/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/*
 * Header: /usr/people/gresh/code/svs/src/dxmods/RCS/attribute.m,v 5.0 92/11/12 09:14:40 svs Exp Locker: gresh 
 * 
 */

#include <dxconfig.h>



#include <string.h>
#include <dx/dx.h>


Error m_Attribute(Object *in, Object *out)
{
    Object s;
    char *attrname;

    /* 
     *  input object is required
     */
    if (in[0] == NULL) {
	DXSetError(ERROR_BAD_PARAMETER, "#10000", "input");
        return ERROR;
    }

    /* 
     * get attribute name to return.
     */
    if (!in[1])
        attrname = "name";
    else {
        if (!DXExtractString(in[1], &attrname)) {
            DXSetError(ERROR_BAD_PARAMETER, "#10200", "name");
            return ERROR;
        }
    }

    /* 
     * check input object for that attribute.
     */
    s = DXGetAttribute(in[0], attrname);
    if (s == NULL) {
	DXSetError(ERROR_DATA_INVALID, "#10257", "input", attrname);
        return ERROR;
    }

    out[0] = s;
    return OK;
}
