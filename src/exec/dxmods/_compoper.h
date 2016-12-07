/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>

#ifndef __COMPOPER_H_
#define __COMPOPER_H_

#define MAX_INTYPES 4		/* the addition of any operator that requires 
				 * more than MAX_INTYPES inputs types will 
				 * require that this number be bumped.
				 */
typedef struct OperBinding OperBinding;
typedef int (*TypeCheckFunc)(PTreeNode *, ObjStruct *, OperBinding *);
struct OperBinding {
    int numArgs;	/* -1 -> arbitrary, match with first arg. */
    CompFuncV impl;
    TypeCheckFunc typeCheck;
    MetaType outType;
    MetaType inTypes[MAX_INTYPES];
};

typedef struct {
    int op;
    char *name;
    int numBindings;
    OperBinding *bindings;
} OperDesc;


/* Function calls recognised by this file */
#define FUNC_sqrt	(LAST_OPER+  1)
#define FUNC_pow	(LAST_OPER+  2)
#define FUNC_sin	(LAST_OPER+  3)
#define FUNC_cos	(LAST_OPER+  4)
#define FUNC_tan	(LAST_OPER+  5)
#define FUNC_ln		(LAST_OPER+  6)
#define FUNC_log10	(LAST_OPER+  7)
#define FUNC_min	(LAST_OPER+  8)
#define FUNC_max	(LAST_OPER+  9)
#define FUNC_floor	(LAST_OPER+ 10)
#define FUNC_ceil	(LAST_OPER+ 11)
#define FUNC_trunc	(LAST_OPER+ 12)
#define FUNC_rint	(LAST_OPER+ 13)
#define FUNC_exp	(LAST_OPER+ 14)
#define FUNC_tanh	(LAST_OPER+ 15)
#define FUNC_sinh	(LAST_OPER+ 16)
#define FUNC_cosh	(LAST_OPER+ 17)
#define FUNC_acos	(LAST_OPER+ 18)
#define FUNC_asin	(LAST_OPER+ 19)
#define FUNC_atan	(LAST_OPER+ 20)
#define FUNC_atan2	(LAST_OPER+ 21)
#define FUNC_mag	(LAST_OPER+ 22)
#define FUNC_norm	(LAST_OPER+ 23)
#define FUNC_abs	(LAST_OPER+ 24)
#define FUNC_int	(LAST_OPER+ 25)
#define FUNC_float	(LAST_OPER+ 26)
#define FUNC_real	(LAST_OPER+ 27)
#define FUNC_imag	(LAST_OPER+ 28)
#define FUNC_complex	(LAST_OPER+ 29)
#define FUNC_imagi	(LAST_OPER+ 30)
#define FUNC_imagj	(LAST_OPER+ 31)
#define FUNC_imagk	(LAST_OPER+ 32)
#define FUNC_quaternion	(LAST_OPER+ 33)
#define FUNC_sign	(LAST_OPER+ 34)
#define FUNC_qsin	(LAST_OPER+ 35)
#define FUNC_qcos	(LAST_OPER+ 36)
#define FUNC_char	(LAST_OPER+ 37)
#define FUNC_short	(LAST_OPER+ 38)
#define FUNC_double	(LAST_OPER+ 39)
#define FUNC_signed_int	(LAST_OPER+ 40)
#define FUNC_rank	(LAST_OPER+ 41)
#define FUNC_shape	(LAST_OPER+ 42)
#define FUNC_and	(LAST_OPER+ 43)
#define FUNC_or		(LAST_OPER+ 44)
#define FUNC_xor	(LAST_OPER+ 45)
#define FUNC_not	(LAST_OPER+ 46)
#define FUNC_random	(LAST_OPER+ 47)
#define FUNC_arg	(LAST_OPER+ 48)
#define FUNC_invalid	(LAST_OPER+ 49)
#define FUNC_sbyte	(LAST_OPER+ 50)
#define FUNC_uint	(LAST_OPER+ 51)
#define FUNC_ushort	(LAST_OPER+ 52)
#define FUNC_strcmp	(LAST_OPER+ 53)
#define FUNC_stricmp	(LAST_OPER+ 54)
#define FUNC_strlen	(LAST_OPER+ 57)
#define FUNC_strstr	(LAST_OPER+ 58)
#define FUNC_stristr	(LAST_OPER+ 59)
#define FUNC_finite	(LAST_OPER+ 60)
#define FUNC_isnan	(LAST_OPER+ 61)


typedef struct {
    float realPart;
    float iPart;
    float jPart;
    float kPart;
} quaternionFloat;
typedef struct {
    int realPart;
    int iPart;
    int jPart;
    int kPart;
} quaternionInt;

typedef struct {
    char realPart;
    char imagPart;
} complexChar;
typedef struct {
    short realPart;
    short imagPart;
} complexShort;
typedef struct {
    int realPart;
    int imagPart;
} complexInt;
typedef struct {
    float realPart;
    float imagPart;
} complexFloat;
typedef struct {
    double realPart;
    double imagPart;
} complexDouble;

#define REAL(x) ((x).realPart)
#define IMAG(x) ((x).imagPart)
#define IMAGI(x) ((x).iPart)
#define IMAGJ(x) ((x).jPart)
#define IMAGK(x) ((x).kPart)

int
_dxfComputeCopy(
    PTreeNode *pt, 
    ObjStruct *os, 
    int numInputs,
    Pointer *inputs,
    Pointer result,
    InvalidComponentHandle *invalids,
    InvalidComponentHandle outInvalid);

/* A set of macros to build generic routines.  BINOP builds
 * a routine called `name', that takes inputs of type `t0', and `t1', and
 * returns an output of type `t1' after performing `op' on the inputs.
 * Similarly is UNOP (unary operator).  Also, VBINOP (and the others) 
 * are define which * work on similarly shaped, non-scalar, inputs and 
 * output on an element by element basis.  These macros can be used for 
 * many of the given operators.
 *
 * Note that in each case, the size of an input array will be either
 * 1 or the size of the output array.  If all input arrays are of size 1,
 * then the size of the output array will also be 1.
 */
#define BINOP(name, tr, t0, t1, op)		\
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
    register tr *out = (tr *) result;		\
    register t0 *in0 = (t0 *) inputs[0];	\
    register t1 *in1 = (t1 *) inputs[1];	\
    int nelts = pt->metaType.items;		\
    int size0 = pt->args->metaType.items;	\
    int size1 = pt->args->next->metaType.items;	\
    int i;					\
    int allValid = 				\
	(invalids[0] == NULL || DXGetInvalidCount(invalids[0]) == 0) && \
	(invalids[1] == NULL || DXGetInvalidCount(invalids[1]) == 0);	\
						\
    if (size0 > 1 && size1 > 1)	{		\
	for (i = 0; i < nelts; ++i) {		\
	    if (allValid || _dxfComputeInvalidOr(outInvalid, i, invalids[0], i, invalids[1], i)) \
		out[i] = in0[i] op in1[i];	\
	}					\
    }						\
    else if (size0 > 1) {			\
	for (i = 0; i < nelts; ++i) {		\
	    if (allValid || _dxfComputeInvalidOr(outInvalid, i, invalids[0], i, invalids[1], 0)) \
		out[i] = in0[i] op in1[0];	\
	}					\
    }						\
    else if (size1 > 1) {			\
	for (i = 0; i < nelts; ++i) {		\
	    if (allValid || _dxfComputeInvalidOr(outInvalid, i, invalids[0], 0, invalids[1], i)) \
		out[i] = in0[0] op in1[i];	\
	}					\
    }						\
    else {					\
	if (allValid || _dxfComputeInvalidOr(outInvalid, 0, invalids[0], 0, invalids[1], 0)) \
	    out[0] = in0[0] op in1[0];		\
    }						\
    return (OK);				\
}

