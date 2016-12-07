/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


/* This file contains the description of each operator from a type and 
 * execution pint of view.  For each operator (such as COND (?:), *, 
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

#if defined(HAVE_STRINGS_H)
#include <strings.h>
#endif

#include <stdio.h>
#include <math.h>
#include <dx/dx.h>
#include "_compute.h"
#include "_compputils.h"
#include "_compoper.h"

#define SIZEINT(x) int x##Size = (sizeof (x) / sizeof(x[0]))

BINOP (ltUBUB, int, unsigned char, unsigned char, <)
BINOP (ltBB, int, signed char, signed char, <)
BINOP (ltUSUS, int, unsigned short, unsigned short, <)
BINOP (ltSS, int, signed short, signed short, <)
BINOP (ltUIUI, int, unsigned int, unsigned int, <)
BINOP (ltII, int, signed int, signed int, <)
BINOP (ltFF, int, float, float, <)
BINOP (ltDD, int, double, double, <)

BINOP (leUBUB, int, unsigned char, unsigned char, <=)
BINOP (leBB, int, signed char, signed char, <=)
BINOP (leUSUS, int, unsigned short, unsigned short, <=)
BINOP (leSS, int, signed short, signed short, <=)
BINOP (leUIUI, int, unsigned int, unsigned int, <=)
BINOP (leII, int, signed int, signed int, <=)
BINOP (leFF, int, float, float, <=)
BINOP (leDD, int, double, double, <=)


BINOP (gtUBUB, int, unsigned char, unsigned char, >)
BINOP (gtBB, int, signed char, signed char, >)
BINOP (gtUSUS, int, unsigned short, unsigned short, >)
BINOP (gtSS, int, signed short, signed short, >)
BINOP (gtUIUI, int, unsigned int, unsigned int, >)
BINOP (gtII, int, signed int, signed int, >)
BINOP (gtFF, int, float, float, >)
BINOP (gtDD, int, double, double, >)

BINOP (geUBUB, int, unsigned char, unsigned char, >=)
BINOP (geBB, int, signed char, signed char, >=)
BINOP (geUSUS, int, unsigned short, unsigned short, >=)
BINOP (geSS, int, signed short, signed short, >=)
BINOP (geUIUI, int, unsigned int, unsigned int, >=)
BINOP (geII, int, signed int, signed int, >=)
BINOP (geFF, int, float, float, >=)
BINOP (geDD, int, double, double, >=)

OperBinding _dxdComputeLts[] = {
    {2, (CompFuncV)ltUBUB, _dxfComputeCheckMatch,
	{0, TYPE_INT, CATEGORY_REAL, 0},
	{   {0, TYPE_UBYTE, CATEGORY_REAL, 0},
	    {0, TYPE_UBYTE, CATEGORY_REAL, 0}}},
    {2, (CompFuncV)ltBB, _dxfComputeCheckMatch,
	{0, TYPE_INT, CATEGORY_REAL, 0},
	{   {0, TYPE_BYTE, CATEGORY_REAL, 0},
	    {0, TYPE_BYTE, CATEGORY_REAL, 0}}},
    {2, (CompFuncV)ltUSUS, _dxfComputeCheckMatch,
	{0, TYPE_INT, CATEGORY_REAL, 0},
	{   {0, TYPE_USHORT, CATEGORY_REAL, 0},
	    {0, TYPE_USHORT, CATEGORY_REAL, 0}}},
    {2, (CompFuncV)ltSS, _dxfComputeCheckMatch,
	{0, TYPE_INT, CATEGORY_REAL, 0},
	{   {0, TYPE_SHORT, CATEGORY_REAL, 0},
	    {0, TYPE_SHORT, CATEGORY_REAL, 0}}},
    {2, (CompFuncV)ltUIUI, _dxfComputeCheckMatch,
	{0, TYPE_INT, CATEGORY_REAL, 0},
	{   {0, TYPE_UINT, CATEGORY_REAL, 0},
	    {0, TYPE_UINT, CATEGORY_REAL, 0}}},
    {2, (CompFuncV)ltII, _dxfComputeCheckMatch,
	{0, TYPE_INT, CATEGORY_REAL, 0},
	{   {0, TYPE_INT, CATEGORY_REAL, 0},
	    {0, TYPE_INT, CATEGORY_REAL, 0}}},
    {2, (CompFuncV)ltFF, _dxfComputeCheckMatch,
	{0, TYPE_INT, CATEGORY_REAL, 0},
	{   {0, TYPE_FLOAT, CATEGORY_REAL, 0},
	    {0, TYPE_FLOAT, CATEGORY_REAL, 0}}},
    {2, (CompFuncV)ltDD, _dxfComputeCheckMatch,
	{0, TYPE_INT, CATEGORY_REAL, 0},
	{   {0, TYPE_DOUBLE, CATEGORY_REAL, 0},
	    {0, TYPE_DOUBLE, CATEGORY_REAL, 0}}}
};
SIZEINT(_dxdComputeLts);

OperBinding _dxdComputeLes[] = {
    {2, (CompFuncV)leUBUB, _dxfComputeCheckMatch,
	{0, TYPE_INT, CATEGORY_REAL, 0},
	{   {0, TYPE_UBYTE, CATEGORY_REAL, 0},
	    {0, TYPE_UBYTE, CATEGORY_REAL, 0}}},
    {2, (CompFuncV)leBB, _dxfComputeCheckMatch,
	{0, TYPE_INT, CATEGORY_REAL, 0},
	{   {0, TYPE_BYTE, CATEGORY_REAL, 0},
	    {0, TYPE_BYTE, CATEGORY_REAL, 0}}},
    {2, (CompFuncV)leUSUS, _dxfComputeCheckMatch,
	{0, TYPE_INT, CATEGORY_REAL, 0},
	{   {0, TYPE_USHORT, CATEGORY_REAL, 0},
	    {0, TYPE_USHORT, CATEGORY_REAL, 0}}},
    {2, (CompFuncV)leSS, _dxfComputeCheckMatch,
	{0, TYPE_INT, CATEGORY_REAL, 0},
	{   {0, TYPE_SHORT, CATEGORY_REAL, 0},
	    {0, TYPE_SHORT, CATEGORY_REAL, 0}}},
    {2, (CompFuncV)leUIUI, _dxfComputeCheckMatch,
	{0, TYPE_INT, CATEGORY_REAL, 0},
	{   {0, TYPE_UINT, CATEGORY_REAL, 0},
	    {0, TYPE_UINT, CATEGORY_REAL, 0}}},
    {2, (CompFuncV)leII, _dxfComputeCheckMatch,
	{0, TYPE_INT, CATEGORY_REAL, 0},
	{   {0, TYPE_INT, CATEGORY_REAL, 0},
	    {0, TYPE_INT, CATEGORY_REAL, 0}}},
    {2, (CompFuncV)leFF, _dxfComputeCheckMatch,
	{0, TYPE_INT, CATEGORY_REAL, 0},
	{   {0, TYPE_FLOAT, CATEGORY_REAL, 0},
	    {0, TYPE_FLOAT, CATEGORY_REAL, 0}}},
    {2, (CompFuncV)leDD, _dxfComputeCheckMatch,
	{0, TYPE_INT, CATEGORY_REAL, 0},
	{   {0, TYPE_DOUBLE, CATEGORY_REAL, 0},
	    {0, TYPE_DOUBLE, CATEGORY_REAL, 0}}}
};
SIZEINT(_dxdComputeLes);

OperBinding _dxdComputeGts[] = {
    {2, (CompFuncV)gtUBUB, _dxfComputeCheckMatch,
	{0, TYPE_INT, CATEGORY_REAL, 0},
	{   {0, TYPE_UBYTE, CATEGORY_REAL, 0},
	    {0, TYPE_UBYTE, CATEGORY_REAL, 0}}},
    {2, (CompFuncV)gtBB, _dxfComputeCheckMatch,
	{0, TYPE_INT, CATEGORY_REAL, 0},
	{   {0, TYPE_BYTE, CATEGORY_REAL, 0},
	    {0, TYPE_BYTE, CATEGORY_REAL, 0}}},
    {2, (CompFuncV)gtUSUS, _dxfComputeCheckMatch,
	{0, TYPE_INT, CATEGORY_REAL, 0},
	{   {0, TYPE_USHORT, CATEGORY_REAL, 0},
	    {0, TYPE_USHORT, CATEGORY_REAL, 0}}},
    {2, (CompFuncV)gtSS, _dxfComputeCheckMatch,
	{0, TYPE_INT, CATEGORY_REAL, 0},
	{   {0, TYPE_SHORT, CATEGORY_REAL, 0},
	    {0, TYPE_SHORT, CATEGORY_REAL, 0}}},
    {2, (CompFuncV)gtUIUI, _dxfComputeCheckMatch,
	{0, TYPE_INT, CATEGORY_REAL, 0},
	{   {0, TYPE_UINT, CATEGORY_REAL, 0},
	    {0, TYPE_UINT, CATEGORY_REAL, 0}}},
    {2, (CompFuncV)gtII, _dxfComputeCheckMatch,
	{0, TYPE_INT, CATEGORY_REAL, 0},
	{   {0, TYPE_INT, CATEGORY_REAL, 0},
	    {0, TYPE_INT, CATEGORY_REAL, 0}}},
    {2, (CompFuncV)gtFF, _dxfComputeCheckMatch,
	{0, TYPE_INT, CATEGORY_REAL, 0},
	{   {0, TYPE_FLOAT, CATEGORY_REAL, 0},
	    {0, TYPE_FLOAT, CATEGORY_REAL, 0}}},
    {2, (CompFuncV)gtDD, _dxfComputeCheckMatch,
	{0, TYPE_INT, CATEGORY_REAL, 0},
	{   {0, TYPE_DOUBLE, CATEGORY_REAL, 0},
	    {0, TYPE_DOUBLE, CATEGORY_REAL, 0}}}
};

SIZEINT(_dxdComputeGts);

OperBinding _dxdComputeGes[] = {
    {2, (CompFuncV)geUBUB, _dxfComputeCheckMatch,
	{0, TYPE_INT, CATEGORY_REAL, 0},
	{   {0, TYPE_UBYTE, CATEGORY_REAL, 0},
	    {0, TYPE_UBYTE, CATEGORY_REAL, 0}}},
    {2, (CompFuncV)geBB, _dxfComputeCheckMatch,
	{0, TYPE_INT, CATEGORY_REAL, 0},
	{   {0, TYPE_BYTE, CATEGORY_REAL, 0},
	    {0, TYPE_BYTE, CATEGORY_REAL, 0}}},
    {2, (CompFuncV)geUSUS, _dxfComputeCheckMatch,
	{0, TYPE_INT, CATEGORY_REAL, 0},
	{   {0, TYPE_USHORT, CATEGORY_REAL, 0},
	    {0, TYPE_USHORT, CATEGORY_REAL, 0}}},
    {2, (CompFuncV)geSS, _dxfComputeCheckMatch,
	{0, TYPE_INT, CATEGORY_REAL, 0},
	{   {0, TYPE_SHORT, CATEGORY_REAL, 0},
	    {0, TYPE_SHORT, CATEGORY_REAL, 0}}},
    {2, (CompFuncV)geUIUI, _dxfComputeCheckMatch,
	{0, TYPE_INT, CATEGORY_REAL, 0},
	{   {0, TYPE_UINT, CATEGORY_REAL, 0},
	    {0, TYPE_UINT, CATEGORY_REAL, 0}}},
    {2, (CompFuncV)geII, _dxfComputeCheckMatch,
	{0, TYPE_INT, CATEGORY_REAL, 0},
	{   {0, TYPE_INT, CATEGORY_REAL, 0},
	    {0, TYPE_INT, CATEGORY_REAL, 0}}},
    {2, (CompFuncV)geFF, _dxfComputeCheckMatch,
	{0, TYPE_INT, CATEGORY_REAL, 0},
	{   {0, TYPE_FLOAT, CATEGORY_REAL, 0},
	    {0, TYPE_FLOAT, CATEGORY_REAL, 0}}},
    {2, (CompFuncV)geDD, _dxfComputeCheckMatch,
	{0, TYPE_INT, CATEGORY_REAL, 0},
	{   {0, TYPE_DOUBLE, CATEGORY_REAL, 0},
	    {0, TYPE_DOUBLE, CATEGORY_REAL, 0}}}
};
SIZEINT(_dxdComputeGes);

static int
eq(
    PTreeNode *pt, 
    ObjStruct *os, 
    int numInputs,
    Pointer *inputs,
    Pointer result,
    InvalidComponentHandle *invalids,
    InvalidComponentHandle outInvalid)
{
    register int *out = (int *) result;

    int size0 = pt->args->metaType.items;
    int size1 = pt->args->next->metaType.items;
    int i, j, k;
    int numBasic;
    int size;
    int items;
    int allValid =
	(invalids[0] == NULL || DXGetInvalidCount(invalids[0]) == 0) &&
	(invalids[1] == NULL || DXGetInvalidCount(invalids[1]) == 0);

    for (numBasic = 1, i = 0; i < pt->args->metaType.rank; ++i) {
	numBasic *= pt->args->metaType.shape[i];
    }
    size = DXTypeSize (pt->args->metaType.type) *
	   DXCategorySize (pt->args->metaType.category) *
	   numBasic;

    items = pt->metaType.items;
    for (i = j = k = 0;
	    i < items;
	    ++i) {
	if (allValid || _dxfComputeInvalidOr(outInvalid, i, invalids[0], j, invalids[1], k))
	    out[i] = !bcmp (((char *)inputs[0]) + j * size, 
			    ((char *)inputs[1]) + k * size, size);
	++j, ++k;
	if (j >= size0) j = 0;
	if (k >= size1) k = 0;
    }
    return (OK);
}
static int
ne(
    PTreeNode *pt, 
    ObjStruct *os, 
    int numInputs,
    Pointer *inputs,
    Pointer result,
    InvalidComponentHandle *invalids,
    InvalidComponentHandle outInvalid)
{
    register int *out = (int *) result;

    int size0 = pt->args->metaType.items;
    int size1 = pt->args->next->metaType.items;
    int i, j, k;
    int numBasic;
    int size;
    int items;
    int allValid =
	(invalids[0] == NULL || DXGetInvalidCount(invalids[0]) == 0) &&
	(invalids[1] == NULL || DXGetInvalidCount(invalids[1]) == 0);

    for (numBasic = 1, i = 0; i < pt->args->metaType.rank; ++i) {
	numBasic *= pt->args->metaType.shape[i];
    }
    size = DXTypeSize (pt->args->metaType.type) *
	   DXCategorySize (pt->args->metaType.category) *
	   numBasic;

    items = pt->metaType.items;
    for (i = j = k = 0;
	    i < items;
	    ++i) {
	if (allValid || _dxfComputeInvalidOr(outInvalid, i, invalids[0], j, invalids[1], k)) 
	    out[i] = (0 != bcmp (((char *)inputs[0]) + j * size, 
			    ((char *)inputs[1]) + k * size, size));
	++j, ++k;
	if (j >= size0) j = 0;
	if (k >= size1) k = 0;
    }
    return (OK);
}

VCMP(eqF, int, float, float, ==, &&)
VCMP(neF, int, float, float, !=, ||)
VCMP(eqD, int, double, double, ==, &&)
VCMP(neD, int, double, double, !=, ||)

OperBinding _dxdComputeEqs[] = {
    {2, (CompFuncV)eqF, _dxfComputeCheckSameTypeSpecReturn,
	{0, TYPE_INT, CATEGORY_REAL, 0},
	{{0, TYPE_FLOAT}}},
    {2, (CompFuncV)eqD, _dxfComputeCheckSameTypeSpecReturn,
	{0, TYPE_INT, CATEGORY_REAL, 0},
	{{0, TYPE_DOUBLE}}},
    {2, (CompFuncV)eq, _dxfComputeCheckSameSpecReturn,
	{0, TYPE_INT, CATEGORY_REAL, 0}}
};
SIZEINT(_dxdComputeEqs);
OperBinding _dxdComputeNes[] = {
    {2, (CompFuncV)neF, _dxfComputeCheckSameTypeSpecReturn,
	{0, TYPE_INT, CATEGORY_REAL, 0},
	{{0, TYPE_FLOAT}}},
    {2, (CompFuncV)neD, _dxfComputeCheckSameTypeSpecReturn,
	{0, TYPE_INT, CATEGORY_REAL, 0},
	{{0, TYPE_DOUBLE}}},
    {2, (CompFuncV)ne, _dxfComputeCheckSameSpecReturn,
	{0, TYPE_INT, CATEGORY_REAL, 0}}
};
SIZEINT(_dxdComputeNes);

BINOP (and, int, int, int, &&)
BINOP (or, int, int, int, ||)
UNOP (not, int, int, !)
OperBinding _dxdComputeAnds[] = {
    {2, (CompFuncV)and, _dxfComputeCheckMatch,
	{0, TYPE_INT, CATEGORY_REAL, 0},
	{   {0, TYPE_INT, CATEGORY_REAL, 0},
	    {0, TYPE_INT, CATEGORY_REAL, 0}}}
};
OperBinding _dxdComputeOrs[] = {
    {2, (CompFuncV)or, _dxfComputeCheckMatch,
	{0, TYPE_INT, CATEGORY_REAL, 0},
	{   {0, TYPE_INT, CATEGORY_REAL, 0},
	    {0, TYPE_INT, CATEGORY_REAL, 0}}}
};
OperBinding _dxdComputeNots[] = {
    {1, (CompFuncV)not, _dxfComputeCheckMatch,
	{0, TYPE_INT, CATEGORY_REAL, 0},
	{   {0, TYPE_INT, CATEGORY_REAL, 0}}}
};
SIZEINT(_dxdComputeAnds);
SIZEINT(_dxdComputeOrs);
SIZEINT(_dxdComputeNots);



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
#define SIGN(x) ((x) >= 0? (1): (-1))


/*
 * A set of casting routines follow.  Casts from larger numbers to smaller
 * occur first with checks to prevent under/overflow, followed by a set
 * of routines that just cast the values for converting from smaller to larger
 * types.
 */
