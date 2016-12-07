/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/*
 * $Header: /src/master/dx/src/exec/dxmods/tube.c,v 1.4 2000/08/24 20:04:54 davidt Exp $
 */

#include <dxconfig.h>



#include <stdio.h>
#include <math.h>
#include <dx/dx.h>
#include "_autocolor.h"

static Error tube_diameter();
static Error tube_ngon();

int m_Tube(in, out)
    Object	*in;
    Object	*out;
{
    Object	in0, in1, in2, in3;
    double	diameter;
    float	fval;
    int		ngon;
    char	*style;

    out[0] = NULL;

    in0 = in[0];
    in1 = in[1];
    in2 = in[2];
    in3 = in[3];

    if (in0 == NULL)
	DXErrorReturn(ERROR_BAD_PARAMETER, "missing input object");

    /* return empty field for empty field input */
    if (DXGetObjectClass(in0)==CLASS_FIELD && DXEmptyField((Field)in0)) {
	out[0] = in0;
	return OK;
    }

    /*
     * If the "diameter" input is NULL, default value will be chosen
     * based on valid bounding-box diagonal.
     */
    if (in1 == NULL)
    {
	diameter = -1;
    }
    else
    {
	if (! tube_diameter(in1, &fval))
	    return ERROR;
	diameter = (double) fval;
    }

    /*
     * If the "ngon" input is NULL, use a default value of 8.
     * Otherwise, process the input.
     */

    if (in2 == NULL)
	ngon = (int) 8;
    else {
	if (! tube_ngon(in2, &ngon))
	    return ERROR;
    }

    /*
     * If the "style" input is NULL, use a default value of "sphere".
     * Otherwise, process the input.
     */

    if (in3 == NULL) {
	style = "sphere";
    } else {
	if (! DXExtractString(in3, &style)) 
	    DXErrorReturn(ERROR_BAD_PARAMETER, "bad style input");
    }

    if (! (out[0] = (Object) DXTube(in0, diameter, ngon)))
	return ERROR;

    return OK;
}

static Error tube_diameter(ino, fval)
    Object 		ino;
    float 		*fval;
{
    if (! DXExtractParameter(ino, TYPE_FLOAT, 1, 1, (Pointer) fval))
	DXErrorReturn(ERROR_BAD_PARAMETER, "bad tube diameter");

    return OK;
}

static Error tube_ngon(ino, ival)
    Object 		ino;
    int 		*ival;
{
    if (! DXExtractParameter(ino, TYPE_INT, 1, 1, (Pointer) ival))
	DXErrorReturn(ERROR_BAD_PARAMETER, "bad number of sides");

    return OK;
}
