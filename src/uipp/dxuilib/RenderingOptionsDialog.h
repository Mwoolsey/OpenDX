/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#ifndef _RenderingOptionsDialog_h
#define _RenderingOptionsDialog_h


#include "Dialog.h"


//
// Class name definition:
//
#define ClassRenderingOptionsDialog	"RenderingOptionsDialog"

//
// XtCallbackProc (*CB), XtEventHandler (*EH) and XtActionProc (*AP)
// DialogCallback (*DCB), XtInputCallbackProc (*ICP), XtWorkProc (*WP)
// functions for this and derived classes
//
extern "C" void RenderingOptionsDialog_NumberCB(Widget, XtPointer, XtPointer);

//
// Referenced Classes
//
class ToggleButtonInterface;
class ButtonInterface;
class Command;
class ImageWindow;



//
// RenderingOptionsDialog class definition:
//				
class RenderingOptionsDialog : public Dialog
{
  private:
    //
    // Private member data:
    //
    static boolean ClassInitialized;
    friend void RenderingOptionsDialog_NumberCB(Widget	 widget,
		         XtPointer clientData,
		         XtPointer callData);
  protected:
    //
    // Protected member data:
    //
    ImageWindow *imageWindow;
    static String DefaultResources[];

    ToggleButtonInterface	*hardwareButton;
    ToggleButtonInterface	*softwareButton;

    Widget buttonUpOptionMenu;
    ButtonInterface	*upNone;
    ButtonInterface	*upWireframe;
    ButtonInterface	*upDots;
    ButtonInterface	*upBox;
    Widget upEveryNumber;

    Widget buttonDownOptionMenu;
    ButtonInterface	*downNone;
    ButtonInterface	*downWireframe;
    ButtonInterface	*downDots;
    ButtonInterface	*downBox;
    Widget downEveryNumber;


    virtual Widget createDialog(Widget parent);
    virtual Widget createUpPulldown(Widget parent);
    virtual Widget createDownPulldown(Widget parent);

    //
    // Install the default resources for this class and then call the
    // same super class method to get the default resources from the
    // super classes.
    //
    virtual void installDefaultResources(Widget baseWidget);

  public:
    //
    // Constructor:
    //
    RenderingOptionsDialog(Widget parent, ImageWindow *w);

    //
    // Destructor:
    //
    ~RenderingOptionsDialog();

    virtual void manage();
    virtual void resetApproximations();

    void sensitizeRenderMode(boolean flag);
    void sensitizeButtonUpApprox(boolean flag);
    void sensitizeButtonDownApprox(boolean flag);
    void sensitizeButtonUpDensity(boolean flag);
    void sensitizeButtonDownDensity(boolean flag);

    void update();

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassRenderingOptionsDialog;
    }
};


#endif // _RenderingOptionsDialog_h
