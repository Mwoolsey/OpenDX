/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"



#ifndef _MacroParameterDefinition_h
#define _MacroParameterDefinition_h


#include "List.h"
#include "NodeDefinition.h"


//
// Class name definition:
//
#define ClassMacroParameterDefinition	"MacroParameterDefinition"

//
// Referenced classes
class Network;
class MacroParameterNode;

//
// MacroParameterDefinition class definition:
//				
class MacroParameterDefinition : public NodeDefinition
{
  private:
    //
    // Private member data:
    //

  protected:
    //
    // Protected member data:
    //
    virtual CDBAllocator getCDBAllocator();

    //
    // Allocate a new Node of the corresponding type.
    //
    virtual Node *newNode(Network *net, int instance = -1); 

  public:
    //
    // Constructor:
    //
    MacroParameterDefinition();

    //
    // Destructor:
    //
    ~MacroParameterDefinition();

    //
    // Create a new Module of 'this' type. 
    //
    static NodeDefinition *AllocateDefinition();

    void finishDefinition();
    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassMacroParameterDefinition;
    }
};


#endif // _MacroParameterDefinition_h
