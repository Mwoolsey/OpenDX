/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"





#ifndef _NoUndoNetworkCommand_h
#define _NoUndoNetworkCommand_h


#include "NoUndoCommand.h"

typedef long NetworkCommandType;

//
// Class name definition:
//
#define ClassNoUndoNetworkCommand	"NoUndoNetworkCommand"

class   Network;

//
// NoUndoNetworkCommand class definition:
//				
class NoUndoNetworkCommand : public NoUndoCommand
{
    
  protected:
    //
    // Protected member data:
    //
    Network		*network;
    NetworkCommandType 	commandType;
 
    virtual boolean doIt(CommandInterface *ci);

  public:
    //
    // Constructor:
    //
    NoUndoNetworkCommand(const char*   name,
                   CommandScope  *scope,
                   boolean       active,
		   Network	*n,
		   NetworkCommandType comType);

    //
    // Destructor:
    //
    ~NoUndoNetworkCommand(){}

    // 
    // These are the various operations that the NoUndoNetworkCommand can 
    // implement on behalf of a control panel.
    // 
    enum {
	HelpOnNetwork,		// Display help on the network. 
	SetNetworkName,		// Set the name of the network.
	SaveNetwork,		// Save the network with current name.
	SaveNetworkAs,		// Save network with new name.
	OpenConfiguration,	// 
	SaveConfiguration	// 
    };

    //
    // Only activate the save and SaveAs commands when theDXApplication 
    // allows saving networks as defined by appAllowsSavingNetFile()
    //
    virtual void activate();

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassNoUndoNetworkCommand;
    }
};


#endif // _NoUndoNetworkCommand_h
