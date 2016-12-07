/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#ifndef _DictionaryIterator_h
#define _DictionaryIterator_h


#include "Base.h"
#include "Link.h"
#include "ListIterator.h"
#include "Dictionary.h"


//
// Class name definition:
//
#define ClassDictionaryIterator	"DictionaryIterator"


//
// DictionaryIterator class definition:
//				
class DictionaryIterator : protected ListIterator
{
  private:
    //
    // Private member data:
    //

  public:
    //
    // Constructors:
    //
    DictionaryIterator() {}
    DictionaryIterator(Dictionary& list) : ListIterator(list) {}

    //
    // Destructor:
    //
    ~DictionaryIterator()  {}

    //
    // Returns the element in the list corresponding to the next position;
    // return NULL if end of list reached.
    // Side effect: bumps up the next position.
    //
    const void* getNextDefinition() {
    		const void *v;
    		v = ListIterator::getNext();
    		if (v) v = ((DictionaryEntry*)v)->definition;
		return v; }

    Symbol getNextSymbol() {
    		const void *v;
    		v = ListIterator::getNext();
    		return v? ((DictionaryEntry*)v)->name: 0; }

    void setList(Dictionary &d) { this->ListIterator::setList(d); }

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassDictionaryIterator;
    }
};


#endif // _DictionaryIterator_h
