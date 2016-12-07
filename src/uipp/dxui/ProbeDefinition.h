/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"



#ifndef _ProbeDefinition_h
#define _ProbeDefinition_h


#include "NodeDefinition.h"


//
// Class name definition:
//
#define ClassProbeDefinition	"ProbeDefinition"

//
// Referenced classes
class Network;

//
// ProbeDefinition class definition:
//				
class ProbeDefinition : public NodeDefinition
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
    ProbeDefinition();

    //
    // Destructor:
    //
    ~ProbeDefinition(){}

    //
    // Create a new Module and NodeDefinition of 'this' type. 
    //
    static NodeDefinition *AllocateDefinition();

    virtual boolean isAllowedInMacro() { return FALSE; }
    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassProbeDefinition;
    }
};


#endif // _ProbeDefinition_h
