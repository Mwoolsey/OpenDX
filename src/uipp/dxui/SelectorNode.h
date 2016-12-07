/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#ifndef _SelectorNode_h
#define _SelectorNode_h


#include "SelectionNode.h"
#include "List.h"

class Network;

//
// Class name definition:
//
#define ClassSelectorNode	"SelectorNode"

//
// SelectorNode class definition:
//				
class SelectorNode : public SelectionNode 
{
    friend class SelectorInstance;	// For setSelectedOptionIndex().
    friend class SelectorInteractor;	// For [un]deferVisualNotification().
    friend class SelectorToggleInteractor;// For [un]deferVisualNotification().

  private:
    //
    // Private member data:
    //

  protected:
    //
    // Protected member data:
    //

    //
    // Get a a SelectorInstance instead of an InteractorInstance.
    //
    InteractorInstance *newInteractorInstance();

    //
    // These define the initial values
    //
    virtual const char *getInitialValueList();
    virtual const char *getInitialStringList();

  public:
    //
    // Constructor:
    //
    SelectorNode(NodeDefinition *nd, Network *net, int instnc);

    //
    // Destructor:
    //
    ~SelectorNode();

    //
    // Deselect all current selections and make the given index
    // the current selection.
    //
    void setSelectedOptionIndex(int index, boolean send = TRUE);

    //
    // If there is only one item selected, then return its index,
    // otherwise return 0.
    //
    int getSelectedOptionIndex();

    //
    // Determine if this node is a node of the given class
    //
    virtual boolean isA(Symbol classname);

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassSelectorNode;
    }
};


#endif // _SelectorNode_h


