/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"



#include "DXLOutputDefinition.h"
#include "DXLOutputNode.h"
#include "ItalicLabeledStandIn.h"

NodeDefinition *DXLOutputDefinition::AllocateDefinition()
{
    return new DXLOutputDefinition;
}


DXLOutputDefinition::DXLOutputDefinition() : 
    DrivenDefinition()
{
}

Node *DXLOutputDefinition::newNode(Network *net, int instance)
{
    DXLOutputNode *d = new DXLOutputNode(this, net, instance);
    return d;
}

SIAllocator DXLOutputDefinition::getSIAllocator()
{
   return ItalicLabeledStandIn::AllocateStandIn;
}
