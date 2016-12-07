/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"


#include "VectorListDefinition.h"
#include "ScalarListNode.h"

Node *VectorListDefinition::newNode(Network *net, int inst)
{
    ScalarListNode *n = new ScalarListNode(this, net, inst, TRUE, 3);
    return n;
}

NodeDefinition *
VectorListDefinition::AllocateDefinition()
{
    return (NodeDefinition*) new VectorListDefinition;
}