#define VALID_SBYTE(x)  ((x) < 128 && (x) >= -128)
#define VALID_SHORT(x)  ((x) < 32768 && (x) >= -32768)
#define VALID_INT(x)    ((x) < 2147483648.0 && (x) >= -2147483648.0)
#define VALID_SBYTEU(x) ((x) < 128)
#define VALID_SHORTU(x) ((x) < 32768)
#define VALID_INTU(x)   ((x) < 2147483648.0)
#define VALID_UBYTE(x)  ((x) < 256 && (x) >= 0)
#define VALID_USHORT(x) ((x) < 65536 && (x) >= 0)
#define VALID_UINT(x)   ((x) < 4294967296.0 && (x) >= 0.0)
#define VALID_FLOAT(x)  ((x) < DXD_MAX_FLOAT && (x) > -DXD_MAX_FLOAT)

/*  Turn off unsigned being compared to zero warnings  */
#ifdef sgi
#  pragma set woff 1183
#endif

VFUNC1RC(ubyteCastBV, unsigned char, signed char, (unsigned char),
    VALID_UBYTE, "#11999")
VFUNC1RC(ubyteCastUSV, unsigned char, unsigned short, (unsigned char),
    VALID_UBYTE, "#12000")
VFUNC1RC(ubyteCastSV, unsigned char, signed short, (unsigned char),
    VALID_UBYTE, "#11999")
