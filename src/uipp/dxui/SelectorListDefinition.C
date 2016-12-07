/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"


#include "SelectorListDefinition.h"
#include "SelectorListNode.h"

Node *SelectorListDefinition::newNode(Network *net, int inst)
{
    SelectorListNode *n = new SelectorListNode(this, net, inst);
    return n;
}

NodeDefinition *SelectorListDefinition::AllocateDefinition()
{
    return new SelectorListDefinition;
}
