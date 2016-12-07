/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include <defines.h>


#include "StringTable.h"
#include "List.h"
#include "ListIterator.h"
#include "DXStrings.h"

#define LOG_MAX_LISTSIZE	20	
#define MAX_LISTSIZE		(1<<LOG_MAX_LISTSIZE)	
#define HASH_SIZE		64	

#define LISTINDEX_TO_ID(listnum, index) (listnum * MAX_LISTSIZE + index)
#define ID_TO_LISTNUM(index)   (index >> LOG_MAX_LISTSIZE)
#define ID_TO_LISTINDEX(index) (index & (MAX_LISTSIZE-1))

static int hash(const char *str)
{
    char c, *p = (char*)str;    
    int i, sum = 0;

    for (i=0 ; (c = *p) && i<12 ; i++, p++)
	sum = sum + c;

    int key = sum & (HASH_SIZE -1); 

    return key;
    
}

StringTable::StringTable()
{
    this->lists = new List[HASH_SIZE];
    this->size = 0;
    //
    // No op.
    //
}


StringTable::~StringTable()
{
    this->clear();
    delete[] this->lists;
}


void StringTable::clear()
{
    int 	i;

    ASSERT(this);

    for (i=0 ; i<HASH_SIZE ; i++) {
	List *l = &this->lists[i];
	ListIterator li(*l);
	char *s;
	while ( (s = (char*)li.getNext()) ) 
	    delete s;
	l->clear();
    }
    //
    // Clear the list.
    //
    this->size = 0;
}

#if 0
static boolean _EqualString(const void *s1, const void *s2)
{
	return EqualString((const char*)s1,(const char*)s2);
}

static void *_DuplicateString(const void *s)
{
	return (void*) DuplicateString((const char*)s);
}
#endif

boolean StringTable::addString(const char* string, long& index)
{
    int key = hash(string); 
    List *l = &this->lists[key];
    ListIterator li(*l);
    char *s;

    while ( (s = (char*) li.getNext()) ) {
	if (EqualString(s,string))
	   return FALSE;
    }
 
#if  DEBUG_HASH_FUNC
    int i;
    static int hist[HASH_SIZE];
    hist[key]++;
    for (i=0 ; i<4 ; i++) {
	int j;
	for (j=0 ; j<16 ; j++) 
	    printf("%d ",hist[i*4 + j]);
	printf("\n");
    }
    printf("\n");
#endif
    //
    // The string was not found in the list, so add it.
    //
    ASSERT(l->getSize() < MAX_LISTSIZE);
    s = DuplicateString(string);
    l->appendElement((const void*)s);
    index = LISTINDEX_TO_ID(key,l->getSize());
    this->size++;
    return TRUE;
}

const char *StringTable::getString(int id)
{
   int list  = ID_TO_LISTNUM(id); 
   int index = ID_TO_LISTINDEX(id); 
   List *l = &this->lists[list];
   return (const char*)l->getElement(index);
}

int StringTable::findString(const char* string)
{
    if (NOT string)
	return 0;

    int key = hash(string); 
    List *l = &this->lists[key];
    //
    // Search the table.
    //
    ListIterator li(*l);
    int index = 0;
    char *s;
    while ( (s = (char*) li.getNext()) ) {
	index++;
	if (EqualString(s,string))
	    return LISTINDEX_TO_ID(key,index); 
    }
    //
    // Not found, return 0.
    //
    return 0;
}


