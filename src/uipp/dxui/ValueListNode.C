/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#include "ValueListNode.h"
#include "ValueListInstance.h"

//
// The Constructor... 
//
ValueListNode::ValueListNode(NodeDefinition *nd, Network *net, int instnc) :
                        ValueNode(nd, net, instnc)
{ 
}

//
// Destructure: needs to delete all its instances. 
//
ValueListNode::~ValueListNode()
{
}
InteractorInstance* ValueListNode::newInteractorInstance()
{
    ValueListInstance *ii;

    ii = new ValueListInstance(this);

    return ii;
}
//
// Determine if this node is of the given class.
//
boolean ValueListNode::isA(Symbol classname)
{
    Symbol s = theSymbolManager->registerSymbol(ClassValueListNode);
    if (s == classname)
	return TRUE;
    else
	return this->ValueNode::isA(classname);
}
