/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#ifndef _NoUndoPanelCommand_h
#define _NoUndoPanelCommand_h


#include "NoUndoCommand.h"

typedef long PanelCommandType;

//
// Class name definition:
//
#define ClassNoUndoPanelCommand	"NoUndoPanelCommand"

class   ControlPanel;

//
// NoUndoPanelCommand class definition:
//				
class NoUndoPanelCommand : public NoUndoCommand
{
    
  protected:
    //
    // Protected member data:
    //
    ControlPanel *controlPanel;
    PanelCommandType commandType;
 
    virtual boolean doIt(CommandInterface *ci);

  public:
    //
    // Constructor:
    //
    NoUndoPanelCommand(const char*   name,
                   CommandScope  *scope,
                   boolean       active,
		   ControlPanel  *cp,
		   PanelCommandType comType);

    //
    // Destructor:
    //
    ~NoUndoPanelCommand(){}

    // 
    // These are the various operations that the NoUndoPanelCommand can 
    // implement on behalf of a control panel.
    // 
    enum {
	AddInteractors		= 1,	// Add Selected Interactors
	ShowInteractors		= 2,	// Show Selected Interactors
	DeleteInteractors	= 3,	// Delete selected interactors
	SetInteractorAttributes = 4,	// Set Attributes of Selected interactor
	SetInteractorLabel	= 5,	// Set the selected interactors label 
	SetPanelComment		= 6,	// Set the control panel comment
	SetPanelName 		= 7,	// Set the control panel name 
	SetPanelGrid		= 8,	// Manipulate the grid settings 
#if 0
	TogglePanelStartup	= 9,	// Flip startup state of panel on/off
#endif
#if 00
	OpenFile		= 10,	// Open a cfg file
	SaveFile		= 11,   // Save a cfg file
#endif
	SetHorizontalLayout	= 12,	// Change the Layout of an interactor 
	SetVerticalLayout	= 13,   // Change the Layout of an interactor 
	SetPanelAccess		= 14,   // Pop up the PanelAccessDialog
	ShowStandIns		= 15,	// Show the Selected Interactor's Node 
	HelpOnPanel		= 16,	// Display help on this panel.
	TogglePanelStyle	= 17,	// Toggle between user/developer mode
	HitDetection		= 18 	// Toggle a Workspace widget resource
    };
    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassNoUndoPanelCommand;
    }
};


#endif // _NoUndoPanelCommand_h
