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
#include <math.h>
#include <dx/dx.h>
#include "_compute.h"
#include "_compputils.h"
#include "_compoper.h"

#define SIZEINT(x) int x##Size = (sizeof (x) / sizeof(x[0]))

VSBINOP (addVSUBUB, unsigned char, unsigned char, unsigned char, +)
VSBINOP (addVSBB, signed char, signed char, signed char, +)
VSBINOP (addVSUSUS, unsigned short, unsigned short, unsigned short, +)
VSBINOP (addVSSS, signed short, signed short, signed short, +)
VSBINOP (addVSUIUI, unsigned int, unsigned int, unsigned int, +)
VSBINOP (addVSII, int, int, int, +)
VSBINOP(addVSFF, float, float, float, +)
VSBINOP(addVSDD, double, double, double, +)
VSFUNC2(addVSFFC, complexFloat, complexFloat, complexFloat, _dxfComputeAddComplexFloat)
SVBINOP (addSVUBUB, unsigned char, unsigned char, unsigned char, +)
SVBINOP (addSVBB, signed char, signed char, signed char, +)
SVBINOP (addSVUSUS, unsigned short, unsigned short, unsigned short, +)
SVBINOP (addSVSS, signed short, signed short, signed short, +)
SVBINOP (addSVUIUI, unsigned int, unsigned int, unsigned int, +)
SVBINOP (addSVII, signed int, signed int, signed int, +)
SVBINOP(addSVFF, float, float, float, +)
SVBINOP(addSVDD, double, double, double, +)
SVFUNC2(addSVFFC, complexFloat, complexFloat, complexFloat, _dxfComputeAddComplexFloat)
VBINOP (addVUBUB, unsigned char, unsigned char, unsigned char, +)
VBINOP (addVBB, signed char, signed char, signed char, +)
VBINOP (addVUSUS, unsigned short, unsigned short, unsigned short, +)
VBINOP (addVSS, signed short, signed short, signed short, +)
VBINOP (addVUIUI, unsigned int, unsigned int, unsigned int, +)
VBINOP (addVII, signed int, signed int, signed int, +)
VFUNC2 (addVFFC, complexFloat, complexFloat, complexFloat, _dxfComputeAddComplexFloat)
VBINOP (addVDD, double, double, double, +)
#ifndef KAILIB
VBINOP (addVFF, float, float, float, +)
#else
static int
addVFF(	
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
    int size0 = pt->args->metaType.items;
    int size1 = pt->args->next->metaType.items;
    int i, j;
    int numBasic;

    for (numBasic = 1, i = 0; 
	    i < pt->metaType.rank;
	    ++i)
	numBasic *= pt->metaType.shape[i];

    size0 *= numBasic;
    size1 *= numBasic;

    if (size0 > numBasic && size1 > numBasic) {
	vadd(in0, 1, in1, 1, out, 1, pt->metaType.items * numBasic);
    }
    else if (size0 > numBasic) {
	for (j = 0; j < numBasic; ++j)
	    vsadd(in0 + j, numBasic, in1[j], out + j, numBasic,
		  pt->metaType.items);
    }
    else if (size1 > numBasic) {
	for (j = 0; j < numBasic; ++j)
	    vsadd(in1 + j, numBasic, in0[j], out + j, numBasic,
		  pt->metaType.items);
    }
    else {
	vadd(in0, 1, in1, 1, out, 1, numBasic);
    }
    return (OK);
}
#endif

