/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"



#ifndef _ColormapEditCommand_h
#define _ColormapEditCommand_h


#include "NoUndoCommand.h"
#include <Xm/Xm.h>

class  ColormapEditor;

//
// ColormapEditCommand class definition:
//				

#define ClassColormapEditCommand  "ColormapEditCommand"

#define DISPLAY_OFF 0
#define DISPLAY_ALL 1
#define DISPLAY_SELECTED 2

class ColormapEditCommand : public NoUndoCommand 
{
  private:

    static  String	DefaultResources[];
    ColormapEditor* 	editor;
    int     option;

  protected:
    virtual boolean doIt(CommandInterface *ci);

  public:
    //
    // Constructor:
    //
    ColormapEditCommand(const char*,
		CommandScope*,
		boolean active,
                ColormapEditor*,
		int option);

    enum {
           New,
           Undo,
           Copy,
           Paste,
           AddControl,
           NBins,
           Waveform,
           DeleteSelected,
           SelectAll,
           SetBackground,
           Ticks,
           Histogram,
           LogHistogram,
           ConstrainH,
           ConstrainV,
	   DisplayCPOff,
	   DisplayCPAll,
	   DisplayCPSelected,
	   ResetAll,
	   ResetMin,
	   ResetMax,
	   ResetMinMax,
	   ResetHSV,
	   ResetOpacity,
	   SetColormapName
    };

	
    //
    // Destructor:
    //
    ~ColormapEditCommand(){}

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassColormapEditCommand;
    }
};


#endif // _ColormapEditCommand_h
