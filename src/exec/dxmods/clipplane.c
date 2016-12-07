/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


/*
 * $Header: /src/master/dx/src/exec/dxmods/clipplane.c,v 1.3 1999/05/10 15:45:22 gda Exp $
 */


#include <string.h>
#include <dx/dx.h>

Error
m_ClipPlane(Object *in, Object *out)
{
    Point p;
    Vector n;

    if (!in[0]) {
	DXSetError(ERROR_BAD_PARAMETER, "#10000", "input");
	return ERROR;
    }

    if (!in[1]) {
	Point box[8];
	if (!DXBoundingBox(in[0],box)) {
	    if (DXGetError()!=ERROR_NONE) {
		DXAddMessage("#10460", "point");
		return ERROR;
	    } else {
		out[0] = in[0];
		return OK;
	    }
	}
	p = DXDiv(DXAdd(box[0],box[7]), 2.0);
    } 
    else if (!DXExtractParameter(in[1], TYPE_FLOAT, 3, 1, (Pointer)&p)) {
	DXSetError(ERROR_BAD_PARAMETER, "#10460", "point");
	return ERROR;
    }

    n = DXVec(0,0,1);
    if (in[2] && !DXExtractParameter(in[2], TYPE_FLOAT, 3, 1, (Pointer)&n)) {
	DXSetError(ERROR_BAD_PARAMETER, "#10230", "normal", 3);
	return ERROR;
    }

    out[0] = DXClipPlane(in[0], p, n);
    if(!out[0])
	return ERROR;

    return OK;
}