#define UNOP(name, tr, t0, op)			\
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
    register tr *out = (tr *) result;		\
    register t0 *in0 = (t0 *) inputs[0];	\
    int nelts = pt->metaType.items;		\
    int i;					\
    int allValid = 				\
	(invalids[0] == NULL || DXGetInvalidCount(invalids[0]) == 0); \
						\
    for (i = 0; i < nelts; ++i) {		\
	if (allValid || _dxfComputeInvalidOne(outInvalid, i, invalids[0], i)) \
	    out[i] = op in0[i];			\
    }						\
    return (OK);				\
}

#define VBINOP(name, tr, t0, t1, op)		\
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
    register tr *out = (tr *) result;		\
    register t0 *in0 = (t0 *) inputs[0];	\
    register t1 *in1 = (t1 *) inputs[1];	\
    int nelts = pt->metaType.items;		\
    int size0 = pt->args->metaType.items;	\
    int size1 = pt->args->next->metaType.items;	\
    int i, j;					\
    int numBasic;				\
    int allValid = 				\
	(invalids[0] == NULL || DXGetInvalidCount(invalids[0]) == 0) && \
	(invalids[1] == NULL || DXGetInvalidCount(invalids[1]) == 0);	\
						\
    for (numBasic = 1, i = 0; 			\
	    i < pt->metaType.rank;		\
	    ++i)				\
	numBasic *= pt->metaType.shape[i];	\
						\
    if (size0 > 1 && size1 > 1) {		\
	for (i = 0; i < nelts; ++i)		\
	    if (allValid || _dxfComputeInvalidOr(outInvalid, i, invalids[0], i, invalids[1], i)) \
		for (j = 0; j < numBasic; ++j)	\
		    out[i*numBasic+j] = in0[i*numBasic+j] op in1[i*numBasic+j];\
    }						\
    else if (size0 > 1) {			\
	for (i = 0; i < nelts; ++i)		\
	    if (allValid || _dxfComputeInvalidOr(outInvalid, i, invalids[0], i, invalids[1], 0)) \
		for (j = 0; j < numBasic; ++j)	\
		    out[j + i * numBasic] = in0[j + i * numBasic] op in1[j]; \
    }						\
    else if (size1 > 1) {			\
	for (i = 0; i < nelts; ++i)		\
	    if (allValid || _dxfComputeInvalidOr(outInvalid, i, invalids[0], 0, invalids[1], i)) \
		for (j = 0; j < numBasic; ++j)	\
		    out[j + i * numBasic] = in0[j] op in1[j + i * numBasic]; \
    }						\
    else {					\
	if (allValid || _dxfComputeInvalidOr(outInvalid, 0, invalids[0], 0, invalids[1], 0)) \
	    for (j = 0; j < numBasic; ++j)	\
		out[j] = in0[j] op in1[j];	\
    }						\
    return (OK);				\
}

#define VSBINOP(name, tr, t0, t1, op)		\
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
    register tr *out = (tr *) result;		\
    register t0 *in0 = (t0 *) inputs[0];	\
    register t1 *in1 = (t1 *) inputs[1];	\
    int nelts = pt->metaType.items;		\
    int size0 = pt->args->metaType.items;	\
    int size1 = pt->args->next->metaType.items;	\
    int i, j;					\
    int numBasic;				\
    int allValid = 				\
	(invalids[0] == NULL || DXGetInvalidCount(invalids[0]) == 0) && \
	(invalids[1] == NULL || DXGetInvalidCount(invalids[1]) == 0);	\
						\
    for (numBasic = 1, i = 0; 			\
	    i < pt->metaType.rank;		\
	    ++i)				\
	numBasic *= pt->metaType.shape[i];	\
						\
    if (size0 > 1 && size1 > 1) {	\
	for (i = 0; i < nelts; ++i)		\
	    if (allValid || _dxfComputeInvalidOr(outInvalid, i, invalids[0], i, invalids[1], i)) \
		for (j = 0; j < numBasic; ++j)	\
		    out[i * numBasic + j] = in0[i * numBasic + j] op in1[i]; \
    }						\
    else if (size0 > 1) {		\
	for (i = 0; i < nelts; ++i)		\
	    if (allValid || _dxfComputeInvalidOr(outInvalid, i, invalids[0], i, invalids[1], 0)) \
		for (j = 0; j < numBasic; ++j)	\
		    out[i * numBasic + j] = in0[i * numBasic + j] op in1[0]; \
    }						\
    else if (size1 > 1) {		\
	for (i = 0; i < nelts; ++i)		\
	    if (allValid || _dxfComputeInvalidOr(outInvalid, i, invalids[0], 0, invalids[1], i)) \
		for (j = 0; j < numBasic; ++j)	\
		    out[i * numBasic + j] = in0[j] op in1[i]; \
    }						\
    else {					\
	if (allValid || _dxfComputeInvalidOr(outInvalid, 0, invalids[0], 0, invalids[1], 0)) \
	    for (j = 0; j < numBasic; ++j)	\
		out[j] = in0[j] op in1[0];	\
    }						\
    return (OK);				\
}

#define SVBINOP(name, tr, t0, t1, op)		\
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
    register tr *out = (tr *) result;		\
    register t0 *in0 = (t0 *) inputs[0];	\
    register t1 *in1 = (t1 *) inputs[1];	\
    int nelts = pt->metaType.items;		\
    int size0 = pt->args->metaType.items;	\
    int size1 = pt->args->next->metaType.items;	\
    int i, j;					\
    int numBasic;				\
    int allValid = 				\
	(invalids[0] == NULL || DXGetInvalidCount(invalids[0]) == 0) && \
	(invalids[1] == NULL || DXGetInvalidCount(invalids[1]) == 0);	\
						\
    for (numBasic = 1, i = 0; 			\
	    i < pt->metaType.rank;		\
	    ++i)				\
	numBasic *= pt->metaType.shape[i];	\
						\
    if (size0 > 1 && size1 > 1) {		\
	for (i = 0; i < nelts; ++i)		\
	    if (allValid || _dxfComputeInvalidOr(outInvalid, i, invalids[0], i, invalids[1], i)) \
		for (j = 0; j < numBasic; ++j)	\
		    out[i * numBasic + j] = in0[i] op in1[i * numBasic + j]; \
    }						\
    else if (size0 > 1) {			\
	for (i = 0; i < nelts; ++i)		\
	    if (allValid || _dxfComputeInvalidOr(outInvalid, i, invalids[0], i, invalids[1], 0)) \
		for (j = 0; j < numBasic; ++j)	\
		    out[i * numBasic + j] = in0[i] op in1[j]; \
    }						\
    else if (size1 > 1) {			\
	for (i = 0; i < nelts; ++i)		\
	    if (allValid || _dxfComputeInvalidOr(outInvalid, i, invalids[0], 0, invalids[1], i)) \
		for (j = 0; j < numBasic; ++j)	\
		    out[i * numBasic + j] = in0[0] op in1[i * numBasic + j]; \
    }						\
    else {					\
	if (allValid || _dxfComputeInvalidOr(outInvalid, 0, invalids[0], 0, invalids[1], 0)) \
	    for (j = 0; j < numBasic; ++j)	\
		out[j] = in0[0] op in1[j];	\
    }						\
    return (OK);				\
}

