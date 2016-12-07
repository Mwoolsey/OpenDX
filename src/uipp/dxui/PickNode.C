/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#include "PickNode.h"
#include "ImageNode.h"
#include "Network.h"
#include "Parameter.h"
#include "ErrorDialogManager.h"
#include "ImageWindow.h"
#include "ListIterator.h"

#define PICK_NAME 1
#define PICK_FROM 2
#define PICK_PTS  3
#define PICK_INDEX 4

PickNode::PickNode(NodeDefinition *nd, Network *net, int instnc) :
    ProbeNode(nd, net, instnc)
{
}

PickNode::~PickNode()
{
    ImageWindow *iw;
    List *l = this->getNetwork()->getImageList();
    ListIterator iter(*l);
    while ( (iw = (ImageWindow*)iter.getNext()) ) 
        iw->deletePick(this);
}

boolean PickNode::initialize()
{
    char string[64];
    sprintf(string, "%s_%d", this->getNameString(), this->getInstanceNumber());
    this->setLabelString((const char *) string);

    this->setInputValue(PICK_NAME, this->getLabelString(), 
			DXType::UndefinedType, FALSE);
    return TRUE;
}

void PickNode::initializeAfterNetworkMember()
{
    //
    // This stuff is done after the node is in the network to help out
    // ImageWindow and ViewControlDialog
    //
    ImageWindow *iw;
    List *l = this->getNetwork()->getImageList();
    ListIterator iter(*l);
    while ( (iw = (ImageWindow*)iter.getNext()) ) 
        iw->addPick(this);
}

char *PickNode::valuesString(const char *prefix)
{
    return Node::valuesString(prefix);
}

char *PickNode::netNodeString(const char *prefix)
{
    return Node::netNodeString(prefix);
}

boolean PickNode::setLabelString(const char *label)
{
    boolean result = this->Node::setLabelString(label);
    ImageWindow *iw;
    List *l = this->getNetwork()->getImageList();
    ListIterator iter(*l);
    while ( (iw = (ImageWindow*)iter.getNext()) ) 
        iw->changePick(this);
    return result;
}
//
// Determine if this node is of the given class.
//
boolean PickNode::isA(Symbol classname)
{
    Symbol s = theSymbolManager->registerSymbol(ClassPickNode);
    if (s == classname)
	return TRUE;
    else
	return this->ProbeNode::isA(classname);
}

void PickNode::setCursorValue(int cursor,
							  double x, double y, double /* z ignored */)
{
	char s[100];
	const char *value = this->getInputValueString(PICK_PTS);

	this->incrementIndex();

	sprintf(s, "[%g, %g]", x, y);
	if (this->isInputDefaulting(PICK_PTS) || 
		value == NULL || EqualString(value, "NULL") || *value == '\0')
	{
		char newValue[102];
		strcpy(newValue, "{");
		strcat(newValue, s);
		strcat(newValue, "}");
		this->setInputValue(PICK_PTS, newValue);
	}
	else if (cursor == -1)
	{
		char *newValue = new char[STRLEN(value) + 102];
		strcpy(newValue, value);
		char *brace = strchr(newValue, '}');
		ASSERT(brace);
		*brace = '\0';
		strcat(newValue, ",");
		strcat(newValue, s);
		strcat(newValue, "}");
		this->setInputValue(PICK_PTS, newValue);
		delete newValue;
	}
	else
	{
		char *newValue = new char[STRLEN(value) + 102];
		const char *term = strchr(value, '[');
		strcpy (newValue, "{");
		for (int i = 0; i < cursor; ++i)
		{
			ASSERT(term);
			int termLen;
			const char *end = strchr(term, ']');
			ASSERT(end);
			termLen = end - term + 1;
			strncat(newValue, term, termLen);
			strcat(newValue, ",");
			term = strchr(end, '[');
		}
		strcat(newValue, s);
		if (term)
		{
			strcat(newValue, ",");
			strcat(newValue, term);
		}
		else
			strcat(newValue, "}");

		this->setInputValue(PICK_PTS, newValue);
		delete newValue;
	}

}

void PickNode::resetCursor()
{
    if (this->isInputDefaulting(PICK_PTS))
	return;
    const char *v = this->getInputValueString(PICK_PTS);
    if (*v == '\0' || EqualString(v, "NULL"))
	return;
    this->incrementIndex();
    this->setInputValueQuietly(PICK_PTS, "NULL");
}

void PickNode::incrementIndex()
{
    if (this->isInputDefaulting(PICK_INDEX))
	this->setInputValue(PICK_INDEX, "0");
    else
    {
	const char *s = this->getInputValueString(PICK_INDEX);
	int   i;
	sscanf(s, "%d", &i);
	++i;
	char newS[100];
	sprintf(newS, "%d", i);
	this->setInputValueQuietly(PICK_INDEX, newS, DXType::UndefinedType);
    }
}

void PickNode::pickFrom(Node *n)
{
    if (n)
	this->setInputValueQuietly(PICK_FROM,
				   ((ImageNode*)n)->getPickIdentifier());
    else
    {
	this->setInputValueQuietly(PICK_FROM, "NULL");
	this->setInputValueQuietly(PICK_PTS, "NULL");
    }
}


 

