/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/*
 * $Header: /src/master/dx/src/exec/dxmods/divcurl.c,v 1.4 2000/08/24 20:04:28 davidt Exp $:
 */

#include <dxconfig.h>


/***
MODULE:		
    _dxfDivCurl
SHORTDESCRIPTION:
    Computes divergence and curl
CATEGORY:
    Data Transformation
INPUTS:
    data;     vector field;        none; field to compute divergence and curl of
    method;         string; "manhattan"; gradient method to use
OUTPUTS:
    div;      scalar field;        NULL; divergence field
    curl;     vector field;        NULL; curl field
FLAGS:
BUGS:
AUTHOR:
    Brian P. O'Toole
END:
***/

#include <stdio.h>
#include <math.h>
#include <string.h>
#include <dx/dx.h>
#include "_divcurl.h"

int m_DivCurl (in, out)
    Object		*in;
    Object		*out;
{
    Object		ino0;

    out[0] = NULL;
    out[1] = NULL;

    ino0 = in[0];

    if (ino0 == NULL)
    {
	DXSetError(ERROR_BAD_PARAMETER, "#10000", "data");
	return ERROR;
    }

    if (in[1])
    {
	char *str;

	if (! DXExtractString(in[1], &str))
	{
	    DXSetError(ERROR_BAD_PARAMETER, "#10200", "method");
	    return ERROR;
	}
	if (strcmp(str, "manhattan"))
	{
	    DXSetError(ERROR_BAD_PARAMETER, "#10211", "method", "manhattan");
	    return ERROR;
	}
    }

    if (! _dxfDivCurl (ino0, &out[0], &out[1]))
	return ERROR;

    return (OK);
}
