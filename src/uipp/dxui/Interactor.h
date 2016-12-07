/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"



// Interactor.h -							   
//                                                                        
// Definition for the Interactor class.					 
// 
// The Interactor represents the 'standin' that sits in the ControlPanel
// work space.  Each Interactor is tied to an InteractorNode (a stand in
// in the Editor workspace) through an InteractorInstance.  The
// InteractorInstance stores information local to the Interactor and does
// not make any assumptions about the windowing environment.  Each
// InteractorNode can have an unlimited number of
// Interactor/InteractorInstance pairs.
// 
// There are 3 main sets of methods associated with an Interactor.  The
// first has to do with constructing what is displayed in the control
// panel work space. All Interactors and derived classes have a basic
// layout  and must implement this->createInteractivePart(), which should
// build the part that takes input from the user.  This is then placed in
// the default layout using this->layoutInteractor().  Before
// layoutInteractor() returns this->completeInteractivePart() is called to
// finish any thing having to do with the interactor part.  Typically,
// this is just calling this->passEvents() so that most events get passed
// through interactor part.  The first two of the routines discussed so
// far are virtual and so can be redefined for derived classes.
// 
// The second set of methods (just 1 method here) is what to do when
// the Interactor in the control panel receives indication that the
// default action is to be taken.  this->openDefaultWindow implements
// this action.  It is a virtual function and can be redefined, but
// at this level is defined to open the set attributes dialog box.
// 
//
//

//  This is the widget layout of an Interactor.  These widgets are
//  created in Interactor.C.  Subclasses create children inside
//  this->customPart.  The Workspace widget handles installing translations
//  on form (this->root).  Interactor::createInteractor handles translations
//  for label and customPart.  passEvents() handles whatever extra widgets are
//  created by a subclass.
//
// --------f-o-r-m-------------------
// -                                -
// -                                -
// -   ----l-a-b-e-l------------    -
// -   -                       -    -
// -   -                       -    -
// -   -------------------------    -
// -                                -
// -   ---c-u-s-t-o-m-P-a-r-t---    -
// -   -                       -    -
// -   -                       -    -
// -   -                       -    -
// -   -                       -    -
// -   -                       -    -
// -   -                       -    -
// -   -------------------------    -
// -                                -
// -                                -
// ----------------------------------
//
// Functions for selecting and resizing are no longer here.  They live
// in WorkSpaceComponent instead.
//

#ifndef _Interactor_h
#define _Interactor_h

#include <X11/Intrinsic.h>
#include <Xm/Xm.h>
#include "DXDragSource.h"

#include "WorkSpaceComponent.h"

//
// Class name definition:
//
#define ClassInteractor	"Interactor"

typedef long InteractorStatusChange;

class InteractorNode;
class InteractorInstance;
class Node;
class Network;
class ControlPanel;
class Dialog;

extern void *GetUserData(Widget widget);

extern "C" Boolean Interactor_DragDropWP(XtPointer);

//
// Virtual Interactor class definition:
//				
class Interactor : public WorkSpaceComponent, public DXDragSource
{
    friend class InteractorStyle;	// It calls this->createInteractor(). 

  private:
    //
    // Private member data:
    // 
    // The frame is in this->root
    Widget		label;

    static boolean InteractorClassInitialized;
    static Widget DragIcon;

    // 1 dictionary for the class... Holds the type which can be dragged from
    // an interactor.
    static Dictionary *DragTypeDictionary;

    // 1 enum for each type of Drag data we can supply.  Pass these to addSupportedType
    // and decode them in decodeDragType.  These replace the use of func pointers.
    enum {
	Modules,
	Trash
    };

    //
    // Shift+Dragging an Interactor has the effect of deleting the
    // original interactor.  The new Motif doesn't permit deleting
    // an object involved in a drag-n-drop operation during the
    // operation.  We used to delete the interactor in the dropFinish()
    // method.  Now we create a work proc.  This way the deletion
    // will happen immediately following completion of the drag-n-drop.
    //
    int drag_drop_wpid;

    friend Boolean Interactor_DragDropWP(XtPointer);

  protected:
    //
    // Protected member data:
    //

    //
    // The interactor instance that this interactor is associated with
    //
    InteractorInstance		*interactorInstance;

    static String DefaultResources[]; 

    // 
    // To the parent's version add dnd participation.
    //
    virtual void passEvents(Widget w, boolean dnd);

