/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"



#ifndef _DisplayDefinition_h
#define _DisplayDefinition_h


#include "DrivenDefinition.h"


//
// Class name definition:
//
#define ClassDisplayDefinition	"DisplayDefinition"

//
// Referenced classes
class Network;

//
// DisplayDefinition class definition:
//				
class DisplayDefinition : public DrivenDefinition
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
    DisplayDefinition();

    //
    // Destructor:
    //
    ~DisplayDefinition(){}

    //
    // Create a new Module and NodeDefinition of 'this' type. 
    //
    static NodeDefinition *AllocateDefinition();

    virtual boolean isAllowedInMacro() { return TRUE; }

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassDisplayDefinition;
    }
};


#endif // _DisplayDefinition_h
