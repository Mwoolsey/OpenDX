/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#ifndef _NoUndoAnchorCommand_h
#define _NoUndoAnchorCommand_h


#include "NoUndoCommand.h"

typedef long AnchorCommandType;

//
// Class name definition:
//
#define ClassNoUndoAnchorCommand	"NoUndoAnchorCommand"

class DXAnchorWindow;

//
// NoUndoAnchorCommand class definition:
//				
class NoUndoAnchorCommand : public NoUndoCommand
{
    
  protected:
    //
    // Protected member data:
    //
    DXAnchorWindow *anchorWindow;
    AnchorCommandType commandType;
 
    virtual boolean doIt(CommandInterface *ci);

  public:
    //
    // Constructor:
    //
    NoUndoAnchorCommand(const char*   name,
                   CommandScope  *scope,
                   boolean       active,
		   DXAnchorWindow  *fw,
		   AnchorCommandType comType);

    //
    // Destructor:
    //
    ~NoUndoAnchorCommand(){}

    // 
    // These are the various operations that the NoUndoAnchorCommand can 
    // implement on behalf of a DXAnchorWidnow.
    // 
    enum {
	OpenVPE	        = 4	// Open an editor on theDXApplication->network
    };
    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassNoUndoAnchorCommand;
    }
};


#endif // _NoUndoAnchorCommand_h
