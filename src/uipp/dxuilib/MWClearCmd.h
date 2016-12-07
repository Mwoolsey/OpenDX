/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#ifndef _MWClearCmd_h
#define _MWClearCmd_h


#include "NoUndoCommand.h"


//
// Class name definition:
//
#define ClassMWClearCmd	"MWClearCmd"

class MsgWin;

//
// MWClearCmd class definition:
//				
class MWClearCmd : public NoUndoCommand
{
  private:
    //
    // Private member data:
    //

  protected:
    //
    // Protected member data:
    //
    MsgWin *messageWindow;

    virtual boolean doIt(CommandInterface *ci);

  public:
    //
    // Constructor:
    //
    MWClearCmd(const char *name,
	       CommandScope *scope,
	       boolean active,
	       MsgWin *win);

    //
    // Destructor:
    //
    ~MWClearCmd(){}

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassMWClearCmd;
    }
};


#endif // _MWClearCmd_h
