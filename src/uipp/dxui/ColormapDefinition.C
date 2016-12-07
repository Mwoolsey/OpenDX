/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"


#include "ColormapDefinition.h"
#include "ColormapNode.h"

NodeDefinition *ColormapDefinition::AllocateDefinition()
{
    return new ColormapDefinition;
}


ColormapDefinition::ColormapDefinition() : DrivenDefinition()
{
}

Node *ColormapDefinition::newNode(Network *net, int instance)
{
    ColormapNode *n = new ColormapNode(this, net, instance);
    return n;
}
