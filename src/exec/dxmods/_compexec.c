/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


#include <string.h>
#include <dx/dx.h>
#include "_compute.h"
#include "_compoper.h"
#include "_compputils.h"

/* Some concepts:
 * This probably belongs elsewhere, and will get moved, but here it is.
 * Definitions:
 * Heterogeneous Groups are Objects with both class and subClass CLASS_GROUP.
 * Homogeneous Groups are Groups whose members have the same metaType.
 * Parts (as elsewhere) are all fields and leaves.
 * Virtual Parts are all Objects that aren't heterogeneous groups and are not
 *	members of homogeneous groups.
 * The Parents of a part is the closest ancestor in the group hierarchy that
 * 	is a virtual part.  A part may be its own parent.
 *
 * Virtual Parts have a copy of the parse tree which contains type
 * information unique to that virtual part.
 * Parts have pointers to parent's parse trees.
 *
 * Execution:
 * The general model is that to execute a parse node, 
 *	result data space is allocated for each child parse node
 *	those nodes are executed recursively
 *	the operation represented by this node is performed
 *	the temporary data space for children is freed,
 *	the result is returned.
 *
 * For each virtual part, execute each child;
 * To execute a child (part):
 *	Allocate a result array
 *      Execute (part, parseTree, result);
 *	if part is field
 *	    replace part->output."data" with result
 *	else part is array
 *	    Allocate new array with result data into part->output
 *	    if part is a member of an group, insert part->output into group.
 *	Free result
 * Both of the above routines may be executed in parallel.
 * The model should be that if there is more than one part, go parallel
 * building descriptors of a pointer to the part and a pointer to the parse 
 * tree.  Each node in the ObjStruct array has an appropriate "output"
 * field in which to insert the results.
 */
typedef struct {
    ObjStruct *os;
    PTreeNode *pt;
} TaskArg;

#define IS_CONTAINER_CLASS(x)  (\
	(x) == CLASS_GROUP || \
	(x) == CLASS_XFORM || \
	(x) == CLASS_SCREEN || \
	(x) == CLASS_CLIPPED)


