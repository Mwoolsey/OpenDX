/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/*
 * $Header: /src/master/dx/src/exec/dpexec/_macro.h,v 1.6 2004/06/09 16:14:27 davidt Exp $
 */

#include <dxconfig.h>


#if 0
These access methods are used to operate on dictionary entries which are
macro and function definitions.  The macro definitions are actually stored
as parse trees to facilitate their inclusion upon execution.  Function
definitions are stored as a pointer to a function.  Each of the definitions
are wrapped in a parse node to store additional information such as
number of inputs and their names and number of outputs and their names.
#endif

#ifndef __MACRO_H
#define __MACRO_H

#include "d.h"

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

Error	_dxf_ExMacroInit	(void);
int	_dxf_ExMacroInsert	(char *name, EXObj obj);
int	_dxf_ExMacroDelete	(char *name);
EXObj	_dxf_ExMacroSearch	(char *name);

extern	EXDictionary	_dxd_exMacroDict; /* defined in macro.c */

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif /* __MACRO_H */
