/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"







// This is the dictionary of StandIn allocating functions.
// It is a defaulting dictionary which means that if a name/allocator 
// pair is not found then a default allocator is used (if installed).
// The constructor is responsible for setting the intial state of this
// type of dictionary.  
//
#include "SIAllocatorDictionary.h"
#include "StandIn.h"

//
// Declaring an instance, instead of generating one at startup with 'new'
// may make for faster startup.
//
///* static */ SIAllocatorDictionary __theSIAllocatorDictionary;
SIAllocatorDictionary *theSIAllocatorDictionary = NULL; 

//
// Constructor function to do one-time initializations for the
// dictionary that contains 'named' functions that create StandIns.
//
SIAllocatorDictionary::SIAllocatorDictionary() 
{
 //   theSIAllocatorDictionary = this;
    this->pushDefaultAllocator(StandIn::AllocateStandIn);

}
//
// 
//
SIAllocatorDictionary::~SIAllocatorDictionary() 
{
    this->popDefaultAllocator();
  //  theSIAllocatorDictionary = NULL;
}

//
// Find the allocator associated with name, or use the default allocator.
// Call the allocator with the arguments w and n, and return the result.
//
StandIn *SIAllocatorDictionary::allocate(Symbol namsym, WorkSpace *w, Node *n)
{
    StandIn *i = NUL(StandIn*);
    SIAllocator a = this->findAllocator(namsym);
    if (a != NUL(SIAllocator)) i = a(w, n);
    return i;
}
#if 0
StandIn *SIAllocatorDictionary::allocate(const char *name, Widget w, Node *n)
{
   Symbol symbol = this->getSymbol(name);

   if (!symbol)
	return NUL(StandIn*);
   return this->allocate(symbol,name,w,n);
}
#endif