OperBinding _dxdComputeAdds[] = {
    {1, (CompFuncV)_dxfComputeCopy, _dxfComputeCheckSame},
    {2, (CompFuncV)addVSUBUB, _dxfComputeCheckDimAnyShape,
	{0, TYPE_UBYTE, CATEGORY_REAL},
	{   {0, TYPE_UBYTE, CATEGORY_REAL, 1},
	    {0, TYPE_UBYTE, CATEGORY_REAL, 0}}},
    {2, (CompFuncV)addVSBB, _dxfComputeCheckDimAnyShape,
	{0, TYPE_BYTE, CATEGORY_REAL},
	{   {0, TYPE_BYTE, CATEGORY_REAL, 1},
	    {0, TYPE_BYTE, CATEGORY_REAL, 0}}},
    {2, (CompFuncV)addVSUSUS, _dxfComputeCheckDimAnyShape,
	{0, TYPE_USHORT, CATEGORY_REAL},
	{   {0, TYPE_USHORT, CATEGORY_REAL, 1},
	    {0, TYPE_USHORT, CATEGORY_REAL, 0}}},
    {2, (CompFuncV)addVSSS, _dxfComputeCheckDimAnyShape,
	{0, TYPE_SHORT, CATEGORY_REAL},
	{   {0, TYPE_SHORT, CATEGORY_REAL, 1},
	    {0, TYPE_SHORT, CATEGORY_REAL, 0}}},
    {2, (CompFuncV)addVSII, _dxfComputeCheckDimAnyShape,
	{0, TYPE_INT, CATEGORY_REAL},
	{   {0, TYPE_INT, CATEGORY_REAL, 1},
	    {0, TYPE_INT, CATEGORY_REAL, 0}}},
    {2, (CompFuncV)addVSUIUI, _dxfComputeCheckDimAnyShape,
	{0, TYPE_UINT, CATEGORY_REAL},
	{   {0, TYPE_UINT, CATEGORY_REAL, 1},
	    {0, TYPE_UINT, CATEGORY_REAL, 0}}},
    {2, (CompFuncV)addVSFF, _dxfComputeCheckDimAnyShape,
	{0, TYPE_FLOAT, CATEGORY_REAL},
	{   {0, TYPE_FLOAT, CATEGORY_REAL, 1},
	    {0, TYPE_FLOAT, CATEGORY_REAL, 0}}},
    {2, (CompFuncV)addVSDD, _dxfComputeCheckDimAnyShape,
	{0, TYPE_DOUBLE, CATEGORY_REAL},
	{   {0, TYPE_DOUBLE, CATEGORY_REAL, 1},
	    {0, TYPE_FLOAT, CATEGORY_REAL, 0}}},
    {2, (CompFuncV)addVSFFC, _dxfComputeCheckDimAnyShape,
	{0, TYPE_FLOAT, CATEGORY_COMPLEX},
	{   {0, TYPE_FLOAT, CATEGORY_COMPLEX, 1},
	    {0, TYPE_FLOAT, CATEGORY_COMPLEX, 0}}},
    {2, (CompFuncV)addSVUBUB, _dxfComputeCheckDimAnyShape,
	{0, TYPE_UBYTE, CATEGORY_REAL},
	{   {0, TYPE_UBYTE, CATEGORY_REAL, 0},
	    {0, TYPE_UBYTE, CATEGORY_REAL, 1}}},
    {2, (CompFuncV)addSVBB, _dxfComputeCheckDimAnyShape,
	{0, TYPE_BYTE, CATEGORY_REAL},
	{   {0, TYPE_BYTE, CATEGORY_REAL, 0},
	    {0, TYPE_BYTE, CATEGORY_REAL, 1}}},
    {2, (CompFuncV)addSVUSUS, _dxfComputeCheckDimAnyShape,
	{0, TYPE_USHORT, CATEGORY_REAL},
	{   {0, TYPE_USHORT, CATEGORY_REAL, 0},
	    {0, TYPE_USHORT, CATEGORY_REAL, 1}}},
    {2, (CompFuncV)addSVSS, _dxfComputeCheckDimAnyShape,
	{0, TYPE_SHORT, CATEGORY_REAL},
	{   {0, TYPE_SHORT, CATEGORY_REAL, 0},
	    {0, TYPE_SHORT, CATEGORY_REAL, 1}}},
    {2, (CompFuncV)addSVUIUI, _dxfComputeCheckDimAnyShape,
	{0, TYPE_UINT, CATEGORY_REAL},
	{   {0, TYPE_UINT, CATEGORY_REAL, 0},
	    {0, TYPE_UINT, CATEGORY_REAL, 1}}},
    {2, (CompFuncV)addSVII, _dxfComputeCheckDimAnyShape,
	{0, TYPE_INT, CATEGORY_REAL},
	{   {0, TYPE_INT, CATEGORY_REAL, 0},
	    {0, TYPE_INT, CATEGORY_REAL, 1}}},
    {2, (CompFuncV)addSVFF, _dxfComputeCheckDimAnyShape,
	{0, TYPE_FLOAT, CATEGORY_REAL},
	{   {0, TYPE_FLOAT, CATEGORY_REAL, 0},
	    {0, TYPE_FLOAT, CATEGORY_REAL, 1}}},
    {2, (CompFuncV)addSVDD, _dxfComputeCheckDimAnyShape,
	{0, TYPE_DOUBLE, CATEGORY_REAL},
	{   {0, TYPE_DOUBLE, CATEGORY_REAL, 0},
	    {0, TYPE_DOUBLE, CATEGORY_REAL, 1}}},
    {2, (CompFuncV)addSVFFC, _dxfComputeCheckDimAnyShape,
	{0, TYPE_FLOAT, CATEGORY_COMPLEX},
	{   {0, TYPE_FLOAT, CATEGORY_COMPLEX, 0},
	    {0, TYPE_FLOAT, CATEGORY_COMPLEX, 1}}},
    {2, (CompFuncV)addVUBUB, _dxfComputeCheckSameShape,
	{0, TYPE_UBYTE, CATEGORY_REAL},
	{   {0, TYPE_UBYTE, CATEGORY_REAL},
	    {0, TYPE_UBYTE, CATEGORY_REAL}}},
    {2, (CompFuncV)addVBB, _dxfComputeCheckSameShape,
	{0, TYPE_BYTE, CATEGORY_REAL},
	{   {0, TYPE_BYTE, CATEGORY_REAL},
	    {0, TYPE_BYTE, CATEGORY_REAL}}},
    {2, (CompFuncV)addVUSUS, _dxfComputeCheckSameShape,
	{0, TYPE_USHORT, CATEGORY_REAL},
	{   {0, TYPE_USHORT, CATEGORY_REAL},
	    {0, TYPE_USHORT, CATEGORY_REAL}}},
    {2, (CompFuncV)addVSS, _dxfComputeCheckSameShape,
	{0, TYPE_SHORT, CATEGORY_REAL},
	{   {0, TYPE_SHORT, CATEGORY_REAL},
	    {0, TYPE_SHORT, CATEGORY_REAL}}},
    {2, (CompFuncV)addVUIUI, _dxfComputeCheckSameShape,
	{0, TYPE_UINT, CATEGORY_REAL},
	{   {0, TYPE_UINT, CATEGORY_REAL},
	    {0, TYPE_UINT, CATEGORY_REAL}}},
    {2, (CompFuncV)addVII, _dxfComputeCheckSameShape,
	{0, TYPE_INT, CATEGORY_REAL},
	{   {0, TYPE_INT, CATEGORY_REAL},
	    {0, TYPE_INT, CATEGORY_REAL}}},
    {2, (CompFuncV)addVFF, _dxfComputeCheckSameShape,
	{0, TYPE_FLOAT, CATEGORY_REAL},
	{   {0, TYPE_FLOAT, CATEGORY_REAL},
	    {0, TYPE_FLOAT, CATEGORY_REAL}}},
    {2, (CompFuncV)addVDD, _dxfComputeCheckSameShape,
	{0, TYPE_DOUBLE, CATEGORY_REAL},
	{   {0, TYPE_DOUBLE, CATEGORY_REAL},
	    {0, TYPE_DOUBLE, CATEGORY_REAL}}},
    {2, (CompFuncV)addVFFC, _dxfComputeCheckSameShape,
	{0, TYPE_FLOAT, CATEGORY_COMPLEX},
	{   {0, TYPE_FLOAT, CATEGORY_COMPLEX},
	    {0, TYPE_FLOAT, CATEGORY_COMPLEX}}}
};
SIZEINT(_dxdComputeAdds);



