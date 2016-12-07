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

#ifndef _CDBAllocatorDictionary
#define _CDBAllocatorDictionary

#include <X11/Intrinsic.h>


#ifdef NO_CC_TEMPLATES
# include "DefaultingDictionary.h"
#else
# include "AllocatorDictionary.h"
#endif


class ConfigurationDialog;
class CDBAllocatorDictionary;
class Node;

//
// Class name definition:
//
#define ClassCDBAllocatorDictionary        "CDBAllocatorDictionary"

//
// The type of function that is placed in AllocatorDictionary dictionaries.
//
typedef ConfigurationDialog* (*CDBAllocator)(Widget, Node*);

extern CDBAllocatorDictionary *theCDBAllocatorDictionary; 

//
// A dictionary of name/function pairs. Lookup by name returns a pointer
// to a function that can allocate ConfigurationDialog for the given name.
// This is a DefaultingDictionary, so if the name is not found, the current
// default definition (function) is used.
//


class CDBAllocatorDictionary : 
#ifndef NO_CC_TEMPLATES
        public AllocatorDictionary<ConfigurationDialog, CDBAllocator>
#else
        public DefaultingDictionary
#endif
{

    public:

    //
    // Constructor 
    // Initializes the state of theCDBAllocatorDictionary, installing a default
    // allocator and any special name/dialog pairs.
    //
    CDBAllocatorDictionary();

    //
    // Destructor 
    //
    ~CDBAllocatorDictionary();

#ifdef NO_CC_TEMPLATES           //SMH Bring in templated stuff manually
    //
    // Record a function to be used to do allocation for the 'Instance'
    // associated with the given name.
    //
    boolean addAllocator(const char *name, CDBAllocator alloc)
        {  ASSERT(this); return this->addDefinition(name,(const void*)alloc); }

    //
    // Push (intall) a default Allocator onto the stack.
    //
    boolean pushDefaultAllocator(CDBAllocator nda)
        { ASSERT(this); return this->pushDefaultDefinition((const void*)nda); }

    //
    // Pop the default Allocator from the stack.
    //
    CDBAllocator popDefaultAllocator()
        { ASSERT(this); return (CDBAllocator) this->popDefaultDefinition(); }

    CDBAllocator findAllocator(const char *name)
        { ASSERT(this); return (CDBAllocator) this->findDefinition(name); }
    CDBAllocator findAllocator(Symbol name)
        { ASSERT(this); return (CDBAllocator) this->findDefinition(name); }
#endif          //SMH  end add templated stuff

    //
    // Find the allocator associated with name, or use the default allocator.
    // Call the allocator with the arguments name, w, and n, and return the 
    // result.
    //
    ConfigurationDialog* allocate(Symbol namesym, Widget w, Node *n);
    // ConfigurationDialog* allocate(Symbol name, Widget w, Node *n);
		
    //
    // Returns a pointer to the class name.
    // 
    const char* getClassName() 
		{ return ClassCDBAllocatorDictionary; }

};



#endif // _CDBAllocatorDictionary
