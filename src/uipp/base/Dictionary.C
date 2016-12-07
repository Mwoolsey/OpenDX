/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"



//////////////////////////////////////////////////////////////////////////////

#include <string.h>

#include "DXStrings.h"
#include "Dictionary.h"
#include "Definition.h"
#include "ListIterator.h"
#include "DictionaryIterator.h"

#ifndef STRCMP
#ifdef NON_NULL_STRCMP
#define STRCMP(a,b)  ((a) ? ((b) ? strcmp(a,b) : strcmp(a,"")) : \
			    ((b) ? strcmp("",b) : 0))
#else
#define STRCMP(a,b) strcmp(a,b)
#endif
#endif

#if 0 /* Never used */
//
// Determine if two symbol values entries have the same name.  
//
static boolean _EqualName(const void *sv1, const void *sv2)
{
	return ((DictionaryEntry*)sv1)->name == ((DictionaryEntry*)sv2)->name;
}
#endif

//
// Determine if two symbol values entries have the same name.  
//
static boolean _EqualValue(const void *sv1, const void *sv2)
{
	return ((DictionaryEntry*)sv1)->definition == 
			((DictionaryEntry*)sv2)->definition;
}

Dictionary::Dictionary(boolean sorted, boolean privateSymbols )
{
    this->isSorted = sorted;
    if (privateSymbols)
	this->symbolManager = new SymbolManager;
    else
	this->symbolManager = NULL; 
}
//
// When we destroy 'this' we must a free the DictionaryEntries
//
Dictionary::~Dictionary()
{
    this->clear();
    if (this->symbolManager)
	delete this->symbolManager;
}

void Dictionary::clear()
{
    DictionaryEntry *de;
    ListIterator iterator(*this);

    while ((de=(DictionaryEntry*)iterator.getNext())) 
	    delete de;
    this->List::clear();

    // FIXME: should be clear the local symbol table.
}

//
// Retrieve the symbol table associated with this instance.
// By default, see the constructor, we use the global static symbol
// table, but can use one local to this instance if desired.
// This table holds a list of symbol/string pairs and is used to 
// generate keys.
//
SymbolManager *Dictionary::getSymbolManager() 
{ 
    if (this->symbolManager)
	return this->symbolManager;
    else
	return theSymbolManager;
}
//
//  Get the Nth definition in the dictionary, indexed from 1.
//
const void *Dictionary::getDefinition(int n)
{ 
    DictionaryEntry *de = (DictionaryEntry*)(this->getElement(n));
    return de? de->definition: NUL(void*);
}
//
//  Get the symbol for the Nth definition.
//
Symbol      Dictionary::getSymbol(int n)
{ 
    DictionaryEntry *de = (DictionaryEntry*)(this->getElement(n));
    return de ? de->name : 0; 
}
//
//  Get the string key for the n'th definition.
//
const char *Dictionary::getStringKey(int n)
{ 
    DictionaryEntry *de = (DictionaryEntry*)(this->getElement(n));
    return de ? this->getSymbolManager()->getSymbolString(de->name) : 
					NUL(const char*); 
}

