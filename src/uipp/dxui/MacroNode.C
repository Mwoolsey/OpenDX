/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#include "MacroNode.h"
#include "MacroDefinition.h"
#include "Parameter.h"
#include "Network.h"
#include "DXApplication.h"

MacroNode::MacroNode(NodeDefinition *nd, Network *net, int instnc) :
    Node(nd, net, instnc)
{
}

MacroNode::~MacroNode()
{
    MacroDefinition *d = (MacroDefinition*)this->getDefinition();
    d->dereference(this);
}

boolean
MacroNode::initialize()
{
    MacroDefinition *d = (MacroDefinition*)this->getDefinition();
    d->reference(this);

    this->Node::initialize();
    return TRUE;
}
//
// Handle changing numbers and types of parameters. . .
void MacroNode::updateDefinition()
{
    this->Node::updateDefinition();
}

boolean MacroNode::sendValues(boolean     ignoreDirty)
{
    MacroDefinition *md = (MacroDefinition *)this->getDefinition();
    md->updateServer();
    return this->Node::sendValues(ignoreDirty);
}

void MacroNode::openMacro()
{
    MacroDefinition *md = (MacroDefinition *)this->getDefinition();
    md->openMacro();
}
//
// Determine if this node is of the given class.
//
boolean MacroNode::isA(Symbol classname)
{
    Symbol s = theSymbolManager->registerSymbol(ClassMacroNode);
    if (s == classname)
	return TRUE;
    else
	return this->Node::isA(classname);
}

boolean MacroNode::hasJavaRepresentation()
{
    return (EqualString(this->getNameString(), "WebOptions"));
}



boolean MacroNode::printInputAsJava(int i)
{
    if (EqualString(this->getNameString(), "WebOptions") == FALSE) return FALSE;
    boolean retval = FALSE;
    switch (i) {
	case 1:
	case 9:
	case 10:
	    retval = TRUE;
	    break;
    }
    return retval;
}