VBINOP (subVUBUB, unsigned char, unsigned char, unsigned char, -)
VBINOP (subVBB, signed char, signed char, signed char, -)
VBINOP (subVUSUS, unsigned short, unsigned short, unsigned short, -)
VBINOP (subVSS, signed short, signed short, signed short, -)
VBINOP (subVUIUI, unsigned int, unsigned int, unsigned int, -)
VBINOP (subVII, signed int, signed int, signed int, -)
VBINOP (subVFF, float, float, float, -)
VBINOP (subVDD, double, double, double, -)
VFUNC2 (subVFFC, complexFloat, complexFloat, complexFloat, _dxfComputeSubComplexFloat)
VSBINOP (subVSUBUB, unsigned char, unsigned char, unsigned char, -)
VSBINOP (subVSBB, signed char, signed char, signed char, -)
VSBINOP (subVSUSUS, unsigned short, unsigned short, unsigned short, -)
VSBINOP (subVSSS, signed short, signed short, signed short, -)
VSBINOP (subVSUIUI, unsigned int, unsigned int, unsigned int, -)
VSBINOP (subVSII, signed int, signed int, signed int, -)
VSBINOP (subVSFF, float, float, float, -)
VSBINOP (subVSDD, double, double, double, -)
VSFUNC2 (subVSFFC, complexFloat, complexFloat, complexFloat, _dxfComputeSubComplexFloat)
SVBINOP (subSVUBUB, unsigned char, unsigned char, unsigned char, -)
SVBINOP (subSVBB, signed char, signed char, signed char, -)
SVBINOP (subSVUSUS, unsigned short, unsigned short, unsigned short, -)
SVBINOP (subSVSS, signed short, signed short, signed short, -)
SVBINOP (subSVUIUI, unsigned int, unsigned int, unsigned int, -)
SVBINOP (subSVII, signed int, signed int, signed int, -)
SVBINOP (subSVFF, float, float, float, -)
SVBINOP (subSVDD, double, double, double, -)
SVFUNC2 (subSVFFC, complexFloat, complexFloat, complexFloat, _dxfComputeSubComplexFloat)
VUNOP (negVB, signed char, signed char, -)
VUNOP (negVS, signed short, signed short, -)
VUNOP (negVI, int, int, -)
VUNOP (negVF, float, float, -)
VUNOP (negVD, double, double, -)
VFUNC1 (negVFC, complexFloat, complexFloat, _dxfComputeNegComplexFloat)
OperBinding _dxdComputeSubs[] = {
    {1, (CompFuncV)negVB, _dxfComputeCheckSameShape,
	{0, TYPE_BYTE, CATEGORY_REAL, 1},
	{   {0, TYPE_BYTE, CATEGORY_REAL, 0}}},
    {1, (CompFuncV)negVS, _dxfComputeCheckSameShape,
	{0, TYPE_SHORT, CATEGORY_REAL, 1},
	{   {0, TYPE_SHORT, CATEGORY_REAL, 0}}},
    {1, (CompFuncV)negVI, _dxfComputeCheckSameShape,
	{0, TYPE_INT, CATEGORY_REAL, 1},
	{   {0, TYPE_INT, CATEGORY_REAL, 0}}},
    {1, (CompFuncV)negVF, _dxfComputeCheckSameShape,
	{0, TYPE_FLOAT, CATEGORY_REAL, 1},
	{   {0, TYPE_FLOAT, CATEGORY_REAL, 0}}},
    {1, (CompFuncV)negVD, _dxfComputeCheckSameShape,
	{0, TYPE_DOUBLE, CATEGORY_REAL, 1},
	{   {0, TYPE_DOUBLE, CATEGORY_REAL, 0}}},
    {1, (CompFuncV)negVFC, _dxfComputeCheckSameShape,
	{0, TYPE_FLOAT, CATEGORY_COMPLEX, 1},
	{   {0, TYPE_FLOAT, CATEGORY_COMPLEX, 0}}},
    {2, (CompFuncV)subVSUBUB, _dxfComputeCheckDimAnyShape,
	{0, TYPE_UBYTE, CATEGORY_REAL, 1},
	{   {0, TYPE_UBYTE, CATEGORY_REAL, 1},
	    {0, TYPE_UBYTE, CATEGORY_REAL, 0}}},
    {2, (CompFuncV)subVSBB, _dxfComputeCheckDimAnyShape,
	{0, TYPE_BYTE, CATEGORY_REAL, 1},
	{   {0, TYPE_BYTE, CATEGORY_REAL, 1},
	    {0, TYPE_BYTE, CATEGORY_REAL, 0}}},
    {2, (CompFuncV)subVSUSUS, _dxfComputeCheckDimAnyShape,
	{0, TYPE_USHORT, CATEGORY_REAL, 1},
	{   {0, TYPE_USHORT, CATEGORY_REAL, 1},
	    {0, TYPE_USHORT, CATEGORY_REAL, 0}}},
    {2, (CompFuncV)subVSSS, _dxfComputeCheckDimAnyShape,
	{0, TYPE_SHORT, CATEGORY_REAL, 1},
	{   {0, TYPE_SHORT, CATEGORY_REAL, 1},
	    {0, TYPE_SHORT, CATEGORY_REAL, 0}}},
    {2, (CompFuncV)subVSUIUI, _dxfComputeCheckDimAnyShape,
	{0, TYPE_UINT, CATEGORY_REAL, 1},
	{   {0, TYPE_UINT, CATEGORY_REAL, 1},
	    {0, TYPE_UINT, CATEGORY_REAL, 0}}},
    {2, (CompFuncV)subVSII, _dxfComputeCheckDimAnyShape,
	{0, TYPE_INT, CATEGORY_REAL, 1},
	{   {0, TYPE_INT, CATEGORY_REAL, 1},
	    {0, TYPE_INT, CATEGORY_REAL, 0}}},
    {2, (CompFuncV)subVSFF, _dxfComputeCheckDimAnyShape,
	{0, TYPE_FLOAT, CATEGORY_REAL, 1},
	{   {0, TYPE_FLOAT, CATEGORY_REAL, 1},
	    {0, TYPE_FLOAT, CATEGORY_REAL, 0}}},
    {2, (CompFuncV)subVSDD, _dxfComputeCheckDimAnyShape,
	{0, TYPE_DOUBLE, CATEGORY_REAL, 1},
	{   {0, TYPE_DOUBLE, CATEGORY_REAL, 1},
	    {0, TYPE_DOUBLE, CATEGORY_REAL, 0}}},
    {2, (CompFuncV)subVSFFC, _dxfComputeCheckDimAnyShape,
	{0, TYPE_FLOAT, CATEGORY_COMPLEX, 1},
	{   {0, TYPE_FLOAT, CATEGORY_COMPLEX, 1},
	    {0, TYPE_FLOAT, CATEGORY_COMPLEX, 0}}},
    {2, (CompFuncV)subSVUBUB, _dxfComputeCheckDimAnyShape,
	{0, TYPE_UBYTE, CATEGORY_REAL, 1},
	{   {0, TYPE_UBYTE, CATEGORY_REAL, 0},
	    {0, TYPE_UBYTE, CATEGORY_REAL, 1}}},
    {2, (CompFuncV)subSVBB, _dxfComputeCheckDimAnyShape,
	{0, TYPE_BYTE, CATEGORY_REAL, 1},
	{   {0, TYPE_BYTE, CATEGORY_REAL, 0},
	    {0, TYPE_BYTE, CATEGORY_REAL, 1}}},
    {2, (CompFuncV)subSVUSUS, _dxfComputeCheckDimAnyShape,
	{0, TYPE_USHORT, CATEGORY_REAL, 1},
	{   {0, TYPE_USHORT, CATEGORY_REAL, 0},
	    {0, TYPE_USHORT, CATEGORY_REAL, 1}}},
    {2, (CompFuncV)subSVSS, _dxfComputeCheckDimAnyShape,
	{0, TYPE_SHORT, CATEGORY_REAL, 1},
	{   {0, TYPE_SHORT, CATEGORY_REAL, 0},
	    {0, TYPE_SHORT, CATEGORY_REAL, 1}}},
    {2, (CompFuncV)subSVUIUI, _dxfComputeCheckDimAnyShape,
	{0, TYPE_UINT, CATEGORY_REAL, 1},
	{   {0, TYPE_UINT, CATEGORY_REAL, 0},
	    {0, TYPE_UINT, CATEGORY_REAL, 1}}},
    {2, (CompFuncV)subSVII, _dxfComputeCheckDimAnyShape,
	{0, TYPE_INT, CATEGORY_REAL, 1},
	{   {0, TYPE_INT, CATEGORY_REAL, 0},
	    {0, TYPE_INT, CATEGORY_REAL, 1}}},
    {2, (CompFuncV)subSVFF, _dxfComputeCheckDimAnyShape,
	{0, TYPE_FLOAT, CATEGORY_REAL, 1},
	{   {0, TYPE_FLOAT, CATEGORY_REAL, 0},
	    {0, TYPE_FLOAT, CATEGORY_REAL, 1}}},
    {2, (CompFuncV)subSVDD, _dxfComputeCheckDimAnyShape,
	{0, TYPE_DOUBLE, CATEGORY_REAL, 1},
	{   {0, TYPE_DOUBLE, CATEGORY_REAL, 0},
	    {0, TYPE_DOUBLE, CATEGORY_REAL, 1}}},
    {2, (CompFuncV)subSVFFC, _dxfComputeCheckDimAnyShape,
	{0, TYPE_FLOAT, CATEGORY_COMPLEX, 1},
	{   {0, TYPE_FLOAT, CATEGORY_COMPLEX, 0},
	    {0, TYPE_FLOAT, CATEGORY_COMPLEX, 1}}},
    {2, (CompFuncV)subVUBUB, _dxfComputeCheckSameShape,
	{0, TYPE_UBYTE, CATEGORY_REAL, 1},
	{   {0, TYPE_UBYTE, CATEGORY_REAL, 0},
	    {0, TYPE_UBYTE, CATEGORY_REAL, 0}}},
    {2, (CompFuncV)subVBB, _dxfComputeCheckSameShape,
	{0, TYPE_BYTE, CATEGORY_REAL, 1},
	{   {0, TYPE_BYTE, CATEGORY_REAL, 0},
	    {0, TYPE_BYTE, CATEGORY_REAL, 0}}},
    {2, (CompFuncV)subVUSUS, _dxfComputeCheckSameShape,
	{0, TYPE_USHORT, CATEGORY_REAL, 1},
	{   {0, TYPE_USHORT, CATEGORY_REAL, 0},
	    {0, TYPE_USHORT, CATEGORY_REAL, 0}}},
    {2, (CompFuncV)subVSS, _dxfComputeCheckSameShape,
	{0, TYPE_SHORT, CATEGORY_REAL, 1},
	{   {0, TYPE_SHORT, CATEGORY_REAL, 0},
	    {0, TYPE_SHORT, CATEGORY_REAL, 0}}},
    {2, (CompFuncV)subVUIUI, _dxfComputeCheckSameShape,
	{0, TYPE_UINT, CATEGORY_REAL, 1},
	{   {0, TYPE_UINT, CATEGORY_REAL, 0},
	    {0, TYPE_UINT, CATEGORY_REAL, 0}}},
    {2, (CompFuncV)subVII, _dxfComputeCheckSameShape,
	{0, TYPE_INT, CATEGORY_REAL, 1},
	{   {0, TYPE_INT, CATEGORY_REAL, 0},
	    {0, TYPE_INT, CATEGORY_REAL, 0}}},
    {2, (CompFuncV)subVFF, _dxfComputeCheckSameShape,
	{0, TYPE_FLOAT, CATEGORY_REAL, 1},
	{   {0, TYPE_FLOAT, CATEGORY_REAL, 0},
	    {0, TYPE_FLOAT, CATEGORY_REAL, 0}}},
    {2, (CompFuncV)subVDD, _dxfComputeCheckSameShape,
	{0, TYPE_DOUBLE, CATEGORY_REAL, 1},
	{   {0, TYPE_DOUBLE, CATEGORY_REAL, 0},
	    {0, TYPE_DOUBLE, CATEGORY_REAL, 0}}},
    {2, (CompFuncV)subVFFC, _dxfComputeCheckSameShape,
	{0, TYPE_FLOAT, CATEGORY_COMPLEX, 1},
	{   {0, TYPE_FLOAT, CATEGORY_COMPLEX, 0},
	    {0, TYPE_FLOAT, CATEGORY_COMPLEX, 0}}}
};
SIZEINT(_dxdComputeSubs);

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
#ifdef freebsd
#   define trunc(x) ((float)((int)(x)))
#endif
#ifdef cygwin
#   define trunc(x) ((float)((int)(x)))
#endif
#ifdef linux
#   define trunc(x) ((float)((int)(x)))
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


