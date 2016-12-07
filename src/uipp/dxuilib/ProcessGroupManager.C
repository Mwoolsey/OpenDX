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
#include "ProcessGroupManager.h"
#include "ProcessGroupAssignDialog.h"
#include "Dictionary.h"
#include "ListIterator.h"
#include "List.h"
#include "Network.h"
#include "ErrorDialogManager.h"
#include "EditorWindow.h"
#include "DXPacketIF.h"
#include "DXApplication.h"

ProcessGroupManager::ProcessGroupManager(Network *net):
    GroupManager(net, theSymbolManager->registerSymbol(PROCESS_GROUP))
{
    // Would like to assert this, but in Network::Network, theDXApplication->network
    // isn't set yet.
    //ASSERT (theDXApplication->network == net);
    this->assignment = NULL;
}
ProcessGroupManager::~ProcessGroupManager()
{
}

void ProcessGroupManager::clear()
{
    this->GroupManager::clear();
    this->clearArgument();

    if(this->assignment)
    {
        this->sendAssignment(DETACH);
	this->clearAssignment();
    }

    if(theDXApplication->processGroupAssignDialog)
	theDXApplication->processGroupAssignDialog->unmanage();
}

const char* ProcessGroupManager::getGroupHost(int n)
{
    return this->getGroupHost(this->getGroupName(n));
}

const char* ProcessGroupManager::getGroupHost(const char *name)
{
    ProcessGroupRecord *record
    	= (ProcessGroupRecord*)this->groups.findDefinition(name);

    if(NOT record)
	return NULL;
    else
	return record->host;
}

const char* ProcessGroupManager::getGroupNewHost(int n)
{
    return this->getGroupNewHost(this->getGroupName(n));
}

const char* ProcessGroupManager::getGroupNewHost(const char *name)
{
    ProcessGroupRecord *record
    	= (ProcessGroupRecord*)this->groups.findDefinition(name);

    if(NOT record)
	return NULL;
    else
	return record->newhost;
}

boolean ProcessGroupManager::getGroupHostDirty(const char *name)
{
    ProcessGroupRecord *record
    	= (ProcessGroupRecord*)this->groups.findDefinition(name);

    if(NOT record)
	return FALSE;
    else
	return record->isDirty();
}

void ProcessGroupManager::clearGroupHostDirty(const char *name)
{
    ProcessGroupRecord *record
    	= (ProcessGroupRecord*)this->groups.findDefinition(name);

    if(record)
	record->setDirty(FALSE);
}

boolean ProcessGroupManager::assignHost(int n, const char *hostname)
{
    return assignHost(this->getGroupName(n), hostname);
}

boolean ProcessGroupManager::assignHost(const char *groupname,
                                      const char *hostname)
{
    ProcessGroupRecord *record
        = (ProcessGroupRecord*)this->groups.findDefinition(groupname);

    if(NOT record)
        return FALSE;

    if(record->newhost)
         delete record->newhost;

    if(hostname)
        record->newhost = DuplicateString(hostname);
    else
        record->newhost = DuplicateString("localhost");

    return TRUE;
}

void ProcessGroupManager::updateHosts()
{
    int  i;
    const char *group;
    ProcessGroupRecord *record;

    for(i = 1; i <= this->getGroupCount(); i++)
    {
        group = this->getGroupName(i);
        record = (ProcessGroupRecord*)this->groups.findDefinition(group);

        if (!record OR !record->newhost)  continue;

	if (!record->host || !EqualString(record->host, record->newhost)) {
	    if(record->host)
		 delete record->host;

	    record->host = DuplicateString(record->newhost);
	    record->setDirty(TRUE);
	    record->getNetwork()->setFileDirty();
	}
        delete record->newhost;
        record->newhost = NULL;
    }
}

void ProcessGroupManager::clearNewHosts()
{
    int  i;
    const char *group;
    ProcessGroupRecord *record;

    for(i = 1; i <= this->getGroupCount(); i++)
    {
        group = this->getGroupName(i);
        record = (ProcessGroupRecord*)this->groups.findDefinition(group);

        if (!record OR !record->newhost)  continue;

        delete record->newhost;
        record->newhost = NULL;
    }
}

