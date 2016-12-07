/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#ifndef _NoUndoChooserCommand_h
#define _NoUndoChooserCommand_h


#include "NoUndoCommand.h"

typedef long ChooserCommandType;

class GARChooserWindow;

//
// Class name definition:
//
#define ClassNoUndoChooserCommand	"NoUndoChooserCommand"

//
// NoUndoChooserCommand class definition:
//				
class NoUndoChooserCommand : public NoUndoCommand
{
    
  protected:
    //
    // Protected member data:
    //
    GARChooserWindow     *chooser;
    ChooserCommandType commandType;
 
    virtual boolean doIt(CommandInterface *ci);

  public:
    //
    // Constructor:
    //
    NoUndoChooserCommand(const char*   name,
                   CommandScope  *scope,
                   boolean       active,
		   GARChooserWindow *chooser,
		   ChooserCommandType comType);

    //
    // Destructor:
    //
    ~NoUndoChooserCommand(){}

    // 
    // These are the various operations 
    // 
    enum {
	ShowMessages	= 2,  
	Browse		= 3,
	Visualize	= 4,
	SetGridType	= 5,
	Positions	= 6,
	NoOp		= 7,
	Prompter	= 8,
	RestrictNames	= 9,
	SpecifyRows	= 10,
	SelectDataFile	= 11,
	OpenPrompter    = 12,
	VerifyData	= 99
    };

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassNoUndoChooserCommand;
    }
};


#endif // _NoUndoChooserCommand_h
