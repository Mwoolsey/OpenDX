/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"


#ifndef _SelectorDefinition_h
#define _SelectorDefinition_h

#include "SelectionDefinition.h"


//
// Class name definition:
//
#define ClassSelectorDefinition	"SelectorDefinition"

//
// Forward definitions 
//


//
// SelectorDefinition class definition:
//				
class SelectorDefinition : public SelectionDefinition 
{
  private:
	
  protected:
    //
    // Protected member data:
    //

    //
    // Allocate a new Node of the corresponding type.
    //
    virtual Node *newNode(Network *net, int instance = -1); 


  public:
    //
    // Constructor:
    //
    SelectorDefinition() { }

    //
    // Destructor:
    //
    ~SelectorDefinition() { }
	
    //
    // Allocate a new SelectorDefinition.  
    // This function is intended to be used as an allocator in
    // theNDAllocatorDictionary.
    //
    static NodeDefinition *AllocateDefinition();


    //
    // Returns a pointer to the class name.
    //
    virtual const char* getClassName() 
		{ return ClassSelectorDefinition; }
};


#endif // _SelectorDefinition_h
