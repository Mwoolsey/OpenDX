/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"


#include "SelectorDefinition.h"
#include "SelectorNode.h"

Node *SelectorDefinition::newNode(Network *net, int inst)
{
    SelectorNode *n = new SelectorNode(this, net, inst);
    return n;
}

NodeDefinition *SelectorDefinition::AllocateDefinition()
{
    return new SelectorDefinition;
}
