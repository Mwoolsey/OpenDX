/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/* if all points in generic positions arrays are to be compared (as
 * opposed to just the number of items in the positions arrays), uncomment 
 * the following #defines.
 */
/* #define CHECK_ANY_POSITIONS */
/* #define CHECK_ALL_POSITIONS */

#include <dxconfig.h>


#include <string.h>
#include <dx/dx.h>
#include "_compute.h"
#include "_compputils.h"

/*
 * _ComputeInitInputs initialize the set of inputs for compute
 */
void
_dxfComputeInitInputs(CompInput *inputs)
{
    int i;

    for (i = 0; i < MAX_INPUTS; ++i) {
	inputs[i].used = 0;
    }
}

#if 0
static int
ComparePositions(Array master, Array checked)
{
    Class masterClass;
    Class checkedClass;
    int   masterN;
    int   checkedN;
    int   *masterCounts  = NULL;
    int   *checkedCounts = NULL;
    float *masterOrigin  = NULL;
    float *checkedOrigin = NULL;
    float *masterDeltas  = NULL;
    float *checkedDeltas = NULL;
    MetaType masterType;
    MetaType checkedType;
    Pointer masterData;
    Pointer checkedData;
    int status;
    int i, j;


    if (master == checked)
	return (OK);
    if (master == NULL || checked == NULL) 
	return (ERROR);
    
    masterClass = DXGetArrayClass (master);
    checkedClass = DXGetArrayClass (checked);

    if (masterClass != checkedClass)
	return (ERROR);
    
    switch (masterClass) {
    case CLASS_ARRAY:
    default:
	if (DXGetArrayInfo (master, 
		&masterType.items, &masterType.type, &masterType.category,
		&masterType.rank, masterType.shape) == NULL) {
	    return (ERROR);
	}
	if (DXGetArrayInfo (checked, 
		&checkedType.items, &checkedType.type, &checkedType.category,
		&checkedType.rank, checkedType.shape) == NULL) {
	    return (ERROR);
	}

	if (    masterType.items != checkedType.items ||
		masterType.type != checkedType.type ||
		masterType.category != checkedType.category ||
		masterType.rank != checkedType.rank) {
	    return (ERROR);
	}
#ifdef CHECK_ALL_POSITIONS
	numBasic = 1;
#endif
	for (i = 0; i < masterType.rank; ++i) {
	    if (masterType.shape[i] != checkedType.shape[i]) {
		return (ERROR);
	    }
#ifdef CHECK_ALL_POSITIONS
	    numBasic *= masterType.shape[i];
#endif
	}

#ifdef CHECK_ALL_POSITIONS
	masterData = DXGetArrayData (master);
	if (masterData == NULL) {
	    return (ERROR);
	}
	checkedData = DXGetArrayData (checked);
	if (checkedData == NULL) {
	    return (ERROR);
	}
	status = bcmp (masterData, checkedData, 
	    masterType.items * TypeSize (masterType.type) *
	    CategorySize (masterType.category) * numBasic) == 0? OK: ERROR;
#else
	status = OK;
#endif
	break;

    case CLASS_CONSTANTARRAY:
	if (DXGetArrayInfo (master, 
		&masterType.items, &masterType.type, &masterType.category,
		&masterType.rank, masterType.shape) == NULL) {
	    return (ERROR);
	}
	if (DXGetArrayInfo (checked, 
		&checkedType.items, &checkedType.type, &checkedType.category,
		&checkedType.rank, checkedType.shape) == NULL) {
	    return (ERROR);
	}

	if (    masterType.items != checkedType.items ||
		masterType.type != checkedType.type ||
		masterType.category != checkedType.category ||
		masterType.rank != checkedType.rank) {
	    return (ERROR);
	}
	for (i = 0; i < masterType.rank; ++i) {
	    if (masterType.shape[i] != checkedType.shape[i]) {
		return (ERROR);
	    }
	}
	masterData = DXGetConstantArrayData(master);
	checkedData = DXGetConstantArrayData(checked);
	if (bcmp(masterData, checkedData, DXGetItemSize(master)) == 0)
	    status = OK;
	else
	    status = ERROR;
	break;

    case CLASS_REGULARARRAY:
    case CLASS_PRODUCTARRAY:
    case CLASS_PATHARRAY:
    case CLASS_MESHARRAY:
	if (DXQueryGridPositions (master, &masterN, NULL, NULL, NULL) == NULL) {
	    DXSetError (ERROR_INTERNAL, 
		"ComparePositions, %d: Can't get regular array data", __LINE__);
	    status = ERROR;
	    break;
	}
	if (DXQueryGridPositions (checked, &checkedN, NULL, NULL, NULL) == NULL) {
	    DXSetError (ERROR_INTERNAL, 
		"ComparePositions, %d: Can't get regular array data", __LINE__);
	    status = ERROR;
	    break;
	}

	if (masterN != checkedN) {
	    status = ERROR;
	    break;
	}

	if ((masterCounts = (int *) DXAllocateLocal (masterN * sizeof (int))) == 
		NULL) {
	    status = ERROR;
	    break;
	}
	if ((checkedCounts = (int *) DXAllocateLocal (masterN * sizeof (int))) == 
		NULL) {
	    status = ERROR;
	    break;
	}
	if ((masterOrigin = (float *) DXAllocateLocal (masterN * sizeof (float))) == 
		NULL) {
	    status = ERROR;
	    break;
	}
	if ((checkedOrigin = (float *) DXAllocateLocal (masterN * sizeof (float))) == 
		NULL) {
	    status = ERROR;
	    break;
	}
	if ((masterDeltas = (float *) DXAllocateLocal (masterN * masterN * sizeof (float))) == 
		NULL) {
	    status = ERROR;
	    break;
	}
	if ((checkedDeltas = (float *) DXAllocateLocal (masterN * masterN * sizeof (float))) == 
		NULL) {
	    status = ERROR;
	    break;
	}

	if (DXQueryGridPositions (master, &masterN, 
		masterCounts, masterOrigin, masterDeltas) == NULL) {
	    DXSetError (ERROR_INTERNAL, 
		"ComparePositions, %d: Can't get regular array data", __LINE__);
	    status = ERROR;
	    break;
	}
	if (DXQueryGridPositions (checked, &checkedN, 
		checkedCounts, checkedOrigin, checkedDeltas) == NULL) {
	    DXSetError (ERROR_INTERNAL, 
		"ComparePositions, %d: Can't get regular array data", __LINE__);
	    status = ERROR;
	    break;
	}

	status = OK;
	for (i = 0; i < masterN && status == OK; ++i) {
	    if (masterCounts[i] != checkedCounts[i] ||
		masterOrigin[i] != checkedOrigin[i]) {
		status = ERROR;
	    }
	    for (j = 0; j < masterN && status == OK; ++j) {
		if (masterDeltas[i * masterN + j] != 
		    checkedDeltas[i * masterN + j]) {
		    status = ERROR;
		}
	    }
	}
	break;
    }

    DXFree ((Pointer) masterCounts);
    DXFree ((Pointer) checkedCounts);
    DXFree ((Pointer) masterOrigin);
    DXFree ((Pointer) checkedOrigin);
    DXFree ((Pointer) masterDeltas);
    DXFree ((Pointer) checkedDeltas);
    return (status);
}
#endif

