/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/*
 * $Header: /src/master/dx/src/exec/dxmods/transform.c,v 1.5 2006/01/03 17:02:26 davidt Exp $
 */

#include <dxconfig.h>

#if defined(HAVE_STRING_H)
#include <string.h>
#endif

#include <dx/dx.h>

/* special purpose; allows only 3x3 or 3x4 rank 2 matricies */
static Error ExtractMatrix(Object o, Matrix *m);

int
m_Transform(Object *in, Object *out)
{
    Matrix m;

    out[0] = NULL;

    if (!in[0]) {
	DXSetError(ERROR_BAD_PARAMETER, "#10000", "input");
	return ERROR;
    }

    if (!in[1]) {
	out[0] = in[0];
	return OK;
    }

    /* default to all zeros */
    memset((char *)&m, '\0', sizeof(Matrix));

    /* either a 9 or 12-item list or 9 or 12-member vector */
    if (DXExtractParameter(in[1], TYPE_FLOAT, 1, 9, (Pointer)&m))
        ;
    else if (DXExtractParameter(in[1], TYPE_FLOAT, 1, 12, (Pointer)&m))
        ;
    else if (DXExtractParameter(in[1], TYPE_FLOAT, 9, 1, (Pointer)&m))
        ;
    else if (DXExtractParameter(in[1], TYPE_FLOAT, 12, 1, (Pointer)&m))
        ;
    else if (ExtractMatrix(in[1], &m))
	;
    else {
        DXSetError(ERROR_BAD_PARAMETER, 
		 "%s must be a 9 or 12 item list, or a 3x3 or 3x4 matrix",
		 "transform matrix");
	return ERROR;
    }

        
    out[0] = (Object)DXNewXform(in[0], m);

    return(out[0] ? OK : ERROR);
}

static Error 
ExtractMatrix(Object o, Matrix *m)
{
    Pointer p;
    int i, items, shape[2];
    Type type;
    float *fp;

    if (DXGetObjectClass(o) != CLASS_ARRAY)
	return ERROR;
    
    if (!DXTypeCheck((Array)o, TYPE_FLOAT, CATEGORY_REAL, 2, 3, 3) &&
	!DXTypeCheck((Array)o, TYPE_FLOAT, CATEGORY_REAL, 2, 4, 3) &&
        !DXTypeCheck((Array)o, TYPE_INT, CATEGORY_REAL, 2, 3, 3) &&
	!DXTypeCheck((Array)o, TYPE_INT, CATEGORY_REAL, 2, 4, 3))
	return ERROR;

    if (!DXGetArrayInfo((Array)o, &items, &type, NULL, NULL, shape))
	return ERROR;

    if (items != 1)
	return ERROR;

    p = DXGetArrayData((Array)o);
    items = shape[0] * shape[1];

    if (type == TYPE_FLOAT)
	memcpy((char *)m, (char *)p, items * sizeof(float));
    else
	for (i=0, fp=(float *)m; i<items; i++, fp++)
	    *fp = (float)((int *)p)[i];

    return OK;
}
