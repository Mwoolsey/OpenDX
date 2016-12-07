/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


/***
MODULE:		
    Compute
SHORTDESCRIPTION:
    computes a pointwise expression over a field
CATEGORY:
    Transformation
INPUTS:
    expression; string;         NULL;       expression to evaluate
    input0;     value or field; NULL;       input value
    input1;     value or field; NULL;       input value
    input2;     value or field; NULL;       input value
    input3;     value or field; NULL;       input value
    input4;     value or field; NULL;       input value
    input5;     value or field; NULL;       input value
    input6;     value or field; NULL;       input value
    input7;     value or field; NULL;       input value
    input8;     value or field; NULL;       input value
    input9;     value or field; NULL;       input value
    input10;    value or field; NULL;       input value
    input11;    value or field; NULL;       input value
    input12;    value or field; NULL;       input value
    input13;    value or field; NULL;       input value
    input14;    value or field; NULL;       input value
    input15;    value or field; NULL;       input value
OUTPUTS:
    output;     value or field; NULL;       output values
FLAGS:
BUGS:
    Always expands constants into arrays
AUTHOR:
    Channing Verbeck
END:
***/

#include <string.h>
#include <dx/dx.h>
#include "_compute.h"
#include "_compputils.h"
#include "_compoper.h"


/* These are global variables to allow communication with the parser */
PTreeNode *_dxdcomputeTree = NULL;
CompInput _dxdcomputeInput[MAX_INPUTS] = {{0, (Class)NULL, NULL}};


/*
 * A brief description of Compute:
 *
 * Compute is a general expression evaluator.  It parses an input expression
 * string and and performs that expression on its given inputs pointwise.  
 *
 * Inputs are (as expressed in the MODULE section of this file) are the 
 * expression and input objects.  The expression is in pseudo-C format and 
 * refers to input objects as $0, $1, etc.  An example of the use of Compute
 * follows:
 *	res = Compute ("mag ($0) * ($1 dot [1.,2,3])", vect1, vect2);
 * This takes the magnitude of each element of `vect1', which is a set of 
 * n-vectors (where n is arbitrary), and multiplies that with the dot product
 * of each point in `vect2' with [1.,2.,3.].  `Vect2' must be a set of 
 * 3-vectors.  `Vect1' and `vect2' must have the same structure (i.e. they
 * must both be the same Class objects, and the parts of the objects (if they
 * are groups) must have the same number of elements.
 *
 * A `master' input is determined.  It is the first input that is not a one 
 * element array.  All inputs must match this input.
 * If the parts of any input groups are fields,
 * they are checked to see if the `positions' components are the same, and the
 * user is warned if they are not; Compute doesn't map data onto the same
 * positions.  Trivial arrays (only one element) match with anything, and
 * is applied to all positions.
 *
 * Types are strictly checked, with ints automatically promoted to floats 
 * if the expression fails type checking as is.  Data elements are also 
 * checked to ensure they are of proper shape.  Most binary operators work 
 * on non- * scalars on an element-by-element basis.  Some operators enforce
 * that objects be of the correct rank, and have the same shape (e.g. dot).
 * 
 * Operators allowed are (where {} denotes optional, and ... denotes more
 * allowed):
 *	int or float constants -- 0x123abc is hex, 012 is octal (018 == 16,
 *		and the user is warned), and 01.0 and 1.3e-1 are floats.
 *	$int -- (e.g. $3) referes to an input.
 *	[ expression {,expression...}] -- vector constructor, can construct
 *		vectors of vectors of ....
 *	$int0.int1 -- (e.g. $3.2) selects the int1 vector from input int0.
 *		int1 may be `x', `y', or `z', which represent 0, 1, and 2.
 *	binary expressions +, -, *, /, %, ^, dot, cross -- dot and cross
 *		only work on vectors of the same length.
 *	unary expressions - and +.
 *	logical binarys <, <=, >, >=, ==, !=, ||, && -- || and && only
 *		work on ints, the others return ints.  0 is FALSE, !0 is TRUE.
 *	logical unary ! -- not.
 *	int Cexpr? Texpr: Eexpr -- Conditional expression.  Both Texpr and 
 *		Eexpr are evaluated.  Cexpr must be of type int.
 * 	funcname({args {,args...}}) -- built in function.  The following 
 * 		functions are supported:
 * 	likevectorwithrank-1 select(vector, int)
 * 	float sqrt(float)
 * 	float pow(float,float)
 * 	int pow(int,int)
 * 	float sin(float)
 * 	float cos(float)
 * 	float tan(float)
 * 	float ln(float)
 * 	float log(float)
 * 	int min(int{,int...})
 * 	float min(float{,float...})
 * 	int max(int{,int...})
 * 	float max(float{,float...})
 * 	float floor(float)
 * 	float ceil(float)
 * 	float trunc(float)
 * 	float rint(float)
 * 	float exp(float)
 * 	float tanh(float)
 * 	float sinh(float)
 * 	float cosh(float)
 * 	float acos(float)
 * 	float asin(float)
 * 	float atan(float)
 * 	float atan2(float, float)
 * 	float mag(float vector)
 * 	int mag(int vector)
 * 	float vector norm(float vector)
 * 	int vector norm(int vector)
 * 	float abs(float)
 * 	int abs(int)
 * 	int int(float)
 * 	float float(int)
 * 	float or int real(complex)
 * 	float or int real(quaternion)
 * 	float or int imag(complex)
 * 	complex complex(float)
 * 	complex complex(int)
 * 	complex complex(float, float)
 * 	complex complex(int, int)
 * 	float or int imagi(quaternion)
 * 	float or int imagj(quaternion)
 * 	float or int imagk(quaternion)
 * 	quaternion quaternion(float)
 * 	quaternion quaternion(int)
 * 	quaternion quaternion(float, float, float, float)
 * 	quaternion quaternion(int, int, int, int)
 * 	int sign(float)
 *
 * Compute Internals:
 *	The expression is parsed using a YACC built parser and a home-grown
 * lexor into _parseTree.  The inputs is then verified to be of the same 
 * structure, and a copy of this structure is created into inputStruct.
 * This structure contains a node for each object in the group hierarchy,
 * and an output object (a copy of the master input) is created.  Each 
 * object in the heirarchy which is not a Generic Group and not a member
 * of a Homogeneous Group has a copy of the parse tree. The objects with parse
 * trees are typechecked and executed.  For more details on that process,
 * see _compoper.c.  The results are inserted into inputStruct.output, which
 * is the result of Compute.
 */

