/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"


#include "ValueListDefinition.h"
#include "ValueListNode.h"

Node *ValueListDefinition::newNode(Network *net, int inst)
{
    InteractorNode *n = new ValueListNode(this, net, inst);
    return n;
}

NodeDefinition *ValueListDefinition::AllocateDefinition()
{
    return (NodeDefinition*) new ValueListDefinition();
}

