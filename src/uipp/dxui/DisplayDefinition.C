/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"



#include "DisplayDefinition.h"
#include "DisplayNode.h"

NodeDefinition *DisplayDefinition::AllocateDefinition()
{
    return new DisplayDefinition;
}


DisplayDefinition::DisplayDefinition() : 
    DrivenDefinition()
{
}


Node *DisplayDefinition::newNode(Network *net, int instance)
{
    DisplayNode *d = new DisplayNode(this, net, instance);
    return d;
}