/* 
 * SaveObjStruct records the structure of the output object, and returns
 * a tree of ObjStructs.  O should be a copy of the first input that is
 * not a trivial (unit length) array.
 */
static ObjStruct *
SaveObjStruct (Object o, int *id)
{
    ObjStruct *res = (ObjStruct *) DXAllocateZero(sizeof (ObjStruct));
    ObjStruct *so;		/* Subobject */
    int memInd;
    Object mem;
    Array a;
    ObjStruct **lastSubObjPtr;

    if (res) {
	res->output = o;
	res->members = NULL;
	res->parseTree = NULL;
	switch (res->class = DXGetObjectClass (o)) {
	case CLASS_MIN:		/* o was null */
	    res->class = CLASS_ARRAY;
	    res->metaType.items = 1;
	    break;

	case CLASS_ARRAY:
	    DXGetArrayInfo ((Array)o,
		&res->metaType.items,
		&res->metaType.type,
		&res->metaType.category,
		&res->metaType.rank,
		res->metaType.shape);
	    res->metaType.id = (*id)++;
	    break;

	case CLASS_STRING:
	    res->metaType.items = 1;
	    res->metaType.type = TYPE_STRING;
	    res->metaType.category = CATEGORY_REAL;
	    res->metaType.rank = 1;
	    res->metaType.shape[0] = strlen(DXGetString((String) o))+1;
	    res->metaType.id = (*id)++;
	    break;

	case CLASS_FIELD:
	    if ((a = (Array) DXGetComponentValue((Field)o, "data")) == NULL)
		a = (Array) DXGetComponentValue((Field)o, "positions");
	    if (a == NULL) {
		res->metaType.items = 0;
		res->metaType.type = TYPE_FLOAT;
		res->metaType.category = CATEGORY_REAL;
		res->metaType.rank = 1;
		res->metaType.shape[0] = 1;
	    }
	    else {
		DXGetArrayInfo (a,
		    &res->metaType.items,
		    &res->metaType.type,
		    &res->metaType.category,
		    &res->metaType.rank,
		    res->metaType.shape);
	    }
	    res->metaType.id = (*id)++;
	    break;

	case CLASS_GROUP:
	    /* Append each member 'so' to the end of res's member
	     * list
	     */
	    res->subClass = DXGetGroupClass((Group)o);
	    lastSubObjPtr = &res->members;
	    for (memInd = 0; 
		(mem = DXGetEnumeratedMember ((Group)o, memInd, (char **)NULL)) 
		    != NULL;
		++memInd) {
		so = SaveObjStruct ((Object)mem, id);
		if (!so)
		    goto SaveObjError;
		so->next = NULL;
		*lastSubObjPtr = so;
		lastSubObjPtr = &(so->next);
	    }
	    break;
	
	case CLASS_XFORM:
	    if (DXGetXformInfo((Xform)o, &mem, NULL) == NULL) {
		DXSetError (ERROR_BAD_PARAMETER, "#13210");
		goto SaveObjError;
	    }
	    res->members = SaveObjStruct ((Object)mem, id);
	    if (!res->members)
		goto SaveObjError;
	    res->members->next = NULL;
	    break;

	case CLASS_SCREEN:
	    if (DXGetScreenInfo((Screen)o, &mem, NULL, NULL) == NULL) {
		DXSetError (ERROR_BAD_PARAMETER, "#13240");
		goto SaveObjError;
	    }
	    res->members = SaveObjStruct ((Object)mem, id);
	    if (!res->members)
		goto SaveObjError;
	    res->members->next = NULL;
	    break;

	case CLASS_CLIPPED:
	    if (DXGetClippedInfo((Clipped)o, &mem, NULL) == NULL) {
		DXSetError (ERROR_BAD_PARAMETER, "#13220");
		goto SaveObjError;
	    }
	    res->members = SaveObjStruct ((Object)mem, id);
	    if (!res->members)
		goto SaveObjError;
	    res->members->next = NULL;
	    break;

	default:
	    DXSetError (ERROR_BAD_CLASS,
		"#10370", "input", "Array, Field, Group, Transform, Screen, or Clipped");
	    goto SaveObjError;
	}
    }
    return (res);

SaveObjError:
    DXFree((Pointer)res);
    return(NULL);
}

