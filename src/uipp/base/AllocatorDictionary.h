/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"





#ifndef _AllocatorDictionary
#define _AllocatorDictionary



#include "DefaultingDictionary.h"


//
// Class name definition:
//
#define ClassAllocatorDictionary	"AllocatorDictionary"


//
// Allocator class definition:
//				
#ifndef NO_CC_TEMPLATES
template<class Instance, class Allocator>
class AllocatorDictionary : public DefaultingDictionary
{
  private:
    //
    // Private member data:
    //

  protected:
    //
    // Protected member data:
    //


  public:
    //
    // Constructor:
    //
    AllocatorDictionary(){}

    //
    // Destructor:
    //
    ~AllocatorDictionary(){}

    //
    // Record a function to be used to do allocation for the 'Instance' 
    // associated with the given name.
    //
    boolean addAllocator(const char *name, Allocator alloc) 
    	{  ASSERT(this); return this->addDefinition(name,(const void*)alloc); }

    //
    // Push (intall) a default Allocator onto the stack.
    //
    boolean pushDefaultAllocator(Allocator nda)
	{ ASSERT(this); return this->pushDefaultDefinition((const void*)nda); }

    //
    // Pop the default Allocator from the stack.
    //
    Allocator popDefaultAllocator()
	{ ASSERT(this); return (Allocator) this->popDefaultDefinition(); }

    Allocator findAllocator(const char *name)
	{ ASSERT(this); return (Allocator) this->findDefinition(name); }
    Allocator findAllocator(Symbol name)
	{ ASSERT(this); return (Allocator) this->findDefinition(name); }
    
    //
    // The following function is to implemented by the subclass and should
    // provide the following functionality
    //
    // Lookup up a function to do 'Instance' allocations for the give name.
    // If the name has been registered look it up in this dictionary of 
    // allocators.
    //
    // Otherwise, use the default allocator (if one exists).
    // Returns a node definition for the given name, otherwise NULL.
    //
    // An Example...
    //
    // Instance *allocate(const char *name, ......) 
    //	{    Allocator  a;
    //       Instance *i = NUL(Instance*);
    //       a = this->findAllocator(name);
    //       if (a != NUL(Allocator)) i = a(......);
    //       return i;
    //  }

    //
    // Returns a pointer to the class name.
    // Making this virtual forces the subclass to create this
    // and hopefully allocate().
    //
    virtual const char* getClassName() = 0;
};
#endif    // end NO_CC_TEMPLATES
///////////////////////////////////////////////////////////////////////////////

#endif // _AllocatorDictionary
