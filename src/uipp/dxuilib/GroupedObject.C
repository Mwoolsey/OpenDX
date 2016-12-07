/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


#include "GroupManager.h"
#include "GroupedObject.h"
#include "Dictionary.h"
#include "DictionaryIterator.h"
#include "Network.h"
#include "lex.h"

static char *__indent = "    ";

GroupedObject::~GroupedObject()
{
    if (this->groups) delete this->groups;
}


//
// Maintains a dictionary of dictionaries.  The key for main dictionary
// identifies a group manager class.  The subdictionary identifies by name and
// contains group records.  
//
void GroupedObject::setGroupName(GroupRecord *groupRec, Symbol groupID)
{
GroupRecord *grec = NUL(GroupRecord*);

    if (this->groups) 
	grec = (GroupRecord*)this->groups->findDefinition (groupID);
    else if (groupRec)
	this->groups = new Dictionary;

    if (grec) {
	if (groupRec)
	    this->groups->replaceDefinition (groupID, groupRec);
	else
	    this->groups->removeDefinition (groupID);
    } else if (groupRec)
	this->groups->addDefinition (groupID, groupRec);

    this->getNetwork()->setDirty();
}


const char * GroupedObject::getGroupName (Symbol groupID)
{

    if (!this->groups) return NUL(const char *);

    GroupRecord *grec = (GroupRecord*)this->groups->findDefinition (groupID);
    if (!grec) return NUL(const char*);

    return grec->getName();
}


void GroupedObject::addToGroup(const char* name, Symbol groupID)
{
    GroupManager *gmgr = (GroupManager*)
	this->getNetwork()->getGroupManagers()->findDefinition(groupID);
    ASSERT(gmgr);

    if (!gmgr->hasGroup(name))
	gmgr->createGroup (name, this->getNetwork());

    GroupRecord *grec = gmgr->getGroup (name);
    ASSERT(grec);

    this->setGroupName (grec, groupID);
}


boolean GroupedObject::printGroupComment (FILE *f)
{
    if (this->groups) {
	int i;
	int count = this->groups->getSize();
	for (i=1; i<=count; i++) {

	    Symbol gsym = this->groups->getSymbol(i);
	    const char *manager = theSymbolManager->getSymbolString(gsym);
	    const char *group_name = this->getGroupName(gsym);

	    if ((!group_name) || (!group_name[0]))
		return TRUE;
	    
	    if (fprintf (f, "%s// %s group: %s\n", __indent, manager, group_name) < 0)
		return FALSE;

	}
    }
    return TRUE;
}



//
// Loop over all group managers, using the name of each to form
// a parse string.
//
boolean GroupedObject::parseGroupComment (const char* comment,
				  const char* , int )
{
    int i;
    Dictionary* groupManagers = this->getNetwork()->getGroupManagers();
    int count = groupManagers->getSize();
    boolean group_comment = FALSE;
    GroupManager *gmgr = NUL(GroupManager*);
    for (i=1; i<=count; i++) {
	gmgr = (GroupManager*)groupManagers->getDefinition(i);
	const char *mgr_name = gmgr->getManagerName();
	char buf[128];
	sprintf (buf, " %s group:", mgr_name);
	if (EqualSubstring (buf, comment, strlen(buf))) {
	    group_comment = TRUE;
	    break;
	}
    }

    if (!group_comment) return FALSE;
    char *group_name = (char *) strchr(comment, ':');
    group_name++;
    SkipWhiteSpace(group_name);

    if (!group_name) return FALSE;

    this->addToGroup(group_name, gmgr->getManagerSymbol());
    return TRUE;
}