VFUNC1RC(ubyteCastUIV, unsigned char, unsigned int, (unsigned char),
    VALID_UBYTE, "#12000")
VFUNC1RC(ubyteCastIV, unsigned char, signed int, (unsigned char),
    VALID_UBYTE, "#11999")
VFUNC1RC(ubyteCastFV, unsigned char, float, (unsigned char),
    VALID_UBYTE, "#12001")
VFUNC1RC(ubyteCastDV, unsigned char, double, (unsigned char),
    VALID_UBYTE, "#12001")

VFUNC1RC(sbyteCastUBV, signed char, unsigned char, (signed char),
    VALID_SBYTEU, "#11991")
VFUNC1RC(sbyteCastUSV, signed char, unsigned short, (signed char),
    VALID_SBYTEU, "#11991")
VFUNC1RC(sbyteCastSV, signed char, signed short, (signed char),
    VALID_SBYTE, "#11990")
VFUNC1RC(sbyteCastUIV, signed char, unsigned int, (signed char),
    VALID_SBYTEU, "#11991")
VFUNC1RC(sbyteCastIV, signed char, signed int, (signed char),
    VALID_SBYTE, "#11990")
VFUNC1RC(sbyteCastFV, signed char, float, (signed char),
    VALID_SBYTE, "#11992")
VFUNC1RC(sbyteCastDV, signed char, double, (signed char),
    VALID_SBYTE, "#11992")

VCATFUNC1 (ushortCastUBV, unsigned short, unsigned char, (unsigned short) )
VFUNC1RC(ushortCastBV, unsigned short, signed char, (unsigned short),
    VALID_USHORT, "#12002")
VFUNC1RC(ushortCastSV, unsigned short, signed short, (unsigned short),
    VALID_USHORT, "#12002")
VFUNC1RC(ushortCastUIV, unsigned short, unsigned int, (unsigned short),
    VALID_USHORT, "#12003")
VFUNC1RC(ushortCastIV, unsigned short, signed int, (unsigned short),
    VALID_USHORT, "#12002")
VFUNC1RC(ushortCastFV, unsigned short, float, (unsigned short),
    VALID_USHORT, "#12004")
VFUNC1RC(ushortCastDV, unsigned short, float, (unsigned short),
    VALID_USHORT, "#12004")

VCATFUNC1 (shortCastUBV, short, unsigned char, (short int) )
VCATFUNC1 (shortCastBV, short, signed char, (short int) )
VFUNC1RC(shortCastUSV, short, unsigned short, (short),
    VALID_SHORTU, "#11994")
VFUNC1RC(shortCastUIV, short, unsigned int, (short),
    VALID_SHORTU, "#11994")
VFUNC1RC(shortCastIV, short, signed int, (short),
    VALID_SHORT, "#11993")
VFUNC1RC(shortCastFV, short, float, (short),
    VALID_SHORT, "#11995")
VFUNC1RC(shortCastDV, short, float, (short),
    VALID_SHORT, "#11995")

VCATFUNC1 (uintCastUBV, unsigned int, unsigned char, (unsigned int) )
VFUNC1RC(uintCastBV, unsigned int, signed char, (unsigned int),
    VALID_UINT, "#12006")
VCATFUNC1 (uintCastUSV, unsigned int, unsigned short, (unsigned int) )
VFUNC1RC(uintCastSV, unsigned int, signed short, (unsigned int),
    VALID_UINT, "#12006")
VFUNC1RC(uintCastIV, unsigned int, signed int, (unsigned int),
    VALID_UINT, "#12006")
VFUNC1RC(uintCastFV, unsigned int, float, (unsigned int),
    VALID_UINT, "#12007")
VFUNC1RC(uintCastDV, unsigned int, double, (unsigned int),
    VALID_UINT, "#12007")

VCATFUNC1 (intCastUBV, int, unsigned char, (int) )
VCATFUNC1 (intCastBV, int, signed char, (int) )
VCATFUNC1 (intCastUSV, int, unsigned short, (int) )
VCATFUNC1 (intCastSV, int, signed short, (int) )
VFUNC1RC (intCastUIV, signed int, unsigned int, (signed int),
    VALID_INTU, "#11997")
VFUNC1RC (intCastFV, int, float, (int),
    VALID_INT, "#11998")
VFUNC1RC (intCastDV, int, double, (int),
    VALID_INT, "#11998")

VCATFUNC1 (floatCastUBV, float, unsigned char, (float) )
VCATFUNC1 (floatCastBV, float, signed char, (float) )
VCATFUNC1 (floatCastSV, float, short, (float) )
VCATFUNC1 (floatCastUSV, float, unsigned short, (float) )
VCATFUNC1 (floatCastIV, float, int, (float) )
VCATFUNC1 (floatCastUIV, float, unsigned int, (float) )
VFUNC1RC(floatCastDV, float, double, (float),
    VALID_FLOAT, "#12008")

VCATFUNC1 (doubleCastUBV, double, unsigned char, (double) )
VCATFUNC1 (doubleCastBV, double, signed char, (double) )
VCATFUNC1 (doubleCastSV, double, short, (double) )
VCATFUNC1 (doubleCastUSV, double, unsigned short, (double) )
VCATFUNC1 (doubleCastIV, double, int, (double) )
VCATFUNC1 (doubleCastUIV, double, unsigned int, (double) )
VCATFUNC1 (doubleCastFV, double, float, (double) )

/*  Turn off unsigned being compared to zero warnings  */
#ifdef sgi
#  pragma reset woff 1183
#endif

#define SignedCharCast(x) (((x) & 0x80)? ((unsigned int)(x)) - 256: (x))
VCATFUNC1 (signed_intCastCV, int, unsigned char, SignedCharCast)

OperBinding _dxdComputeInts[] = {
    {1, (CompFuncV)intCastUBV, _dxfComputeCheckSameShape,
	{0, TYPE_INT, (Category)-1, 1},
	{   {0, TYPE_UBYTE, (Category)-1, 0}}},
    {1, (CompFuncV)intCastBV, _dxfComputeCheckSameShape,
	{0, TYPE_INT, (Category)-1, 1},
	{   {0, TYPE_BYTE, (Category)-1, 0}}},
    {1, (CompFuncV)intCastUSV, _dxfComputeCheckSameShape,
	{0, TYPE_INT, (Category)-1, 1},
	{   {0, TYPE_USHORT, (Category)-1, 0}}},
    {1, (CompFuncV)intCastSV, _dxfComputeCheckSameShape,
	{0, TYPE_INT, (Category)-1, 1},
	{   {0, TYPE_SHORT, (Category)-1, 0}}},
    {1, (CompFuncV)_dxfComputeCopy, _dxfComputeCheckSameShape,
	{0, TYPE_INT, (Category)-1, 1},
	{   {0, TYPE_INT, (Category)-1, 0}}},
    {1, (CompFuncV)intCastUIV, _dxfComputeCheckSameShape,
	{0, TYPE_INT, (Category)-1, 1},
	{   {0, TYPE_UINT, (Category)-1, 0}}},
    {1, (CompFuncV)intCastFV, _dxfComputeCheckSameShape,
	{0, TYPE_INT, (Category)-1, 1},
	{   {0, TYPE_FLOAT, (Category)-1, 0}}},
    {1, (CompFuncV)intCastDV, _dxfComputeCheckSameShape,
	{0, TYPE_INT, (Category)-1, 1},
	{   {0, TYPE_DOUBLE, (Category)-1, 0}}}
};
SIZEINT(_dxdComputeInts);

OperBinding _dxdComputeUints[] = {
    {1, (CompFuncV)uintCastUBV, _dxfComputeCheckSameShape,
	{0, TYPE_UINT, (Category)-1, 1},
	{   {0, TYPE_UBYTE, (Category)-1, 0}}},
    {1, (CompFuncV)uintCastBV, _dxfComputeCheckSameShape,
	{0, TYPE_UINT, (Category)-1, 1},
	{   {0, TYPE_BYTE, (Category)-1, 0}}},
    {1, (CompFuncV)uintCastUSV, _dxfComputeCheckSameShape,
	{0, TYPE_UINT, (Category)-1, 1},
	{   {0, TYPE_USHORT, (Category)-1, 0}}},
    {1, (CompFuncV)uintCastSV, _dxfComputeCheckSameShape,
	{0, TYPE_UINT, (Category)-1, 1},
	{   {0, TYPE_SHORT, (Category)-1, 0}}},
    {1, (CompFuncV)uintCastIV, _dxfComputeCheckSameShape,
	{0, TYPE_UINT, (Category)-1, 1},
	{   {0, TYPE_INT, (Category)-1, 0}}},
    {1, (CompFuncV)_dxfComputeCopy, _dxfComputeCheckSameShape,
	{0, TYPE_UINT, (Category)-1, 1},
	{   {0, TYPE_UINT, (Category)-1, 0}}},
    {1, (CompFuncV)uintCastFV, _dxfComputeCheckSameShape,
	{0, TYPE_UINT, (Category)-1, 1},
	{   {0, TYPE_FLOAT, (Category)-1, 0}}},
    {1, (CompFuncV)uintCastDV, _dxfComputeCheckSameShape,
	{0, TYPE_UINT, (Category)-1, 1},
	{   {0, TYPE_DOUBLE, (Category)-1, 0}}}
};
SIZEINT(_dxdComputeUints);