#define VBINOPRC(name, tr, t0, t1, op, range, msg)		\
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
    register tr *out = (tr *) result;		\
    register t0 *in0 = (t0 *) inputs[0];	\
    register t1 *in1 = (t1 *) inputs[1];	\
    int nelts = pt->metaType.items;		\
    int size0 = pt->args->metaType.items;	\
    int size1 = pt->args->next->metaType.items;	\
    int i, j;					\
    int numBasic;				\
    int allValid = 				\
	(invalids[0] == NULL || DXGetInvalidCount(invalids[0]) == 0) && \
	(invalids[1] == NULL || DXGetInvalidCount(invalids[1]) == 0);	\
						\
    for (numBasic = 1, i = 0; 			\
	    i < pt->metaType.rank;		\
	    ++i)				\
	numBasic *= pt->metaType.shape[i];	\
						\
    if (size0 > 1 && size1 > 1) {		\
	for (i = 0; i < nelts; ++i)	\
	    if (allValid || _dxfComputeInvalidOr(outInvalid, i, invalids[0], i, invalids[1], i)) \
		for (j = 0; j < numBasic; ++j)	\
		    if (!range(in0[i*numBasic+j], in1[i*numBasic+j]))	\
		    {				\
			DXSetError(ERROR_BAD_PARAMETER, msg, i, in0[i*numBasic+j], in1[i*numBasic+j]); \
			return ERROR;		\
		    }				\
    }						\
    else if (size0 > 1) {			\
	for (i = 0; i < nelts; ++i)		\
	    if (allValid || _dxfComputeInvalidOr(outInvalid, i, invalids[0], i, invalids[1], 0)) \
		for (j = 0; j < numBasic; ++j)	\
		    if (!range(in0[j + i * numBasic], in1[j])) \
		    {				\
			DXSetError(ERROR_BAD_PARAMETER, msg, i, in0[j + i * numBasic], in1[j]); \
			return ERROR;		\
		    }				\
    }						\
    else if (size1 > 1) {			\
	for (i = 0; i < nelts; ++i)		\
	    if (allValid || _dxfComputeInvalidOr(outInvalid, i, invalids[0], 0, invalids[1], i)) \
		for (j = 0; j < numBasic; ++j)	\
		    if (!range(in0[j], in1[j + i * numBasic])) \
		    {				\
			DXSetError(ERROR_BAD_PARAMETER, msg, i, in0[j], in1[j + i * numBasic]); \
			return ERROR;		\
		    }				\
    }						\
    else {					\
	if (allValid || _dxfComputeInvalidOr(outInvalid, 0, invalids[0], 0, invalids[1], 0)) \
	    for (j = 0; j < numBasic; ++j)	\
		if (!range(in0[j], in1[j]))	\
		{				\
		    DXSetError(ERROR_BAD_PARAMETER, msg, 0, in0[j], in1[j]); \
		    return ERROR;		\
		}				\
    }						\
						\
    if (size0 > 1 && size1 > 1) {		\
	for (i = 0; i < nelts; ++i)	\
	    if (allValid || _dxfComputeInvalidOr(outInvalid, i, invalids[0], i, invalids[1], i)) \
		for (j = 0; j < numBasic; ++j)	\
		    out[i*numBasic+j] = in0[i*numBasic+j] op in1[i*numBasic+j];\
    }						\
    else if (size0 > 1) {			\
	for (i = 0; i < nelts; ++i)		\
	    if (allValid || _dxfComputeInvalidOr(outInvalid, i, invalids[0], i, invalids[1], 0)) \
		for (j = 0; j < numBasic; ++j)	\
		    out[j + i * numBasic] = in0[j + i * numBasic] op in1[j]; \
    }						\
    else if (size1 > 1) {			\
	for (i = 0; i < nelts; ++i)		\
	    if (allValid || _dxfComputeInvalidOr(outInvalid, i, invalids[0], 0, invalids[1], i)) \
		for (j = 0; j < numBasic; ++j)	\
		    out[j + i * numBasic] = in0[j] op in1[j + i * numBasic]; \
    }						\
    else {					\
	if (allValid || _dxfComputeInvalidOr(outInvalid, 0, invalids[0], 0, invalids[1], 0)) \
	    for (j = 0; j < numBasic; ++j)		\
		out[j] = in0[j] op in1[j];		\
    }						\
    return (OK);				\
}

#define VSBINOPRC(name, tr, t0, t1, op, range, msg)		\
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
    register tr *out = (tr *) result;		\
    register t0 *in0 = (t0 *) inputs[0];	\
    register t1 *in1 = (t1 *) inputs[1];	\
    int nelts = pt->metaType.items;		\
    int size0 = pt->args->metaType.items;	\
    int size1 = pt->args->next->metaType.items;	\
    int i, j;					\
    int numBasic;				\
    int allValid = 				\
	(invalids[0] == NULL || DXGetInvalidCount(invalids[0]) == 0) && \
	(invalids[1] == NULL || DXGetInvalidCount(invalids[1]) == 0);	\
						\
    for (numBasic = 1, i = 0; 			\
	    i < pt->metaType.rank;		\
	    ++i)				\
	numBasic *= pt->metaType.shape[i];	\
						\
    if (size0 > 1 && size1 > 1) {	\
	for (i = 0; i < nelts; ++i)		\
	    if (allValid || _dxfComputeInvalidOr(outInvalid, i, invalids[0], i, invalids[1], i)) \
		for (j = 0; j < numBasic; ++j)	\
		    if (!range(in0[i * numBasic + j], in1[i]))	\
		    {					\
			DXSetError(ERROR_BAD_PARAMETER, msg, i, in0[i * numBasic + j], in1[i]); \
			return ERROR;			\
		    }					\
    }						\
    else if (size0 > 1) {		\
	for (i = 0; i < nelts; ++i)		\
	    if (allValid || _dxfComputeInvalidOr(outInvalid, i, invalids[0], i, invalids[1], 0)) \
		for (j = 0; j < numBasic; ++j)	\
		    if (!range(in0[i * numBasic + j], in1[0]))	\
		    {					\
			DXSetError(ERROR_BAD_PARAMETER, msg, i, in0[i * numBasic + j], in1[0]); \
			return ERROR;			\
		    }					\
    }						\
    else if (size1 > 1) {		\
	for (i = 0; i < nelts; ++i)		\
	    if (allValid || _dxfComputeInvalidOr(outInvalid, i, invalids[0], 0, invalids[1], i)) \
		for (j = 0; j < numBasic; ++j)	\
		    if (!range(in0[j], in1[i]))	\
		    {					\
			DXSetError(ERROR_BAD_PARAMETER, msg, i, in0[j], in1[i]); \
			return ERROR;			\
		    }					\
    }						\
    else {					\
	if (allValid || _dxfComputeInvalidOr(outInvalid, 0, invalids[0], 0, invalids[1], 0)) \
	    for (j = 0; j < numBasic; ++j)		\
		if (!range(in0[j], in1[0]))		\
		{					\
		    DXSetError(ERROR_BAD_PARAMETER, msg, 0, in0[j], in1[0]); \
		    return ERROR;			\
		}					\
    }						\
						\
    if (size0 > 1 && size1 > 1) {	\
	for (i = 0; i < nelts; ++i)		\
	    if (allValid || _dxfComputeInvalidOr(outInvalid, i, invalids[0], i, invalids[1], i)) \
		for (j = 0; j < numBasic; ++j)	\
		    out[i * numBasic + j] = in0[i * numBasic + j] op in1[i]; \
    }						\
    else if (size0 > 1) {		\
	for (i = 0; i < nelts; ++i)		\
	    if (allValid || _dxfComputeInvalidOr(outInvalid, i, invalids[0], i, invalids[1], 0)) \
		for (j = 0; j < numBasic; ++j)	\
		    out[i * numBasic + j] = in0[i * numBasic + j] op in1[0]; \
    }						\
    else if (size1 > 1) {		\
	for (i = 0; i < nelts; ++i)		\
	    if (allValid || _dxfComputeInvalidOr(outInvalid, i, invalids[0], 0, invalids[1], i)) \
		for (j = 0; j < numBasic; ++j)	\
		    out[i * numBasic + j] = in0[j] op in1[i]; \
    }						\
    else {					\
	if (allValid || _dxfComputeInvalidOr(outInvalid, 0, invalids[0], 0, invalids[1], 0)) \
	    for (j = 0; j < numBasic; ++j)		\
		out[j] = in0[j] op in1[0];		\
    }						\
    return (OK);				\
}