VFUNC1 (floorVF, float, float, ffloor )
VFUNC1 (ceilVF, float, float, fceil )
VFUNC1 (truncVF, float, float, ftrunc )
VFUNC1 (rintVF, float, float, rint )
VFUNC1 (floorVD, double, double, floor )
VFUNC1 (ceilVD, double, double, ceil )
VFUNC1 (truncVD, double, double, trunc )
VFUNC1 (rintVD, double, double, rint )

VFUNC1 (signVF, int, float,  SIGN)
VFUNC1 (signVD, int, double,  SIGN)
VFUNC1 (signVI, int, int,  SIGN)
VFUNC1 (signVS, int, short,  SIGN)
VFUNC1 (signVB, int, signed char,  SIGN)
OperBinding _dxdComputeFloors[] = {
    {1, (CompFuncV)floorVF, _dxfComputeCheckSameShape,
	{0, TYPE_FLOAT, CATEGORY_REAL, 0},
	{   {0, TYPE_FLOAT, CATEGORY_REAL, 0}}},
    {1, (CompFuncV)floorVD, _dxfComputeCheckSameShape,
	{0, TYPE_FLOAT, CATEGORY_REAL, 0},
	{   {0, TYPE_DOUBLE, CATEGORY_REAL, 0}}}
};
OperBinding _dxdComputeCeils[] = {
    {1, (CompFuncV)ceilVF, _dxfComputeCheckSameShape,
	{0, TYPE_FLOAT, CATEGORY_REAL, 0},
	{   {0, TYPE_FLOAT, CATEGORY_REAL, 0}}},
    {1, (CompFuncV)ceilVD, _dxfComputeCheckSameShape,
	{0, TYPE_FLOAT, CATEGORY_REAL, 0},
	{   {0, TYPE_DOUBLE, CATEGORY_REAL, 0}}}
};
OperBinding _dxdComputeTruncs[] = {
    {1, (CompFuncV)truncVF, _dxfComputeCheckSameShape,
	{0, TYPE_FLOAT, CATEGORY_REAL, 0},
	{   {0, TYPE_FLOAT, CATEGORY_REAL, 0}}},
    {1, (CompFuncV)truncVD, _dxfComputeCheckSameShape,
	{0, TYPE_FLOAT, CATEGORY_REAL, 0},
	{   {0, TYPE_DOUBLE, CATEGORY_REAL, 0}}}
};
OperBinding _dxdComputeRints[] = {
    {1, (CompFuncV)rintVF, _dxfComputeCheckSameShape,
	{0, TYPE_FLOAT, CATEGORY_REAL, 0},
	{   {0, TYPE_FLOAT, CATEGORY_REAL, 0}}},
    {1, (CompFuncV)rintVD, _dxfComputeCheckSameShape,
	{0, TYPE_FLOAT, CATEGORY_REAL, 0},
	{   {0, TYPE_DOUBLE, CATEGORY_REAL, 0}}}
};
OperBinding _dxdComputeSigns[] = {
    {1, (CompFuncV)signVF, _dxfComputeCheckSameShape,
	{0, TYPE_INT, CATEGORY_REAL, 0},
	{   {0, TYPE_FLOAT, CATEGORY_REAL, 0}}},
    {1, (CompFuncV)signVD, _dxfComputeCheckSameShape,
	{0, TYPE_INT, CATEGORY_REAL, 0},
	{   {0, TYPE_DOUBLE, CATEGORY_REAL, 0}}},
    {1, (CompFuncV)signVI, _dxfComputeCheckSameShape,
	{0, TYPE_INT, CATEGORY_REAL, 0},
	{   {0, TYPE_INT, CATEGORY_REAL, 0}}},
    {1, (CompFuncV)signVS, _dxfComputeCheckSameShape,
	{0, TYPE_INT, CATEGORY_REAL, 0},
	{   {0, TYPE_SHORT, CATEGORY_REAL, 0}}},
    {1, (CompFuncV)signVB, _dxfComputeCheckSameShape,
	{0, TYPE_INT, CATEGORY_REAL, 0},
	{   {0, TYPE_BYTE, CATEGORY_REAL, 0}}}
};
SIZEINT(_dxdComputeFloors);
SIZEINT(_dxdComputeCeils);
SIZEINT(_dxdComputeTruncs);
SIZEINT(_dxdComputeRints);
SIZEINT(_dxdComputeSigns);


