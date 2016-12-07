/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"


#include "ScalarDefinition.h"
#include "ScalarNode.h"

Node *ScalarDefinition::newNode(Network *net, int inst)
{
    ScalarNode *n = new ScalarNode(this, net, inst);
    return n;
}

NodeDefinition *
ScalarDefinition::AllocateDefinition()
{
    return (NodeDefinition*) new ScalarDefinition;
}

