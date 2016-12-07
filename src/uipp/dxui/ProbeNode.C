/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"



#include "ProbeNode.h"
#include "Network.h"
#include "Parameter.h"
#include "ErrorDialogManager.h"
#include "ListIterator.h"
#include "ImageWindow.h"
#include "InfoDialogManager.h"

ProbeNode::ProbeNode(NodeDefinition *nd, Network *net, int instnc) :
    Node(nd, net, instnc)
{

}

ProbeNode::~ProbeNode()
{
    ImageWindow *iw;
    List *l = this->getNetwork()->getImageList();
    ListIterator iterator(*l);
    while ( (iw = (ImageWindow*)iterator.getNext()) ) 
    	iw->deleteProbe(this);
}

boolean ProbeNode::initialize()
{
    char string[20];
    sprintf(string, "%s_%d", this->getNameString(), this->getInstanceNumber());
    this->setLabelString((const char *) string);
    return TRUE;
}

void ProbeNode::initializeAfterNetworkMember()
{
    //
    // This stuff is done after the node is in the network to help out
    // ImageWindow and ViewControlDialog
    //
    ImageWindow *iw;
    List *l = this->getNetwork()->getImageList();
    ListIterator iterator(*l);
    while ( (iw = (ImageWindow*)iterator.getNext()) ) 
    	iw->addProbe(this);
}

boolean ProbeNode::setLabelString(const char *label)
{
    boolean result = this->Node::setLabelString(label);
    ImageWindow *iw;
    List *l = this->getNetwork()->getImageList();
    ListIterator iterator(*l);
    while ( (iw = (ImageWindow*)iterator.getNext()) ) 
    	iw->changeProbe(this);
    return result;
}

Parameter* ProbeNode::getOutput()
{
    return this->getOutputParameter(1);
}

//
// Modify the given cursor value.
// Cursor numbers are 0 based, -1 numbered cursor implies a new cursor
// appended to the current list.
//
void ProbeNode::setCursorValue(int cursor, double x, double y, double z)
{
    //
    // Set up the output value.
    //
    if(EqualString(this->getNameString(),"Probe"))
    {
	char  string[40];
	sprintf(string,"[%g,%g,%g]",x,y,z);
	// Since each cursor creation callback is followed by a MOVE
	// callback, don't send value on creation.
	this->setOutputValue(1,string, DXType::VectorType,
				cursor == -1 ? FALSE : TRUE);
    }
    else
	this->resetValueList(cursor, FALSE, x, y, z);

}

//
// Modify the given cursor value.
// Cursor numbers are 0 based, -1 numbered cursor implies a new cursor
// appended to the current list.
//
void ProbeNode::resetValueList(int cursor, boolean toDelete, 
				double x, double y, double z)
{
	char *newList, list[10];
	const char *p, *oldList= this->getOutputValueString(1);

	if (EqualString(oldList,"NULL")) {
	    sprintf(list, "{ }");
	    p = list;
	} else {
	    p = oldList;
  	}

	if (toDelete) {
	    if (p == list)
		newList = NULL; 
	    else  if (cursor == -1)
		newList = DXValue::DeleteListItem(p,
				DXType::VectorListType, cursor);
	    else
		newList = DXValue::DeleteListItem(p,
				DXType::VectorListType, cursor+1);
 	} else {
	    char value[64];
	    sprintf(value,"[%g %g %g]",x,y,z);
	    if (cursor == -1) {
		newList = DXValue::AppendListItem(p, value);
	    } else {
		newList = DXValue::ReplaceListItem(p, value, 
				    DXType::VectorListType, cursor+1);
	    }
	}

	if (!newList || STRLEN(newList) < 4)
	    this->setOutputValue(1,"NULL", DXType::VectorListType);
	else
	    this->setOutputValue(1,	newList,
				DXType::VectorListType,
				cursor == -1 ? FALSE : TRUE);
 
	if (newList) delete newList;
}


//
// Modify the given cursor value.
// Cursor numbers are 0 based, -1 numbered cursor implies a new cursor
// appended to the current list.
//
void ProbeNode::deleteCursor(int cursor)
{
    if(EqualString(this->getNameString(),"Probe"))
    {
//	this->setOutputValue(1,"NULL", DXType::VectorType);
	this->clearOutputValue(1);
    }
    else
    {
	this->resetValueList(cursor);
    }
}

char *ProbeNode::valuesString(const char *prefix)
{
    int       len = STRLEN(this->getOutputValueString(1));
    const char*outputParam = this->outputParameterNamesString(prefix);
    char      *string = new char[len + STRLEN(outputParam) + 10];

    sprintf(string,"%s = %s;\n", outputParam, this->getOutputValueString(1));

    return string;
}

char *ProbeNode::netNodeString(const char *prefix)
{
    char      *string = new char[1];

    string[0] = '\0';

    return string;
}
//
// Determine if this node is of the given class.
//
boolean ProbeNode::isA(Symbol classname)
{
    Symbol s = theSymbolManager->registerSymbol(ClassProbeNode);
    if (s == classname)
	return TRUE;
    else
	return this->Node::isA(classname);
}

//
// Switch the node from one net to another.  Resolve any name space collisions.
//
// FIXME: this label checking code is very similar to what is in 
// TransmitterNode::switchNetwork() and should be placed in a Node method
// that can be called from any of the subclasses.  I won't bother right
// before the 3.1 release.  dawood - 9/15/95
//
void ProbeNode::switchNetwork(Network *from, Network *to, boolean silently)
{
    boolean change_label = FALSE; 
    const char *nodename = this->getNameString();
    const char *curr_label = this->getLabelString();
    char name[128];
    int   instance;

    //
    // Reset the name if the label looks anything like the default, which
    // may have been set with an incorrect instance number during
    // initialize()
    //
    if ((sscanf("%s_%d",curr_label,name,&instance) == 2) &&
	EqualString(name,nodename) && 
	(instance != this->getInstanceNumber()))  {
	    change_label = TRUE;
    }

    //
    // Look for tools that have the same label.
    //
    if (!change_label) {
	List *curr = to->makeClassifiedNodeList(this->getClassName(), FALSE);
	if (curr) {
	    ListIterator iter(*curr);
	    Node *node;
	    while (!change_label && (node = (Node*)iter.getNext())) {
		if (EqualString(node->getLabelString(), curr_label))
		    change_label = TRUE;
	    }
	    delete curr;
	}
    }

    if (change_label) {
	char dflt_label[128];
	sprintf(dflt_label, "%s_%d", nodename, this->getInstanceNumber());
	if (!silently)
	    InfoMessage("%s %s has been relabeled %s", nodename, curr_label, dflt_label);
	this->setLabelString(dflt_label);
    }

    this->Node::switchNetwork(from,to,silently);
}