#define SVBINOPRC(name, tr, t0, t1, op, range, msg)		\
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
    register tr *out = (tr *) result;		\
    register t0 *in0 = (t0 *) inputs[0];	\
    register t1 *in1 = (t1 *) inputs[1];	\
    int nelts = pt->metaType.items;		\
    int size0 = pt->args->metaType.items;	\
    int size1 = pt->args->next->metaType.items;	\
    int i, j;					\
    int numBasic;				\
    int allValid = 				\
	(invalids[0] == NULL || DXGetInvalidCount(invalids[0]) == 0) && \
	(invalids[1] == NULL || DXGetInvalidCount(invalids[1]) == 0);	\
						\
    for (numBasic = 1, i = 0; 			\
	    i < pt->metaType.rank;		\
	    ++i)				\
	numBasic *= pt->metaType.shape[i];	\
						\
    if (size0 > 1 && size1 > 1) {		\
	for (i = 0; i < nelts; ++i)		\
	    if (allValid || _dxfComputeInvalidOr(outInvalid, i, invalids[0], i, invalids[1], i)) \
		for (j = 0; j < numBasic; ++j)	\
		    if (!range(in0[i], in1[i * numBasic + j]))	\
		    {					\
			DXSetError(ERROR_BAD_PARAMETER, msg, i, in0[i], in1[i * numBasic + j]); \
			return ERROR;			\
		    }					\
    }						\
    else if (size0 > 1) {			\
	for (i = 0; i < nelts; ++i)		\
	    if (allValid || _dxfComputeInvalidOr(outInvalid, i, invalids[0], i, invalids[1], 0)) \
		for (j = 0; j < numBasic; ++j)	\
		    if (!range(in0[i], in1[j]))	\
		    {					\
			DXSetError(ERROR_BAD_PARAMETER, msg, i, in0[i], in1[j]); \
			return ERROR;			\
		    }					\
    }						\
    else if (size1 > 1) {			\
	for (i = 0; i < nelts; ++i)		\
	    if (allValid || _dxfComputeInvalidOr(outInvalid, i, invalids[0], 0, invalids[1], i)) \
		for (j = 0; j < numBasic; ++j)	\
		    if (!range(in0[0], in1[i * numBasic + j]))	\
		    {					\
			DXSetError(ERROR_BAD_PARAMETER, msg, i, in0[0], in1[i * numBasic + j]); \
			return ERROR;			\
		    }					\
    }						\
    else {					\
	if (allValid || _dxfComputeInvalidOr(outInvalid, 0, invalids[0], 0, invalids[1], 0)) \
	    for (j = 0; j < numBasic; ++j)		\
		if (!range(in0[0], in1[j]))		\
		{					\
		    DXSetError(ERROR_BAD_PARAMETER, msg, 0, in0[0], in1[j]); \
		    return ERROR;			\
		}					\
    }						\
						\
    if (size0 > 1 && size1 > 1) {		\
	for (i = 0; i < nelts; ++i)		\
	    if (allValid || _dxfComputeInvalidOr(outInvalid, i, invalids[0], i, invalids[1], i)) \
		for (j = 0; j < numBasic; ++j)	\
		    out[i * numBasic + j] = in0[i] op in1[i * numBasic + j]; \
    }						\
    else if (size0 > 1) {			\
	for (i = 0; i < nelts; ++i)		\
	    if (allValid || _dxfComputeInvalidOr(outInvalid, i, invalids[0], i, invalids[1], 0)) \
		for (j = 0; j < numBasic; ++j)	\
		    out[i * numBasic + j] = in0[i] op in1[j]; \
    }						\
    else if (size1 > 1) {			\
	for (i = 0; i < nelts; ++i)		\
	    if (allValid || _dxfComputeInvalidOr(outInvalid, i, invalids[0], 0, invalids[1], i)) \
		for (j = 0; j < numBasic; ++j)	\
		    out[i * numBasic + j] = in0[0] op in1[i * numBasic + j]; \
    }						\
    else {					\
	if (allValid || _dxfComputeInvalidOr(outInvalid, 0, invalids[0], 0, invalids[1], 0)) \
	    for (j = 0; j < numBasic; ++j)		\
		out[j] = in0[0] op in1[j];		\
    }						\
    return (OK);				\
}

#define VUNOP(name, tr, t0, op)			\
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
    register tr *out = (tr *) result;		\
    register t0 *in0 = (t0 *) inputs[0];	\
    int i,j;					\
    int numBasic;				\
    int items;					\
    int allValid = 				\
	(invalids[0] == NULL || DXGetInvalidCount(invalids[0]) == 0); \
						\
    for (numBasic = 1, i = 0; 			\
	    i < pt->metaType.rank;		\
	    ++i)				\
	numBasic *= pt->metaType.shape[i];	\
						\
    items = pt->metaType.items;			\
    for (i = 0; i < items; ++i) {		\
	if (allValid || _dxfComputeInvalidOne(outInvalid, i, invalids[0], i)) \
	for (j = 0; j < numBasic; ++j) {	\
	    out[i*numBasic+j] = op in0[i*numBasic+j];	\
	}					\
    }						\
    return (OK);				\
}

#define VFUNC2(name, tr, t0, t1, op)		\
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
    register tr *out = (tr *) result;		\
    register t0 *in0 = (t0 *) inputs[0];	\
    register t1 *in1 = (t1 *) inputs[1];	\
    int nelts = pt->metaType.items;		\
    int size0 = pt->args->metaType.items;	\
    int size1 = pt->args->next->metaType.items;	\
    int i, j;					\
    int numBasic;				\
    int allValid = 				\
	(invalids[0] == NULL || DXGetInvalidCount(invalids[0]) == 0) && \
	(invalids[1] == NULL || DXGetInvalidCount(invalids[1]) == 0);	\
						\
    for (numBasic = 1, i = 0; 			\
	    i < pt->metaType.rank;		\
	    ++i)				\
	numBasic *= pt->metaType.shape[i];	\
						\
    if (size0 > 1 && size1 > 1) {	\
	for (i = 0; i < nelts; ++i)		\
	    if (allValid || _dxfComputeInvalidOr(outInvalid, i, invalids[0], i, invalids[1], i)) \
		for (j = 0; j < numBasic; ++j)	\
		    out[i*numBasic+j] = op(in0[i*numBasic+j], in1[i*numBasic+j]);\
    }						\
    else if (size0 > 1) {		\
	for (i = 0; i < nelts; ++i)		\
	    if (allValid || _dxfComputeInvalidOr(outInvalid, i, invalids[0], i, invalids[1], 0)) \
		for (j = 0; j < numBasic; ++j)	\
		    out[j + i * numBasic] = op(in0[j + i * numBasic], in1[j]); \
    }						\
    else if (size1 > 1) {		\
	for (i = 0; i < nelts; ++i)		\
	    if (allValid || _dxfComputeInvalidOr(outInvalid, i, invalids[0], 0, invalids[1], i)) \
		for (j = 0; j < numBasic; ++j)	\
		    out[j + i * numBasic] = op(in0[j], in1[j + i * numBasic]); \
    }						\
    else {					\
	if (allValid || _dxfComputeInvalidOr(outInvalid, 0, invalids[0], 0, invalids[1], 0)) \
	    for (j = 0; j < numBasic; ++j)		\
		out[j] = op(in0[j], in1[j]);	\
    }						\
    return (OK);				\
}

