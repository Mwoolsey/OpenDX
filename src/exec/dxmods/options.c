/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


/*
 * $Header: /src/master/dx/src/exec/dxmods/options.c,v 1.4 2000/08/24 20:04:41 davidt Exp $
 */


#include <dx/dx.h>

#define PAIRS 20

extern Array _dxfReallyCopyArray(Array a); /* from libdx/component.c */

Error
m_Options(Object *in, Object *out)
{
    Object a, v;
    char *s;
    int i;

    /* check input object */
    if (!in[0]) {
	DXSetError(ERROR_BAD_PARAMETER, "#10000", "input");
	return ERROR;
    }
   
    /* create output copy */
    out[0] = DXCopy(in[0], COPY_HEADER);
    if (!out[0])
	return ERROR;

    /* arrays don't copy by default - you have to call something special
     *  to get a new copy.
     */
    if (out[0] == in[0] && DXGetObjectClass(in[0]) == CLASS_ARRAY) {
	out[0] = (Object)_dxfReallyCopyArray((Array)in[0]);
	if (!out[0])
	    return ERROR;
    }

    /* set attributes */
    for (i=0; i<PAIRS; i++) {
	a = in[2*i+1];
	v = in[2*i+2];
	if (a) {
	    if (!DXExtractString(a, &s)) {
		DXSetError(ERROR_BAD_PARAMETER, "#10200", "attribute name");
		goto error;
	    }
	    /* NB - ok for v to be NULL - this deletes the attribute */
	    if (!DXSetAttribute(out[0], s, v)) 
		goto error;
	}
    }
    
    return OK;

error:
    if (out[0] != in[0])
	DXDelete(out[0]);
    out[0] = NULL;
    return ERROR;
}

