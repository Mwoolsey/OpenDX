/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


/* This file contains the description of each operator from a type and 
 * execution point of view.  For each operator (such as COND (?:), *, 
 * min, [...], etc.) there is a structure containing the operator
 * code, name, and a list of valid operator bindings.  Each binding contains
 * the routine to implement the operation, a routine to check the type
 * of the operator, and type descriptors that the typeCheck routine may or
 * may not use.  For an operator, the bindings are searched sequentially
 * until one is found to match the given arguments.  If the search fails,
 * all arguments which are ints are promoted to floats, and the search is
 * repeated.  The typecheck routine that finds a match must load the parse
 * tree\'s type field.
 *
 * All of the operators defined herein function on vectors, and (if the 
 * compiler supports it) should be easily vectorisable and utilize the 
 * processor\'s floating point pipeline.
 *
 * Note that the routines defining the operation are followed by the
 * OperBinding structure, and that this structure is referenced by the
 * OperDesc structure defined just before the Externally Callable Routines.
 *
 * Note also that the OperBinding structures are ordered such that if
 * the inputs match a vector routine, that routine is called first.  If 
 * the inputs miss the vector binding, one vectors are treated as scalars,
 * later on.
 *
 * The file exists in several sections:
 *	Macros and utility routines.
 *	Structure operators (such as const, input, construct, select, and cond).
 *	Binary and unary operators (such as *, -, dot)
 *	Builtin functions (such as min, float, sin).
 *	Externally callable routines _dxfComputeTypeCheck, 
 *		_dxfComputeExecuteNode, and _dxfComputeLookupFunction
 *
 * To add a new operator: 
 *	The Parser must be changed to recognize it, and it must be added to
 * the set of operator codes in _compute.h
 * 	The OperBinding structure must be defined that
 * contains pointers to routines to implement and check the type must be
 * defined.
 *	This structure must be inserted into the OperDesc structure.
 *
 * Adding new built-ins is a little easier, because to make the parse
 * recognize it, it must be added to the list in _ComputeLookupFunction and
 * the set of builtin codes shown below.
 *
 * Several generic routines are provided to do typechecking, but new ones may
 * be needed for special case typechecking.  The extant routines are:
 *	CheckVects -- Check input for vectors of same length
 *	CheckSameShape -- Check input for same shaped objects
 *	CheckMatch -- Check that the inputs match the types specified in
 *		the binding structure.
 *	CheckSame -- Check that the inputs (any number) match the type
 *		of input[0], which is also the output type.
 *	CheckSameAs -- Check that the inputs (any number) match the type
 *		specified for input[0] in the binding structure.
 *	CheckSameSpecReturn -- Same as CheckSame, but use the output type
 *		given in the binding structure.
 *	CheckDimAnyShape -- matches scalars in the binding exactly, but 
 *		if the binding shape == 1, it allows any dimension and shape.
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <float.h>
#include <string.h>
#include <dx/dx.h>
#if defined(HAVE_CTYPE_H)
#include <ctype.h>
#endif
#include "_compute.h"
#include "_compputils.h"
#include "_compoper.h"
#include "vsincos.h"
#if !defined(DXD_STANDARD_IEEE)
#include <sys/signal.h>
#endif

/* A macro to ease the definition of the OperDesc field */
#define OP_RECORD(c, s, b) {(c), (s), (sizeof (b) / sizeof ((b)[0])), (b)}
#define OP_RECORD_SIZE(c, s, b, size) {(c), (s), 0, (b)}
static int TypeCheckLeaf(PTreeNode *tree, ObjStruct *input);

/* A readonly variable comparable to an integer */
static MetaType intExpr = {0, TYPE_INT, CATEGORY_REAL, 0};


/*
 * Routines to manipulate invalid data.
 */
Error _dxfComputeInvalidCopy(
    InvalidComponentHandle out,
    InvalidComponentHandle in0)
{
    int i;

    if (in0 == NULL)
	return OK;
    if (DXInitGetNextInvalidElementIndex(in0) == ERROR)
	return ERROR;
    while ((i = DXGetNextInvalidElementIndex(in0)) >= 0)
	DXSetElementInvalid(out, i);

    return OK;
}
int _dxfComputeInvalidOne(
    InvalidComponentHandle out,
    int indexout,
    InvalidComponentHandle in0,
    int index0)
{
    int result;

    if (in0 == NULL)
	return 1;

    if ((result = DXIsElementInvalidSequential(in0, index0)))
	DXSetElementInvalid(out, indexout);

    return !result;
}

int _dxfComputeInvalidOr(
    InvalidComponentHandle out,
    int indexout,
    InvalidComponentHandle in0,
    int index0,
    InvalidComponentHandle in1,
    int index1)
{
    int result;

    if (in0 == NULL || in1 == NULL)
	return 1;

    if ((result = (DXIsElementInvalidSequential(in0, index0) ||
		   DXIsElementInvalidSequential(in1, index1))))
	DXSetElementInvalid(out, indexout);

    return !result;
}


int
_dxfComputeCompareType(MetaType *left, MetaType *right)
{
    int i;

    if (left->type == right->type &&
	left->category == right->category &&
	left->rank == right->rank) {

	/* 03 Aug 2006 (jer) string types are equiv, even if their lengths are diff. */
	if(left->type == TYPE_STRING)
		return (OK);

	for (i = 0; i < left->rank; ++i) {
	    if (left->shape[i] != right->shape[i]) {
		return (ERROR);
	    }
	}
	return (OK);
    }
    else {
	return (ERROR);
    }
}

/* Checks a arguments to be sure all are same type 
 * as binding says arg0 should be.  It defines the result 
 * to be of speced rank (scalar or vector) from the binding structure. 
 */
int
_dxfComputeCheckStrings (PTreeNode *pt, ObjStruct *os, OperBinding *binding)
{
    int searchFailed;
    PTreeNode *arg;
    int j;
    int items = pt->metaType.items;

    searchFailed = 0;
    arg = pt->args;

    if (pt->args != NULL) {
	pt->metaType.shape[0] = pt->args->metaType.shape[0];
    }

    /* Search through argument list to ensure match */
    for (j = 0; !searchFailed && j < binding->numArgs && arg != NULL; ++j) {
	if (arg->metaType.rank != 1 ||
	    arg->metaType.type != binding->inTypes[j].type ||
	    arg->metaType.category != binding->inTypes[j].category) {

	    searchFailed = 1;
	}
	arg = arg->next;
    }
    if (!searchFailed) {
	if (arg != NULL || j != binding->numArgs) {
	    DXSetError(ERROR_BAD_TYPE, "#11945", binding->numArgs);
	    return (ERROR);
	}
	pt->impl = binding->impl;
	pt->metaType.type = binding->outType.type;
	pt->metaType.category = binding->outType.category;
	pt->metaType.rank = binding->outType.rank;
	pt->metaType.items = items;
	return (OK);
    }
    else {
	return (ERROR);
    }
}



/* This is the start of the Structure Operator Section */
static int
ExecConstant (
    PTreeNode *pt, 
    ObjStruct *os, 
    int numInputs,
    Pointer *inputs,
    Pointer result,
    InvalidComponentHandle *invalids,
    InvalidComponentHandle outInvalid)
{
    int size;

#ifdef COMP_DEBUG
    DXDebug ("C", "ExecConst, %d: pt = 0x%08x, os = 0x%08x, os items = %d, pt items = %d",
	__LINE__, pt, os, os->metaType.items, pt->metaType.items);
#endif
    
    /* Note that pt->metaType.items should always be 1 in this case. */
    if (pt->metaType.type == TYPE_STRING)
	size = pt->metaType.shape[0];
    else
        size = DXTypeSize (pt->metaType.type) *
	   DXCategorySize (pt->metaType.category);
    bcopy (&pt->u, ((char*)result), size);

    return (OK);
}

static int 
CheckConstant (PTreeNode *pt, ObjStruct *os, OperBinding *binding)
{
    /* pt already contains out type, return Success */
    pt->impl = binding->impl;
    return (OK);
}

static Pointer
ComputeProductArray_GetArrayData(ProductArray a, Pointer *p)
{
    int i=0, j, k, l, nterms, ndim, nl, nr, rank;
    Array *terms = NULL;
    float *result = (float*)p, *res, *right=NULL, *rt, *left, *lt;
    int freelocal = 0;
    int shape[100];
    int totalItems;

    DXGetProductArrayInfo(a, &nterms, NULL);
    terms = DXAllocateLocal(nterms * sizeof (Array));
    if (!terms)
	goto error;
    DXGetProductArrayInfo(a, NULL, terms);
    DXGetArrayInfo((Array)a, &totalItems, NULL, NULL, &rank, shape);
    for (i=0, ndim=1;  i<rank;  i++)
	ndim *= shape[0];

    /* first left operand is first term */
    left = (float *) DXGetArrayData(terms[0]);
    if (!left)
	goto error;
    DXGetArrayInfo(terms[0], &nl, NULL, NULL, NULL, NULL);
    /*ASSERT(terms[0]->shape[0]==ndim);*/

    /* copy first term data if nterms is 1 */
    /* XXX - is there any way to avoid this? */
    if (nterms==1) {

	int size = nl * ndim * sizeof(float);
	if (!result)
	    goto error;
	memcpy(result, left, size);

    /* accumulate product left to right */
    } else for (i=1; i<nterms; i++) {

	/* right operand is next term */
	right = (float *) DXGetArrayDataLocal(terms[i]);
	if (!right) {
	    freelocal = 0;
	    right = (float *) DXGetArrayData(terms[i]);
	    if (!right)
		goto error;
	} else
	    freelocal = 1;
	    
	DXGetArrayInfo(terms[i], &nr, NULL, NULL, NULL, NULL);
	/*ASSERT(terms[i]->shape[0]==ndim);*/
	
	/* allocate result */
	if (i < nterms - 1)
	    res = result = (float *) DXAllocate(nl*nr*ndim*sizeof(float));
	else
	    res = result = (float*)p;
	if (!result)
	    goto error;

	/* left operand plus right operand */
	switch (ndim) {
	case 1:
	    for (j=0, lt=left; j<nl; j++, lt+=ndim) {	/* ea left point */
		float t0 = lt[0];
		for (k=0, rt=right; k<nr; k++, rt+=ndim)/* ea right point */
		    *res++ = t0 + rt[0];		/* sum */
	    }
	    break;

	case 2:
	    /* left operand plus right operand */
	    for (j=0, lt=left; j<nl; j++, lt+=ndim) {	/* ea left point */
		float t0 = lt[0];
		float t1 = lt[1];
		for (k=0, rt=right; k<nr; k++, rt+=ndim) {/* ea right point */
		    *res++ = t0 + rt[0];		/* sum */
		    *res++ = t1 + rt[1];		/* sum */
		}
	    }
	    break;

	case 3:
	    /* left operand plus right operand */
	    for (j=0, lt=left; j<nl; j++, lt+=ndim) {	/* ea left point */
		float t0 = lt[0];
		float t1 = lt[1];
		float t2 = lt[2];
		for (k=0, rt=right; k<nr; k++, rt+=ndim) { /* ea right point */
		    *res++ = t0 + rt[0];		/* sum */
		    *res++ = t1 + rt[1];		/* sum */
		    *res++ = t2 + rt[2];		/* sum */
		}
	    }
	    break;

	default:
	    /* left operand plus right operand */
	    for (j=0, lt=left; j<nl; j++, lt+=ndim) {	/* ea left point */
		for (k=0, rt=right; k<nr; k++, rt+=ndim)/* ea right point */
		    for (l=0; l<ndim; l++)		/* ea dimension */
			*res++ = lt[l] + rt[l];		/* sum */
	    }
	    break;
	}

	/* next left operand is result */
	if (i>1)
	    DXFree((Pointer)left);
	left = result;
	nl *= nr;
	if (freelocal)
	    DXFreeArrayDataLocal(terms[i], (Pointer)right);
    }

/*     ASSERT(nl==totalItems); */

    DXFree((Pointer)terms);

    /* DXunlock and return */
    return (Pointer) result;

error:
    if (freelocal)
	DXFreeArrayDataLocal(terms[i], (Pointer)right);    /* and fall thru */
    DXFree((Pointer)terms);
    return NULL;
}

static int
ExecInput (
    PTreeNode *pt, 
    ObjStruct *os, 
    int numInputs,
    Pointer *inputs,
    Pointer result,
    InvalidComponentHandle *invalids,
    InvalidComponentHandle outInvalid)
{
    Object o;
    int i;
    int numBasic;
    int size;
    Pointer data;
    Pointer scratch;
    InvalidComponentHandle fieldInvalid = NULL;
    int status = OK;
    ArrayHandle arrayHandle = NULL;
    int items;

#ifdef COMP_DEBUG
    DXDebug ("C", "ExecInput, %d: pt = 0x%08x, os = 0x%08x, os items = %d, pt items = %d",
	__LINE__, pt, os, os->metaType.items, pt->metaType.items);
#endif

    /* Quit if this is some kind of empty field. */
    if (pt->metaType.items == 0)
	return OK;

    /* Ojbect is either an array or a field, make it an array, and
     * copy the array data into the result.
     */
    o = os->inputs[pt->u.i];
    if (DXGetObjectClass(o) == CLASS_FIELD) {
	Object a;
	char *dep;

	a = DXGetComponentAttribute((Field)o, "data", "dep");
	if (a && DXExtractString(a, &dep)) {
	    fieldInvalid = DXCreateInvalidComponentHandle(o, NULL, dep);
	    if (!fieldInvalid) {
		status = ERROR;
		goto error;
	    }
	}

	o = DXGetComponentValue ((Field)o, "data");
    }
    /* Copy input data into result array */
    for (numBasic = 1, i = 0; i < pt->metaType.rank; ++i) {
	numBasic *= pt->metaType.shape[i];
    }
    size = DXTypeSize (pt->metaType.type) *
	   DXCategorySize (pt->metaType.category) *
	   numBasic;
    switch (DXGetArrayClass((Array)o)) {

    case CLASS_STRING:
	strcpy(result, DXGetString((String)o));
	break;

    case CLASS_ARRAY:
	data = DXGetArrayData((Array)o);
	bcopy (data, result, pt->metaType.items * size);
	break;

    case CLASS_PRODUCTARRAY:
	if (ComputeProductArray_GetArrayData((ProductArray)o, result) == NULL) {
	    status = ERROR;
	    goto error;
	}
	break;
	    
    case CLASS_REGULARARRAY:
	if (pt->metaType.items == 1) {
	    if (DXGetRegularArrayInfo((RegularArray)o, NULL, result, NULL) 
		    == NULL) {
		DXSetError (ERROR_INTERNAL, 
		    "CheckInput, %d: Unable to get regular data", __LINE__);
		status = ERROR;
		goto error;
	    }
	}
	else
	    goto defaultCase;
	break;


    case CLASS_CONSTANTARRAY:
	data = DXGetConstantArrayData((Array)o);
	if (data == NULL) {
	    DXSetError (ERROR_INTERNAL, 
		"CheckInput, %d: Unable to get constant data", __LINE__);
	    status = ERROR;
	    goto error;
	}
	if (pt->metaType.items == 1)
	    bcopy (data, result, size);
	else
	    for (i = 0; i < pt->metaType.items; ++i)
		bcopy (data, ((char*)result) + i * size, size);
	break;

	/* ELSE Continue on for CLASS_PATHARRAY,
	 *    CLASS_PRODUCTARRAY, CLASS_MESHARRAY, or any one
	 *    we don't yet understand.
	 */
    default:
defaultCase:
	arrayHandle = DXCreateArrayHandle((Array)o);
	if (!arrayHandle) {
	    status = ERROR;
	    goto error;
	}
	scratch = DXAllocateLocal(size);
	if (!scratch) {
	    status = ERROR;
	    goto error;
	}
	items = pt->metaType.items;
	for (i = 0; i < items; ++i) {
	    data = DXGetArrayEntry(arrayHandle, i, scratch);
	    bcopy (data, ((char*)result) + i * size, size);
	}
	DXFree(scratch);
	break;
    }

error:
    if (arrayHandle) {
	DXFreeArrayHandle(arrayHandle);
    }
    if (fieldInvalid) {
	status = _dxfComputeInvalidCopy(outInvalid, fieldInvalid) && status;
	DXFreeInvalidComponentHandle(fieldInvalid);
    }

    return status;
}

static int
CheckInput (PTreeNode *pt, ObjStruct *os, OperBinding *binding)
{
    Array 	a;
    Object	attr;
    char 	*dep;
    int		invalidSize = 0;

    if (DXGetObjectClass (os->inputs[pt->u.i]) == CLASS_STRING) {
	pt->metaType.items = 1;
	pt->metaType.type = TYPE_STRING;
	pt->metaType.category = CATEGORY_REAL;
	pt->metaType.rank = 1;
	pt->metaType.shape[0] = strlen(DXGetString((String)os->inputs[pt->u.i]))+1;
        pt->impl = binding->impl;
        return (OK);
    }
    
    if (DXGetObjectClass (os->inputs[pt->u.i]) == CLASS_FIELD) {
	a = (Array)DXGetComponentValue ((Field)os->inputs[pt->u.i], "data");
    }
    else {
	a = (Array)os->inputs[pt->u.i];
    }
    if (a != NULL) {
	if (DXGetArrayInfo (	a,
			    &pt->metaType.items,
			    &pt->metaType.type, 
			    &pt->metaType.category, 
			    &pt->metaType.rank, 
			    pt->metaType.shape) == NULL) {
	    DXSetError (ERROR_INTERNAL, 
		"CheckInput, %d: Unable to get input type info", __LINE__);
	    return (ERROR);
	}
	if (DXGetObjectClass (os->inputs[pt->u.i]) == CLASS_FIELD) {
	    attr = DXGetComponentAttribute((Field)os->inputs[pt->u.i],
					   "data", "dep");
	    if (attr && DXExtractString(attr, &dep)) {
		char invalidName[1000];
		Array invalidArray;

		sprintf(invalidName, "invalid %s", dep);
		invalidArray = (Array)DXGetComponentValue(
		    (Field)os->inputs[pt->u.i], invalidName);
		if (invalidArray != NULL) {
		    if (DXGetArrayInfo (invalidArray,
					&invalidSize,
					NULL, 
					NULL, 
					NULL, 
					NULL) == NULL) {
			DXSetError (ERROR_INTERNAL, 
			    "CheckInput, %d: Unable to get input type info",
				__LINE__);
			return (ERROR);
		    }
		}
	    }
	}

	if (DXQueryConstantArray (a, NULL, NULL) && invalidSize == 0)
	    pt->metaType.items = 1;
    }
    else {
	pt->metaType.items    = 0;
	pt->metaType.type     = TYPE_FLOAT;
	pt->metaType.category = CATEGORY_REAL; 
	pt->metaType.rank     = 1;
	pt->metaType.shape[0] = 1;
    }
    pt->impl = binding->impl;
    return (OK);
}

static int
ExecConstruct (
    PTreeNode *pt, 
    ObjStruct *os, 
    int numInputs,
    Pointer *inputs,
    Pointer result,
    InvalidComponentHandle *invalids,
    InvalidComponentHandle outInvalid)
{
    int i, j;
    int numBasic;
    int size;
    int subSize;
    PTreeNode *arg;
    int items;
    int allValid;

#ifdef COMP_DEBUG
    DXDebug ("C", "ExecConstruct, %d: pt = 0x%08x, os = 0x%08x",
	__LINE__, pt, os);
#endif
    
    /* Get type of arguments, and allocate nargs of pointers.
     * For each arg, allocate a result array, and execute it
     */
    arg = pt->args;
    for (numBasic = 1, i = 0; i < arg->metaType.rank; ++i) {
	numBasic *= arg->metaType.shape[i];
    }
    subSize = DXTypeSize (arg->metaType.type) *
	   DXCategorySize (arg->metaType.category) *
	   numBasic;
    
    /* Put the results together into a list */
    size = subSize * numInputs;

    items = pt->metaType.items;
    for (i = 0; i < items; ++i) {
	arg = pt->args;
	for (j = 0; j < numInputs; ++j) {
	    allValid =
		(invalids[j] == NULL || DXGetInvalidCount(invalids[j]) == 0);
	    if (allValid || _dxfComputeInvalidOne(outInvalid, i, invalids[j], i)) {
		if (arg->metaType.items == 1)
		    bcopy ((Pointer)((char *)inputs[j]),
			   (Pointer)(((char *)result) + (i * size) + j*subSize),
			   subSize);
		else
		    bcopy ((Pointer)(((char *)inputs[j]) + (i * subSize)),
			   (Pointer)(((char *)result) + (i * size) + j*subSize),
			   subSize);
	    }
	    else
		break;
	    arg = arg->next;
	}
    }

    return (OK);
}

