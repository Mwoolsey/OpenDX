/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#ifndef _NoUndoDXWindowCommand_h
#define _NoUndoDXWindowCommand_h


#include "NoUndoCommand.h"

typedef long DXWindowCommandType;

//
// Class name definition:
//
#define ClassNoUndoDXWindowCommand	"NoUndoDXWindowCommand"

class   DXWindow;

//
// NoUndoDXWindowCommand class definition:
//				
class NoUndoDXWindowCommand : public NoUndoCommand
{
    
  protected:
    //
    // Protected member data:
    //
    DXWindow *dxWindow;
    DXWindowCommandType commandType;
 
    virtual boolean doIt(CommandInterface *ci);

  public:
    //
    // Constructor:
    //
    NoUndoDXWindowCommand(const char*   name,
                   CommandScope  *scope,
                   boolean       active,
		   DXWindow  *cp,
		   DXWindowCommandType comType);

    //
    // Destructor:
    //
    ~NoUndoDXWindowCommand(){}

    // 
    // These are the various operations that the NoUndoDXWindowCommand can 
    // implement on behalf of a DXWindow.
    // 
    enum {
	ToggleWindowStartup	= 1	// Flip startup state of window on/off
    };
    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassNoUndoDXWindowCommand;
    }
};


#endif // _NoUndoDXWindowCommand_h
