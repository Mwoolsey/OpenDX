/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/*
 * $Header: /src/master/dx/src/exec/dxmods/_compputils.h,v 1.4 2000/08/24 20:04:09 davidt Exp $
 */

#include <dxconfig.h>

#ifndef __COMPPUTILS_H_
#define __COMPPUTILS_H_


PTreeNode *_dxfMakeArg(int);
PTreeNode *_dxfMakeFunCall(char *, PTreeNode *);
PTreeNode *_dxfMakeList(int op, PTreeNode *);
PTreeNode *_dxfMakeBinOp(int, PTreeNode *, PTreeNode *);
PTreeNode *_dxfMakeUnOp(int, PTreeNode *);
PTreeNode *_dxfMakeConditional(PTreeNode *, PTreeNode *, PTreeNode *);
PTreeNode *_dxfMakeInput(int);
PTreeNode *_dxfMakeAssignment(char *, PTreeNode*);
PTreeNode *_dxfMakeVariable(char *);
void    _dxfComputeFreeTree(PTreeNode *);
PTreeNode *_dxfComputeCopyTree(PTreeNode *);

extern int _dxdparseError;

#endif /* __COMPPUTILS_H_ */

