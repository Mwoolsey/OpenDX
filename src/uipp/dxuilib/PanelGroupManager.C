/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#include "DXStrings.h"
#include "lex.h"
#include "ControlPanel.h"
#include "PanelGroupManager.h"
#include "Dictionary.h"
#include "ListIterator.h"
#include "List.h"
#include "Network.h"
#include "ErrorDialogManager.h"


PanelGroupManager::PanelGroupManager(Network *n)
{
    this->network = n;
}
PanelGroupManager::~PanelGroupManager()
{
     this->clear(); 
}

//
// Remove all panels from all groups and all groups from this panel manager.
//
void PanelGroupManager::clear()
{
    List *l;

    while ( (l = (List*) this->panelGroups.getDefinition(1)) ) {
	this->panelGroups.removeDefinition((const void*)l);
	delete l;
    }
}
//
// Create an empty group of control panels with the given name.
// If name is already active, then return FALSE.
//
boolean     PanelGroupManager::createPanelGroup(const char *name)
{
    List *l;
    Boolean result;

    if (IsBlankString(name))
	return FALSE;

    if (this->getPanelGroup(name,NULL))
	return FALSE;	// Named group already exists.

    l = new List;

    result = this->panelGroups.addDefinition(name,(const void*)l); 

    if(result)
    	this->network->setFileDirty();

    return result;
}
//
// Add the panel indicated by panelInstance to the given named panel group.
// The group must already exist, and the panelInstance must not be present
// in the group. The panelInstance, should be a member of the network
// with which this PanelGroupManager is associated.
//
boolean     PanelGroupManager::addGroupMember(const char *name,  
					int panelInstance)
{
    List *l;
    Boolean result;

    ASSERT(panelInstance > 0);
    l = (List*)this->panelGroups.findDefinition(name); 
    if (!l)
	return FALSE;
    ASSERT(!l->isMember((const void*)panelInstance));
    result = l->appendElement((const void*)panelInstance);

    if(result)
    	this->network->setFileDirty();

    return result;
}
//
// Removes the given panelInstance from the named panel group.
// Return FALSE if either the group does not exist or the instance
// is not a member of the given group.
//
boolean     PanelGroupManager::removeGroupMember(const char *name, 
					int panelInstance)
{
    List *l;
    Boolean result;

    ASSERT(panelInstance > 0);
    l = (List*)this->panelGroups.findDefinition(name); 
    if (!l)
	return FALSE;

    result = l->removeElement((const void*)panelInstance);
    if(result)
    	this->network->setFileDirty();

    return result;
}
//
// Removes the named panel group from this managers list of named groups.
//
void PanelGroupManager::removePanelGroup(const char *name)
{
    List *l;
    l = (List*)this->panelGroups.removeDefinition(name); 
    if (l)
        delete l;
    this->network->setFileDirty();
}
//
// Get a panel group by 1 based index.  If l is not NULL, then return in 
// a l, a list of ControlPanel instance nubmer each of which is a member 
// of the group.
// Returns NULL if the group does not exist in this panel manager, 
// otherwise the name of the group is returned.
//
const char *PanelGroupManager::getPanelGroup(int gindex, List *panels)
{
    ASSERT(gindex > 0);
    List *l = (List*)this->panelGroups.getDefinition(gindex); 
    const char *s;

    if (l) {
        s = this->panelGroups.getStringKey(gindex);
	ASSERT(s);
	if (panels)
	    this->buildPanelList(l,panels);
    } else
	s = NULL;
    return s; 
}
//
// Get a panel group by name.  If l is not NULL, then return in a l, a
// list of ControlPanel instance numbers each of which is a member of the group.
// Returns FALSE if the named group does not exist in this panel manager.
//
boolean PanelGroupManager::getPanelGroup(const char *name, List *panels)
{
    List *l = (List*)this->panelGroups.findDefinition(name); 

    if (l && panels) 
	this->buildPanelList(l,panels);
    return (l ? TRUE : FALSE);
}

//
// Make a list of ControlPanels instance numbers from a list of control panel 
// instance numbers making sure that each panel is still in the network.
//
void PanelGroupManager::buildPanelList(List *src, List *dest)
{
    ASSERT(src && dest);

    ListIterator i(*src);
    int instance;
    while ( (instance = (int)(long) i.getNext()) ) {
	// Make sure the panel has not been deleted from the network.
	if ( this->network->getPanelByInstance(instance) )
	    dest->appendElement((const void*)instance);
    }
}
//
// Print the group information into the cfg file.
//
boolean PanelGroupManager::cfgPrintComment(FILE *f)
{
    int	  i, inst;
    List  plist;
    ListIterator li;
    char  *name;

    for(i=1; (name = (char*)this->getPanelGroup(i, &plist)); i++)
    {
    	if (fprintf(f, "// panel group: \"%s\"", name) < 0)
	    return FALSE;

    	li.setList(plist);

    	while( (inst = (int)(long)li.getNext()) )
	    if (fprintf(f, " %d", inst-1) < 0)
		return FALSE;

    	if (fputc('\n', f) < 0)
	    return FALSE;

    	plist.clear();
    } 

    return TRUE;
}

//
// Parse the group information from the cfg file.
//
boolean PanelGroupManager::cfgParseComment(const char *comment,
                                const char *filename, int lineno)

{
    char *p, *c, name[128];
    int  inst;

    if (strncmp(comment," panel group",12))
        return FALSE;

    p = (char *) strchr(comment,'"');
    if (!p) {
	ErrorMessage("Bad panel group name (%s, line %d)",filename,lineno); 
	return FALSE;
    }

    c = p + 1;
    p = strchr(c,'"'); 
    if (!p) {
	ErrorMessage("Bad panel group name (%s, line %d)",filename,lineno); 
	return FALSE;
    }
    *p = '\0';
    strcpy(name,c);
    if (!this->createPanelGroup(name)) {
	ErrorMessage("Can't create panel group named '%s'",name);
	return FALSE;
    }

    *p = '"';	// Put back the "
    p++;
    SkipWhiteSpace(p);
    while(*p) 
    {
	inst = atoi(p) + 1;
	if (!this->addGroupMember(name, inst)) {
	    ErrorMessage("Can't add panel instance %d to group '%s'",inst,name);
	    return FALSE;
	}
	FindWhiteSpace(p);	// Skip over the number or to end-of-string
	SkipWhiteSpace(p);	// Skip to the next number or end-of-string 
    }

    return TRUE;
}

