/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"


#include "VectorDefinition.h"
#include "ScalarNode.h"

Node *VectorDefinition::newNode(Network *net, int inst)
{
    ScalarNode *n = new ScalarNode(this, net, inst, TRUE, 3);
    return n;
}

NodeDefinition *
VectorDefinition::AllocateDefinition()
{
    return (NodeDefinition*) new VectorDefinition;
}
