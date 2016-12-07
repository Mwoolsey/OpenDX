/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/*
 * $Header: /src/master/dx/src/exec/dpexec/d.h,v 1.6 2004/06/09 16:14:27 davidt Exp $
 */

#include <dxconfig.h>

#ifndef	__EX_DICTIONARY_H
#define	__EX_DICTIONARY_H

#include <dx/dx.h>

#include "exobject.h"

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

typedef struct _EXDictionary		*EXDictionary;

EXDictionary _dxf_ExDictionaryCreate(int size, int local, int locking);
EXDictionary _dxf_ExDictionaryCreateSorted(int size, int local, int locking);
Error	     _dxf_ExDictionaryDestroy(EXDictionary d);
int  	     _dxf_ExDictionaryCompact(EXDictionary d);
Error	     _dxf_ExDictionaryDelete(EXDictionary d, Pointer key);
Error 	     _dxf_ExDictionaryDeleteNoLock(EXDictionary d, Pointer key);
Error	     _dxf_ExDictionaryInsert(EXDictionary d, char *key, EXObj obj);
int	     _dxf_ExDictionaryPurge(EXDictionary d);
EXObj	     _dxf_ExDictionarySearch(EXDictionary d, char *key);
Error	     _dxf_ExDictionaryBeginIterate(EXDictionary d);
EXObj	     _dxf_ExDictionaryIterate(EXDictionary d, char **key);
Error	     _dxf_ExDictionaryEndIterate(EXDictionary d);
int          _dxf_ExDictionaryIterateMany(EXDictionary d, char **key, EXObj *obj, int n);
int	     _dxf_ExIsDictionarySorted(EXDictionary d);
Error 	     _dxf_ExDictionaryBeginIterateSorted(EXDictionary d, int reverse);
EXObj 	     _dxf_ExDictionaryIterateSorted(EXDictionary d, char **key);
int          _dxf_ExDictionaryIterateSortedMany(EXDictionary d, char **key, EXObj *obj, int n);
Error	     _dxf_ExDictPrint(EXDictionary d);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif


#endif /* __EX_DICTIONARY_H */
