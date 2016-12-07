/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"


#ifndef _ViewControlDialog_h
#define _ViewControlDialog_h


#include "Dialog.h"
#include "Dictionary.h"

//
// Class name definition:
//
#define ClassViewControlDialog	"ViewControlDialog"

//
// XtCallbackProc (*CB), XtEventHandler (*EH) and XtActionProc (*AP)
// DialogCallback (*DCB), XtInputCallbackProc (*ICP), XtWorkProc (*WP)
// functions for this and derived classes
//
extern "C" void ViewControlDialog_SelectPickCB(Widget, XtPointer, XtPointer);
extern "C" void ViewControlDialog_SelectProbeCB(Widget, XtPointer, XtPointer);
extern "C" void ViewControlDialog_NumberCB(Widget, XtPointer, XtPointer);
extern "C" void ViewControlDialog_ScaleCB(Widget, XtPointer, XtPointer);

//
// Referenced Classes
//
class ButtonInterface;
class Command;
class ImageWindow;


//
// ViewControlDialog class definition:
//				
class ViewControlDialog : public Dialog
{
  friend class  ImageWindow;

  private:
    //
    // Private member data:
    //
    static boolean ClassInitialized;

    friend void ViewControlDialog_ScaleCB(Widget	 widget,
		        XtPointer clientData,
		        XtPointer callData);
    friend void ViewControlDialog_NumberCB(Widget	 widget,
		         XtPointer clientData,
		         XtPointer callData);
    friend void ViewControlDialog_SelectProbeCB(Widget widget,
			      XtPointer clientData,
			      XtPointer callData);
    friend void ViewControlDialog_SelectPickCB(Widget widget,
			      XtPointer clientData,
			      XtPointer callData);

    // Is the form going to expand due to the mode change?
    boolean isExpanding();

  protected:
    //
    // Protected member data:
    //
    ImageWindow *imageWindow;
    static String DefaultResources[];
    Dictionary probeWidgetList;  
    Dictionary pickWidgetList;  


    Widget mainForm;
    Widget modeOptionMenu;
    ButtonInterface	*modeNone;
    ButtonInterface	*modeCamera;
    ButtonInterface	*modeCursors;
    ButtonInterface	*modePick;
    ButtonInterface	*modeNavigate;
    ButtonInterface	*modePanZoom;
    ButtonInterface	*modeRoam;
    ButtonInterface	*modeRotate;
    ButtonInterface	*modeZoom;
    Widget setViewOptionMenu;
    ButtonInterface	*setViewNone;
    ButtonInterface	*setViewTop;
    ButtonInterface	*setViewBottom;
    ButtonInterface	*setViewFront;
    ButtonInterface	*setViewBack;
    ButtonInterface	*setViewLeft;
    ButtonInterface	*setViewRight;
    ButtonInterface	*setViewDiagonal;
    ButtonInterface	*setViewOffTop;
    ButtonInterface	*setViewOffBottom;
    ButtonInterface	*setViewOffFront;
    ButtonInterface	*setViewOffBack;
    ButtonInterface	*setViewOffLeft;
    ButtonInterface	*setViewOffRight;
    ButtonInterface	*setViewOffDiagonal;
    Widget projectionOptionMenu;
    ButtonInterface     *orthographic;
    ButtonInterface     *perspective;
    Widget viewAngleStepper;
    ButtonInterface	*constraintNone;
    ButtonInterface	*constraintX;
    ButtonInterface	*constraintY;
    ButtonInterface	*constraintZ;



    boolean cursorFormManaged;
    Widget cursorForm;
    Widget probeSeparator;
    Widget probeLabel;
    Widget probePulldown;
    Widget probeOptionMenu;
    Widget constraintLabel;
    Widget constraintOptionMenu;

    boolean roamFormManaged;

    boolean pickFormManaged;
    Widget pickForm;
    Widget pickPulldown;
    Widget pickOptionMenu;

    boolean cameraFormManaged;
    Widget cameraForm;
    Widget cameraWhichOptionMenu;
    ButtonInterface	*cameraTo;
    ButtonInterface	*cameraFrom;
    ButtonInterface	*cameraUp;
    Widget cameraXNumber;
    Widget cameraYNumber;
    Widget cameraZNumber;
    Widget cameraWidthNumber;
    Widget cameraWindowWidthNumber;
    Widget cameraWindowHeightNumber;

    boolean navigateFormManaged;
    Widget navigateForm;
    Widget motionScale;
    Widget pivotScale;
    Widget navigateLookOptionMenu;
    ButtonInterface     *lookForward;
    ButtonInterface     *lookLeft45;
    ButtonInterface     *lookRight45;
    ButtonInterface     *lookUp45;
    ButtonInterface     *lookDown45;
    ButtonInterface     *lookLeft90;
    ButtonInterface     *lookRight90;
    ButtonInterface     *lookUp90;
    ButtonInterface     *lookDown90;
    ButtonInterface     *lookBackward;
    ButtonInterface     *lookAlign;
   
    ButtonInterface	*undoButton;
    ButtonInterface	*redoButton;
    ButtonInterface	*resetButton;

    Widget buttonForm;

    Command *cameraToVectorCmd;
    Command *cameraFromVectorCmd;
    Command *cameraUpVectorCmd;

    virtual Widget createDialog(Widget parent);
    virtual Widget createModePulldown(Widget parent);
    virtual Widget createSetViewPulldown(Widget parent);
    virtual Widget createProjectionPulldown(Widget parent);
    virtual Widget createConstraintPulldown(Widget parent);
    virtual Widget createCameraWhichPulldown(Widget parent);
    virtual Widget createNavigateLookPulldown(Widget parent);

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
    ViewControlDialog(Widget parent, ImageWindow *w);

    //
    // Destructor:
    //
    ~ViewControlDialog();

    virtual void createProbePulldown();
    virtual void createPickPulldown();
    virtual void manage();
    virtual void manageCursorForm();
    virtual void unmanageCursorForm();
    virtual void managePickForm();
    virtual void unmanagePickForm();
    virtual void manageCameraForm();
    virtual void unmanageCameraForm();
    virtual void manageNavigationForm();
    virtual void unmanageNavigationForm();
    virtual void manageRoamForm();
    virtual void unmanageRoamForm();

    virtual void resetMode();
    virtual void resetLookDirection();
    virtual void resetSetView();
    virtual void resetProjection();

    virtual void setNavigateSpeed(double s);
    virtual void setNavigatePivot(double s);

    virtual void newCamera(double *from, double *to, double *up,
	int image_width, int image_height, double width,
	boolean perspective, double viewAngle);
    void setSensitivity(boolean s);

    void setWhichCameraVector();
    //
    // Returns a pointer to the class name.
    //

    void setCurrentProbeByInstance(int i);
    void setCurrentPickByInstance(int i);

    void sensitizeProbeOptionMenu(Boolean sensitize);
    void sensitizePickOptionMenu(Boolean sensitize);


    const char* getClassName()
    {
	return ClassViewControlDialog;
    }
};


#endif // _ViewControlDialog_h