static int
Execute (Pointer ptr)
{
    TaskArg *ta = (TaskArg *)ptr;    
    ObjStruct 	*os = ta->os;
    PTreeNode 	*pt = ta->pt;
    Pointer 	result = NULL;
    InvalidComponentHandle invalid = NULL;
    int      	numBasic;
    int	     	i;
    Array    	a = NULL;
    Error	status = OK;
    PTreeNode   *this=NULL;
    PTreeNode   *next;
    int		executionFinished = 0;

#ifdef COMP_DEBUG
    DXDebug ("C", "TypeCheck (from ExecuteTask if parallel)");
#endif
    _dxfComputeInitExecution();

    for (next = pt->args; next; ) {
	this = next;
	next = this->next;

	if (os->parseTree)
	    _dxfComputeFreeTree(os->parseTree);

	if (_dxfComputeTypeCheck (this, os) == ERROR) {
	    goto error;
	}
	this = os->parseTree;

	if (os->metaType.items == 0) {
	    continue;
	}

#ifdef COMP_DEBUG
	DXDebug ("C", "Execute Node (from Execute Task if parallel)");
#endif

	if (this->metaType.items != 0) {
	    for (numBasic = 1, i = 0; i < this->metaType.rank; ++i) {
		numBasic *= this->metaType.shape[i];
	    }
	    if (result)
		DXFree(result);
	    result = DXAllocateLocalZero (
		this->metaType.items * 
		DXTypeSize (this->metaType.type) *
		DXCategorySize (this->metaType.category) *
		numBasic);
	    if (result == NULL) {
		DXResetError();
		result = DXAllocateZero (
		    this->metaType.items * 
		    DXTypeSize (this->metaType.type) *
		    DXCategorySize (this->metaType.category) *
		    numBasic);
		if (result == NULL) {
		    goto error;
		}
	    }

	    if (invalid)
	    {
		DXFreeInvalidComponentHandle(invalid);
		invalid = NULL;
	    }
		
	    status = _dxfComputeExecuteNode (this, os, result, &invalid);
	    if (status == ERROR) {
		goto error;
	    }
	}
    }
    _dxfComputeFinishExecution();
    executionFinished = 1;

    if (os->metaType.items != 0) {
	if (this->metaType.items == 1 && os->metaType.items > 1) {
	    a = (Array) DXNewConstantArrayV(
		os->metaType.items, 
		result,
		this->metaType.type,
		this->metaType.category,
		this->metaType.rank,
		this->metaType.shape);
	    if (a == NULL) {
		goto error;
	    }
	}
	else {
	    a = DXNewArrayV (this->metaType.type, this->metaType.category,
			   this->metaType.rank, this->metaType.shape);
	    if (a == NULL) {
		goto error;
	    }
	    if (DXAddArrayData (a, 0, os->metaType.items, result) == NULL) {
		goto error;
	    }
	}

	if (os->class == CLASS_FIELD) {
	    Object origData; 
	    int    hadData;
	    int    hasPositions;
	    Object attr;
	    int    hasDepComponent = 0;
	    char  *dep;

	    origData = 
		DXGetComponentValue((Field)os->output, "data");
	    hadData = origData != NULL;
	    hasPositions =
		DXGetComponentValue((Field)os->output, "positions") != NULL;

	    if (origData) {
		attr = DXGetAttribute (origData, "dep");
		if (DXExtractString(attr, &dep))
		    hasDepComponent =
			DXGetComponentValue((Field)os->output, dep) != NULL;
	    }
	    else
		hasDepComponent = hasPositions;

	    if (DXSetComponentValue ((Field)os->output, "data", (Object)a) 
		    == NULL) {
		goto error;
	    }
	    a = NULL;

	    if (invalid != NULL) {
		if (hasDepComponent && 
		    DXSaveInvalidComponent((Field)os->output, invalid)
			== ERROR) {
		    goto error;
		}
		DXFreeInvalidComponentHandle(invalid);
		invalid = NULL;
	    }
	    if (DXChangedComponentValues ((Field)os->output, "data")
		    == NULL) {
		goto error;
	    }
	    if (!hadData && hasPositions)
	    {
		if (DXSetComponentAttribute((Field)os->output,
					  "data",
					  "dep",
					  (Object)DXNewString("positions"))
			== NULL)
		{
		    goto error;
		}
	    }
	}
	else {
	    DXCopyAttributes((Object)a, os->output);
	    os->output = (Object)a;
	}
    }

    DXFree(result);
    return (OK);
error: 
    if (!executionFinished) {
	_dxfComputeFinishExecution();
    }
    DXDelete((Object)a);
    DXFree(result);
    DXFreeInvalidComponentHandle(invalid);
    return ERROR;
}

/* A routine to take a part and a parse tree, and execute it.
 */
static int
ExecutePart (ObjStruct *inputStruct, PTreeNode *parseTree, int parallel)
{
    TaskArg ta;

#ifdef COMP_DEBUG
    DXDebug ("C", "ExecutePart (parallel = %d)", parallel);
#endif

    ta.os = inputStruct;
    ta.pt = parseTree;

    if (parallel) {
	return (DXAddTask (Execute, (Pointer)&ta, sizeof (TaskArg), 1.0));
    }
    else {
	return (Execute (&ta));
    }
}

/* A routine to take a virtual-part (group), and execute it wrt its parse
 * tree.
 */
static int
ExecuteVirtualPart (ObjStruct *inputStruct, PTreeNode *tree, int parallel)
{
    ObjStruct *subStruct;

#ifdef COMP_DEBUG
    DXDebug ("C", "ExecuteVirtualPart (parallel = %d)", parallel);
#endif

    /* Execute each child */
    if (!IS_CONTAINER_CLASS(inputStruct->class)) {
	if (ExecutePart (inputStruct, tree, parallel) != OK)
	    return (ERROR);
    }
    else {
	subStruct = inputStruct->members;
	while (subStruct != NULL) {
	    if (ExecuteVirtualPart (subStruct, tree, parallel) != OK)
		return (ERROR);
	    subStruct = subStruct->next;
	}
    }
    return (OK);
}

/* A routine to take a node, and determine if it is a virtual-part ready for 
 * execution, or recurse.
 */
