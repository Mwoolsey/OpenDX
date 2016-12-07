/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>



#include <dx/dx.h>
#include <math.h>


/*
 * in:   0 = input
 *      (1) = component name
 *
 * out: 0 = avg
 *      1 = std
 *      2 = var
 *      3 = min
 *      4 = max
 */
int
m_Statistics(Object *in, Object *out)
{
    float min, max, avg, stddev, var;
    double dvar;


    if (!in[0]) {
	DXSetError(ERROR_BAD_PARAMETER, "#10000", "input");
	return ERROR;
    }

    if (!DXStatistics(in[0], "data", &min, &max, &avg, &stddev))
	return ERROR;


    out[0] = (Object)DXNewArray(TYPE_FLOAT, CATEGORY_REAL, 0);
    if (!out[0] || !DXAddArrayData((Array)out[0], 0, 1, (Pointer)&avg))
	goto error;
			
    out[1] = (Object)DXNewArray(TYPE_FLOAT, CATEGORY_REAL, 0);
    if (!out[1]  || !DXAddArrayData((Array)out[1], 0, 1, (Pointer)&stddev))
	goto error;

    dvar = (double)stddev * stddev;
    if (dvar > DXD_MAX_FLOAT) {
	out[2] = (Object)DXNewArray(TYPE_DOUBLE, CATEGORY_REAL, 0);
	if (!out[2] || !DXAddArrayData((Array)out[2], 0, 1, (Pointer)&dvar))
	    goto error;
    } else {
	var = (float)dvar;
	out[2] = (Object)DXNewArray(TYPE_FLOAT, CATEGORY_REAL, 0);
	if (!out[2] || !DXAddArrayData((Array)out[2], 0, 1, (Pointer)&var))
	    goto error;
    }
	
    out[3] = (Object)DXNewArray(TYPE_FLOAT, CATEGORY_REAL, 0);
    if (!out[3] || !DXAddArrayData((Array)out[3], 0, 1, (Pointer)&min))
	goto error;
			
    out[4] = (Object)DXNewArray(TYPE_FLOAT, CATEGORY_REAL, 0);
    if (!out[4] || !DXAddArrayData((Array)out[4], 0, 1, (Pointer)&max))
	goto error;


    return OK;

error:
    DXDelete(out[0]);  out[0] = NULL;
    DXDelete(out[1]);  out[1] = NULL;
    DXDelete(out[2]);  out[2] = NULL;
    DXDelete(out[3]);  out[3] = NULL;
    DXDelete(out[4]);  out[4] = NULL;
    return ERROR;

}

