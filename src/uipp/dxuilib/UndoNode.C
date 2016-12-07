/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
#include "UndoNode.h"
#include "EditorWindow.h"
#include "UIComponent.h"
#include "StandIn.h"
#include "Node.h"
#include "List.h"
#include "ListIterator.h"
#include "Ark.h"
#include "ArkStandIn.h"
#include "VPEAnnotator.h"
#include "Network.h"
#include "WorkSpace.h"

//
// Never ever store a pointer to an object.  Do that, and you're bound
// to crash because you can't be sure you can keep track of the the life cycle
// of the object.  It might get deleted and you won't know.  We keep track
// of information that we can use to look up an object.  If something happens
// to the object, then we won't be able to look it up but that's OK.  The undo
// will fail but we won't crash.
//

UndoNode::UndoNode(EditorWindow* editor, Node* n) : UndoableAction(editor)
{
    this->className = n->getNameString();
    this->instance_number = n->getInstanceNumber();
    StandIn* si = n->getStandIn();
    ASSERT (si);
    this->workSpace = (EditorWorkSpace*)si->getWorkSpace();
}

Node* UndoNode::lookupNode()
{
    return UndoNode::LookupNode(this->editor, this->className, this->instance_number);
}
Node* UndoNode::LookupNode(EditorWindow* editor, const char* className, int instance_number)
{

    Network* net = editor->getNetwork();

    // list will contain nodes that aren't in the current vpe
    // page, so we'll have to check that before using any of the nodes' info.
    List *node_list = net->makeNamedNodeList(className);
    if (!node_list) return NUL(Node*);

    ListIterator iter(*node_list);
    Node* n=NUL(Node*);
    Node* lookedup = NUL(Node*);
    while (n=(Node*)iter.getNext()) {
	if (n->getInstanceNumber() == instance_number) {
	    lookedup = n;
	    break;
	}
    }
    delete node_list;
    return lookedup;
}
