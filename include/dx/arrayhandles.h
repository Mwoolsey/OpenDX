/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#define class __class__
#endif

#ifndef _DXI_ARRAYHANDLES_H_
#define _DXI_ARRAYHANDLES_H_

struct arrayHandle 
{
    /*
     * All arrays
     */
    Class    class;
    int      itemSize;
    Type     type; 
    Category cat;
    int      nValues;	/* number of TYPE vlaues in each array element	*/
    int      nElements; /* number of elements in the array		*/

    /*
     * Generic arrays
     */
    Pointer data;

    /*
     * Constant arrays
     */
    Pointer cdata;

    /*
     * Regular arrays
     */
    Pointer origin, delta;

    /*
     * Compound arrays
     */
    int nTerms;
    int *strides;
    int *counts;
    int *indices;
    struct arrayHandle **handles;
    Pointer *scratch;

    /*
     * Path arrays only
     */
    int *scratch1, *scratch2;
};

typedef struct arrayHandle *ArrayHandle;

ArrayHandle DXCreateArrayHandle(Array);
Error       DXFreeArrayHandle(ArrayHandle);
Pointer     DXCalculateArrayEntry(ArrayHandle, int, Pointer);

#define DXGetArrayEntry(_h, _off, _scr)				  	  \
	(((_h)->data != NULL) ?				       	  	  \
	    (Pointer)(((ubyte *)((_h)->data)) + (_off)*(_h)->itemSize) 	  \
	  : ((_h)->cdata != NULL) ? (_h)->cdata 			  \
	  : DXCalculateArrayEntry((_h), (_off), (_scr)))

#define DXIterateArray(_h, _off, last, _scr)				  \
	(((_h)->data != NULL) ?					  	  \
		(((_off) == 0) ?					  \
			  (_h)->data					  \
			: (Pointer)(((ubyte *)last) + (_h)->itemSize))	  \
	      : (((_h)->cdata != NULL) ?				  \
			  (_h)->cdata 				  	  \
			: DXCalculateArrayEntry((_h), (_off), (_scr))))

#define DXGetArrayEntries(_h, _cnt, _off, _ptrs, _scr)			  \
do {									  \
    int __i, __cnt = (_cnt), *(__off) = (_off);				  \
    Pointer *__ptrs = (Pointer *)(_ptrs);				  \
    ubyte   *__scr  = (ubyte *)(_scr);					  \
    for(__i = 0; __i < __cnt; __i++)					  \
    {									  \
	__ptrs[__i] = DXGetArrayEntry((_h), (__off)[_i], (__scr));	  \
	(__scr) += (_h)->itemSize;					  \
    }									  \
} while(0);

#endif /* _DXI_ARRAYHANDLES_H_ */

#if defined(__cplusplus) || defined(c_plusplus)
#undef class
}
#endif