int Dictionary::getPosition(Symbol s)
{
 	DictionaryEntry *de;
	ListIterator iterator(*this);
	int pos = 0;

	while ((de=(DictionaryEntry*)iterator.getNext())) {
	    pos++;
	    if (de->name == s)	
		return pos;	
 	}
	return 0;
}
//
// Add an instance of class Definition or class derived from Definition to
// this dictionary. Definitions are inserted in alphabetic order. 
// If a definition already exists for the given name then fail.
// FIXME: should we be adding a copy of definition instead of definition.
//
boolean Dictionary::addDefinition(const char *name, const void *def)
{
	const char *str;
	int	r, pos;
	Symbol s;
	DictionaryEntry *de, *newde;
	ListIterator iterator(*this);
	SymbolManager *sm = getSymbolManager();

		
	if (this->isSorted) {
	    pos = 1;
	    while ((de=(DictionaryEntry*)iterator.getNext())) {
		    str = sm->getSymbolString(de->name);
		    ASSERT(str);
		    r = STRCMP(str, name);
		    if (r == 0)
			return FALSE;
		    else if (r >= 0)
			break;
		    pos++;
	    }
	} else {
	    pos = 0;
	}
	s = sm->registerSymbol(name);
	newde = new DictionaryEntry(s,(void*)def);
   	if (pos == 0)
	    return this->appendElement(newde);
	else
	    return this->insertElement(newde, pos);
}
boolean Dictionary::addDefinition(Symbol name, const void *def)
{
	const char *str;
	SymbolManager *sm = this->getSymbolManager();

	str = sm->getSymbolString(name);
	if (!str)
	    return FALSE;

	return this->addDefinition(str, def);
}
//
// Replace the current element by the new one.
//
boolean Dictionary::replaceDefinition(const char *name, 
					const void *def, void **v)
{
	void *old_def = this->removeDefinition(name);
	if (v)
	    *v = old_def; 
	this->addDefinition(name, def);
	return TRUE;
}
boolean Dictionary::replaceDefinition(Symbol name, const void *def, void **v)
{
	const char *str;
	SymbolManager *sm = this->getSymbolManager();

	str = sm->getSymbolString(name);
	if (!str)
	    return FALSE;

	return this->replaceDefinition(str, def, v);
}
//
// Replace the current elements with those in the given dictionary. 
// If olddefList is not NULL, the return a list containing the old definitions. 
//
boolean Dictionary::replaceDefinitions(Dictionary *d, Dictionary *olddefDict)
{
    DictionaryEntry *de;
    ListIterator iterator(*d);
	    
    while ((de=(DictionaryEntry*)iterator.getNext())) {
	Symbol sym = de->name;
	void *olddef;
	void *def = de->definition;
	if (!this->replaceDefinition(sym,def, &olddef))
	    return FALSE;
	if (olddefDict && olddef)
	    olddefDict->addDefinition(sym,olddef);
	    
    }
    
   return TRUE;
}
//
// Find the Dictionary Element that has the give key value.
//
DictionaryEntry* Dictionary::getDictionaryEntry(Symbol findkey)
{
	DictionaryEntry *d;

        ListIterator di(*this);
	while ((d=(DictionaryEntry*)di.getNext())) {
	    if (d->name == findkey)
		break;
	}
	return d;	
}

//
// Find the Definition associated with the given symbol. 
// If the symbol is not found return FALSE, otherwise TRUE.
//
void *Dictionary::findDefinition(Symbol findkey)
{
	void	*d;
	DictionaryEntry *de;
					
	if (findkey == 0)
	    return NULL;

	if (!(de = this->getDictionaryEntry(findkey)))
		return NUL(void*);

	d =  de->definition;
	
	return d;	
}
//
// Find the Definition associated with the given string 
// Return a pointer to the symbol found there. 
//
void *Dictionary::findDefinition(const char *n)
{
	SymbolManager *sm = this->getSymbolManager();
 	Symbol s = sm->getSymbol(n);
 	if (s == 0)
	    return NULL;
	else
	    return this->findDefinition(s);
}
//
// Find the definition associated with the given symbol. 
// If the symbol is not found return FALSE, otherwise TRUE.
//
void *Dictionary::removeDefinition(Symbol findkey)
{
	int	index;
	void	*d;
	DictionaryEntry *de;
					

	if (!(de = this->getDictionaryEntry(findkey)))
		return NUL(void*);

	d =  de->definition;

	index = this->List::getPosition(de);
	ASSERT(index);

	if (!this->deleteElement(index))
		return NUL(void*);

	delete de;
	
	return d; 
}
//
// Find the definition associated with the given string  and remove it from
// the Dictionary.
// Return a pointer to the Definition found there. 
//
void *Dictionary::removeDefinition(const char *n)
{
	SymbolManager *sm = this->getSymbolManager();
 	Symbol s = sm->getSymbol(n);
 	if (s == 0)
	    return NULL;
	else
	    return this->removeDefinition(s);
}
//
// Find the definition associated with the given string  and remove it from
// the Dictionary.
// Return a pointer to the Definition found there. 
//
void *Dictionary::removeDefinition(const void *def)
{
	int	index;
	DictionaryEntry *d;
	DictionaryEntry de(0,(void*)def);	

	if (!this->findElementValue((const void*)&de, _EqualValue, index)) 
		return NUL(void*);

	d = (DictionaryEntry*)this->getElement(index);
	ASSERT(d);
	
	index = this->List::getPosition(d);
	ASSERT(index);

	boolean r = this->deleteElement(index);
	ASSERT(r);

	delete d;

	return (void*)def;
	
}

