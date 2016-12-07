/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


#ifndef DictionaryH
#define DictionaryH
typedef struct _dictitem_t DictItem;
typedef struct __Dictionary {
	DictItem	**harr;
	int		num_in_dict;
	short		size;
	} _Dictionary;
#define	DictSize(D)		((D)->num_in_dict)
void DeleteDictionary(_Dictionary *d);
_Dictionary *NewDictionary(void);
int DictInsert(_Dictionary *d, const char *key, void *def, void (*)(void*));
int DictDelete(_Dictionary *d, const char *key);
void *DictFind(_Dictionary *d, const char *key);
#endif
