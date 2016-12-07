/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#ifndef _SelectorListNode_h
#define _SelectorListNode_h


#include "SelectionNode.h"
#include "List.h"

class Network;

//
// Class name definition:
//
#define ClassSelectorListNode	"SelectorListNode"

//
// SelectorListNode class definition:
//				
class SelectorListNode : public SelectionNode 
{
    friend class SelectorListToggleInteractor;// For [un]deferVisualNotification().

  private:
    //
    // Private member data:
    //

  protected:
    //
    // Protected member data:
    //

    //
    // Get a a SelectorListInstance instead of an InteractorInstance.
    //
    virtual InteractorInstance *newInteractorInstance();

    //
    // These define the initial values
    //
    virtual const char *getInitialValueList();
    virtual const char *getInitialStringList();

  public:
    //
    // Constructor:
    //
    SelectorListNode(NodeDefinition *nd, Network *net, int instnc);


    //
    // Destructor:
    //
    ~SelectorListNode();

    //
    // Determine if this node is a node of the given class
    //
    virtual boolean isA(Symbol classname);

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassSelectorListNode;
    }
};


#endif // _SelectorListNode_h


