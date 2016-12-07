/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

#ifndef _DXI_HASH_H_
#define _DXI_HASH_H_

/*
\section{Hashing}
This section describes support for hashing.  This is an implementation
of the method described in 'Extendible Hashing - A Fast Access Method
for Dynamic Files', by Fagin, Nievergelt, Pippinger and Strong in ACM
Transactions on Data Systems, Vol. 4, No. 3, 9/1979 which provides
storage for arbitrary numbers of elements with a fixed access time.
This implementation, while not perhaps the most efficient in every
case, is designed to be fairly general-purpose for ease of use in many
applications.

Elements stored in hash tables created using this support may be
of any fixed size, set at the time the hash table is created.  Each
element contains a key identifying the element, folowed by any data
to be associated with that key.

Elements are stored into the table using 32-bit pseudo-keys.  These
pseudo-keys should be uniformly distributed in any range beginning at
zero.  The keys themselves either contain the pseudo-key as their first 
32-bit word or are derived from the key via a call-back function
provided at hash-table creation time.

More than one element may be stored under the same pseudo-key if a
compare function is provided.  Whenever {\tt DXQueryHashElement} is
called with some search key, the hash table is searched for an element
whose pseudo-key matches that either contained in or derived from the
search key.  If no compare function has been provided, the found
element is returned.  However, if a compare function has been provided,
it is called by {\tt DXQueryHashElement} to allow the calling application
to match the search key against each element contained in the table
that matches the pseudo-key.  When the compare function succeeds
(returns a 0) the element is returned by {\tt DXQueryHashElement}.  Note
that a similar sequence occurs when {\tt DXInsertHashElement} is used to
either insert a unique element of overwrite a previously inserted
element of the same key.

Optionally provided by the calling application:

PseudoKey DXhashFunc(Key key);

Called on insertion and query to convert the arbitrary-size search key 
into the 32-bit pseudo-key used to store the hash table elements.

int DXcmpFunc(Key searchKey, Element element);

Called on insertion and query when an element from the table matches the
pseudo-key derived from the search key.  Returns 0 if the search key matches
the key contained in the element found in the table, non-zero otherwise.
*/

typedef struct hashTable  *HashTable;

typedef long  		  PseudoKey;
typedef Pointer 	  Key;
typedef Pointer 	  Element;

HashTable DXCreateHash(int elementSize, 
		PseudoKey (*DXhashFunc)(), int (*DXcmpFunc)());
/**
\index{DXCreateHash}
Creates a hash table storing elements (key + data) of size {\tt elementSize}.
{\tt DXhashFunc} is the optional hash function callback.  Given an element, 
{\tt DXhashFunc} should return an uniformly distributed 32-bit pseudo-key.
If no hash function is supplied, the first 32-bit word of each key is
assumed to be the pseudo-key.  {\tt DXcmpFunc} is the optional compare
function callback.  Given a search key and an element, {\tt DXcmpFunc} 
should return 0 if the key matches the element.  If no compare function
is supplied, any element matching the pseudo-key derived from a search key
is assumed to match the search key.
*/

Error DXDestroyHash(HashTable hashTable);
/**
\index{DXDestroyHash}
DXDelete {\tt hashTable} and all storage associated with it.
*/

Element	DXQueryHashElement(HashTable hashTable, Key searchKey);
/**
\index{DXQueryHashElement}
Search the hash table for an element matching {\tt searchKey}.  Returns
the element if found, NULL otherwise.
*/

Error DXDeleteHashElement(HashTable hashTable, Key searchKey);
/**
\index{DXDeleteHashElement}
Removes any element that matches the {\tt searchKey}.  Always returns OK.
*/

Error DXInsertHashElement(HashTable hashTable, Element element);
/**
\index{DXInsertHashElement}
Inserts {\tt element} into {\tt hashTable}.  If an element matching the key
contained in {\tt element} already exists in the table, it is overwritten.
*/

/**
\paragraph{Hash Table Iteration}
The following routines support iteration through the elements of the 
hash table.  The elements are produced in no pre-defined order.
*/

Error DXInitGetNextHashElement(HashTable hashTable);
/**
\index{DXInitGetNextHashElement}
Initializes iteration through the contents of {\tt hashTable}.
*/

Element DXGetNextHashElement(HashTable hashTable);
/**
\index{DXGetNextHashElement}
Returns the next element in the iteration.  Returns NULL when the
entire contents of {\tt hashTable} have been passed.
*/

#endif /* _DXI_HASH_H_ */

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif
