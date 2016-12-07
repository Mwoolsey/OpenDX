/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/*
 * $Header: /src/master/dx/src/exec/dxmods/rubbersheet.c,v 1.4 2000/08/24 20:04:46 davidt Exp $:
 */

#include <dxconfig.h>


/***
MODULE:		
    _dxfRubberSheet
SHORTDESCRIPTION:
    Produces a rubber-sheet graph.
CATEGORY:
    Realization
INPUTS:
    data;      scalar field;        none; the data to graph
    scale;           scalar; appropriate; displacement scaling
    min; scalar or field;         0.0; offset to apply to data values
    max; scalar or field;         0.0; data value to use to set scale 
OUTPUTS:
    graph;     scalar field;         NULL; the graph
FLAGS:
BUGS:
AUTHOR:
    Brian P. O'Toole
END:
***/

#include <stdio.h>
#include <math.h>
#include <dx/dx.h>
#include "_rubbersheet.h"

int m_RubberSheet (in, out)
    Object		*in;
    Object		*out;
{
    Object		ino0, ino1, ino2, ino3;
    int			i;
    Field		field;

    out[0] = NULL;

    ino0 = in[0];
    ino1 = in[1];
    ino2 = in[2];
    ino3 = in[3];

    if (ino0 == NULL)
    {
        /*
	 * data must be specified 
	 */ 
	DXSetError (ERROR_BAD_PARAMETER, "#10000", "data");
        return ERROR;
    }

    for (i = 0; NULL != (field = DXGetPart(ino0, i)); i++)
	 if (! DXEmptyField(field))
	     break;
	
    if (field == NULL)
    {
	out[0] = (Object)DXEndField(DXNewField());
	return OK;
    }

    if (ino1)
    {
	float scale;

        if (! DXExtractParameter (ino1, TYPE_FLOAT, 1, 1, (Pointer) &scale))
	{
            /*
	     * scale must be a scalar value
	     */
            DXSetError (ERROR_BAD_PARAMETER, "#10080", "scale");
            return ERROR;
        }
    }

    if (! _dxfRubberSheet (ino0, ino1, ino2, ino3, &out[0]))
	return ERROR;

    return (OK);
}