#define VSFUNC2(name, tr, t0, t1, op)		\
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
    register tr *out = (tr *) result;		\
    register t0 *in0 = (t0 *) inputs[0];	\
    register t1 *in1 = (t1 *) inputs[1];	\
    int nelts = pt->metaType.items;		\
    int size0 = pt->args->metaType.items;	\
    int size1 = pt->args->next->metaType.items;	\
    int i, j;					\
    int numBasic;				\
    int allValid = 				\
	(invalids[0] == NULL || DXGetInvalidCount(invalids[0]) == 0) && \
	(invalids[1] == NULL || DXGetInvalidCount(invalids[1]) == 0);	\
						\
    for (numBasic = 1, i = 0; 			\
	    i < pt->metaType.rank;		\
	    ++i)				\
	numBasic *= pt->metaType.shape[i];	\
						\
    if (size0 > 1 && size1 > 1) {		\
	for (i = 0; i < nelts; ++i)		\
	    if (allValid || _dxfComputeInvalidOr(outInvalid, i, invalids[0], i, invalids[1], i)) \
		for (j = 0; j < numBasic; ++j)	\
		    out[i * numBasic + j] = op(in0[i * numBasic + j], in1[i]); \
    }						\
    else if (size0 > 1) {			\
	for (i = 0; i < nelts; ++i)		\
	    if (allValid || _dxfComputeInvalidOr(outInvalid, i, invalids[0], i, invalids[1], 0)) \
		for (j = 0; j < numBasic; ++j)	\
		    out[i * numBasic + j] = op(in0[i * numBasic + j], in1[0]); \
    }						\
    else if (size1 > 1) {			\
	for (j = 0; j < numBasic; ++j)		\
	    if (allValid || _dxfComputeInvalidOr(outInvalid, i, invalids[0], 0, invalids[1], i)) \
		for (i = 0; i < nelts; ++i)		\
		    out[i * numBasic + j] = op(in0[j], in1[i]); \
    }						\
    else {					\
	if (allValid || _dxfComputeInvalidOr(outInvalid, 0, invalids[0], 0, invalids[1], 0)) \
	    for (j = 0; j < numBasic; ++j)		\
		out[j] = op(in0[j], in1[0]);	\
    }						\
    return (OK);				\
}

#define SVFUNC2(name, tr, t0, t1, op)		\
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
    register tr *out = (tr *) result;		\
    register t0 *in0 = (t0 *) inputs[0];	\
    register t1 *in1 = (t1 *) inputs[1];	\
    int nelts = pt->metaType.items;		\
    int size0 = pt->args->metaType.items;	\
    int size1 = pt->args->next->metaType.items;	\
    int i, j;					\
    int numBasic;				\
    int allValid = 				\
	(invalids[0] == NULL || DXGetInvalidCount(invalids[0]) == 0) && \
	(invalids[1] == NULL || DXGetInvalidCount(invalids[1]) == 0);	\
						\
    for (numBasic = 1, i = 0; 			\
	    i < pt->metaType.rank;		\
	    ++i)				\
	numBasic *= pt->metaType.shape[i];	\
						\
    if (size0 > 1 && size1 > 1) {		\
	for (i = 0; i < nelts; ++i)		\
	    if (allValid || _dxfComputeInvalidOr(outInvalid, i, invalids[0], i, invalids[1], i)) \
		for (j = 0; j < numBasic; ++j)	\
		    out[i * numBasic + j] = op(in0[i], in1[i * numBasic + j]); \
    }						\
    else if (size0 > 1) {			\
	for (i = 0; i < nelts; ++i)		\
	    if (allValid || _dxfComputeInvalidOr(outInvalid, i, invalids[0], i, invalids[1], 0)) \
		for (j = 0; j < numBasic; ++j)	\
		    out[i * numBasic + j] = op(in0[i], in1[j]); \
    }						\
    else if (size1 > 1) {			\
	for (j = 0; j < numBasic; ++j)		\
	    if (allValid || _dxfComputeInvalidOr(outInvalid, i, invalids[0], 0, invalids[1], i)) \
		for (i = 0; i < nelts; ++i)		\
		    out[i * numBasic + j] = op(in0[0], in1[i * numBasic + j]); \
    }						\
    else {					\
	if (allValid || _dxfComputeInvalidOr(outInvalid, 0, invalids[0], 0, invalids[1], 0)) \
	    for (j = 0; j < numBasic; ++j)		\
		out[j] = op(in0[0], in1[j]);	\
    }						\
						\
    return (OK);				\
}


#define VFUNC2RC(name, tr, t0, t1, op, range, msg)	\
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
    register tr *out = (tr *) result;		\
    register t0 *in0 = (t0 *) inputs[0];	\
    register t1 *in1 = (t1 *) inputs[1];	\
    int nelts = pt->metaType.items;		\
    int size0 = pt->args->metaType.items;	\
    int size1 = pt->args->next->metaType.items;	\
    int i, j;					\
    int numBasic;				\
    int allValid = 				\
	(invalids[0] == NULL || DXGetInvalidCount(invalids[0]) == 0) && \
	(invalids[1] == NULL || DXGetInvalidCount(invalids[1]) == 0);	\
						\
    for (numBasic = 1, i = 0; 			\
	    i < pt->metaType.rank;		\
	    ++i)				\
	numBasic *= pt->metaType.shape[i];	\
						\
    if (size0 > 1 && size1 > 1) {	\
	for (i = 0; i < nelts; ++i)	\
	    if (allValid || _dxfComputeInvalidOr(outInvalid, i, invalids[0], i, invalids[1], i)) \
		for (j = 0; j < numBasic; ++j)	\
		    if (!range(in0[i*numBasic+j], in1[i*numBasic+j])) \
		    {					\
			DXSetError(ERROR_BAD_PARAMETER,msg,i, in0[i*numBasic+j], in1[i*numBasic+j]);\
			return (ERROR);			\
		    }					\
    }						\
    else if (size0 > 1) {		\
	for (i = 0; i < nelts; ++i)		\
	    if (allValid || _dxfComputeInvalidOr(outInvalid, i, invalids[0], i, invalids[1], 0)) \
		for (j = 0; j < numBasic; ++j)	\
		    if (!range(in0[j+i*numBasic], in1[j]))		\
		    {					\
			DXSetError(ERROR_BAD_PARAMETER,msg,i, in0[j+i*numBasic], in1[j]);\
			return (ERROR);			\
		    }					\
    }						\
    else if (size1 > 1) {		\
	for (i = 0; i < nelts; ++i)		\
	    if (allValid || _dxfComputeInvalidOr(outInvalid, i, invalids[0], 0, invalids[1], i)) \
		for (j = 0; j < numBasic; ++j)	\
		    if (!range(in0[j], in1[j+i*numBasic]))		\
		    {					\
			DXSetError(ERROR_BAD_PARAMETER,msg,i, in0[j], in1[j+i*numBasic]);\
			return (ERROR);			\
		    }					\
    }						\
    else {					\
	if (allValid || _dxfComputeInvalidOr(outInvalid, 0, invalids[0], 0, invalids[1], 0)) \
	    for (j = 0; j < numBasic; ++j)		\
		if (!range(in0[j], in1[j]))		\
		{					\
		    DXSetError(ERROR_BAD_PARAMETER,msg,0, in0[j], in1[j]);\
		    return (ERROR);			\
		}					\
    }						\
						\
    if (size0 > 1 && size1 > 1) {	\
	for (i = 0; i < nelts; ++i)	\
	    if (allValid || _dxfComputeInvalidOr(outInvalid, i, invalids[0], i, invalids[1], i)) \
		for (j = 0; j < numBasic; ++j)	\
		    out[i*numBasic+j] = op(in0[i*numBasic+j], in1[i*numBasic+j]);\
    }						\
    else if (size0 > 1) {		\
	for (i = 0; i < nelts; ++i)		\
	    if (allValid || _dxfComputeInvalidOr(outInvalid, i, invalids[0], i, invalids[1], 0)) \
		for (j = 0; j < numBasic; ++j)	\
		    out[j + i * numBasic] = op(in0[j + i * numBasic], in1[j]); \
    }						\
    else if (size1 > 1) {		\
	for (i = 0; i < nelts; ++i)		\
	    if (allValid || _dxfComputeInvalidOr(outInvalid, i, invalids[0], 0, invalids[1], i)) \
		for (j = 0; j < numBasic; ++j)	\
		    out[j + i * numBasic] = op(in0[j], in1[j + i * numBasic]); \
    }						\
    else {					\
	if (allValid || _dxfComputeInvalidOr(outInvalid, 0, invalids[0], 0, invalids[1], 0)) \
	    for (j = 0; j < numBasic; ++j)		\
		out[j] = op(in0[j], in1[j]);	\
    }						\
    return (OK);				\
}

