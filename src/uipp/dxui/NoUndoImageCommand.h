/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#ifndef _NoUndoImageCommand_h
#define _NoUndoImageCommand_h


#include "NoUndoCommand.h"

typedef long ImageCommandType;

//
// Class name definition:
//
#define ClassNoUndoImageCommand	"NoUndoImageCommand"

class   ImageWindow;

//
// NoUndoImageCommand class definition:
//				
class NoUndoImageCommand : public NoUndoCommand
{
    
  protected:
    //
    // Protected member data:
    //
    ImageWindow 	*image;
    ImageCommandType 	commandType;
 
    virtual boolean doIt(CommandInterface *ci);

  public:
    //
    // Constructor:
    //
    NoUndoImageCommand(const char*   name,
                   CommandScope  *scope,
                   boolean       active,
		   ImageWindow  *image,
		   ImageCommandType comType);

    //
    // Destructor:
    //
    ~NoUndoImageCommand(){}

    // 
    // These are the various operations that the NoUndoImageCommand can 
    // implement on behalf of an image.
    // 
    enum {
	SetBGColor		= 1,	// Post the SetBGColor dialog...
	ViewControl		= 2,	// Post the ViewControlDialog...
	RenderingOptions	= 3,	// Post the RenderingOptionsDialog...
	Throttle		= 4,	// Post the ThrottleDialog...
	ChangeImageName		= 5,	// Post the Change Image Name Dialog...
	AutoAxes		= 6,	// Post the AutoAxes Dialog...
	OpenVPE			= 7,    // Open VPE window...
	DisplayGlobe		= 8,    // Display the Globe...
	Depth8			= 9,    // Set the image depth to 8
	Depth12			= 10,   // Set the image depth to 12
	Depth24			= 11,   // Set the image depth to 24
	SetCPAccess		= 12,   // Set the Control Panel access
	SaveImage		= 13,   // Save image 
	SaveAsImage		= 14,   // Post the SaveAsImageDialog...
	PrintImage		= 15,   // Post the PrintImageDialog...
	Depth16			= 16,   // Set the image depth to 16
	Depth15			= 17,    // Set the image depth to 15
	Depth32			= 18	// Set the image depth to 32
    };
    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassNoUndoImageCommand;
    }
};


#endif // _NoUndoImageCommand_h