VMULTIOP(minFs, float, float, MIN, DXD_MAX_FLOAT)
VMULTIOP(minDs, double, double, MIN, DXD_MAX_DOUBLE)
VMULTIOP(minUIs, unsigned int, unsigned int, MIN, 0xffffffff)
VMULTIOP(minIs, int, int, MIN, 0x7fffffff)
VMULTIOP(minUBs, unsigned char, unsigned char, MIN, 0xff)
VMULTIOP(minBs, signed char, signed char, MIN, 0x7f)
VMULTIOP(minUSs, unsigned short, unsigned short, MIN, 0xffff)
VMULTIOP(minSs, signed short, signed short, MIN, 0x7fff)
OperBinding _dxdComputeMins[] = {
    {-1, (CompFuncV)minUBs, _dxfComputeCheckSameAs,
	{0, TYPE_UBYTE, CATEGORY_REAL, 0},
	{   {0, TYPE_UBYTE, CATEGORY_REAL, 0}}},
    {-1, (CompFuncV)minBs, _dxfComputeCheckSameAs,
	{0, TYPE_BYTE, CATEGORY_REAL, 0},
	{   {0, TYPE_BYTE, CATEGORY_REAL, 0}}},
    {-1, (CompFuncV)minUSs, _dxfComputeCheckSameAs,
	{0, TYPE_USHORT, CATEGORY_REAL, 0},
	{   {0, TYPE_USHORT, CATEGORY_REAL, 0}}},
    {-1, (CompFuncV)minSs, _dxfComputeCheckSameAs,
	{0, TYPE_SHORT, CATEGORY_REAL, 0},
	{   {0, TYPE_SHORT, CATEGORY_REAL, 0}}},
    {-1, (CompFuncV)minUIs, _dxfComputeCheckSameAs,
	{0, TYPE_UINT, CATEGORY_REAL, 0},
	{   {0, TYPE_UINT, CATEGORY_REAL, 0}}},
    {-1, (CompFuncV)minIs, _dxfComputeCheckSameAs,
	{0, TYPE_INT, CATEGORY_REAL, 0},
	{   {0, TYPE_INT, CATEGORY_REAL, 0}}},
    {-1, (CompFuncV)minDs, _dxfComputeCheckSameAs,
	{0, TYPE_DOUBLE, CATEGORY_REAL, 0},
	{   {0, TYPE_DOUBLE, CATEGORY_REAL, 0}}},
    {-1, (CompFuncV)minFs, _dxfComputeCheckSameAs,
	{0, TYPE_FLOAT, CATEGORY_REAL, 0},
	{   {0, TYPE_FLOAT, CATEGORY_REAL, 0}}}
};
SIZEINT(_dxdComputeMins);