#define VSFUNC2RC(name, tr, t0, t1, op, range,msg)	\
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
    register tr *out = (tr *) result;		\
    register t0 *in0 = (t0 *) inputs[0];	\
    register t1 *in1 = (t1 *) inputs[1];	\
    int nelts = pt->metaType.items;		\
    int size0 = pt->args->metaType.items;	\
    int size1 = pt->args->next->metaType.items;	\
    int i, j;					\
    int numBasic;				\
    int allValid = 				\
	(invalids[0] == NULL || DXGetInvalidCount(invalids[0]) == 0) && \
	(invalids[1] == NULL || DXGetInvalidCount(invalids[1]) == 0);	\
						\
    for (numBasic = 1, i = 0; 			\
	    i < pt->metaType.rank;		\
	    ++i)				\
	numBasic *= pt->metaType.shape[i];	\
						\
    if (size0 > 1 && size1 > 1) {		\
	for (i = 0; i < nelts; ++i)		\
	    if (allValid || _dxfComputeInvalidOr(outInvalid, i, invalids[0], i, invalids[1], i)) \
		for (j = 0; j < numBasic; ++j)	\
		    if (!range(in0[i*numBasic+j], in1[i]))		\
		    {					\
			DXSetError(ERROR_BAD_PARAMETER,msg,i, in0[i*numBasic+j], in1[i]);\
			return (ERROR);			\
		    }					\
    }						\
    else if (size0 > 1) {			\
	for (i = 0; i < nelts; ++i)		\
	    if (allValid || _dxfComputeInvalidOr(outInvalid, i, invalids[0], i, invalids[1], 0)) \
		for (j = 0; j < numBasic; ++j)	\
		    if (!range(in0[i * numBasic + j], in1[0]))		\
		    {					\
			DXSetError(ERROR_BAD_PARAMETER,msg,i, in0[i * numBasic + j], in1[0]);\
			return (ERROR);			\
		    }					\
    }						\
    else if (size1 > 1) {			\
	for (i = 0; i < nelts; ++i)		\
	    if (allValid || _dxfComputeInvalidOr(outInvalid, i, invalids[0], 0, invalids[1], i)) \
		for (j = 0; j < numBasic; ++j)		\
		    if (!range(in0[j], in1[i]))		\
		    {					\
			DXSetError(ERROR_BAD_PARAMETER,msg,i, in0[j], in1[i]);\
			return (ERROR);			\
		    }					\
    }						\
    else {					\
	if (allValid || _dxfComputeInvalidOr(outInvalid, 0, invalids[0], 0, invalids[1], 0)) \
	    for (j = 0; j < numBasic; ++j)		\
		if (!range(in0[j], in1[0]))		\
		{					\
		    DXSetError(ERROR_BAD_PARAMETER,msg,0, in0[j], in1[0]);\
		    return (ERROR);			\
		}					\
    }						\
						\
    if (size0 > 1 && size1 > 1) {		\
	for (i = 0; i < nelts; ++i)		\
	    if (allValid || _dxfComputeInvalidOr(outInvalid, i, invalids[0], i, invalids[1], i)) \
		for (j = 0; j < numBasic; ++j)	\
		    out[i * numBasic + j] = op(in0[i * numBasic + j], in1[i]); \
    }						\
    else if (size0 > 1) {			\
	for (i = 0; i < nelts; ++i)		\
	    if (allValid || _dxfComputeInvalidOr(outInvalid, i, invalids[0], i, invalids[1], 0)) \
		for (j = 0; j < numBasic; ++j)	\
		    out[i * numBasic + j] = op(in0[i * numBasic + j], in1[0]); \
    }						\
    else if (size1 > 1) {			\
	for (i = 0; i < nelts; ++i)		\
	    if (allValid || _dxfComputeInvalidOr(outInvalid, i, invalids[0], 0, invalids[1], i)) \
		for (j = 0; j < numBasic; ++j)		\
		    out[i * numBasic + j] = op(in0[j], in1[i]); \
    }						\
    else {					\
	if (allValid || _dxfComputeInvalidOr(outInvalid, 0, invalids[0], 0, invalids[1], 0)) \
	    for (j = 0; j < numBasic; ++j)		\
		out[j] = op(in0[j], in1[0]);	\
    }						\
    return (OK);				\
}

