/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"


#include "TransmitterDefinition.h"
#include "TransmitterNode.h"
#include "ItalicLabeledStandIn.h"

NodeDefinition *TransmitterDefinition::AllocateDefinition()
{
    return new TransmitterDefinition;
}


TransmitterDefinition::TransmitterDefinition() : 
    NodeDefinition()
{
}


Node *TransmitterDefinition::newNode(Network *net, int instance)
{
    TransmitterNode *d = new TransmitterNode(this, net, instance);
    return d;
}

SIAllocator TransmitterDefinition::getSIAllocator()
{
   return ItalicLabeledStandIn::AllocateStandIn;
}