const char* ProcessGroupManager::getArgument(const char *name)
{
    ProcessHostRecord *record = 
                (ProcessHostRecord*)this->arguments.findDefinition(name);
    if(record) 
    	return record->args; 
    else
	return NUL(char*);
}

boolean ProcessGroupManager::getArgumentDirty(const char *name)
{
    ProcessHostRecord *record = 
                (ProcessHostRecord*)this->arguments.findDefinition(name);
    
    if(record) 
    	return record->dirty; 
    else
	return FALSE;
}

void ProcessGroupManager::clearArgumentDirty(const char *name)
{
    ProcessHostRecord *record = 
                (ProcessHostRecord*)this->arguments.findDefinition(name);
    
    if(record) 
    	record->dirty = FALSE; 
}

void  ProcessGroupManager::assignArgument(const char *host,const char *args)
{
    ProcessHostRecord *record = 
		(ProcessHostRecord*)this->arguments.findDefinition(host);

    if (record)
    {
	if(record->args)
		delete record->args;
	if(args)
	    record->args = DuplicateString(args);
	else
	    record->args = NUL(char*);
    }
    else
    {
    	record = new ProcessHostRecord((char*)args);
    	this->arguments.addDefinition(host,(const void*)record); 
    }

    record->dirty = TRUE;
    this->app->network->setFileDirty();
}
// clear the argument dictionary
void  ProcessGroupManager::clearArgument()
{
    ProcessHostRecord *record;

    while ( (record = (ProcessHostRecord*) this->arguments.getDefinition(1)) )
    {
        this->arguments.removeDefinition((const void*)record);
        delete record;
    }
}


Dictionary *ProcessGroupManager::createAssignment()
{
    int i, size = this->groups.getSize();

    if(size == 0)
	return NULL;

    Dictionary *dic = new Dictionary();
    List *list;

    for (i=1; i<=size; i++)
    {
	const char *host = this->getGroupHost(i);
	char *group;
	if(NOT host) 
	    host = "localhost";
//	    continue;
	
	group = DuplicateString((char*)this->groups.getStringKey(i));

	if( (list = (List*)dic->findDefinition(host)) )
        {
	    list->appendElement((void*)group);
	}   
	else
	{
	    list = new List();
	    list->appendElement((void*)group);
	    dic->addDefinition(host, (void*)list);
	} 
    }

    if(dic->getSize())
	return dic;
   
    delete dic;
    return NULL;
}

void ProcessGroupManager::clearAssignment()
{
    if(NOT this->assignment)
	return;

    int i, size = this->assignment->getSize();
    ListIterator li;
    char *group;
    List *list;

    for (i=1; i<=size; i++)
    {
	list = (List*)this->assignment->getDefinition(i);
	li.setList(*list);
	
	while( (group = (char*)li.getNext()) )
	     delete group;

	list->clear();
	delete list;
    }

    delete this->assignment;
    this->assignment = NULL;
}

