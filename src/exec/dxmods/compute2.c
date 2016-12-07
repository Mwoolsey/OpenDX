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
    Compute2
SHORTDESCRIPTION:
    computes a pointwise expression over a field
CATEGORY:
    Transformation
INPUTS:
    expression; string or stringlist; NULL;       inpute expression
    name0;      string;         "a";        name of input0   
    input0;     value or field; NULL;       input value
    name1;      string;         "b";        name of input1   
    input1;     value or field; NULL;       input value
    name2;      string;         "c";        name of input2   
    input2;     value or field; NULL;       input value
    name3;      string;         "d";        name of input3   
    input3;     value or field; NULL;       input value
       :
OUTPUTS:
    output;     value or field; NULL;       output values
FLAGS:
BUGS:
    Always expands constants into arrays
AUTHOR:
    Frank Suits 
END:
***/

#include <string.h>
#include <dx/dx.h>
#include "_compute.h"
#include "_compoper.h"
#include "_compputils.h"


/* These are global variables to allow communication with the parser */
/* They're declared in compute.m, so extern here.                    */
extern PTreeNode *_dxdcomputeTree;
extern CompInput _dxdcomputeInput[MAX_INPUTS];


int _dxfccparse();

int
m_Compute2(Object *in, Object *out)
{
#define NAME_SIZE 256

    char		*expression0;
    ObjStruct		*inputStruct;
    Object              in2[MAX_INPUTS+1];
    int                 i,cnt,n;
    char                s[NAME_SIZE];
    char                *expression;
    int                 exp_size;
    char                *name;
    char                default_name[4];

#define DEFAULT_NAMES "abcdefghijklmnopqrstuvwxyz"
#define DELTA_SIZE 512
#define ADD_STRING(p, s, sz)	 		    \
	if ((s) && strlen(s)) {			    \
	    if (!(p)) {				    \
		p = DXAllocate(DELTA_SIZE);	    \
		if (!(p))			    \
		    goto error;			    \
		sz = DELTA_SIZE;		    \
		*p = '\0';			    \
	    }					    \
	    while (strlen(p)+strlen(s)+1 > sz) {    \
		sz += DELTA_SIZE;		    \
		p = DXReAllocate(p, sz);	    \
		if (!(p))			    \
		    goto error;			    \
	    }					    \
	    strcat(p,s);			    \
	}

    _dxfComputeInitInputs(_dxdcomputeInput);

    out[0]     = NULL;
    expression = NULL;
    exp_size   = 0;

    if (in[0] == NULL) {
	DXSetError (ERROR_BAD_PARAMETER, "#10200", "expression");
	return ERROR;
    }
	    
    for (i=0;i<MAX_INPUTS+1;i++)
	in2[i] = NULL;

    cnt = 0;
    for (i=1;i<MAX_INPUTS;i+=2)
    {
	if (in[i+1]) {
	    int bad_name = 0;
	    if (!in[i] || (bad_name = !DXExtractString(in[i],&name) || !strlen(name))) {
		default_name[0] = DEFAULT_NAMES[i/2];
		default_name[1] = '\0';
		name = default_name;
		if (bad_name)
		    DXWarning("Compute2: Bad input%d name.  Defaulting to %s.", i/2, name);
	    }
	    if (strlen(name)+5 > NAME_SIZE) {
		DXSetError(ERROR_BAD_PARAMETER, "Compute2: Name length exceeds maximum");
		goto error;
	    }
            sprintf(s,"%s=$%d;", name, cnt);
	    ADD_STRING(expression, s, exp_size);
	    in2[cnt]=in[i+1];
	    cnt++;
	}
    }

    switch (DXGetObjectClass(in[0])) {

        case CLASS_STRING:
	    if (!DXExtractString (in[0], &expression0)) {
		DXSetError (ERROR_BAD_TYPE, "#10200", "expression");
		goto error;
	    }

	    if (!strlen(expression0)) {
		DXSetError (ERROR_BAD_PARAMETER, "#10210", expression0, "expression");
		goto error;
	    }

	    ADD_STRING(expression, expression0, exp_size);
	    break;
	
	default:
	    n = 0;
	    while (DXExtractNthString(in[0], n++, &expression0)) {
		ADD_STRING(expression, expression0, exp_size);
	    }
	    break;

    }

    if (!expression || !strlen(expression))
	DXSetError(ERROR_BAD_PARAMETER, "#10210", expression, "expression");

    /* Build parse tree into _dxdcomputeTree */
    _dxfccinput (expression);
    if (_dxfccparse () == 1 || _dxdcomputeTree == NULL) {
	goto error;
    }

#if COMP_DEBUG
    _dxfComputeDumpParseTree (_dxdcomputeTree, 0);
#endif

    /* Get the inputs used in the expression */
    if ((inputStruct = _dxfComputeGetInputs (in2, _dxdcomputeInput)) == NULL) {
	_dxfComputeFreeTree(_dxdcomputeTree);
	goto error;
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
	goto error;
    }
    out[0] = inputStruct->output;

    _dxfComputeFreeTree (_dxdcomputeTree);
    _dxfComputeFreeObjStruct (inputStruct);

    DXFree(expression);
    return (OK);

error:

    DXFree(expression);
    return (ERROR);
}
