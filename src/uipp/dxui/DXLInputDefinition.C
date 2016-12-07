/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"


#include "DXLInputDefinition.h"
#include "DXLInputNode.h"
#include "ItalicLabeledStandIn.h"

NodeDefinition *DXLInputDefinition::AllocateDefinition()
{
    return new DXLInputDefinition;
}


DXLInputDefinition::DXLInputDefinition() : 
    NodeDefinition()
{
}

Node *DXLInputDefinition::newNode(Network *net, int instance)
{
    DXLInputNode *d = new DXLInputNode(this, net, instance);
    return d;
}

SIAllocator DXLInputDefinition::getSIAllocator()
{
   return ItalicLabeledStandIn::AllocateStandIn;
}