void ProcessGroupManager::sendAssignment(int function)
{
    DXPacketIF *p = this->app->getPacketIF();
    if (!p)
        return;

    this->setDirty(FALSE);
    if (!this->assignment)
        return;

    List *list;
    ListIterator li;
    int i;
    boolean first;
    char *func = NULL, *host, *grouplist,*group, *cmd;

	switch(function)
	{
	   case ATTACH:

		func = "group attach";
		this->dirty = FALSE;
		break;

	   case DETACH:

		func = "group detach";
		break;

	   default:

		ASSERT(FALSE);
	}

        for(i = 1; i <= this->assignment->getSize(); i++)
        {
            host = (char*)this->assignment->getStringKey(i);
	    this->clearArgumentDirty(host);
	    const char *args = this->getArgument(host);
            list = (List*)this->assignment->getDefinition(i);

	    /* figure out how much, and allocate space for all group names */
	    int grouplistSize = 0;
	    li.setList(*list);
	    while( (group = (char*)li.getNext()) )
		grouplistSize += STRLEN(group);
	    grouplist = new char[grouplistSize + 20 * list->getSize()];
	    grouplist[0] = '\0';

            first = TRUE;
            li.setList(*list);
			while( (group = (char*)li.getNext()) )
			{
				this->clearGroupHostDirty(group);
				if(first)
					first = FALSE;
				else
					strcat(grouplist, ",");

				if(function == DETACH)
					strcat(grouplist, "\"");	

				strcat(grouplist, group);

				if(function == DETACH)
					strcat(grouplist, "\"");	
			}

           cmd = new char[STRLEN("exective( ), ")
                          + STRLEN(func)
                          + STRLEN(host)
                          + STRLEN(args)
                          + STRLEN(grouplist) + 20];

	   if(function == ATTACH)
	   {
	   	if(args)
                    SPRINTF(cmd, "Executive(\"%s\",{\"%s: %s %s\"});\n",
				   	func,grouplist,host,args);
	   	else
                    SPRINTF(cmd, "Executive(\"%s\",{\"%s: %s\"});\n",
				   	func,grouplist,host);
	   }
	   else
		SPRINTF(cmd, "Executive(\"%s\",{%s});\n",func,grouplist);

           p->send(PacketIF::FOREGROUND, cmd);
    	   p->sendImmediate("sync");

           delete grouplist;
           delete cmd;
        }


}

void   ProcessGroupManager::updateAssignment()
{
    DXPacketIF *p = this->app->getPacketIF();

    //
    // If the assignment hasn't been sent, don't bother detaching.
    //
    if(this->isDirty() OR !this->assignment OR !p)
    {
	this->clearAssignment();
	this->assignment = this->createAssignment();
        if(p)
            this->sendAssignment(ATTACH);
        else
            this->setDirty();
	return;
    }

    List *list, *oldList;
    ListIterator li, oldLi;
    int i;
    boolean first, argsDirty;
    char *host, *grouplist,*group,*oldGroup, *cmd;

    //
    // Detach groups that changed their assignment.
    //
    for(i = 1; i <= this->assignment->getSize(); i++)
    {
        host = (char*)this->assignment->getStringKey(i);
	list = (List*)this->assignment->getDefinition(i);
	argsDirty = this->getArgumentDirty(host);

        li.setList(*list);
        while( (group = (char*)li.getNext()) )
	    if(argsDirty OR this->getGroupHostDirty(group))
		this->detachGroup(host,group);       
    }

    Dictionary *newAss = this->createAssignment(); 

    if(NOT newAss)
    {
	this->clearAssignment();
        this->setDirty();
	return;
    }	

    //
    // Attach groups that changed their assignment.
    //
    for(i = 1; i <= newAss->getSize(); i++)
    {
        host = (char*)newAss->getStringKey(i);
	const char *args = this->getArgument(host);
	argsDirty = this->getArgumentDirty(host);
        list = (List*)newAss->getDefinition(i);

	/* figure out how much, and allocate space for all group names */
	int grouplistSize = 0;
	li.setList(*list);
	while( (group = (char*)li.getNext()) )
	    grouplistSize += STRLEN(group);
	grouplist = new char[grouplistSize + 20 * list->getSize()];
	grouplist[0] = '\0';


	this->clearArgumentDirty(host);

	if(argsDirty)
	    oldList = NULL;
	else
	    oldList = (List*)this->assignment->findDefinition(host);

        first = TRUE;
        li.setList(*list);
        while( (group = (char*)li.getNext()) )
	{
	    if(oldList)
	    {
		oldLi.setList(*oldList);
		while( (oldGroup  = (char*)oldLi.getNext()) )
		    if(EqualString(oldGroup, group)
			AND NOT this->getGroupHostDirty(group))
			break;

		if(oldGroup)
		    continue;
	    }

	    this->clearGroupHostDirty(group);

            if(first)
            {
                 strcpy(grouplist, group);
                 first = FALSE;
            }
            else
            {
                    strcat(grouplist, ",");
                    strcat(grouplist, group);
            }
	}
	if(NOT first)
	{
            cmd = new char[STRLEN("exective( group attach ), ")
                          + STRLEN(host)
                          + STRLEN(args)
                          + STRLEN(grouplist) + 20];

	   if(args)
               SPRINTF(cmd, "Executive(\"group attach\",{\"%s: %s %s\"});\n",
				   	grouplist,host,args);
	   else
               SPRINTF(cmd, "Executive(\"group attach\",{\"%s: %s\"});\n",
				   	grouplist,host);

            p->send(PacketIF::FOREGROUND, cmd);
    	    p->sendImmediate("sync");
            delete cmd;
	}
        delete grouplist;
    }

    this->clearAssignment();
    this->assignment = newAss;
    this->dirty = FALSE;
}


