/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


/*
 * $Header: /src/master/dx/src/exec/dxmods/_refine.c,v 1.4 2000/08/24 20:04:18 davidt Exp $
 */

#include <string.h>
#include "math.h"
#include <dx/dx.h>
#include "_refine.h"

static Field RefineField(Field, char *, int);

typedef struct 
{
    int		 limit;
} Args;

Object
_dxfRefine(Object object, int limit)
{
    Args	args;
    Object	result;

    args.limit     = limit;
	
    result = DXProcessParts
                 (object, RefineField, (char *)&args, sizeof(args), 1, 0);
    
    return result;
}

static Field
RefineField(Field field, char *args, int l)
{
    int	  limit = ((Args *)args)->limit;
    Field newField;
    Array array;

    if (DXEmptyField(field))
	return DXEndField(DXNewField());
    
    field = (Field)DXCopy((Object)field, COPY_STRUCTURE);
    if (! field)
	return NULL;

    array = (Array)DXGetComponentValue(field, "connections");

    if (DXQueryGridConnections(array, NULL, NULL))
	newField = _dxfRefineReg(field, limit);
    else
	newField = _dxfRefineIrreg(field, limit);

    if (newField == NULL)
	DXDelete((Object)field);
    
    return newField;
}
