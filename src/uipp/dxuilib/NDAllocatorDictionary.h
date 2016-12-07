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

#ifndef _NDAllocatorDictionary
#define _NDAllocatorDictionary

#include <X11/Intrinsic.h>

#ifdef NO_CC_TEMPLATES
# include "DefaultingDictionary.h"
#else
# include "AllocatorDictionary.h"
#endif



class NodeDefinition;
class NDAllocatorDictionary;
class Node;

//
// Class name definition:
//
#define ClassNDAllocatorDictionary        "NDAllocatorDictionary"

//
// The type of function that is placed in AllocatorDictionary dictionaries.
//
typedef NodeDefinition* (*NDAllocator)(void);

extern NDAllocatorDictionary *theNDAllocatorDictionary; 

//
// A dictionary of name/function pairs. Lookup by name returns a pointer
// to a function that can allocate NodeDefinition for the given name.
// This is a DefaultingDictionary, so if the name is not found, the current
// default definition (function) is used.
//


class NDAllocatorDictionary : 
#ifndef NO_CC_TEMPLATES
        public AllocatorDictionary<NodeDefinition, NDAllocator>
#else
        public DefaultingDictionary
#endif
{

    public:

    static Symbol GetNodeNameSymbol;
    static Symbol SetNodeNameSymbol;

    //
    // Constructor 
    // Initializes the state of theNDAllocatorDictionary, installing a default
    // allocator and any special name/definition pairs.
    //
    NDAllocatorDictionary();

    //
    // Destructor 
    //
    ~NDAllocatorDictionary();

#ifdef NO_CC_TEMPLATES           //SMH Bring in templated stuff manually
    //
    // Record a function to be used to do allocation for the 'Instance'
    // associated with the given name.
    //
    boolean addAllocator(const char *name, NDAllocator alloc)
        {  ASSERT(this); return this->addDefinition(name,(const void*)alloc); }

    //
    // Push (intall) a default Allocator onto the stack.
    //
    boolean pushDefaultAllocator(NDAllocator nda)
        { ASSERT(this); return this->pushDefaultDefinition((const void*)nda); }

    //
    // Pop the default Allocator from the stack.
    //
    NDAllocator popDefaultAllocator()
        { ASSERT(this); return (NDAllocator) this->popDefaultDefinition(); }

    NDAllocator findAllocator(const char *name)
        { ASSERT(this); return (NDAllocator) this->findDefinition(name); }
    NDAllocator findAllocator(Symbol name)
        { ASSERT(this); return (NDAllocator) this->findDefinition(name); }
#endif          //SMH  end add templated stuff

    //
    // Find the allocator associated with name, or use the default allocator.
    // Call the allocator with no arguments, and return the result.
    //
    NodeDefinition* allocate(const char *name);
		
    //
    // Returns a pointer to the class name.
    // 
    const char* getClassName() 
		{ return ClassNDAllocatorDictionary; }

};



#endif // _NDAllocatorDictionary
