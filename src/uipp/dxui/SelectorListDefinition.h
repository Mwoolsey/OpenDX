/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"


#ifndef _SelectorListDefinition_h
#define _SelectorListDefinition_h

#include "SelectionDefinition.h"


//
// Class name definition:
//
#define ClassSelectorListDefinition	"SelectorListDefinition"

//
// Forward definitions 
//


//
// SelectorListDefinition class definition:
//				
class SelectorListDefinition : public SelectionDefinition 
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
    SelectorListDefinition() { }

    //
    // Destructor:
    //
    ~SelectorListDefinition() { }
	
    //
    // Allocate a new SelectorListDefinition.  
    // This function is intended to be used as an allocator in
    // theNDAllocatorDictionary.
    //
    static NodeDefinition *AllocateDefinition();


    //
    // Returns a pointer to the class name.
    //
    virtual const char* getClassName() 
		{ return ClassSelectorListDefinition; }
};


#endif // _SelectorListDefinition_h