OperBinding _dxdComputeSigned_ints[] = {
    {1, (CompFuncV)signed_intCastCV, _dxfComputeCheckSameShape,
	{0, TYPE_INT, (Category)-1, 1},
	{   {0, TYPE_UBYTE, (Category)-1, 0}}},
    {1, (CompFuncV)intCastSV, _dxfComputeCheckSameShape,
	{0, TYPE_INT, (Category)-1, 1},
	{   {0, TYPE_SHORT, (Category)-1, 0}}},
    {1, (CompFuncV)_dxfComputeCopy, _dxfComputeCheckSameShape,
	{0, TYPE_INT, (Category)-1, 1},
	{   {0, TYPE_INT, (Category)-1, 0}}},
    {1, (CompFuncV)intCastFV, _dxfComputeCheckSameShape,
	{0, TYPE_INT, (Category)-1, 1},
	{   {0, TYPE_FLOAT, (Category)-1, 0}}},
    {1, (CompFuncV)intCastDV, _dxfComputeCheckSameShape,
	{0, TYPE_INT, (Category)-1, 1},
	{   {0, TYPE_DOUBLE, (Category)-1, 0}}}
};
SIZEINT(_dxdComputeSigned_ints);

OperBinding _dxdComputeFloats[] = {
    {1, (CompFuncV)floatCastUBV, _dxfComputeCheckSameShape,
	{0, TYPE_FLOAT, (Category)-1, 1},
	{   {0, TYPE_UBYTE, (Category)-1, 0}}},
    {1, (CompFuncV)floatCastBV, _dxfComputeCheckSameShape,
	{0, TYPE_FLOAT, (Category)-1, 1},
	{   {0, TYPE_BYTE, (Category)-1, 0}}},
    {1, (CompFuncV)floatCastUSV, _dxfComputeCheckSameShape,
	{0, TYPE_FLOAT, (Category)-1, 1},
	{   {0, TYPE_USHORT, (Category)-1, 0}}},
    {1, (CompFuncV)floatCastSV, _dxfComputeCheckSameShape,
	{0, TYPE_FLOAT, (Category)-1, 1},
	{   {0, TYPE_SHORT, (Category)-1, 0}}},
    {1, (CompFuncV)floatCastIV, _dxfComputeCheckSameShape,
	{0, TYPE_FLOAT, (Category)-1, 1},
	{   {0, TYPE_INT, (Category)-1, 0}}},
    {1, (CompFuncV)floatCastUIV, _dxfComputeCheckSameShape,
	{0, TYPE_FLOAT, (Category)-1, 1},
	{   {0, TYPE_UINT, (Category)-1, 0}}},
    {1, (CompFuncV)_dxfComputeCopy, _dxfComputeCheckSameShape,
	{0, TYPE_FLOAT, (Category)-1, 1},
	{   {0, TYPE_FLOAT, (Category)-1, 0}}},
    {1, (CompFuncV)floatCastDV, _dxfComputeCheckSameShape,
	{0, TYPE_FLOAT, (Category)-1, 1},
	{   {0, TYPE_DOUBLE, (Category)-1, 0}}}
};
SIZEINT(_dxdComputeFloats);

OperBinding _dxdComputeUbytes[] = {
    {1, (CompFuncV)_dxfComputeCopy, _dxfComputeCheckSameShape,
	{0, TYPE_UBYTE, (Category)-1, 1},
	{   {0, TYPE_UBYTE, (Category)-1, 0}}},
    {1, (CompFuncV)ubyteCastBV, _dxfComputeCheckSameShape,
	{0, TYPE_UBYTE, (Category)-1, 1},
	{   {0, TYPE_BYTE, (Category)-1, 0}}},
    {1, (CompFuncV)ubyteCastUSV, _dxfComputeCheckSameShape,
	{0, TYPE_UBYTE, (Category)-1, 1},
	{   {0, TYPE_USHORT, (Category)-1, 0}}},
    {1, (CompFuncV)ubyteCastSV, _dxfComputeCheckSameShape,
	{0, TYPE_UBYTE, (Category)-1, 1},
	{   {0, TYPE_SHORT, (Category)-1, 0}}},
    {1, (CompFuncV)ubyteCastIV, _dxfComputeCheckSameShape,
	{0, TYPE_UBYTE, (Category)-1, 1},
	{   {0, TYPE_INT, (Category)-1, 0}}},
    {1, (CompFuncV)ubyteCastUIV, _dxfComputeCheckSameShape,
	{0, TYPE_UBYTE, (Category)-1, 1},
	{   {0, TYPE_UINT, (Category)-1, 0}}},
    {1, (CompFuncV)ubyteCastFV, _dxfComputeCheckSameShape,
	{0, TYPE_UBYTE, (Category)-1, 1},
	{   {0, TYPE_FLOAT, (Category)-1, 0}}},
    {1, (CompFuncV)ubyteCastDV, _dxfComputeCheckSameShape,
	{0, TYPE_UBYTE, (Category)-1, 1},
	{   {0, TYPE_DOUBLE, (Category)-1, 0}}}
};
SIZEINT(_dxdComputeUbytes);
OperBinding _dxdComputeSbytes[] = {
    {1, (CompFuncV)sbyteCastUBV, _dxfComputeCheckSameShape,
	{0, TYPE_BYTE, (Category)-1, 1},
	{   {0, TYPE_UBYTE, (Category)-1, 0}}},
    {1, (CompFuncV)_dxfComputeCopy, _dxfComputeCheckSameShape,
	{0, TYPE_BYTE, (Category)-1, 1},
	{   {0, TYPE_BYTE, (Category)-1, 0}}},
    {1, (CompFuncV)sbyteCastUSV, _dxfComputeCheckSameShape,
	{0, TYPE_BYTE, (Category)-1, 1},
	{   {0, TYPE_USHORT, (Category)-1, 0}}},
    {1, (CompFuncV)sbyteCastSV, _dxfComputeCheckSameShape,
	{0, TYPE_BYTE, (Category)-1, 1},
	{   {0, TYPE_SHORT, (Category)-1, 0}}},
    {1, (CompFuncV)sbyteCastIV, _dxfComputeCheckSameShape,
	{0, TYPE_BYTE, (Category)-1, 1},
	{   {0, TYPE_INT, (Category)-1, 0}}},
    {1, (CompFuncV)sbyteCastUIV, _dxfComputeCheckSameShape,
	{0, TYPE_BYTE, (Category)-1, 1},
	{   {0, TYPE_UINT, (Category)-1, 0}}},
    {1, (CompFuncV)sbyteCastFV, _dxfComputeCheckSameShape,
	{0, TYPE_BYTE, (Category)-1, 1},
	{   {0, TYPE_FLOAT, (Category)-1, 0}}},
    {1, (CompFuncV)sbyteCastDV, _dxfComputeCheckSameShape,
	{0, TYPE_BYTE, (Category)-1, 1},
	{   {0, TYPE_DOUBLE, (Category)-1, 0}}}
};
SIZEINT(_dxdComputeSbytes);

OperBinding _dxdComputeShorts[] = {
    {1, (CompFuncV)shortCastUBV, _dxfComputeCheckSameShape,
	{0, TYPE_SHORT, (Category)-1, 1},
	{   {0, TYPE_UBYTE, (Category)-1, 0}}},
    {1, (CompFuncV)shortCastBV, _dxfComputeCheckSameShape,
	{0, TYPE_SHORT, (Category)-1, 1},
	{   {0, TYPE_BYTE, (Category)-1, 0}}},
    {1, (CompFuncV)shortCastUSV, _dxfComputeCheckSameShape,
	{0, TYPE_SHORT, (Category)-1, 1},
	{   {0, TYPE_USHORT, (Category)-1, 0}}},
    {1, (CompFuncV)_dxfComputeCopy, _dxfComputeCheckSameShape,
	{0, TYPE_SHORT, (Category)-1, 1},
	{   {0, TYPE_SHORT, (Category)-1, 0}}},
    {1, (CompFuncV)shortCastIV, _dxfComputeCheckSameShape,
	{0, TYPE_SHORT, (Category)-1, 1},
	{   {0, TYPE_INT, (Category)-1, 0}}},
    {1, (CompFuncV)shortCastUIV, _dxfComputeCheckSameShape,
	{0, TYPE_SHORT, (Category)-1, 1},
	{   {0, TYPE_UINT, (Category)-1, 0}}},
    {1, (CompFuncV)shortCastFV, _dxfComputeCheckSameShape,
	{0, TYPE_SHORT, (Category)-1, 1},
	{   {0, TYPE_FLOAT, (Category)-1, 0}}},
    {1, (CompFuncV)shortCastDV, _dxfComputeCheckSameShape,
	{0, TYPE_SHORT, (Category)-1, 1},
	{   {0, TYPE_DOUBLE, (Category)-1, 0}}}
};
SIZEINT(_dxdComputeShorts);
OperBinding _dxdComputeUshorts[] = {
    {1, (CompFuncV)ushortCastUBV, _dxfComputeCheckSameShape,
	{0, TYPE_USHORT, (Category)-1, 1},
	{   {0, TYPE_UBYTE, (Category)-1, 0}}},
    {1, (CompFuncV)ushortCastBV, _dxfComputeCheckSameShape,
	{0, TYPE_USHORT, (Category)-1, 1},
	{   {0, TYPE_BYTE, (Category)-1, 0}}},
    {1, (CompFuncV)_dxfComputeCopy, _dxfComputeCheckSameShape,
	{0, TYPE_USHORT, (Category)-1, 1},
	{   {0, TYPE_USHORT, (Category)-1, 0}}},
    {1, (CompFuncV)ushortCastSV, _dxfComputeCheckSameShape,
	{0, TYPE_USHORT, (Category)-1, 1},
	{   {0, TYPE_SHORT, (Category)-1, 0}}},
    {1, (CompFuncV)ushortCastIV, _dxfComputeCheckSameShape,
	{0, TYPE_USHORT, (Category)-1, 1},
	{   {0, TYPE_INT, (Category)-1, 0}}},
    {1, (CompFuncV)ushortCastUIV, _dxfComputeCheckSameShape,
	{0, TYPE_USHORT, (Category)-1, 1},
	{   {0, TYPE_UINT, (Category)-1, 0}}},
    {1, (CompFuncV)ushortCastFV, _dxfComputeCheckSameShape,
	{0, TYPE_USHORT, (Category)-1, 1},
	{   {0, TYPE_FLOAT, (Category)-1, 0}}},
    {1, (CompFuncV)ushortCastDV, _dxfComputeCheckSameShape,
	{0, TYPE_USHORT, (Category)-1, 1},
	{   {0, TYPE_DOUBLE, (Category)-1, 0}}}
};
SIZEINT(_dxdComputeUshorts);