void   ProcessGroupManager::detachGroup(const char *host, const char *group)
{
    DXPacketIF *p = theDXApplication->getPacketIF();
    if (NOT p)
        return;

    char* cmd = new char[STRLEN("Executive( group detach );") + 10 
                         + STRLEN(host) + STRLEN(group)];

    sprintf(cmd, "Executive(\"group detach\",{\"%s\"});\n", group);

    p->send(PacketIF::FOREGROUND, cmd);
    p->sendImmediate("sync");

    delete cmd;

}

void ProcessGroupManager::attachGroup(const char *host, const char *group)
{
    DXPacketIF *p = theDXApplication->getPacketIF();
    if (NOT p)
        return;

    this->clearGroupHostDirty(group);

    const char* args = this->getArgument(host);
    char* cmd = new char[STRLEN("Executive( group attach );") + 10 
                         + STRLEN(host) + STRLEN(args) + STRLEN(group)];

    if(args)
    	sprintf(cmd, "Executive(\"group attach\", {\"%s: %s %s\"});\n", 
			    	group,host,args);
    else
    	sprintf(cmd, "Executive(\"group attach\", {\"%s: %s\"});\n", 
			    	group,host);

    p->send(PacketIF::FOREGROUND, cmd);
    p->sendImmediate("sync");

    delete cmd;
}

boolean ProcessGroupManager::addGroupAssignment(const char* host, 
						const char *group)
{
    if(NOT this->assignment)
    {
	this->assignment = this->createAssignment();
	this->sendAssignment(ATTACH);
	return TRUE;
    }

    List *list = (List*)this->assignment->findDefinition(host);
    if(NOT list)
    {
	list = new List();
	if(NOT this->assignment->addDefinition(host, (void*)list))
	    return FALSE;
    }

    list->appendElement((void*)DuplicateString(group));	
    this->attachGroup(host, group);

    return TRUE;
}

boolean ProcessGroupManager::removeGroupAssignment(const char *group)
{
    if(NOT this->assignment)
	return FALSE;

    List *list;
    ListIterator li;
    int i, size = this->assignment->getSize();
    boolean found;
    char *name;

    for(i=1; i<=size; i++)
    {
    	list = (List*)this->assignment->getDefinition(i);
	found = FALSE;
	li.setList(*list);
	while( (name = (char*)li.getNext()) )
	    if(EqualString(name, group))
	    {
		found = TRUE;
		break;
	    }

    	if(found)
	{
	    list->removeElement((void*)name);
	    if(NOT this->isDirty())
	    	this->detachGroup(this->assignment->getStringKey(i), group);

    	    if(list->getSize() == 0)
		this->assignment->removeDefinition((void*)list);

    	    if(this->assignment->getSize() == 0)
		this->clearAssignment();

	    return TRUE;
        }
    }

    return FALSE;
}

boolean ProcessGroupManager::printComment(FILE *f)
{

    int	  i;
    List  *list;
    ListIterator li;
    const char  *host, *args, *group;

    if(NOT this->assignment)
	return TRUE;

    if (fprintf(f, "//\n") <= 0)
	return FALSE;

    for(i=1; i <= this->assignment->getSize(); i++)
    {
	host = this->assignment->getStringKey(i);
	args = this->getArgument(host);
	list = (List*)this->assignment->getDefinition(i);

	if(args)
	{
    	    if (fprintf(f, "// pgroup assignment: \"%s(%s): ", host,args) <= 0)
	    	return FALSE;
	}
	else
	{
    	    if (fprintf(f, "// pgroup assignment: \"%s: ", host) <= 0)
	    	return FALSE;
	}

    	li.setList(*list);

	group = (char*)li.getNext();
	if (fprintf(f, "%s", group) <= 0)
            return FALSE;

    	while( (group = (char*)li.getNext()) )
	    if (fprintf(f, ", %s", group) <= 0)
		return FALSE;

	if (fprintf(f, "\"\n") <= 0)
	    return FALSE;
    } 

    return TRUE;
}

