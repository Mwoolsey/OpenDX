/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"


 

#ifndef _WorkSpaceComponent_h
#define _WorkSpaceComponent_h


#include "UIComponent.h"
#include "SymbolManager.h"

//
// Class name definition:
//
#define ClassWorkSpaceComponent	"WorkSpaceComponent"

extern "C" void Component_SelectWorkSpaceComponentCB(Widget, XtPointer, XtPointer);
extern "C" void Component_ResizeCB(Widget, XtPointer, XtPointer);
extern "C" void Component_motionEH(Widget , XtPointer , XEvent *, Boolean *);
extern "C" void Component_buttonReleaseEH(Widget , XtPointer , XEvent *, Boolean *);
extern "C" void Component_buttonPressEH(Widget , XtPointer , XEvent *, Boolean *);

typedef int ComponentLayout;
class List;
class Network;
class WorkSpace;

//
// Virtual WorkSpaceComponent class definition:
// Used only in ControlPanels.
//
// Support objects which display on either a Workspace widget like the vpe
// or the ControlPanelWorkSpace.  Provide the callback for user selection
// of the component and for resizing.
//
// The example of widget creation provided by the Interactor class would be
// good one to follow, but that is not the case here.  It would be good
// to create the top level form widget here and then invoke a pure virtual
// function in the subclass to continue the process of widget creation.  Both 
// Interactor and Decorator classes work that way.  I didn't do that in this
// class because WorkSpaceComponent wants to provide support to StandIn also,
// but StandIn doesn't need the same widgets.  So rather than encapsulate
// the highest level of stuff which Interactor and Decorator have in common,
// I left them to fend for themselves in the hope that integrating WorkSpaceComponent
// and StandIn would be less painfull when the time comes.
//
class WorkSpaceComponent : public UIComponent
{

  private:
    //
    // Private member data:
    // 

    static boolean WorkSpaceComponentClassInitialized;

    //
    // developerStyle should really be an enum instead of a boolean.
    //
    boolean visualsInited;

    //
    // the user specified a size using ctrl-mouse.  ...gets reset
    // due to various ops - i.e. set layout
    //
    int userAssignedWidth;
    int userAssignedHeight;

    friend void Component_SelectWorkSpaceComponentCB(Widget, XtPointer, XtPointer);
    friend void Component_ResizeCB(Widget, XtPointer, XtPointer);

    //
    // Similar to a node's instance number.  I use this to uniquely
    // identify a decorator.  This number doesn't need to be saved in the .net
    // file, however because it isn't used in writing out executable code to the
    // exec.  It's only needed at runtime and doesn't have to be the same every
    // time the net is used.
    //
    long instance;

    static long NextInstanceNumber;

  protected:
    //
    // Protected member data:
    //
    static String DefaultResources[]; 
    boolean developerStyle;
    boolean selected;
    WorkSpace *workSpace;

    // Unset means that the value is known but the widgets haven't been laid out
    // It's using powers of 2 in order to AND
    enum {
	Vertical 	    = 1,
	Horizontal 	    = 2,
	NotSet 	    	    = 4,
	BlankLabel    	    = 8,
	VerticalUnset 	    = Vertical | NotSet,
	HorizontalUnset     = Horizontal | NotSet
    };
    ComponentLayout currentLayout;


    //
    // This is the widget created by the derived class that implements 
    // the interactive part of the interactor (i.e. stepper, dial...)
    // It is set to the return value createInteractivePart().
    // 
    Widget			customPart;

    // Make sure that the Workspace widget gets certain events.
    virtual void passEvents (Widget , boolean);

    //
    // Change '\' 'n' to an actual newline, etc.
    static char *FilterLabelString(const char *label);

    //
    // W I D T H   A N D   H E I G H T   I S S U E S
    // W I D T H   A N D   H E I G H T   I S S U E S
    // resizeCB is called in response to a user-initiated resize of 
    // a workspace child. By default do nothing and do not require subclasses.
    // to do anything either.
    virtual void resizeCB() {};
    void installResizeHandler();

    //
    // Flatten out the parent change the color, and turn off selectibility.
    //
    static void SetAllChildrenOf (Widget, Arg *, int);

    List *setResourceList;