static int
MatchObjStruct (ObjStruct *master, Object o, int inputNum)
{
    ObjStruct *child;
    int memInd;
    Object mem;
    int items;
    Array a;
    Class class;

    if (master) {
	master->inputs[inputNum] = o;

	/* Classes must match, EXCEPT unit length arrays match anything */
	if (master->class != (class = DXGetObjectClass (o))) {
	    if (class == CLASS_ARRAY) {
		DXGetArrayInfo ((Array) o, &items, NULL, NULL, NULL, NULL);
		if (items != 1) {
		    DXErrorReturn (ERROR_BAD_CLASS, 
			"#11830");
		}
	    }
	    else
	    {
		if (class != CLASS_STRING)
		    DXErrorReturn (ERROR_BAD_CLASS, 
		    "#11830");
	    }
	}

	/* Perform class type checking */
	switch (master->class) {
	case CLASS_GROUP:
	    /* if the tested class is an array and we're here, the array
	     * is unit lenght, and should be matched with all subobjects 
	     */
	    if (class == CLASS_ARRAY) {
		for (child = master->members; 
			child != NULL;
			child = child->next) {
		    if (!MatchObjStruct (child, o, inputNum)) {
			return (ERROR);
		    }
		}
	    }
	    else {
		if (master->subClass != DXGetGroupClass((Group)o)) {
		    DXErrorReturn (ERROR_BAD_CLASS, 
			"#11830");
		}
		for (memInd = 0, child = master->members,
			(mem = DXGetEnumeratedMember ((Group)o, memInd, NULL));
		    child && mem;
		    ++memInd, child = child->next,
			(mem = DXGetEnumeratedMember ((Group)o, memInd, NULL))){
		    if (!MatchObjStruct (child, mem, inputNum)) {
			return (ERROR);
		    }
		}
		if (child != NULL || mem != NULL) {
		    DXErrorReturn (ERROR_BAD_PARAMETER, 
			"#11830");
		}
	    }
	    break;

	case CLASS_FIELD:
	    /* if the tested class is an array and we're here, the array
	     * is unit lenght, and should be matched with all subobjects 
	     */
	    if (class != CLASS_ARRAY) {
#ifdef CHECK_ANY_POSITIONS
		positions = (Array)DXGetComponentValue ((Field)o, "positions");
		masterPositions = (Array)DXGetComponentValue 
			((Field)master->output, "positions");
		if (ComparePositions (masterPositions, positions) == ERROR) {
		    DXWarning ("#11840");
		}
#endif

		a = (Array) DXGetComponentValue ((Field) o, "data");
		if (a == NULL) {
		    if (master->metaType.items != 0) {
			DXSetError(ERROR_BAD_PARAMETER, 
			    "#10240", "data");
			return ERROR;
		    }
		}
		else {
		    DXGetArrayInfo (a, &items, NULL, NULL, NULL, NULL);
		    if (items != master->metaType.items) {
			DXErrorReturn (ERROR_BAD_PARAMETER, 
			    "#11820");
		    }
		}
	    }
	    break;

	case CLASS_ARRAY:
	    DXGetArrayInfo ((Array) o, &items, NULL, NULL, NULL, NULL);
	    /* unit length arrays match with anything */
	    if (items != 1) {
		if (items != master->metaType.items) {
		    DXErrorReturn (ERROR_BAD_PARAMETER, 
			"#11820");
		}
	    }
	    break;

	case CLASS_STRING:
	    break;

	case CLASS_XFORM:
	    if (class != CLASS_ARRAY) {
		if (DXGetXformInfo((Xform)o, &mem, NULL) == NULL) {
		    DXSetError (ERROR_BAD_PARAMETER, "#13210");
		    return ERROR;
		}
	    }
	    else
		mem = o;
	    if (!MatchObjStruct(master->members, mem, inputNum)) {
		return ERROR;
	    }
	    break;

	case CLASS_SCREEN:
	    if (class != CLASS_ARRAY) {
		if (DXGetScreenInfo((Screen)o, &mem, NULL, NULL) == NULL) {
		    DXSetError (ERROR_BAD_PARAMETER, "#13240");
		    return ERROR;
		}
	    }
	    else
		mem = o;
	    if (!MatchObjStruct(master->members, mem, inputNum)) {
		return ERROR;
	    }
	    break;

	case CLASS_CLIPPED:
	    if (class != CLASS_ARRAY) {
		if (DXGetClippedInfo((Clipped)o, &mem, NULL) == NULL) {
		    DXSetError (ERROR_BAD_PARAMETER, "#13220");
		    return ERROR;
		}
	    }
	    else
		mem = o;
	    if (!MatchObjStruct(master->members, mem, inputNum)) {
		return ERROR;
	    }
	    break;

	default:
	    DXSetError (ERROR_BAD_CLASS,
		"#10370", "input", "Array, Field, Group, Transform, Screen, or Clipped");
	    return ERROR;
	}
    }
    return (OK);
}

