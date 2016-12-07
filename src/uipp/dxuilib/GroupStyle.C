/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>



#include "Network.h"
#include "PageGroupManager.h"
#include "ProcessGroupManager.h"
#include "AnnotationGroupManager.h"

Dictionary* BuildTheGroupManagerDictionary(Network* net)
{
GroupManager *gmgr;
Dictionary* groupManagers = new Dictionary;

    boolean isMacro = net->isMacro();
    if ((!isMacro) || (ProcessGroupManager::SupportsMacros())) {
	gmgr = new ProcessGroupManager(net);
	groupManagers->addDefinition (gmgr->getManagerName(), gmgr);
    }

    if ((!isMacro) || (PageGroupManager::SupportsMacros())) {
	gmgr = new PageGroupManager(net);
	groupManagers->addDefinition (gmgr->getManagerName(), gmgr);
    }

    if ((!isMacro) || (AnnotationGroupManager::SupportsMacros())) {
	gmgr = new AnnotationGroupManager(net);
	groupManagers->addDefinition (gmgr->getManagerName(), gmgr);
    }
    return groupManagers;
}