boolean ProcessGroupManager::printAssignment(FILE *f)
{
    if (NOT this->assignment)
	return TRUE;

    List *list;
    ListIterator li;
    int i;
    boolean first;
    char *func, *host, *grouplist,*group, *cmd;

	func = "group attach";

	if(fprintf(f, "\n") <= 0)
	    return FALSE;

        for(i = 1; i <= this->assignment->getSize(); i++)
        {
            host = (char*)this->assignment->getStringKey(i);
	    const char *args = this->getArgument(host);
            list = (List*)this->assignment->getDefinition(i);

	    /* figure out how much, and allocate space for all group names */
	    int grouplistSize = 0;
	    li.setList(*list);
	    while( (group = (char*)li.getNext()) )
		grouplistSize += STRLEN(group);
	    grouplist = new char[grouplistSize + 20 * list->getSize()];
	    grouplist[0] = '\0';


            first = TRUE;
            li.setList(*list);
            while( (group = (char*)li.getNext()) )
	    {
                if(first)
                {
                    strcpy(grouplist, group);
                    first = FALSE;
                }
                else
                {
                    strcat(grouplist, ",");
                    strcat(grouplist, group);
                }
	   }
           cmd = new char[STRLEN("exective( ), ")
                          + STRLEN(func)
                          + STRLEN(host)
                          + STRLEN(args)
                          + STRLEN(grouplist) + 20];

	   if(args)
               SPRINTF(cmd, "Executive(\"%s\",{\"%s: %s %s\"});\n",
				   	func,grouplist,host,args);
	   else
               SPRINTF(cmd, "Executive(\"%s\",{\"%s: %s\"});\n",
				   	func,grouplist,host);

	   if(fprintf(f, "%s", cmd) <= 0)
		return FALSE;
 
	   if(fprintf(f, "$sync\n") <= 0)
	        return FALSE;

           delete grouplist;
           delete cmd;
        }


    return TRUE;
}

//
// Parse the group information from the cfg file.
//
boolean ProcessGroupManager::parseComment(const char *comment,
                                const char *filename, int lineno,Network *net)
{

    char *p, *c, *host, *args;

    if (strncmp(comment," pgroup assignment",STRLEN(" pgroup assignment")))
        return FALSE;

    char *line = DuplicateString(comment);

    p = (char *) strchr(comment,'"');
    if (!p)
	goto error;

    // Parse the host name.
    host = p + 1;
    p = strchr(host,'"'); 
    if (!p) 
	goto error;

    p = strchr(host,':'); 
    if (!p)
	goto error;

    *p = '\0';

    // Parse the options.
    args = strchr(host,'(');
    if (args)
    {
	*args++ = '\0';
	c = strchr(args,')');
        if (!c)
	   goto error;
	*c = '\0';
    }
 
    p++;
    SkipWhiteSpace(p);

    while(*p) 
    {
	c = strchr(p, ',');
	if(!c)
	    c = strchr(p, '"');

	*c = '\0';

	this->registerGroup(p, net);
	this->assignHost(p, host);

	*c = ',';
	p = c + 1;

	SkipWhiteSpace(p);	
    }

    if(args)
	this->assignArgument(host, args);

    this->updateHosts();
    this->clearAssignment();
    this->assignment = this->createAssignment();

    this->dirty = TRUE;

    delete line;
    return TRUE;

error:

    ErrorMessage("Bad process group aasignment(%s, line %d).",filename,lineno); 
    delete line;
    return FALSE;
}

boolean ProcessGroupManager::removeGroup(const char *name, Network *net)
{
    boolean ret = this->GroupManager::removeGroup (name, net);
    this->removeGroupAssignment(name);

    return ret;
}

