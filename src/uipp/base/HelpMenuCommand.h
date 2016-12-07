/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#ifndef _HelpMenuCommand_h
#define _HelpMenuCommand_h


#include "NoUndoCommand.h"

typedef long HelpMenuCommandType;

//
// Class name definition:
//
#define ClassHelpMenuCommand	"HelpMenuCommand"


//
// HelpMenuCommand class definition:
//				
class HelpMenuCommand : public NoUndoCommand
{
    
  protected:
    //
    // Protected member data:
    //
    HelpMenuCommandType commandType;
 
    virtual boolean doIt(CommandInterface *ci);

  public:
    //
    // Constructor:
    //
    HelpMenuCommand(const char*   name,
                   CommandScope  *scope,
                   boolean       active,
		   HelpMenuCommandType comType);

    //
    // Destructor:
    //
    ~HelpMenuCommand(){}

    // 
    // These are the various operations that the HelpMenuCommand can 
    // implement on behalf of a the Application.
    // 
    enum {
	GenericHelp,	// Use CommandInterface name as help tag
	HelpTutorial,	// Start tutorial for the application 
	TechSupport,	// Provide info on obtaining techical support 
	AboutApp	// Put up a dialog that shows application info. 
    };


    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassHelpMenuCommand;
    }
};


#endif // _HelpMenuCommand_h
