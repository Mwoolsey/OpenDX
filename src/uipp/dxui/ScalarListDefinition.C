/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"


#include "ScalarListDefinition.h"
#include "ScalarListNode.h"

Node *ScalarListDefinition::newNode(Network *net, int inst)
{
    ScalarListNode *n = new ScalarListNode(this, net, inst, FALSE, 1);
    return n;
}

NodeDefinition *
ScalarListDefinition::AllocateDefinition()
{
    return (NodeDefinition*) new ScalarListDefinition;
}
