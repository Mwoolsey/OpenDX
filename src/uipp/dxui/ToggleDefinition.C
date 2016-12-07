/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"


#include "ToggleDefinition.h"
#include "ToggleNode.h"

Node *ToggleDefinition::newNode(Network *net, int inst)
{
    ToggleNode *n = new ToggleNode(this, net, inst);
    return n;
}

NodeDefinition *
ToggleDefinition::AllocateDefinition()
{
    return (NodeDefinition*) new ToggleDefinition;
}

