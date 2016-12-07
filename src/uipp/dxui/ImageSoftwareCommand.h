/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#ifndef _ImageSoftwareCommand_h
#define _ImageSoftwareCommand_h


#include "NoUndoCommand.h"


//
// Class name definition:
//
#define ClassImageSoftwareCommand	"ImageSoftwareCommand"

//
// Referenced classes.
//
class ImageWindow;

//
// ImageSoftwareCommand class definition:
//				
class ImageSoftwareCommand : public NoUndoCommand
{
  private:
    //
    // Private member data:
    //
    ImageWindow *imageWindow;

  protected:
    //
    // Protected member data:
    //

    virtual boolean doIt(CommandInterface *ci);


  public:
    //
    // Constructor:
    //
    ImageSoftwareCommand(const char   *name,
		   CommandScope *scope,
		   boolean       active,
		   ImageWindow  *w);

    //
    // Destructor:
    //
    ~ImageSoftwareCommand(){}

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassImageSoftwareCommand;
    }
};


#endif // _ImageSoftwareCommand_h
