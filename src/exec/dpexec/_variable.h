/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/*
 * $Header: /src/master/dx/src/exec/dpexec/_variable.h,v 1.6 2004/06/09 16:14:27 davidt Exp $
 */

#include <dxconfig.h>


#if 0
These access methods are specifically used to operate on dictionary entries
which are user-variable definitions.
#endif

#ifndef __VARIABLE_H
#define __VARIABLE_H

#include "d.h"

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

int	_dxf_ExVariableInsert(char *name, EXDictionary dict, EXObj obj);
int	_dxf_ExVariableDelete(char *name, EXDictionary dict);
EXObj	_dxf_ExVariableSearch(char *name, EXDictionary dict);
void    _dxf_ExGVariableSetStr(char *var, char *val);
char *  _dxf_ExGVariableGetStr(char *var);
int     _dxf_ExVariableInsertNoBackground(char *name, EXDictionary dict, EXO_Object obj);

Error   _dxf_ExUpdateGlobalDict (char *name, Object obj, 
                                 int cause_execution);
EXObj   _dxf_ExGlobalVariableSearch (char *name);
Error   DXSetIntVariable(char *name, int val, int sync, 
                         int cause_execution);
Error   DXSetVariable(char *name, Object obj, int sync, 
                      int cause_execution);
Error   DXGetIntVariable(char *name, int *val);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif /* __VARIABLE_H */
