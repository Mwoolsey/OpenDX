/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include <defines.h>




#include "ListIterator.h"
#include "List.h"
#include "Link.h"


ListIterator::ListIterator()
{	
    //
    // Sets the default state, i.e., the next position is set to
    // to the first position.
    //
    this->list     = NUL(List*);
    this->next     = NUL(Link*);
    this->position = 0;
}


ListIterator::ListIterator(List& list)
{
    //
    // Sets the default state, i.e., the next position is set to
    // to the first position.
    //
    this->list     = &list;
    this->next     = list.first;
    this->position = 1;
}


void ListIterator::setList(List& list)
{
    this->list     = &list;
    this->next     = list.first;
    this->position = 1;
}


const void* ListIterator::getNext()
{
    Link* link;

    if (this->next)
    {
	link       = this->next;
	this->next = link->next;
	this->position++;

	return link->element;
    }
    else
    {
	return NUL(const void*);
    }
}


void ListIterator::setPosition(int position)
{
    Link* link;
    int   i;

    if (position <= 1)
    {
	this->position = 1;
	this->next     = this->list->first;
    }
    else if (position >= this->list->size)
    {
	this->position = this->list->size;
	this->next     = this->list->last;
    }
    else
    {
	for (link = this->list->first, i = 1;
	     link AND i < position;
	     link = link->next, i++)
	    ;
	ASSERT(link);

	this->next     = link;
	this->position = position;
    }
}


