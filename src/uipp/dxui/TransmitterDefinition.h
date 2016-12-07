/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"



#ifndef _TransmitterDefinition_h
#define _TransmitterDefinition_h


#include "NodeDefinition.h"


//
// Class name definition:
//
#define ClassTransmitterDefinition	"TransmitterDefinition"

//
// Referenced classes
class Network;

//
// TransmitterDefinition class definition:
//				
class TransmitterDefinition : public NodeDefinition
{
  private:
    //
    // Private member data:
    //

  protected:
    //
    // Protected member data:
    //
    virtual SIAllocator getSIAllocator();

    //
    // Allocate a new Node of the corresponding type.
    //
    virtual Node *newNode(Network *net, int instance = -1); 

  public:
    //
    // Constructor:
    //
    TransmitterDefinition();

    //
    // Destructor:
    //
    ~TransmitterDefinition(){}

    //
    // Create a new Module and NodeDefinition of 'this' type. 
    //
    static NodeDefinition *AllocateDefinition();

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassTransmitterDefinition;
    }
};


#endif // _TransmitterDefinition_h
