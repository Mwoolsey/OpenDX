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
#include "PanelAccessManager.h"
#include "PanelGroupManager.h"
#include "Network.h"
#include "ControlPanel.h"
#include "ErrorDialogManager.h"
#include "List.h"
#include "ListIterator.h"

PanelAccessManager::PanelAccessManager(Network *n, ControlPanel *cp)
{
    this->network = n;
    this->cp = cp;
    this->isActive = FALSE;
}
PanelAccessManager::~PanelAccessManager()
{
    this->clear();
}

void PanelAccessManager::clear()
{
    this->allowAllPanelAccess(); 
    this->allowAllGroupAccess(); 
    this->deactivate();
}

//
// Determine if the given panel (specified by instance number) is accessible 
//
boolean PanelAccessManager::isAccessiblePanel(int instanceNumber)
{
    return !this->isActivated() ||
 	   !this->inaccessiblePanels.isMember((void*)instanceNumber);
}
boolean PanelAccessManager::isActivated()
{
    return this->isActive;
}
void PanelAccessManager::activate()
{
    this->isActive = TRUE;
}
void PanelAccessManager::deactivate()
{
    this->isActive = FALSE;
}
//
// Determine if the given panel group  is accessible 
//
boolean     PanelAccessManager::isAccessibleGroup(const char *name)
{
    if (!this->isActivated())
	return TRUE;

    ListIterator li(this->inaccessibleGroups);
    char *gname;
    while ( (gname = (char*)li.getNext()) )
        if (EqualString(gname, name))
            return FALSE;

    return TRUE;
}
//
// Get the index'th (1 based) inaccessible panel. 
//
ControlPanel *PanelAccessManager::getInaccessiblePanel(int index)
{
    ListIterator iterator(this->inaccessiblePanels);
    List	obsolete;
    Network	*net = this->network;
    int		instance;
    ControlPanel *p, *found = NULL;

    ASSERT(index > 0); 

    while (!found && (instance = (int)(long)iterator.getNext())) {
	p = net->getPanelByInstance(instance);
	if (!p)  {
	    //
	    // cp was deleted from the network behind our backs so save
	    // it and remove it later.
	    //
	    obsolete.appendElement((void*)instance);
	} else if (--index == 0)
	    found = p;	
    }

    //
    // Clean the deleted panels out of the inaccessible list. 
    // Note, that because we stop the above loop as soon as we find the
    // requested panel, there may be deleted panels left in the list
    // after the following loop.
    //
    if (obsolete.getSize() > 0) {
	iterator.setList(obsolete);
	while ( (instance = (int)(long)iterator.getNext()) ) {
	    this->inaccessiblePanels.removeElement((void*)instance);
	}
    }
	
    return found;
}
//
// Get the index'th (1 based) inaccessible group.
//
const char *PanelAccessManager::getInaccessibleGroup(int index)
{
    ListIterator iterator(this->inaccessibleGroups);
    List        obsolete;
    Network     *net = this->network;
    PanelGroupManager *pgm = net->getPanelGroupManager();
    char        *name, *found = NULL;

    ASSERT(index > 0);

    while (!found && (name = (char*)iterator.getNext())) {
        if (!pgm->getPanelGroup(name, NULL))  {
            //
            // group was deleted from the network behind our backs so save
            // it and remove it later.
            //
            obsolete.appendElement((void*)name);
        } else if (--index == 0)
            found = name;
    }


    //
    // Clean the deleted groups out of the inaccessible list.
    // Note, that because we stop the above loop as soon as we find the
    // requested panel, there may be deleted groups left in the list
    // after the following loop.
    //
    if (obsolete.getSize() > 0) {
        ListIterator iterator(obsolete);
        while ( (name = (char*)iterator.getNext()) ) {
	    char gname[64];
	    strcpy(gname,name);
            this->inaccessibleGroups.removeElement(name);
        }
    }
       
    return found;
}