int _dxfccparse();

int
m_Compute(Object *in, Object *out)
{
    char		*expression;
    ObjStruct		*inputStruct;

    _dxfComputeInitInputs(_dxdcomputeInput);

    out[0] = NULL;

    if (in[0] == NULL) {
	DXSetError (ERROR_BAD_PARAMETER, "#10200", "expression");
	return ERROR;
    }

    if (!DXExtractString (in[0], &expression)) {
	DXSetError (ERROR_BAD_TYPE, "#10200", "expression");
	return ERROR;
    }

    if (strlen(expression) == 0) {
	DXSetError (ERROR_BAD_PARAMETER, "#10210", expression, "expression");
	return ERROR;
    }

    /* Build parse tree into _dxdcomputeTree */
    _dxfccinput (expression);
    if (_dxfccparse () == 1 || _dxdcomputeTree == NULL) {
	return (ERROR);
    }

#if COMP_DEBUG
    _dxfComputeDumpParseTree (_dxdcomputeTree, 0);
#endif

    /* Get the inputs used in the expression */
    if ((inputStruct = _dxfComputeGetInputs (in + 1, _dxdcomputeInput)) == NULL) {
	_dxfComputeFreeTree(_dxdcomputeTree);
	return (ERROR);
    }

#if COMP_DEBUG
    _dxfComputeDumpInputs (_dxdcomputeInput, inputStruct);
#endif

    /* Type check the expression with the inputs, returning an output 
     * description in the top tree element, and execute the tree with
     * respect to that parse tree.
     */
    if (_dxfComputeExecute (_dxdcomputeTree, inputStruct) == ERROR) {
	if (inputStruct->class != CLASS_ARRAY)
	    DXDelete(inputStruct->output);
	_dxfComputeFreeTree (_dxdcomputeTree);
	_dxfComputeFreeObjStruct (inputStruct);
	return (ERROR);
    }
    out[0] = inputStruct->output;

    _dxfComputeFreeTree (_dxdcomputeTree);
    _dxfComputeFreeObjStruct (inputStruct);
    return (OK);
}
