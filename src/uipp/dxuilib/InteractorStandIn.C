/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#include "InteractorNode.h"
#include "InteractorStandIn.h"
#include "Interactor.h"
#include "ControlPanel.h"
#include "Network.h"

//
// Static allocator found in theSIAllocatorDictionary
//
StandIn *InteractorStandIn::AllocateStandIn(WorkSpace *w, Node *node)
{
    StandIn *si = new InteractorStandIn(w,node);
    si->createStandIn();
    return si;
}
//
// Called when the StandIn has been selected by the Editor.
//
void InteractorStandIn::handleSelectionChange(boolean selected)
{
    int i;
    Node *n = this->node;
    Network *net = n->getNetwork();
    ControlPanel *cp;

    //
    // Tell all the network's control panel's that the selection
    // has changed.
    //
    for (i=1 ; (cp = net->getPanelByIndex(i)) ; i++) {
	cp->handleNodeStatusChange(node, selected ? 
				Interactor::InteractorSelected : 
				Interactor::InteractorDeselected);
    }

    // And finally, call the super class's method. 
    this->StandIn::handleSelectionChange(selected);
}


