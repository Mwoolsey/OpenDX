/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/*
 * $Header: /src/master/dx/src/exec/dpexec/cache.h,v 1.7 2004/06/09 16:14:27 davidt Exp $
 */

#include <dxconfig.h>


#ifndef _CACHE_H_
#define _CACHE_H_

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

#if 0
These access methods are specifically used to operate on dictionary entries
that are associated with the recipe cache. These are the only access methods
which will be available from the modules.
#endif

#include "d.h"
#include "graph.h"

#define EXTAG(tag) ((tag) | 0x80000000)
typedef struct pathtag
{
    ModPath     mod_path;		/* module path			*/
    uint32      reccrc;                 /* cache tag                    */
    int         outnbr;                 /* nth output of module         */
    int         modnbr;                 /* nth instance of module       */
    int         entry_inuse;
} pathtag;

int     _dxf_ExCacheInit(void);
uint32  _dxf_ExGenCacheTag(char *function, int key, int n, uint32 *in);
int     _dxf_ExCacheInsert(gvar *obj);
int     _dxf_ExCacheDelete(uint32 key);
gvar   *_dxf_ExCacheSearch(uint32 key);
Error   _dxf_ExCacheFlush(int all);
int     _dxf_ExCacheInsertRemoteTag (int fd, int swap);
int     _dxf_ExCacheDeleteRemoteTag (int fd, int swap);
int     _dxf_ExCacheListDeleteRemoteTag(int fd, int swap);
extern EXDictionary _dxd_exCacheDict; /* defined in cache.c */

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif /* _CACHE_H_ */
