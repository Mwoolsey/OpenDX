/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/*
 * $Header: /src/master/dx/src/exec/dpexec/cachegraph.h,v 1.3 2000/08/11 15:28:09 davidt Exp $
 */

#include <dxconfig.h>


#ifndef	_CACHEGRAPH_H
#define	_CACHEGRAPH_H

#include "graph.h"

/*---------------------------------------------------------------*/
/* This function maintains a table with path/cache tag entries   */
/* It searches a table looking for the current path and if found */
/*   1. does nothing if the cache tag hasn't changed             */
/*   2. replaces the current entry with the new path/tag entry   */
/*      if the cache tag has changed, and deletes the object     */
/*      identified by the old cache tag from the cache           */
/*  Note: cache entry may not necessarily be in the cache ...    */
/*  Question: Should this be conditional -i.e. based on the      */
/*            setting of a user specified "cache once" switch?   */
/*---------------------------------------------------------------*/
int _dxf_ExManageCacheTable(ModPath *cpath, uint32 reccrc, int outnbr);

#endif	/* _CACHEGRAPH_H */