#define SVFUNC2RC(name, tr, t0, t1, op, range, msg)	\
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
    register tr *out = (tr *) result;		\
    register t0 *in0 = (t0 *) inputs[0];	\
    register t1 *in1 = (t1 *) inputs[1];	\
    int nelts = pt->metaType.items;		\
    int size0 = pt->args->metaType.items;	\
    int size1 = pt->args->next->metaType.items;	\
    int i, j;					\
    int numBasic;				\
    int allValid = 				\
	(invalids[0] == NULL || DXGetInvalidCount(invalids[0]) == 0) && \
	(invalids[1] == NULL || DXGetInvalidCount(invalids[1]) == 0);	\
						\
    for (numBasic = 1, i = 0; 			\
	    i < pt->metaType.rank;		\
	    ++i)				\
	numBasic *= pt->metaType.shape[i];	\
						\
    if (size0 > 1 && size1 > 1) {		\
	for (i = 0; i < nelts; ++i)		\
	    if (allValid || _dxfComputeInvalidOr(outInvalid, i, invalids[0], i, invalids[1], i)) \
		for (j = 0; j < numBasic; ++j)	\
		    if (!range(in0[i], in1[i * numBasic + j]))		\
		    {					\
			DXSetError(ERROR_BAD_PARAMETER,msg,i, in0[i], in1[i * numBasic + j]);\
			return (ERROR);			\
		    }					\
    }						\
    else if (size0 > 1) {			\
	for (i = 0; i < nelts; ++i)		\
	    if (allValid || _dxfComputeInvalidOr(outInvalid, i, invalids[0], i, invalids[1], 0)) \
		for (j = 0; j < numBasic; ++j)	\
		    if (!range(in0[i], in1[j]))		\
		    {					\
			DXSetError(ERROR_BAD_PARAMETER,msg,i, in0[i], in1[j]);\
			return (ERROR);			\
		    }					\
    }						\
    else if (size1 > 1) {			\
	for (i = 0; i < nelts; ++i)		\
	    if (allValid || _dxfComputeInvalidOr(outInvalid, i, invalids[0], 0, invalids[1], i)) \
		for (j = 0; j < numBasic; ++j)		\
		    if (!range(in0[0], in1[i * numBasic + j]))		\
		    {					\
			DXSetError(ERROR_BAD_PARAMETER,msg,i, in0[0], in1[i * numBasic + j]);\
			return (ERROR);			\
		    }					\
    }						\
    else {					\
	if (allValid || _dxfComputeInvalidOr(outInvalid, 0, invalids[0], 0, invalids[1], i)) \
	    for (j = 0; j < numBasic; ++j)		\
		if (!range(in0[0], in1[j]))		\
		{					\
		    DXSetError(ERROR_BAD_PARAMETER,msg,0, in0[0], in1[j]);\
		    return (ERROR);			\
		}					\
    }						\
						\
    if (size0 > 1 && size1 > 1) {		\
	for (i = 0; i < nelts; ++i)		\
	    if (allValid || _dxfComputeInvalidOr(outInvalid, i, invalids[0], i, invalids[1], i)) \
		for (j = 0; j < numBasic; ++j)	\
		    out[i * numBasic + j] = op(in0[i], in1[i * numBasic + j]); \
    }						\
    else if (size0 > 1) {			\
	for (i = 0; i < nelts; ++i)		\
	    if (allValid || _dxfComputeInvalidOr(outInvalid, i, invalids[0], i, invalids[1], 0)) \
		for (j = 0; j < numBasic; ++j)	\
		    out[i * numBasic + j] = op(in0[i], in1[j]); \
    }						\
    else if (size1 > 1) {			\
	for (i = 0; i < nelts; ++i)		\
	    if (allValid || _dxfComputeInvalidOr(outInvalid, i, invalids[0], 0, invalids[1], i)) \
		for (j = 0; j < numBasic; ++j)		\
		    out[i * numBasic + j] = op(in0[0], in1[i * numBasic + j]); \
    }						\
    else {					\
	if (allValid || _dxfComputeInvalidOr(outInvalid, 0, invalids[0], 0, invalids[1], 0)) \
	    for (j = 0; j < numBasic; ++j)		\
		out[j] = op(in0[0], in1[j]);	\
    }						\
						\
    return (OK);				\
}

#define VFUNC1(name, tr, t0, op)		\
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
    register tr *out = (tr *) result;		\
    register t0 *in0 = (t0 *) inputs[0];	\
    int i,j;					\
    int numBasic, items;			\
    int allValid = 				\
	(invalids[0] == NULL || DXGetInvalidCount(invalids[0]) == 0); \
						\
    for (numBasic = 1, i = 0; 			\
	    i < pt->metaType.rank;		\
	    ++i)				\
	numBasic *= pt->metaType.shape[i];	\
    numBasic *= DXCategorySize(pt->metaType.category); \
    items = pt->metaType.items;			\
    for (i = 0; i < items; ++i)			\
	if (allValid || _dxfComputeInvalidOne(outInvalid, i, invalids[0], i)) \
	    for (j = 0; j < numBasic; ++j)	\
		out[i*numBasic+j] = op (in0[i*numBasic+j]);		\
    return (OK);				\
}

#define VFUNC1RC(name, tr, t0, op, range, msg)	\
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
    register tr *out = (tr *) result;		\
    register t0 *in0 = (t0 *) inputs[0];	\
    int i, j;					\
    int numBasic, items;			\
    int allValid = 				\
	(invalids[0] == NULL || DXGetInvalidCount(invalids[0]) == 0); \
						\
    for (numBasic = 1, i = 0; 			\
	    i < pt->metaType.rank;		\
	    ++i)				\
	numBasic *= pt->metaType.shape[i];	\
    numBasic *= DXCategorySize(pt->metaType.category); \
    items = pt->metaType.items;			\
    for (i = 0; i < items; ++i) {		\
	if (allValid || _dxfComputeInvalidOne(outInvalid, i, invalids[0], i)) \
	    for (j = 0; j < numBasic; ++j) {	\
		if (!range(in0[i*numBasic+j]))	\
		{				\
		    DXSetError(ERROR_BAD_PARAMETER, msg, i, in0[i*numBasic+j]);\
		    return(ERROR);		\
		}				\
	    }					\
    }						\
    for (i = 0; i < items; ++i)			\
	if (allValid || _dxfComputeInvalidOne(outInvalid, i, invalids[0], i)) \
	    for (j = 0; j < numBasic; ++j)	\
		out[i*numBasic+j] = op (in0[i*numBasic+j]);		\
    return (OK);				\
}


#define VCATFUNC1(name, tr, t0, op)		\
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
    register tr *out = (tr *) result;		\
    register t0 *in0 = (t0 *) inputs[0];	\
    int i, j;					\
    int numBasic, items;			\
    int allValid = 				\
	(invalids[0] == NULL || DXGetInvalidCount(invalids[0]) == 0); \
						\
    for (numBasic = 1, i = 0; 			\
	    i < pt->metaType.rank;		\
	    ++i)				\
	numBasic *= pt->metaType.shape[i];	\
						\
    numBasic *= DXCategorySize(pt->metaType.category); \
    items = pt->metaType.items;			\
    for (i = 0; i < items; ++i)			\
	if (allValid || _dxfComputeInvalidOne(outInvalid, i, invalids[0], i)) \
	    for (j = 0; j < numBasic; ++j)		\
		out[i*numBasic+j] = op (in0[i*numBasic+j]);		\
    return (OK);				\
}

#define VCMP(name, tr, t0, t1, op, conjunction)		\
static int						\
name(							\
    PTreeNode *pt, 					\
    ObjStruct *os, 					\
    int numInputs,					\
    Pointer *inputs,					\
    Pointer result,					\
    InvalidComponentHandle *invalids,			\
    InvalidComponentHandle outInvalid)			\
{							\
    register int *out = (tr *) result;			\
							\
    int size0 = pt->args->metaType.items;		\
    int size1 = pt->args->next->metaType.items;		\
    int i, j, k, l;					\
    int numBasic;					\
    int size;						\
    int items;						\
    int allValid = 				\
	(invalids[0] == NULL || DXGetInvalidCount(invalids[0]) == 0) && \
	(invalids[1] == NULL || DXGetInvalidCount(invalids[1]) == 0);	\
							\
    for (numBasic = 1, i = 0; i < pt->args->metaType.rank; ++i) {	\
	numBasic *= pt->args->metaType.shape[i];		\
    }							\
    size = DXCategorySize (pt->args->metaType.category) *	\
	   numBasic;					\
							\
    items = pt->metaType.items;				\
    for (i = j = k = 0;					\
	    i < items;					\
	    ++i) {					\
	if (allValid || _dxfComputeInvalidOr(outInvalid, i, invalids[0], j, invalids[1], k)) {	\
	    out[i] = (*(((t0 *)inputs[0]) + j * size) op \
		 *(((t1 *)inputs[1]) + k * size));  \
	    for (l = 1; l < size; ++l)					\
		out[i] = out[i] conjunction		\
		    (*(((t0 *)inputs[0]) + j * size + l) op \
		     *(((t1 *)inputs[1]) + k * size + l));  \
	}						\
	++j, ++k;					\
	if (j >= size0) j = 0;				\
	if (k >= size1) k = 0;				\
    }							\
    return (OK);					\
}

