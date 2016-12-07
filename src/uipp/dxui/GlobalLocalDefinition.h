/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"


#ifndef _GlobalLocalDefinition_h
#define _GlobalLocalDefinition_h

#include <Xm/Xm.h>

#include "NodeDefinition.h"


//
// Class name definition:
//
#define ClassGlobalLocalDefinition	"GlobalLocalDefinition"

//
// Forward definitions 
//
class Network;

//
// GlobalLocalDefinition class definition:
//				
class GlobalLocalDefinition : public NodeDefinition 
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
    GlobalLocalDefinition() { }

    //
    // Destructor:
    //
    ~GlobalLocalDefinition() { }
	
    //
    // Allocate a new GlobalLocalDefinition.  
    // This function is intended to be used as an allocator in
    // theNDAllocatorDictionary.
    //
    static NodeDefinition *AllocateDefinition();

    //
    // Returns a pointer to the class name.
    //
    virtual const char* getClassName() { return ClassGlobalLocalDefinition; }
};


#endif // _GlobalLocalDefinition_h
