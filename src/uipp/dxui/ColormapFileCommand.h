/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"


#ifndef _ColormapFileCommand_h
#define _ColormapFileCommand_h


#include "NoUndoCommand.h"

#include <Xm/Xm.h>


//
// Class name definition:
//
#define ClassColormapFileCommand	"ColormapFileCommand"

class ColormapEditor;

//
// ColormapFileCommand class definition:
//				
class ColormapFileCommand : public NoUndoCommand
{

  private:
    ColormapEditor    *editor;
    boolean		opening;

  protected:
    //
    // Implements the command:

    virtual boolean doIt(CommandInterface *ci);

  public:
    //
    // Constructor:
    //
    ColormapFileCommand(const char*     name,
                        CommandScope*   scope,
                        boolean         active,
                        ColormapEditor* editor,
			boolean		opening);


    //
    // Destructor:
    //
    ~ColormapFileCommand();

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassColormapFileCommand;
    }
};


#endif // _ColormapFileCommand_h
