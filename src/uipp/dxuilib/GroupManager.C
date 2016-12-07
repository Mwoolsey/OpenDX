/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#include <stdlib.h>

#include "DXStrings.h"
#include "lex.h"
#include "GroupManager.h"
#include "Dictionary.h"
#include "ListIterator.h"
#include "List.h"
#include "Network.h"
#include "ErrorDialogManager.h"
#include "EditorWindow.h"
#include "DXApplication.h"
#include "SymbolManager.h"

GroupManager::GroupManager(Network *net, Symbol groupID)
{
    this->app = theDXApplication;
    this->network = net;
    this->setDirty();
    this->groupID = groupID;
}
GroupManager::~GroupManager()
{
     this->clear(); 
}

void GroupManager::clear()
{
    GroupRecord *record;

    while ( (record = (GroupRecord*) this->groups.getDefinition(1)) ) 
    {
	this->groups.removeDefinition((const void*)record);
	delete record;
    }
}

Network* GroupManager::getGroupNetwork(const char *name)
{
    GroupRecord *record
    	= (GroupRecord*)this->groups.findDefinition(name);

    if(NOT record)
	return NULL;
    else
	return record->network;
}


boolean GroupManager::registerGroup(const char *name, Network *net)
{
    GroupRecord *record;
    ASSERT (net == this->network);

    if (NOT this->groups.findDefinition(name))
    {
	record = this->recordAllocator(net, name);
    	this->groups.addDefinition(name,(const void*)record); 
    }

    return TRUE;
}

boolean GroupManager::addGroupMember(const char *name, Network *net)
{
    ASSERT (net == this->network);

    if (IsBlankString(name))
	return FALSE;

    if (NOT this->hasGroup(name))
	return FALSE;	

    GroupRecord *grec = this->getGroup(name);

    EditorWindow* editor = net->getEditor();
    if (editor) {
	editor->setGroup(grec, this->groupID);
	net->setFileDirty();
    }

    return TRUE;
}

boolean  GroupManager::removeGroupMember(const char *name, Network *net)
{
    ASSERT (net == this->network);

    if (NOT this->hasGroup(name))
	return FALSE;	

    EditorWindow* editor = net->getEditor();
    if (editor) {
	editor->setGroup(NULL, this->groupID);
	net->setFileDirty();
    }

    return TRUE;
}

boolean     GroupManager::createGroup(const char *name, Network *net)
{
    ASSERT (net == this->network);

    Boolean result;

    if (IsBlankString(name))
	return FALSE;

    if (this->groups.findDefinition(name))
	return FALSE;	// Named group already exists.

    GroupRecord *record = this->recordAllocator(net, name);

    result = this->groups.addDefinition(name,(const void*)record); 

    if(result)
    {
	EditorWindow* editor = net->getEditor();
	if (editor) {
	    editor->setGroup(record, this->groupID);
	    net->setFileDirty();
	    net->changeExistanceWork(NUL(Node*), TRUE);
	}
    }

    return result;
}

boolean     GroupManager::removeGroup(const char *name, Network *net) 
{
    ASSERT (net == this->network);

    GroupRecord *record;

    record = (GroupRecord*)this->groups.removeDefinition(name);

    if (NOT record) 
	return FALSE;

    EditorWindow* editor = net->getEditor();
    if (editor) {
	editor->resetGroup(name, this->groupID);
	net->setFileDirty();
	net->changeExistanceWork(NUL(Node*), FALSE);
    }

    delete record;

    return TRUE;
}


boolean     GroupManager::selectGroupNodes(const char *name) 
{
    GroupRecord *record;

    record = (GroupRecord*) this->groups.findDefinition(name);

    if(NOT record)
	return FALSE;	
    
    EditorWindow* editor = record->network->getEditor();
    if (editor)
	editor->selectGroup(name, this->groupID);

    return TRUE;
}


const char *GroupManager::getManagerName() 
    { return theSymbolManager->getSymbolString(this->groupID); }


