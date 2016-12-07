/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"


#include "ProbeDefinition.h"
#include "ProbeNode.h"
#include "LabeledStandIn.h"

NodeDefinition *ProbeDefinition::AllocateDefinition()
{
    return new ProbeDefinition;
}


ProbeDefinition::ProbeDefinition() : 
    NodeDefinition()
{
}

SIAllocator ProbeDefinition::getSIAllocator()
{
   return LabeledStandIn::AllocateStandIn;
}


Node *ProbeDefinition::newNode(Network *net, int instance)
{
    return new ProbeNode(this, net, instance);
}
