/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
#include "UndoMove.h"
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
#include "UndoNode.h"

char UndoMove::OperationName[] = "move";

//
// Never ever store a pointer to an object.  Do that, and you're bound
// to crash because you can't be sure you can keep track of the the life cycle
// of the object.  It might get deleted and you won't know.  We keep track
// of information that we can use to look up an object.  If something happens
// to the object, then we won't be able to look it up but that's OK.  The undo
// will fail but we won't crash.
//

//
// In order to track movements in the canvas we'll store
// things in this object and keep them in a list.  It would be
// better to make this a generic Undo object and then make UndoMove
// a subclass of Undo, but I'm not planning on really implementing Undo/Redo.
//
UndoMove::UndoMove (EditorWindow* editor, UIComponent* uic) :
    UndoableAction(editor)
{
    uic->getXYPosition (&this->x, &this->y);
}

UndoStandInMove::UndoStandInMove(EditorWindow* editor, StandIn* standIn, const char* className, int instance) : UndoMove(editor, standIn)
{
    this->className = className;
    this->instance_number = instance;
}

void UndoStandInMove::undo(boolean first_in_list)
{
    int i;
    Node* n = UndoNode::LookupNode(this->editor, this->className, this->instance_number);
    if (!n) return ;
    StandIn* si = n->getStandIn();
    ASSERT(si);
    si->manage();
    si->setXYPosition (this->x, this->y);

    int input_count = n->getInputCount();
    Ark* arc;
    for (i=1; i<=input_count; i++) {
	if (!n->isInputVisible(i)) continue;
	List* arcs = (List*)n->getInputArks(i);
	if (!arcs) continue;
	ListIterator iter(*arcs);
	while (arc = (Ark*)iter.getNext()) {
	    ArkStandIn* asi = arc->getArkStandIn();
	    delete asi;
	    si->addArk (this->editor, arc);
	}
    }

    int output_count = n->getOutputCount();
    for (i=1; i<=output_count; i++) {
	if (!n->isOutputVisible(i)) continue;
	List* arcs = (List*)n->getOutputArks(i);
	if (!arcs) continue;
	ListIterator iter(*arcs);
	while (arc = (Ark*)iter.getNext()) {
	    ArkStandIn* asi = arc->getArkStandIn();
	    delete asi;
	    si->addArk(this->editor, arc);
	}
    }
    if (first_in_list) this->editor->selectNode (n, TRUE, TRUE);
}

boolean UndoStandInMove::canUndo()
{
    Node* n = UndoNode::LookupNode (this->editor, this->className, this->instance_number);
    if (!n) return FALSE;
    StandIn* si = n->getStandIn();
    if (!si) return FALSE;

    // determine if the destination location is empty
    int w,h;
    si->getXYSize(&w,&h);
    WorkSpace* ews = si->getWorkSpace();

    return (ews->isEmpty(this->x, this->y, w, h));
}

void UndoStandInMove::prepare()
{
    Node* n = UndoNode::LookupNode (this->editor, this->className, this->instance_number);
    if (!n) return ;
    StandIn* si = n->getStandIn();
    if (!si) return ;
    si->unmanage();
}

void UndoStandInMove::postpare()
{
    Node* n = UndoNode::LookupNode (this->editor, this->className, this->instance_number);
    if (!n) return ;
    StandIn* si = n->getStandIn();
    if (!si) return ;
    if (si->isManaged()) return ;
    si->manage();
}

UndoDecoratorMove::UndoDecoratorMove(EditorWindow* editor, VPEAnnotator* dec) :
    UndoMove (editor, dec)
{
    // store the id in the decorator and use that to look up the decorator in undo().
    this->instance = dec->getInstanceNumber();
}

UndoDecoratorMove::~UndoDecoratorMove()
{
}

void UndoDecoratorMove::undo(boolean first_in_list)
{
    VPEAnnotator* dec = this->lookupDecorator();
    if (dec) {
	dec->manage();
	dec->setXYPosition (this->x, this->y);
	if (first_in_list) this->editor->selectDecorator (dec, TRUE, TRUE);
    }
}

VPEAnnotator* UndoDecoratorMove::lookupDecorator()
{
    Network* net = this->editor->getNetwork();
    List* decorators = (List*)net->getDecoratorList();
    // can't be null
    ListIterator iter(*decorators);
    VPEAnnotator* dec;
    while (dec=(VPEAnnotator*)iter.getNext()) {
	if (this->instance == dec->getInstanceNumber()) {
	    return dec;
	}
    }
}

boolean UndoDecoratorMove::canUndo()
{
    VPEAnnotator* dec = this->lookupDecorator();
    if (!dec) return FALSE;
    int w,h;
    dec->getXYSize(&w,&h);
    WorkSpace* ews = dec->getWorkSpace();

    return (ews->isEmpty(this->x, this->y, w, h));
}

void UndoDecoratorMove::prepare()
{
    VPEAnnotator* dec = this->lookupDecorator();
    if (dec) dec->unmanage();
}
void UndoDecoratorMove::postpare()
{
    VPEAnnotator* dec = this->lookupDecorator();
    if (!dec) return ;
    if (dec->isManaged()) return ;
    dec->manage();
}
