/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#include "DXStrings.h"
#include "CmdEntry.h"
#include "Dictionary.h"
#include "DictionaryIterator.h"
#include "Command.h"

#include <ctype.h>

CmdEntry::CmdEntry(Dictionary *d, void *clientData)
{
    this->function = NULL;
    this->command = NULL;
    this->dictionary = d;
    this->clientData = clientData;
}
CmdEntry::CmdEntry(Command *c)
{
    this->function = NULL;
    this->command = c;
    this->dictionary = NULL;
    this->clientData = NULL;
}
CmdEntry::CmdEntry(CmdEntryFunction f, void *clientData)
{
    this->function = f;
    this->command = NULL;
    this->dictionary = NULL;
    this->clientData = clientData;
}

//
// Destructor:
//
CmdEntry::~CmdEntry()
{
    if (this->dictionary)
    {
	DictionaryIterator di(*this->dictionary);
	CmdEntry *e;
	while((e=(CmdEntry*)di.getNextDefinition()))
	    delete e;
	delete this->dictionary;
    }
}

//
// Function to handle a character string (a command) with this CmdEntry
//
boolean CmdEntry::parse(const char *c, int id)
{
    if (this->function)
	return (*this->function)(c, id, this->clientData);
    else if (this->command)
	return this->command->execute();
    else if (this->dictionary)
    {
	// the alpha C++ compiler can't handle the decl of next being
	// inside the for loop - you get an error on the sscanf line
	// about the variable 'next' being undefined.
	const char *next = c;
	for( ; next && *next && isspace(*next); ++next)
	    ;
	char s[100];
	sscanf(next, "%s", s);
	for(next = c + STRLEN(s); next && *next && isspace(*next); ++next)
	    ;
	CmdEntry *e = (CmdEntry*)this->dictionary->findDefinition(s);
	if (e)
	    return e->parse(next,id);
	else
	    return FALSE;
    }
    else
	ASSERT(0);
    return FALSE;
}

Dictionary *
CmdEntry::getDictionary()
{
    return this->dictionary;
}
