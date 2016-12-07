/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
//////////////////////////////////////////////////////////////////////////////

#include <dxconfig.h>
#include "../base/defines.h"


#include "FileSelectorDefinition.h"
#include "FileSelectorNode.h"

Node *FileSelectorDefinition::newNode(Network *net, int inst)
{
    InteractorNode *n = new FileSelectorNode(this, net, inst);
    return n;
}

NodeDefinition *FileSelectorDefinition::AllocateDefinition()
{
    return (NodeDefinition*) new FileSelectorDefinition();
}

