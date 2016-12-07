/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#ifndef _NoUndoTutorAppCommand_h
#define _NoUndoTutorAppCommand_h


#include "NoUndoCommand.h"

typedef long TutorAppCommandType;

//
// Class name definition:
//
#define ClassNoUndoTutorAppCommand	"NoUndoTutorAppCommand"

class   TutorApplication;

//
// NoUndoTutorAppCommand class definition:
//				
class NoUndoTutorAppCommand : public NoUndoCommand
{
    
  protected:
    //
    // Protected member data:
    //
    TutorApplication     *application;
    TutorAppCommandType commandType;
 
    virtual boolean doIt(CommandInterface *ci);

  public:
    //
    // Constructor:
    //
    NoUndoTutorAppCommand(const char*   name,
                   CommandScope  *scope,
                   boolean       active,
		   TutorApplication *application,
		   TutorAppCommandType comType);

    //
    // Destructor:
    //
    ~NoUndoTutorAppCommand(){}

    // 
    // These are the various operations that the NoUndoTutorAppCommand can 
    // implement on behalf of a DXApplication.
    // 
    enum {
	Quit			= 0,
	GotoHelpText		= 1
    };


    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassNoUndoTutorAppCommand;
    }
};


#endif // _NoUndoTutorAppCommand_h
