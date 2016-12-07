/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"



#ifndef _ColormapEditor_h
#define _ColormapEditor_h

#include <X11/Intrinsic.h>
#include "DXWindow.h"


//
// Class name definition:
//
#define ClassColormapEditor	"ColormapEditor"

//
// XtCallbackProc (*CB), XtEventHandler (*EH) and XtActionProc (*AP)
// DialogCallback (*DCB), XtInputCallbackProc (*ICP), XtWorkProc (*WP)
// functions for this and derived classes
//
extern "C" void ColormapEditor_EditMenuMapCB(Widget, XtPointer, XtPointer);
extern "C" void ColormapEditor_ActiveCB(Widget, XtPointer, XtPointer);


//
// Referenced classes:
//
class Network;
class CommandScope;
class CommandInterface;
class Command;
class ColormapNode;
class ColormapAddCtlDialog;
class ColormapNBinsDialog;
class OpenColormapDialog;
class ColormapWaveDialog;
class SetColormapNameDialog;
class CascadeMenu;

//
// ColormapEditor class definition:
//				
class ColormapEditor : public DXWindow
{
  friend class ColormapEditCommand;
  friend class ColormapWaveDialog;
  friend class SetColormapNameDialog;

  private:
    //
    // Private class data:
    //
    static boolean ClassInitialized;
    static String  DefaultResources[];
    friend void ColormapEditor_ActiveCB(Widget, XtPointer, XtPointer);
    friend void ColormapEditor_EditMenuMapCB(Widget, XtPointer, XtPointer);
    void editMenuMapCallback();
    void activeCallback(XtPointer callData);


    ColormapNBinsDialog		*nBinsDialog;
    ColormapAddCtlDialog	*addCtlDialog;
    ColormapWaveDialog		*waveformDialog;
    OpenColormapDialog		*openColormapDialog;
    SetColormapNameDialog	*setNameDialog;

    int hue_selected;
    int sat_selected;
    int val_selected;
    int op_selected;
    int selected_area;

    boolean doingActivateCallback;
    boolean neverManaged;

    boolean constrain_vert;
    boolean constrain_hor;
    int background_style;
    int draw_mode;

    // Set the drawModeCmd based on whether we are data driven.
    void setDrawMode();

  protected:

    ColormapNode	*colormapNode;		// node associated.
    Widget		colormapEditor; 
    boolean		needResetOnManage;

    //
    // commands:
    //
    Command		*closeCmd;
    Command		*addCtlCmd;
    Command		*nBinsCmd;
    Command		*undoCmd;
    Command		*copyCmd;
    Command		*pasteCmd;
    Command		*waveformCmd;
    Command		*delSelectedCmd;
    Command		*selectAllCmd;
    Command		*setBkgdCmd;
    Command		*ticksCmd;
    Command		*histogramCmd;
    Command		*logHistogramCmd;
    Command		*consHCmd;
    Command		*consVCmd;
    Command		*newCmd;
    Command		*resetAllCmd;
    Command		*resetHSVCmd;
    Command		*resetOpacityCmd;
    Command		*resetMinMaxCmd;
    Command		*resetMinCmd;
    Command		*resetMaxCmd;
    Command		*openFileCmd;
    Command		*saveFileCmd;
    Command		*displayCPOffCmd;
    Command		*displayCPAllCmd;
    Command		*displayCPSelectedCmd;
    Command		*setColormapNameCmd;

    //
    // Menus & pulldowns:
    //
    Widget		fileMenu;
    Widget		editMenu;
    Widget		optionsMenu;

    Widget		fileMenuPulldown;
    Widget		editMenuPulldown;
    Widget		optionsMenuPulldown;
    Widget		displayCPButton;
    Widget		displayCPPulldown;
    Widget		drawModePulldown;

    Widget		drawModeButton;

    //
    // File menu options:
    //
    CommandInterface*	newOption;
    CommandInterface*	openOption;
    CommandInterface*	saveAsOption;
    CommandInterface*   closeOption;

    //
    // Edit menu options:
    //
    CascadeMenu		*resetMapCascade;
    CommandInterface*	addControlOption;
    CommandInterface*	undoOption;
    CommandInterface*	copyOption;
    CommandInterface*	pasteOption;
    CommandInterface*	waveformOption;
    CommandInterface*	deleteSelectedOption;
    CommandInterface*	selectAllOption;

    //
    // Options menu options:
    //
    CommandInterface*	nBinsOption;
    CommandInterface*	setBackgroundOption;
    CommandInterface*	ticksOption;
    CommandInterface*	histogramOption;
    CommandInterface*	logHistogramOption;
    CommandInterface*	horizontalOption;
    CommandInterface*	verticalOption;
    CommandInterface	*displayCPOffOption;
    CommandInterface	*displayCPAllOption;
    CommandInterface	*displayCPSelectedOption;
    CommandInterface	*setColormapNameOption;
    CommandInterface	*onVisualProgramOption;

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
    virtual void createEditMenu(Widget parent);
    virtual void createOptionsMenu(Widget parent);
    virtual void createHelpMenu(Widget parent);

    void    openNBinsDialog();
    void    openAddCtlDialog();
    void    openWaveformDialog();

    //
    // Edit the name of this Colormap editor.
    //
    void editColormapName();

    //
    // Set the widget sensitivity based on the whether or not the node thinks
    // the HSV, opacity and min and/or max are writeable.
    // This includes sensitizing the editMenu, min/max fields, and hsv and
    // opacity control point spaces.
    //
    void adjustSensitivities();



    //
    // Install the default resources for this class and then call the
    // same super class method to get the default resources from the
    // super classes.
    //
    virtual void installDefaultResources(Widget baseWidget);

    //
    // Allow subclasses to supply a string for the XmNgeometry resource
    // (i.e. -geam 90x60-0-0) which we'll use instead of createX,createY,
    // createWidth, createHeight when making the new window.  If the string
    // is available then initialize() won't call setGeometry.
    //
    virtual void getGeometryAlternateNames(String*, int*, int);

  public:


    //
    // Flags:
    //
    enum {
	   NEW,
	   UNDO,
	   COPY,
	   PASTE,
	   ADD_CONTROL,
	   N_BINS,
	   WAVEFORM,
	   DELETE_SELECTED,
	   SELECT_ALL,
	   SET_BACKGROUND,
	   TICKS,
	   HISTOGRAM,
	   LOGHISTOGRAM,
	   CONSTRAIN_H,
	   CONSTRAIN_V,
	   DISPLAY_CP_OFF,
	   DISPLAY_CP_ALL,
	   DISPLAY_CP_SELECTED
    };

    //
    // Constructor:
    //
    ColormapEditor(ColormapNode*    cmnode);

    //
    // Destructor:
    //
    ~ColormapEditor();


    //
    // Overrides the DXWindow class manage() function.
    //  Calls the superclass manage() first then adjusts the sensitivities
    //
    virtual void manage();

    void mapRaise();
    void postOpenColormapDialog(boolean opening);
    ColormapNode *getColormapNode() { return this->colormapNode; }
    Widget getEditor() { return this->colormapEditor; }

    //
    // Save and open a color map file.
    //
    boolean cmOpenFile(const char* filename);
    boolean cmSaveFile(const char* filename);

    //
    // Handle any state change that is associated with either the editor or
    // the Node.  Typically, called by a Colormap node when it receives
    // a message from the executive.
    //
    void handleStateChange();

    //
    // Sets the default resources before calling the superclass
    // initialize() function.
    //
    virtual void initialize();

    //
    // Same as the super class, but also sets the icon name.
    //
    virtual void setWindowTitle(const char *name, boolean check_geometry=FALSE);

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassColormapEditor;
    }
};


#endif // _ColormapEditor_h
