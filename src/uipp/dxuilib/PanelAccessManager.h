/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#ifndef _PanelAccessManager_h
#define _PanelAccessManager_h


#include <stdio.h>

#include "Base.h"
#include "List.h"


//
// Class name definition:
//
#define ClassPanelAccessManager	"PanelAccessManager"

class Network;
class ControlPanel;


//
// PanelAccessManager class definition:
//				
class PanelAccessManager : public Base
{
  private:
    //
    // Private member data:
    //

    //
    // This is a list of panels that can NOT be opened. 
    // We make it private, because it does not get notification when
    // panels are deleted.  Instead we implement the access methods
    // which make sure the panels being operated on are still in the
    // network.
    //
    List inaccessiblePanels;
    List inaccessibleGroups;

  protected:
    //
    // Protected member data:
    //
    Network *network;
    ControlPanel *cp;
    boolean 	isActive;


    //
    // Enable/disable the manager.
    //
    void activate();
    void deactivate(); 

  public:
    //
    // Constructor:
    //
    PanelAccessManager(Network *n, ControlPanel *cp = NULL);

    //
    // Destructor:
    //
    ~PanelAccessManager();

    Network *getNetwork() { return this->network; };
    ControlPanel *getControlPanel() { return this->cp; };

    //
    // Parse/Print a control panel's 'inaccessible' comment.
    //
    boolean cfgParseInaccessibleComment(const char *comment,
                                const char *filename, int lineno);
    boolean cfgPrintInaccessibleComment(FILE *f);

    //
    // Return TRUE if cfgPrintInaccessibleComment needs to be called.
    //
    boolean hasCfgComment();

 
    //
    // Add a panel (specified by instance number) to the list of inaccessible
    // panels.
    //
    void        addInaccessiblePanel(int instance);
    void        addInaccessibleGroup(const char *name);

    //
    // Allow access to all panels.
    //
    void        allowAllPanelAccess();
    void        allowAllGroupAccess();

    //
    // Free memory allow all panels/groups acess and deactivate().
    //
    void 	clear();

    //
    // Get the index'th (1 based) inaccessible panel.
    //
    ControlPanel *getInaccessiblePanel(int index);
    const char   *getInaccessibleGroup(int index);

    //
    // Determine if the given panel (specified by instance number) is accessible
    //
    boolean     isAccessiblePanel(int instanceNumber);
    boolean     isAccessibleGroup(const char* name);

    boolean isActivated();

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassPanelAccessManager;
    }
};


#endif // _PanelAccessManager_h
