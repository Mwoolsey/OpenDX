/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"


#ifndef _SequencerDefinition_h
#define _SequencerDefinition_h


#include "ShadowedOutputDefinition.h"


//
// Class name definition:
//
#define ClassSequencerDefinition	"SequencerDefinition"

//
// Referenced classes
class Network;

//
// SequencerDefinition class definition:
//				
class SequencerDefinition : public ShadowedOutputDefinition
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
    SequencerDefinition() { }

    //
    // Destructor:
    //
    ~SequencerDefinition() { }

    //
    // Create a new Module and NodeDefinition of 'this' type. 
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
	return ClassSequencerDefinition;
    }
};


#endif // _SequencerDefinition_h