//
// Allow access to all panels. 
//
void        PanelAccessManager::allowAllPanelAccess()
{
    if (this->inaccessiblePanels.getSize() != 0) {
	this->inaccessiblePanels.clear();
	this->network->setFileDirty();
    }
    this->activate();
}
//
// Allow access to all groups. 
//
void        PanelAccessManager::allowAllGroupAccess()
{

    if (this->inaccessibleGroups.getSize()) {
	char *name;
	ListIterator li(this->inaccessibleGroups);

	while( (name = (char*)li.getNext()) )
	    delete name;

	this->inaccessibleGroups.clear();
	this->network->setFileDirty();
    }
    this->activate();
}
//
// Add a panel (specified by instance number) to the list of inaccessible 
// panels.
//
void        PanelAccessManager::addInaccessiblePanel(int instance)
{
    if (!this->inaccessiblePanels.isMember((void*)instance)){
        this->inaccessiblePanels.appendElement((void*)instance);
    	this->network->setFileDirty();
    }
    this->activate();
}
//
// Add a group(specified by name) to the list of inaccessible groups. 
//
void        PanelAccessManager::addInaccessibleGroup(const char *name)
{
    if (this->isAccessibleGroup(name)) {
	char *n = DuplicateString(name);
        this->inaccessibleGroups.appendElement((void*)n);
        this->network->setFileDirty();
    }
    this->activate();
}
//
// Return TRUE if cfgPrintInaccessibleComment needs to be called.
//
boolean PanelAccessManager::hasCfgComment()
{
#if 0
    return (this->isActivated() &&
	    ((this->inaccessiblePanels.getSize() > 0) ||
    	     (this->inaccessibleGroups.getSize() > 0)));
#else
    return this->isActivated();
#endif
}
//
// Print a control panel's 'inaccessible' comment, which indicates
// which panel instances can not be opened from this panel.
// 	Format = '// inaccessible: %d %d...'
// where the numbers printed are the panel instance numbers.
// NOTE: instance numbers in the file are 0 based, while internally they
// 	are 1 based.
//
boolean PanelAccessManager::cfgPrintInaccessibleComment(FILE *f)
{
    ControlPanel *cp;
    int	i;
    const char *name;

    if (!this->isActivated())
	return TRUE;


    //
    // Print inaccessible panels 
    //
    if (fprintf(f, "// inaccessible panels:") < 0)
	return FALSE;
    cp = this->getInaccessiblePanel(1); 
    for (i=1 ; (cp=this->getInaccessiblePanel(i)) ; i++)  
    { 
    	if (fprintf(f, " %d",cp->getInstanceNumber()-1) < 0)
	   return FALSE; 
   
    }
    if (fprintf(f, "\n") < 0)
       return FALSE;

    //
    // Print inaccessible groups
    //
    if (fprintf(f, "// inaccessible groups:") < 0)
	return FALSE;

    for (i=1 ; (name = this->getInaccessibleGroup(i)) ; i++) 
    {
	if (fprintf(f, " \"%s\"",name) < 0)
	    return FALSE; 
    }
    if (fprintf(f, "\n") < 0)
	return FALSE;

    return TRUE;
}
//
// Parse a control panel's 'inaccessible' comment, which indicates
// which panel instances can not be opened from this panel.
// 	Format = '// inaccessible: %d %d...'
// where the numbers printed are the panel instance numbers.
//
boolean PanelAccessManager::cfgParseInaccessibleComment(const char *comment,
                                const char *filename, int lineno)
{
    char gname[64], name[32];

    if (strncmp(comment," inaccessible",STRLEN(" inaccessible"))) 
	return FALSE;

    char* p = (char *) strchr(comment,':');
    if (!p)
	return FALSE;

    p++;

    sscanf(comment, "%*s %s", name);
    if (EqualString(name, "panels:"))
    {
      SkipWhiteSpace(p);
      while (*p && *p != '\n') {
	int index = 0;
	if (!IsInteger(p, index)) {
	    ErrorMessage("Non-integer panel instance number (file %s, line %d)",
					filename, lineno);
	    return TRUE;
	}
	int instance = atoi(p);
	if (instance < 0) {
	    ErrorMessage("Negative panel instance number (file %s, line %d)",
					filename, lineno);
	    return TRUE;
	}
	instance++;		// instances are 1 based internally
	this->addInaccessiblePanel(instance);
  	p = &p[index];
        SkipWhiteSpace(p);
      }
      this->activate();
    }
    else if (EqualString(name, "groups:"))
    {
        p = strchr(p, '"');
        if (p) {
	  do {
	    p++;
	    if (!*p) {
		ErrorMessage("Panel group name format error (file %s, line %d)",
						filename, lineno);
		return TRUE;
	    }
	    int i;
	    for (i=0 ; *p && *p != '"'; p++, i++)
		gname[i] = *p;
	    if (!*p) {
	        ErrorMessage("Panel group name format error (file %s, line %d)",
					    filename, lineno);
    		return TRUE;
	    }
	    gname[i] = '\0';
	    this->addInaccessibleGroup(gname);
	    // Move to first quote of next group name
	    p++;
	    if (*p)
		p = strchr(p, '"');
	    else
		p = NULL;
	  } while (p);
	}
        this->activate();
    } 
    else 
    {
	return FALSE;
    }

    return TRUE;

}

