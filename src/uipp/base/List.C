/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include <defines.h>




#include <memory.h>

#include "List.h"

#include "ListIterator.h"
#include "Link.h"


 List::List()
{
    //
    // Initialize instance data.
    //
    this->first = NUL(Link*);
    this->last  = NUL(Link*);
    this->size  = 0;
}


List::~List()
{	
    this->clear();
}


void List::clear()
{

    ASSERT(this);

    Link *next = this->first;
    while (next)
    {
	Link *link = next ;
	next = next->next;
	delete link;
    }

    this->first = NUL(Link*);
    this->last  = NUL(Link*);
    this->size  = 0;
}


boolean List::isMember(const void* element)
{
    Link* link;

    ASSERT(this);

    if (element == NUL(const void*))
    {
        //
	// If element is NULL, return FALSE.
        //
	return FALSE;
    }
    else
    {
	//
        // Otherwise, iterate through the list and
	// look for the matching element.
        //
	for (link = this->first; link; link = link->next)
	{
	    if (link->element == element)
	    {
		return TRUE;
	    }
	}

	//
	// If no match, return FALSE.
	//
	return FALSE;
    }
}


const void* List::getElement(const int position)
{
    Link* link;
    int   i;

    ASSERT(this);

    if (position < 1 OR position > this->size)
    {
	return NUL(const void*);
    }
    else if (position == 1)
    {
	return this->first->element;
    }
    else if (position == this->size)
    {
	return this->last->element;
    }
    else
    {
	for (link = this->first, i = 1;
	     link AND i < position;
	     link = link->next, i++)
	    ;
	ASSERT(link);

	return link->element;
    }

}


int List::getPosition(const void* element)
{
    Link* link;
    int   position;

    ASSERT(this);

    //
    // Look for the element in the list; if found, return its position.
    //
    for (link = this->first, position = 1; link; link = link->next, position++)
    {
	if (element == link->element)
	{
	    return position;
	}
    }

    //
    // Element not found; return zero.
    //
    return 0;
}


boolean List::insertElement(const void* element,
			    const int   position)
{
    Link* link;
    Link* before;
    int   i;

    ASSERT(this);

    if ((position < 1 OR position > this->size + 1) OR
	element == NUL(const void*))
    {
	//
	// Invalid element or bogus position: return unsuccessfully.
	//
	return FALSE;
    }
    else
    {
	//
	// Create a new link and assign the element to it.
	//
	link = new Link();
	ASSERT(link);
	link->element = element;

	//
	// Insert the link at the specified position.
	//
	if (this->size == 0)
	{
	    this->first = this->last = link;
	}
	else if (position == 1)
	{
	    link->next  = this->first;
	    this->first = link;
	}
	else if (position == this->size + 1)
	{
	    this->last->next = link;
	    this->last       = link;
	}
	else
	{
	    for (before = this->first, i = 1;
		 before AND i < position - 1;
		 before = before->next, i++)
		;
	    ASSERT(before);

	    link->next   = before->next;
	    before->next = link;
	}

	//
	// Increment the list count.
	//
	this->size++;

	//
	// Element inserted into the list successfully...
	//
	return TRUE;
    }
}


boolean List::appendElement(const void* element)
{
    ASSERT(this);
    return this->insertElement(element, this->size + 1);
}


boolean List::replaceElement(const void* element,
			     const int   position)
{
    Link* link;
    int   i;

    ASSERT(this);

    if ((position < 1 OR position > this->size) OR
	element == NUL(const void*))
    {
	//
	// Invalid element or bogus position: return unsuccessfully.
	//
	return FALSE;
    }
    else
    {
	//
	// Find the link in the specified position.
	//
	if (position == 1)
	{
	    link = this->first;
	}
	else if (position == this->size)
	{
	    link = this->last;
	}
	else
	{
	    for (link = this->first, i = 1;
		 link AND i < position;
		 link = link->next, i++)
		;
	    ASSERT(link);
	}

	//
	// Replace the element.
	//
	link->element = element;

	//
	// Element replaced successfully...
	//
	return TRUE;
    }
}


boolean List::deleteElement(const int position)
{
    Link* before;
    Link* to_delete;
    int   i;

    ASSERT(this);

    if (position < 1 OR position > this->size)
    {
	//
	// Bogus position: return unsuccessfully.
	//
	return FALSE;
    }
    else
    {
	if (position == 1)
	{
	    to_delete   = this->first;
	    this->first = to_delete->next;

	    if (this->size == 1)
	    {
		this->last = this->first;
	    }
	}
	else
	{
	    for (before = this->first, i = 1;
		 before AND i < position - 1;
		 before = before->next, i++)
		;
	    ASSERT(before);

	    to_delete    = before->next;
	    before->next = to_delete->next;

	    if (to_delete == this->last)
	    {
		this->last = before;
	    }
	}

	//
	// Free the memory.
	//
	delete to_delete;

	//
	// Decrement the list count.
	//
	this->size--;

	//
	// Element (link) deleted successfully.
	//
	return TRUE;
    }
}

boolean List::findElementValue(const void *element, isElementEqual f,int& index)
{
    ListIterator iterator(*this);
    void *entry;

    index = 0;
    while( (entry = (void*)iterator.getNext()) )
    {
        index++;
        if (f(element, (const void*)entry))
        {
            //
            // Duplicate found... return successfully.
            //
            return TRUE;
        }
    }

    return FALSE;

}

