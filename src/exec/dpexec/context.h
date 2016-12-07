/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/*
 * $Header: /src/master/dx/src/exec/dpexec/context.h,v 1.6 2004/06/09 16:14:27 davidt Exp $
 */

#include <dxconfig.h>

#ifndef _CONTEXT_H
#define _CONTEXT_H

#include "graph.h"

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

typedef struct Context
{
    int userId;
    int graphId;
    Program *program;
    Program **subp;
} Context;

void _dxfCopyContext(Context *to, Context *from);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif /* _CONTEXT_H */
