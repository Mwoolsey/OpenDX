/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"


#ifndef _AutoAxesDialog_h
#define _AutoAxesDialog_h


#include "Dialog.h"

//
// Class name definition:
//
#define ClassAutoAxesDialog	"AutoAxesDialog"

//
// XtCallbackProc (*CB), XtEventHandler (*EH) and XtActionProc (*AP)
// DialogCallback (*DCB), XtInputCallbackProc (*ICP), XtWorkProc (*WP)
// functions for this and derived classes
//
extern "C" void AutoAxesDialog_BgOMCB(Widget, XtPointer, XtPointer);
extern "C" void AutoAxesDialog_BgColorCB(Widget, XtPointer, XtPointer);
extern "C" void AutoAxesDialog_CursorCB(Widget, XtPointer, XtPointer);
extern "C" void AutoAxesDialog_CornersCB(Widget, XtPointer, XtPointer);
extern "C" void AutoAxesDialog_TicksCB(Widget, XtPointer, XtPointer);
extern "C" void AutoAxesDialog_TicksValueCB(Widget, XtPointer, XtPointer);
extern "C" void AutoAxesDialog_PushbuttonCB(Widget, XtPointer, XtPointer);
extern "C" void AutoAxesDialog_DirtyCB(Widget, XtPointer, XtPointer);
extern "C" void AutoAxesDialog_DirtyTextCB(Widget, XtPointer, XtPointer);
extern "C" void AutoAxesDialog_NoteBookCB(Widget, XtPointer, XtPointer);
extern "C" void AutoAxesDialog_ListToggleCB(Widget, XtPointer, XtPointer);
extern "C" void AutoAxesDialog_TickLabelToggleCB(Widget, XtPointer, XtPointer);

#define AA_PARAMS 18  // total number of paramters

class ImageWindow;
class TextPopup;
class TickLabelList;

#define MAIN_FORM	0
#define LABEL_INPUTS    1
#define FLAG_INPUTS     2
#define COLOR_INPUTS    3
#define CORNER_INPUTS   4
#define TICK_INPUTS     5
#define TICK_LABELS	6
#define BUTTON_FORM 	7

//
// AutoAxesDialog class definition:
//				

class AutoAxesDialog : public Dialog
{
  private:
    //
    // Private member data:
    //
    static boolean ClassInitialized;
    static String  DefaultResources[];
    static int MinWidths[];

    static void FontChanged(TextPopup *tp, const char *font, void *callData);
    static void BGroundChanged(TextPopup *tp, const char *bgcolor, void *callData);

    char     *saved_bg_color;

    Widget toplevelform;
    Widget subForms[BUTTON_FORM+1];
    Widget notebookToggles[BUTTON_FORM+1];

    //  M A I N F O R M
    Widget enable_tb;

    // C O L O R S F O R M 
    Widget background_om;
    Widget background_color;
    TextPopup *bgColor;
    Widget grid_color;
    Widget tick_color;
    Widget label_color;

    // T I C K S F O R M 
    Widget ticks_om;
    Widget ticks_label[3];
    Widget ticks_number[3];
    Widget ticks_direction[3];
    Widget axes_label[3];
    Widget ticks_tb;
    static Pixmap TicksIn;
    static Pixmap TicksOut;
    static Pixmap TicksInGrey;
    static Pixmap TicksOutGrey;

    // F L A G S F O R M 
    TextPopup  *fontSelection;
    Widget label_scale_stepper;
    Widget frame_om;
    Widget grid_om;
    Widget adjust_om;
    Widget font_popup_menu;

    // L A B E L S F O R M 

    // T I C K L A B E L S F O R M 
    TickLabelList *tickList[3];

    // C O R N E R S F O R M 
    Widget corners_tb;
    Widget corners_number[6];
    Widget cursor_tb;
    Widget cursor_number[3];

    // B U T T O N S F O R M
    Widget okbtn, applybtn, restorebtn, cancelbtn, expandbtn;

    ImageWindow* image;

    Widget createTicksPulldown		(Widget parent);
    Widget createTicksDirectionPulldown	(Widget parent);
    Widget createFramePulldown		(Widget parent);
    Widget createGridPulldown		(Widget parent);
    Widget createBackgroundPulldown	(Widget parent);
    Widget createAdjustPulldown		(Widget parent);

