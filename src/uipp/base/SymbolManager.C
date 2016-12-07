/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include <defines.h>




#include "SymbolManager.h"


SymbolManager* theSymbolManager = new SymbolManager;


Symbol SymbolManager::registerSymbol(const char* symbolString)
{
    Symbol symbol;

    symbol = 0;
    if (NOT this->StringTable::addString(symbolString, symbol))
    {
	symbol = this->StringTable::findString(symbolString);
    }

    return symbol;
}


