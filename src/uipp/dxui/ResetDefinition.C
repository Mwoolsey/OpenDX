/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"


#include "ResetDefinition.h"
#include "ResetNode.h"

Node *ResetDefinition::newNode(Network *net, int inst)
{
    ResetNode *n = new ResetNode(this, net, inst);
    return n;
}

NodeDefinition *
ResetDefinition::AllocateDefinition()
{
    return (NodeDefinition*) new ResetDefinition;
}

