/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"



#include "MacroParameterDefinition.h"
#include "MacroParameterNode.h"
#include "ErrorDialogManager.h"
#include "ParameterCDB.h"
#include "Network.h"
#include "ListIterator.h"
#include "ParameterDefinition.h"


MacroParameterDefinition::MacroParameterDefinition() : 
    NodeDefinition()
{
}

MacroParameterDefinition::~MacroParameterDefinition()
{
}

void MacroParameterDefinition::finishDefinition()
{
    this->NodeDefinition::finishDefinition();
    ParameterDefinition *pd;

    ListIterator iterator(this->outputDefs);
    while ( (pd = (ParameterDefinition*)iterator.getNext()) ) {
        pd->setWriteableCacheability(FALSE);
        pd->setDefaultCacheability(OutputFullyCached);
    }

}

NodeDefinition *MacroParameterDefinition::AllocateDefinition()
{
    return new MacroParameterDefinition;
}

Node *MacroParameterDefinition::newNode(Network *net, int instance)
{
    if (!net->canBeMacro())
    {
	ErrorMessage(
	    "The %s tool, which converts this network to a macro, is not\n" 
	    "allowed in the current network.  It is likely that one or more\n"
	    "of the tools in the current network is not allowed in a macro.",
	    this->getNameString());
	return NULL;
    }
    MacroParameterNode *d = new MacroParameterNode(this, net, instance);
    return d;
}

CDBAllocator MacroParameterDefinition::getCDBAllocator()
{
    return ParameterCDB::AllocateConfigurationDialog;
}
