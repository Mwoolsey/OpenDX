/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#include "ValueInstance.h"
#include "ValueNode.h"


ValueInstance::ValueInstance(ValueNode *n) : InteractorInstance(n)
{ 
}
	
ValueInstance::~ValueInstance() 
{ 
} 

#if 0  // This is moved to ValueInteractor on 4/9/93
//
// Get the current text value. For String and Value nodes, this is saved
// globally with the node, but for String/ValueListNodes, this is stored
// with the instance and is probably the currenlty selected list item.
//
const char *ValueInstance::getCurrentTextValue()
{
    Node *n = this->getNode();
    return n->getOutputValueString(1);
}

#endif
