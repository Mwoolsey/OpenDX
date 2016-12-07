/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#ifndef _PanelGroupManager_h
#define _PanelGroupManager_h


#include "Base.h"
#include "Dictionary.h"

//
// Class name definition:
//
#define ClassPanelGroupManager	"PanelGroupManager"

class List;
class Network;

//
// PanelGroupManager class definition:
//				
class PanelGroupManager : public Base
{
  private:
    //
    // Private member data:
    //

  protected:
    //
    // Protected member data:
    //

    //
    // A dictionary of lists of panel instance numbers.
    //
    Dictionary	panelGroups;
    Network	*network;

    //
    // Make a list of ControlPanels instance numbers from a list of ControlPanel
    // instance numbers making sure that each panel is still in the network.
    //
    void buildPanelList(List *src, List *dest);

  public:
    //
    // Constructor:
    //
    PanelGroupManager(Network *n);

    //
    // Destructor:
    //
    ~PanelGroupManager();

    //
    // Remove all panels from all groups and all groups from this panel manager.
    //
    void	clear();    

    //
    // Create an empty group of control panels with the given name.
    // If name is already active, then return FALSE.
    //
    boolean 	createPanelGroup(const char *name);

    //
    // Add the panel indicated by panelInstance to the given named panel group.
    // The group must already exist, and the panelInstance must not be present
    // in the group. The panelInstance, should be a member of the network 
    // with which this PanelGroupManager is associated.
    //
    boolean 	addGroupMember(const char *name,  int panelInstance);

    //
    // Removes the given panelInstance from the named panel group.
    // Return FALSE if either the group does not exist or the instance
    // is not a member of the given group.
    //
    boolean 	removeGroupMember(const char *name, int panelInstance); 

    //
    // Removes the named panel group from this managers list of named groups.
    //
    void	removePanelGroup(const char *name);

    //
    // Get a panel group by name.  If l is not NULL, then return in a l, a
    // list of ControlPanel instance numbers each of which is a member of the 
    // group.
    // Returns FALSE if the named group does not exist in this panel manager.
    //
    boolean	getPanelGroup(const char *name, List *l);

    //
    // Get a panel group by 1 based index.  If l is not NULL, then return in 
    // a l, a list of ControlPanel instance numbers, each of which is a 
    // member of the group.
    // Returns NULL if the group does not exist in this panel manager, 
    // otherwise the name of the group is returned.
    //
    const char  *getPanelGroup(int gindex, List *l);

    //
    // Parse/Print the network's group comment.
    //
    boolean cfgParseComment(const char *comment, 
					const char *filename, int lineno);
    boolean cfgPrintComment(FILE *f);


    Network* getNetwork()
    {
	return this->network;
    }

    int   getGroupCount()
    {
	return this->panelGroups.getSize();
    }

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassPanelGroupManager;
    }
};


#endif // _PanelGroupManager_h
