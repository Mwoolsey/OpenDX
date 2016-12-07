/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#ifndef _SymbolManager_h
#define _SymbolManager_h


#include "StringTable.h"


//
// Class name definition:
//
#define ClassSymbolManager	"SymbolManager"


//
// Symbol type definition:
//
typedef long Symbol;


//
// SymbolManager class definition:
//				
class SymbolManager : private StringTable
{
  public:
    //
    // Constructor:
    //
    SymbolManager(){}

    //
    // Destructor:
    //
    ~SymbolManager(){}

    //
    // Registers the symbol string and returns a unique symbol ID
    // for the symbol string.
    //
    Symbol registerSymbol(const char* symbolString);

    //
    // Returns the corresponding symbol ID for the specified string.
    // Returns 0 if the string is not registered.
    //
    Symbol getSymbol(const char* symbolString)
    {
	return this->StringTable::findString(symbolString);
    }

    //
    // Returns the corresponding symbol string for the specified symbol.
    // Returns NULL if an invalid symbol value has been specified.
    //
    const char* getSymbolString(const Symbol symbol)
    {
	return this->StringTable::getString(symbol);
    }

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassSymbolManager;
    }
};


extern SymbolManager* theSymbolManager;


#endif // _SymbolManager_h
