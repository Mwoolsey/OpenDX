/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#ifndef _NoUndoHelpCmd_h
#define _NoUndoHelpCmd_h


#include "NoUndoCommand.h"

typedef long HelpCmdType;

//
// Class name definition:
//
#define ClassNoUndoHelpCmd	"NoUndoHelpCmd"

class   HelpWin;

//
// NoUndoHelpCmd class definition:
//				
class NoUndoHelpCmd : public NoUndoCommand
{
    
  protected:
    //
    // Protected member data:
    //
    HelpWin	*helpWin;
    HelpCmdType 	 commandType;
 
    virtual boolean doIt(CommandInterface *ci);

  public:
    //
    // Constructor:
    //
    NoUndoHelpCmd(const char*   name,
                CommandScope  *scope,
                boolean       active,
		HelpWin      *helpWin,
		HelpCmdType     comType);

    //
    // Destructor:
    //
    ~NoUndoHelpCmd(){}

    // 
    // These are the various operations that the NoUndoHelpCmd can 
    // implement on behalf of an image.
    // 
    enum {
	Close			= 3	// Unmanage the window.
    };
    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassNoUndoHelpCmd;
    }
};


#endif // _NoUndoHelpCmd_h