static int
ExecuteStruct (ObjStruct *inputStruct, PTreeNode *tree, int parallel)
{
    ObjStruct *subStruct;

#ifdef COMP_DEBUG
    DXDebug ("C", "ExecuteStruct (parallel = %d)", parallel);
#endif

    switch (inputStruct->class) {
    case CLASS_XFORM:
    case CLASS_SCREEN:
    case CLASS_CLIPPED:
    case CLASS_GROUP:
	switch (inputStruct->subClass) {
	default:
	case CLASS_GROUP:
	    subStruct = inputStruct->members;
	    while (subStruct != NULL) {
		if (ExecuteStruct (subStruct, tree, parallel) == ERROR)
		    return (ERROR);
		subStruct = subStruct->next;
	    }
	    break;
	case CLASS_SERIES:
	case CLASS_COMPOSITEFIELD:
	case CLASS_MULTIGRID:
	    if (ExecuteVirtualPart (inputStruct, tree, parallel) == ERROR)
		return (ERROR);
	    /* Set my new type */
	    break;
	}
	break;

    case CLASS_STRING:
    case CLASS_FIELD:
    case CLASS_ARRAY:
	if (ExecuteVirtualPart (inputStruct, tree, parallel) == ERROR)
	    return (ERROR);
	break;

    default:
	DXSetError (ERROR_INTERNAL, 
	    "#10370", "input", "Array, Field, Group, Transform, Screen, or Clipped");
	return (ERROR);
    }
    return (OK);
}

/* A routine to take a node, and determine if it is a virtual-part ready for 
 * execution, or recurse.
 */
static int
FixupStruct (ObjStruct *inputStruct)
{
    ObjStruct *subStruct;
    int items;

#ifdef COMP_DEBUG
    DXDebug ("C", "FixupStruct");
#endif

    if (inputStruct == NULL)
	return OK;

    switch(inputStruct->class) {
    case CLASS_GROUP:
	if (inputStruct->members != NULL)
	{
	    subStruct = inputStruct->members;
	    while (subStruct != NULL) {
		if (FixupStruct (subStruct) == ERROR)
		    return (ERROR);
		subStruct = subStruct->next;
	    }
	    inputStruct->metaType = inputStruct->members->metaType;
	    if (inputStruct->subClass != CLASS_GROUP) {
		subStruct = inputStruct->members;
		while (subStruct != NULL) {
		    if (subStruct->class != CLASS_FIELD ||
			subStruct->metaType.items != 0) {
			inputStruct->metaType = subStruct->metaType;
			break;
		    }
		    subStruct = subStruct->next;
		}
		if (subStruct && DXSetGroupTypeV (
			(Group)inputStruct->output, 
			inputStruct->metaType.type,
			inputStruct->metaType.category,
			inputStruct->metaType.rank,
			inputStruct->metaType.shape) == NULL) {
		    return (ERROR);
		}
	    }
	}
        break;

    case CLASS_XFORM:
    case CLASS_SCREEN:
    case CLASS_CLIPPED:
	if (FixupStruct (inputStruct->members) == ERROR)
	    return (ERROR);
	break;

    default:
	items = inputStruct->metaType.items;
	inputStruct->metaType = inputStruct->parseTree->metaType;
	inputStruct->metaType.items = items;
	break;
    }

    return (OK);
}


/* A routine to take the root node and execute it.
 */
int
_dxfComputeExecute (PTreeNode *tree, ObjStruct *inputStruct)
{
#ifdef COMP_DEBUG
    DXDebug ("C", "_ComputeExecute");
#endif

    if (DXProcessors(0) == 1) { 
#ifdef COMP_DEBUG
	DXDebug ("C", "Handle uniprocessor case.");
#endif
	if (ExecuteStruct (inputStruct, tree, 0) == ERROR)
	    return (ERROR);
    }
    else {
#ifdef COMP_DEBUG
	DXDebug ("C", "Handle multiproc case.");
#endif
	if (DXCreateTaskGroup() == ERROR)
	    return (ERROR);

	if (ExecuteStruct (inputStruct, tree, 1) == ERROR) {
	    DXAbortTaskGroup();
	    return (ERROR);
	}

	if (DXExecuteTaskGroup () == ERROR)
	    return (ERROR);
    }

    return (FixupStruct (inputStruct));
}