    //
    // Constructor:
    //
    WorkSpaceComponent(const char * name, boolean developerStyle);

    //
    // In addition to superclass work, put null into root inthe objects in
    // this->setResourceList
    //
    virtual void widgetDestroyed();

    //
    // Set XmNuserData on the root widget so that we can track movement
    // in the workspace for the purpose of implementing undo
    //
    virtual void setRootWidget(Widget root, boolean standardDestroy = TRUE);

  public:
    //
    // W I D T H   A N D   H E I G H T   I S S U E S
    // W I D T H   A N D   H E I G H T   I S S U E S
    //
    static void GetDimensions(Widget w, int *height, int *width);
    virtual void setArgs (Arg *, int *) { } // for creation-only resources
    void getUserDimensions (int *w, int *h)
	{*w= this->userAssignedWidth; *h= this->userAssignedHeight; }
    void setUserDimensions (int w, int h) 
	{this->userAssignedWidth = w; this->userAssignedHeight = h; }
    void resetUserDimensions ()
	{this->userAssignedWidth = this->userAssignedHeight = 0; }
    virtual void getXYSize (int *width, int *height);
    boolean isUserSizeSet() 
	{ return (this->userAssignedHeight && this->userAssignedWidth); }


    //
    // V E R T I C A L / H O R I Z O N T A L   L A Y O U T
    // V E R T I C A L / H O R I Z O N T A L   L A Y O U T
    //
    virtual void setVerticalLayout (boolean vertical);
    boolean verticallyLaidOut();
    virtual boolean acceptsLayoutChanges() { return TRUE; }

    //
    // DynamicResource's
    // Needed are methods for fetching the setResourceList and setting resources.
    // They're not included because they wouldn't be used very much and so why
    // burden everybody.  The necessary routines are just stuck into LabelDecorator
    // which is the only class which needs them.
    //
    virtual boolean parseResourceComment (const char *,
	const char *, int ) { return FALSE; }
#   if MIGHT_NEED_THIS_LATER
    void *getResource (const char *);
#   endif
    const char *getResourceString (const char *);
    boolean isResourceSet (const char *);
    void transferResources (WorkSpaceComponent* );



    //
    // Flatten out the parent change the color, and turn off selectibility.
    //
    virtual void setAppearance(boolean developerStyle);
    virtual boolean getAppearance () { return this->developerStyle; }

    //
    // Open the window for the default action (a double click) of this 
    // interactor;
    // 
    virtual void openDefaultWindow()=0;


    //
    // Indicate that the interactor is selected.  Generally, this means 
    // highlighting.  This really only needs to be called when the
    // control panel is first opened up, after that the workspace takes
    // care of highlighting.  This can be called if the root widget
    // has not yet been created, but the selection may not be reflected.
    // 
    void indicateSelect(boolean selected);
    virtual void setSelected (boolean state);
    boolean isSelected() { return this->selected; }

    //
    // Provide the ability to set the contents of XmNlabelString in 
    // an XmLabel widget.  The setLabel() method is here only so that
    // ControlPanel doesn't have to call the isA method for every decorator.
    //
    static void SetLabelResource(Widget w, const char *labelString);
    virtual const char * getLabel() { return NULL; };
    virtual void setLabel (const char *, boolean re_layout=TRUE){};


    //
    // Used primarily inside ControlPanel.C
    //
    boolean isA (const char *classname);
    virtual boolean isA(Symbol classname);

    //
    // Interactor has one thru InteractorInstance.  Decorator gets one by
    // creating its own field to store its network.
    //
    virtual Network* getNetwork() = 0;
    WorkSpace *getWorkSpace() { return this->workSpace; }

    //
    // In addition to superclass work, also recreate an XmWorkspaceLine because
    // the start/end locations of the lines are fixed when they're created.
    // We need to do this on behalf of the new function that rearranges the
    // contents of the vpe based on connectivity.
    //
    virtual void setXYPosition (int x, int y);

    // Destructor:
    ~WorkSpaceComponent(); 

    //
    // Used during undo operations, so that decorators can be identified
    // uniquely without the use of a UIComponent*
    //
    long getInstanceNumber() { return this->instance; }

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassWorkSpaceComponent;
    }
};


#endif // _WorkSpaceComponent_h
