/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/UIConfig.h"
#include "../base/defines.h"

#ifndef _SIAllocatorDictionary
#define _SIAllocatorDictionary


#ifdef NO_CC_TEMPLATES
# include "DefaultingDictionary.h"
#else
# include "AllocatorDictionary.h"
#endif


class StandIn;
class SIAllocatorDictionary;
class Node;
class WorkSpace;

//
// Class name definition:
//
#define ClassSIAllocatorDictionary        "SIAllocatorDictionary"

//
// The type of function that is placed in AllocatorDictionary dictionaries.
//
typedef StandIn* (*SIAllocator)(WorkSpace*, Node*);

extern SIAllocatorDictionary *theSIAllocatorDictionary; 

//
// A dictionary of name/function pairs. Lookup by name returns a pointer
// to a function that can allocate StandIn for the given name.
// This is a DefaultingDictionary, so if the name is not found, the current
// default definition (function) is used.
//


class SIAllocatorDictionary : 
#ifndef NO_CC_TEMPLATES
        public AllocatorDictionary<StandIn, SIAllocator>
#else
        public DefaultingDictionary
#endif
{

    public:

    //
    // Constructor 
    // Initializes the state of theSIAllocatorDictionary, intalling a default
    // allocator and any special name/standin pairs.
    //
    SIAllocatorDictionary();

    //
    // Destructor 
    //
    ~SIAllocatorDictionary();

#ifdef NO_CC_TEMPLATES           //SMH Bring in templated stuff manually
    //
    // Record a function to be used to do allocation for the 'Instance'
    // associated with the given name.
    //
    boolean addAllocator(const char *name, SIAllocator alloc)
        {  ASSERT(this); return this->addDefinition(name,(const void*)alloc); }

    //
    // Push (intall) a default Allocator onto the stack.
    //
    boolean pushDefaultAllocator(SIAllocator nda)
        { ASSERT(this); return this->pushDefaultDefinition((const void*)nda); }

    //
    // Pop the default Allocator from the stack.
    //
    SIAllocator popDefaultAllocator()
        { ASSERT(this); return (SIAllocator) this->popDefaultDefinition(); }

    SIAllocator findAllocator(const char *name)
        { ASSERT(this); return (SIAllocator) this->findDefinition(name); }
    SIAllocator findAllocator(Symbol name)
        { ASSERT(this); return (SIAllocator) this->findDefinition(name); }
#endif          //SMH  end add templated stuff

    //
    // Find the allocator associated with name, or use the default allocator.
    // Call the allocator with the arguments w and n, and return the result.
    //
    //StandIn* allocate(const char *name, WorkSpace *w, Node *n);
    StandIn* allocate(Symbol namsym, WorkSpace *w, Node *n);
		
    //
    // Returns a pointer to the class name.
    // 
    const char* getClassName() 
		{ return ClassSIAllocatorDictionary; }

};



#endif // _SIAllocatorDictionary
