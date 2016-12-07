/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"



#include "ModuleMessagingNode.h"

//
// Determine if this node is of the given class.
//
boolean ModuleMessagingNode::isA(Symbol classname)
{
    Symbol s = theSymbolManager->registerSymbol(ClassModuleMessagingNode);
    if (s == classname)
	return TRUE;
    else
	return this->Node::isA(classname);
}
