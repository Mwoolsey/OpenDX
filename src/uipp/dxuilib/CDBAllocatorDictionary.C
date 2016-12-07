/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"



//
// This is the dictionary of ConfigurationDialog allocating functions.
// It is a defaulting dictionary which means that if a name/allocator 
// pair is not found then a default allocator is used (if installed).
// The constructor is responsible for setting the intial state of this
// type of dictionary.  
//
#include "CDBAllocatorDictionary.h"
#include "ConfigurationDialog.h"

//
// Declaring an instance, instead of generating one at startup with 'new'
// may make for faster startup.
//
// /* static */ CDBAllocatorDictionary __theCDBAllocatorDictionary;
CDBAllocatorDictionary *theCDBAllocatorDictionary = NULL; 

//
// Constructor function to do one-time initializations for the
// dictionary that contains 'named' functions that create ConfigurationDialogs.
//
CDBAllocatorDictionary::CDBAllocatorDictionary() 
{
    this->pushDefaultAllocator(
		ConfigurationDialog::AllocateConfigurationDialog);
}
//
// 
//
CDBAllocatorDictionary::~CDBAllocatorDictionary() 
{
    this->popDefaultAllocator();
}
//
// Find the allocator associated with name, or use the default allocator.
// Call the allocator with the arguments name, w, and n, and return the 
// result.
//
ConfigurationDialog *CDBAllocatorDictionary::allocate(Symbol namesym,
		Widget w, Node *n)
{
    ConfigurationDialog *i = NUL(ConfigurationDialog*);
    CDBAllocator a = this->findAllocator(namesym);
    if (a != NUL(CDBAllocator)) i = a(w, n);
    return i;
}