static int
CheckConstruct (PTreeNode *pt, ObjStruct *os, OperBinding *binding)
{
    PTreeNode *subTree;
    int listElement;
    int i;
    int seenScalar = 0;
    int seenOneVect = 0;

#ifdef COMP_DEBUG
    DXDebug ("C", "Check type of CA_CONSTRUCT node");
#endif
    /* Ensure that all elements of a vector are the same type.
     * Result is same type with rank and shape adjusted.
     * Number of elements is same as largest (if any != 0).
     */
    pt->metaType.type = pt->args->metaType.type;
    pt->metaType.category = pt->args->metaType.category;
    pt->metaType.rank = pt->args->metaType.rank;
    for (i = 0; i < pt->args->metaType.rank; ++i) {
	pt->metaType.shape[i] = pt->args->metaType.shape[i];
    }

    listElement = 0;
    subTree = pt->args;
    /* Resolve types of subtree */
    while (subTree != NULL) {
	if (subTree->metaType.rank == 0)
	    seenScalar = 1;
	else if (subTree->metaType.rank == 1 && 
		 subTree->metaType.shape[0] == 1)
	    seenOneVect = 1;
	if (_dxfComputeCompareType (&pt->metaType, &subTree->metaType) == 
		ERROR) {
	    if (!((subTree->metaType.rank == 0 && seenOneVect) ||
		((subTree->metaType.rank == 1 && 
		     subTree->metaType.shape[0] == 1) && seenScalar)))
		return (ERROR);
	}
	subTree = subTree->next;
	++listElement;
    }

    /* Adjust rank and shape */
    if (seenScalar && seenOneVect) {
	pt->metaType.shape[0] = listElement;
	pt->metaType.rank = 1;
    }
    else {
	for (i = pt->args->metaType.rank; i > 0 ; --i) {
	    pt->metaType.shape[i] = pt->metaType.shape[i-1];
	}
	pt->metaType.shape[0] = listElement;
	pt->metaType.rank++;
    }
    pt->impl = binding->impl;
    return (OK);
}

static int
ExecSelect (
    PTreeNode *pt, 
    ObjStruct *os, 
    int numInputs,
    Pointer *inputs,
    Pointer result,
    InvalidComponentHandle *invalids,
    InvalidComponentHandle outInvalid)
{
    int size0 = pt->args->metaType.items;
    int size1 = pt->args->next->metaType.items;
    int i, j, k;
    int numBasic;
    int size;
    int subSize;
    PTreeNode *vect;
    int vectLen;
    int selector;
    int items;
    int allValid =
	(invalids[0] == NULL || DXGetInvalidCount(invalids[0]) == 0) &&
	(invalids[1] == NULL || DXGetInvalidCount(invalids[1]) == 0);

#ifdef COMP_DEBUG
    DXDebug ("C", "ExecSelect, %d: pt = 0x%08x, os = 0x%08x",
	__LINE__, pt, os);
#endif
    
    /* Get type of arguments, and allocate nargs of pointers.
     * For each arg, allocate a result array, and execute it
     */
    vect = pt->args;
    if (vect->metaType.rank == 0) {
	subSize = DXTypeSize (vect->metaType.type) *
	       DXCategorySize (vect->metaType.category);
	size = subSize;
	vectLen = 1;
    }
    else {
	for (numBasic = 1, i = 1; i < vect->metaType.rank; ++i) {
	    numBasic *= vect->metaType.shape[i];
	}
	subSize = DXTypeSize (vect->metaType.type) *
	       DXCategorySize (vect->metaType.category) *
	       numBasic;
	
	/* Put the results together into a list */
	vectLen = vect->metaType.shape[0];
	size = subSize * vectLen;
    }

    /* For each item in list, select out the value */
    items = pt->metaType.items;
    for (i = j = k = 0;
	    i < items;
	    ++i) {
	if (allValid || _dxfComputeInvalidOr(outInvalid, i, invalids[0], j, invalids[1], k)) {
	    selector = *(int *)(((char *)inputs[1]) + k * sizeof (int));
	    if (selector < vectLen && selector >= 0) {
		bcopy ((Pointer)(((char *)inputs[0]) + 
			(j * size) + selector * subSize),
		   (Pointer)(((char *)result) + (i * subSize)),
		   subSize);
	    }
	    else {
		DXSetError (ERROR_BAD_TYPE, 
		    "#12010",
		    selector, vectLen);
		return (ERROR);
	    }
	}
	if (size0 != 1) ++j;
	if (size1 != 1) ++k;
    }

    return (OK);
}

static int
CheckSelect (PTreeNode *pt, ObjStruct *os, OperBinding *binding)
{
    PTreeNode *vect;
    PTreeNode *select;
    int i;

#ifdef COMP_DEBUG
    DXDebug ("C", "Check type of Selection node");
#endif
    /* Ensure that vect is at least a vector, and select is an int */
    vect = pt->args;
    if (vect == NULL) {
	DXSetError (ERROR_INTERNAL, "CheckSelect, %d: no vector expression",
	    __LINE__);
	return (ERROR);
    }

    select = vect->next;
    if (select == NULL) {
	DXSetError (ERROR_INTERNAL, "CheckSelect, %d: no select expression",
	    __LINE__);
	return (ERROR);
    }
    if (_dxfComputeCompareType (&intExpr, &select->metaType) == ERROR) {
	return (ERROR);
    }
    pt->metaType.type = vect->metaType.type;
    pt->metaType.category = vect->metaType.category;
    if (vect->metaType.rank != 0) {
	pt->metaType.rank = vect->metaType.rank - 1;
	for (i = 0; i < pt->metaType.rank; ++i) {
	    pt->metaType.shape[i] = vect->metaType.shape[i + 1];
	}
    }
    pt->impl = binding->impl;

    return (OK);
}

static int
ExecStrcmp(
    PTreeNode *pt, 
    ObjStruct *os, 
    int numInputs,
    Pointer *inputs,
    Pointer result,
    InvalidComponentHandle *invalids,
    InvalidComponentHandle outInvalid)
{
    register int *out = (int *) result;
    register char *in0 = (char *) inputs[0];
    register char *in1 = (char *) inputs[1];
    int vectorLen0 = pt->args->metaType.shape[0];
    int vectorLen1 = pt->args->next->metaType.shape[0];
    int size0 = pt->args->metaType.items;
    int size1 = pt->args->next->metaType.items;
    int i, j, k;
    int ncmp;
    int allValid =
	(invalids[0] == NULL || DXGetInvalidCount(invalids[0]) == 0) &&
	(invalids[1] == NULL || DXGetInvalidCount(invalids[1]) == 0);
    ncmp = MAX(vectorLen0, vectorLen1);
    for (i = j = k = 0;
	    i < pt->metaType.items;
	    ++i) {
	if (allValid || _dxfComputeInvalidOr(outInvalid, i, invalids[0], j, invalids[1], k)) {
		out[i] = strncmp(&in0[j*vectorLen0], &in1[k*vectorLen1], ncmp);
	}
	if (size0 != 1) ++j;
	if (size1 != 1) ++k;
    }
    return (OK);
}

static int
ExecStricmp(
    PTreeNode *pt, 
    ObjStruct *os, 
    int numInputs,
    Pointer *inputs,
    Pointer result,
    InvalidComponentHandle *invalids,
    InvalidComponentHandle outInvalid)
{
    register int *out = (int *) result;
    register char *in0 = (char *) inputs[0];
    register char *in1 = (char *) inputs[1];
    int vectorLen0 = pt->args->metaType.shape[0];
    int vectorLen1 = pt->args->next->metaType.shape[0];
    int size0 = pt->args->metaType.items;
    int size1 = pt->args->next->metaType.items;
    int i, j, k;
    int ncmp;
    int allValid =
	(invalids[0] == NULL || DXGetInvalidCount(invalids[0]) == 0) &&
	(invalids[1] == NULL || DXGetInvalidCount(invalids[1]) == 0);
    ncmp = MAX(vectorLen0, vectorLen1);
    for (i = j = k = 0;
	    i < pt->metaType.items;
	    ++i) {
	if (allValid || _dxfComputeInvalidOr(outInvalid, i, invalids[0], j, invalids[1], k)) {
	    int cc = 0;
	    char *s1 = &in0[j*vectorLen0];
	    char *s2 = &in1[k*vectorLen1];
	    for ( ; cc<ncmp && toupper(*s1) == toupper(*s2); cc++, s1++, s2++) {
		if (*s1 == '\0')
		    break;
	    }
	    out[i] = (cc == ncmp) ? 0 : (int) (toupper(*s1) - toupper(*s2));
	}
	if (size0 != 1) ++j;
	if (size1 != 1) ++k;
    }
    return (OK);
}

static int
ExecStrstr(
    PTreeNode *pt, 
    ObjStruct *os, 
    int numInputs,
    Pointer *inputs,
    Pointer result,
    InvalidComponentHandle *invalids,
    InvalidComponentHandle outInvalid)
{
    register int *out = (int *) result;
    register char *in0 = (char *) inputs[0];
    register char *in1 = (char *) inputs[1];
    int vectorLen0 = pt->args->metaType.shape[0];
    int vectorLen1 = pt->args->next->metaType.shape[0];
    int size0 = pt->args->metaType.items;
    int size1 = pt->args->next->metaType.items;
    int i, j, k;
    int allValid =
	(invalids[0] == NULL || DXGetInvalidCount(invalids[0]) == 0) &&
	(invalids[1] == NULL || DXGetInvalidCount(invalids[1]) == 0);
    /*ncmp = MAX(vectorLen0, vectorLen1);*/
    for (i = j = k = 0;
	    i < pt->metaType.items;
	    ++i) {
	if (allValid || _dxfComputeInvalidOr(outInvalid, i, invalids[0], j, invalids[1], k)) {
		char *s = strstr(&in0[j*vectorLen0], &in1[k*vectorLen1]);
		out[i] = (s == NULL) ? 0 : (int) (s - &in0[i*vectorLen0] + 1);
	}
	if (size0 != 1) ++j;
	if (size1 != 1) ++k;
    }
    return (OK);
}

static int
ExecStristr(
    PTreeNode *pt, 
    ObjStruct *os, 
    int numInputs,
    Pointer *inputs,
    Pointer result,
    InvalidComponentHandle *invalids,
    InvalidComponentHandle outInvalid)
{
    register int *out = (int *) result;
    register char *in0 = (char *) inputs[0];
    register char *in1 = (char *) inputs[1];
    int vectorLen0 = pt->args->metaType.shape[0];
    int vectorLen1 = pt->args->next->metaType.shape[0];
    int size0 = pt->args->metaType.items;
    int size1 = pt->args->next->metaType.items;
    int i, j, k;
    int ncmp;
    char *bigbuff;
    char *buff1, *buff2;
    int allValid =
	(invalids[0] == NULL || DXGetInvalidCount(invalids[0]) == 0) &&
	(invalids[1] == NULL || DXGetInvalidCount(invalids[1]) == 0);
    ncmp = MAX(vectorLen0, vectorLen1);
    bigbuff = DXAllocate(2*ncmp*sizeof(char));
    if (!bigbuff)
	return (ERROR);
    buff1 = &bigbuff[0];
    buff2 = &bigbuff[ncmp];
    for (i = j = k = 0;
	    i < pt->metaType.items;
	    ++i) {
	if (allValid || _dxfComputeInvalidOr(outInvalid, i, invalids[0], j, invalids[1], k)) {
		char *s, *s1, *s2;
		int cc, sz1, sz2;
		s1 = &in0[j*vectorLen0];
		sz1 = strlen(s1);
		s2 = &in1[k*vectorLen1];
		sz2 = strlen(s2);
		strcpy(buff1, s1);
		strcpy(buff2, s2);
		for (cc = 0, s = buff1; cc < sz1; s++, cc++)
		    *s = toupper(*s);
		for (cc = 0, s = buff2; cc < sz2; s++, cc++)
		    *s = toupper(*s);
		s = strstr(buff1, buff2);
		out[i] = (s == NULL) ? 0 : (int) (s - buff1 + 1);
	}
	if (size0 != 1) ++j;
	if (size1 != 1) ++k;
    }
    DXFree(bigbuff);
    return (OK);
}

static int
ExecStrlen(
    PTreeNode *pt, 
    ObjStruct *os, 
    int numInputs,
    Pointer *inputs,
    Pointer result,
    InvalidComponentHandle *invalids,
    InvalidComponentHandle outInvalid)
{
    register int *out = (int *) result;
    register char *in0 = (char *) inputs[0];
    int vectorLen0 = pt->args->metaType.shape[0];
    int size0 = pt->args->metaType.items;
    int i, j;
    int allValid =
	(invalids[0] == NULL || DXGetInvalidCount(invalids[0]) == 0);
    for (i = j = 0;
	    i < pt->metaType.items;
	    ++i) {
	if (allValid || _dxfComputeInvalidOne(outInvalid, i, invalids[0], j)) {
	    out[i] = strlen(&in0[j*vectorLen0]);
	}
	if (size0 != 1) ++j;
    }
    return (OK);
}


static int
ExecCond (
    PTreeNode *pt, 
    ObjStruct *os, 
    int numInputs,
    Pointer *inputs,
    Pointer result,
    InvalidComponentHandle *invalids,
    InvalidComponentHandle outInvalid)
{
    register int *cond = (int *) inputs[0];
    int size0 = pt->args->metaType.items;
    int size1 = pt->args->next->metaType.items;
    int size2 = pt->args->next->next->metaType.items;
    int i, j, k, l;
    PTreeNode *arg;
    int numBasic;
    int size;
    int tsize; /* size of then-expr values */
    int esize; /* size of else-expr values */
    int items;
    int allValid =
	(invalids[0] == NULL || DXGetInvalidCount(invalids[0]) == 0);

#ifdef COMP_DEBUG
    DXDebug ("C", "ExecCond, %d: pt = 0x%08x, os = 0x%08x",
	__LINE__, pt, os);
#endif

    arg = pt->args->next;
    for (numBasic = 1, i = 0; i < arg->metaType.rank; ++i) {
	numBasic *= arg->metaType.shape[i];
    }
    size = DXTypeSize (arg->metaType.type) *
	   DXCategorySize (arg->metaType.category) *
	   numBasic;

    /* 03 Aug 2006 (jer) Use separate bcopy sizes for then
     * and else values. Only matters when type is string,
     * and then/else sides have different shapes. In all other
     * cases, tsize, esize, and size are all equal.
     */
    tsize = esize = size;
    if(pt->metaType.type == TYPE_STRING){
	tsize = pt->args->next->metaType.shape[0];
	esize = pt->args->next->next->metaType.shape[0];
    }

    items = pt->metaType.items;
    for (i = j = k = l = 0;
	    i < items;
	    ++i) {
	if (allValid || _dxfComputeInvalidOne(outInvalid, i, invalids[0], j)) {
	    if (cond[j]) {
		if (_dxfComputeInvalidOne(outInvalid, i, invalids[1], k)) 
		    bcopy ((Pointer)(((char *)inputs[1]) + k * tsize),
			    (Pointer)(((char *)result) + i * size), size);
	    }
	    else {
		if (_dxfComputeInvalidOne(outInvalid, i, invalids[2], l)) 
		    bcopy ((Pointer)(((char *)inputs[2]) + l * esize),
			    (Pointer)(((char *)result) + i * size), size);
	    }
	}
	++j, ++k, ++l;
	if (j >= size0) j = 0;
	if (k >= size1) k = 0;
	if (l >= size2) l = 0;
    }

    return (OK);
}

static int
CheckCond (PTreeNode *pt, ObjStruct *os, OperBinding *binding)
{
    PTreeNode 	*condExpr;
    PTreeNode 	*thenExpr;
    PTreeNode 	*elseExpr;
    int		result;
    int		items;
    InvalidComponentHandle invalid = NULL;

#ifdef COMP_DEBUG
    DXDebug ("C", "Check type of Conditional node");
#endif
    pt->metaType.items = 0;

    /* Ensure that first arg is an int, and others are the same
     * Result is type of both arguments
     */
    condExpr = pt->args;
    if (condExpr == NULL) {
	DXSetError (ERROR_INTERNAL, "CheckCond, %d: no conditional expression",
	    __LINE__);
	goto error;
    }
    thenExpr = condExpr->next;
    if (thenExpr == NULL) {
	DXSetError (ERROR_INTERNAL, "CheckCond, %d: no then expression",
	    __LINE__);
	goto error;
    }
    elseExpr = thenExpr->next;
    if (elseExpr == NULL) {
	DXSetError (ERROR_INTERNAL, "CheckCond, %d: no else expression",
	    __LINE__);
	goto error;
    }

    if (TypeCheckLeaf(condExpr, os) != OK)
	goto error;
    if (pt->metaType.items < condExpr->metaType.items)
	pt->metaType.items = condExpr->metaType.items;

    if (_dxfComputeCompareType (&intExpr, &condExpr->metaType) == ERROR) {
	DXSetError (ERROR_BAD_TYPE, "#11985");
	goto error;
    }

    if (condExpr->metaType.items == 1) {
	if (_dxfComputeExecuteNode (condExpr, os, (Pointer) &result, &invalid)
		== ERROR)
	    goto error;
	if (invalid == NULL || DXGetInvalidCount(invalid) == 0) {
	    if (result) {
		if (TypeCheckLeaf(thenExpr, os) != OK) {
		    goto error;
		}
		pt->metaType = thenExpr->metaType;
		if (pt->metaType.items < condExpr->metaType.items)
		    pt->metaType.items = condExpr->metaType.items;
	    }
	    else {
		if (TypeCheckLeaf(elseExpr, os) != OK) {
		    goto error;
		}
		pt->metaType = elseExpr->metaType;
		if (pt->metaType.items < condExpr->metaType.items)
		    pt->metaType.items = condExpr->metaType.items;
	    }
	}
	else {
	    if (TypeCheckLeaf(thenExpr, os) != OK)
		goto error;
	    if (pt->metaType.items < thenExpr->metaType.items)
		pt->metaType.items = thenExpr->metaType.items;
	    if (TypeCheckLeaf(elseExpr, os) != OK)
		goto error;
	    if (pt->metaType.items < elseExpr->metaType.items)
		pt->metaType.items = elseExpr->metaType.items;
	    if (thenExpr->metaType.rank == 1 &&
		    thenExpr->metaType.shape[0] == 1)
		thenExpr->metaType.rank = 0;
	    if (elseExpr->metaType.rank == 1 &&
		    elseExpr->metaType.shape[0] == 1)
		elseExpr->metaType.rank = 0;
	    if (_dxfComputeCompareType (&thenExpr->metaType,
					&elseExpr->metaType) == ERROR)
		goto error;

	    items = pt->metaType.items;
	    pt->metaType = thenExpr->metaType;
	    pt->metaType.items = items;;
	}
    }
    else {
	if (TypeCheckLeaf(thenExpr, os) != OK)
	    goto error;
	if (pt->metaType.items < thenExpr->metaType.items)
	    pt->metaType.items = thenExpr->metaType.items;
	if (TypeCheckLeaf(elseExpr, os) != OK)
	    goto error;
	if (pt->metaType.items < elseExpr->metaType.items)
	    pt->metaType.items = elseExpr->metaType.items;
	if (thenExpr->metaType.rank == 1 &&
		thenExpr->metaType.shape[0] == 1)
	    thenExpr->metaType.rank = 0;
	if (elseExpr->metaType.rank == 1 &&
		elseExpr->metaType.shape[0] == 1)
	    elseExpr->metaType.rank = 0;
	if (_dxfComputeCompareType (&thenExpr->metaType,
				    &elseExpr->metaType) == ERROR) {
	    DXSetError(ERROR_BAD_TYPE, "#11986");
	    goto error;
	}

	items = pt->metaType.items;
	pt->metaType = thenExpr->metaType;
	pt->metaType.items = items;
    }
    pt->impl = binding->impl;

    /* 03 Aug 2006 (jer) set the result shape */
    if(pt->metaType.type == TYPE_STRING){
        pt->metaType.shape[0] = 
            (thenExpr->metaType.shape[0] > elseExpr->metaType.shape[0]) ?
		thenExpr->metaType.shape[0] : elseExpr->metaType.shape[0];
    }

    if (invalid) {
	DXFreeInvalidComponentHandle(invalid);
    }
    return (OK);
error:
    if (invalid) {
	DXFreeInvalidComponentHandle(invalid);
    }
    return (ERROR);
}

typedef struct SymTab {
    char    *name;
    Pointer  value;
    InvalidComponentHandle invalid;
    MetaType metaType;
} SymTab;

static int     symbolTableSize = 0;
static int     symbolTableUsed = 0;
static SymTab *symbolTable = NULL;

static int
AddSymbol(char *name, MetaType *type)
{
    int i;
    for (i = 0; i < symbolTableUsed; ++i)
	if (strcmp(name, symbolTable[i].name) == 0) {
	    symbolTable[i].metaType = *type;
	    return OK;
	}
    
    /* We have to extend the symbol table */
    if (symbolTableSize == 0)
	symbolTable = (SymTab*)DXAllocateLocal(5 * sizeof (SymTab));
    else
	symbolTable = (SymTab*)DXReAllocate((Pointer)symbolTable,
	    (symbolTableSize + 5) * sizeof (SymTab));
    symbolTableSize += 5;

    symbolTable[symbolTableUsed].name = (char*)DXAllocateLocal(strlen(name)+1);
    if (symbolTable[symbolTableUsed].name == NULL)
	return ERROR;
    strcpy(symbolTable[symbolTableUsed].name, name);
    symbolTable[symbolTableUsed].metaType = *type;
    symbolTable[symbolTableUsed].value = NULL;
    symbolTable[symbolTableUsed].invalid = NULL;
    symbolTableUsed++;
    return OK;
}

