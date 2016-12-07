/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


#include <math.h>
#include <string.h>
#include <dx/dx.h>
#include "_compute.h"
#include "_compputils.h"
#include "_compoper.h"

extern int _dxdparseError;

/*
 * create a data node for the expression tree
 */
PTreeNode *
_dxfMakeArg(int type)
{
    PTreeNode *arg;

    arg = (PTreeNode *)DXAllocate(sizeof(PTreeNode));
    if (arg == NULL)
	return(arg);

    arg->oper = type;
    arg->next = (PTreeNode *)NULL;
    OPRL_INIT (arg, (PTreeNode *)NULL);
    arg->metaType.items = 0;
    arg->metaType.type = (Type) 0;
    arg->metaType.category = (Category) 0;
    arg->metaType.rank = 0;
    arg->metaType.shape[0] = 0;
    arg->u.i = 0;
    return(arg);
}

/*
 * create a function call node
 */
PTreeNode *
_dxfMakeFunCall(char *func, PTreeNode *args)
{
    PTreeNode *res;
    PTreeNode *nextArgs;
    int funcId;

    if ((funcId = _dxfComputeLookupFunction (func)) == NT_ERROR) {
	++_dxdparseError;
	DXSetError(ERROR_BAD_PARAMETER, "#12090", func);
	while (args) {
	    nextArgs = args->next;
	    _dxfComputeFreeTree (args);
	    args = nextArgs;
	}
	return (NULL);
    }

    res = _dxfMakeArg (funcId);

    if (res == NULL) {
	while (args) {
	    nextArgs = args->next;
	    _dxfComputeFreeTree (args);
	    args = nextArgs;
	}
	return(NULL);
    }


    strncpy (res->u.s, func, MAX_CA_STRING);
    res->operName = res->u.s;
    OPRL_INIT (res, args);

    return (res);
}

/*
 * create a unary operator node
 */
PTreeNode *
_dxfMakeUnOp(int op, PTreeNode *arg)
{
    PTreeNode  *res;

    if (arg == NULL) {
	DXSetError (ERROR_INTERNAL, "_dxfMakeUnOp, insufficient arguments");
	return(NULL);
    }


    res = _dxfMakeArg (op);
    if (res == NULL) {
	_dxfComputeFreeTree (arg);
	return(NULL);
    }
    OPRL_INIT (res, arg);
    arg->next = NULL;

    return(res);
}

/*
 * create a binary operator node
 */
PTreeNode *
_dxfMakeBinOp(int op, PTreeNode *left, PTreeNode *right)
{
    PTreeNode *res;

    if (left == NULL || right == NULL)
    {
	DXSetError(ERROR_INTERNAL, "_dxfMakeBinOp, insufficient arguments");
	_dxdparseError++;
	if (left)
	    _dxfComputeFreeTree(left);
	if (right)
	    _dxfComputeFreeTree(right);
	return(NULL);
    }

    res = _dxfMakeArg (op);
    if (res == NULL) {
	_dxfComputeFreeTree(left);
	_dxfComputeFreeTree(right);
	return(NULL);
    }

    OPRL_INIT   (res, right);
    OPRL_INSERT (res, left);

    return(res);
}

/*
 * make a conditional expression node (eg.  (a < b) ? a : b) )
 */
PTreeNode *
_dxfMakeConditional(PTreeNode *relation, PTreeNode *trueexp, PTreeNode *falseexp)
{
    PTreeNode *res;

    if (relation == NULL || trueexp == NULL || falseexp == NULL)
    {
	DXSetError(ERROR_INTERNAL, "_dxfMakeConditional, insufficient arguments");
	_dxdparseError++;
	if (relation)
	    _dxfComputeFreeTree(relation);
	if (trueexp)
	    _dxfComputeFreeTree(trueexp);
	if (falseexp)
	    _dxfComputeFreeTree(falseexp);
	return(NULL);
    }

    res = _dxfMakeArg (NT_COND);
    if (res == NULL) {
	_dxfComputeFreeTree(relation);
	_dxfComputeFreeTree(trueexp);
	_dxfComputeFreeTree(falseexp);
	return(NULL);
    }

    OPRL_INIT   (res, falseexp);
    OPRL_INSERT (res, trueexp);
    OPRL_INSERT (res, relation);

    return(res);
}