OperBinding _dxdComputeDoubles[] = {
    {1, (CompFuncV)doubleCastUBV, _dxfComputeCheckSameShape,
	{0, TYPE_DOUBLE, (Category)-1, 1},
	{   {0, TYPE_UBYTE, (Category)-1, 0}}},
    {1, (CompFuncV)doubleCastBV, _dxfComputeCheckSameShape,
	{0, TYPE_DOUBLE, (Category)-1, 1},
	{   {0, TYPE_BYTE, (Category)-1, 0}}},
    {1, (CompFuncV)doubleCastUSV, _dxfComputeCheckSameShape,
	{0, TYPE_DOUBLE, (Category)-1, 1},
	{   {0, TYPE_USHORT, (Category)-1, 0}}},
    {1, (CompFuncV)doubleCastSV, _dxfComputeCheckSameShape,
	{0, TYPE_DOUBLE, (Category)-1, 1},
	{   {0, TYPE_SHORT, (Category)-1, 0}}},
    {1, (CompFuncV)doubleCastIV, _dxfComputeCheckSameShape,
	{0, TYPE_DOUBLE, (Category)-1, 1},
	{   {0, TYPE_INT, (Category)-1, 0}}},
    {1, (CompFuncV)doubleCastUIV, _dxfComputeCheckSameShape,
	{0, TYPE_DOUBLE, (Category)-1, 1},
	{   {0, TYPE_UINT, (Category)-1, 0}}},
    {1, (CompFuncV)doubleCastFV, _dxfComputeCheckSameShape,
	{0, TYPE_DOUBLE, (Category)-1, 1},
	{   {0, TYPE_FLOAT, (Category)-1, 0}}},
    {1, (CompFuncV)_dxfComputeCopy, _dxfComputeCheckSameShape,
	{0, TYPE_DOUBLE, (Category)-1, 1},
	{   {0, TYPE_DOUBLE, (Category)-1, 0}}}
};
SIZEINT(_dxdComputeDoubles);


VFUNC1 (realVF, float, complexFloat, REAL)
VFUNC1 (realVI, int,   complexInt, REAL)
VFUNC1 (imagVF, float, complexFloat, IMAG)
VFUNC1 (imagVI, int,   complexInt, IMAG)
VFUNC1 (realVQF, float, quaternionFloat, REAL)
VFUNC1 (realVQI, int,   quaternionInt, REAL)
VFUNC1 (imagiVQF, float, quaternionFloat, IMAGI)
VFUNC1 (imagiVQI, int,   quaternionInt, IMAGI)
VFUNC1 (imagjVQF, float, quaternionFloat, IMAGJ)
VFUNC1 (imagjVQI, int,   quaternionInt, IMAGJ)
VFUNC1 (imagkVQF, float, quaternionFloat, IMAGK)
VFUNC1 (imagkVQI, int,   quaternionInt, IMAGK)
OperBinding _dxdComputeReals[] = {
    {1, (CompFuncV)realVF, _dxfComputeCheckSameShape,
	{0, TYPE_FLOAT, CATEGORY_REAL, 0},
	{   {0, TYPE_FLOAT, CATEGORY_COMPLEX, 0}}},
    {1, (CompFuncV)realVI, _dxfComputeCheckSameShape,
	{0, TYPE_INT, CATEGORY_REAL, 0},
	{   {0, TYPE_INT, CATEGORY_COMPLEX, 0}}},
    {1, (CompFuncV)realVQF, _dxfComputeCheckSameShape,
	{0, TYPE_FLOAT, CATEGORY_REAL, 0},
	{   {0, TYPE_FLOAT, CATEGORY_QUATERNION, 0}}},
    {1, (CompFuncV)realVQI, _dxfComputeCheckSameShape,
	{0, TYPE_INT, CATEGORY_REAL, 0},
	{   {0, TYPE_INT, CATEGORY_QUATERNION, 0}}}
};
OperBinding _dxdComputeImags[] = {
    {1, (CompFuncV)imagVF, _dxfComputeCheckSameShape,
	{0, TYPE_FLOAT, CATEGORY_REAL, 0},
	{   {0, TYPE_FLOAT, CATEGORY_COMPLEX, 0}}},
    {1, (CompFuncV)imagVI, _dxfComputeCheckSameShape,
	{0, TYPE_INT, CATEGORY_REAL, 0},
	{   {0, TYPE_INT, CATEGORY_COMPLEX, 0}}}
};
OperBinding _dxdComputeImagis[] = {
    {1, (CompFuncV)imagiVQF, _dxfComputeCheckSameShape,
	{0, TYPE_FLOAT, CATEGORY_REAL, 0},
	{   {0, TYPE_FLOAT, CATEGORY_QUATERNION, 0}}},
    {1, (CompFuncV)imagiVQI, _dxfComputeCheckSameShape,
	{0, TYPE_INT, CATEGORY_REAL, 0},
	{   {0, TYPE_INT, CATEGORY_QUATERNION, 0}}}
};
OperBinding _dxdComputeImagjs[] = {
    {1, (CompFuncV)imagjVQF, _dxfComputeCheckSameShape,
	{0, TYPE_FLOAT, CATEGORY_REAL, 0},
	{   {0, TYPE_FLOAT, CATEGORY_QUATERNION, 0}}},
    {1, (CompFuncV)imagjVQI, _dxfComputeCheckSameShape,
	{0, TYPE_INT, CATEGORY_REAL, 0},
	{   {0, TYPE_INT, CATEGORY_QUATERNION, 0}}}
};
OperBinding _dxdComputeImagks[] = {
    {1, (CompFuncV)imagkVQF, _dxfComputeCheckSameShape,
	{0, TYPE_FLOAT, CATEGORY_REAL, 0},
	{   {0, TYPE_FLOAT, CATEGORY_QUATERNION, 0}}},
    {1, (CompFuncV)imagkVQI, _dxfComputeCheckSameShape,
	{0, TYPE_INT, CATEGORY_REAL, 0},
	{   {0, TYPE_INT, CATEGORY_QUATERNION, 0}}}
};
SIZEINT(_dxdComputeReals);
SIZEINT(_dxdComputeImags);
SIZEINT(_dxdComputeImagis);
SIZEINT(_dxdComputeImagjs);
SIZEINT(_dxdComputeImagks);

#define COMPLEX1(name, tr, t0)			\
static int					\
name(						\
    PTreeNode *pt, 				\
    ObjStruct *os, 				\
    int numInputs,				\
    Pointer *inputs,				\
    Pointer result,				\
    InvalidComponentHandle *invalids,		\
    InvalidComponentHandle outInvalid)		\
{						\
    register tr *out = (tr *) result;\
    register t0 *in0 = (t0 *) inputs[0];	\
    int size0 = pt->args->metaType.items;	\
    int i, j;					\
    int numBasic, items;			\
    int allValid = 				\
	(invalids[0] == NULL || DXGetInvalidCount(invalids[0]) == 0); \
						\
    for (numBasic = 1, i = 0;			\
	    i < pt->metaType.rank;		\
	    ++i)				\
	numBasic *= pt->metaType.shape[i];	\
						\
    size0 *= numBasic;				\
						\
    items = pt->metaType.items;			\
    for (i = 0; i < items; ++i) 		\
	if (allValid || _dxfComputeInvalidOne(outInvalid, i, invalids[0], i)) 	\
	    for (j = 0; j < numBasic; ++j) {	\
		out[i].realPart = in0[i];	\
		out[i].imagPart = 0;		\
	    }					\
    return (OK);				\
}
COMPLEX1(complexVF, complexFloat, float)
COMPLEX1(complexVI, complexInt,   int)

#define COMPLEX2(name, tr, t0, t1)		\
static int					\
name(						\
    PTreeNode *pt, 				\
    ObjStruct *os, 				\
    int numInputs,				\
    Pointer *inputs,				\
    Pointer result,				\
    InvalidComponentHandle *invalids,		\
    InvalidComponentHandle outInvalid)		\
{						\
    register tr *out = (tr *) result;\
    register t0 *in0 = (t0 *) inputs[0];	\
    register t1 *in1 = (t1 *) inputs[1];	\
    int size0 = pt->args->metaType.items;	\
    int size1 = pt->args->next->metaType.items;	\
    int i, j, k, l;				\
    int numBasic, items;			\
    int allValid = 				\
	(invalids[0] == NULL || DXGetInvalidCount(invalids[0]) == 0) && \
	(invalids[1] == NULL || DXGetInvalidCount(invalids[1]) == 0);	\
						\
    for (numBasic = 1, i = 0;			\
	    i < pt->metaType.rank;		\
	    ++i)				\
	numBasic *= pt->metaType.shape[i];	\
						\
    size0 *= numBasic;				\
    size1 *= numBasic;				\
						\
    items = pt->metaType.items;			\
    for (i = j = k = 0; i < items; ++i) {	\
	for (l = 0; l < numBasic; ++l) {	\
	    if (allValid || _dxfComputeInvalidOr(outInvalid, i, invalids[0], j, invalids[1], k)) {\
		out[i*numBasic+l].realPart = in0[j];	\
		out[i*numBasic+l].imagPart = in1[k];	\
	    }					\
	    ++j, ++k;				\
	    if (j >= size0) j = 0;		\
	    if (k >= size1) k = 0;		\
	}					\
    }						\
    return (OK);				\
}
COMPLEX2(complexVFF, complexFloat, float, float)
COMPLEX2(complexVII, complexInt, int, int)

