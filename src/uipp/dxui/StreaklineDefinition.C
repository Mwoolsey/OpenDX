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

#include "StreaklineDefinition.h"
#include "StreaklineNode.h"

Node *StreaklineDefinition::newNode(Network *net, int inst)
{
    StreaklineNode *n = new StreaklineNode(this, net, inst);
    return n;
}

NodeDefinition *StreaklineDefinition::AllocateDefinition()
{
    return (NodeDefinition*) new StreaklineDefinition();
}