void _dxfComputeFreeObjStruct (ObjStruct *os)
{
    ObjStruct *subObj;
    ObjStruct *nextObj;

    for (subObj = os->members; subObj != NULL; subObj = nextObj) {
	nextObj = subObj->next;
	_dxfComputeFreeObjStruct (subObj);
    }

    if (os->parseTree != NULL) {
	_dxfComputeFreeTree (os->parseTree);
    }

    DXFree ((Pointer)os);
}

/*
 * _ComputeGetInputs sets up the inputs array, which contains the type of each
 * input, and returns an ObjStruct tree which contains the output object
 * and pairings for actually doing work.
 */
ObjStruct *
_dxfComputeGetInputs(Object *in, CompInput *inputs)
{
    int i;
    ObjStruct *res = NULL;
    Object outputObject;
    int masterIndex;
    int masterItems;
    int id = 0;

    /* Find the first input that is a non-trivial (unit length) array, 
     * and set up a copy as the output array
     */
    for (masterIndex = i = 0; i < MAX_INPUTS; ++i) {
	if (in[i] == NULL) {
	    continue;
	}
	if (DXGetObjectClass(in[i]) == CLASS_ARRAY) {
	    DXGetArrayInfo ((Array)in[i], &masterItems, NULL, NULL, NULL, NULL);
	    if (masterItems == 1) {
		continue;
	    }
	}
	masterIndex = i;
	break;
    }
    if (in[masterIndex] == NULL ||
	    DXGetObjectClass(in[masterIndex]) == CLASS_ARRAY) {
	outputObject = in[masterIndex];
    }
    else {
	outputObject = DXCopy (in[masterIndex], COPY_STRUCTURE);
	if (outputObject == NULL)
	    return (NULL);
    }
	
    res = SaveObjStruct (outputObject, &id);
    if (!res)
	return (NULL);
	

    /* Now, collect the type information and verify the structure of all 
     * inputs.
     */
    for (i = 0; i < MAX_INPUTS; ++i) {
	if (inputs[i].used && in[i]) {
	    inputs[i].class = DXGetObjectClass (in[i]);
	    inputs[i].object = in[i];
	    /* Get type info */
	    if (MatchObjStruct (res, in[i], i) == ERROR) {
		if (i != masterIndex) {
		    DXAddMessage ("Input %d not matching the master (input %d)",
			i, masterIndex);
		}

		_dxfComputeFreeObjStruct(res);
		return (NULL);
	    }
	}
	else if (inputs[i].used) {
	    DXSetError (ERROR_BAD_PARAMETER, 
		"#11810", i);
	    _dxfComputeFreeObjStruct(res);
	    return (NULL);
	}
    }

    return (res);
}
#ifdef COMP_DEBUG
static char *
ClassString (Class class)
{
    char *cs;

    switch (class) {
    case CLASS_ARRAY: cs = "CLASS_ARRAY"; break;
    case CLASS_FIELD: cs = "CLASS_FIELD"; break;
    case CLASS_GROUP: cs = "CLASS_GROUP"; break;
    default:	      cs = "Unknown Class"; break;
    }

    return (cs);
}


