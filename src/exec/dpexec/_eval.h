/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/*
 * $Header: /src/master/dx/src/exec/dpexec/_eval.h,v 1.4 2000/08/11 15:09:37 davidt Exp $
 */

#include <dxconfig.h>

#ifndef __EVAL_H
#define __EVAL_H


Object _evalMacro(node *n, int dictId);
Object _evalModule(node *n, int dictId);
Object _evalLoop(node *n, int dictId);
Object _evalAssignment(node *n, int dictId);
Object _evalPrint(node *n, int dictId);
Object _evalAttribute(node *n, int dictId);
Object _evalCall(node *n, int dictId);
Object _evalArgument(node *n, int dictId);
Object _evalArithmetic(node *n, int dictId);
Object _evalLogical(node *n, int dictId);
Object _evalArithmetic(node *n, int dictId);
Object _evalArithmetic(node *n, int dictId);
Object _evalExid(node *n, int dictId);


#endif /* __EVAL_H */
