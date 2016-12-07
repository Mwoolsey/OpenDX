/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


/*
 * $Header: /src/master/dx/src/exec/dxmods/clipbox.c,v 1.3 1999/05/10 15:45:22 gda Exp $
 */


#include <string.h>
#include <dx/dx.h>

Error
m_ClipBox(Object *in, Object *out)
{
    Point box[8], min, max;
    struct { float x, y; } b2[2];
    float b1[2];

    if(!in[0]) {
	DXSetError(ERROR_BAD_PARAMETER, "#10000","object");
        return ERROR;
    }

    /* list of two points, or object bounds */
    if (!in[1]) {
        /* default is no clipping */
        out[0] = in[0];
        return OK;
    }


    if (DXGetObjectClass(in[1])==CLASS_ARRAY) {
	if (DXExtractParameter(in[1], TYPE_FLOAT, 3, 2, (Pointer)box)) {
	    min = box[0];
	    max = box[1];
	} else if (DXExtractParameter(in[1], TYPE_FLOAT, 2, 2, (Pointer)b2)) {
	    min = DXVec(b2[0].x, b2[0].y, -1000000);
	    max = DXVec(b2[1].x, b2[1].y, 1000000);
	} else if (DXExtractParameter(in[1], TYPE_FLOAT, 1, 2, (Pointer)b1)) {
	    min = DXVec(b1[0], -1000000, -1000000);
	    max = DXVec(b1[1], 1000000, 1000000);
	} else {
	    DXSetError(ERROR_BAD_PARAMETER, "#10581","corners", 2,2,2,3);
            return ERROR;
        }
    } else {
	if (DXBoundingBox(in[1], box)) {
	    min = box[0];
	    max = box[7];
	} else {
	    DXSetError(ERROR_BAD_PARAMETER, "#11020", "corners");
            return ERROR;
	}
    }

    out[0] = DXClipBox(in[0], min, max);
    if(!out[0])
	return ERROR;

    return OK;
}




