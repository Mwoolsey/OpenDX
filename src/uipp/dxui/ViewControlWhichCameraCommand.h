/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#ifndef _ViewControlWhichCameraCommand_h
#define _ViewControlWhichCameraCommand_h


#include "NoUndoCommand.h"


//
// Class name definition:
//
#define ClassViewControlWhichCameraCommand	"ViewControlWhichCameraCommand"

//
// Referenced classes.
//
class ViewControlDialog;

//
// ViewControlWhichCameraCommand class definition:
//				
class ViewControlWhichCameraCommand : public NoUndoCommand
{
  private:
    //
    // Private member data:
    //
    ViewControlDialog *viewControlDialog;

  protected:
    //
    // Protected member data:
    //

    virtual boolean doIt(CommandInterface *ci);


  public:
    //
    // Constructor:
    //
    ViewControlWhichCameraCommand(const char   *name,
		   CommandScope      *scope,
		   boolean            active,
		   ViewControlDialog *w);

    //
    // Destructor:
    //
    ~ViewControlWhichCameraCommand(){}

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassViewControlWhichCameraCommand;
    }
};


#endif // _ViewControlWhichCameraCommand_h
