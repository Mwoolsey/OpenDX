/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"



//////////////////////////////////////////////////////////////////////////////

#include "GlobalLocalDefinition.h"
#include "GlobalLocalNode.h"

Node *GlobalLocalDefinition::newNode(Network *net, int inst)
{
    GlobalLocalNode *n = new GlobalLocalNode(this, net, inst);
    return n;
}

NodeDefinition *GlobalLocalDefinition::AllocateDefinition()
{
    return (NodeDefinition*) new GlobalLocalDefinition();
}

