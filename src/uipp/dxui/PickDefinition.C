/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"


#include "PickDefinition.h"
#include "PickNode.h"

NodeDefinition *PickDefinition::AllocateDefinition()
{
    return new PickDefinition;
}


PickDefinition::PickDefinition() : ProbeDefinition()
{
}

Node *PickDefinition::newNode(Network *net, int instance)
{
    return new PickNode(this, net, instance);
}