    // L A Y O U T   M E T H O D S
    void layoutTicks(Widget parent);
    void layoutCorners(Widget parent);
    void layoutColors(Widget parent);
    void layoutFlags(Widget parent);
    void layoutLabels(Widget parent);
    void layoutTickLabels(Widget parent);
    void layoutButtons(Widget parent);
 
    int    dirtyBits;
    boolean isDirty();
    boolean isDirty(int member);
    boolean deferNotify;

    static void TLLModifyCB (TickLabelList *, void*);

  protected:
    //
    // Protected member data:
    //
    Widget createDialog(Widget);

    friend void AutoAxesDialog_PushbuttonCB(Widget, XtPointer , XtPointer);
    friend void AutoAxesDialog_TicksCB(Widget, XtPointer , XtPointer);
    friend void AutoAxesDialog_TicksValueCB(Widget, XtPointer , XtPointer);
    friend void AutoAxesDialog_CornersCB(Widget, XtPointer , XtPointer);
    friend void AutoAxesDialog_CursorCB(Widget, XtPointer , XtPointer);
    friend void AutoAxesDialog_BgOMCB(Widget, XtPointer , XtPointer);
    friend void AutoAxesDialog_BgColorCB(Widget, XtPointer , XtPointer);
    friend void AutoAxesDialog_DirtyCB(Widget, XtPointer , XtPointer);
    friend void AutoAxesDialog_DirtyTextCB(Widget, XtPointer , XtPointer);
    friend void AutoAxesDialog_NoteBookCB(Widget, XtPointer , XtPointer);
    friend void AutoAxesDialog_ListToggleCB(Widget, XtPointer, XtPointer);
    friend void AutoAxesDialog_TickLabelToggleCB(Widget, XtPointer, XtPointer);

    void   installValues();
    void   flipAllTabsUp(boolean send);

    //
    // W I D G E T     S E N S I T I V I T Y
    // W I D G E T     S E N S I T I V I T Y
    //
    void setColorsSensitivity (boolean clear);
    void setCornersSensitivity (boolean button_set);
    void setCursorSensitivity (boolean button_set);
    void setTicksSensitivity (boolean button_set);
    void setLabelsSensitivity ();
    void setGridSensitivity ();
    void setAdjustSensitivity ();
    void setFrameSensitivity ();
    void setFontSensitivity ();
    void setLabelScaleSensitivity ();
    void setEnableSensitivity ();

    //
    // Install the default resources for this class and then call the
    // same super class method to get the default resources from the
    // super classes.
    //
    virtual void installDefaultResources(Widget baseWidget);

    // This is a workaround for an MWM bug on aix 312.  You can't have minWidth
    // set for an unmanaged dialog box.  If you do, then you'll never be able
    // to unset minWidth.  FIXME: make this a method in Dialog?
    void setMinWidth(boolean );

  public:

    //
    // Constructor:
    //
    AutoAxesDialog(ImageWindow*);

    //
    // Destructor:
    //
    ~AutoAxesDialog();

    virtual void manage();
    virtual void unmanage();

    void update();

    void setAutoAxesDialogTicks();
    void setAutoAxesDialogFrame();
    void setAutoAxesDialogGrid();
    void setAutoAxesDialogAdjust();
    void setAutoAxesDialogLabels();
    void setAutoAxesDialogLabelScale();
    void setAutoAxesDialogFont();
    void setAutoAxesDialogAnnotationColors();
    void setAutoAxesDialogColors();
    void setAutoAxesDialogCorners();
    void setAutoAxesDialogCursor();
    void setAutoAxesDialogEnable();
    void setAutoAxesDialogXTickLocs();
    void setAutoAxesDialogYTickLocs();
    void setAutoAxesDialogZTickLocs();
    void setAutoAxesDialogXTickLabels();
    void setAutoAxesDialogYTickLabels();
    void setAutoAxesDialogZTickLabels();
    
    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassAutoAxesDialog;
    }
};


#endif // _AutoAxesDialog_h
