/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#ifndef _NoUndoDXAppCommand_h
#define _NoUndoDXAppCommand_h


#include "NoUndoCommand.h"

typedef long DXAppCommandType;

//
// Class name definition:
//
#define ClassNoUndoDXAppCommand	"NoUndoDXAppCommand"

class   DXApplication;

//
// NoUndoDXAppCommand class definition:
//				
class NoUndoDXAppCommand : public NoUndoCommand
{
    
  protected:
    //
    // Protected member data:
    //
    DXApplication     *application;
    DXAppCommandType commandType;
 
    virtual boolean doIt(CommandInterface *ci);

  public:
    //
    // Constructor:
    //
    NoUndoDXAppCommand(const char*   name,
                   CommandScope  *scope,
                   boolean       active,
		   DXApplication *application,
		   DXAppCommandType comType);

    //
    // Destructor:
    //
    ~NoUndoDXAppCommand(){}

    // 
    // These are the various operations that the NoUndoDXAppCommand can 
    // implement on behalf of a DXApplication.
    // 
    enum {
	StartServer		= 1,	// Open a connection to the server 
	ResetServer 		= 2,	// Reset the server 
        ExecuteOnce		= 3,	// Execute the network once. 
        ExecuteOnChange		= 4,	// Go into Execute on change mode
        EndExecution		= 5,	// End execute on change mode
        LoadMacro		= 6,	// Load a macro
        OpenAllColormaps	= 8,	// Open all the network's colormaps
        OpenMessageWindow	= 9,	// Open the list of errors 
        OpenNetwork		= 10,	// Open and read a network file 
        OpenSequencer		= 11,	// Open the network's sequencer 
#if USE_REMAP   // 6/14/93
	RemapInteractorOutputs  = 12,	// Toggle interactor output rescaling
#endif
	ToggleInfoEnable  	= 13,	// Toggle Information Messages
	ToggleWarningEnable	= 14,	// Toggle Warning Messages
	ToggleErrorEnable	= 15,	// Toggle Error Messages
	LoadUserMDF             = 16,	// Load another MDF. 
	AssignProcessGroup	= 17,   // Open Process Group Assignment dialog
	HelpOnManual		= 18,   // Help command
	HelpOnHelp		= 19    // Help command
    };


    //
    // Conditionally call the super class method.  Currently, we don't
    // activate if there is no connection to the server and we are activating
    // ExecuteOnChange, ExecuteOnce or EndExecution command.
    //
    virtual void activate();

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassNoUndoDXAppCommand;
    }
};


#endif // _NoUndoDXAppCommand_h