static int
DefineSymbol(char *name, ObjStruct *os, Pointer value,
	     InvalidComponentHandle invalid)
{
    int i, j;
    int size;
    Object o;

    for (i = 0; i < symbolTableUsed; ++i)
	if (strcmp(name, symbolTable[i].name) == 0) {
	    if (symbolTable[i].value)
		DXFree((Pointer)symbolTable[i].value);
	    if (symbolTable[i].invalid)
		DXFreeInvalidComponentHandle(symbolTable[i].invalid);
	    symbolTable[i].value = NULL;
	    symbolTable[i].invalid = NULL;

	    size = DXTypeSize(symbolTable[i].metaType.type) * 
		   DXCategorySize(symbolTable[i].metaType.category) *
		   symbolTable[i].metaType.items;
	    for (j = 0; j < symbolTable[i].metaType.rank; ++j)
		size *= symbolTable[i].metaType.shape[j];

	    symbolTable[i].value = DXAllocateLocal(size);
	    if (symbolTable[i].value == NULL) {
		DXResetError();
		symbolTable[i].value = DXAllocate(size);
	    }
	    if (symbolTable[i].value == NULL) {
		return ERROR;
	    }
	    bcopy((char*)value, (char*)symbolTable[i].value, size);
	    o = os->output;
	    if (invalid && DXGetObjectClass(o) == CLASS_FIELD) {
		Object a;
		char *dep;

		a = DXGetComponentAttribute((Field)o, "data", "dep");
		if (a && DXExtractString(a, &dep)) {
		    symbolTable[i].invalid =
			DXCreateInvalidComponentHandle(o, NULL, dep);
		    if (!symbolTable[i].invalid) {
			return ERROR;
		    }
		}
		return _dxfComputeInvalidCopy(symbolTable[i].invalid,
		    invalid);
	    }
	    else
		return OK;
	}
    DXSetError(ERROR_INTERNAL, "Defining an unadded symbol `%s'", name);
    return ERROR;
}

static void
ClearSymbolTable()
{
    int i;
    for (i = 0; i < symbolTableUsed; ++i) {
	DXFree((Pointer)symbolTable[i].name);
	DXFree((Pointer)symbolTable[i].value);
	DXFreeInvalidComponentHandle(symbolTable[i].invalid);
    }
    DXFree((Pointer)symbolTable);
    symbolTable = NULL;
    symbolTableUsed = symbolTableSize = 0;
}

static int
GetSymbol(char *name, Pointer *p, InvalidComponentHandle *invalid)
{
    int i;
    for (i = 0; i < symbolTableUsed; ++i) {
	if (strcmp(name, symbolTable[i].name) == 0) {
	    *p = symbolTable[i].value;
	    *invalid = symbolTable[i].invalid;
	    return OK;
	}
    }
    return ERROR;
}

static int
GetSymbolType(char *name, MetaType **metaType)
{
    int i;
    for (i = 0; i < symbolTableUsed; ++i) {
	if (strcmp(name, symbolTable[i].name) == 0) {
	    *metaType = &symbolTable[i].metaType;
	    return OK;
	}
    }
    return ERROR;
}

static int
ExecAssignment (
    PTreeNode *pt, 
    ObjStruct *os, 
    int numInputs,
    Pointer *inputs,
    Pointer result,
    InvalidComponentHandle *invalids,
    InvalidComponentHandle outInvalid)
{
    if (DefineSymbol(pt->u.s, os, inputs[0], invalids[0]) == ERROR)
	return ERROR;

    return _dxfComputeCopy(pt,
			   os,
			   numInputs,
			   inputs,
			   result,
			   invalids,
			   outInvalid);
}

static int
CheckAssignment (PTreeNode *pt, ObjStruct *os, OperBinding *binding)
{
    if (AddSymbol(pt->u.s, &pt->args->metaType) == ERROR)
	return ERROR;
    pt->metaType = pt->args->metaType;
    pt->impl = binding->impl;

    return OK;
}

static int
ExecVariable (
    PTreeNode *pt, 
    ObjStruct *os, 
    int numInputs,
    Pointer *inputs,
    Pointer result,
    InvalidComponentHandle *invalids,
    InvalidComponentHandle outInvalid)
{
    Pointer p;
    InvalidComponentHandle definedInvalid;

    if (GetSymbol(pt->u.s, &p, &definedInvalid) == ERROR) {
	DXSetError(ERROR_INTERNAL, "Refering to an unadded symbol `%s'", pt->u.s);
	return ERROR;
    }

    return _dxfComputeCopy(pt,
			   os,
			   1,
			   &p,
			   result,
			   &definedInvalid,
			   outInvalid);
}

static int
CheckVariable (PTreeNode *pt, ObjStruct *os, OperBinding *binding)
{
    MetaType *t;

    if (GetSymbolType(pt->u.s, &t) == ERROR) {
	DXSetError(ERROR_BAD_PARAMETER, "#12091", pt->u.s);
	return ERROR;
    }
    pt->metaType = *t;
    pt->impl = binding->impl;

    return OK;
}

static OperBinding inputs[] = {
    {0, (CompFuncV)ExecInput, CheckInput}
};
static OperBinding consts[] = {
    {0, (CompFuncV)ExecConstant, CheckConstant}
};
static OperBinding construct[] = {
    {-1, (CompFuncV)ExecConstruct, CheckConstruct}

};
static OperBinding cond[] = {
    {3, (CompFuncV)ExecCond, CheckCond}
};
static OperBinding selects[] = {
    {2, (CompFuncV)ExecSelect, CheckSelect}
};
static OperBinding strcmps[] = {
    {2, (CompFuncV)ExecStrcmp, _dxfComputeCheckStrings,
	{0, TYPE_INT, CATEGORY_REAL, 0 },
	{   {0, TYPE_STRING, CATEGORY_REAL},
	    {0, TYPE_STRING, CATEGORY_REAL}}},
};
static OperBinding stricmps[] = {
    {2, (CompFuncV)ExecStricmp, _dxfComputeCheckStrings,
	{0, TYPE_INT, CATEGORY_REAL, 0 },
	{   {0, TYPE_STRING, CATEGORY_REAL},
	    {0, TYPE_STRING, CATEGORY_REAL}}},
};
static OperBinding strstrs[] = {
    {2, (CompFuncV)ExecStrstr, _dxfComputeCheckStrings,
	{0, TYPE_INT, CATEGORY_REAL, 0 },
	{   {0, TYPE_STRING, CATEGORY_REAL},
	    {0, TYPE_STRING, CATEGORY_REAL}}},
};
static OperBinding stristrs[] = {
    {2, (CompFuncV)ExecStristr, _dxfComputeCheckStrings,
	{0, TYPE_INT, CATEGORY_REAL, 0 },
	{   {0, TYPE_STRING, CATEGORY_REAL},
	    {0, TYPE_STRING, CATEGORY_REAL}}},
};
static OperBinding strlens[] = {
    {1, (CompFuncV)ExecStrlen, _dxfComputeCheckStrings,
	{0, TYPE_INT, CATEGORY_REAL, 0 },
	{ {0, TYPE_STRING, CATEGORY_REAL}}},
};

static OperBinding assignments[] = {
    {1, (CompFuncV)ExecAssignment, CheckAssignment}
};
static OperBinding variables[] = {
    {0, (CompFuncV)ExecVariable, CheckVariable}
};

/* Checks a arguments to be sure all are vectors of same type and Category
 * as binding says arg0 should be, and same length.  It defines the result 
 * to be of speced rank (scalar or vector) from the binding structure, 
 * and if vector, the same length as the inputs.
 */
int
_dxfComputeCheckVects (PTreeNode *pt, ObjStruct *os, OperBinding *binding)
{
    int searchFailed;
    PTreeNode *arg;
    int j;
    int items = pt->metaType.items;

    searchFailed = 0;
    arg = pt->args;

    if (pt->args != NULL) {
	pt->metaType.shape[0] = pt->args->metaType.shape[0];
    }

    /* Search through argument list to ensure match */
    for (j = 0; !searchFailed && j < binding->numArgs && arg != NULL; ++j) {
	if (arg->metaType.rank != 1 ||
	    arg->metaType.shape[0] != pt->metaType.shape[0] ||
	    arg->metaType.type != binding->inTypes[j].type ||
	    arg->metaType.category != binding->inTypes[j].category) {

	    searchFailed = 1;
	}
	arg = arg->next;
    }
    if (!searchFailed) {
	if (arg != NULL || j != binding->numArgs) {
	    DXSetError(ERROR_BAD_TYPE, "#11945", binding->numArgs);
	    return (ERROR);
	}
	pt->impl = binding->impl;
	pt->metaType.type = binding->outType.type;
	pt->metaType.category = binding->outType.category;
	pt->metaType.rank = binding->outType.rank;
	pt->metaType.items = items;
	return (OK);
    }
    else {
	return (ERROR);
    }
}

/* Checks a arguments to be sure all are objects of same type and Category
 * as binding says they should be, and rank and shape.  It defines the result 
 * to be of input cat, type, rank and shape.
 */
int
_dxfComputeCheckSameShape (PTreeNode *pt, ObjStruct *os, OperBinding *binding)
{
    int searchFailed;
    PTreeNode *arg;
    int j;
    int i;
    int items = pt->metaType.items;

    searchFailed = 0;
    arg = pt->args;

    if (pt->args != NULL) {
	pt->metaType = pt->args->metaType;
	pt->metaType.items = items;
    }

    /* Search through argument list to ensure match */
    for (j = 0; !searchFailed && j < binding->numArgs && arg != NULL; ++j) {
	if (arg->metaType.rank != pt->metaType.rank ||
	    arg->metaType.type != binding->inTypes[j].type ||
	    ! (binding->inTypes[j].category != (Category) -1? 
		arg->metaType.category == binding->inTypes[j].category:
		arg->metaType.category == pt->metaType.category)) {

	    searchFailed = 1;
	}
	else {
	    for (i = 0; i < arg->metaType.rank; ++i) {
		if (arg->metaType.shape[i] != pt->metaType.shape[i]) {
		    searchFailed = 1;
		}
	    }
	}
	arg = arg->next;
    }
    if (!searchFailed) {
	if (arg != NULL || j != binding->numArgs) {
	    DXSetError(ERROR_BAD_TYPE, "#11945", binding->numArgs);
	    return (ERROR);
	}
	pt->impl = binding->impl;
	pt->metaType.type = binding->outType.type;
	if (binding->outType.category != (Category)-1)
	    pt->metaType.category = binding->outType.category;
	return (OK);
    }
    else {
	return (ERROR);
    }
}

/* Checks a arguments to be sure all are objects of same type, Category, and 
 * rank (if scalar) as binding says they should be, and any shape allowed.  
 * If bindings says rank == 1, no scalars are allowed.
 * Output is that rank and shape of first non-scalar in binding.
 */
int
_dxfComputeCheckDimAnyShape (PTreeNode *pt, ObjStruct *os, OperBinding *binding)
{
    int searchFailed;
    PTreeNode *arg;
    int j;
    int first = 1;
    int items = pt->metaType.items;

    searchFailed = 0;
    arg = pt->args;

    if (pt->args != NULL) {
	pt->metaType = pt->args->metaType;
	pt->metaType.items = items;
    }

    /* Search through argument list to ensure match */
    for (j = 0; !searchFailed && j < binding->numArgs && arg != NULL; ++j) {
	/* if binding says scalar, compare */
	if (binding->inTypes[j].rank == 0) {
	    searchFailed = !_dxfComputeCompareType (&binding->inTypes[j], &arg->metaType);
	}
	else if (arg->metaType.type != binding->inTypes[j].type ||
		 arg->metaType.category != binding->inTypes[j].category) {
	    searchFailed = 1;
	}
	else if (arg->metaType.rank == 0) {
	    return (ERROR);
	}
	else {
	    if (first) {
		pt->metaType = arg->metaType;
		pt->metaType.items = items;
		first = 0;
	    }
	}
	arg = arg->next;
    }
    if (!searchFailed) {
	if (arg != NULL || j != binding->numArgs) {
	    DXSetError(ERROR_BAD_TYPE, "#11945", binding->numArgs);
	    return (ERROR);
	}
	pt->impl = binding->impl;
	return (OK);
    }
    else {
	return (ERROR);
    }
}

/*
 * Check that the inputs match the types specified in the binding structure.
 * Return the type specified in the binding structure.
 */
int
_dxfComputeCheckMatch (PTreeNode *pt, ObjStruct *os, OperBinding *binding)
{
    int searchFailed;
    PTreeNode *arg;
    int j;
    int items = pt->metaType.items;

    searchFailed = 0;
    arg = pt->args;

    /* Search through argument list to ensure match */
    for (j = 0; !searchFailed && j < binding->numArgs && arg != NULL; ++j) {
	if (!_dxfComputeCompareType (
		&binding->inTypes[j],
		&arg->metaType)) {
	    searchFailed = 1;
	}
	arg = arg->next;
    }
    if (!searchFailed) {
	if (arg != NULL || j != binding->numArgs) {
	    DXSetError(ERROR_BAD_TYPE, "#11945", binding->numArgs);
	    return (ERROR);
	}
	pt->impl = binding->impl;
	pt->metaType = binding->outType;
	pt->metaType.items = items;
	return (OK);
    }
    else {
	return (ERROR);
    }
}

/*
 * Check that the inputs (any number > 1) match the type
 * of input[0], which is also used as the output type.
 */
int
_dxfComputeCheckSame (PTreeNode *pt, ObjStruct *os, OperBinding *binding)
{
    PTreeNode *subTree;
    int listElement;
    int items = pt->metaType.items;
    int count;

    /* Count number of arguments (ensure match) */
    if (binding->numArgs != -1) {
	for (count = 0, subTree = pt->args; 
	     subTree != NULL; 
	     ++count, subTree = subTree->next)
	;
	if (count != binding->numArgs) {
	    DXSetError(ERROR_BAD_TYPE, "#11945", binding->numArgs);
	    return (ERROR);
	}
    }
    else if (pt->args == NULL) {
	return (ERROR);
    }

    /* Ensure that all elements of a vector are the same type.
     * Result is same type with rank and shape adjusted.
     * Number of elements is same as largest (if any != 0).
     */

    pt->metaType = pt->args->metaType;
    pt->metaType.items = items;

    listElement = 0;
    subTree = pt->args;
    /* Resolve types of subtree */
    while (subTree != NULL) {
	if (_dxfComputeCompareType (&pt->metaType, &subTree->metaType) == 
		ERROR) {
	    return (ERROR);
	}
	subTree = subTree->next;
	++listElement;
    }

    if (listElement != binding->numArgs) {
	DXSetError(ERROR_BAD_TYPE, "#11945", binding->numArgs);
	return (ERROR);
    }
    else {
	pt->impl = binding->impl;
	return (OK);
    }
}

/*
 * Check that the inputs (any number) match the type specified in the binding
 * structure for input[0], which is also used as the output type.
 */
int
_dxfComputeCheckSameAs (PTreeNode *pt, ObjStruct *os, OperBinding *binding)
{
    PTreeNode *subTree;
    int listElement;
    int items = pt->metaType.items;
    int count;

    /* Count number of arguments (ensure match) */
    if (binding->numArgs != -1) {
	for (count = 0, subTree = pt->args; 
	     subTree != NULL; 
	     ++count, subTree = subTree->next)
	;
	if (count != binding->numArgs) {
	    DXSetError(ERROR_BAD_TYPE, "#11945", binding->numArgs);
	    return (ERROR);
	}
    }
    else if (pt->args == NULL) {
	return (ERROR);
    }

    /* Ensure that all elements of a vector are the same type.
     * Result is same type with rank and shape adjusted.
     * Number of elements is same as largest (if any != 0).
     */
    pt->metaType = binding->outType;
    pt->metaType.items = items;

    listElement = 0;
    subTree = pt->args;
    /* Resolve types of subtree */
    while (subTree != NULL) {
	if (_dxfComputeCompareType (&binding->inTypes[0], &subTree->metaType) == 
		ERROR) {
	    return (ERROR);
	}
	subTree = subTree->next;
	++listElement;
    }

    pt->impl = binding->impl;
    return (OK);
}

/*
 * Check that the inputs (any number) match the type of input[0], like 
 * CheckSame, but use the outputType specified in the binding structure.
 */
int
_dxfComputeCheckSameSpecReturn (PTreeNode *pt, ObjStruct *os, OperBinding *binding)
{
    PTreeNode *subTree;
    int listElement;
    int items = pt->metaType.items;
    int count;

    /* Count number of arguments (ensure match) */
    if (binding->numArgs != -1) {
	for (count = 0, subTree = pt->args; 
	     subTree != NULL; 
	     ++count, subTree = subTree->next)
	;
	if (count != binding->numArgs) {
	    DXSetError(ERROR_BAD_TYPE, "#11945", binding->numArgs);
	    return (ERROR);
	}
    }
    else if (pt->args == NULL) {
	return (ERROR);
    }

    /* Ensure that all elements of a vector are the same type.
     * Result is same type with rank and shape adjusted.
     * Number of elements is same as largest (if any != 0).
     */
    pt->metaType = pt->args->metaType;
    pt->metaType.items = items;

    listElement = 0;
    subTree = pt->args;
    /* Resolve types of subtree */
    while (subTree != NULL) {
	if (_dxfComputeCompareType (&pt->metaType, &subTree->metaType) == 
		ERROR) {
	    return (ERROR);
	}
	subTree = subTree->next;
	++listElement;
    }

    pt->metaType = binding->outType;
    pt->metaType.items = items;
    pt->impl = binding->impl;
    return (OK);
}
/*
 * Check that the inputs (any number) match the type of input[0], like 
 * CheckSame, have the same Type specified in the binding, but use 
 * the outputType specified in the binding structure.
 */
int
_dxfComputeCheckSameTypeSpecReturn (PTreeNode *pt, ObjStruct *os, OperBinding *binding)
{
    PTreeNode *subTree;
    int listElement;
    int items = pt->metaType.items;
    int count;

    /* Count number of arguments (ensure match) */
    if (binding->numArgs != -1) {
	for (count = 0, subTree = pt->args; 
	     subTree != NULL; 
	     ++count, subTree = subTree->next)
	;
	if (count != binding->numArgs) {
	    DXSetError(ERROR_BAD_TYPE, "#11945", binding->numArgs);
	    return (ERROR);
	}
    }
    else if (pt->args == NULL) {
	return (ERROR);
    }

    /* Ensure that all elements of a vector are the same type.
     * Result is same type with rank and shape adjusted.
     * Number of elements is same as largest (if any != 0).
     */
    pt->metaType = pt->args->metaType;
    pt->metaType.items = items;

    listElement = 0;
    subTree = pt->args;
    /* Resolve types of subtree */
    while (subTree != NULL) {
	if (subTree->metaType.type != binding->inTypes[0].type ||
	    _dxfComputeCompareType (&pt->metaType, &subTree->metaType) == ERROR) {
	    return (ERROR);
	}
	subTree = subTree->next;
	++listElement;
    }

    pt->metaType = binding->outType;
    pt->metaType.items = items;
    pt->impl = binding->impl;
    return (OK);
}


/* 
 * Now follows the set of binary and unary operators */
int
_dxfComputeCopy(
    PTreeNode *pt, 
    ObjStruct *os, 
    int numInputs,
    Pointer *inputs,
    Pointer result,
    InvalidComponentHandle *invalids,
    InvalidComponentHandle outInvalid)	
{
    int numBasic;
    int i;
    int size;

    for (numBasic = 1, i = 0; i < pt->metaType.rank; ++i) {
	numBasic *= pt->metaType.shape[i];
    }
    size = DXTypeSize (pt->metaType.type) *
	   DXCategorySize (pt->metaType.category) *
	   numBasic;

    if (pt->args != NULL && pt->args->metaType.items == 1)
	for (i = 0; i < pt->metaType.items; ++i)
	    bcopy (inputs[0], ((char*)result) + i * size, size);
    else
	bcopy (inputs[0], result, size * pt->metaType.items);

    return _dxfComputeInvalidCopy(outInvalid, invalids[0]);
}


VBINOP (mulVUBUB, unsigned char, unsigned char, unsigned char, *)
VSBINOP (mulVSUBUB, unsigned char, unsigned char, unsigned char, *)
SVBINOP (mulSVUBUB, unsigned char, unsigned char, unsigned char, *)
VBINOP (mulVBB, signed char, signed char, signed char, *)
VSBINOP (mulVSBB, signed char, signed char, signed char, *)
SVBINOP (mulSVBB, signed char, signed char, signed char, *)

VBINOP (mulVUSUS, unsigned short, unsigned short, unsigned short, *)
VSBINOP (mulVSUSUS, unsigned short, unsigned short, unsigned short, *)
SVBINOP (mulSVUSUS, unsigned short, unsigned short, unsigned short, *)
VBINOP (mulVSS, signed short, signed short, signed short, *)
VSBINOP (mulVSSS, signed short, signed short, signed short, *)
SVBINOP (mulSVSS, signed short, signed short, signed short, *)

VBINOP (mulVUIUI, unsigned int, unsigned int, unsigned int, *)
VSBINOP (mulVSUIUI, unsigned int, unsigned int, unsigned int, *)
SVBINOP (mulSVUIUI, unsigned int, unsigned int, unsigned int, *)
VBINOP (mulVII, signed int, signed int, signed int, *)
VSBINOP (mulVSII, signed int, signed int, signed int, *)
SVBINOP (mulSVII, signed int, signed int, signed int, *)

VSBINOP (mulVSFF, float, float, float, *)
SVBINOP (mulSVFF, float, float, float, *)
VBINOP (mulVFF, float, float, float, *)

VSBINOP (mulVSDD, double, double, double, *)
SVBINOP (mulSVDD, double, double, double, *)
VBINOP (mulVDD, double, double, double, *)

