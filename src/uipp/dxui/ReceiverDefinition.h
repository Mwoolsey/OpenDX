/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"



#ifndef _ReceiverDefinition_h
#define _ReceiverDefinition_h


#include "NodeDefinition.h"


//
// Class name definition:
//
#define ClassReceiverDefinition	"ReceiverDefinition"

//
// Referenced classes
class Network;

//
// ReceiverDefinition class definition:
//				
class ReceiverDefinition : public NodeDefinition
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
    ReceiverDefinition();

    //
    // Destructor:
    //
    ~ReceiverDefinition(){}

    //
    // Called after the NodeDefinition has been parsed out of the mdf file
    //
    virtual void finishDefinition();

    //
    // Create a new Module and NodeDefinition of 'this' type. 
    //
    static NodeDefinition *AllocateDefinition();

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassReceiverDefinition;
    }
};


#endif // _ReceiverDefinition_h