VMULTIOP(maxFs, float, float, MAX, -DXD_MAX_FLOAT)
VMULTIOP(maxDs, double, double, MAX, -DXD_MAX_DOUBLE)
VMULTIOP(maxIs, int, int, MAX, DXD_MIN_INT)
VMULTIOP(maxSs, short, short, MAX, DXD_MIN_SHORT)
VMULTIOP(maxBs, signed char, signed char, MAX, DXD_MIN_BYTE)
VMULTIOP(maxUIs, unsigned int, unsigned int, MAX, 0)
VMULTIOP(maxUSs, unsigned short, unsigned short, MAX, 0)
VMULTIOP(maxUBs, unsigned char, unsigned char, MAX, 0)

OperBinding _dxdComputeMaxs[] = {
    {-1, (CompFuncV)maxBs, _dxfComputeCheckSameAs,
	{0, TYPE_BYTE, CATEGORY_REAL, 0},
	{   {0, TYPE_BYTE, CATEGORY_REAL, 0}}},
    {-1, (CompFuncV)maxSs, _dxfComputeCheckSameAs,
	{0, TYPE_SHORT, CATEGORY_REAL, 0},
	{   {0, TYPE_SHORT, CATEGORY_REAL, 0}}},
    {-1, (CompFuncV)maxIs, _dxfComputeCheckSameAs,
	{0, TYPE_INT, CATEGORY_REAL, 0},
	{   {0, TYPE_INT, CATEGORY_REAL, 0}}},
    {-1, (CompFuncV)maxUBs, _dxfComputeCheckSameAs,
	{0, TYPE_UBYTE, CATEGORY_REAL, 0},
	{   {0, TYPE_UBYTE, CATEGORY_REAL, 0}}},
    {-1, (CompFuncV)maxUSs, _dxfComputeCheckSameAs,
	{0, TYPE_USHORT, CATEGORY_REAL, 0},
	{   {0, TYPE_USHORT, CATEGORY_REAL, 0}}},
    {-1, (CompFuncV)maxUIs, _dxfComputeCheckSameAs,
	{0, TYPE_UINT, CATEGORY_REAL, 0},
	{   {0, TYPE_UINT, CATEGORY_REAL, 0}}},
    {-1, (CompFuncV)maxDs, _dxfComputeCheckSameAs,
	{0, TYPE_DOUBLE, CATEGORY_REAL, 0},
	{   {0, TYPE_DOUBLE, CATEGORY_REAL, 0}}},
    {-1, (CompFuncV)maxFs, _dxfComputeCheckSameAs,
	{0, TYPE_FLOAT, CATEGORY_REAL, 0},
	{   {0, TYPE_FLOAT, CATEGORY_REAL, 0}}}
};
SIZEINT(_dxdComputeMaxs);


