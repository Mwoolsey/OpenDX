/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#ifndef _ActiveItemDictionary_h
#define _ActiveItemDictionary_h


#include "Dictionary.h"


//
// Class name definition:
//
#define ClassActiveItemDictionary	"ActiveItemDictionary"


//
// ActiveItemDictionary class definition:
//				
class ActiveItemDictionary : public Dictionary 
{
    friend class DictionaryIterator;

  private:
    //
    // Private member data:
    //
    Symbol active;		// The 'active' item in the Dictionary.

  protected:
    //
    // Protected member data:
    //


  public:
    //
    // Constructor:
    //
    ActiveItemDictionary() { active = 0; }

    //
    // Destructor:
    //
    ~ActiveItemDictionary() {} 

    void setActiveItem(Symbol s) 
		{ ASSERT(this->findDefinition(s)); this->active = s;}
    Symbol getActiveItem() { return this->active; }
    int isActive() { return this->active != 0; }

    void *getActiveDefinition() { return this->findDefinition(this->active); }

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassActiveItemDictionary;
    }
};


#endif // _ActiveItemDictionary_h
