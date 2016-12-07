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

#ifndef _DXI_INVALID_H_
#define _DXI_INVALID_H_

/* TeX starts here. Do not remove this comment. */

/*
\section{Invalid Data}

In order to support the selective elimination of positions and
connections from datasets, fields may contain ``invalid positions''
and ``invalid connections'' components.  These contain flags in a
one-to-one correspondence with the elements of the positions and
connections components that denote the current validity of the
correspondong element.  Elements carrying a {\tt DATA\_INVALID}
validity status will not participate in further processing.
Note: in the current release of the Data Explorer, invalid data
marking is not supported by all modules.

Data may be invalidated by modifying (creating, if necessary) these
invalid positions and invalid connections arrays.  If none exists, all
elements are assumed to be valid and an invalid array may be created
and initialized to {\tt DATA\_VALID} for all elements.  Once positions
have been invalidated, it is possible to propagate the validity of
positions to the connections that are incident on them by calling {\tt
DXInvalidateConnections} which will create an invalid connections
component, if necessary.  Note that the ``invalid connections''
component cannot be assumed to be up-to-date with regard to invalid
positions unless this routine is called.
 
Invalid positions and connections (and their dependent information) may
be actually removed from the data set be calling {\tt DXCull}.  This
routine removes all invalidated positions and connections and the
corresponding elements of components that are dependent on positions
and connections, renumbers components that reference positions or
connections (inserting a -1 for indices that refer to removed positions
or connections) and, finally, removes the invalid positions and
connections components.
 
The removal of invalid positions and connections may have significantly
detrimental effect on the performance of the system as a whole since it
may require the conversion or regular positions and connections
components to irregular form, thereby increasing the memory usage for
these components dramatically.  However, the invalid components can
then be removed, saving space.  {\tt DXCullConditional} is included to
weigh these considerations for the calling application.  {\tt
DXCullConditional} may either return the original structure with invalid
components intact, cull the connections component, leaving the invalid
positions component intact, or cull both positions and connections.

*/
 
#define DATA_VALID   0
#define DATA_INVALID 1
 
Object DXInvalidateConnections(Object object);
/**
\index{DXInvalidateConnections}
Propagate the validity of positions within the fields of {\tt object}
to the connections.  A connection is invalidated if any position on
which it is incident is invalid.  An ``invalid connections'' component
will be created if necessary.  Returns the updated object on success,
or returns null and sets the error code to indicate an error.
**/
 
Object DXInvalidateUnreferencedPositions(Object object);
/**
\index{DXInvalidateUnreferencedPositions}
Determine which positions in the constituent fields of {\tt object}
are not referenced by any connections element and invalidate them.  An
``invalid positions'' component will be created if necessary.  Returns
the updated object, or returns null and sets the error code to
indicate an error.
**/
 
Object DXCull(Object object);
/**
\index{DXCull}
DXRemove any invalidated positions and/or connections from the
constituent fields of {\tt object}.  DXRemove corresponding elements
from components that are dependent on positions or connections.
Renumber components that reference either positions and connections,
inserting -1 for indices that reference removed positions or
connections.  DXRemove "invalid positions" and "invalid connections"
components.  Returns the updated object, or returns null and sets the
error code to indicate an error.
**/
 
Object DXCullConditional(Object object);
/**
\index{DXCullConditional}
Conditionally cull {\tt object}.  Based on effiency criteria, {\tt
DXCullConditional} may either leave the original structure with invalid
components intact, cull the connections component, leaving the invalid
positions component intact, or cull both positions and connections.
Components dependent on a culled component are culled.  Components
that reference culled components are renumbered.  Returns the updated
object, or returns null and sets the error code to indicate an error.
**/

#define IC_HASH				1
#define IC_DEP_ARRAY 			2
#define IC_SEGLIST   			3
#define IC_SORTED_LIST  		4
#define IC_ALL_MARKED 			5
#define IC_ALL_UNMARKED 		6

#define IC_ELEMENT_MARKED		DATA_INVALID
#define IC_ELEMENT_UNMARKED		DATA_VALID

#define IC_MARKS_INDICATE_VALID		0
#define IC_MARKS_INDICATE_INVALID	1

typedef struct invalidComponentHandle
{
    int       type;		/* current type, eg. dep array or hash	*/
    int	      myData;		/* did I allocate the data pointer?   	*/
    int	      nItems;		/* total number of items in component 	*/
    int	      nMarkedItems;	/* current number of marked items	*/
    char      *iName;		/* invalid component name		*/
    HashTable hash;		/* hash table pointer			*/
    Pointer   data;		/* data pointer				*/
    SegList   *seglist;		/* seg list pointer			*/
    int	      next;		/* next pointer for sequential queries  */
    Array     array;		/* original array			*/
    int	      sense;		/* do marks indicate valid?		*/

    /*
     * For iteration
     */
    int       *sortList;	/* for hashed handles			*/
    int	      sortListSize;
    int	      nextSlot;		
    int	      nextCand;
    int       nextMark;
    int       nextMarkI;
} *InvalidComponentHandle;

InvalidComponentHandle DXCreateInvalidComponentHandle(Object, Array, char *);
Error DXFreeInvalidComponentHandle(InvalidComponentHandle);
Error DXSaveInvalidComponent(Field, InvalidComponentHandle);
Array DXGetInvalidComponentArray(InvalidComponentHandle);
Error DXSetElementInvalid(InvalidComponentHandle, int);
Error DXSetElementValid(InvalidComponentHandle, int);
Error DXInvertValidity(InvalidComponentHandle);
Error DXSetAllValid(InvalidComponentHandle);
Error DXSetAllInvalid(InvalidComponentHandle);
int   DXIsElementInvalid(InvalidComponentHandle, int);
int   DXIsElementValid(InvalidComponentHandle, int);
int   DXIsElementValidSequential(InvalidComponentHandle, int);
int   DXIsElementInvalidSequential(InvalidComponentHandle, int);
int   DXGetInvalidCount(InvalidComponentHandle);
int   DXGetValidCount(InvalidComponentHandle);
Error DXInitGetNextInvalidElementIndex(InvalidComponentHandle);
Error DXInitGetNextValidElementIndex(InvalidComponentHandle);
int   DXGetNextValidElementIndex(InvalidComponentHandle);
int   DXGetNextInvalidElementIndex(InvalidComponentHandle);
Array DXForceInvalidArrayDependent(Array, Array);

#endif /* _DXI_INVALID_H_ */

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif
