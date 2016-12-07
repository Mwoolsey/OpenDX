/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"


#ifndef _ImageWindow_h
#define _ImageWindow_h

#include <X11/Intrinsic.h>
#include "DXWindow.h"
#include "NoUndoImageCommand.h"
#include "ToggleButtonInterface.h"
#include "../widgets/Picture.h"
#include "enums.h"
#include "Network.h"
//#include "AutoAxesDialog.h"
//#include "SetBGColorDialog.h"


//
// Class name definition:
//
#define ClassImageWindow	"ImageWindow"

//
// XtCallbackProc (*CB), XtEventHandler (*EH) and XtActionProc (*AP)
// DialogCallback (*DCB), XtInputCallbackProc (*ICP), XtWorkProc (*WP)
// functions for this and derived classes
//
extern "C" void ImageWindow_ChangeImageNameCB(Widget, XtPointer, XtPointer);
extern "C" void ImageWindow_TrackFrameBufferEH(Widget, XtPointer, XEvent*, Boolean*);
extern "C" void ImageWindow_PropertyNotifyCB(Widget, XtPointer, XtPointer);
extern "C" void ImageWindow_ClientMessageCB(Widget, XtPointer, XtPointer);
extern "C" void ImageWindow_ResizeCB(Widget, XtPointer, XtPointer);
extern "C" void ImageWindow_UndoCB(Widget, XtPointer, XtPointer);
extern "C" void ImageWindow_KeyCB(Widget, XtPointer, XtPointer);
extern "C" void ImageWindow_ModeCB(Widget, XtPointer, XtPointer);
extern "C" void ImageWindow_PickCB(Widget, XtPointer, XtPointer);
extern "C" void ImageWindow_CursorCB(Widget, XtPointer, XtPointer);
extern "C" void ImageWindow_RotationCB(Widget, XtPointer, XtPointer);
extern "C" void ImageWindow_NavigateCB(Widget, XtPointer, XtPointer);
extern "C" void ImageWindow_RoamCB(Widget, XtPointer, XtPointer);
extern "C" void ImageWindow_ZoomCB(Widget, XtPointer, XtPointer);
extern "C" void ImageWindow_RedrawCB(Widget, XtPointer, XtPointer);
extern "C" void ImageWindow_WindowsMenuMapCB(Widget, XtPointer, XtPointer);
extern "C" void ImageWindow_FileMenuMapCB(Widget, XtPointer, XtPointer);
extern "C" Boolean ImageWindow_ResetEORWP(XtPointer);
extern "C" void ImageWindow_ResizeTO (XtPointer , XtIntervalId* );

//
// Referenced classes:
//
/*
class Network;
*/
class CommandScope;
class CommandInterface;
class Command;
class Node;
class ProbeNode;
class PickNode;
class ViewControlDialog;
class RenderingOptionsDialog;
class Dialog;
class AutoAxesDialog;
class XHandler;
class ThrottleDialog;
class PanelAccessManager;
class SaveImageDialog;
class SetImageNameDialog;
class PrintImageDialog;
class CascadeMenu;
class SetBGColorDialog;
class Stack;

enum ApproxMode 
{
    APPROX_NONE,
    APPROX_WIREFRAME,
    APPROX_DOTS,
    APPROX_BOX
};

enum ViewDirection
{
    VIEW_NONE,
    VIEW_TOP,
    VIEW_BOTTOM,
    VIEW_FRONT,
    VIEW_BACK,
    VIEW_LEFT,
    VIEW_RIGHT,
    VIEW_DIAGONAL,
    VIEW_OFF_TOP,
    VIEW_OFF_BOTTOM,
    VIEW_OFF_FRONT,
    VIEW_OFF_BACK,
    VIEW_OFF_LEFT,
    VIEW_OFF_RIGHT,
    VIEW_OFF_DIAGONAL
};
enum LookDirection
{
    LOOK_FORWARD,
    LOOK_LEFT45,
    LOOK_RIGHT45,
    LOOK_UP45,
    LOOK_DOWN45,
    LOOK_LEFT90,
    LOOK_RIGHT90,
    LOOK_UP90,
    LOOK_DOWN90,
    LOOK_BACKWARD,
    LOOK_ALIGN
};

enum ConstraintDirection
{
    CONSTRAINT_NONE,
    CONSTRAINT_X,
    CONSTRAINT_Y,
    CONSTRAINT_Z
};

struct ImageAtoms
{
    Atom                pick_point;        // Atoms for GL client  */
    Atom                start_pick;        // Atoms for GL client  */
    Atom                set_view;          // Atoms for GL client  */
    Atom                start_rotate;      // Atoms for GL client  */
    Atom                stop;              // Atoms for GL client  */
    Atom                start_zoom;        // Atoms for GL client  */
    Atom                start_panzoom;     // Atoms for GL client  */
    Atom                start_cursor;      // Atoms for GL client  */
    Atom                start_roam;        // Atoms for GL client  */
    Atom                start_navigate;    // Atoms for GL client  */
    Atom                from_vector;       // Atoms for GL client  */
    Atom                up_vector;         // Atoms for GL client  */
    Atom                zoom1;             // Atoms for GL client  */
    Atom                zoom2;             // Atoms for GL client  */
    Atom                roam;              // Atoms for GL client  */
    Atom                cursor_change;     // Atoms for GL client  */
    Atom                cursor_constraint; // Atoms for GL client  */
    Atom                cursor_speed;      // Atoms for GL client  */
    Atom                gl_window0;        // Atoms for GL server  */
    Atom                gl_window1;        // Atoms for GL server  */
    Atom                gl_window2;        // Atoms for GL server  */
    Atom                gl_window2_execute;// Atoms for GL server  */
    Atom                execute_on_change; // Atoms for GL server  */
    Atom                gl_destroy_window; // Atoms for GL server  */
    Atom                gl_shutdown;       // Atoms for GL server  */
    Atom                image_reset;       // Atoms for DX server  */
    Atom                dx_pixmap_id;      // Atoms for DX server  */
    Atom                dx_flag;           // Atoms for DX server  */
    Atom                display_globe;     // Atoms for DX server  */
    Atom                motion;            // Atoms for DX server  */
    Atom                pivot;             // Atoms for DX server  */
    Atom                undoable;          // Atoms for DX server  */
    Atom                redoable;          // Atoms for DX server  */
    Atom                undo_camera;       // Atoms for DX server  */
    Atom                redo_camera;       // Atoms for DX server  */
    Atom                push_camera;       // Atoms for DX server  */
    Atom                button_mapping1;   // Atoms for DX server  */
    Atom                button_mapping2;   // Atoms for DX server  */
    Atom                button_mapping3;   // Atoms for DX server  */
    Atom                navigate_look_at;  // Atoms for DX server  */
};
struct ImageCamera {
    double  to[3];
    double  up[3];
    double  from[3];
    double  width;
    int     windowWidth;
    int     windowHeight;
    double  viewAngle;
    double  zoomFactor;
    double  aspect;
    int     projection;
    boolean undoable;
    boolean redoable;
};
struct ImageState {
    int			width;
    int			height;
    Pixmap		pixmap;
    GC			gc;
    boolean		hardwareRender;
    boolean		hardwareRenderExists;
    boolean		resizeFromServer;
    boolean		frameBuffer;
    long		hardwareWindow;
    ImageCamera		hardwareCamera;
    boolean		globeDisplayed;
    boolean		degenerateBox;
    int			imageCount;
    boolean		resizeCausesExecution;
    //
    // when turning resizeCausesExecution back on (it's turned off during
    // parsing), set a flag which will cause it to be turned on after a resize
    // event is delivered.  Otherwise the async arrival of a resize event will
    // cause an execution.
    //
    struct {
	Window			window;
	int			x;
	int			y;
	int			width;
	int			height;
    }			parent;
    double 		navigateTranslateSpeed;
    double 		navigateRotateSpeed;
};


//
// ImageWindow class definition:
//				
class ImageWindow : public DXWindow
{
    friend class NoUndoImageCommand;

  private:
    //
    // Private class data:
    //
    static boolean ClassInitialized;

    friend void ImageWindow_FileMenuMapCB(Widget, XtPointer, XtPointer);
    friend void ImageWindow_WindowsMenuMapCB(Widget, XtPointer, XtPointer);

    //
    // Static callback routines.
    //
    friend void ImageWindow_RedrawCB(Widget	 drawingArea,
		         XtPointer clientData,
		         XtPointer callData);
    friend void ImageWindow_ZoomCB(Widget	 drawingArea,
		       XtPointer clientData,
		       XtPointer callData);
    friend void ImageWindow_RoamCB(Widget	 drawingArea,
		       XtPointer clientData,
		       XtPointer callData);
    friend void ImageWindow_NavigateCB(Widget	 drawingArea,
		           XtPointer clientData,
		           XtPointer callData);
    friend void ImageWindow_RotationCB(Widget	 drawingArea,
		           XtPointer clientData,
		           XtPointer callData);
    friend void ImageWindow_CursorCB(Widget	 drawingArea,
		         XtPointer clientData,
		         XtPointer callData);
    friend void ImageWindow_PickCB(Widget	 drawingArea,
		       XtPointer clientData,
		       XtPointer callData);
    friend void ImageWindow_ModeCB(Widget	 drawingArea,
		       XtPointer clientData,
		       XtPointer callData);
    friend void ImageWindow_KeyCB(Widget	 drawingArea,
		      XtPointer clientData,
		      XtPointer callData);
    friend void ImageWindow_UndoCB(Widget	 drawingArea,
		       XtPointer clientData,
		       XtPointer callData);
    friend void ImageWindow_ResizeCB(Widget	 drawingArea,
		        XtPointer clientData,
		        XtPointer callData);
    friend void ImageWindow_ClientMessageCB(Widget 	  drawingArea,
			        XtPointer clientData,
			        XtPointer callData);
    friend void ImageWindow_PropertyNotifyCB(Widget   drawingArea,
				XtPointer clientData,
				XtPointer callData);
    friend void ImageWindow_TrackFrameBufferEH(Widget    drawingArea,
				   XtPointer clientData,
				   XEvent    *callData,
				   Boolean   *continue_to_dispatch);
    static boolean HandleExposures(XEvent *event, void *clientData);
    friend void ImageWindow_ChangeImageNameCB(Widget    selectBox,
		      		  XtPointer clientData,
		      		  XtPointer callData);
    friend Boolean ImageWindow_ResetEORWP(XtPointer);
    friend void ImageWindow_ResizeTO (XtPointer , XtIntervalId* );
    boolean adjustDepth(int &depth);

    void completePictureCreation();
    List         panelNameList;
    List         panelGroupList;

    //
    // Ah yes, interaction modes.  I recently added the 
    // 'pendingInteractionMode' member, because it seems that under 
    // certain cirmumstances, setInteractionMode() was not actually
    // installing the mode in the window.  In these cases, we save
    // the new mode in pendingInteractionMode and set currentInteractionMode
    // to NONE.  Then at a later time (when we get a canvas) we can
    // apply the pending interaction mode with applyPendingInteractionMode().
    //
    // Install the given mode as the new interaction mode. 
    // If ignoreMatchingModes is FALSE, then we return immediately if the
    // mode is already set otherwise, we go ahead and set the window for
    // the given mode.  See, set/activateInteractionMode().
    //
    DirectInteractionMode pendingInteractionMode;
    DirectInteractionMode currentInteractionMode;
    boolean setInteractionMode(DirectInteractionMode mode,
                        boolean ignoreMatchingModes);
#if 00
    boolean activateInteractionMode();
#endif
    boolean applyPendingInteractionMode();

    Network*		network;

    //
    // Image state 
    //
    boolean		directInteraction;
    ImageAtoms		atoms;
    ImageState		state;
    Stack*		managed_state;

    // are we just switching between hardware and software?
    boolean		switchingSoftware;

    // Have we pushed since the last execution?
    boolean		pushedSinceExec;

    int		currentProbeInstance;	
    int		currentPickInstance;		

    Node		*node;		// Display or Image node associated.


    ViewControlDialog       *viewControlDialog;
    RenderingOptionsDialog  *renderingOptionsDialog;
    SetBGColorDialog        *backgroundColorDialog;
    ThrottleDialog          *throttleDialog;
    AutoAxesDialog          *autoAxesDialog;
    SetImageNameDialog	    *changeImageNameDialog;
    SaveImageDialog         *saveImageDialog;
    PrintImageDialog        *printImageDialog;

    XHandler		    *fbEventHandler;



    virtual void resizeImage(boolean ok_to_send=TRUE);
    virtual void zoomImage(XmPictureCallbackStruct*);
    virtual void roamImage(XmPictureCallbackStruct*);
    virtual void rotateImage(XmPictureCallbackStruct*);
    virtual void navigateImage(XmPictureCallbackStruct*);
    virtual void handleMode(XmPictureCallbackStruct*);
    virtual void handleKey(XmPictureCallbackStruct*);
    virtual void pickImage(double x, double y);
    virtual void handleCursor(int reason, int cursor_num,
			      double x, double y, double z);

    //
    // Image window commands:
    //
    Command		*renderingOptionsCmd;
    Command			*softwareCmd;
    Command			*hardwareCmd;
    Command			*upNoneCmd;
    Command			*upWireframeCmd;
    Command			*upDotsCmd;
    Command			*upBoxCmd;
    Command			*downNoneCmd;
    Command			*downWireframeCmd;
    Command			*downDotsCmd;
    Command			*downBoxCmd;
    Command		*autoAxesCmd;
    Command		*throttleCmd;
    Command		*viewControlCmd;
    Command			*modeNoneCmd;
    Command			*modeCameraCmd;
    Command			*modeCursorsCmd;
    Command			*modePickCmd;
    Command			*modeNavigateCmd;
    Command			*modePanZoomCmd;
    Command			*modeRoamCmd;
    Command			*modeRotateCmd;
    Command			*modeZoomCmd;
    Command			*setViewNoneCmd;
    Command			*setViewTopCmd;
    Command			*setViewBottomCmd;
    Command			*setViewFrontCmd;
    Command			*setViewBackCmd;
    Command			*setViewLeftCmd;
    Command			*setViewRightCmd;
    Command			*setViewDiagonalCmd;
    Command			*setViewOffTopCmd;
    Command			*setViewOffBottomCmd;
    Command			*setViewOffFrontCmd;
    Command			*setViewOffBackCmd;
    Command			*setViewOffLeftCmd;
    Command			*setViewOffRightCmd;
    Command			*setViewOffDiagonalCmd;
    Command			*perspectiveCmd;
    Command			*parallelCmd;
    Command			*constrainNoneCmd;
    Command			*constrainXCmd;
    Command			*constrainYCmd;
    Command			*constrainZCmd;
    Command			*lookForwardCmd;
    Command			*lookLeft45Cmd;
    Command			*lookRight45Cmd;
    Command			*lookUp45Cmd;
    Command			*lookDown45Cmd;
    Command			*lookLeft90Cmd;
    Command			*lookRight90Cmd;
    Command			*lookUp90Cmd;
    Command			*lookDown90Cmd;
    Command			*lookBackwardCmd;
    Command			*lookAlignCmd;
    Command		*undoCmd;
    Command		*redoCmd;
    Command		*resetCmd;
    Command		*changeImageNameCmd;
    Command		*backgroundColorCmd;
    Command		*displayRotationGlobeCmd;
    Command             *openVPECmd;
    CascadeMenu		*modeOptionCascade;
    CascadeMenu		*imageDepthCascade;
    Command             *imageDepth8Cmd;
    Command             *imageDepth12Cmd;
    Command             *imageDepth15Cmd;
    Command             *imageDepth16Cmd;
    Command             *imageDepth24Cmd;
    Command		*imageDepth32Cmd;
    Command		*closeCmd;
    Command		*setPanelAccessCmd;
    Command		*saveImageCmd;
    Command		*printImageCmd;

    //
    // Image window components:
    //
    Widget		form;
    Widget		canvas;

    //
    // Menus & pulldowns:
    //
    Widget		fileMenu;
    Widget		windowsMenu;
    Widget		optionsMenu;

    Widget		fileMenuPulldown;
    Widget		editMenuPulldown;
    Widget		windowsMenuPulldown;
    Widget		optionsMenuPulldown;

    //
    // File menu options:
    //
    CommandInterface*	openOption;
    CommandInterface*	saveOption;
    CommandInterface*	saveAsOption;
    CommandInterface*	openCfgOption;
    CommandInterface*	saveCfgOption;
    CommandInterface*	loadMacroOption;
    CommandInterface*	loadMDFOption;
    CommandInterface*   quitOption;
    CommandInterface*   closeOption;
    CommandInterface*	saveImageOption;
    CommandInterface*	printImageOption;
    CascadeMenu*	cfgSettingsCascadeMenu;

    //
    // Windows menu options:
    //
    CommandInterface*	openVisualProgramEditorOption;
    CommandInterface*	openAllControlPanelsOption;
    CascadeMenu		*openControlPanelByNameMenu;
    CommandInterface*	openAllColormapEditorsOption;
    CommandInterface*	messageWindowOption;

    //
    // Options menu options:
    //
    CommandInterface*	renderingOptionsOption;
    CommandInterface*	autoAxesOption;
    CommandInterface*	throttleOption;
    CommandInterface*	viewControlOption;
    CommandInterface*	modeOption;
    CommandInterface*	undoOption;
    CommandInterface*	redoOption;
    CommandInterface*	resetOption;
    CommandInterface*	changeImageNameOption;
    CommandInterface*	backgroundColorOption;
    CommandInterface*	displayRotationGlobeOption;

    ToggleButtonInterface*	modeNoneOption;
    ToggleButtonInterface*	modeCameraOption;
    ToggleButtonInterface*	modeCursorsOption;
    ToggleButtonInterface*	modePickOption;
    ToggleButtonInterface*	modeNavigateOption;
    ToggleButtonInterface*	modePanZoomOption;
    ToggleButtonInterface*	modeRoamOption;
    ToggleButtonInterface*	modeRotateOption;
    ToggleButtonInterface*	modeZoomOption;

    ToggleButtonInterface*	imageDepth8Option;
    ToggleButtonInterface*	imageDepth12Option;
    ToggleButtonInterface*	imageDepth15Option;
    ToggleButtonInterface*	imageDepth16Option;
    ToggleButtonInterface*	imageDepth24Option;
    ToggleButtonInterface*	imageDepth32Option;
    CommandInterface*	setPanelAccessOption;

    //
    // Addition help menu options:
    //
    CommandInterface*	onVisualProgramOption;

    boolean cameraInitialized;

    //
    // add a work proc for resetting the execute-on-resize flag because
    // resize events are async and must be turned off following parseCommonComments.
    // But before turning it back on, do a sync and wait once.  This is once per
    // class because doing a sync affects all image windows.  Therefore it's not
    // necessary to do one for each window.
    //
    XtWorkProcId reset_eor_wp;
    int  pending_resize_x, pending_resize_y; 
    int  pending_resize_width, pending_resize_height;
    static boolean NeedsSyncForResize;

  protected:

    static String  DefaultResources[];

    //
    // Creates the editor window workarea (as required by MainWindow class).
    //
    virtual Widget createWorkArea(Widget parent);

    //
    // Creates the editor window menus (as required by DXWindow class).
    //
    virtual void createMenus(Widget parent);

    //
    // Creation routine for each of the editor menus:
    //   Intended to be used in derived class createMenus(), if needed,
    //   to better control the menu creation, or to be overriden in
    //   the derived classes.
    //
    virtual void createFileMenu(Widget parent);
    virtual void createWindowsMenu(Widget parent);
    virtual void createOptionsMenu(Widget parent);
    virtual void createHelpMenu(Widget parent);

    //
    // These routines send messages to the gl rendering portion of the
    // server software.
    void sendClientMessage(Atom atom, int num, long  *longs);
    void sendClientMessage(Atom atom, long oneLong)
    {
	this->sendClientMessage(atom, 1, &oneLong);
    }
    void sendClientMessage(Atom atom) 
    {
	this->sendClientMessage(atom, 0, (long*)NULL);
    }
    void sendClientMessage(Atom atom, int num, short *shorts);
    void sendClientMessage(Atom atom, int num, char  *chars);
    void sendClientMessage(Atom atom, int num, float *floats);
    void sendClientMessage(Atom atom, float oneFloat)
    {
	this->sendClientMessage(atom, 1, &oneFloat);
    }

    void wait4GLAcknowledge();

    //
    // Do any image window specific stuff to get the canvas ready for an
    // image.
    virtual void newCanvasImage();
    virtual void trackFrameBuffer(XEvent *event, Boolean *continue_to_dispatch);
    virtual boolean handleExposures(XEvent *event);

    virtual void serverDisconnected();

    void saveImage();

    //
    // Set the state and sensitivity of the ImageDepth cascade menu and its
    // command interfaces.
    //
    void configureImageDepthMenu();

    //
    // Set the state and sensitivity of the Mode cascade menu and its
    // command interfaces.
    //
    void configureModeMenu();


    Node *getNodeByInstance(const char *classname, const char *name, 
					int instance);
    
    //
    // Install the default resources for this class and then call the
    // same super class method to get the default resources from the
    // super classes.
    //
    virtual void installDefaultResources(Widget baseWidget);

    //
    // The superclass has responsibility for the wizard window but here we'll
    // substitute the application comment for the wizard window.  They look pretty
    // much the same.
    //
    virtual void postWizard();

    //
    // Allow subclasses to supply a string for the XmNgeometry resource
    // (i.e. -geam 90x60-0-0) which we'll use instead of createX,createY,
    // createWidth, createHeight when making the new window.  If the string
    // is available then initialize() won't call setGeometry.
    //
    virtual void getGeometryAlternateNames(String*, int*, int);

    //virtual int displayWidth() { return 1280; }
    //virtual int displayHeight() { return 1024; }

    //
    // Rather than triggering an execution following a window resize,
    // Queue the execution so that it happens soon after the execution
    // but not immediately after.  This gives better results for users
    // with window mgrs that deliver tons of resize events
    //
    XtIntervalId execute_after_resize_to;


  public:
    //
    // Constructor for the masses:
    //
    ImageWindow(boolean  isAnchor, Network* network);


    //
    // Destructor:
    //
    ~ImageWindow();

    //
    // Sets the default resources before calling the superclass
    // initialize() function.
    //
    virtual void initialize();

    //
    // Handle client messages (not from the exec).
    //
    virtual void notify
	(const Symbol message, const void *msgdata=NULL, const char *msg=NULL);

    //
    // Redefine unmanage to unmanaged all child dialogs
    virtual void unmanage();

    //
    // Redefine manage to add tracking on frame buffer
    virtual void manage();

    //
    // Redefine setGeometry in order to deal with the WHERE parameter
    void setGeometry (int x, int y, int width, int height);

    // 
    // A routine to install all per-canvas callbacks.
    virtual void installCallbacks();

    //
    // Probe and Pick management:
    //
    void    addProbe(Node*);
    void    deleteProbe(Node*);
    void    changeProbe(Node*);
    boolean selectProbeByInstance(int instanceNumber);
    ProbeNode    *getCurrentProbeNode();
    boolean setCurrentProbe(int instanceNumber);

    void    addPick(Node*);
    void    deletePick(Node*);
    void    changePick(Node*);
    boolean selectPickByInstance(int instanceNumber);
    PickNode   *getCurrentPickNode();
    boolean setCurrentPick(int instanceNumber);

    //
    // three functions to control the hightlighting of the Execute button.
    //
    virtual void beginExecution();
    virtual void standBy();
    virtual void endExecution();

    Widget getCanvas()
    {
	return this->canvas;
    }
    boolean directInteractionAllowed()
    {
	return this->directInteraction;
    }
    void allowDirectInteraction(boolean allow);

    virtual void clearFrameBufferOverlay();

    //
    // Associates an Image or Display style node with an ImageWindow.
    // Returns TRUE if there wasn't another node already associated, FALSE
    // if there was.
    boolean associateNode(Node *n);
    Node   *getAssociatedNode() { return this->node; }
    boolean isAssociatedWithNode();


    boolean enablePerspective(boolean enable);
    boolean getPerspective();
    void setViewAngle(double viewAngle);
    void getViewAngle(double &viewAngle);
    void setResolution(int x, int y);
    void getResolution(int &x, int &y);
    void setWidth(double w);
    void getWidth(double &w);
    void setTo(double *v);
    void setFrom(double *v);
    void setUp(double *v);
    void getTo(double *v);
    void getFrom(double *v);
    void getUp(double *v);
    void getThrottle(double &v);
    void setThrottle(double v);
    boolean updateThrottleDialog(double v);

    boolean isAutoAxesCursorSet ();
    boolean isAutoAxesCornersSet ();
    boolean isSetAutoAxesFrame ();
    boolean isSetAutoAxesGrid ();
    boolean isSetAutoAxesAdjust ();
    boolean isAutoAxesAnnotationSet ();
    boolean isAutoAxesLabelsSet ();
    boolean isAutoAxesColorsSet ();
    boolean isAutoAxesFontSet ();
    boolean isAutoAxesTicksSet ();
    boolean isAutoAxesLabelScaleSet ();
    boolean isAutoAxesXTickLocsSet ();
    boolean isAutoAxesYTickLocsSet ();
    boolean isAutoAxesZTickLocsSet ();
    boolean isAutoAxesXTickLabelsSet ();
    boolean isAutoAxesYTickLabelsSet ();
    boolean isAutoAxesZTickLabelsSet ();

    boolean isBGColorConnected ();
    boolean isThrottleConnected ();
    boolean isRecordEnableConnected ();
    boolean isRecordFileConnected ();
    boolean isRecordFormatConnected ();
    boolean isRecordResolutionConnected ();
    boolean isRecordAspectConnected ();
    boolean isAutoAxesEnableConnected ();
    boolean isAutoAxesCornersConnected ();
    boolean isAutoAxesCursorConnected ();
    boolean isAutoAxesFrameConnected ();
    boolean isAutoAxesGridConnected ();
    boolean isAutoAxesAdjustConnected ();
    boolean isAutoAxesAnnotationConnected ();
    boolean isAutoAxesLabelsConnected ();
    boolean isAutoAxesColorsConnected ();
    boolean isAutoAxesFontConnected ();
    boolean isAutoAxesTicksConnected ();
    boolean isAutoAxesXTickLocsConnected ();
    boolean isAutoAxesYTickLocsConnected ();
    boolean isAutoAxesZTickLocsConnected ();
    boolean isAutoAxesXTickLabelsConnected ();
    boolean isAutoAxesYTickLabelsConnected ();
    boolean isAutoAxesZTickLabelsConnected ();
    boolean isAutoAxesLabelScaleConnected ();
    boolean isRenderModeConnected ();
    boolean isButtonUpApproxConnected ();
    boolean isButtonDownApproxConnected ();
    boolean isButtonUpDensityConnected ();
    boolean isButtonDownDensityConnected ();

    void getAutoAxesCorners (double dval[]);
    void getAutoAxesCursor (double *x, double *y, double *z);
    int getAutoAxesAnnotation (char *sval[]);
    int getAutoAxesLabels (char *sval[]);
    int getAutoAxesColors (char *sval[]);
    int getAutoAxesFont (char *sval);
    void getAutoAxesTicks (int *t1, int *t2, int *t3);
    void getAutoAxesTicks (int *t);
    void getAutoAxesXTickLocs (double **t, int *size);
    void getAutoAxesYTickLocs (double **t, int *size);
    void getAutoAxesZTickLocs (double **t, int *size);
    int getAutoAxesXTickLabels (char *sval[]);
    int getAutoAxesYTickLabels (char *sval[]);
    int getAutoAxesZTickLabels (char *sval[]);
    int getAutoAxesEnable ();
    double getAutoAxesLabelScale ();
    int getAutoAxesTicksCount ();

    void setAutoAxesCorners (double dval[], boolean send);
    void setAutoAxesCursor (double x, double  y, double  z, boolean send);
    void setAutoAxesFrame (boolean state, boolean send);
    void setAutoAxesGrid (boolean state, boolean send);
    void setAutoAxesAdjust (boolean state, boolean send);
    void setAutoAxesEnable (int d, boolean send);
    void setAutoAxesLabelScale (double d, boolean send);
    void setAutoAxesAnnotation (char *value, boolean send);
    void setAutoAxesColors (char *value, boolean send);
    void setAutoAxesLabels (char *value, boolean send);
    void setAutoAxesFont (char *value, boolean send);
    void setAutoAxesTicks (int t1, int t2, int t3, boolean send);
    void setAutoAxesTicks (int t, boolean send);
    void setAutoAxesXTickLocs (double *t, int size, boolean send);
    void setAutoAxesYTickLocs (double *t, int size, boolean send);
    void setAutoAxesZTickLocs (double *t, int size, boolean send);
    void setAutoAxesXTickLabels (char *value, boolean send);
    void setAutoAxesYTickLabels (char *value, boolean send);
    void setAutoAxesZTickLabels (char *value, boolean send);

    void unsetAutoAxesCorners (boolean send);
    void unsetAutoAxesCursor (boolean send);
    void unsetAutoAxesTicks (boolean send);
    void unsetAutoAxesXTickLocs (boolean send);
    void unsetAutoAxesYTickLocs (boolean send);
    void unsetAutoAxesZTickLocs (boolean send);
    void unsetAutoAxesXTickLabels (boolean send);
    void unsetAutoAxesYTickLabels (boolean send);
    void unsetAutoAxesZTickLabels (boolean send);
    void unsetAutoAxesAnnotation (boolean send);
    void unsetAutoAxesLabels (boolean send);
    void unsetAutoAxesColors (boolean send);
    void unsetAutoAxesFont (boolean send);
    void unsetAutoAxesEnable (boolean send);
    void unsetAutoAxesFrame (boolean send);
    void unsetAutoAxesGrid (boolean send);
    void unsetAutoAxesAdjust (boolean send);
    void unsetAutoAxesLabelScale (boolean send);

    void setTranslateSpeed(double);
    void setRotateSpeed(double);
    double getTranslateSpeed();
    double getRotateSpeed();

    void setSoftware(boolean sw);
    void getSoftware(boolean &sw);
    void setApproximation(boolean up, ApproxMode mode);
    void getApproximation(boolean up, ApproxMode &mode);
    void setDensity(boolean up, int density);
    void getDensity(boolean up, int &density);

    boolean setInteractionMode(DirectInteractionMode mode);
    DirectInteractionMode getInteractionMode();

    void newCamera(int image_width, int image_height);
    void newCamera(double box[4][3], double aamat[4][4], double *from, double *to, 
	double *up, int image_width, int image_height, double width,
	boolean perspective, double viewAngle);
    void undoCamera();
    void redoCamera();
    void resetCamera();
    boolean setView(ViewDirection direction); 
    boolean setLook(LookDirection direction); 
    boolean setConstraint(ConstraintDirection direction); 
    ConstraintDirection getConstraint();

    boolean setBackgroundColor(const char *color); 
    boolean updateBGColorDialog(const char *color); 
    const char *getBackgroundColor();
    void  setDisplayGlobe();

    boolean enableRecord(boolean enable);
    boolean useRecord();
    boolean setRecordFile(const char *file);
    boolean setRecordFormat(const char *format);
    const char *getRecordFile();
    const char *getRecordFormat();
    void getRecordResolution(int &x, int &y);
    double getRecordAspect();

    //
    // What to do when we need to reset the window because the server has
    // forgotten about us.
    //
    virtual void resetWindow();
    //
    // getDisplayString() returns a static char *.  It goes away on the next
    // call.  DON'T FREE or DELETE IT!  It returns NULL on error.
    //
    char *getDisplayString();

    //
    // Change the frame buffer depth of the picture widget
    //
    void changeDepth(int depth);

    //
    // Post several of the controlling dialogs.
    //
    boolean postRenderingOptionsDialog();
    boolean postAutoAxesDialog();
    boolean postThrottleDialog();
    boolean postViewControlDialog();
    boolean postChangeImageNameDialog();
    boolean postBackgroundColorDialog();
    boolean postVPE();
    boolean postSaveImageDialog();
    boolean postPrintImageDialog();


    void sensitizePrintImageDialog(boolean flag);
    void sensitizeSaveImageDialog(boolean flag);
    void sensitizeThrottleDialog(boolean flag);

    //
    // Get commands.
    //
    CommandScope *getCommandScope() { return this->commandScope; }
    Command *getUndoCmd() { return this->undoCmd; }
    Command *getRedoCmd() { return this->redoCmd; }
    Command *getResetCmd() { return this->resetCmd; }
    Command *getModeNoneCmd() { return this->modeNoneCmd; }
    Command *getModeCameraCmd() { return this->modeCameraCmd; }
    Command *getModeCursorsCmd() { return this->modeCursorsCmd; }
    Command *getModePickCmd() { return this->modePickCmd; }
    Command *getModeNavigateCmd() { return this->modeNavigateCmd; }
    Command *getModePanZoomCmd() { return this->modePanZoomCmd; }
    Command *getModeRoamCmd() { return this->modeRoamCmd; }
    Command *getModeRotateCmd() { return this->modeRotateCmd; }
    Command *getModeZoomCmd() { return this->modeZoomCmd; }
    Command *getSetViewNoneCmd() { return this->setViewNoneCmd; }
    Command *getSetViewTopCmd() { return this->setViewTopCmd; }
    Command *getSetViewBottomCmd() { return this->setViewBottomCmd; }
    Command *getSetViewFrontCmd() { return this->setViewFrontCmd; }
    Command *getSetViewBackCmd() { return this->setViewBackCmd; }
    Command *getSetViewLeftCmd() { return this->setViewLeftCmd; }
    Command *getSetViewRightCmd() { return this->setViewRightCmd; }
    Command *getSetViewDiagonalCmd() { return this->setViewDiagonalCmd; }
    Command *getSetViewOffTopCmd() { return this->setViewOffTopCmd; }
    Command *getSetViewOffBottomCmd() { return this->setViewOffBottomCmd; }
    Command *getSetViewOffFrontCmd() { return this->setViewOffFrontCmd; }
    Command *getSetViewOffBackCmd() { return this->setViewOffBackCmd; }
    Command *getSetViewOffLeftCmd() { return this->setViewOffLeftCmd; }
    Command *getSetViewOffRightCmd() { return this->setViewOffRightCmd; }
    Command *getSetViewOffDiagonalCmd() { return this->setViewOffDiagonalCmd; }
    Command *getPerspectiveCmd() { return this->perspectiveCmd; }
    Command *getParallelCmd() { return this->parallelCmd; }
    Command *getConstrainNoneCmd() { return this->constrainNoneCmd; }
    Command *getConstrainXCmd() { return this->constrainXCmd; }
    Command *getConstrainYCmd() { return this->constrainYCmd; }
    Command *getConstrainZCmd() { return this->constrainZCmd; }
    Command *getLookForwardCmd() { return this->lookForwardCmd; }
    Command *getLookLeft45Cmd() { return this->lookLeft45Cmd; }
    Command *getLookRight45Cmd() { return this->lookRight45Cmd; }
    Command *getLookUp45Cmd() { return this->lookUp45Cmd; }
    Command *getLookDown45Cmd() { return this->lookDown45Cmd; }
    Command *getLookLeft90Cmd() { return this->lookLeft90Cmd; }
    Command *getLookRight90Cmd() { return this->lookRight90Cmd; }
    Command *getLookUp90Cmd() { return this->lookUp90Cmd; }
    Command *getLookDown90Cmd() { return this->lookDown90Cmd; }
    Command *getLookBackwardCmd() { return this->lookBackwardCmd; }
    Command *getLookAlignCmd() { return this->lookAlignCmd; }
    Command *getSoftwareCmd()    { return this->softwareCmd; }
    Command *getHardwareCmd()    { return this->hardwareCmd; }
    Command *getUpNoneCmd()    { return this->upNoneCmd; }
    Command *getUpWireframeCmd()    { return this->upWireframeCmd; }
    Command *getUpDotsCmd()    { return this->upDotsCmd; }
    Command *getUpBoxCmd()    { return this->upBoxCmd; }
    Command *getDownNoneCmd()    { return this->downNoneCmd; }
    Command *getDownWireframeCmd()    { return this->downWireframeCmd; }
    Command *getDownDotsCmd()    { return this->downDotsCmd; }
    Command *getDownBoxCmd()    { return this->downBoxCmd; }

    void setAutoAxesDialogTicks();
    void setAutoAxesDialogFrame();
    void setAutoAxesDialogGrid();
    void setAutoAxesDialogAdjust();
    void setAutoAxesDialogLabels();
    void setAutoAxesDialogLabelScale();
    void setAutoAxesDialogFont();
    void setAutoAxesDialogAnnotationColors();
    void setAutoAxesDialogCorners();
    void setAutoAxesDialogCursor();
    void setAutoAxesDialogEnable();
    void setAutoAxesDialogXTickLocs();
    void setAutoAxesDialogYTickLocs();
    void setAutoAxesDialogZTickLocs();
    void setAutoAxesDialogXTickLabels();
    void setAutoAxesDialogYTickLabels();
    void setAutoAxesDialogZTickLabels();

    void sensitizeViewControl(boolean flag);
    void sensitizeChangeImageName(boolean flag);
    void sensitizeRenderMode(boolean flag);
    void sensitizeButtonUpApprox(boolean flag);
    void sensitizeButtonDownApprox(boolean flag);
    void sensitizeButtonUpDensity(boolean flag);
    void sensitizeButtonDownDensity(boolean flag);


    //
    // Set the title of the window based on this windows associated node
    // and whether or not this window is the anchor or not.
    // The title will either be the assigned name, the name of the
    // associated node or the name of the network file.
    //
    void resetWindowTitle();

    //
    // Allow the DisplayNode to turn off execution on resizes due to
    // reading the 'window: pos=... size=...' comments. 
    //
    boolean isExecuteOnResize() { return this->state.resizeCausesExecution; }
    void setExecuteOnResize(boolean setting = TRUE) 
				{ this->state.resizeCausesExecution = setting; }
    void resetExecuteOnResizeWhenAble();
    boolean hasPendingWindowPlacement() { return (boolean)(this->reset_eor_wp != 0); }
 
    //
    // Before calling the super class method we verify that doing this will 
    // leave at least one startup image window.  If not, issue an error and
    // return without calling the super class method.
    //
    virtual void toggleWindowStartup();

    Network *getNetwork();

    //
    // Update the autoaxes dialog with whatever the current
    // values in the ImageNode are data-driven
    void updateAutoAxesDialog();
    void updateRenderingOptionsDialog();

    //
    // Update any displayed information from the new cfg state found in
    // the associated ImageNode. 
    //
    void updateFromNewCfgState();

    //
    // Allow the node to set the sensitivity of the background
    // color dialog box when the associated tab is attached/deattached
    // to an arc.
    void sensitizeBackgroundColorDialog(boolean flag);

    //
    // Determine whether the camera has been initializes
    boolean cameraIsInitialized() { return this->cameraInitialized;}

    //
    // Called by the MainWindow CloseCallback.  We call the super class
    // method and then, if we are not an anchor and are in DataViewer mode
    // then we exit the program.
    //
    virtual void closeWindow();

    //
    // On behalf of ImageFormatDialog (Save/Print Image dialogs) which needs to
    // know what strategy to use for saving the current image.
    //
    boolean hardwareMode() { return this->state.hardwareRender; }

    //
    // Make sure the changing the window title doesn't change the where
    // parameter (due to new window placement features.)
    //
    virtual void setWindowTitle(const char* name, boolean check_geometry=FALSE);


    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassImageWindow;
    }
};


#endif // _ImageWindow_h
