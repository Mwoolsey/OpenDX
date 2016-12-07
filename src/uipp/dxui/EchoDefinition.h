/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"


#ifndef _EchoDefinition_h
#define _EchoDefinition_h

#include <Xm/Xm.h>

#include "NodeDefinition.h"


//
// Class name definition:
//
#define ClassEchoDefinition	"EchoDefinition"

//
// Forward definitions 
//
class Network;

//
// EchoDefinition class definition:
//				
class EchoDefinition : public NodeDefinition 
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
    EchoDefinition() { }

    //
    // Destructor:
    //
    ~EchoDefinition() { }
	
    //
    // Allocate a new EchoDefinition.  
    // This function is intended to be used as an allocator in
    // theNDAllocatorDictionary.
    //
    static NodeDefinition *AllocateDefinition();

    //
    // Returns a pointer to the class name.
    //
    virtual const char* getClassName() { return ClassEchoDefinition; }
};


#endif // _EchoDefinition_h
