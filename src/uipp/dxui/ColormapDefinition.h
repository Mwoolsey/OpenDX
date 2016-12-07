/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"


#ifndef _ColormapDefinition_h
#define _ColormapDefinition_h


#include "DrivenDefinition.h"


//
// Class name definition:
//
#define ClassColormapDefinition	"ColormapDefinition"

//
// Referenced classes
class Network;

//
// ColormapDefinition class definition:
//				
class ColormapDefinition : public DrivenDefinition
{
  private:
    //
    // Private member data:
    //

  protected:
    //
    // Allocate a new Node of the corresponding type.
    //
    virtual Node *newNode(Network *net, int instance = -1); 

  public:
    //
    // Constructor:
    //
    ColormapDefinition();

    //
    // Destructor:
    //
    ~ColormapDefinition(){}

    //
    // Create a new Module and DrivenDefinition of 'this' type. 
    //
    static NodeDefinition *AllocateDefinition();

#if 0 // Moved to DrivenNode 6/30/93
    virtual boolean isAllowedInMacro() { return FALSE; }
#endif

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassColormapDefinition;
    }
};


#endif // _ColormapDefinition_h