OperBinding _dxdComputeComplexes[] = {
    {1, (CompFuncV)complexVF, _dxfComputeCheckSameShape,
	{0, TYPE_FLOAT, CATEGORY_COMPLEX, 0},
	{   {0, TYPE_FLOAT, CATEGORY_REAL, 0}}},
    {1, (CompFuncV)complexVI, _dxfComputeCheckSameShape,
	{0, TYPE_INT, CATEGORY_COMPLEX, 0},
	{   {0, TYPE_INT, CATEGORY_REAL, 0}}},
    {2, (CompFuncV)complexVFF, _dxfComputeCheckSameShape,
	{0, TYPE_FLOAT, CATEGORY_COMPLEX, 0},
	{   {0, TYPE_FLOAT, CATEGORY_REAL, 0},
	    {0, TYPE_FLOAT, CATEGORY_REAL, 0}}},
    {2, (CompFuncV)complexVII, _dxfComputeCheckSameShape,
	{0, TYPE_INT, CATEGORY_COMPLEX, 0},
	{   {0, TYPE_INT, CATEGORY_REAL, 0},
	    {0, TYPE_INT, CATEGORY_REAL, 0}}}
};
SIZEINT(_dxdComputeComplexes);

static int
quaternionVF(
    PTreeNode *pt, 
    ObjStruct *os, 
    int numInputs,
    Pointer *inputs,
    Pointer result,
    InvalidComponentHandle *invalids,
    InvalidComponentHandle outInvalid)
{
    register quaternionFloat *out = (quaternionFloat *) result;
    register float *in0 = (float *) inputs[0];
    int size0 = pt->args->metaType.items;
    int i;
    int numBasic;

    for (numBasic = 1, i = 0;
	    i < pt->metaType.rank;
	    ++i)
	numBasic *= pt->metaType.shape[i];

    size0 *= numBasic;

    for (i = 0;
	    i < pt->metaType.items * numBasic;
	    ++i) {
	out[i].realPart = in0[i];
	out[i].iPart = 0;
	out[i].jPart = 0;
	out[i].kPart = 0;
    }
    return (OK);
}
static int
quaternionVI(
    PTreeNode *pt, 
    ObjStruct *os, 
    int numInputs,
    Pointer *inputs,
    Pointer result,
    InvalidComponentHandle *invalids,
    InvalidComponentHandle outInvalid)
{
    register quaternionInt *out = (quaternionInt *) result;
    register int *in0 = (int *) inputs[0];
    int i;
    int numBasic;

    for (numBasic = 1, i = 0;
	    i < pt->metaType.rank;
	    ++i)
	numBasic *= pt->metaType.shape[i];

    for (i = 0;
	    i < pt->metaType.items;
	    ++i) {
	out[i].realPart = in0[i];
	out[i].iPart = 0;
	out[i].jPart = 0;
	out[i].kPart = 0;
    }
    return (OK);
}
static int
quaternionVFFFF(
    PTreeNode *pt, 
    ObjStruct *os, 
    int numInputs,
    Pointer *inputs,
    Pointer result,
    InvalidComponentHandle *invalids,
    InvalidComponentHandle outInvalid)
{
    register quaternionFloat *out = (quaternionFloat *) result;
    register float *in0 = (float *) inputs[0];
    register float *in1 = (float *) inputs[1];
    register float *in2 = (float *) inputs[2];
    register float *in3 = (float *) inputs[3];
    int size0 = pt->args->metaType.items;
    int size1 = pt->args->next->metaType.items;
    int size2 = pt->args->next->next->metaType.items;
    int size3 = pt->args->next->next->next->metaType.items;
    int i, j, k, l, m;
    int numBasic;

    for (numBasic = 1, i = 0;
	    i < pt->metaType.rank;
	    ++i)
	numBasic *= pt->metaType.shape[i];

    size0 *= numBasic;
    size1 *= numBasic;
    size2 *= numBasic;
    size3 *= numBasic;

    for (i = j = k = l = m = 0;
	    i < pt->metaType.items;
	    ++i) {
	out[i].realPart = in0[j];
	out[i].iPart = in1[k];
	out[i].jPart = in2[l];
	out[i].kPart = in3[m];
	++j, ++k, ++l, ++m;
	if (j >= size0) j = 0;
	if (k >= size1) k = 0;
	if (l >= size2) l = 0;
	if (m >= size3) m = 0;
    }
    return (OK);
}
static int
quaternionVIIII(
    PTreeNode *pt, 
    ObjStruct *os, 
    int numInputs,
    Pointer *inputs,
    Pointer result,
    InvalidComponentHandle *invalids,
    InvalidComponentHandle outInvalid)
{
    register quaternionInt *out = (quaternionInt *) result;
    register int *in0 = (int *) inputs[0];
    register int *in1 = (int *) inputs[1];
    register int *in2 = (int *) inputs[2];
    register int *in3 = (int *) inputs[3];
    int size0 = pt->args->metaType.items;
    int size1 = pt->args->next->metaType.items;
    int size2 = pt->args->next->next->metaType.items;
    int size3 = pt->args->next->next->next->metaType.items;
    int i, j, k, l, m;
    int numBasic;

    for (numBasic = 1, i = 0;
	    i < pt->metaType.rank;
	    ++i)
	numBasic *= pt->metaType.shape[i];

    size0 *= numBasic;
    size1 *= numBasic;
    size2 *= numBasic;
    size3 *= numBasic;

    for (i = j = k = l = m = 0;
	    i < pt->metaType.items;
	    ++i) {
	out[i].realPart = in0[j];
	out[i].iPart = in1[k];
	out[i].jPart = in2[l];
	out[i].kPart = in3[m];
	++j, ++k, ++l, ++m;
	if (j >= size0) j = 0;
	if (k >= size1) k = 0;
	if (l >= size2) l = 0;
	if (m >= size3) m = 0;
    }
    return (OK);
}
OperBinding _dxdComputeQuaternions[] = {
    {1, (CompFuncV)quaternionVF, _dxfComputeCheckSameShape,
	{0, TYPE_FLOAT, CATEGORY_QUATERNION, 0},
	{   {0, TYPE_FLOAT, CATEGORY_REAL, 0}}},
    {1, (CompFuncV)quaternionVI, _dxfComputeCheckSameShape,
	{0, TYPE_INT, CATEGORY_QUATERNION, 0},
	{   {0, TYPE_INT, CATEGORY_REAL, 0}}},
    {4, (CompFuncV)quaternionVFFFF, _dxfComputeCheckSameShape,
	{0, TYPE_FLOAT, CATEGORY_QUATERNION, 0},
	{   {0, TYPE_FLOAT, CATEGORY_REAL, 0},
	    {0, TYPE_FLOAT, CATEGORY_REAL, 0},
	    {0, TYPE_FLOAT, CATEGORY_REAL, 0},
	    {0, TYPE_FLOAT, CATEGORY_REAL, 0}}},
    {4, (CompFuncV)quaternionVIIII, _dxfComputeCheckSameShape,
	{0, TYPE_INT, CATEGORY_QUATERNION, 0},
	{   {0, TYPE_INT, CATEGORY_REAL, 0},
	    {0, TYPE_INT, CATEGORY_REAL, 0},
	    {0, TYPE_INT, CATEGORY_REAL, 0},
	    {0, TYPE_INT, CATEGORY_REAL, 0}}}
};
SIZEINT(_dxdComputeQuaternions);


static int
ExecInvalid (
    PTreeNode *pt, 
    ObjStruct *os, 
    int numInputs,
    Pointer *inputs,
    Pointer result,
    InvalidComponentHandle *invalids,
    InvalidComponentHandle outInvalid)
{
    int items = pt->metaType.items;
    int i;
    for (i = 0; i < items; ++i)
	((int*)result)[i] = DXIsElementInvalidSequential(invalids[0], i);
    if (outInvalid)
	DXSetAllValid(outInvalid);
    return OK;
}


OperBinding _dxdComputeInvalids[] = {
    {1, (CompFuncV)ExecInvalid, _dxfComputeCheckSameSpecReturn,
	{0, TYPE_INT, CATEGORY_REAL, 0}}
};
SIZEINT(_dxdComputeInvalids);

#define AND(x,y)  ((x) & (y))
VSFUNC2 (andVSII, int, int, int, AND)
VSFUNC2 (andVSSS, signed short, signed short, signed short, AND)
VSFUNC2 (andVSBB, signed char, signed char, signed char, AND)

SVFUNC2 (andSVBB, signed char, signed char, signed char, AND)
SVFUNC2 (andSVSS, signed short, signed short, signed short, AND)
SVFUNC2 (andSVII, int, int, int, AND)

VFUNC2 (andB, unsigned char, unsigned char, unsigned char, AND)
VFUNC2 (andS, short, short, short, AND)
VFUNC2 (andI, int, int, int, AND)

