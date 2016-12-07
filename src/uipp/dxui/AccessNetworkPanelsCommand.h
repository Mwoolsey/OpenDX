/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#ifndef _AccessNetworkPanelsCommand_h
#define _AccessNetworkPanelsCommand_h


#include "NoUndoCommand.h"


//
// Class name definition:
//
#define ClassAccessNetworkPanelsCommand	"AccessNetworkPanelsCommand"


//
// Referenced classes:
//
class Network;
typedef long AccessPanelType;



//
// AccessNetworkPanelsCommand class definition:
//				
class AccessNetworkPanelsCommand : public NoUndoCommand
{
    
  protected:
    //
    // Protected member data:
    //
    Network* network;
    AccessPanelType	accessType;

    //
    // Does nothing;
    //
    virtual boolean doIt(CommandInterface *ci);

  public:
    //
    // Constructor:
    //
    AccessNetworkPanelsCommand(const char*   name,
				CommandScope* scope,
				boolean       active,
				Network*      network,
				AccessPanelType	how);

    //
    // Destructor:
    //
    ~AccessNetworkPanelsCommand(){}

    // 
    // These are the various operations that the AccessNetworkPanelsCommand can 
    // implement on behalf of a network.
    // 
    enum {
        OpenAllPanels,	
        CloseAllPanels,		
	OpenPanelByInstance,
	OpenPanelGroupByIndex
    };

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassAccessNetworkPanelsCommand;
    }
};


#endif // _AccessNetworkPanelsCommand_h
