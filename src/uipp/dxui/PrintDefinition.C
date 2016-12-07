/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"



#include "PrintDefinition.h"
#include "PrintNode.h"

Node *PrintDefinition::newNode(Network *net, int inst)
{
    PrintNode *n = new PrintNode(this, net, inst);
    return n;
}

NodeDefinition *PrintDefinition::AllocateDefinition()
{
    return (NodeDefinition*) new PrintDefinition();
}

