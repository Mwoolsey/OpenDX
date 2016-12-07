/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"


#ifndef _Definition_h
#define _Definition_h


#include "Base.h"
#include "SymbolManager.h"


//
// Forward declarations 
//


//
// Definition class definition:
//				
class Definition : public Base 
{
  private:
    //
    // Private member data:
    //
    Symbol	key;	// Key to look up item by (symbol value found in
			// symbol table returned by getSymbolManager().

  protected:
    //
    // Protected member data:
    //

  public:
    //
    // Constructor(s):
    //
    Definition() { key = 0; }
    Definition(Symbol k) { key = k; }

    //
    // Destructor:
    // We don't free any string symbols because we don't know who references 
    // them.
    //
    virtual ~Definition() { } 


    //
    // Get the key for this definition 
    //
    Symbol getNameSymbol() { return key; }

    const char *getNameString() 
			{ return theSymbolManager->getSymbolString(key); }

    //
    // Set the key for this definition 
    //
    Symbol setName(const char *name)
	{
	    this->key = theSymbolManager->registerSymbol(name);
	    return this->key;
	}

    //
    // Make this pure virtual so no instances of this class are created. 
    //
    virtual const char* getClassName() = 0; 
};


#endif // _Definition_h
