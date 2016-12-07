/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#ifndef _WorkSpace_h
#define _WorkSpace_h

#include <Xm/Xm.h>

#include "UIComponent.h"


//
// Class name definition:
//
#define ClassWorkSpace	"WorkSpace"

class NodeDefinition;
class WorkSpaceGrid;
class WorkSpaceInfo;
class List;


//
// XtCallbackProc (*CB), XtEventHandler (*EH) and XtActionProc (*AP)
// DialogCallback (*DCB) functions for this and derived classes
//
extern "C" void WorkSpace_PosChangeCB(Widget, XtPointer, XtPointer);
extern "C" void WorkSpace_BackgroundCB(Widget, XtPointer, XtPointer);
extern "C" void WorkSpace_ResizeHandlerEH(Widget, XtPointer, XEvent*, Boolean*);
extern "C" void WorkSpace_DefaultActionCB(Widget, XtPointer, XtPointer);
extern "C" void WorkSpace_SelectionCB(Widget, XtPointer, XtPointer);

#define UP_CURSOR               0
#define DOWN_CURSOR             1
#define MAX_CURSORS             2

#define UPPER_LEFT 0
#define FLEUR      1
#define WATCH      2
#define LEFT_PTR   3


//
// WorkSpace class definition:
//				
class WorkSpace : public UIComponent
{
  private:
    //
    // Private member data:
    //
    friend void WorkSpace_DefaultActionCB(Widget w, XtPointer clientData, 
							XtPointer callData);
    friend void WorkSpace_BackgroundCB(Widget w, XtPointer clientData, 
							XtPointer callData);
    friend void WorkSpace_SelectionCB(Widget w, XtPointer clientData, 
							XtPointer callData);
    friend void WorkSpace_PosChangeCB(Widget w, XtPointer clientData, 
							XtPointer callData);

    // Handle the resizing event of its parent.
    friend void WorkSpace_ResizeHandlerEH(Widget, XtPointer, XEvent*, Boolean*);

    // Saved in constructor for use during initializeRootWidget().
    Widget   parent;	

    boolean wasLineDrawingEnabled;
    boolean wasAllowOverlap;
    boolean wasManhattanEnabled;
    boolean activeManyPlacements;

  protected:

    //
    // Protected member data:
    //

    //
    // Default resources are made available to the derived classes so
    // that they can install them.  They are not installed by this class.
    //
    static String  DefaultResources[];

    Display*        	display;        /* workspace display            */
    WorkSpaceInfo	*info;

    //
    // Called when the workspace background gets a double click.
    //
    virtual void doDefaultAction(Widget w, XtPointer callData) = 0;

    //
    // Called when the workspace background gets a single click.
    //
    virtual void doBackgroundAction(Widget w, XtPointer callData) = 0;
    virtual void doSelectionAction(Widget w, XtPointer callData) {};

    virtual void doPosChangeAction(Widget /* w */, XtPointer /* callData */) {};

    //
    // Constructor (for derived classes only):
    //
    WorkSpace(const char *name, Widget parent, WorkSpaceInfo *info);

    virtual boolean isRoot() { return TRUE; }

    //
    // Record lineDrawing, overlap, and manhattan values so that they can
    // be toggled later on, on behalf of pages.  A subclass may want to
    // fetch values from the parent WorkSpace instead of relying on Xdefaults.
    // That way we don't require to make 2 Xdefaults settings - 1 for parent,
    // 1 for page.  Instead the user can make use the same Xdefault setting
    // as was used in 3.1.4 and earlier and have it work globally.
    //
    virtual void saveWorkSpaceParams(WorkSpace*);

  public:
    //
    // Destructor:
    //
    ~WorkSpace(); 
  
    //
    // One time class initializations 
    //
    virtual void initializeRootWidget(boolean loadDefaultResources = FALSE);

    virtual void setCursor(int cursorType);
    virtual void resetCursor();

    //
    // Optimize the workspace for placement of many children 
    //
    virtual void beginManyPlacements();
    virtual void   endManyPlacements();

    //
    // manage the widget tree and call initializeRootWidget() if widget tree 
    // has not been created yet.
    //
    virtual void manage();

    WorkSpaceInfo *getInfo() { return this->info; }; 
    void setWidth(int);
    void setHeight(int);
    void setWidthHeight(int, int);

    virtual void resize();

    //
    // Install the new grid in the WorkSpace
    //
    virtual void installInfo(WorkSpaceInfo *);

    //
    // Get the maximun x/y of any of the children of the widget
    //
    void getMaxWidthHeight(int *, int *);

    //
    // Get the bounding box of the children which are selected
    //
    virtual void getSelectedBoundingBox (int *minx, int *miny, int *maxx, int *maxy) = 0;

    //
    // Used by undo operations to determine if an undone move will clobber
    // other objects.
    //
    virtual boolean isEmpty (int x, int y, int width, int height);

    //
    // On behalf of Page/Root
    //
    int getPlacementCount() { return this->activeManyPlacements; }
    void setPlacementCount(int count);
    boolean getLineDrawing() { return this->wasLineDrawingEnabled; }
    boolean getOverlap() { return this->wasAllowOverlap; }
    boolean getManhattan() { return this->wasManhattanEnabled; }

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassWorkSpace;
    }
};


#endif // _WorkSpace_h
