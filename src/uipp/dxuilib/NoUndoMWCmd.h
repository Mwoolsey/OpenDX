/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#ifndef _NoUndoMWCmd_h
#define _NoUndoMWCmd_h


#include "NoUndoCommand.h"

typedef long MWCmdType;

//
// Class name definition:
//
#define ClassNoUndoMWCmd	"NoUndoMWCmd"

class   MsgWin;

//
// NoUndoMWCmd class definition:
//				
class NoUndoMWCmd : public NoUndoCommand
{
    
  protected:
    //
    // Protected member data:
    //
    MsgWin	*messageWindow;
    MWCmdType 	 commandType;
 
    virtual boolean doIt(CommandInterface *ci);

  public:
    //
    // Constructor:
    //
    NoUndoMWCmd(const char*   name,
                CommandScope  *scope,
                boolean       active,
		MsgWin       *messageWindow,
		MWCmdType     comType);

    //
    // Destructor:
    //
    ~NoUndoMWCmd(){}

    // 
    // These are the various operations that the NoUndoMWCmd can 
    // implement on behalf of an image.
    // 
    enum {
	Log			= 1,	// Post the log dialog.
	Save			= 2,	// Post the save dialog.
	Close			= 3,	// Unmanage the window.
	NextError		= 4,	// Find the next/previous error.
	PrevError		= 5,	// Find the next/previous error.
	ExecScript		= 6,	// Allow the user to issue a script cmd 
	Tracing			= 7,	// Automatically issue trace on/off cmds
	Memory			= 8	// Automatically issue Usage cmd
    };
    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassNoUndoMWCmd;
    }
};


#endif // _NoUndoMWCmd_h