OperBinding _dxdComputeBands[] = {
    {2, (CompFuncV)andVSII, _dxfComputeCheckDimAnyShape,
	{0, TYPE_INT, CATEGORY_REAL, 0},
	{   {0, TYPE_INT, CATEGORY_REAL, 1},
	    {0, TYPE_INT, CATEGORY_REAL, 0}}},
    {2, (CompFuncV)andSVII, _dxfComputeCheckDimAnyShape,
	{0, TYPE_INT, CATEGORY_REAL, 0},
	{   {0, TYPE_INT, CATEGORY_REAL, 0},
	    {0, TYPE_INT, CATEGORY_REAL, 1}}},
    {2, (CompFuncV)andI, _dxfComputeCheckSameShape,
	{0, TYPE_INT, CATEGORY_REAL, 0},
	{   {0, TYPE_INT, CATEGORY_REAL, 0},
	    {0, TYPE_INT, CATEGORY_REAL, 0}}},
    {2, (CompFuncV)andVSSS, _dxfComputeCheckDimAnyShape,
	{0, TYPE_SHORT, CATEGORY_REAL, 0},
	{   {0, TYPE_SHORT, CATEGORY_REAL, 1},
	    {0, TYPE_SHORT, CATEGORY_REAL, 0}}},
    {2, (CompFuncV)andSVSS, _dxfComputeCheckDimAnyShape,
	{0, TYPE_SHORT, CATEGORY_REAL, 0},
	{   {0, TYPE_SHORT, CATEGORY_REAL, 0},
	    {0, TYPE_SHORT, CATEGORY_REAL, 1}}},
    {2, (CompFuncV)andS, _dxfComputeCheckSameShape,
	{0, TYPE_SHORT, CATEGORY_REAL, 0},
	{   {0, TYPE_SHORT, CATEGORY_REAL, 0},
	    {0, TYPE_SHORT, CATEGORY_REAL, 0}}},
    {2, (CompFuncV)andVSBB, _dxfComputeCheckDimAnyShape,
	{0, TYPE_BYTE, CATEGORY_REAL, 0},
	{   {0, TYPE_BYTE, CATEGORY_REAL, 1},
	    {0, TYPE_BYTE, CATEGORY_REAL, 0}}},
    {2, (CompFuncV)andSVBB, _dxfComputeCheckDimAnyShape,
	{0, TYPE_BYTE, CATEGORY_REAL, 0},
	{   {0, TYPE_BYTE, CATEGORY_REAL, 0},
	    {0, TYPE_BYTE, CATEGORY_REAL, 1}}},
    {2, (CompFuncV)andB, _dxfComputeCheckSameShape,
	{0, TYPE_BYTE, CATEGORY_REAL, 0},
	{   {0, TYPE_BYTE, CATEGORY_REAL, 0},
	    {0, TYPE_BYTE, CATEGORY_REAL, 0}}},
    {2, (CompFuncV)andVSII, _dxfComputeCheckDimAnyShape,
	{0, TYPE_UINT, CATEGORY_REAL, 0},
	{   {0, TYPE_UINT, CATEGORY_REAL, 1},
	    {0, TYPE_UINT, CATEGORY_REAL, 0}}},
    {2, (CompFuncV)andSVII, _dxfComputeCheckDimAnyShape,
	{0, TYPE_UINT, CATEGORY_REAL, 0},
	{   {0, TYPE_UINT, CATEGORY_REAL, 0},
	    {0, TYPE_UINT, CATEGORY_REAL, 1}}},
    {2, (CompFuncV)andI, _dxfComputeCheckSameShape,
	{0, TYPE_UINT, CATEGORY_REAL, 0},
	{   {0, TYPE_UINT, CATEGORY_REAL, 0},
	    {0, TYPE_UINT, CATEGORY_REAL, 0}}},
    {2, (CompFuncV)andVSSS, _dxfComputeCheckDimAnyShape,
	{0, TYPE_USHORT, CATEGORY_REAL, 0},
	{   {0, TYPE_USHORT, CATEGORY_REAL, 1},
	    {0, TYPE_USHORT, CATEGORY_REAL, 0}}},
    {2, (CompFuncV)andSVSS, _dxfComputeCheckDimAnyShape,
	{0, TYPE_USHORT, CATEGORY_REAL, 0},
	{   {0, TYPE_USHORT, CATEGORY_REAL, 0},
	    {0, TYPE_USHORT, CATEGORY_REAL, 1}}},
    {2, (CompFuncV)andS, _dxfComputeCheckSameShape,
	{0, TYPE_USHORT, CATEGORY_REAL, 0},
	{   {0, TYPE_USHORT, CATEGORY_REAL, 0},
	    {0, TYPE_USHORT, CATEGORY_REAL, 0}}},
    {2, (CompFuncV)andVSBB, _dxfComputeCheckDimAnyShape,
	{0, TYPE_UBYTE, CATEGORY_REAL, 0},
	{   {0, TYPE_UBYTE, CATEGORY_REAL, 1},
	    {0, TYPE_UBYTE, CATEGORY_REAL, 0}}},
    {2, (CompFuncV)andSVBB, _dxfComputeCheckDimAnyShape,
	{0, TYPE_UBYTE, CATEGORY_REAL, 0},
	{   {0, TYPE_UBYTE, CATEGORY_REAL, 0},
	    {0, TYPE_UBYTE, CATEGORY_REAL, 1}}},
    {2, (CompFuncV)andB, _dxfComputeCheckSameShape,
	{0, TYPE_UBYTE, CATEGORY_REAL, 0},
	{   {0, TYPE_UBYTE, CATEGORY_REAL, 0},
	    {0, TYPE_UBYTE, CATEGORY_REAL, 0}}}
};
SIZEINT(_dxdComputeBands);
#define OR(x,y)  ((x) | (y))
VSFUNC2 (orVSII, int, int, int, OR)
SVFUNC2 (orSVII, int, int, int, OR)
VFUNC2 (orI, int, int, int, OR)
VSFUNC2 (orVSBB, signed char, signed char, signed char, OR)
SVFUNC2 (orSVBB, signed char, signed char, signed char, OR)
VFUNC2 (orB, signed char, signed char, signed char, OR)
VSFUNC2 (orVSSS, signed short, signed short, signed short, OR)
SVFUNC2 (orSVSS, signed short, signed short, signed short, OR)
VFUNC2 (orS, signed short, signed short, signed short, OR)
OperBinding _dxdComputeBors[] = {
    {2, (CompFuncV)orVSII, _dxfComputeCheckDimAnyShape,
	{0, TYPE_INT, CATEGORY_REAL, 0},
	{   {0, TYPE_INT, CATEGORY_REAL, 1},
	    {0, TYPE_INT, CATEGORY_REAL, 0}}},
    {2, (CompFuncV)orSVII, _dxfComputeCheckDimAnyShape,
	{0, TYPE_INT, CATEGORY_REAL, 0},
	{   {0, TYPE_INT, CATEGORY_REAL, 0},
	    {0, TYPE_INT, CATEGORY_REAL, 1}}},
    {2, (CompFuncV)orI, _dxfComputeCheckSameShape,
	{0, TYPE_INT, CATEGORY_REAL, 0},
	{   {0, TYPE_INT, CATEGORY_REAL, 0},
	    {0, TYPE_INT, CATEGORY_REAL, 0}}},
    {2, (CompFuncV)orVSSS, _dxfComputeCheckDimAnyShape,
	{0, TYPE_SHORT, CATEGORY_REAL, 0},
	{   {0, TYPE_SHORT, CATEGORY_REAL, 1},
	    {0, TYPE_SHORT, CATEGORY_REAL, 0}}},
    {2, (CompFuncV)orSVSS, _dxfComputeCheckDimAnyShape,
	{0, TYPE_SHORT, CATEGORY_REAL, 0},
	{   {0, TYPE_SHORT, CATEGORY_REAL, 0},
	    {0, TYPE_SHORT, CATEGORY_REAL, 1}}},
    {2, (CompFuncV)orS, _dxfComputeCheckSameShape,
	{0, TYPE_SHORT, CATEGORY_REAL, 0},
	{   {0, TYPE_SHORT, CATEGORY_REAL, 0},
	    {0, TYPE_SHORT, CATEGORY_REAL, 0}}},
    {2, (CompFuncV)orVSBB, _dxfComputeCheckDimAnyShape,
	{0, TYPE_BYTE, CATEGORY_REAL, 0},
	{   {0, TYPE_BYTE, CATEGORY_REAL, 1},
	    {0, TYPE_BYTE, CATEGORY_REAL, 0}}},
    {2, (CompFuncV)orSVBB, _dxfComputeCheckDimAnyShape,
	{0, TYPE_BYTE, CATEGORY_REAL, 0},
	{   {0, TYPE_BYTE, CATEGORY_REAL, 0},
	    {0, TYPE_BYTE, CATEGORY_REAL, 1}}},
    {2, (CompFuncV)orB, _dxfComputeCheckSameShape,
	{0, TYPE_BYTE, CATEGORY_REAL, 0},
	{   {0, TYPE_BYTE, CATEGORY_REAL, 0},
	    {0, TYPE_BYTE, CATEGORY_REAL, 0}}},
    {2, (CompFuncV)orVSII, _dxfComputeCheckDimAnyShape,
	{0, TYPE_UINT, CATEGORY_REAL, 0},
	{   {0, TYPE_UINT, CATEGORY_REAL, 1},
	    {0, TYPE_UINT, CATEGORY_REAL, 0}}},
    {2, (CompFuncV)orSVII, _dxfComputeCheckDimAnyShape,
	{0, TYPE_UINT, CATEGORY_REAL, 0},
	{   {0, TYPE_UINT, CATEGORY_REAL, 0},
	    {0, TYPE_UINT, CATEGORY_REAL, 1}}},
    {2, (CompFuncV)orI, _dxfComputeCheckSameShape,
	{0, TYPE_UINT, CATEGORY_REAL, 0},
	{   {0, TYPE_UINT, CATEGORY_REAL, 0},
	    {0, TYPE_UINT, CATEGORY_REAL, 0}}},
    {2, (CompFuncV)orVSSS, _dxfComputeCheckDimAnyShape,
	{0, TYPE_USHORT, CATEGORY_REAL, 0},
	{   {0, TYPE_USHORT, CATEGORY_REAL, 1},
	    {0, TYPE_USHORT, CATEGORY_REAL, 0}}},
    {2, (CompFuncV)orSVSS, _dxfComputeCheckDimAnyShape,
	{0, TYPE_USHORT, CATEGORY_REAL, 0},
	{   {0, TYPE_USHORT, CATEGORY_REAL, 0},
	    {0, TYPE_USHORT, CATEGORY_REAL, 1}}},
    {2, (CompFuncV)orS, _dxfComputeCheckSameShape,
	{0, TYPE_USHORT, CATEGORY_REAL, 0},
	{   {0, TYPE_USHORT, CATEGORY_REAL, 0},
	    {0, TYPE_USHORT, CATEGORY_REAL, 0}}},
    {2, (CompFuncV)orVSBB, _dxfComputeCheckDimAnyShape,
	{0, TYPE_UBYTE, CATEGORY_REAL, 0},
	{   {0, TYPE_UBYTE, CATEGORY_REAL, 1},
	    {0, TYPE_UBYTE, CATEGORY_REAL, 0}}},
    {2, (CompFuncV)orSVBB, _dxfComputeCheckDimAnyShape,
	{0, TYPE_UBYTE, CATEGORY_REAL, 0},
	{   {0, TYPE_UBYTE, CATEGORY_REAL, 0},
	    {0, TYPE_UBYTE, CATEGORY_REAL, 1}}},
    {2, (CompFuncV)orB, _dxfComputeCheckSameShape,
	{0, TYPE_UBYTE, CATEGORY_REAL, 0},
	{   {0, TYPE_UBYTE, CATEGORY_REAL, 0},
	    {0, TYPE_UBYTE, CATEGORY_REAL, 0}}},
};
SIZEINT(_dxdComputeBors);
#define XOR(x,y)  ((x) ^ (y))
VSFUNC2 (xorVSII, int, int, int, XOR)
SVFUNC2 (xorSVII, int, int, int, XOR)
VFUNC2 (xorI, int, int, int, XOR)
VSFUNC2 (xorVSSS, signed short, signed short, signed short, XOR)
SVFUNC2 (xorSVSS, signed short, signed short, signed short, XOR)
VFUNC2 (xorS, signed short, signed short, signed short, XOR)
VSFUNC2 (xorVSBB, signed char, signed char, signed char, XOR)
SVFUNC2 (xorSVBB, signed char, signed char, signed char, XOR)
VFUNC2 (xorB, signed char, signed char, signed char, XOR)
OperBinding _dxdComputeBxors[] = {
    {2, (CompFuncV)xorVSII, _dxfComputeCheckDimAnyShape,
	{0, TYPE_INT, CATEGORY_REAL, 0},
	{   {0, TYPE_INT, CATEGORY_REAL, 1},
	    {0, TYPE_INT, CATEGORY_REAL, 0}}},
    {2, (CompFuncV)xorSVII, _dxfComputeCheckDimAnyShape,
	{0, TYPE_INT, CATEGORY_REAL, 0},
	{   {0, TYPE_INT, CATEGORY_REAL, 0},
	    {0, TYPE_INT, CATEGORY_REAL, 1}}},
    {2, (CompFuncV)xorI, _dxfComputeCheckSameShape,
	{0, TYPE_INT, CATEGORY_REAL, 0},
	{   {0, TYPE_INT, CATEGORY_REAL, 0},
	    {0, TYPE_INT, CATEGORY_REAL, 0}}},
    {2, (CompFuncV)xorVSSS, _dxfComputeCheckDimAnyShape,
	{0, TYPE_SHORT, CATEGORY_REAL, 0},
	{   {0, TYPE_SHORT, CATEGORY_REAL, 1},
	    {0, TYPE_SHORT, CATEGORY_REAL, 0}}},
    {2, (CompFuncV)xorSVSS, _dxfComputeCheckDimAnyShape,
	{0, TYPE_SHORT, CATEGORY_REAL, 0},
	{   {0, TYPE_SHORT, CATEGORY_REAL, 0},
	    {0, TYPE_SHORT, CATEGORY_REAL, 1}}},
    {2, (CompFuncV)xorS, _dxfComputeCheckSameShape,
	{0, TYPE_SHORT, CATEGORY_REAL, 0},
	{   {0, TYPE_SHORT, CATEGORY_REAL, 0},
	    {0, TYPE_SHORT, CATEGORY_REAL, 0}}},
    {2, (CompFuncV)xorVSBB, _dxfComputeCheckDimAnyShape,
	{0, TYPE_BYTE, CATEGORY_REAL, 0},
	{   {0, TYPE_BYTE, CATEGORY_REAL, 1},
	    {0, TYPE_BYTE, CATEGORY_REAL, 0}}},
    {2, (CompFuncV)xorSVBB, _dxfComputeCheckDimAnyShape,
	{0, TYPE_BYTE, CATEGORY_REAL, 0},
	{   {0, TYPE_BYTE, CATEGORY_REAL, 0},
	    {0, TYPE_BYTE, CATEGORY_REAL, 1}}},
    {2, (CompFuncV)xorB, _dxfComputeCheckSameShape,
	{0, TYPE_BYTE, CATEGORY_REAL, 0},
	{   {0, TYPE_BYTE, CATEGORY_REAL, 0},
	    {0, TYPE_BYTE, CATEGORY_REAL, 0}}},
    {2, (CompFuncV)xorVSII, _dxfComputeCheckDimAnyShape,
	{0, TYPE_UINT, CATEGORY_REAL, 0},
	{   {0, TYPE_UINT, CATEGORY_REAL, 1},
	    {0, TYPE_UINT, CATEGORY_REAL, 0}}},
    {2, (CompFuncV)xorSVII, _dxfComputeCheckDimAnyShape,
	{0, TYPE_UINT, CATEGORY_REAL, 0},
	{   {0, TYPE_UINT, CATEGORY_REAL, 0},
	    {0, TYPE_UINT, CATEGORY_REAL, 1}}},
    {2, (CompFuncV)xorI, _dxfComputeCheckSameShape,
	{0, TYPE_UINT, CATEGORY_REAL, 0},
	{   {0, TYPE_UINT, CATEGORY_REAL, 0},
	    {0, TYPE_UINT, CATEGORY_REAL, 0}}},
    {2, (CompFuncV)xorVSSS, _dxfComputeCheckDimAnyShape,
	{0, TYPE_USHORT, CATEGORY_REAL, 0},
	{   {0, TYPE_USHORT, CATEGORY_REAL, 1},
	    {0, TYPE_USHORT, CATEGORY_REAL, 0}}},
    {2, (CompFuncV)xorSVSS, _dxfComputeCheckDimAnyShape,
	{0, TYPE_USHORT, CATEGORY_REAL, 0},
	{   {0, TYPE_USHORT, CATEGORY_REAL, 0},
	    {0, TYPE_USHORT, CATEGORY_REAL, 1}}},
    {2, (CompFuncV)xorS, _dxfComputeCheckSameShape,
	{0, TYPE_USHORT, CATEGORY_REAL, 0},
	{   {0, TYPE_USHORT, CATEGORY_REAL, 0},
	    {0, TYPE_USHORT, CATEGORY_REAL, 0}}},
    {2, (CompFuncV)xorVSBB, _dxfComputeCheckDimAnyShape,
	{0, TYPE_UBYTE, CATEGORY_REAL, 0},
	{   {0, TYPE_UBYTE, CATEGORY_REAL, 1},
	    {0, TYPE_UBYTE, CATEGORY_REAL, 0}}},
    {2, (CompFuncV)xorSVBB, _dxfComputeCheckDimAnyShape,
	{0, TYPE_UBYTE, CATEGORY_REAL, 0},
	{   {0, TYPE_UBYTE, CATEGORY_REAL, 0},
	    {0, TYPE_UBYTE, CATEGORY_REAL, 1}}},
    {2, (CompFuncV)xorB, _dxfComputeCheckSameShape,
	{0, TYPE_UBYTE, CATEGORY_REAL, 0},
	{   {0, TYPE_UBYTE, CATEGORY_REAL, 0},
	    {0, TYPE_UBYTE, CATEGORY_REAL, 0}}}
};
SIZEINT(_dxdComputeBxors);
#define NOT(x)  (~(x))
VFUNC1 (notI, int, int, NOT)
VFUNC1 (notS, signed short, signed short, NOT)
VFUNC1 (notB, signed char, signed char, NOT)
OperBinding _dxdComputeBnots[] = {
    {1, (CompFuncV)notI, _dxfComputeCheckSameShape,
	{0, TYPE_INT, CATEGORY_REAL, 0},
	{   {0, TYPE_INT, CATEGORY_REAL, 0}}},
    {1, (CompFuncV)notS, _dxfComputeCheckSameShape,
	{0, TYPE_SHORT, CATEGORY_REAL, 0},
	{   {0, TYPE_SHORT, CATEGORY_REAL, 0}}},
    {1, (CompFuncV)notB, _dxfComputeCheckSameShape,
	{0, TYPE_BYTE, CATEGORY_REAL, 0},
	{   {0, TYPE_BYTE, CATEGORY_REAL, 0}}},
    {1, (CompFuncV)notI, _dxfComputeCheckSameShape,
	{0, TYPE_UINT, CATEGORY_REAL, 0},
	{   {0, TYPE_UINT, CATEGORY_REAL, 0}}},
    {1, (CompFuncV)notS, _dxfComputeCheckSameShape,
	{0, TYPE_USHORT, CATEGORY_REAL, 0},
	{   {0, TYPE_USHORT, CATEGORY_REAL, 0}}},
    {1, (CompFuncV)notB, _dxfComputeCheckSameShape,
	{0, TYPE_UBYTE, CATEGORY_REAL, 0},
	{   {0, TYPE_UBYTE, CATEGORY_REAL, 0}}}
};
SIZEINT(_dxdComputeBnots);