VFUNC2 (mulVFFC, complexFloat, complexFloat, complexFloat, _dxfComputeMulComplexFloat)
VSFUNC2 (mulVSFFC, complexFloat, complexFloat, complexFloat, _dxfComputeMulComplexFloat)
SVFUNC2 (mulSVFFC, complexFloat, complexFloat, complexFloat, _dxfComputeMulComplexFloat)
static OperBinding muls[] = {
    {2, (CompFuncV)mulVSUBUB, _dxfComputeCheckDimAnyShape,
	{0, TYPE_UBYTE, CATEGORY_REAL, 1},
	{   {0, TYPE_UBYTE, CATEGORY_REAL, 1},
	    {0, TYPE_UBYTE, CATEGORY_REAL, 0}}},
    {2, (CompFuncV)mulVSBB, _dxfComputeCheckDimAnyShape,
	{0, TYPE_BYTE, CATEGORY_REAL, 1},
	{   {0, TYPE_BYTE, CATEGORY_REAL, 1},
	    {0, TYPE_BYTE, CATEGORY_REAL, 0}}},
    {2, (CompFuncV)mulVSUSUS, _dxfComputeCheckDimAnyShape,
	{0, TYPE_USHORT, CATEGORY_REAL, 1},
	{   {0, TYPE_USHORT, CATEGORY_REAL, 1},
	    {0, TYPE_USHORT, CATEGORY_REAL, 0}}},
    {2, (CompFuncV)mulVSSS, _dxfComputeCheckDimAnyShape,
	{0, TYPE_SHORT, CATEGORY_REAL, 1},
	{   {0, TYPE_SHORT, CATEGORY_REAL, 1},
	    {0, TYPE_SHORT, CATEGORY_REAL, 0}}},
    {2, (CompFuncV)mulVSUIUI, _dxfComputeCheckDimAnyShape,
	{0, TYPE_UINT, CATEGORY_REAL, 1},
	{   {0, TYPE_UINT, CATEGORY_REAL, 1},
	    {0, TYPE_UINT, CATEGORY_REAL, 0}}},
    {2, (CompFuncV)mulVSII, _dxfComputeCheckDimAnyShape,
	{0, TYPE_INT, CATEGORY_REAL, 1},
	{   {0, TYPE_INT, CATEGORY_REAL, 1},
	    {0, TYPE_INT, CATEGORY_REAL, 0}}},
    {2, (CompFuncV)mulVSFF, _dxfComputeCheckDimAnyShape,
	{0, TYPE_FLOAT, CATEGORY_REAL, 1},
	{   {0, TYPE_FLOAT, CATEGORY_REAL, 1},
	    {0, TYPE_FLOAT, CATEGORY_REAL, 0}}},
    {2, (CompFuncV)mulVSDD, _dxfComputeCheckDimAnyShape,
	{0, TYPE_DOUBLE, CATEGORY_REAL, 1},
	{   {0, TYPE_DOUBLE, CATEGORY_REAL, 1},
	    {0, TYPE_DOUBLE, CATEGORY_REAL, 0}}},
    {2, (CompFuncV)mulVSFFC, _dxfComputeCheckDimAnyShape,
	{0, TYPE_FLOAT, CATEGORY_COMPLEX, 1},
	{   {0, TYPE_FLOAT, CATEGORY_COMPLEX, 1},
	    {0, TYPE_FLOAT, CATEGORY_COMPLEX, 0}}},

    {2, (CompFuncV)mulSVUBUB, _dxfComputeCheckDimAnyShape,
	{0, TYPE_UBYTE, CATEGORY_REAL, 1},
	{   {0, TYPE_UBYTE, CATEGORY_REAL, 0},
	    {0, TYPE_UBYTE, CATEGORY_REAL, 1}}},
    {2, (CompFuncV)mulSVBB, _dxfComputeCheckDimAnyShape,
	{0, TYPE_BYTE, CATEGORY_REAL, 1},
	{   {0, TYPE_BYTE, CATEGORY_REAL, 0},
	    {0, TYPE_BYTE, CATEGORY_REAL, 1}}},
    {2, (CompFuncV)mulSVUSUS, _dxfComputeCheckDimAnyShape,
	{0, TYPE_USHORT, CATEGORY_REAL, 1},
	{   {0, TYPE_USHORT, CATEGORY_REAL, 0},
	    {0, TYPE_USHORT, CATEGORY_REAL, 1}}},
    {2, (CompFuncV)mulSVSS, _dxfComputeCheckDimAnyShape,
	{0, TYPE_SHORT, CATEGORY_REAL, 1},
	{   {0, TYPE_SHORT, CATEGORY_REAL, 0},
	    {0, TYPE_SHORT, CATEGORY_REAL, 1}}},
    {2, (CompFuncV)mulSVUIUI, _dxfComputeCheckDimAnyShape,
	{0, TYPE_UINT, CATEGORY_REAL, 1},
	{   {0, TYPE_UINT, CATEGORY_REAL, 0},
	    {0, TYPE_UINT, CATEGORY_REAL, 1}}},
    {2, (CompFuncV)mulSVII, _dxfComputeCheckDimAnyShape,
	{0, TYPE_INT, CATEGORY_REAL, 1},
	{   {0, TYPE_INT, CATEGORY_REAL, 0},
	    {0, TYPE_INT, CATEGORY_REAL, 1}}},
    {2, (CompFuncV)mulSVFF, _dxfComputeCheckDimAnyShape,
	{0, TYPE_FLOAT, CATEGORY_REAL, 1},
	{   {0, TYPE_FLOAT, CATEGORY_REAL, 0},
	    {0, TYPE_FLOAT, CATEGORY_REAL, 1}}},
    {2, (CompFuncV)mulSVDD, _dxfComputeCheckDimAnyShape,
	{0, TYPE_DOUBLE, CATEGORY_REAL, 1},
	{   {0, TYPE_DOUBLE, CATEGORY_REAL, 0},
	    {0, TYPE_DOUBLE, CATEGORY_REAL, 1}}},
    {2, (CompFuncV)mulSVFFC, _dxfComputeCheckDimAnyShape,
	{0, TYPE_FLOAT, CATEGORY_COMPLEX, 1},
	{   {0, TYPE_FLOAT, CATEGORY_COMPLEX, 0},
	    {0, TYPE_FLOAT, CATEGORY_COMPLEX, 1}}},

    {2, (CompFuncV)mulVUBUB, _dxfComputeCheckSameShape,
	{0, TYPE_UBYTE, CATEGORY_REAL, 1},
	{   {0, TYPE_UBYTE, CATEGORY_REAL, 0},
	    {0, TYPE_UBYTE, CATEGORY_REAL, 0}}},
    {2, (CompFuncV)mulVBB, _dxfComputeCheckSameShape,
	{0, TYPE_BYTE, CATEGORY_REAL, 1},
	{   {0, TYPE_BYTE, CATEGORY_REAL, 0},
	    {0, TYPE_BYTE, CATEGORY_REAL, 0}}},
    {2, (CompFuncV)mulVUSUS, _dxfComputeCheckSameShape,
	{0, TYPE_USHORT, CATEGORY_REAL, 1},
	{   {0, TYPE_USHORT, CATEGORY_REAL, 0},
	    {0, TYPE_USHORT, CATEGORY_REAL, 0}}},
    {2, (CompFuncV)mulVSS, _dxfComputeCheckSameShape,
	{0, TYPE_SHORT, CATEGORY_REAL, 1},
	{   {0, TYPE_SHORT, CATEGORY_REAL, 0},
	    {0, TYPE_SHORT, CATEGORY_REAL, 0}}},
    {2, (CompFuncV)mulVUIUI, _dxfComputeCheckSameShape,
	{0, TYPE_UINT, CATEGORY_REAL, 1},
	{   {0, TYPE_UINT, CATEGORY_REAL, 0},
	    {0, TYPE_UINT, CATEGORY_REAL, 0}}},
    {2, (CompFuncV)mulVII, _dxfComputeCheckSameShape,
	{0, TYPE_INT, CATEGORY_REAL, 1},
	{   {0, TYPE_INT, CATEGORY_REAL, 0},
	    {0, TYPE_INT, CATEGORY_REAL, 0}}},
    {2, (CompFuncV)mulVFF, _dxfComputeCheckSameShape,
	{0, TYPE_FLOAT, CATEGORY_REAL, 1},
	{   {0, TYPE_FLOAT, CATEGORY_REAL, 0},
	    {0, TYPE_FLOAT, CATEGORY_REAL, 0}}},
    {2, (CompFuncV)mulVDD, _dxfComputeCheckSameShape,
	{0, TYPE_DOUBLE, CATEGORY_REAL, 1},
	{   {0, TYPE_DOUBLE, CATEGORY_REAL, 0},
	    {0, TYPE_DOUBLE, CATEGORY_REAL, 0}}},
    {2, (CompFuncV)mulVFFC, _dxfComputeCheckSameShape,
	{0, TYPE_FLOAT, CATEGORY_COMPLEX, 1},
	{   {0, TYPE_FLOAT, CATEGORY_COMPLEX, 0},
	    {0, TYPE_FLOAT, CATEGORY_COMPLEX, 0}}},
};

#define NONZ(x,y) ((y) != 0)
#define FCNONZ(x,y) (REAL(y) != 0 || IMAG(y) != 0)
VBINOPRC (divVUBUB, unsigned char, unsigned char, unsigned char, /, NONZ,
	  "#12020")
VBINOPRC (divVBB, signed char, signed char, signed char, /, NONZ,
	  "#12020")
VBINOPRC (divVUSUS, unsigned short, unsigned short, unsigned short, /, NONZ,
	  "#12020")
VBINOPRC (divVSS, signed short, signed short, signed short, /, NONZ,
	  "#12020")
VBINOPRC (divVUIUI, unsigned int, unsigned int, unsigned int, /, NONZ,
	  "#12020")
VBINOPRC (divVII, signed int, signed int, signed int, /, NONZ,
	  "#12020")
VBINOPRC (divVFF, float, float, float, /, NONZ,
	  "#12020")
VBINOPRC (divVDD, double, double, double, /, NONZ,
	  "#12020")
VFUNC2RC (divVFFC, complexFloat, complexFloat, complexFloat, _dxfComputeDivComplexFloat,
	  FCNONZ, "#12020")

VSBINOPRC (divVSUBUB, unsigned char, unsigned char, unsigned char, /, NONZ,
	  "#12020")
VSBINOPRC (divVSBB, signed char, signed char, signed char, /, NONZ,
	  "#12020")
VSBINOPRC (divVSUSUS, unsigned short, unsigned short, unsigned short, /, NONZ,
	  "#12020")
VSBINOPRC (divVSSS, signed short, signed short, signed short, /, NONZ,
	  "#12020")
VSBINOPRC (divVSUIUI, unsigned int, unsigned int, unsigned int, /, NONZ,
	  "#12020")
VSBINOPRC (divVSII, signed int, signed int, signed int, /, NONZ,
	  "#12020")
VSBINOPRC (divVSFF, float, float, float, /, NONZ,
	  "#12020")
VSBINOPRC (divVSDD, double, double, double, /, NONZ,
	  "#12020")
VSFUNC2RC (divVSFFC, complexFloat, complexFloat, complexFloat, _dxfComputeDivComplexFloat,
	  FCNONZ, "#12020")

SVBINOPRC (divSVUBUB, unsigned char, unsigned char, unsigned char, /, NONZ,
	  "#12020")
SVBINOPRC (divSVBB, signed char, signed char, signed char, /, NONZ,
	  "#12020")
SVBINOPRC (divSVUSUS, unsigned short, unsigned short, unsigned short, /, NONZ,
	  "#12020")
SVBINOPRC (divSVSS, signed short, signed short, signed short, /, NONZ,
	  "#12020")
SVBINOPRC (divSVUIUI, unsigned int, unsigned int, unsigned int, /, NONZ,
	  "#12020")
SVBINOPRC (divSVII, signed int, signed int, signed int, /, NONZ,
	  "#12020")
SVBINOPRC (divSVFF, float, float, float, /, NONZ,
	  "#12020")
SVBINOPRC (divSVDD, double, double, double, /, NONZ,
	  "#12020")
SVFUNC2RC (divSVFFC, complexFloat, complexFloat, complexFloat, _dxfComputeDivComplexFloat,
	  FCNONZ, "#12020")

VBINOPRC (modVUBUB, unsigned char, unsigned char, unsigned char, %, NONZ,
	  "#12020")
VBINOPRC (modVBB, signed char, signed char, signed char, %, NONZ,
	  "#12020")
VBINOPRC (modVUSUS, unsigned short, unsigned short, unsigned short, %, NONZ,
	  "#12020")
VBINOPRC (modVSS, signed short, signed short, signed short, %, NONZ,
	  "#12020")
VBINOPRC (modVUIUI, unsigned int, unsigned int, unsigned int, %, NONZ,
	  "#12020")
VBINOPRC (modVII, signed int, signed int, signed int, %, NONZ,
	  "#12020")
VFUNC2RC (modVFF, float, float, float, fmod, NONZ,
	  "#12020")
VFUNC2RC (modVDD, double, double, double, fmod, NONZ,
	  "#12020")

VSBINOPRC (modVSUBUB, unsigned char, unsigned char, unsigned char, %, NONZ,
	  "#12020")
VSBINOPRC (modVSBB, signed char, signed char, signed char, %, NONZ,
	  "#12020")
VSBINOPRC (modVSUSUS, unsigned short, unsigned short, unsigned short, %, NONZ,
	  "#12020")
VSBINOPRC (modVSSS, signed short, signed short, signed short, %, NONZ,
	  "#12020")
VSBINOPRC (modVSUIUI, unsigned int, unsigned int, unsigned int, %, NONZ,
	  "#12020")
VSBINOPRC (modVSII, signed int, signed int, signed int, %, NONZ,
	  "#12020")
VSFUNC2RC (modVSFF, float, float, float, fmod, NONZ,
	  "#12020")
VSFUNC2RC (modVSDD, double, double, double, fmod, NONZ,
	  "#12020")

SVBINOPRC (modSVUBUB, unsigned char, unsigned char, unsigned char, %, NONZ,
	  "#12020")
SVBINOPRC (modSVBB, signed char, signed char, signed char, %, NONZ,
	  "#12020")
SVBINOPRC (modSVUSUS, unsigned short, unsigned short, unsigned short, %, NONZ,
	  "#12020")
SVBINOPRC (modSVSS, signed short, signed short, signed short, %, NONZ,
	  "#12020")
SVBINOPRC (modSVUIUI, unsigned int, unsigned int, unsigned int, %, NONZ,
	  "#12020")
SVBINOPRC (modSVII, signed int, signed int, signed int, %, NONZ,
	  "#12020")
SVFUNC2RC (modSVFF, float, float, float, fmod, NONZ,
	  "#12020")
SVFUNC2RC (modSVDD, double, double, double, fmod, NONZ,
	  "#12020")

static OperBinding divs[] = {
    {2, (CompFuncV)divVSUBUB, _dxfComputeCheckDimAnyShape,
	{0, TYPE_UBYTE, CATEGORY_REAL, 1},
	{   {0, TYPE_UBYTE, CATEGORY_REAL, 1},
	    {0, TYPE_UBYTE, CATEGORY_REAL, 0}}},
    {2, (CompFuncV)divVSBB, _dxfComputeCheckDimAnyShape,
	{0, TYPE_BYTE, CATEGORY_REAL, 1},
	{   {0, TYPE_BYTE, CATEGORY_REAL, 1},
	    {0, TYPE_BYTE, CATEGORY_REAL, 0}}},
    {2, (CompFuncV)divVSUSUS, _dxfComputeCheckDimAnyShape,
	{0, TYPE_USHORT, CATEGORY_REAL, 1},
	{   {0, TYPE_USHORT, CATEGORY_REAL, 1},
	    {0, TYPE_USHORT, CATEGORY_REAL, 0}}},
    {2, (CompFuncV)divVSSS, _dxfComputeCheckDimAnyShape,
	{0, TYPE_SHORT, CATEGORY_REAL, 1},
	{   {0, TYPE_SHORT, CATEGORY_REAL, 1},
	    {0, TYPE_SHORT, CATEGORY_REAL, 0}}},
    {2, (CompFuncV)divVSUIUI, _dxfComputeCheckDimAnyShape,
	{0, TYPE_UINT, CATEGORY_REAL, 1},
	{   {0, TYPE_UINT, CATEGORY_REAL, 1},
	    {0, TYPE_UINT, CATEGORY_REAL, 0}}},
    {2, (CompFuncV)divVSII, _dxfComputeCheckDimAnyShape,
	{0, TYPE_INT, CATEGORY_REAL, 1},
	{   {0, TYPE_INT, CATEGORY_REAL, 1},
	    {0, TYPE_INT, CATEGORY_REAL, 0}}},
    {2, (CompFuncV)divVSFF, _dxfComputeCheckDimAnyShape,
	{0, TYPE_FLOAT, CATEGORY_REAL, 1},
	{   {0, TYPE_FLOAT, CATEGORY_REAL, 1},
	    {0, TYPE_FLOAT, CATEGORY_REAL, 0}}},
    {2, (CompFuncV)divVSDD, _dxfComputeCheckDimAnyShape,
	{0, TYPE_DOUBLE, CATEGORY_REAL, 1},
	{   {0, TYPE_DOUBLE, CATEGORY_REAL, 1},
	    {0, TYPE_DOUBLE, CATEGORY_REAL, 0}}},
    {2, (CompFuncV)divVSFFC, _dxfComputeCheckDimAnyShape,
	{0, TYPE_FLOAT, CATEGORY_COMPLEX, 1},
	{   {0, TYPE_FLOAT, CATEGORY_COMPLEX, 1},
	    {0, TYPE_FLOAT, CATEGORY_COMPLEX, 0}}},

    {2, (CompFuncV)divSVUBUB, _dxfComputeCheckDimAnyShape,
	{0, TYPE_UBYTE, CATEGORY_REAL, 1},
	{   {0, TYPE_UBYTE, CATEGORY_REAL, 0},
	    {0, TYPE_UBYTE, CATEGORY_REAL, 1}}},
    {2, (CompFuncV)divSVBB, _dxfComputeCheckDimAnyShape,
	{0, TYPE_BYTE, CATEGORY_REAL, 1},
	{   {0, TYPE_BYTE, CATEGORY_REAL, 0},
	    {0, TYPE_BYTE, CATEGORY_REAL, 1}}},
    {2, (CompFuncV)divSVUSUS, _dxfComputeCheckDimAnyShape,
	{0, TYPE_USHORT, CATEGORY_REAL, 1},
	{   {0, TYPE_USHORT, CATEGORY_REAL, 0},
	    {0, TYPE_USHORT, CATEGORY_REAL, 1}}},
    {2, (CompFuncV)divSVSS, _dxfComputeCheckDimAnyShape,
	{0, TYPE_SHORT, CATEGORY_REAL, 1},
	{   {0, TYPE_SHORT, CATEGORY_REAL, 0},
	    {0, TYPE_SHORT, CATEGORY_REAL, 1}}},
    {2, (CompFuncV)divSVUIUI, _dxfComputeCheckDimAnyShape,
	{0, TYPE_UINT, CATEGORY_REAL, 1},
	{   {0, TYPE_UINT, CATEGORY_REAL, 0},
	    {0, TYPE_UINT, CATEGORY_REAL, 1}}},
    {2, (CompFuncV)divSVII, _dxfComputeCheckDimAnyShape,
	{0, TYPE_INT, CATEGORY_REAL, 1},
	{   {0, TYPE_INT, CATEGORY_REAL, 0},
	    {0, TYPE_INT, CATEGORY_REAL, 1}}},
    {2, (CompFuncV)divSVFF, _dxfComputeCheckDimAnyShape,
	{0, TYPE_FLOAT, CATEGORY_REAL, 1},
	{   {0, TYPE_FLOAT, CATEGORY_REAL, 0},
	    {0, TYPE_FLOAT, CATEGORY_REAL, 1}}},
    {2, (CompFuncV)divSVDD, _dxfComputeCheckDimAnyShape,
	{0, TYPE_DOUBLE, CATEGORY_REAL, 1},
	{   {0, TYPE_DOUBLE, CATEGORY_REAL, 0},
	    {0, TYPE_DOUBLE, CATEGORY_REAL, 1}}},
    {2, (CompFuncV)divSVFFC, _dxfComputeCheckDimAnyShape,
	{0, TYPE_FLOAT, CATEGORY_COMPLEX, 1},
	{   {0, TYPE_FLOAT, CATEGORY_COMPLEX, 0},
	    {0, TYPE_FLOAT, CATEGORY_COMPLEX, 1}}},

    {2, (CompFuncV)divVUBUB, _dxfComputeCheckSameShape,
	{0, TYPE_UBYTE, CATEGORY_REAL, 1},
	{   {0, TYPE_UBYTE, CATEGORY_REAL, 0},
	    {0, TYPE_UBYTE, CATEGORY_REAL, 0}}},
    {2, (CompFuncV)divVBB, _dxfComputeCheckSameShape,
	{0, TYPE_BYTE, CATEGORY_REAL, 1},
	{   {0, TYPE_BYTE, CATEGORY_REAL, 0},
	    {0, TYPE_BYTE, CATEGORY_REAL, 0}}},
    {2, (CompFuncV)divVUSUS, _dxfComputeCheckSameShape,
	{0, TYPE_USHORT, CATEGORY_REAL, 1},
	{   {0, TYPE_USHORT, CATEGORY_REAL, 0},
	    {0, TYPE_USHORT, CATEGORY_REAL, 0}}},
    {2, (CompFuncV)divVSS, _dxfComputeCheckSameShape,
	{0, TYPE_SHORT, CATEGORY_REAL, 1},
	{   {0, TYPE_SHORT, CATEGORY_REAL, 0},
	    {0, TYPE_SHORT, CATEGORY_REAL, 0}}},
    {2, (CompFuncV)divVUIUI, _dxfComputeCheckSameShape,
	{0, TYPE_UINT, CATEGORY_REAL, 1},
	{   {0, TYPE_UINT, CATEGORY_REAL, 0},
	    {0, TYPE_UINT, CATEGORY_REAL, 0}}},
    {2, (CompFuncV)divVII, _dxfComputeCheckSameShape,
	{0, TYPE_INT, CATEGORY_REAL, 1},
	{   {0, TYPE_INT, CATEGORY_REAL, 0},
	    {0, TYPE_INT, CATEGORY_REAL, 0}}},
    {2, (CompFuncV)divVFF, _dxfComputeCheckSameShape,
	{0, TYPE_FLOAT, CATEGORY_REAL, 1},
	{   {0, TYPE_FLOAT, CATEGORY_REAL, 0},
	    {0, TYPE_FLOAT, CATEGORY_REAL, 0}}},
    {2, (CompFuncV)divVDD, _dxfComputeCheckSameShape,
	{0, TYPE_DOUBLE, CATEGORY_REAL, 1},
	{   {0, TYPE_DOUBLE, CATEGORY_REAL, 0},
	    {0, TYPE_DOUBLE, CATEGORY_REAL, 0}}},
    {2, (CompFuncV)divVFFC, _dxfComputeCheckSameShape,
	{0, TYPE_FLOAT, CATEGORY_COMPLEX, 1},
	{   {0, TYPE_FLOAT, CATEGORY_COMPLEX, 0},
	    {0, TYPE_FLOAT, CATEGORY_COMPLEX, 0}}},
};

