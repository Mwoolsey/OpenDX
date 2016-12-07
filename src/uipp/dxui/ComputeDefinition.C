/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"



#include "ComputeDefinition.h"
#include "ComputeNode.h"
#include "ParameterDefinition.h"
#include "ComputeCDB.h"
#include "CDBAllocatorDictionary.h"

NodeDefinition *ComputeDefinition::AllocateDefinition()
{
    return new ComputeDefinition;
}


ComputeDefinition::ComputeDefinition() : 
    NodeDefinition()
{
}


Node *ComputeDefinition::newNode(Network *net, int instance)
{
    ComputeNode *c = new ComputeNode(this, net, instance);
    return c;
}


void ComputeDefinition::finishDefinition()
{
    ParameterDefinition *p = this->getInputDefinition(1);
    p->setDefaultVisibility(FALSE);
}


//
// Define the function that allocates CDB's for this node.
//
CDBAllocator ComputeDefinition::getCDBAllocator()
{
   return ComputeCDB::AllocateConfigurationDialog;
}