static void
DumpObjStruct (ObjStruct *os, int indent)
{
    ObjStruct *subObj;
    static char *spaces = "                    ";
    char *ind = spaces + (strlen (spaces) - indent);
    int i;

    /* Print stuff about the os */

    if (os->class != CLASS_GROUP || os->subClass != CLASS_GROUP) {
	DXDebug ("C", "%sPseudo-leaf node 0x%08x, class = %d", 
	    ind, os, os->class);
	DXDebug ("C", "    items = %d", os->metaType.items);
	DXDebug ("C", "    type = %d", os->metaType.type);
	DXDebug ("C", "    category = %d", os->metaType.category);
	DXDebug ("C", "    rank = %d [", os->metaType.rank);
	for (i = 0; i < os->metaType.rank; ++i) {
	    DXDebug ("C", "        %d", os->metaType.shape[i]);
	}
	DXDebug ("C", "]");
    }
    else {
	DXDebug ("C", "%sHeterogeneous Group node 0x%08x", ind, os);
    }

    /* Recurse */
    for (subObj = os->members; subObj != NULL; subObj = subObj->next) {
	DumpObjStruct (subObj, indent + 4);
    }
}

void
_dxfComputeDumpInputs(CompInput *inputs, ObjStruct *os)
{
    int i;
    char *cs;

    for (i = 0; i < MAX_INPUTS; ++i) {
	if (inputs[i].used) {
	    cs = ClassString(inputs[i].class);
	    DXDebug ("C", "Input[%d] (%s)", i, cs);
	}
    }
    DXDebug ("C", "Object Structure Tree");
    DumpObjStruct (os, 0);
}
#endif
