/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/*
 * $Header: /src/master/dx/src/exec/dpexec/graphqueue.h,v 1.6 2004/06/09 16:14:28 davidt Exp $
 */

#include <dxconfig.h>

#ifndef _GRAPHQUEUE_H
#define _GRAPHQUEUE_H

#include "graph.h"

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

int _dxf_ExGQInit (int ngraphs);
int _dxf_ExGQAllDone (void);
void _dxf_ExGQEnqueue (Program *func);
Program *_dxf_ExGQDequeue (void);

#if 0
extern int *gq_sort;
extern int ngraphs;
#endif

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif /* _GRAPHQUEUE_H */
