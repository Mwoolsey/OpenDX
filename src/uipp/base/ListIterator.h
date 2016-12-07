/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#ifndef _ListIterator_h
#define _ListIterator_h


#include "Base.h"

class Link;
class List;


//
// Class name definition:
//
#define ClassListIterator	"ListIterator"


//
// ListIterator class definition:
//				
class ListIterator : public Base
{
  private:
    //
    // Private member data:
    //
    List* list;		// pointer to the associated list
    Link* next;		// pointer to next link in the list
    int	  position;	// position of next link in the list
    
  public:
    //
    // Constructors:
    //
    ListIterator();
    ListIterator(List& list);

    //
    // Destructor:
    //
    ~ListIterator(){}

    //
    // Sets the list to be iterated.
    //
    void setList(List& list);

    //
    // Returns the element in the list corresponding to the next position;
    // return NULL if end of list reached.
    // Side effect: bumps up the next position.
    //
    const void* getNext();

    //
    // Sets the next position.
    //
    void setPosition(int position);

    //
    // Returns the next position.
    //
    int getPosition()
    {
	return this->position;
    }

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassListIterator;
    }
};


#endif // _ListIterator_h
