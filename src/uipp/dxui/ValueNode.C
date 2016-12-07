/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




 
#include "ValueNode.h"
#include "ValueInstance.h"

//
// The Constructor... 
//
ValueNode::ValueNode(NodeDefinition *nd, Network *net, int instnc) :
                        NondrivenInteractorNode(nd, net, instnc)
{ 

}

//
// Destructure: needs to delete all its instances. 
//
ValueNode::~ValueNode()
{
}

boolean ValueNode::initialize()
{
    Type t = this->setOutputValue(1,"NULL", DXType::UndefinedType, FALSE);
    return t == DXType::UndefinedType ? FALSE : TRUE;
}

InteractorInstance* ValueNode::newInteractorInstance()
{
    ValueInstance *ii;

    ii = new ValueInstance(this);

    return ii;
}
//
// Determine if this node is of the given class.
//
boolean ValueNode::isA(Symbol classname)
{
    Symbol s = theSymbolManager->registerSymbol(ClassValueNode);
    if (s == classname)
	return TRUE;
    else
	return this->NondrivenInteractorNode::isA(classname);
}
