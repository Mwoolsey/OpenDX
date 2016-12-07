/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/*
 * $Header: /src/master/dx/src/exec/dxmods/ribbon.c,v 1.4 2000/08/24 20:04:46 davidt Exp $
 */

#include <dxconfig.h>
#include "_autocolor.h"


#include <stdio.h>
#include <math.h>
#include <dx/dx.h>

static Error ribbon_width ();

int m_Ribbon (in, out)
    Object	*in;
    Object	*out;
{
    Object	in0;
    Object	in1;
    double	width;
    float	fval;

    out[0] = NULL;

    in0 = in[0];
    in1 = in[1];

    if (in0 == NULL)
	DXErrorReturn (ERROR_BAD_PARAMETER, "missing input object");

    /* return empty field for empty field input */
    if (DXGetObjectClass(in0)==CLASS_FIELD && DXEmptyField((Field)in0)) {
	out[0] = in0;
	return OK;
    }

    /*
     * If the "width" input is NULL, default width will be set
     * in DXTube
     */

    if (in1 == NULL)
    {
	width = -1;
    }
    else
    {
	if (! ribbon_width (in1, &fval))
	    return (ERROR);	
	width = fval;
    }

    if (! (out[0] = (Object) DXRibbon (in0, width)))
       return ERROR;

    return (OK);
}

static Error ribbon_width (ino, fval)
    Object 		ino;
    float 		*fval;
{
    if (! DXExtractParameter (ino, TYPE_FLOAT, 1, 1, (Pointer) fval))
	DXErrorReturn (ERROR_BAD_PARAMETER,"bad ribbon width");

    return (OK);
}
