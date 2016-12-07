/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/*
 * $Header: /src/master/dx/src/exec/dxmods/refine.c,v 1.4 2000/08/24 20:04:45 davidt Exp $:
 */

#include <dxconfig.h>


/***
MODULE:
 _dxfRefine
SHORTDESCRIPTION:
 Refinement of input field
CATEGORY:
 Data Transformation
INPUTS:
 input;   field;   none;      field to refine
 level;   scalar;     1;      level of refinement
OUTPUTS:
 refined; field;   NULL;      refined input
FLAGS:
BUGS:
AUTHOR:
 Greg Abram
END:
***/

#define PARALLEL

#include <stdio.h>
#include <dx/dx.h>
#include "_refine.h"

#define NEW_LEVEL	1
#define NEW_TOPOLOGY	2

Error
m_Refine(Object *in, Object *out)
{
    int   level, refineType;
    char *newEltType;

    out[0] = NULL;

    if(! in[0])
    {
	DXSetError(ERROR_BAD_PARAMETER, "#10000", "input");
	return ERROR;
    }

    level = 1;
    refineType = NEW_LEVEL;
    if (in[1])
    {
	float fLevel;

	if (DXExtractInteger(in[1], &level))
	{
	    refineType = NEW_LEVEL;
	}
	else if (DXExtractFloat(in[1], &fLevel))
	{
	    if (fLevel != (int)fLevel)
	    {
		DXSetError(ERROR_BAD_PARAMETER, "#10620", "level");
		return ERROR;
	    }
	    refineType = NEW_LEVEL;
	    level = (int)fLevel;
	}
	else if (DXExtractString(in[1], &newEltType))
	{
	    refineType = NEW_TOPOLOGY;
	}
	else
	{
	    DXSetError(ERROR_BAD_PARAMETER, "#10620", "level");
	    return ERROR;
	}
    }

    if (refineType == NEW_LEVEL && level < 0)
    {
	DXSetError(ERROR_BAD_PARAMETER, "#10620", "level");
	return ERROR;
    }

    if (refineType == NEW_LEVEL)
    {
	out[0] = _dxfRefine(in[0], level);

	if (! out[0])
	    return ERROR;
	
	return OK;
    }
    else /* refineType == NEW_TOPOLOGY */
    {
	out[0] = _dxfChgTopology(in[0], newEltType);

	if (! out[0])
	    return ERROR;
    }

    return OK;
}