static OperBinding mods[] = {
    {2, (CompFuncV)modVSUBUB, _dxfComputeCheckDimAnyShape,
	{0, TYPE_UBYTE, CATEGORY_REAL, 1},
	{   {0, TYPE_UBYTE, CATEGORY_REAL, 1},
	    {0, TYPE_UBYTE, CATEGORY_REAL, 0}}},
    {2, (CompFuncV)modVSBB, _dxfComputeCheckDimAnyShape,
	{0, TYPE_BYTE, CATEGORY_REAL, 1},
	{   {0, TYPE_BYTE, CATEGORY_REAL, 1},
	    {0, TYPE_BYTE, CATEGORY_REAL, 0}}},
    {2, (CompFuncV)modVSUSUS, _dxfComputeCheckDimAnyShape,
	{0, TYPE_USHORT, CATEGORY_REAL, 1},
	{   {0, TYPE_USHORT, CATEGORY_REAL, 1},
	    {0, TYPE_USHORT, CATEGORY_REAL, 0}}},
    {2, (CompFuncV)modVSSS, _dxfComputeCheckDimAnyShape,
	{0, TYPE_SHORT, CATEGORY_REAL, 1},
	{   {0, TYPE_SHORT, CATEGORY_REAL, 1},
	    {0, TYPE_SHORT, CATEGORY_REAL, 0}}},
    {2, (CompFuncV)modVSUIUI, _dxfComputeCheckDimAnyShape,
	{0, TYPE_UINT, CATEGORY_REAL, 1},
	{   {0, TYPE_UINT, CATEGORY_REAL, 1},
	    {0, TYPE_UINT, CATEGORY_REAL, 0}}},
    {2, (CompFuncV)modVSII, _dxfComputeCheckDimAnyShape,
	{0, TYPE_INT, CATEGORY_REAL, 1},
	{   {0, TYPE_INT, CATEGORY_REAL, 1},
	    {0, TYPE_INT, CATEGORY_REAL, 0}}},
    {2, (CompFuncV)modVSFF, _dxfComputeCheckDimAnyShape,
	{0, TYPE_FLOAT, CATEGORY_REAL, 1},
	{   {0, TYPE_FLOAT, CATEGORY_REAL, 1},
	    {0, TYPE_FLOAT, CATEGORY_REAL, 0}}},
    {2, (CompFuncV)modVSDD, _dxfComputeCheckDimAnyShape,
	{0, TYPE_DOUBLE, CATEGORY_REAL, 1},
	{   {0, TYPE_DOUBLE, CATEGORY_REAL, 1},
	    {0, TYPE_DOUBLE, CATEGORY_REAL, 0}}},

    {2, (CompFuncV)modSVUBUB, _dxfComputeCheckDimAnyShape,
	{0, TYPE_UBYTE, CATEGORY_REAL, 1},
	{   {0, TYPE_UBYTE, CATEGORY_REAL, 0},
	    {0, TYPE_UBYTE, CATEGORY_REAL, 1}}},
    {2, (CompFuncV)modSVBB, _dxfComputeCheckDimAnyShape,
	{0, TYPE_BYTE, CATEGORY_REAL, 1},
	{   {0, TYPE_BYTE, CATEGORY_REAL, 0},
	    {0, TYPE_BYTE, CATEGORY_REAL, 1}}},
    {2, (CompFuncV)modSVUSUS, _dxfComputeCheckDimAnyShape,
	{0, TYPE_USHORT, CATEGORY_REAL, 1},
	{   {0, TYPE_USHORT, CATEGORY_REAL, 0},
	    {0, TYPE_USHORT, CATEGORY_REAL, 1}}},
    {2, (CompFuncV)modSVSS, _dxfComputeCheckDimAnyShape,
	{0, TYPE_SHORT, CATEGORY_REAL, 1},
	{   {0, TYPE_SHORT, CATEGORY_REAL, 0},
	    {0, TYPE_SHORT, CATEGORY_REAL, 1}}},
    {2, (CompFuncV)modSVUIUI, _dxfComputeCheckDimAnyShape,
	{0, TYPE_UINT, CATEGORY_REAL, 1},
	{   {0, TYPE_UINT, CATEGORY_REAL, 0},
	    {0, TYPE_UINT, CATEGORY_REAL, 1}}},
    {2, (CompFuncV)modSVII, _dxfComputeCheckDimAnyShape,
	{0, TYPE_INT, CATEGORY_REAL, 1},
	{   {0, TYPE_INT, CATEGORY_REAL, 0},
	    {0, TYPE_INT, CATEGORY_REAL, 1}}},
    {2, (CompFuncV)modSVFF, _dxfComputeCheckDimAnyShape,
	{0, TYPE_FLOAT, CATEGORY_REAL, 1},
	{   {0, TYPE_FLOAT, CATEGORY_REAL, 0},
	    {0, TYPE_FLOAT, CATEGORY_REAL, 1}}},
    {2, (CompFuncV)modSVDD, _dxfComputeCheckDimAnyShape,
	{0, TYPE_DOUBLE, CATEGORY_REAL, 1},
	{   {0, TYPE_DOUBLE, CATEGORY_REAL, 0},
	    {0, TYPE_DOUBLE, CATEGORY_REAL, 1}}},

    {2, (CompFuncV)modVUBUB, _dxfComputeCheckSameShape,
	{0, TYPE_UBYTE, CATEGORY_REAL, 1},
	{   {0, TYPE_UBYTE, CATEGORY_REAL, 0},
	    {0, TYPE_UBYTE, CATEGORY_REAL, 0}}},
    {2, (CompFuncV)modVBB, _dxfComputeCheckSameShape,
	{0, TYPE_BYTE, CATEGORY_REAL, 1},
	{   {0, TYPE_BYTE, CATEGORY_REAL, 0},
	    {0, TYPE_BYTE, CATEGORY_REAL, 0}}},
    {2, (CompFuncV)modVUSUS, _dxfComputeCheckSameShape,
	{0, TYPE_USHORT, CATEGORY_REAL, 1},
	{   {0, TYPE_USHORT, CATEGORY_REAL, 0},
	    {0, TYPE_USHORT, CATEGORY_REAL, 0}}},
    {2, (CompFuncV)modVSS, _dxfComputeCheckSameShape,
	{0, TYPE_SHORT, CATEGORY_REAL, 1},
	{   {0, TYPE_SHORT, CATEGORY_REAL, 0},
	    {0, TYPE_SHORT, CATEGORY_REAL, 0}}},
    {2, (CompFuncV)modVUIUI, _dxfComputeCheckSameShape,
	{0, TYPE_UINT, CATEGORY_REAL, 1},
	{   {0, TYPE_UINT, CATEGORY_REAL, 0},
	    {0, TYPE_UINT, CATEGORY_REAL, 0}}},
    {2, (CompFuncV)modVII, _dxfComputeCheckSameShape,
	{0, TYPE_INT, CATEGORY_REAL, 1},
	{   {0, TYPE_INT, CATEGORY_REAL, 0},
	    {0, TYPE_INT, CATEGORY_REAL, 0}}},
    {2, (CompFuncV)modVFF, _dxfComputeCheckSameShape,
	{0, TYPE_FLOAT, CATEGORY_REAL, 1},
	{   {0, TYPE_FLOAT, CATEGORY_REAL, 0},
	    {0, TYPE_FLOAT, CATEGORY_REAL, 0}}},
    {2, (CompFuncV)modVDD, _dxfComputeCheckSameShape,
	{0, TYPE_DOUBLE, CATEGORY_REAL, 1},
	{   {0, TYPE_DOUBLE, CATEGORY_REAL, 0},
	    {0, TYPE_DOUBLE, CATEGORY_REAL, 0}}},
};

/* Now follows the set of builtin functions for type conversion and 
 * truncation 
 */
#ifndef sgi
#   define ffloor floor
#   define fceil  ceil
#   define ftrunc trunc
#   define flog   log
#   define flog10 log10
#   define fexp   exp
#   define fsin   sin
#   define fcos   cos
#   define ftan   tan
#   define fsinh  sinh
#   define fcosh  cosh
#   define ftanh  tanh
#   define fasin  asin
#   define facos  acos
#   define fatan  atan
#   define fatan2 atan2
#   define fsqrt  sqrt
#endif
#ifdef aviion
#   define trunc(x) ((float)((int)(x)))
#endif
#ifdef sun4
#   define trunc(x) ((float)((int)(x)))
#endif
#ifdef solaris
#   define trunc(x) ((float)((int)(x)))
#endif
#ifdef ibmpvs
#   define rint(x) ((float)((int)((x) + 0.5)))
#   define trunc(x) ((float)((int)(x)))
#endif
#ifdef hp700
#   define rint(x) ((float)((int)((x) + 0.5)))
#   define trunc(x) ((float)((int)(x)))
#endif
#ifdef macos
#   define trunc(x) ((float)((int)(x)))
#endif
#define SIGN(x) ((x) >= 0? (1): (-1))

/* These are the math operators */
#define POWCK(x,y) ((x) >= 0.0 || ceil(y) == (y))
VSFUNC2RC (powVSFF, float, float, float, pow, POWCK, "#12030")
SVFUNC2RC (powSVFF, float, float, float, pow, POWCK, "#12030")
VFUNC2RC (powVFF, float, float, float, pow, POWCK, "#12030")
VSFUNC2RC (powVSDD, double, double, double, pow, POWCK, "#12030")
SVFUNC2RC (powSVDD, double, double, double, pow, POWCK, "#12030")
VFUNC2RC (powVDD, double, double, double, pow, POWCK, "#12030")
VSFUNC2 (powVSFFCC, complexFloat, complexFloat, complexFloat, _dxfComputePowComplexFloat)
SVFUNC2 (powSVFFCC, complexFloat, complexFloat, complexFloat, _dxfComputePowComplexFloat)
VFUNC2 (powVFFCC, complexFloat, complexFloat, complexFloat, _dxfComputePowComplexFloat)
VSFUNC2 (powVSFFCR, complexFloat, complexFloat, float, _dxfComputePowComplexFloatFloat)
SVFUNC2 (powSVFFCR, complexFloat, complexFloat, float, _dxfComputePowComplexFloatFloat)
VFUNC2 (powVFFCR, complexFloat, complexFloat, float, _dxfComputePowComplexFloatFloat)

#define NONNEG(x) ((x) >= 0.0)
VFUNC1RC (sqrtF, float, float, fsqrt, NONNEG, "#12040")
VFUNC1RC (sqrtD, double, double, sqrt, NONNEG, "#12040")
VFUNC1(sqrtFC, complexFloat, complexFloat, _dxfComputeSqrtComplexFloat)
#define POS(x) ((x) > 0.0)
VFUNC1RC (lnF, float, float, flog, POS, "#12050")
VFUNC1RC (lnD, double, double, log, POS, "#12050")
VFUNC1(lnFC, complexFloat, complexFloat, _dxfComputeLnComplexFloat)
VFUNC1RC (log10F, float, float, flog10, POS, "#12050")
VFUNC1RC (log10D, double, double, log10, POS, "#12050")
VFUNC1 (expF, float, float, fexp)
VFUNC1 (expD, double, double, exp)
VFUNC1 (expFC, complexFloat, complexFloat, _dxfComputeExpComplexFloat)
#define ABS(x) ((x) < 0? -(x): (x))
VFUNC1 (absF, float, float, ABS )
VFUNC1 (absD, double, double, ABS )
VFUNC1 (absFC, float, complexFloat, _dxfComputeAbsComplexFloat)
VFUNC1 (absI, int, int, ABS )
VFUNC1 (absS, short, short, ABS )
VFUNC1 (absB, signed char, signed char, ABS )
VFUNC1 (argFC, float, complexFloat, _dxfComputeArgComplexFloat)
static OperBinding sqrts[] = {
    {1, (CompFuncV)sqrtFC, _dxfComputeCheckSameShape,
	{0, TYPE_FLOAT, CATEGORY_COMPLEX, 0},
	{   {0, TYPE_FLOAT, CATEGORY_COMPLEX, 0}}},
    {1, (CompFuncV)sqrtF, _dxfComputeCheckSameShape,
	{0, TYPE_FLOAT, CATEGORY_REAL, 0},
	{   {0, TYPE_FLOAT, CATEGORY_REAL, 0}}},
    {1, (CompFuncV)sqrtD, _dxfComputeCheckSameShape,
	{0, TYPE_DOUBLE, CATEGORY_REAL, 0},
	{   {0, TYPE_DOUBLE, CATEGORY_REAL, 0}}}
};
static OperBinding pows[] = {
    {2, (CompFuncV)powVSFF, _dxfComputeCheckDimAnyShape,
	{0, TYPE_FLOAT, CATEGORY_REAL, 0},
	{   {0, TYPE_FLOAT, CATEGORY_REAL, 1},
	    {0, TYPE_FLOAT, CATEGORY_REAL, 0}}},
    {2, (CompFuncV)powSVFF, _dxfComputeCheckDimAnyShape,
	{0, TYPE_FLOAT, CATEGORY_REAL, 0},
	{   {0, TYPE_FLOAT, CATEGORY_REAL, 0},
	    {0, TYPE_FLOAT, CATEGORY_REAL, 1}}},
    {2, (CompFuncV)powVFF, _dxfComputeCheckSameShape,
	{0, TYPE_FLOAT, CATEGORY_REAL, 0},
	{   {0, TYPE_FLOAT, CATEGORY_REAL, 0},
	    {0, TYPE_FLOAT, CATEGORY_REAL, 0}}},
    {2, (CompFuncV)powVSDD, _dxfComputeCheckDimAnyShape,
	{0, TYPE_DOUBLE, CATEGORY_REAL, 0},
	{   {0, TYPE_DOUBLE, CATEGORY_REAL, 1},
	    {0, TYPE_DOUBLE, CATEGORY_REAL, 0}}},
    {2, (CompFuncV)powSVDD, _dxfComputeCheckDimAnyShape,
	{0, TYPE_DOUBLE, CATEGORY_REAL, 0},
	{   {0, TYPE_DOUBLE, CATEGORY_REAL, 0},
	    {0, TYPE_DOUBLE, CATEGORY_REAL, 1}}},
    {2, (CompFuncV)powVDD, _dxfComputeCheckSameShape,
	{0, TYPE_DOUBLE, CATEGORY_REAL, 0},
	{   {0, TYPE_DOUBLE, CATEGORY_REAL, 0},
	    {0, TYPE_DOUBLE, CATEGORY_REAL, 0}}},
    {2, (CompFuncV)powVSFFCC, _dxfComputeCheckDimAnyShape,
	{0, TYPE_FLOAT, CATEGORY_COMPLEX, 0},
	{   {0, TYPE_FLOAT, CATEGORY_COMPLEX, 1},
	    {0, TYPE_FLOAT, CATEGORY_COMPLEX, 0}}},
    {2, (CompFuncV)powSVFFCC, _dxfComputeCheckDimAnyShape,
	{0, TYPE_FLOAT, CATEGORY_COMPLEX, 0},
	{   {0, TYPE_FLOAT, CATEGORY_COMPLEX, 0},
	    {0, TYPE_FLOAT, CATEGORY_COMPLEX, 1}}},
    {2, (CompFuncV)powVFFCC, _dxfComputeCheckSameShape,
	{0, TYPE_FLOAT, CATEGORY_COMPLEX, 0},
	{   {0, TYPE_FLOAT, CATEGORY_COMPLEX, 0},
	    {0, TYPE_FLOAT, CATEGORY_COMPLEX, 0}}},
    {2, (CompFuncV)powVSFFCR, _dxfComputeCheckDimAnyShape,
	{0, TYPE_FLOAT, CATEGORY_COMPLEX, 0},
	{   {0, TYPE_FLOAT, CATEGORY_COMPLEX, 1},
	    {0, TYPE_FLOAT, CATEGORY_REAL, 0}}},
    {2, (CompFuncV)powSVFFCR, _dxfComputeCheckDimAnyShape,
	{0, TYPE_FLOAT, CATEGORY_COMPLEX, 0},
	{   {0, TYPE_FLOAT, CATEGORY_COMPLEX, 0},
	    {0, TYPE_FLOAT, CATEGORY_REAL, 1}}},
    {2, (CompFuncV)powVFFCR, _dxfComputeCheckSameShape,
	{0, TYPE_FLOAT, CATEGORY_COMPLEX, 0},
	{   {0, TYPE_FLOAT, CATEGORY_COMPLEX, 0},
	    {0, TYPE_FLOAT, CATEGORY_REAL, 0}}}
};
static OperBinding lns[] = {
    {1, (CompFuncV)lnF, _dxfComputeCheckSameShape,
	{0, TYPE_FLOAT, CATEGORY_REAL, 0},
	{   {0, TYPE_FLOAT, CATEGORY_REAL, 0}}},
    {1, (CompFuncV)lnD, _dxfComputeCheckSameShape,
	{0, TYPE_DOUBLE, CATEGORY_REAL, 0},
	{   {0, TYPE_DOUBLE, CATEGORY_REAL, 0}}},
    {1, (CompFuncV)lnFC, _dxfComputeCheckSameShape,
	{0, TYPE_FLOAT, CATEGORY_COMPLEX, 0},
	{   {0, TYPE_FLOAT, CATEGORY_COMPLEX, 0}}}
};
static OperBinding log10s[] = {
    {1, (CompFuncV)log10F, _dxfComputeCheckSameShape,
	{0, TYPE_FLOAT, CATEGORY_REAL, 0},
	{   {0, TYPE_FLOAT, CATEGORY_REAL, 0}}},
    {1, (CompFuncV)log10D, _dxfComputeCheckSameShape,
	{0, TYPE_DOUBLE, CATEGORY_REAL, 0},
	{   {0, TYPE_DOUBLE, CATEGORY_REAL, 0}}}
};
static OperBinding exps[] = {
    {1, (CompFuncV)expF, _dxfComputeCheckSameShape,
	{0, TYPE_FLOAT, CATEGORY_REAL, 0},
	{   {0, TYPE_FLOAT, CATEGORY_REAL, 0}}},
    {1, (CompFuncV)expD, _dxfComputeCheckSameShape,
	{0, TYPE_DOUBLE, CATEGORY_REAL, 0},
	{   {0, TYPE_DOUBLE, CATEGORY_REAL, 0}}},
    {1, (CompFuncV)expFC, _dxfComputeCheckSameShape,
	{0, TYPE_FLOAT, CATEGORY_COMPLEX, 0},
	{   {0, TYPE_FLOAT, CATEGORY_COMPLEX, 0}}}
};
static OperBinding abss[] = {
    {1, (CompFuncV)_dxfComputeCopy, _dxfComputeCheckSameShape,
	{0, TYPE_UBYTE, CATEGORY_REAL, 0},
	{   {0, TYPE_UBYTE, CATEGORY_REAL, 0}}},
    {1, (CompFuncV)_dxfComputeCopy, _dxfComputeCheckSameShape,
	{0, TYPE_USHORT, CATEGORY_REAL, 0},
	{   {0, TYPE_USHORT, CATEGORY_REAL, 0}}},
    {1, (CompFuncV)_dxfComputeCopy, _dxfComputeCheckSameShape,
	{0, TYPE_UINT, CATEGORY_REAL, 0},
	{   {0, TYPE_UINT, CATEGORY_REAL, 0}}},
    {1, (CompFuncV)absI, _dxfComputeCheckSameShape,
	{0, TYPE_INT, CATEGORY_REAL, 0},
	{   {0, TYPE_INT, CATEGORY_REAL, 0}}},
    {1, (CompFuncV)absS, _dxfComputeCheckSameShape,
	{0, TYPE_SHORT, CATEGORY_REAL, 0},
	{   {0, TYPE_SHORT, CATEGORY_REAL, 0}}},
    {1, (CompFuncV)absB, _dxfComputeCheckSameShape,
	{0, TYPE_BYTE, CATEGORY_REAL, 0},
	{   {0, TYPE_BYTE, CATEGORY_REAL, 0}}},
    {1, (CompFuncV)absF, _dxfComputeCheckSameShape,
	{0, TYPE_FLOAT, CATEGORY_REAL, 0},
	{   {0, TYPE_FLOAT, CATEGORY_REAL, 0}}},
    {1, (CompFuncV)absD, _dxfComputeCheckSameShape,
	{0, TYPE_DOUBLE, CATEGORY_REAL, 0},
	{   {0, TYPE_DOUBLE, CATEGORY_REAL, 0}}},
    {1, (CompFuncV)absFC, _dxfComputeCheckSameShape,
	{0, TYPE_FLOAT, CATEGORY_REAL, 0},
	{   {0, TYPE_FLOAT, CATEGORY_COMPLEX, 0}}}
};
static OperBinding args[] = {
    {1, (CompFuncV)argFC, _dxfComputeCheckSameShape,
	{0, TYPE_FLOAT, CATEGORY_REAL, 0},
	{   {0, TYPE_FLOAT, CATEGORY_COMPLEX, 0}}}
};

