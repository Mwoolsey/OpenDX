/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"


#ifndef _InteractorDefinition_h
#define _InteractorDefinition_h

//#include <Xm/Xm.h>

#include "ShadowedOutputDefinition.h"
//#include "SIAllocatorDictionary.h"


//
// Class name definition:
//
#define ClassInteractorDefinition	"InteractorDefinition"

//
// Forward definitions 
//
class ParameterDefinition;
class Interactor;

//
// InteractorDefinition class definition:
//				
class InteractorDefinition : public ShadowedOutputDefinition 
{
  private:
	
  protected:
    //
    // Protected member data:
    //

    //
    // Returns the StandIn allocator for this node.
    //
    virtual SIAllocator getSIAllocator();

#if 0
    //
    // Allocate a new Node of the corresponding type.
    //
    virtual Node *newNode(Network *net, int instance = -1); 
#endif

#if 0 // Moved to DrivenNode 6/30/93
    //
    // Mark all outputs as cache once with read-only cache attributes
    // (as per the semantics of data-driven interactors).
    //
    virtual void finishDefinition();
#endif

  public:
    //
    // Constructor:
    //
    InteractorDefinition() { }

    //
    // Destructor:
    //
    ~InteractorDefinition() { }
	
#if 0
    //
    // Allocate a new InteractorDefinition.  
    // This function is intended to be used as an allocator in
    // theNDAllocatorDictionary.
    //
    static NodeDefinition *AllocateDefinition();
#endif

#if 0 // Moved to DrivenNode 6/30/93
    virtual boolean isAllowedInMacro() { return FALSE; }
#endif


    //
    // Returns a pointer to the class name.
    //
    virtual const char* getClassName() { return ClassInteractorDefinition; }
};


#endif // _InteractorDefinition_h
