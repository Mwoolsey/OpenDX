/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"


#ifndef _PickDefinition_h
#define _PickDefinition_h


#include "ProbeDefinition.h"


//
// Class name definition:
//
#define ClassPickDefinition	"PickDefinition"

//
// Referenced classes
class Network;

//
// PickDefinition class definition:
//				
class PickDefinition : public ProbeDefinition
{
  private:
    //
    // Private member data:
    //

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
    PickDefinition();

    //
    // Destructor:
    //
    ~PickDefinition(){}

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
	return ClassPickDefinition;
    }
};


#endif // _PickDefinition_h