VFUNC1 (sinF, float, float, fsin)
VFUNC1 (cosF, float, float, fcos)
VFUNC1 (tanF, float, float, ftan)
VFUNC1 (sinhF, float, float, fsinh)
VFUNC1 (coshF, float, float, fcosh)
VFUNC1 (tanhF, float, float, ftanh)
#if defined(HAVE_FINITE)
VFUNC1 (finiteF, int, float, finite)
#endif
#if defined(HAVE_ISNAN)
VFUNC1 (isnanF, int, float, isnan)
#endif

VFUNC1 (sinD, double, double, sin)
VFUNC1 (cosD, double, double, cos)
VFUNC1 (tanD, double, double, tan)
VFUNC1 (sinhD, double, double, sinh)
VFUNC1 (coshD, double, double, cosh)
VFUNC1 (tanhD, double, double, tanh)
#if defined(HAVE_FINITE)
VFUNC1 (finiteD, int, double, finite)
#endif
#if defined(HAVE_ISNAN)
VFUNC1 (isnanD, int, double, isnan)
#endif

VFUNC1 (sinFC, complexFloat, complexFloat, _dxfComputeSinComplexFloat)
VFUNC1 (cosFC, complexFloat, complexFloat, _dxfComputeCosComplexFloat)
VFUNC1 (tanFC, complexFloat, complexFloat, _dxfComputeTanComplexFloat)
VFUNC1 (sinhFC, complexFloat, complexFloat, _dxfComputeSinhComplexFloat)
VFUNC1 (coshFC, complexFloat, complexFloat, _dxfComputeCoshComplexFloat)
VFUNC1 (tanhFC, complexFloat, complexFloat, _dxfComputeTanhComplexFloat)
#if defined(HAVE_FINITE)
VFUNC1 (finiteFC, int, complexFloat, _dxfComputeFiniteComplexFloat)
#endif
#if defined(HAVE_ISNAN)
VFUNC1 (isnanFC, int, complexFloat, _dxfComputeIsnanComplexFloat)
#endif

#define FPM1(x) ((x) >= -1 && (x) <= 1)
VFUNC1RC (asinF, float, float, fasin, FPM1, "#12060")
VFUNC1RC (acosF, float, float, facos, FPM1, "#12061")
VFUNC1 (atanF, float, float, fatan)

VFUNC1RC (asinD, double, double, asin, FPM1, "#12060")
VFUNC1RC (acosD, double, double, acos, FPM1, "#12061")
VFUNC1 (atanD, double, double, atan)

VFUNC1 (asinFC, complexFloat, complexFloat, _dxfComputeAsinComplexFloat)
VFUNC1 (acosFC, complexFloat, complexFloat, _dxfComputeAcosComplexFloat)
VFUNC1 (atanFC, complexFloat, complexFloat, _dxfComputeAtanComplexFloat)

VFUNC2 (atan2F, float, float, float, fatan2)
VFUNC2 (atan2D, double, double, double, atan2)

static OperBinding sins[] = {
    {1, (CompFuncV)sinF, _dxfComputeCheckSameShape,
	{0, TYPE_FLOAT, CATEGORY_REAL, 0},
	{   {0, TYPE_FLOAT, CATEGORY_REAL, 0}}},
    {1, (CompFuncV)sinD, _dxfComputeCheckSameShape,
	{0, TYPE_DOUBLE, CATEGORY_REAL, 0},
	{   {0, TYPE_DOUBLE, CATEGORY_REAL, 0}}},
    {1, (CompFuncV)sinFC, _dxfComputeCheckSameShape,
	{0, TYPE_FLOAT, CATEGORY_COMPLEX, 0},
	{   {0, TYPE_FLOAT, CATEGORY_COMPLEX, 0}}}
};
static OperBinding coss[] = {
    {1, (CompFuncV)cosF, _dxfComputeCheckSameShape,
	{0, TYPE_FLOAT, CATEGORY_REAL, 0},
	{   {0, TYPE_FLOAT, CATEGORY_REAL, 0}}},
    {1, (CompFuncV)cosD, _dxfComputeCheckSameShape,
	{0, TYPE_DOUBLE, CATEGORY_REAL, 0},
	{   {0, TYPE_DOUBLE, CATEGORY_REAL, 0}}},
    {1, (CompFuncV)cosFC, _dxfComputeCheckSameShape,
	{0, TYPE_FLOAT, CATEGORY_COMPLEX, 0},
	{   {0, TYPE_FLOAT, CATEGORY_COMPLEX, 0}}}
};
static OperBinding tans[] = {
    {1, (CompFuncV)tanF, _dxfComputeCheckSameShape,
	{0, TYPE_FLOAT, CATEGORY_REAL, 0},
	{   {0, TYPE_FLOAT, CATEGORY_REAL, 0}}},
    {1, (CompFuncV)tanD, _dxfComputeCheckSameShape,
	{0, TYPE_DOUBLE, CATEGORY_REAL, 0},
	{   {0, TYPE_DOUBLE, CATEGORY_REAL, 0}}},
    {1, (CompFuncV)tanFC, _dxfComputeCheckSameShape,
	{0, TYPE_FLOAT, CATEGORY_COMPLEX, 0},
	{   {0, TYPE_FLOAT, CATEGORY_COMPLEX, 0}}}
};
static OperBinding sinhs[] = {
    {1, (CompFuncV)sinhF, _dxfComputeCheckSameShape,
	{0, TYPE_FLOAT, CATEGORY_REAL, 0},
	{   {0, TYPE_FLOAT, CATEGORY_REAL, 0}}},
    {1, (CompFuncV)sinhD, _dxfComputeCheckSameShape,
	{0, TYPE_DOUBLE, CATEGORY_REAL, 0},
	{   {0, TYPE_DOUBLE, CATEGORY_REAL, 0}}},
    {1, (CompFuncV)sinhFC, _dxfComputeCheckSameShape,
	{0, TYPE_FLOAT, CATEGORY_COMPLEX, 0},
	{   {0, TYPE_FLOAT, CATEGORY_COMPLEX, 0}}}
};
static OperBinding coshs[] = {
    {1, (CompFuncV)coshF, _dxfComputeCheckSameShape,
	{0, TYPE_FLOAT, CATEGORY_REAL, 0},
	{   {0, TYPE_FLOAT, CATEGORY_REAL, 0}}},
    {1, (CompFuncV)coshD, _dxfComputeCheckSameShape,
	{0, TYPE_DOUBLE, CATEGORY_REAL, 0},
	{   {0, TYPE_DOUBLE, CATEGORY_REAL, 0}}},
    {1, (CompFuncV)coshFC, _dxfComputeCheckSameShape,
	{0, TYPE_FLOAT, CATEGORY_COMPLEX, 0},
	{   {0, TYPE_FLOAT, CATEGORY_COMPLEX, 0}}}
};
static OperBinding tanhs[] = {
    {1, (CompFuncV)tanhF, _dxfComputeCheckSameShape,
	{0, TYPE_FLOAT, CATEGORY_REAL, 0},
	{   {0, TYPE_FLOAT, CATEGORY_REAL, 0}}},
    {1, (CompFuncV)tanhD, _dxfComputeCheckSameShape,
	{0, TYPE_DOUBLE, CATEGORY_REAL, 0},
	{   {0, TYPE_DOUBLE, CATEGORY_REAL, 0}}},
    {1, (CompFuncV)tanhFC, _dxfComputeCheckSameShape,
	{0, TYPE_FLOAT, CATEGORY_COMPLEX, 0},
	{   {0, TYPE_FLOAT, CATEGORY_COMPLEX, 0}}}
};
#if defined(HAVE_ISNAN)
static OperBinding isnans[] = {
    {1, (CompFuncV)isnanF, _dxfComputeCheckSameShape,
	{0, TYPE_INT, CATEGORY_REAL, 0},
	{   {0, TYPE_FLOAT, CATEGORY_REAL, 0}}},
    {1, (CompFuncV)isnanD, _dxfComputeCheckSameShape,
	{0, TYPE_INT, CATEGORY_REAL, 0},
	{   {0, TYPE_DOUBLE, CATEGORY_REAL, 0}}},
    {1, (CompFuncV)isnanFC, _dxfComputeCheckSameShape,
	{0, TYPE_INT, CATEGORY_REAL, 0},
	{   {0, TYPE_FLOAT, CATEGORY_COMPLEX, 0}}}
};
#endif
#if defined(HAVE_FINITE)
static OperBinding finites[] = {
    {1, (CompFuncV)finiteF, _dxfComputeCheckSameShape,
	{0, TYPE_INT, CATEGORY_REAL, 0},
	{   {0, TYPE_FLOAT, CATEGORY_REAL, 0}}},
    {1, (CompFuncV)finiteD, _dxfComputeCheckSameShape,
	{0, TYPE_INT, CATEGORY_REAL, 0},
	{   {0, TYPE_DOUBLE, CATEGORY_REAL, 0}}},
    {1, (CompFuncV)finiteFC, _dxfComputeCheckSameShape,
	{0, TYPE_INT, CATEGORY_REAL, 0},
	{   {0, TYPE_FLOAT, CATEGORY_COMPLEX, 0}}}
};
#endif
static OperBinding asins[] = {
    {1, (CompFuncV)asinF, _dxfComputeCheckSameShape,
	{0, TYPE_FLOAT, CATEGORY_REAL, 0},
	{   {0, TYPE_FLOAT, CATEGORY_REAL, 0}}},
    {1, (CompFuncV)asinD, _dxfComputeCheckSameShape,
	{0, TYPE_DOUBLE, CATEGORY_REAL, 0},
	{   {0, TYPE_DOUBLE, CATEGORY_REAL, 0}}},
    {1, (CompFuncV)asinFC, _dxfComputeCheckSameShape,
	{0, TYPE_FLOAT, CATEGORY_COMPLEX, 0},
	{   {0, TYPE_FLOAT, CATEGORY_COMPLEX, 0}}}
};
static OperBinding acoss[] = {
    {1, (CompFuncV)acosF, _dxfComputeCheckSameShape,
	{0, TYPE_FLOAT, CATEGORY_REAL, 0},
	{   {0, TYPE_FLOAT, CATEGORY_REAL, 0}}},
    {1, (CompFuncV)acosD, _dxfComputeCheckSameShape,
	{0, TYPE_DOUBLE, CATEGORY_REAL, 0},
	{   {0, TYPE_DOUBLE, CATEGORY_REAL, 0}}},
    {1, (CompFuncV)acosFC, _dxfComputeCheckSameShape,
	{0, TYPE_FLOAT, CATEGORY_COMPLEX, 0},
	{   {0, TYPE_FLOAT, CATEGORY_COMPLEX, 0}}}
};
static OperBinding atans[] = {
    {1, (CompFuncV)atanF, _dxfComputeCheckSameShape,
	{0, TYPE_FLOAT, CATEGORY_REAL, 0},
	{   {0, TYPE_FLOAT, CATEGORY_REAL, 0}}},
    {1, (CompFuncV)atanD, _dxfComputeCheckSameShape,
	{0, TYPE_DOUBLE, CATEGORY_REAL, 0},
	{   {0, TYPE_DOUBLE, CATEGORY_REAL, 0}}},
    {1, (CompFuncV)atanFC, _dxfComputeCheckSameShape,
	{0, TYPE_FLOAT, CATEGORY_COMPLEX, 0},
	{   {0, TYPE_FLOAT, CATEGORY_COMPLEX, 0}}},
    {2, (CompFuncV)atan2F, _dxfComputeCheckSameShape,
	{0, TYPE_FLOAT, CATEGORY_REAL, 0},
	{   {0, TYPE_FLOAT, CATEGORY_REAL, 0},
	    {0, TYPE_FLOAT, CATEGORY_REAL, 0}}},
    {2, (CompFuncV)atan2D, _dxfComputeCheckSameShape,
	{0, TYPE_DOUBLE, CATEGORY_REAL, 0},
	{   {0, TYPE_DOUBLE, CATEGORY_REAL, 0},
	    {0, TYPE_DOUBLE, CATEGORY_REAL, 0}}}
};
static OperBinding atan2s[] = {
    {2, (CompFuncV)atan2F, _dxfComputeCheckSameShape,
	{0, TYPE_FLOAT, CATEGORY_REAL, 0},
	{   {0, TYPE_FLOAT, CATEGORY_REAL, 0},
	    {0, TYPE_FLOAT, CATEGORY_REAL, 0}}},
    {2, (CompFuncV)atan2D, _dxfComputeCheckSameShape,
	{0, TYPE_DOUBLE, CATEGORY_REAL, 0},
	{   {0, TYPE_DOUBLE, CATEGORY_REAL, 0},
	    {0, TYPE_DOUBLE, CATEGORY_REAL, 0}}}
};