    //
    // Build the complete interactor.  Make a form and a label and
    // invoke the subclasses ::createInteractivePart method.
    //
    void createInteractor();

    //
    // Create the actual interactive Widget(s).  The parent is this->customPart
    // and is guaranteed to be an XmForm. 
    //
    virtual Widget createInteractivePart(Widget p)=0;

    // 
    // Perform any actions that need to be done after the parent of 
    // this->interactivePart has been managed.  This may include a call 
    // to Interactor::passEvents().
    // 
    virtual void completeInteractivePart() = 0;


    //
    //  Layout/size the root, label and interactivePart widgets. 
    // 
    void layoutInteractor();
    virtual void layoutInteractorHorizontally();
    virtual void layoutInteractorVertically();
    virtual void layoutInteractorWithNoLabel();

    virtual const char *getComponentHelpTopic();

    //
    // Drag and Drop related stuff
    //
    // Create the .net and .cfg files and the header string.
    // return the names of the files.
    //
    virtual int decideToDrag (XEvent *);
    virtual void dropFinish (long, int, unsigned char);
    boolean createNetFiles (Network *, FILE *netf, char *cfgFile);
    virtual Dictionary *getDragDictionary() { return Interactor::DragTypeDictionary; }
    virtual boolean decodeDragType (int, char *, XtPointer *, unsigned long *, long );

    //
    // Constructor:
    //
    Interactor(const char * name, InteractorInstance *ii);


  public:
    //
    // Destructor:
    //
    ~Interactor(); 


    //
    // Set the displayed label of the interactor and do a layout if
    // indicated to handle a significant change in label width.
    //
    virtual void setLabel(const char *labelString, boolean re_layout = TRUE);

    //
    // The following are used to notify a control panel when an Interactor's
    // status changes.  If it is selected or unselected the control
    // panel needs to be notified.  Currently, notification is done
    // through ControlPanel::handleInteractorInstanceStatusChange().
    //
    enum {
                InteractorSelected   = 1, // Interactor was just selected.
                InteractorDeselected = 2 // Interactor was just unselected.
    };
    virtual void setSelected(boolean state);

    //
    // Default routines for determining the height and width of the 
    // this->interactivePart widget.
    //
    //virtual void getInteractivePartDimensions(Dimension *h, Dimension *w);

   
    //
    // Open the window for the default action (a double click) of this 
    // interactor;
    // 
    virtual void openDefaultWindow();

    //
    // Reset the resources of the widget based on a presumably new set 
    // of attributes.  The src_ii is the InteractorInstance that has
    // initialiated the attribute change.  Some attributes (local or
    // per interactor ones) may be ignored if the attribute change is 
    // initiated by a src_ii such that this->instance != src_ii. 
    // This should also include updating the displayed value.
    // If major_change is TRUE, then the subclass may choose to unmanage
    // itself while changes (presumably major) are made to it.
    // handleInteractorStateChange() remanages the interactor root widget
    // if it was unmanaged by handleInteractivePartStateChange().
    //
    virtual void handleInteractorStateChange(InteractorInstance *src_ii,
					boolean major_change);
    virtual void handleInteractivePartStateChange(
				InteractorInstance *src_ii,
			 	boolean major_change) = 0;

    //
    // Update the display values for an interactor; 
    // Called when an InteractorNode does a this->setOutputValue().
    // 
    virtual void updateDisplayedInteractorValue(void) = 0;

    //
    // Change the layout between horizontal and vertical.
    //
    void setVerticalLayout(boolean vertical = TRUE);
    void setBlankLabelLayout(boolean blank_label = TRUE);
    virtual boolean acceptsLayoutChanges() 
	{ return ((this->currentLayout & WorkSpaceComponent::BlankLabel) == 0); }

    //
    // Indicate that the interactor is selected.  Generally, this means 
    // highlighting.  This really only needs to be called when the
    // control panel is first opened up, after that the workspace takes
    // care of highlighting.  This can be called if the root widget
    // has not yet been created, but the selection may not be reflected.
    // 
    void indicateSelect(boolean selected);

    Network		*getNetwork();
    Node		*getNode();
    ControlPanel	*getControlPanel();
	
    //
    // One time initialize for the class. 
    // 
    virtual void initialize();

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassInteractor;
    }

    virtual boolean isA(Symbol classname);
};


#endif // _Interactor_h
