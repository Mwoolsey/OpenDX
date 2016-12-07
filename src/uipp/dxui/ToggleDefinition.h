/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"


#ifndef _ToggleDefinition_h
#define _ToggleDefinition_h

#include <Xm/Xm.h>

#include "InteractorDefinition.h"


//
// Class name definition:
//
#define ClassToggleDefinition	"ToggleDefinition"

//
// Forward definitions 
//
class ParameterDefinition;
class Interactor;

//
// ToggleDefinition class definition:
//				
class ToggleDefinition : public InteractorDefinition 
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
    ToggleDefinition() { }

    //
    // Destructor:
    //
    ~ToggleDefinition() { }
	
    //
    // Allocate a new ToggleDefinition.  
    // This function is intended to be used as an allocator in
    // theNDAllocatorDictionary.
    //
    static NodeDefinition *AllocateDefinition();

#if SYSTEM_MACROS // 3/13/95 not yet
    virtual const char *getExecModuleNameString() { return "ToggleMacro"; }
#endif

    //
    // Returns a pointer to the class name.
    //
    virtual const char* getClassName() 
		{ return ClassToggleDefinition; }
};


#endif // _ToggleDefinition_h