#define PI 3.1415926535898
static int
#if defined(sgi) && defined(_SYSTYPE_SVR4)
qsinx(
#else
qsin(
#endif
    PTreeNode *pt, 
    ObjStruct *os, 
    int numInputs,
    Pointer *inputs,
    Pointer result,
    InvalidComponentHandle *invalids,
    InvalidComponentHandle outInvalid)
{
    register float *out = (float *) result;
    register float *in0 = (float *) inputs[0];
    int i;
    int numBasic;

    for (numBasic = 1, i = 0; 
	    i < pt->metaType.rank;
	    ++i)
	numBasic *= pt->metaType.shape[i];

    numBasic *= pt->metaType.items;
#ifdef COMMENT_FMOD
    vsdiv(in0, 1, 2 * PI, out, 1, numBasic);
    vfrac(out, 1,         out, 1, numBasic);
    vsmul(out, 1, 2 * PI, out, 1, numBasic);
    for (i = 0; i < numBasic; ++i)
	if (out[i] < -PI)
	    out[i] += 2 * PI;
	else if (out[i] > PI)
	    out[i] -= 2 * PI;
#endif /* COMMENT_FMOD */
    qvsin (numBasic, in0, out);

    return (OK);
}

static int
#if defined(sgi) && defined(_SYSTYPE_SVR4)
qcosx(
#else
qcos(
#endif
    PTreeNode *pt, 
    ObjStruct *os, 
    int numInputs,
    Pointer *inputs,
    Pointer result,
    InvalidComponentHandle *invalids,
    InvalidComponentHandle outInvalid)
{
    register float *out = (float *) result;
    register float *in0 = (float *) inputs[0];
    int i;
    int numBasic;

    for (numBasic = 1, i = 0; 
	    i < pt->metaType.rank;
	    ++i)
	numBasic *= pt->metaType.shape[i];

    numBasic *= pt->metaType.items;
#ifdef COMMENT_FMOD
    vsdiv(in0, 1, 2 * PI, out, 1, numBasic);
    vfrac(out, 1,         out, 1, numBasic);
    vsmul(out, 1, 2 * PI, out, 1, numBasic);
    for (i = 0; i < numBasic; ++i)
	if (out[i] < -PI)
	    out[i] += 2 * PI;
	else if (out[i] > PI)
	    out[i] -= 2 * PI;
#endif /* COMMENT_FMOD */
    qvcos(numBasic, in0, out);

    return (OK);
}
static OperBinding qsins[] = {
    {1, 
#if defined(sgi) && defined(_SYSTYPE_SVR4)
     (CompFuncV)qsinx, 
#else
     (CompFuncV)qsin, 
#endif
        _dxfComputeCheckSameShape,
	{0, TYPE_FLOAT, CATEGORY_REAL, 0},
	{   {0, TYPE_FLOAT, CATEGORY_REAL, 0}}}
};
static OperBinding qcoss[] = {
    {1, 
#if defined(sgi) && defined(_SYSTYPE_SVR4)
     (CompFuncV)qcosx, 
#else
     (CompFuncV)qcos, 
#endif
        _dxfComputeCheckSameShape,
	{0, TYPE_FLOAT, CATEGORY_REAL, 0},
	{   {0, TYPE_FLOAT, CATEGORY_REAL, 0}}}
};

static int
cross3(
    PTreeNode *pt, 
    ObjStruct *os, 
    int numInputs,
    Pointer *inputs,
    Pointer result,
    InvalidComponentHandle *invalids,
    InvalidComponentHandle outInvalid)
{
    typedef float threevect[3];
    register threevect *out = (threevect *) result;
    register threevect *in0 = (threevect *) inputs[0];
    register threevect *in1 = (threevect *) inputs[1];
    int size0 = pt->args->metaType.items;
    int size1 = pt->args->next->metaType.items;
    int i, j, k;
    int allValid =
	(invalids[0] == NULL || DXGetInvalidCount(invalids[0]) == 0) &&
	(invalids[1] == NULL || DXGetInvalidCount(invalids[1]) == 0);

    for (i = j = k = 0;
	    i < pt->metaType.items;
	    ++i) {
	if (allValid || _dxfComputeInvalidOr(outInvalid, i, invalids[0], j, invalids[1], k)) {
	    out[i][0] = in0[j][1] * in1[k][2] - in0[j][2] * in1[k][1];
	    out[i][1] = in0[j][2] * in1[k][0] - in0[j][0] * in1[k][2];
	    out[i][2] = in0[j][0] * in1[k][1] - in0[j][1] * in1[k][0];
	}
	if (size0 != 1) ++j;
	if (size1 != 1) ++k;
    }
    return (OK);
}
static OperBinding crosses[] = {
    {2, (CompFuncV)cross3, _dxfComputeCheckMatch,
	{0, TYPE_FLOAT, CATEGORY_REAL, 1, {3}},
	{   {0, TYPE_FLOAT, CATEGORY_REAL, 1, {3}},
	    {0, TYPE_FLOAT, CATEGORY_REAL, 1, {3}}}}
};

/* dot */
static int
dotFF(
    PTreeNode *pt, 
    ObjStruct *os, 
    int numInputs,
    Pointer *inputs,
    Pointer result,
    InvalidComponentHandle *invalids,
    InvalidComponentHandle outInvalid)
{
    register float *out = (float *) result;
    register float *in0 = (float *) inputs[0];
    register float *in1 = (float *) inputs[1];
    int vectorLen = pt->metaType.shape[0];
    int size0 = pt->args->metaType.items;
    int size1 = pt->args->next->metaType.items;
    int i, j, k, l;
    int allValid =
	(invalids[0] == NULL || DXGetInvalidCount(invalids[0]) == 0) &&
	(invalids[1] == NULL || DXGetInvalidCount(invalids[1]) == 0);

    for (i = j = k = 0;
	    i < pt->metaType.items;
	    ++i) {
	if (allValid || _dxfComputeInvalidOr(outInvalid, i, invalids[0], j, invalids[1], k)) {
	    out[i] = in0[j * vectorLen + 0] *
		     in1[k * vectorLen + 0];
	    for (l = 1; l < vectorLen; ++l) {
		out[i] += in0[j * vectorLen + l] *
			  in1[k * vectorLen + l];
	    }
	}
	if (size0 != 1) ++j;
	if (size1 != 1) ++k;
    }
    return (OK);
}
static OperBinding dots[] = {
    {2, (CompFuncV)dotFF, _dxfComputeCheckVects,
	{0, TYPE_FLOAT, CATEGORY_REAL, 0 },
	{   {0, TYPE_FLOAT, CATEGORY_REAL},
	    {0, TYPE_FLOAT, CATEGORY_REAL}}},
    {2, (CompFuncV)mulVFF, _dxfComputeCheckMatch,
	{0, TYPE_FLOAT, CATEGORY_REAL, 0 },
	{   {0, TYPE_FLOAT, CATEGORY_REAL, 0},
	    {0, TYPE_FLOAT, CATEGORY_REAL, 0}}},
    {2, (CompFuncV)mulVFF, _dxfComputeCheckMatch,
	{0, TYPE_FLOAT, CATEGORY_REAL, 0 },
	{   {0, TYPE_FLOAT, CATEGORY_REAL, 1, { 1 }},
	    {0, TYPE_FLOAT, CATEGORY_REAL, 0}}},
    {2, (CompFuncV)mulVFF, _dxfComputeCheckMatch,
	{0, TYPE_FLOAT, CATEGORY_REAL, 0 },
	{   {0, TYPE_FLOAT, CATEGORY_REAL, 0},
	    {0, TYPE_FLOAT, CATEGORY_REAL, 1, { 1 }}}}
};

/* magv */
static int
magvF(
    PTreeNode *pt, 
    ObjStruct *os, 
    int numInputs,
    Pointer *inputs,
    Pointer result,
    InvalidComponentHandle *invalids,
    InvalidComponentHandle outInvalid)
{
    register float *out = (float *) result;
    register float *in0 = (float *) inputs[0];
    int vectorLen = pt->metaType.shape[0];
    int i, j;
    double   temp;
    int allValid = (invalids[0] == NULL || DXGetInvalidCount(invalids[0]) == 0);

    for (i = 0; i < pt->metaType.items; ++i) {
	if (allValid || _dxfComputeInvalidOne(outInvalid, i, invalids[0], i)) {
	    temp = (double)in0[i * vectorLen + 0] *
		   (double)in0[i * vectorLen + 0];
	    for (j = 1; j < vectorLen; ++j) {
		temp += (double)in0[i * vectorLen + j] *
			(double)in0[i * vectorLen + j];
	    }
	    out[i] = sqrt (temp);
	}
    }

    return (OK);
}
static OperBinding magvs[] = {
    {1, (CompFuncV)magvF, _dxfComputeCheckVects,
	{0, TYPE_FLOAT, CATEGORY_REAL, 0 },
	{   {0, TYPE_FLOAT, CATEGORY_REAL}}},
    {1, (CompFuncV)absF, _dxfComputeCheckMatch,
	{0, TYPE_FLOAT, CATEGORY_REAL, 0 },
	{   {0, TYPE_FLOAT, CATEGORY_REAL, 0}}},
    {1, (CompFuncV)absD, _dxfComputeCheckMatch,
	{0, TYPE_DOUBLE, CATEGORY_REAL, 0 },
	{   {0, TYPE_DOUBLE, CATEGORY_REAL, 0}}}
};

static int
normF(
    PTreeNode *pt, 
    ObjStruct *os, 
    int numInputs,
    Pointer *inputs,
    Pointer result,
    InvalidComponentHandle *invalids,
    InvalidComponentHandle outInvalid)
{
    register float *out = (float *) result;
    register float *in0 = (float *) inputs[0];
    register double mag;
    int vectorLen = pt->metaType.shape[0];
    int i, j;
    int items;
    int allValid =
	(invalids[0] == NULL || DXGetInvalidCount(invalids[0]) == 0);

    items = pt->metaType.items;
    for (i = 0; i < items; ++i) {
	if (allValid || _dxfComputeInvalidOne(outInvalid, i, invalids[0], i)) {
	    mag = (double)in0[i * vectorLen + 0] *
		  (double)in0[i * vectorLen + 0];
	    for (j = 1; j < vectorLen; ++j) {
		mag += (double)in0[i * vectorLen + j] *
		       (double)in0[i * vectorLen + j];
	    }
	    if (mag != 0.0)
		mag = 1. / sqrt(mag);
	    for (j = 0; j < vectorLen; ++j) {
		out[i * vectorLen + j] = mag * in0[i * vectorLen + j];
	    }
	}
    }
    return (OK);
}
#define SIGNF(x) ((float)SIGN(x))
VFUNC1 (normSF, float, float,  SIGNF)

static OperBinding norms[] = {
    {1, (CompFuncV)normF, _dxfComputeCheckVects,
	{0, TYPE_FLOAT, CATEGORY_REAL, 1 },
	{   {0, TYPE_FLOAT, CATEGORY_REAL}}},
    {1, (CompFuncV)normSF, _dxfComputeCheckMatch,
	{0, TYPE_FLOAT, CATEGORY_REAL, 0 },
	{   {0, TYPE_FLOAT, CATEGORY_REAL, 0}}}
};


static int
CheckRank(PTreeNode *pt, ObjStruct *os, OperBinding *binding)
{
    int numArgs;
    PTreeNode *arg;

    for (numArgs = 0, arg = pt->args; arg; ++numArgs, arg = arg->next)
	;
    if (numArgs != binding->numArgs) {
	DXSetError(ERROR_BAD_TYPE, "#11945", binding->numArgs);
	return (ERROR);
    }

    pt->metaType.items = 1;
    pt->metaType.type = TYPE_INT;
    pt->metaType.category = CATEGORY_REAL;
    pt->metaType.rank = 0;
    pt->impl = binding->impl;

    if (numArgs == 2 &&
	_dxfComputeCompareType (&intExpr, &pt->args->next->metaType) == ERROR) {
	DXSetError (ERROR_BAD_TYPE, 
	    "#11980");
	return (ERROR);
    }

    return (OK);
}

static int
ExecRank (
    PTreeNode *pt, 
    ObjStruct *os, 
    int numInputs,
    Pointer *inputs,
    Pointer result,
    InvalidComponentHandle *invalids,
    InvalidComponentHandle outInvalid)
{
    *(int *)result = pt->args->metaType.rank;
    return (OK);
}
static int
ExecShape (
    PTreeNode *pt, 
    ObjStruct *os, 
    int numInputs,
    Pointer *inputs,
    Pointer result,
    InvalidComponentHandle *invalids,
    InvalidComponentHandle outInvalid)
{
    if (*(int*)(inputs[1]) < pt->args->metaType.rank) {
	*(int *)result = pt->args->metaType.shape[*(int*)(inputs[1])];
	return (OK);
    }
    else {
	DXSetError (ERROR_BAD_TYPE, "#11970");
	return (ERROR);
    }
}
static OperBinding shapes[] = {
    {2, (CompFuncV)ExecShape, CheckRank}
};
static OperBinding ranks[] = {
    {1, (CompFuncV)ExecRank, CheckRank}
};


static int
randNULL(
    PTreeNode *pt, 
    ObjStruct *os, 
    int numInputs,
    Pointer *inputs,
    Pointer result,
    InvalidComponentHandle *invalids,
    InvalidComponentHandle outInvalid)
{
    register float *out = (float *) result;
    int i, j, items;
    int *seed0 = (int *)inputs[1];
    int seed;
    int numBasic;
#define RANDSORTBINS 1023
    float rands[RANDSORTBINS];
    int index;

    seed = *seed0 + os->metaType.id;

#ifdef COMP_DEBUG
    DXDebug ("C", "randNULL, %d: pt = 0x%08x, os = 0x%08x, os items = %d, pt items = %d, seed0 = %d, seed = %d",
	__LINE__, pt, os, os->metaType.items, pt->metaType.items, *seed0, seed);
#endif

    srandom(seed);
    for (i = 0; i<RANDSORTBINS; ++i)
	rands[i] = ((float)random()/RAND_MAX);

    for (numBasic = 1, i = 0; i < pt->metaType.rank; i++)
	numBasic *= pt->metaType.shape[i];
    numBasic *= DXCategorySize(pt->metaType.category);

    items = pt->metaType.items;
    for (i = 0; i < items; ++i) 
	for (j = 0; j < numBasic; j++) {
	    index = (int)((float)random()/RAND_MAX)*RANDSORTBINS;
	    out[i*numBasic + j] = rands[index];
	    rands[index] = ((float)random()/RAND_MAX);
	}
    return (OK);
}

static int
CheckRAND (PTreeNode *pt, ObjStruct *os, OperBinding *binding)
{
    PTreeNode *stuff;
    PTreeNode *seed;
    int i;

    /* Ensure that vect is at least a vector, and select is an int */
    stuff = pt->args;
    if (stuff == NULL) {
	DXSetError (ERROR_INTERNAL, "CheckRand, %d: no input object",
	    __LINE__);
	return (ERROR);
    }

    seed = stuff->next;
    if (seed == NULL) {
	DXSetError (ERROR_INTERNAL, "CheckRand, %d: no seed expression",
	    __LINE__);
	return (ERROR);
    }
    if (_dxfComputeCompareType (&intExpr, &seed->metaType) == ERROR) {
	return (ERROR);
    }
    pt->metaType.type = binding->outType.type;
    pt->metaType.category = binding->outType.category;
    pt->metaType.rank = stuff->metaType.rank;
    for (i=0; i<pt->metaType.rank; i++)
        pt->metaType.shape[i] = stuff->metaType.shape[i];
    pt->impl = binding->impl;

    return (OK);
}

static OperBinding randoms[] = {
    {2, (CompFuncV)randNULL, CheckRAND,
	{1, TYPE_FLOAT, CATEGORY_REAL, 0},
	{ {1, TYPE_FLOAT, CATEGORY_REAL, 0},
	  {1, TYPE_INT, CATEGORY_REAL, 0}}
    }
};

extern OperBinding _dxdComputeAdds[];
extern OperBinding _dxdComputeSubs[];
extern OperBinding _dxdComputeLts[];
extern OperBinding _dxdComputeLes[];
extern OperBinding _dxdComputeGts[];
extern OperBinding _dxdComputeGes[];
extern OperBinding _dxdComputeEqs[];
extern OperBinding _dxdComputeNes[];
extern OperBinding _dxdComputeAnds[];
extern OperBinding _dxdComputeOrs[];
extern OperBinding _dxdComputeNots[];
extern OperBinding _dxdComputeMins[];
extern OperBinding _dxdComputeMaxs[];
extern OperBinding _dxdComputeInts[];
extern OperBinding _dxdComputeFloats[];
extern OperBinding _dxdComputeReals[];
extern OperBinding _dxdComputeImags[];
extern OperBinding _dxdComputeComplexes[];
extern OperBinding _dxdComputeImagis[];
extern OperBinding _dxdComputeImagjs[];
extern OperBinding _dxdComputeImagks[];
extern OperBinding _dxdComputeQuaternions[];
extern OperBinding _dxdComputeSigns[];
extern OperBinding _dxdComputeUbytes[];
extern OperBinding _dxdComputeShorts[];
extern OperBinding _dxdComputeDoubles[];
extern OperBinding _dxdComputeSigned_ints[];
extern OperBinding _dxdComputeBands[];
extern OperBinding _dxdComputeBors[];
extern OperBinding _dxdComputeBxors[];
extern OperBinding _dxdComputeBnots[];
extern OperBinding _dxdComputeSbytes[];
extern OperBinding _dxdComputeUints[];
extern OperBinding _dxdComputeUshorts[];
extern OperBinding _dxdComputeInvalids[];
extern OperBinding _dxdComputeFloors[];
extern OperBinding _dxdComputeCeils[];
extern OperBinding _dxdComputeTruncs[];
extern OperBinding _dxdComputeRints[];

extern int _dxdComputeAddsSize;
extern int _dxdComputeSubsSize;
extern int _dxdComputeLtsSize;
extern int _dxdComputeLesSize;
extern int _dxdComputeGtsSize;
extern int _dxdComputeGesSize;
extern int _dxdComputeEqsSize;
extern int _dxdComputeNesSize;
extern int _dxdComputeAndsSize;
extern int _dxdComputeOrsSize;
extern int _dxdComputeNotsSize;
extern int _dxdComputeMinsSize;
extern int _dxdComputeMaxsSize;
extern int _dxdComputeIntsSize;
extern int _dxdComputeFloatsSize;
extern int _dxdComputeRealsSize;
extern int _dxdComputeImagsSize;
extern int _dxdComputeComplexesSize;
extern int _dxdComputeImagisSize;
extern int _dxdComputeImagjsSize;
extern int _dxdComputeImagksSize;
extern int _dxdComputeQuaternionsSize;
extern int _dxdComputeSignsSize;
extern int _dxdComputeUbytesSize;
extern int _dxdComputeShortsSize;
extern int _dxdComputeDoublesSize;
extern int _dxdComputeSigned_intsSize;
extern int _dxdComputeBandsSize;
extern int _dxdComputeBorsSize;
extern int _dxdComputeBxorsSize;
extern int _dxdComputeBnotsSize;
extern int _dxdComputeSbytesSize;
extern int _dxdComputeUintsSize;
extern int _dxdComputeUshortsSize;
extern int _dxdComputeInvalidsSize;
extern int _dxdComputeFloorsSize;
extern int _dxdComputeCeilsSize;
extern int _dxdComputeTruncsSize;
extern int _dxdComputeRintsSize;


/* Here is the operator table that ties it all together */
static OperDesc operators[] = {
    OP_RECORD (NT_INPUT,	"input",	inputs),
    OP_RECORD (NT_CONSTANT,	"constant",	consts),
    OP_RECORD (NT_CONSTRUCT,	"vector constructor", construct),
    OP_RECORD (NT_COND,		"conditional",	cond),
/*     OP_RECORD (NT_TOP,		"top",		tops), */

    OP_RECORD_SIZE (OPER_PLUS,	"+",		_dxdComputeAdds, _dxdComputeAddsSize),
    OP_RECORD_SIZE (OPER_MINUS,	"-",		_dxdComputeSubs, _dxdComputeSubsSize),
    OP_RECORD (OPER_MUL,	"*",		muls),
    OP_RECORD (OPER_DIV,	"/",		divs),
    OP_RECORD (OPER_MOD,	"%",		mods),
    OP_RECORD (OPER_CROSS,	"cross",	crosses),
    OP_RECORD (OPER_DOT,	"dot",		dots),
    OP_RECORD_SIZE (OPER_LT,	"<",		_dxdComputeLts, _dxdComputeLtsSize),
    OP_RECORD_SIZE (OPER_LE,	"<=",		_dxdComputeLes, _dxdComputeLesSize),
    OP_RECORD_SIZE (OPER_GT,	">",		_dxdComputeGts, _dxdComputeGtsSize),
    OP_RECORD_SIZE (OPER_GE,	">=",		_dxdComputeGes, _dxdComputeGesSize),
    OP_RECORD_SIZE (OPER_EQ,	"==",		_dxdComputeEqs, _dxdComputeEqsSize),
    OP_RECORD_SIZE (OPER_NE,	"!=",		_dxdComputeNes, _dxdComputeNesSize),
    OP_RECORD_SIZE (OPER_AND,	"&&",		_dxdComputeAnds, _dxdComputeAndsSize),
    OP_RECORD_SIZE (OPER_OR,	"||",		_dxdComputeOrs, _dxdComputeOrsSize),
    OP_RECORD (OPER_EXP,	"^",		pows),
    OP_RECORD (OPER_PERIOD,	".",		selects),
    OP_RECORD_SIZE (OPER_NOT,	"!",		_dxdComputeNots, _dxdComputeNotsSize),
    OP_RECORD (FUNC_sqrt,	"sqrt",		sqrts),
    OP_RECORD (FUNC_pow,	"pow",		pows),
    OP_RECORD (FUNC_sin,	"sin",		sins),
    OP_RECORD (FUNC_cos,	"cos",		coss),
    OP_RECORD (FUNC_tan,	"tan",		tans),
    OP_RECORD (FUNC_ln,		"ln",		lns),
    OP_RECORD (FUNC_log10,	"log10",	log10s),
    OP_RECORD_SIZE (FUNC_min,	"min",		_dxdComputeMins, _dxdComputeMinsSize),
    OP_RECORD_SIZE (FUNC_max,	"max",		_dxdComputeMaxs, _dxdComputeMaxsSize),
    OP_RECORD_SIZE (FUNC_floor,	"floor",	_dxdComputeFloors, _dxdComputeFloorsSize),
    OP_RECORD_SIZE (FUNC_ceil,	"ceil",		_dxdComputeCeils, _dxdComputeFloorsSize),
    OP_RECORD_SIZE (FUNC_trunc,	"trunc",	_dxdComputeTruncs, _dxdComputeFloorsSize),
    OP_RECORD_SIZE (FUNC_rint,	"rint",		_dxdComputeRints, _dxdComputeFloorsSize),
    OP_RECORD (FUNC_exp,	"exp",		exps),
    OP_RECORD (FUNC_tanh,	"tanh",		tanhs),
    OP_RECORD (FUNC_sinh,	"sinh",		sinhs),
    OP_RECORD (FUNC_cosh,	"cosh",		coshs),
    OP_RECORD (FUNC_acos,	"acos",		acoss),
    OP_RECORD (FUNC_asin,	"asin",		asins),
    OP_RECORD (FUNC_atan,	"atan",		atans),
    OP_RECORD (FUNC_atan2,	"atan2",	atan2s),
    OP_RECORD (FUNC_strcmp,	"strcmp",	strcmps),
    OP_RECORD (FUNC_stricmp,	"stricmp",	stricmps),
    OP_RECORD (FUNC_strstr,	"strstr",	strstrs),
    OP_RECORD (FUNC_stristr,	"stristr",	stristrs),
    OP_RECORD (FUNC_strlen,	"strlen",	strlens),
    OP_RECORD (FUNC_mag,	"mag",		magvs),
    OP_RECORD (FUNC_norm,	"norm",		norms),
    OP_RECORD (FUNC_abs,	"abs",		abss),
    OP_RECORD_SIZE (FUNC_int,	"int",		_dxdComputeInts, _dxdComputeIntsSize),
    OP_RECORD_SIZE (FUNC_float,	"float",	_dxdComputeFloats, _dxdComputeFloatsSize),
    OP_RECORD_SIZE (FUNC_real,	"real",		_dxdComputeReals, _dxdComputeRealsSize),
    OP_RECORD_SIZE (FUNC_imag,	"imag",		_dxdComputeImags, _dxdComputeImagsSize),
    OP_RECORD_SIZE (FUNC_complex,"complex",	_dxdComputeComplexes, _dxdComputeComplexesSize),
    OP_RECORD_SIZE (FUNC_imagi,	"imagi",	_dxdComputeImagis, _dxdComputeImagisSize),
    OP_RECORD_SIZE (FUNC_imagj,	"imagj",	_dxdComputeImagjs, _dxdComputeImagjsSize),
    OP_RECORD_SIZE (FUNC_imagk,	"imagk",	_dxdComputeImagks, _dxdComputeImagksSize),
    OP_RECORD_SIZE (FUNC_quaternion,	"quaternion",	_dxdComputeQuaternions, _dxdComputeQuaternionsSize),
#if defined(HAVE_FINITE)
    OP_RECORD (FUNC_finite,     "finite",  finites),
#endif
#if defined(HAVE_ISNAN)
    OP_RECORD (FUNC_isnan,      "isnan",  isnans),
#endif
    /* Not available on the i860 version 
     * OP_RECORD (FUNC_acosh,	"acosh",	acoshs),
     * OP_RECORD (FUNC_asinh,	"asinh",	asinhs),
     * OP_RECORD (FUNC_atanh,	"atanh",	atanhs),
     */
    OP_RECORD_SIZE (FUNC_sign,	"sign",		_dxdComputeSigns, _dxdComputeSignsSize),
    OP_RECORD (FUNC_qsin,	"qsin",		qsins),
    OP_RECORD (FUNC_qcos,	"qcos",		qcoss),
    OP_RECORD_SIZE (FUNC_char,	"char",		_dxdComputeUbytes, _dxdComputeUbytesSize),
    OP_RECORD_SIZE (FUNC_short,	"short",	_dxdComputeShorts, _dxdComputeShortsSize),
    OP_RECORD_SIZE (FUNC_double,"double",	_dxdComputeDoubles, _dxdComputeDoublesSize),
    OP_RECORD_SIZE (FUNC_signed_int,	"signed_int",	_dxdComputeSigned_ints, _dxdComputeSigned_intsSize),
    OP_RECORD (FUNC_rank,	"rank",		ranks),
    OP_RECORD (FUNC_shape,	"shape",	shapes),
    OP_RECORD_SIZE (FUNC_and,	"and",		_dxdComputeBands, _dxdComputeBandsSize),
    OP_RECORD_SIZE (FUNC_or,	"or",		_dxdComputeBors, _dxdComputeBorsSize),
    OP_RECORD_SIZE (FUNC_xor,	"xor",		_dxdComputeBxors, _dxdComputeBxorsSize),
    OP_RECORD_SIZE (FUNC_not,	"not",		_dxdComputeBnots, _dxdComputeBnotsSize),
    OP_RECORD (FUNC_random,	"random",	randoms),
    OP_RECORD (FUNC_arg,	"arg",		args),
    OP_RECORD_SIZE (FUNC_invalid,"invalid",	_dxdComputeInvalids, _dxdComputeInvalidsSize),
    OP_RECORD_SIZE (FUNC_sbyte,	"sbyte",	_dxdComputeSbytes, _dxdComputeSbytesSize),
    OP_RECORD_SIZE (FUNC_uint, 	"uint",		_dxdComputeUints, _dxdComputeUintsSize),
    OP_RECORD_SIZE (FUNC_ushort, "ushort",	_dxdComputeUshorts, _dxdComputeUshortsSize),
    OP_RECORD (OPER_ASSIGNMENT, "assignment", assignments),
    OP_RECORD (OPER_VARIABLE, "variable", variables)
};

    
static int
TypeCheckLeaf(PTreeNode *tree, ObjStruct *input)
{
    PTreeNode *subTree;
    int i;
    int knownOpers = sizeof (operators) / sizeof (operators[0]);
    int b;
    int op = tree->oper;
    OperBinding *bindings;
    int numBindings;
    PTreeNode *newTree;
    PTreeNode **nextPtr;

#ifdef COMP_DEBUG
    DXDebug ("C", "TypeCheckLeaf for parse node 0x%08x and leaf 0x%08x",
	tree, input);
#endif

    /* Conditional ShortCut to avoid checking and doing one side or the
     * other if the conditional is a constant 
     */
    if (tree->oper == NT_COND) {
	for (i = 0; i < knownOpers && op != operators[i].op; ++i)
	    ;
	bindings = operators[i].bindings;
	if (CheckCond(tree, input, &bindings[0]) == ERROR) {
	    /* We failed, skip to the regular promotion stuff */
	}
	else {
	    DXResetError();
	    tree->operName = operators[i].name;
	    return (OK);
	}
    }

    /* MAIN PATH:
     * Resolve types of subtrees
     */
    subTree = tree->args;
    while (subTree != NULL) {
	if (TypeCheckLeaf(subTree, input) != OK) {
	    return (ERROR);
	}
	if (tree->metaType.items < subTree->metaType.items)
	    tree->metaType.items = subTree->metaType.items;

	subTree = subTree->next;
    }


    /* Find the appropriate operator */
    for (i = 0; i < knownOpers; ++i) {
	if (op == operators[i].op) {
	    bindings = operators[i].bindings;
	    numBindings = operators[i].numBindings;

	    /* Search For the Correct Binding */
	    for (b = 0; b < numBindings; ++b) {
		if ((*bindings[b].typeCheck)(tree, input, bindings + b) == OK) {
#ifdef COMP_DEBUG
		    DXDebug ("C", "TypeCheckLeaf: items in tree 0x%08x after checking = %d", 
			tree, tree->metaType.items);
#endif
		    tree->operName = operators[i].name;
		    DXResetError();
		    return (OK);
		}
	    }

	    /* We failed, Promote all small ints to ints and try again */
	    if (tree->oper != FUNC_int)
	    {
		subTree = tree->args;
		nextPtr = &tree->args;
		while (subTree != NULL) {
		    if ((subTree->metaType.type == TYPE_UBYTE ||
			subTree->metaType.type == TYPE_BYTE ||
			subTree->metaType.type == TYPE_USHORT ||
			subTree->metaType.type == TYPE_SHORT ||
			subTree->metaType.type == TYPE_UINT) &&
		            (subTree->metaType.category == CATEGORY_REAL ||
			     subTree->metaType.category == CATEGORY_COMPLEX)) {
			newTree = _dxfMakeFunCall ("int", subTree);
			newTree->next = subTree->next;
			subTree->next = NULL;
			*nextPtr = newTree;
			/* Type check the new node */
			if (TypeCheckLeaf(newTree, input) != OK) {
			    DXSetError (ERROR_NOT_IMPLEMENTED, "#11950");
			    return (ERROR);
			}
			nextPtr = &newTree->next;
			subTree = newTree->next;
		    }
		    else {
			nextPtr = &subTree->next;
			subTree = subTree->next;
		    }
		}
	    }
	    else
	    {
		DXSetError (ERROR_NOT_IMPLEMENTED, "#11950");
		return (ERROR);
	    }
	    for (b = 0; b < numBindings; ++b) {
		if ((*bindings[b].typeCheck)(tree, input, bindings + b) == OK) {
#ifdef COMP_DEBUG
		    DXDebug ("C", "TypeCheckLeaf: items in tree 0x%08x after checking = %d", 
			tree, tree->metaType.items);
#endif
		    tree->operName = operators[i].name;
		    DXResetError();
		    return (OK);
		}
	    }

	    /* We failed, Promote all non-floats to floats and try again */
	    if (tree->oper != FUNC_float)
	    {
		subTree = tree->args;
		nextPtr = &tree->args;
		for (i = 0; subTree != NULL; ++i) {
		    if ((tree->oper != NT_COND || i != 0) &&
			    subTree->metaType.type != TYPE_FLOAT &&
			    (subTree->metaType.category == CATEGORY_REAL ||
			     subTree->metaType.category == CATEGORY_COMPLEX)) {
			if (subTree->metaType.type == TYPE_DOUBLE)
			    DXWarning("#11960", operators[i].name);
			newTree = _dxfMakeFunCall ("float", subTree);
			newTree->next = subTree->next;
			subTree->next = NULL;
			*nextPtr = newTree;
			/* Type check the new node */
			if (TypeCheckLeaf(newTree, input) != OK) {
			    DXSetError (ERROR_NOT_IMPLEMENTED, "#11950");
			    return (ERROR);
			}
			nextPtr = &newTree->next;
			subTree = newTree->next;
		    }
		    else {
			nextPtr = &subTree->next;
			subTree = subTree->next;
		    }
		}
	    }
	    else
	    {
		DXSetError (ERROR_NOT_IMPLEMENTED, "#11950");
		return (ERROR);
	    }
	    for (b = 0; b < numBindings; ++b) {
		if ((*bindings[b].typeCheck)(tree, input, bindings + b) == OK) {
#ifdef COMP_DEBUG
		    DXDebug ("C", "TypeCheckLeaf: items in tree 0x%08x after checking = %d", 
			tree, tree->metaType.items);
#endif
		    tree->operName = operators[i].name;
		    DXResetError();
		    return (OK);
		}
	    }

	    /* We failed, Demote all 1 vectors to scalars and try again */
	    if (tree->oper != FUNC_float)
	    {
		subTree = tree->args;
		nextPtr = &tree->args;
		while (subTree != NULL) {
		    if (subTree->metaType.rank == 1 &&
			    subTree->metaType.shape[0] == 1) {
			newTree = _dxfMakeFunCall ("select", subTree);
			newTree->next = subTree->next;
			subTree->next = _dxfMakeArg(NT_CONSTANT);
			subTree->next->metaType.items = 1;
			subTree->next->metaType.type = TYPE_INT;
			subTree->next->metaType.category = CATEGORY_REAL;
			subTree->next->u.i = 0;
			subTree->next->next = NULL;
			*nextPtr = newTree;
			/* Type check the new node */
			if (TypeCheckLeaf(newTree, input) != OK) {
			    DXSetError (ERROR_NOT_IMPLEMENTED, "#11950");
			    return (ERROR);
			}
			nextPtr = &newTree->next;
			subTree = newTree->next;
		    }
		    else {
			nextPtr = &subTree->next;
			subTree = subTree->next;
		    }
		}
	    }
	    else
	    {
		DXSetError (ERROR_NOT_IMPLEMENTED, "#11950");
		return (ERROR);
	    }
	    for (b = 0; b < numBindings; ++b) {
		if ((*bindings[b].typeCheck)(tree, input, bindings + b) == OK) {
#ifdef COMP_DEBUG
		    DXDebug ("C", "TypeCheckLeaf: items in tree 0x%08x after checking = %d", 
			tree, tree->metaType.items);
#endif
		    tree->operName = operators[i].name;
		    DXResetError();
		    return (OK);
		}
	    }

	    /* We failed, Promote all floats to complex and try again */
	    if (tree->oper != FUNC_complex)
	    {
		subTree = tree->args;
		nextPtr = &tree->args;
		for (i = 0; subTree != NULL; ++i) {
		    if ((tree->oper != NT_COND || i != 0) &&
			    (subTree->metaType.category == CATEGORY_REAL)) { 
			if (subTree->metaType.type == TYPE_DOUBLE)
			    DXWarning("#11960", operators[i].name);
			newTree = _dxfMakeFunCall ("complex", subTree);
			newTree->next = subTree->next;
			subTree->next = NULL;
			*nextPtr = newTree;
			/* Type check the new node */
			if (TypeCheckLeaf(newTree, input) != OK) {
			    DXSetError (ERROR_NOT_IMPLEMENTED, "#11950");
			    return (ERROR);
			}
			nextPtr = &newTree->next;
			subTree = newTree->next;
		    }
		    else {
			nextPtr = &subTree->next;
			subTree = subTree->next;
		    }
		}
	    }
	    else
	    {
		DXSetError (ERROR_NOT_IMPLEMENTED, "#11950");
		return (ERROR);
	    }
	    for (b = 0; b < numBindings; ++b) {
		if ((*bindings[b].typeCheck)(tree, input, bindings + b) == OK) {
#ifdef COMP_DEBUG
		    DXDebug ("C", "TypeCheckLeaf: items in tree 0x%08x after checking = %d", 
			tree, tree->metaType.items);
#endif
		    tree->operName = operators[i].name;
		    DXResetError();
		    return (OK);
		}
	    }


	    /* We failed again, error out */
	    if (DXGetError() == ERROR_NONE)
		DXSetError(ERROR_BAD_TYPE, "#11940", operators[i].name);
	    else
		DXAddMessage("#11940", operators[i].name);
	    return (ERROR);
	}
    }

    DXSetError (ERROR_INTERNAL, "#11930", op);
    return (ERROR);
}

int
_dxfComputeTypeCheck(PTreeNode *tree, ObjStruct *input)
{
#ifdef COMP_DEBUG
    DXDebug ("C", "_dxfComputeTypeCheck (tree = 0x%08x, input = 0x%08x)", 
	tree, input);
#endif
    if (input->class != CLASS_ARRAY && input->class != CLASS_FIELD && input->class != CLASS_STRING) {
	DXSetError(ERROR_BAD_CLASS, "#10370", "leaves", "field or numeric list");
	return ERROR;
    }

    input->parseTree = _dxfComputeCopyTree (tree);
    if (input->parseTree == NULL)
	return ERROR;
    else
	return TypeCheckLeaf(input->parseTree, input);
}

/*
 * _dxfComputeInitExecution()
 * This initializes lots of execution stuff, and should be called once per
 * processor per object computed upon.
 */
void
_dxfComputeInitExecution(void)
{
    int i;
    int knownOpers = sizeof (operators) / sizeof (operators[0]);

    for (i = 0; i < knownOpers; ++i) {
	switch (operators[i].op) {
	case OPER_PLUS:
	    operators[i].numBindings = _dxdComputeAddsSize;
	    break;
	case OPER_MINUS:
	    operators[i].numBindings = _dxdComputeSubsSize;
	    break;
	case OPER_LT:
	    operators[i].numBindings = _dxdComputeLtsSize;
	    break;
	case OPER_LE:
	    operators[i].numBindings = _dxdComputeLesSize;
	    break;
	case OPER_GT:
	    operators[i].numBindings = _dxdComputeGtsSize;
	    break;
	case OPER_GE:
	    operators[i].numBindings = _dxdComputeGesSize;
	    break;
	case OPER_EQ:
	    operators[i].numBindings = _dxdComputeEqsSize;
	    break;
	case OPER_NE:
	    operators[i].numBindings = _dxdComputeNesSize;
	    break;
	case OPER_AND:
	    operators[i].numBindings = _dxdComputeAndsSize;
	    break;
	case OPER_OR:
	    operators[i].numBindings = _dxdComputeOrsSize;
	    break;
	case OPER_NOT:
	    operators[i].numBindings = _dxdComputeNotsSize;
	    break;
	case FUNC_min:
	    operators[i].numBindings = _dxdComputeMinsSize;
	    break;
	case FUNC_max:
	    operators[i].numBindings = _dxdComputeMaxsSize;
	    break;
	case FUNC_int:
	    operators[i].numBindings = _dxdComputeIntsSize;
	    break;
	case FUNC_float:
	    operators[i].numBindings = _dxdComputeFloatsSize;
	    break;
	case FUNC_real:
	    operators[i].numBindings = _dxdComputeRealsSize;
	    break;
	case FUNC_imag:
	    operators[i].numBindings = _dxdComputeImagsSize;
	    break;
	case FUNC_complex:
	    operators[i].numBindings = _dxdComputeComplexesSize;
	    break;
	case FUNC_imagi:
	    operators[i].numBindings = _dxdComputeImagisSize;
	    break;
	case FUNC_imagj:
	    operators[i].numBindings =  _dxdComputeImagjsSize;
	    break;
	case FUNC_imagk:
	    operators[i].numBindings =  _dxdComputeImagksSize;
	    break;
	case FUNC_quaternion:
	    operators[i].numBindings =  _dxdComputeQuaternionsSize;
	    break;
	case FUNC_sign:
	    operators[i].numBindings =  _dxdComputeSignsSize;
	    break;
	case FUNC_char:
	    operators[i].numBindings =  _dxdComputeUbytesSize;
	    break;
	case FUNC_short:
	    operators[i].numBindings =  _dxdComputeShortsSize;
	    break;
	case FUNC_double:
	    operators[i].numBindings = _dxdComputeDoublesSize;
	    break;
	case FUNC_signed_int:
	    operators[i].numBindings =  _dxdComputeSigned_intsSize;
	    break;
	case FUNC_and:
	    operators[i].numBindings = _dxdComputeBandsSize;
	    break;
	case FUNC_or:
	    operators[i].numBindings = _dxdComputeBorsSize;
	    break;
	case FUNC_xor:
	    operators[i].numBindings = _dxdComputeBxorsSize;
	    break;
	case FUNC_not:
	    operators[i].numBindings = _dxdComputeBnotsSize;
	    break;
	case FUNC_invalid:
	    operators[i].numBindings = _dxdComputeInvalidsSize;
	    break;
	case FUNC_sbyte:
	    operators[i].numBindings = _dxdComputeSbytesSize;
	    break;
	case FUNC_uint:
	    operators[i].numBindings = _dxdComputeUintsSize;
	    break;
	case FUNC_ushort:
	    operators[i].numBindings = _dxdComputeUshortsSize;
	    break;
	case FUNC_floor:
	    operators[i].numBindings = _dxdComputeFloorsSize;
	    break;
	case FUNC_ceil:
	    operators[i].numBindings = _dxdComputeCeilsSize;
	    break;
	case FUNC_trunc:
	    operators[i].numBindings = _dxdComputeTruncsSize;
	    break;
	case FUNC_rint:
	    operators[i].numBindings = _dxdComputeRintsSize;
	    break;
	}
    }
}

int _dxfComputeFinishExecution()
{
    ClearSymbolTable();
    return OK;
}

#if !defined(DXD_STANDARD_IEEE)
static jmp_buf broken;
static void handlefp(void) {
    longjmp(broken, 1);
}
#endif

/*
 * _dxfComputeExecuteNode allocates memory for each sub-argument, and then
 * passes the results off to the function which performs this node.
 */
int
_dxfComputeExecuteNode(
    PTreeNode *pt,
    ObjStruct *os,
    Pointer result,
    InvalidComponentHandle *invalid)
{
    int numArgs, i, argIndex;
    int numBasic;
    int subSize;
    Pointer *subData = NULL;
    InvalidComponentHandle *subInvalids = NULL;
    PTreeNode *args;
    int status = ERROR;

    *invalid = NULL;

#ifdef COMP_DEBUG
    DXDebug ("C", "_dxfComputeExecuteNode, %d: pt = 0x%08x, os = 0x%08x",
	__LINE__, pt, os);
#endif
    
    /* Conditional ShortCut */
    while (pt->oper == NT_COND && pt->args->metaType.items == 1) {
	_dxfComputeExecuteNode (pt->args, os, (Pointer)&status, invalid);
	if (*invalid == NULL || DXGetInvalidCount(*invalid) == 0) {
	    if (status)
		pt = pt->args->next;
	    else
		pt = pt->args->next->next;
	}
	else
	    break;
    }
    if (*invalid) {
	DXFreeInvalidComponentHandle(*invalid);
    }

    /* Get type of arguments, and allocate nargs of pointers.
     * For each arg, allocate a result array, and execute it
     */
    args = pt->args;
    numArgs = 0;
    if (args) {
	/* Count Args and alloc an array of pointers and an array of
	 * invalid component handles for the called routine to fill.
	 */
	for (numArgs = 0; args != NULL; ++numArgs)
	    args = args->next;
	args = pt->args;
	subData = (Pointer *) 
	    DXAllocateLocalZero (numArgs * sizeof (Pointer));
	if (subData == NULL) {
	    DXResetError();
	    subData = (Pointer *) 
		DXAllocateZero (numArgs * sizeof (Pointer));
	    if (subData == NULL)
		goto error;
	}

	subInvalids = (InvalidComponentHandle *) 
	    DXAllocateLocalZero (numArgs * sizeof (InvalidComponentHandle));
	if (subInvalids == NULL) {
	    DXResetError();
	    subInvalids = (InvalidComponentHandle *) 
		DXAllocateZero (numArgs * sizeof (InvalidComponentHandle));
	    if (subInvalids == NULL)
		goto error;
	}
	
	for (argIndex = 0, args = pt->args; 
		args != NULL; args = args->next, 
		++argIndex) {
	    for (numBasic = 1, i = 0; i < args->metaType.rank; ++i) {
		numBasic *= args->metaType.shape[i];
	    }
	    subSize = DXTypeSize (args->metaType.type) *
		   DXCategorySize (args->metaType.category) *
		   numBasic;
	    subData[argIndex] = DXAllocateLocal(pt->metaType.items*subSize);
	    if (subData[argIndex] == NULL) {
		DXResetError();
		subData[argIndex] = DXAllocate(pt->metaType.items*subSize);
		if (subData[argIndex] == NULL) {
		    goto error;
		}
	    }

	    if (_dxfComputeExecuteNode (args,
					os,
					subData[argIndex],
					&subInvalids[argIndex]) == ERROR)
		goto error;
	}
    }
    else {
	subData = NULL;
    }

    /*
     * Allocate an invalidcomponenthandle for the work routine to fill
     */
    if (os->class == CLASS_FIELD) {
	Object a;
	char *dep;

	a = DXGetComponentAttribute((Field)os->output, "data", "dep");
	if (a && DXExtractString(a, &dep) != ERROR) {
	    *invalid = DXCreateInvalidComponentHandle(os->output, NULL, dep);
	    if (!*invalid ||
		DXSetAllValid(*invalid) == ERROR)
		goto error;
	}
    }

#if !defined(DXD_STANDARD_IEEE)
    if (setjmp(broken)) {
	DXSetError (ERROR_DATA_INVALID, "#11790");   /* fpe */
	goto error;
    }
    signal (SIGFPE, handlefp);
#endif

    status = (*pt->impl) (pt, os, numArgs, subData, result,
			  subInvalids, *invalid);

#if !defined(DXD_STANDARD_IEEE)
    signal (SIGFPE, SIG_DFL);
#endif

error:
    /* Free locally allocated memory */
    if (subData != NULL) {
	for (i = 0; i < numArgs; ++i) {
	    DXFree (subData[i]);
	}
	DXFree ((Pointer) subData);
    }
    if (subInvalids != NULL) {
	for (i = 0; i < numArgs; ++i) {
	    if (subInvalids[i]) {
		DXFreeInvalidComponentHandle(subInvalids[i]);
	    }
	}
	DXFree ((Pointer) subInvalids);
    }

    return (status);
}

int
_dxfComputeLookupFunction (char *name)
{
    static struct {
	char *name;
	int   code;
    } functs[] = {
	{ "select",	OPER_PERIOD },
	{ "mod",	OPER_MOD },
	{ "dot",	OPER_DOT },
	{ "cross",	OPER_CROSS },
	{ "sqrt",	FUNC_sqrt },
	{ "pow",	FUNC_pow },
	{ "sin",	FUNC_sin },
	{ "cos",	FUNC_cos },
	{ "tan",	FUNC_tan },
	{ "ln",		FUNC_ln },
	{ "log10",	FUNC_log10 },
	{ "min",	FUNC_min },
	{ "max",	FUNC_max },
	{ "floor",	FUNC_floor },
	{ "ceil",	FUNC_ceil },
	{ "trunc",	FUNC_trunc },
	{ "rint",	FUNC_rint },
	{ "exp",	FUNC_exp },
	{ "tanh",	FUNC_tanh },
	{ "sinh",	FUNC_sinh },
	{ "cosh",	FUNC_cosh },
	{ "acos",	FUNC_acos },
	{ "asin",	FUNC_asin },
	{ "atan",	FUNC_atan },
	{ "atan2",	FUNC_atan2 },
	{ "mag",	FUNC_mag },
	{ "norm",	FUNC_norm },
	{ "abs",	FUNC_abs },
	{ "int",	FUNC_int },
	{ "float",	FUNC_float },
	{ "real",	FUNC_real },
	{ "imag",	FUNC_imag },
	{ "complex",	FUNC_complex },
	{ "imagi",	FUNC_imagi },
	{ "imagj",	FUNC_imagj },
	{ "imagk",	FUNC_imagk },
	{ "quaternion",	FUNC_quaternion },
	{ "sign",	FUNC_sign },
	{ "qsin",	FUNC_qsin },
	{ "qcos",	FUNC_qcos },
	{ "char",	FUNC_char },
	{ "byte",	FUNC_char },
	{ "short",	FUNC_short },
	{ "double",	FUNC_double },
	{ "signed_int",	FUNC_signed_int },
	{ "rank",	FUNC_rank },
	{ "shape",	FUNC_shape },
	{ "log",	FUNC_ln },
	{ "and",	FUNC_and },
	{ "or",		FUNC_or },
	{ "xor",	FUNC_xor },
	{ "not",	FUNC_not },
	{ "random",	FUNC_random },
	{ "arg",	FUNC_arg },
	{ "invalid",	FUNC_invalid },
	{ "ubyte",	FUNC_char },
	{ "sbyte",	FUNC_sbyte },
	{ "uint", 	FUNC_uint },
	{ "sint", 	FUNC_int },
	{ "ushort", 	FUNC_ushort },
	{ "sshort", 	FUNC_short },
	{ "strcmp",	FUNC_strcmp },
	{ "stricmp",	FUNC_stricmp },
	{ "strstr",	FUNC_strstr },
	{ "stristr",	FUNC_stristr },
	{ "strlen",	FUNC_strlen },
	{ "finite",	FUNC_finite },
	{ "isnan",	FUNC_isnan }
    };
    int i;

    for (i = 0; i < sizeof (functs) / sizeof (functs[0]); ++i)
	if (strcmp (functs[i].name, name) == 0)
	    return (functs[i].code);

    return (NT_ERROR);
}
