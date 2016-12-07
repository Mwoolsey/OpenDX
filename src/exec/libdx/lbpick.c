/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


#include <dx/dx.h>

Error
DXQueryPokeCount(Field p, int *knt)
{
    Array pokes;

    if (DXEmptyField(p))
    {
	if (knt) *knt = 0;
	return OK;
    }

    pokes = (Array)DXGetComponentValue(p, "pokes");
    if (! pokes)
    {
	DXSetError(ERROR_BAD_PARAMETER, 
		"field does not contain pick information");
        return ERROR;
    }

    if (knt)
	DXGetArrayInfo(pokes, knt, NULL, NULL, NULL, NULL);
    
    return OK;
}

Error
DXQueryPickCount(Field p, int poke, int *nPicks)
{
    Array pokes;
    Array picks;
    int   nPokes, thisStart, nextStart;

    if (DXEmptyField(p))
    {
	DXSetError(ERROR_BAD_PARAMETER, "non-existent poke");
	return ERROR;
    }

    pokes = (Array)DXGetComponentValue(p, "pokes");
    picks = (Array)DXGetComponentValue(p, "picks");
    if (! pokes || ! picks)
    {
	DXSetError(ERROR_BAD_PARAMETER, 
		"field does not contain pick information");
        return ERROR;
    }

    DXGetArrayInfo(pokes, &nPokes, NULL, NULL, NULL, NULL);

    if (poke >= nPokes)
    {
	DXSetError(ERROR_BAD_PARAMETER, "non-existent poke");
	return ERROR;
    }

    thisStart = ((int *)DXGetArrayData(pokes))[poke];

    if (poke == nPokes-1)
    {
	DXGetArrayInfo(picks, &nextStart, NULL, NULL, NULL, NULL);
    }
    else
    {
	nextStart = ((int *)DXGetArrayData(pokes))[poke+1];
    }

    if (nPicks)
	*nPicks = nextStart - thisStart;

    return OK;
}

Error
DXGetPickPoint(Field p, int poke, int pick, Point *point)
{
    Array pokes;
    Array points;
    int   nPokes, index;

    if (DXEmptyField(p))
    {
	DXSetError(ERROR_BAD_PARAMETER, "non-existent poke");
	return ERROR;
    }

    pokes  = (Array)DXGetComponentValue(p, "pokes");
    points = (Array)DXGetComponentValue(p, "positions");
    if (! pokes || ! points)
    {
	DXSetError(ERROR_BAD_PARAMETER, 
		"field does not contain pick information");
        return ERROR;
    }

    DXGetArrayInfo(pokes, &nPokes, NULL, NULL, NULL, NULL);

    if (poke >= nPokes)
    {
	DXSetError(ERROR_BAD_PARAMETER, "non-existent poke");
	return ERROR;
    }

    index = ((int *)DXGetArrayData(pokes))[poke] + pick;

    if (point)
	*point = ((Point *)DXGetArrayData(points))[index];

    return OK;
}

Error
DXQueryPickPath(Field p, int poke, int pick,
			int *len, int **path, int *eid, int *vid)
{
    Array pokes;
    Array picks;
    Array paths;
    int   nPokes, nPicks, nPathElts;
    int   thisPick;
    int   thisPathStart, nextPathStart;

    if (DXEmptyField(p))
    {
	DXSetError(ERROR_BAD_PARAMETER, "non-existent poke");
	return ERROR;
    }

    pokes = (Array)DXGetComponentValue(p, "pokes");
    picks = (Array)DXGetComponentValue(p, "picks");
    paths = (Array)DXGetComponentValue(p, "pick paths");
    if (! pokes || ! picks || ! paths)
    {
	DXSetError(ERROR_BAD_PARAMETER, 
		"field does not contain pick information");
        return ERROR;
    }

    DXGetArrayInfo(pokes, &nPokes,    NULL, NULL, NULL, NULL);
    DXGetArrayInfo(picks, &nPicks,    NULL, NULL, NULL, NULL);
    DXGetArrayInfo(paths, &nPathElts, NULL, NULL, NULL, NULL);

    if (poke >= nPokes)
    {
	DXSetError(ERROR_BAD_PARAMETER, "non-existent poke");
	return ERROR;
    }

    thisPick = ((int *)DXGetArrayData(pokes))[poke] + pick;
    thisPathStart = ((int *)DXGetArrayData(picks))[thisPick];

    if (thisPick == nPicks-1)
    {
	nextPathStart = nPathElts;
    }
    else
    {
	nextPathStart = ((int *)DXGetArrayData(picks))[thisPick+1];
    }

    if (len)
    {
	*len = (nextPathStart - thisPathStart) - 2;
	if (*len < 0)
	    *len = 0;
    }
    
    if (path)
	*path = ((int *)DXGetArrayData(paths)) + thisPathStart;
    
    if (eid)
	*eid = ((int *)DXGetArrayData(paths))[nextPathStart-2];

    if (vid)
	*vid = ((int *)DXGetArrayData(paths))[nextPathStart-1];

    return OK;
}

Object
DXTraversePickPath(Object current, int index, Matrix *stack)
{
    Object  child;
    Matrix  matrix;

    switch (DXGetObjectClass(current))
    {
	case CLASS_FIELD:
	    child = current;
	    break;
	
	case CLASS_GROUP:
	    child = DXGetEnumeratedMember((Group)current, index, NULL);
	    break;
	
	case CLASS_XFORM:
	{
	    DXGetXformInfo((Xform)current, &child, &matrix);
	    if (stack)
		*stack = DXConcatenate(matrix, *stack);

	    break;
	}

	case CLASS_CLIPPED:
	    DXGetClippedInfo((Clipped)current, &child, NULL);
	    break;
	
	default:
	    child = NULL;
    }

    return child;
}




