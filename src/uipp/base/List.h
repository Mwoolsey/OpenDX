/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"



#ifndef _List_h
#define _List_h


#include "Base.h"


//
// Class name definition:
//
#define ClassList	"List"


//
// Referenced classes:
//
class Link;
class ListIterator;

//  A function to compare element values.
typedef boolean (*isElementEqual)(const void*, const void*);

//  A function to make a copy of an element. 
typedef void* (*dupElement)(const void *);

// A function that will compare to elements of the list and return 0 if
// equal, less than 0 if v<ref, or greater than 0 if v>ref.
typedef int (*ListElementCmpFunc) (const void *v, const void *ref);

class List : virtual public Base
{
    //
    // Friend class:
    //
    friend class ListIterator;

  private:
    //
    // Private member data:
    //
    Link* first;	// pointer to the first link
    Link* last;		// pointer to the last link
    int	  size;		// number of links in the list
    //
    // Sort the elements in the array of sd.  workspace is an array of
    // elements that is used for temporary space.  Both sd and 
    // workspace have cnt elements in them.
    // If cmpfunc is given, then use it to compare element, otherwise sd[i].rank
    // is used to compare the elements.
    // Upon return, sd is sorted.
    //
    void SortOnData(const void **sd, const void **workspace, int cnt, 
					ListElementCmpFunc cmpFunc);


  public:
    //
    // Constructor:
    //
    List();

    //
    // Destructor:
    //
    ~List();

    //
    // Clears the list of all elements (links).
    //
    virtual void clear();

    //
    // Returns the the number of elements in the list.
    //
    int getSize()
    {
	ASSERT(this);
	return this->size;
    }

    //
    // Returns TRUE if the specified element is a member of the list;
    // FALSE, otherwise.
    //
    boolean isMember(const void* element);

    //
    // Returns the element in the specified position within the list;
    // returns NULL if element not found.
    //
    const void* getElement(const int position);

    //
    // Returns the position of the specified element in the list;
    // returns zero if the element not found in the list.
    //
    int getPosition(const void* element);

    //
    // Inserts an element in the specified position within the list.
    // Note that an element cannot be a NULL value.
    // Returns TRUE if the element has been successfully inserted.
    //
    virtual boolean insertElement(const void* element,
			  const int   position);

    //
    // Replaces an element in the specified position within the list.
    // Returns TRUE if the element has been successfully replaced.
    //
    virtual boolean replaceElement(const void* element,
			   const int   position);

    //
    // Appends an element to the end of the list.
    // Returns TRUE if the element has been successfully appended.
    //
    boolean appendElement(const void* element);

    //
    // Finds the element with the specified position in the list.
    // Returns TRUE if the specified element is found. 
    //
    boolean findElementValue(const void *element,isElementEqual f,int& index);

    //
    // Adds the given element to the list if it is not already on the list
    // according to the value yielded from the comparison made by 'cmp' between
    // elements on the list and the new element.   If the element value is not
    // already on the list, then it is added to the list.  If 'dup' is not
    // NULL, then a copy of the element is made before adding to the list.
    // Returns TRUE if the specified element is added to the list and sets
    // index to the position in the list. 
    //
    boolean mergeElementValue(const void *element, isElementEqual cmp, 
			 dupElement dup, int& index);


    //
    // Deletes the element with the specified position in the list.
    // Returns TRUE if the specified element is successfully deleted.
    //
    virtual boolean deleteElement(const int position);

    //
    // Deletes the element if it exists in the list. 
    // Returns TRUE if the specified element is successfully deleted.
    //
    boolean removeElement(const void *element);

    //
    // Create a duplicate list with the same list items.
    //
    virtual List *dup();

    //
    // Sort this list of elements.  Using a comparison function. 
    // Upon return, the list is sorted.
    //
    void sort(ListElementCmpFunc cmpFunc);

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassList;
    }
};


#endif /* _List_h */