void
_dxfComputeFreeTree(PTreeNode *tree)
{
    PTreeNode *subTree;
    PTreeNode *nextSubTree;
#ifdef COMP_DEBUG
    DXDebug("C", "_dxfComputeFreeTree (tree = 0x%08x)", tree);
#endif

    if (tree == NULL)
	return;

    subTree = tree->args;
    while (subTree != NULL) {
	nextSubTree = subTree->next;
	_dxfComputeFreeTree (subTree);
	subTree = nextSubTree;
    }
    DXFree((Pointer)tree);
}

PTreeNode *
_dxfComputeCopyTree(PTreeNode *tree)
{
    PTreeNode *subTree;
    PTreeNode *newTree;
    PTreeNode *newSubTree;
    PTreeNode **lastNext;


    if (tree == NULL)
	return NULL;

    newTree = (PTreeNode *)DXAllocate (sizeof (PTreeNode));
    if (newTree == NULL)
	return NULL;
    *newTree = *tree;

    subTree = tree->args;
    lastNext = &newTree->args;
    while (subTree != NULL) {
	newSubTree = _dxfComputeCopyTree (subTree);
	*lastNext = newSubTree;
	if (newSubTree == NULL) {
	    _dxfComputeFreeTree(newTree);
	    return NULL;
	}
	subTree = subTree->next;
	lastNext = &newSubTree->next;
    }
#ifdef COMP_DEBUG
    DXDebug("C", "_dxfComputeCopyTree(tree = 0x%08x) -> 0x%08x", tree, newTree);
#endif
    return newTree;
}

/*
 */
PTreeNode *
_dxfMakeInput(int inputIndex)
{
    PTreeNode *input;

    input = _dxfMakeArg(NT_INPUT);
    input->u.i = inputIndex;
    _dxdcomputeInput[inputIndex].used = 1;

    return(input);
}


PTreeNode *
_dxfMakeList(int op, PTreeNode *list)
{
    PTreeNode *res;

    res = _dxfMakeArg (op);
    if (res == NULL) {
	while (list != NULL) {
	    res = list->next;
	    _dxfComputeFreeTree (list);
	    list = res;
	}
	return(NULL);
    }

    OPRL_INIT (res, list);

    return(res);
}

PTreeNode *
_dxfMakeAssignment(char *name, PTreeNode *value)
{
    PTreeNode *res;

    res = _dxfMakeArg (OPER_ASSIGNMENT);
    if (res == NULL) {
	_dxfComputeFreeTree (value);
	return(NULL);
    }

    strncpy (res->u.s, name, MAX_CA_STRING);
    OPRL_INIT (res, value);

    return(res);
}

PTreeNode *
_dxfMakeVariable(char *name)
{
    PTreeNode *res;

    res = _dxfMakeArg (OPER_VARIABLE);
    if (res == NULL) {
	return(NULL);
    }

    strncpy (res->u.s, name, MAX_CA_STRING);

    return(res);
}



#ifdef COMP_DEBUG
_dxfComputeDumpParseTree (PTreeNode *tree, int indent)
{
    static char *spaces = "                    ";
    char *ind = spaces + (strlen (spaces) - indent);
    PTreeNode *subTree;

    DXDebug("C", "%snode 0x%08x, type %d", ind, tree, tree->oper);

    if (tree->args) {
	DXDebug("C", "%sSub-Nodes are:", ind);
	for (subTree = tree->args; subTree != NULL; 
		subTree = subTree->next) {
	    _dxfComputeDumpParseTree (subTree, indent+4);
	}
    }
}
#endif
