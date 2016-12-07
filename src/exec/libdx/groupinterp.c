/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>



#include <stdio.h>
#include <math.h>
#include <string.h>
#include <dx/dx.h>
#include "groupinterpClass.h"

GroupInterpolator
_dxfNewGroupInterpolator(Group g,
	enum interp_init initType, float fuzz, Matrix *m)
{
    return _dxf_NewGroupInterpolator(g, initType, fuzz, m,
				&_dxdgroupinterpolator_class);
}

GroupInterpolator
_dxf_NewGroupInterpolator(Group g,
	enum interp_init initType, float fuzz, Matrix *m,
			struct groupinterpolator_class *class)
{
    GroupInterpolator	gi;
    int 		i, j;
    Object		child;
    Interpolator	*subPtr, sub, firstsub = NULL;
    int			isMultigrid;

    CHECK(g, CLASS_GROUP);

    gi=(GroupInterpolator)
	_dxf_NewInterpolator((struct interpolator_class *)class, (Object)g);
    if (!gi) 
	return NULL;
    
    isMultigrid = DXGetGroupClass(g) == CLASS_MULTIGRID;
    
    /*
     * Create array of pointers to interpolators for each descendent
     * of the current group.  First, how many descendents?
     */
    for (i = 0; NULL != (child = DXGetEnumeratedMember(g, i, NULL)); i++);

    gi->subInterp = (Interpolator *)DXAllocate(i*sizeof(Interpolator *));
    if (!gi->subInterp)
    {
	DXFree((Pointer)gi);
	return NULL;
    }

    /*
     * Create interpolators for each.  Collect a group bound box
     * as we do so.
     */
    subPtr = gi->subInterp;
    gi->nMembers = 0;
    for (i = 0; NULL != (child = DXGetEnumeratedMember(g, i, NULL)); i++)
    {
	/*
	 * Ignore fields that have no elements
	 */
	if (DXGetObjectClass(child) == CLASS_FIELD)
	{
	    Array array;
	    int   nElements;

	    if (DXEmptyField((Field)child))
		continue;
	    
	    array = (Array)DXGetComponentValue((Field)child, "connections");
	    if (! array)
		continue;

	    DXGetArrayInfo(array, &nElements, NULL, NULL, NULL, NULL);
	    if (nElements == 0)
		continue;
	}

	sub = (Interpolator)_dxfNewInterpolatorSwitch(child,
						initType, fuzz, m);
	if (! sub)
	{
	    DXDelete((Object)gi);
	    return NULL;
	}

	*subPtr++ = sub;

	DXReference((Object)sub);

	gi->nMembers ++;

	if (i == 0)
	{
	    DXGetType(sub->dataObject, &gi->interpolator.type,
				     &gi->interpolator.category,
				     &gi->interpolator.rank,
				     gi->interpolator.shape);
	    
	    gi->interpolator.nDim = sub->nDim;

	    memcpy((void *)(gi->interpolator.min), (void *)(sub->min),
				gi->interpolator.nDim*sizeof(float));
	    memcpy((void *)(gi->interpolator.max), (void *)(sub->max),
				gi->interpolator.nDim*sizeof(float));
		
	    firstsub = sub;
	}
	else
	{
	    for (j = 0; j < gi->interpolator.nDim; j++)
	    {
		if (sub->min[j] < gi->interpolator.min[j])
		    gi->interpolator.min[j] = sub->min[j];
		if (sub->max[j] > gi->interpolator.max[j])
		    gi->interpolator.max[j] = sub->max[j];
	    }

	    if (! isMultigrid)
		((Interpolator)sub)->matrix = ((Interpolator)firstsub)->matrix;
	}
    }
    
    /*
     * Initially, no hint
     */
    gi->hint = NULL;
    
    return gi;
}

Error
_dxfGroupInterpolator_Delete(GroupInterpolator gi)
{
    int i;

    /*
     * DXFree interpolator pointed to by hint
     */
    if (gi->hint)
    {
	DXDelete((Object)gi->hint);
	gi->hint = NULL;
    }

    /*
     * DXFree descendent interpolators
     */
    if (gi->subInterp)
    {
	for (i = 0; i < gi->nMembers; i++)
	    if (gi->subInterp[i])
	    {
		DXDelete((Object)gi->subInterp[i]);
		gi->subInterp[i] = NULL;
	    }
	
	DXFree((Pointer)gi->subInterp);
	gi->subInterp = NULL;
    }

    _dxfInterpolator_Delete((Interpolator)gi);

    return OK;
}