#define VMULTIOP(name,tr,t,op,init)			\
static int						\
name(							\
    PTreeNode *pt, 					\
    ObjStruct *os, 					\
    int numInputs,					\
    Pointer *inputs,					\
    Pointer result,					\
    InvalidComponentHandle *invalids,			\
    InvalidComponentHandle outInvalid)			\
{							\
    register tr *out = (tr *) result;			\
    register t *in;					\
    PTreeNode *subTree;					\
    int size0;						\
    int i, j, arg;					\
    int anyValid;					\
    int items = pt->metaType.items;			\
    int *allValid;					\
							\
    allValid = (int*)DXAllocateLocal(sizeof(int) * numInputs); \
    if (allValid == NULL)				\
	return ERROR;					\
    for (arg = 0; arg < numInputs; ++arg) {		\
	allValid[arg] = invalids[arg] == NULL || 	\
	    DXGetInvalidCount(invalids[arg]) == 0;	\
    }							\
    /* for each element */				\
    for (i = j = 0; i < items; ++i) {			\
	subTree = pt->args;				\
	out[i] = init;					\
	anyValid = 0;					\
	for (arg = 0; arg < numInputs; ++arg) {		\
	    in = (t *) inputs[arg];			\
	    size0 = subTree->metaType.items;		\
	    if (size0 != 1)				\
		j = i;					\
	    else					\
		j = 0;					\
	    if (allValid[arg] || !DXIsElementInvalidSequential(invalids[arg], j)) { \
		anyValid = 1;				\
		out[i] = op (out[i], in[j]);		\
	    }						\
	    subTree = subTree->next;			\
	}						\
	if (!anyValid)					\
	    DXSetElementInvalid(outInvalid, i);		\
    }							\
    DXFree((Pointer)allValid);				\
    return (OK);					\
}


/*
 * Routines to manipulate invalid data.
 */
Error _dxfComputeComputeInvalidCopy(
    InvalidComponentHandle out,
    InvalidComponentHandle in0);
int _dxfComputeInvalidOne(
    InvalidComponentHandle out,
    int indexout,
    InvalidComponentHandle in0,
    int index0);
int _dxfComputeInvalidOr(
    InvalidComponentHandle out,
    int indexout,
    InvalidComponentHandle in0,
    int index0,
    InvalidComponentHandle in1,
    int index1);

int
_dxfComputeCompareType(MetaType *left, MetaType *right);


/* Checks a arguments to be sure all are vectors of same type and Category
 * as binding says arg0 should be, and same length.  It defines the result 
 * to be of speced rank (scalar or vector) from the binding structure, 
 * and if vector, the same length as the inputs.
 */
int
_dxfComputeCheckVects (PTreeNode *pt, ObjStruct *os, OperBinding *binding);

/* Checks a arguments to be sure all are objects of same type and Category
 * as binding says they should be, and rank and shape.  It defines the result 
 * to be of input cat, type, rank and shape.
 */
int
_dxfComputeCheckSameShape (PTreeNode *pt, ObjStruct *os, OperBinding *binding);

/* Checks a arguments to be sure all are objects of same type, Category, and 
 * rank (if scalar) as binding says they should be, and any shape allowed.  
 * If bindings says rank == 1, no scalars are allowed.
 * Output is that rank and shape of first non-scalar in binding.
 */
int
_dxfComputeCheckDimAnyShape (PTreeNode *pt, ObjStruct *os, OperBinding *binding);

/*
 * Check that the inputs match the types specified in the binding structure.
 * Return the type specified in the binding structure.
 */
int
_dxfComputeCheckMatch (PTreeNode *pt, ObjStruct *os, OperBinding *binding);

/*
 * Check that the inputs (any number > 1) match the type
 * of input[0], which is also used as the output type.
 */
int
_dxfComputeCheckSame (PTreeNode *pt, ObjStruct *os, OperBinding *binding);

/*
 * Check that the inputs (any number) match the type specified in the binding
 * structure for input[0], which is also used as the output type.
 */
int
_dxfComputeCheckSameAs (PTreeNode *pt, ObjStruct *os, OperBinding *binding);

/*
 * Check that the inputs (any number) match the type of input[0], like 
 * CheckSame, but use the outputType specified in the binding structure.
 */
int
_dxfComputeCheckSameSpecReturn (PTreeNode *pt, ObjStruct *os, OperBinding *binding);
/*
 * Check that the inputs (any number) match the type of input[0], like 
 * CheckSame, have the same Type specified in the binding, but use 
 * the outputType specified in the binding structure.
 */
int
_dxfComputeCheckSameTypeSpecReturn (PTreeNode *pt, ObjStruct *os, OperBinding *binding);

int     _dxfComputeLookupFunction (char *);
int     _dxfComputeFinishExecution();
int     _dxfComputeTypeCheck (PTreeNode *, ObjStruct *);
void    _dxfComputeInitExecution (void);
int     _dxfComputeExecuteNode (PTreeNode *, ObjStruct *,
                                Pointer, InvalidComponentHandle *);

#define MAXTOKEN        200


/* Routines to work on complex numbers. */
void         _dxfccinput(char *);
complexFloat _dxfComputeAddComplexFloat(complexFloat x, complexFloat y);
complexFloat _dxfComputeSubComplexFloat(complexFloat x, complexFloat y);
complexFloat _dxfComputeNegComplexFloat(complexFloat x);
complexFloat _dxfComputeMulComplexFloat(complexFloat x, complexFloat y);
complexFloat _dxfComputeDivComplexFloat(complexFloat x, complexFloat y);
float _dxfComputeAbsComplexFloat(complexFloat x);
float _dxfComputeArgComplexFloat(complexFloat x);
complexFloat _dxfComputeSqrtComplexFloat(complexFloat x);
complexFloat _dxfComputeLnComplexFloat(complexFloat x);
complexFloat _dxfComputePowComplexFloatFloat(complexFloat x, float y);
complexFloat _dxfComputeExpComplexFloat(complexFloat x);
complexFloat _dxfComputePowComplexFloat(complexFloat x, complexFloat y);
complexFloat _dxfComputeSinComplexFloat(complexFloat x);
complexFloat _dxfComputeCosComplexFloat(complexFloat x);
complexFloat _dxfComputeSinhComplexFloat(complexFloat x);
complexFloat _dxfComputeCoshComplexFloat(complexFloat x);
complexFloat _dxfComputeTanComplexFloat(complexFloat x);
complexFloat _dxfComputeTanhComplexFloat(complexFloat x);
int _dxfComputeFiniteComplexFloat(complexFloat x);
int _dxfComputeIsnanComplexFloat(complexFloat x);
complexFloat _dxfComputeAsinComplexFloat(complexFloat x);
complexFloat _dxfComputeAcosComplexFloat(complexFloat x);
complexFloat _dxfComputeAtanComplexFloat(complexFloat x);


#endif /* __COMPOPER_H_ */
