/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/*
 * $Header: /src/master/dx/src/exec/dpexec/graphIntr.h,v 1.5 2000/08/11 15:28:11 davidt Exp $
 */

#include <dxconfig.h>


#ifndef __GRAPHINT_H__
#define __GRAPHINT_H__

#if 0

extern lock_type *graph_sem;
extern int graphing_error;
extern EXDictionary exDictStackTop;

EXObj dict_stack_search (char *id, int offset);
EXDictionary pop_dict_stack(int freeIt);
void push_dict_stack (EXDictionary dict);
Error init_dict_stack (int perGraph);

#endif

void		_dxf_ExMacroRecursionInit	(void);
void		_dxf_ExComputeRecipes(Program *p, int funcInd);

#endif /* __GRAPHINT_H__ */
