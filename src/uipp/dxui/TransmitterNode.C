/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#include "TransmitterNode.h"

#include "Ark.h"
#include "ConfigurationDialog.h"
#include "ListIterator.h"
#include "ReceiverNode.h"
#include "ErrorDialogManager.h"
#include "WarningDialogManager.h"
#include "InfoDialogManager.h"
#include "Network.h"
#include "DXVersion.h"
#include "Node.h"
#include "lex.h"

static boolean initializing = FALSE;

TransmitterNode::TransmitterNode(NodeDefinition *nd, Network *net, int instnc) :
    UniqueNameNode(nd, net, instnc)
{
}
boolean TransmitterNode::initialize()
{
    char s[100];

    sprintf(s, "wireless_%d", this->getInstanceNumber());

    initializing = TRUE;
    this->setLabelString(s);
    initializing = FALSE;

    return TRUE;
}

TransmitterNode::~TransmitterNode()
{
}


char *TransmitterNode::netNodeString(const char *prefix)
{
    char      *string = new char[200];
    char      *source;

    source = this->inputParameterNamesString(prefix);
    sprintf(string, "%s = %s;\n", this->getLabelString(), source);

    delete source;
    return string;

}

boolean TransmitterNode::setLabelString(const char *label)
{
    List *l;
    ListIterator li;


    if (EqualString(label, this->getLabelString()))
	return TRUE;

    if (initializing || this->getNetwork()->isReadingNetwork()) {

	if (initializing == FALSE) {
	    //
	    // Because of an old bug (hopefully fixed in 3.1.4), we need to scan
	    // for existing receivers with the same name.  These receivers are supposed
	    // to be placed later in the .net file than the transmitter.  Network::
	    // mergeNetworks() was not connecting up transmitters and receivers properly
	    // until 3.1.4.  So this check will work around a bug.
	    //
	    Network* net = this->getNetwork();
	    int net_major = net->getNetMajorVersion();
	    int net_minor = net->getNetMinorVersion();
	    int net_micro = net->getNetMicroVersion();
	    int net_version = VERSION_NUMBER( net_major, net_minor, net_micro);
	    int fixed_version = VERSION_NUMBER(3,1,1);
	    if (net_version < fixed_version) {
		// grab up any receivers that already have this name
		l = this->getNetwork()->makeClassifiedNodeList(ClassReceiverNode, FALSE);
		if (l) {
		    li.setList(*l);
		    ReceiverNode *node;
		    while ( (node = (ReceiverNode*)li.getNext()) ) {
			if (!node->isTransmitterConnected() &&
			    EqualString(node->getLabelString(), label))
			{
			    Network* net = this->getNetwork();
			    if (!net->checkForCycle (this, node)) {
				// link me to receiver
				new Ark(this, 1, node, 1);
			    } else {
				WarningMessage (
				    "This network contains Transmitter/Receiver\n"
				    "pair \"%s\" which would cause a cyclic \n"
				    "connection. Executing this network will yield \n"
				    "unpredictable results until this is fixed.",
				    this->getLabelString()
				);
				break;
			    }
			}
		    }
		    delete l;
		}
	    }
	}

	return this->UniqueNameNode::setLabelString(label);
    }


    if (!this->verifyRestrictedLabel(label))
	return FALSE;

    const char* conflict = this->getNetwork()->nameConflictExists(this, label);
    if (conflict) {
	ErrorMessage("A %s with name \"%s\" already exists.", conflict, label);
	return FALSE;
    }

    //
    // Check for cyclic connections before changing anyone's name.  (There is
    // similar code in ReceiverNode.C)  Do this only when NOT reading a network.
    // It's pointless to do this while reading a net file, because the ui doesn't
    // have the ability to write out .net file with a cycle.
    //
    if (this->getNetwork()->isReadingNetwork() == FALSE) {
	l = this->getNetwork()->makeClassifiedNodeList(ClassReceiverNode, FALSE);
	if (l) {
	    li.setList(*l);
	    ReceiverNode *node;
	    while ( (node = (ReceiverNode*)li.getNext()) ) {
		if (!node->isTransmitterConnected() &&
		    EqualString(node->getLabelString(), label)) {
		    Network* net = this->getNetwork();
		    if (net->checkForCycle (this, node)) {
			ErrorMessage (
			    "Unable to rename Transmitter \"%s\" to \"%s\"\n"
			    "because it would create a cyclic connection.",
			    this->getLabelString(), label
			);
			delete l;
			return FALSE;
		    }
		}
	    }
	    delete l;
	}
    }

    if (!this->UniqueNameNode::setLabelString(label))
	return FALSE;

    // rename our receivers
    l = (List*)this->getOutputArks(1);
    li.setList(*l);
    Ark *a;
    while ( (a = (Ark*)li.getNext()) )
    {
	int dummy;
	Node *rcvr = a->getDestinationNode(dummy);
	rcvr->setLabelString(label);
    }

    //
    // grab up any receivers that already have this name
    // We've already done the check for cyclic connections.
    //
    l = this->getNetwork()->makeClassifiedNodeList(ClassReceiverNode, FALSE);
    if (l) {
	li.setList(*l);
	ReceiverNode *node;
	while ( (node = (ReceiverNode*)li.getNext()) ) {
	    if (!node->isTransmitterConnected() &&
		EqualString(node->getLabelString(), label))
	    {
		// link me to receiver
		new Ark(this, 1, node, 1);
	    }
        }
	delete l;
    }

    return TRUE;
}
//
// Determine if this node is of the given class.
//
boolean TransmitterNode::isA(Symbol classname)
{
    Symbol s = theSymbolManager->registerSymbol(ClassTransmitterNode);
    if (s == classname)
	return TRUE;
    else
	return this->UniqueNameNode::isA(classname);
}