boolean List::mergeElementValue(const void *element, isElementEqual cmp,
                         dupElement dup, int& index)
{

    ASSERT(element);

    if (this->List::findElementValue(element, cmp, index)) {
        //
        // Duplicate string found... return unsuccessfully.
        //
        return FALSE;
    }

    //
    // Add (possibly a copy) to the end of the list.
    //
    if (dup)
    	this->List::appendElement(dup(element));
    else
    	this->List::appendElement(element);

    //
    // The index is the position of the last entry (i.e., list size).
    //
    index = this->List::getSize();

    //
    // Return successfully.
    //
    return TRUE;
}

boolean List::removeElement(const void *element) 
{
    int pos = this->getPosition(element);
    if (pos > 0) 
	return this->deleteElement(pos);
    return FALSE;
}
//
// Create a duplicate list with the same list items.
//
List *List::dup()
{
    List *l = new List;
    ListIterator iter(*this);
    const void *v;

    while ( (v = (const void*)iter.getNext()) )
	l->appendElement(v);

    return l;
}
//
// Sort the elements in the array of sd.  workspace is an array of
// elements that is used for temporary space.  Both sd and workspace
// have cnt elements in them.  
// If cmpfunc is given, then use it to compare element, otherwise sd[i].rank 
// is used to compare the elements. 
// Upon return, sd is sorted.
//
void List::SortOnData(const void **sd, const void **workspace, int cnt,
		ListElementCmpFunc cmpFunc)
{
    int i;

    if (cnt <= 1)
	return;

    if (cnt == 2) {	
	if (cmpFunc(sd[0],sd[1]) > 0) {
	    const void *tmp = sd[0];
	    sd[0] = sd[1];
	    sd[1] = tmp;
	}
	return;
    }

    //
    // Find an element to pivot on.
    //
    int pivotIndex = cnt/2;	// cnt >= 3, therefore pivotIndex >= 1
    if ((cmpFunc(sd[pivotIndex],sd[pivotIndex-1]) > 0) &&
	(cmpFunc(sd[pivotIndex],sd[pivotIndex+1]) < 0)) {
	//
	// We found a pivot in the middle of the list that is not the min
	// or max.  This is avoids O(n^2) performance when the list is
	// already sorted. 
	//
   	// for (i=0 ; i<cnt ; i++)
	// workspace[i] = sd[i];
    } else {
	const void *min = sd[0];
	const void *max = min; 
	const void *e; 
	//
	// Find the min and max elements
	//
	for (i=0 ; i<cnt ; i++) {
	    e = sd[i];
	    if (cmpFunc(e,min) < 0) 		/* e is less than min */
		min = e;
	    else if (cmpFunc(e,max) > 0) 	/* e is greater than max */
		max = e;
	    //workspace[i] = sd[i];
	}
	if (min == max)				/* All elements are the same */
	    return;				

	//
	// Now that we have the min and max, find a pivot that is preferrably
	// between the min and max.  If we can't find one between, the use 
	// one of the max elements.  Note, using the max (instead of the min)
	// impacts the sort below.  That is, we move equal elements to the
	// top of the list instead of into the bottom of the list.
	//
	for (i=0 ; i<cnt ; i++) {
	    const void *e = sd[i];
	    int cmpMin = cmpFunc(e,min);
	    int cmpMax = cmpFunc(e,max);
	    if ((cmpMin > 0) && (cmpMax < 0)) {	/* e is between min and max */
		pivotIndex = i;
		break;
	    } else if (cmpMax == 0) { 		/* settle for the max value */
		pivotIndex = i;
	    }
	}
    }

    //
    // Now that we are committed to a sub-sort, copy the data into the
    // workspace.
    //
    memcpy(workspace, sd, cnt * sizeof(const void*));

    //
    // Now sort the list into elements that are smaller and larger then
    // the pivotIndex value (cmpFunc != NULL), or the median value 
    // (rankFunc != NULL).
    //
    int bottom, top ; 
    const void *refData = workspace[pivotIndex];
    for (bottom=i=0, top=cnt-1; i<cnt ; i++) {
	int index; 
	const void *v = workspace[i];
	if ((i == pivotIndex) || (cmpFunc(v,refData) >= 0)) 
	    // v >= then pivot element (which may be the max element)
	    index = top--;
	else 
	    index = bottom++;
	sd[index] = v; 
    } 

    //
    // Now sort the two sublists (if they are of non-zero size).
    //
    if (bottom != cnt)
	List::SortOnData(sd,workspace,bottom,cmpFunc);
    if (bottom != 0)
	List::SortOnData(&sd[bottom],workspace,cnt-bottom,cmpFunc);


}
//
// Sort this list of elements using the cmpFunc to determine gt or lt..  
// Upon return, the list is sorted.
//
void List::sort(ListElementCmpFunc cmpFunc)
{
    int size = this->getSize();

    if (size <= 1)
	return;

    ASSERT(cmpFunc);

    const void **sd = new const void*[size * 2];

    ListIterator iter(*this);
    const void *v;
    int i = 0;
    while ( (v = (const void*)iter.getNext()) ) {
	sd[i] = v;
	i++;
    }

    List::SortOnData(sd, &sd[size], size, cmpFunc);

    this->clear();
    for (i=0 ; i<size ; i++)
	this->appendElement(sd[i]);

    delete sd; 
}
