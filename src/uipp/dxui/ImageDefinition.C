/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"



#include "ImageDefinition.h"
#include "ImageNode.h"
#include "ImageCDB.h"
#include "ParameterDefinition.h"

NodeDefinition *ImageDefinition::AllocateDefinition()
{
    return new ImageDefinition;
}


ImageDefinition::ImageDefinition() : 
    NodeDefinition()
{
    this->defaultInternalCacheability = InternalsFullyCached;
}

void ImageDefinition::finishDefinition()
{
}


Node *ImageDefinition::newNode(Network *net, int instance)
{
    ImageNode *d = new ImageNode(this, net, instance);
    return d;
}

//
// Define the function that allocates CDB's for this node.
//
CDBAllocator ImageDefinition::getCDBAllocator()
{
   return ImageCDB::AllocateConfigurationDialog;
}