//
// Switch the node from one net to another.  Resolve any name space collisions.
//
void TransmitterNode::switchNetwork(Network *from, Network *to, boolean silently)
{

    const char* label = this->getLabelString();
    const char* conflict = to->nameConflictExists(this, label);
    char new_name[100];
    boolean name_change_required = FALSE;
    if (conflict) {
	sprintf(new_name, "wireless_%d", this->getInstanceNumber());
	name_change_required = TRUE;
    }

    //
    // If we would create a cyclic connection then reset the name 
    //
    List* l = to->makeClassifiedNodeList(ClassReceiverNode, FALSE);
    if (l) {
	ListIterator li;
	li.setList(*l);
	ReceiverNode *node;
	const char* cp = (name_change_required? new_name : label);
	while ( (node = (ReceiverNode*)li.getNext()) ) {
	    if (!node->isTransmitterConnected() &&
		EqualString(node->getLabelString(), cp))
	    {
		if (to->checkForCycle(this, node)) {
		    sprintf (new_name, "cyclic_connection_%d", this->getInstanceNumber());
		    name_change_required = TRUE;
		    break;
		}
	    }
	}
	delete l;
    }

    if (name_change_required) {
	if (!silently)
	    InfoMessage("Transmitter %s has been relabeled %s", label, new_name);
	this->setLabelString(new_name);
	label = this->getLabelString();
    }

    //
    // grab up any receivers that already have this name
    // We've already done the check for cyclic connections.
    //
    l = to->makeClassifiedNodeList(ClassReceiverNode, FALSE);
    if (l) {
	ListIterator li;
	li.setList(*l);
	ReceiverNode *node;
	while ( (node = (ReceiverNode*)li.getNext()) ) {
	    if (!node->isTransmitterConnected() &&
		EqualString(node->getLabelString(), label))
	    {
		// link me to receiver
		new Ark(this, 1, node, 1);
	    }
        }
	delete l;
    }

    this->UniqueNameNode::switchNetwork(from,to,silently);
}


boolean TransmitterNode::namesConflict (const char* his_label, const char* my_label,
    const char* his_classname)
{

    //
    // You can always match names with other ReceiverNodes.
    //
    if (EqualString (his_classname, ClassReceiverNode)) return FALSE;

    return this->UniqueNameNode::namesConflict (his_label, my_label, his_classname);
}
