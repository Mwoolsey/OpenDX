/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"


#include "ValueDefinition.h"
#include "ValueNode.h"

Node *ValueDefinition::newNode(Network *net, int inst)
{
    InteractorNode *n = new ValueNode(this, net, inst);
    return n;
}

NodeDefinition *ValueDefinition::AllocateDefinition()
{
    return (NodeDefinition*) new ValueDefinition();
}