int
_dxfGroupInterpolator_Interpolate(GroupInterpolator gi, int *n,
    float **points, Pointer *values, Interpolator *parentHint, int fuzzFlag)
{
    Interpolator	*sub, *subBase, myHint, newHint;
    int			i, nMem, nAtStart;

    myHint = NULL;

    nAtStart = *n;

    /*
     * Start with the child (possibly distant descendent)
     * pointed to by the hint.
     */
    if (gi->hint)
	if (! _dxfInterpolate(gi->hint, n, points, values, &myHint, fuzzFlag))
	    return 0;
    
    /*
     * Turn off fuzz as soon as a sample succeeds. All subsequent
     * interpolation attempts will be done sans fuzz.
     */
    if (nAtStart > *n)
	fuzzFlag = FUZZ_OFF;

    /*
     * As long as poits remain to be interpolated, call descendents
     * to interpolate them.  When a point is encountered that does
     * not lie in any child field, quit.
     */
    nMem = gi->nMembers;
    subBase = gi->subInterp;
    while(*n != 0)
    {
	newHint = NULL;

	sub = subBase;
	for (i = 0; i < nMem; i++)
	{
	    if ((*sub) != gi->hint)
		if (! _dxfInterpolate((*sub), n, points, values, &newHint, fuzzFlag))
		    return 0;

	    /*
	     * Again, turn off fuzz as soon as a sample succeeds. All 
	     * subsequent interpolation attempts will be done sans fuzz.
	     */
	    if (nAtStart > *n)
		fuzzFlag = FUZZ_OFF;

	    if (newHint)
		break;

	    sub++;
	}

	/*
	 * Keep going as long as we have more points to interpolate and
	 * we interpolated some last time through the children.
	 */
	if (! newHint)
	    break;
	
	myHint = newHint;
    }

    /*
     * If we didn't interpolate any points and there was a hint,
     * delete the old hint.
     */
    if (gi->hint != myHint)
    {
	if (gi->hint)
	    DXDelete((Object)(gi->hint));

	if (myHint)
	    gi->hint = (Interpolator)DXReference((Object)(myHint));
	else
	    gi->hint = NULL;
    }

    /*
     * If the parent requested a hint, pass it up.
     */
    if (parentHint)
	*parentHint = myHint;

    return OK;
}

Object
_dxfGroupInterpolator_Copy(GroupInterpolator old, enum _dxd_copy copy)
{
    GroupInterpolator new;

    new = (GroupInterpolator)
	_dxf_NewObject((struct object_class *)&_dxdgroupinterpolator_class);

    return (Object)_dxf_CopyGroupInterpolator(new, old, copy);
}

GroupInterpolator
_dxf_CopyGroupInterpolator(GroupInterpolator new, GroupInterpolator old, 
								enum _dxd_copy copy)
{
    Interpolator	*ns, *os;
    int			i;

    if (old == NULL)
	return NULL;

    if (! _dxf_CopyInterpolator((Interpolator)new, (Interpolator)old))
	return NULL;

    new->nMembers = old->nMembers;

    new->subInterp = (Interpolator *)
			    DXAllocate(new->nMembers*sizeof(Interpolator));

    if (! new->subInterp)
    {
	DXFree((Pointer)new);
	return NULL;
    }

    ns = new->subInterp;
    os = old->subInterp;
    for (i = 0; i < new->nMembers; i++, os++, ns++)
    {
	if (NULL ==  (*ns = (Interpolator)DXCopy((Object)*os, copy)))
	{
	    while(i-- > 0)
		DXDelete((Object)*(--ns));
	    DXFree((Pointer)new->subInterp);
	    DXFree((Pointer)new);
	    return NULL;
	}
	DXReference((Object)*ns);
    }
    
    new->hint = NULL;
    
    return new;
}

Interpolator
_dxfGroupInterpolator_LocalizeInterpolator(GroupInterpolator gi)
{
    int			i;
    Interpolator	*childi;

    childi = gi->subInterp;
    for (i = 0; i < gi->nMembers; i++, childi++)
	DXLocalizeInterpolator(*childi);
    
    return (Interpolator)gi;
}
