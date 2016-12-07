/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include <defines.h>


#include "UniqueNameNode.h"
#include "SymbolManager.h"

boolean UniqueNameNode::isA(Symbol classname)
{
    Symbol s = theSymbolManager->registerSymbol (ClassUniqueNameNode);
    if (s == classname)
	return TRUE;
    else
	return this->Node::isA(classname);
}

boolean UniqueNameNode::namesConflict 
    (const char* his_label, const char* my_label, const char* his_classname)
{
    return EqualString(his_label, my_label); 
}
