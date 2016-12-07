/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"


#include "EchoDefinition.h"
#include "EchoNode.h"

Node *EchoDefinition::newNode(Network *net, int inst)
{
    EchoNode *n = new EchoNode(this, net, inst);
    return n;
}

NodeDefinition *EchoDefinition::AllocateDefinition()
{
    return (NodeDefinition*) new EchoDefinition();
}

