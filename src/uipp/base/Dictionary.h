/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"


#ifndef _Dictionary_h
#define _Dictionary_h


#include "Base.h"
#include "SymbolManager.h"
#include "List.h"


//
// Forward declarations 
//
class List;
class DictionaryIterator;
class Dictionary;

//
// Dictionary class name 
//				
#define ClassDictionary "Dictionary"

//
// The items that are inserted in the dictionary
//
class DictionaryEntry {
    public:
        Symbol  name;
        void    *definition;
        DictionaryEntry() {}
        DictionaryEntry(Symbol key) { name = key; }
        DictionaryEntry(Symbol key, void *def) { name = key; definition = def;} 
}; 

//
// Dictionary class definition:
//				
class Dictionary : protected List 
{
    friend class DictionaryIterator;

  private:
    //
    // Private member data:
    //
    boolean isSorted;
    SymbolManager *symbolManager;

  protected:
    //
    // Protected member data:
    //

    //
    // Find the  DictionaryEntry in this dictionary for the given symbol
    // Returns NULL if not found.
    //
    DictionaryEntry *getDictionaryEntry(Symbol findkey);

  public:
    //
    // Constructor(s):
    //
    Dictionary(boolean sorted = TRUE, boolean privateSymbols = FALSE);

    //
    // Destructor:
    //
    ~Dictionary(); 

    //
    // Remove all entries from the dictionary.
    //
    void clear(); 

    //
    // Retrieve the symbol table associated with this instance.
    // By default, see the constructor, we use the global static symbol
    // table, but can use one local to this instance if desired.
    // This table holds a list of symbol/string pairs and is used to 
    // generate keys.
    //
    SymbolManager *getSymbolManager();

    //
    // Indicate how many items are in the dictionary
    //
    int getSize() { return this->List::getSize(); };
    int getPosition(Symbol s); 

    //
    //  Get the Nth definition in the dictionary, indexed from 1.  
    //
    const void *getDefinition(int n);
    Symbol 	getSymbol(int n);
    const char *getStringKey(int n);

    //
    // Find a symbol/value pair in the Dictionary table.
    // Return a pointer to a Definition class item or a derived class.
    //
	virtual void 	*findDefinition(Symbol findkey);
	virtual void 	*findDefinition(const char *n);
    //
    // Add the Definition pointed to by *definition to the dictionary;
    //
	virtual boolean addDefinition(const char *name, const void *definition);
	virtual boolean addDefinition(Symbol key, const void *definition);

    //
    // Replace the Definition pointed to by *definition in the dictionary
    // if it exists, otherwise just add it.
    // If a previous existed and olddef is not NULL, then return the old
    // definition in *olddef.
    //
	virtual boolean replaceDefinition(const char *name,
				const void *newdef,
				void **olddef = NULL);
	virtual boolean replaceDefinition(Symbol key, 
				const void *newdef,
				void **olddef = NULL);
    //
    // Replace the current elements with those in the given dictionary.
    // If olddefDict is not NULL, the return a dictionary containing the old 
    // definitions.
    //
    virtual boolean replaceDefinitions(Dictionary *d, 
					Dictionary *olddefDict = NULL);

    //
    // Remove the Definition pointed to by *definition from the dictionary;
    //
	virtual void	*removeDefinition(Symbol findkey);
	virtual void	*removeDefinition(const char *n);
	virtual void	*removeDefinition(const void *n);
    //
    // Return the class name of the object. 
    //
    virtual const char* getClassName() { return ClassDictionary; }
};


#endif // _Dictionary_h
