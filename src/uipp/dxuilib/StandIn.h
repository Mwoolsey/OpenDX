/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"


#ifndef _StandIn_h
#define _StandIn_h


#include "UIComponent.h"
#include "List.h"
#include "DXDragSource.h"
#include "Tab.h"


//
// Class name definition:
//
#define ClassStandIn	"StandIn"

//
// XtCallbackProc (*CB), XtEventHandler (*EH) and XtActionProc (*AP)
// DialogCallback (*DCB), XtInputCallbackProc (*ICP), XtWorkProc (*WP)
// functions for this and derived classes
//
extern "C" void StandIn_TrackArkEH(Widget, XtPointer, XEvent*, Boolean*);
extern "C" void StandIn_Button2PressEH(Widget, XtPointer, XEvent*, Boolean*);
extern "C" void StandIn_Button2ReleaseEH(Widget, XtPointer, XEvent*, Boolean*);
extern "C" void StandIn_TabKeyNavEH(Widget, XtPointer, XEvent*, Boolean*);
extern "C" void StandIn_DisarmTabCB(Widget, XtPointer, XtPointer);
extern "C" void StandIn_ArmOutputCB(Widget, XtPointer, XtPointer);
extern "C" void StandIn_ArmInputCB(Widget, XtPointer, XtPointer);
extern "C" void StandIn_SelectNodeCB(Widget, XtPointer, XtPointer);
extern "C" Boolean StandIn_DragDropWP(XtPointer);

class Tab;
class Ark;
class Node;
class EditorWindow;
class WorkSpace;

typedef long NodeParameterStatusChange;	// Also defined in Node.h

//
// StandIn class definition:
//				
class StandIn : public UIComponent, public DXDragSource
{

//friend class Node;
//friend class Network;


  private:
   
    static Boolean ClassInitialized;
    static Widget DragIcon;
    static Dictionary *DragTypeDictionary;

    int selected;
    WorkSpace *workSpace;

    List  inputTabList;
    List  outputTabList;

   
    /*
     * module label (button):
     */
    Widget 	buttonWidget;
    Dimension	buttonHeight;
    Dimension   requiredButtonWidth;


    friend void StandIn_SelectNodeCB(Widget widget,
                XtPointer clientData,
                XtPointer cdata);

    friend void StandIn_ArmInputCB(Widget widget,
                XtPointer clientData,
                XtPointer cdata);
    void armInput(Widget widget, XtPointer cdata);

    friend void StandIn_ArmOutputCB(Widget widget,
                XtPointer clientData,
                XtPointer cdata);
    void armOutput(Widget widget, XtPointer cdata);


    friend void StandIn_DisarmTabCB(Widget widget,
                XtPointer clientData,
                XtPointer cdata);
    void disarmTab(Widget widget, XtPointer cdata);


    friend void StandIn_Button2PressEH(Widget widget,
                XtPointer ,
                XEvent* event,
		Boolean *keep_going);
    friend void StandIn_Button2ReleaseEH(Widget widget,
                XtPointer ,
                XEvent* event,
		Boolean *keep_going);

    friend void StandIn_TrackArkEH(Widget widget,
                XtPointer clientData,
                XEvent* event,
		Boolean *cont);
    friend void StandIn_TabKeyNavEH(Widget widget,
                XtPointer ,
                XEvent* event,
		Boolean *keep_going);
    friend Boolean StandIn_DragDropWP(XtPointer);

    void trackArk(Widget widget, XEvent *event);

    boolean  setMinimumWidth(int &width);
    int  getVisibleInputCount();
    int  getVisibleOutputCount();
    void adjustParameterLocations(int width);

    Tab *createInputTab(Widget parent, int index, int width);
    Tab *createOutputTab(Widget parent, int index, int width);

    void indicateSelection(boolean select);

    void setVisibility(int index, boolean outputTab);
    XmString   createButtonLabel();

    //
    // Supply the type dictionary which stores the types we can supply for a dnd 
    //
    virtual Dictionary *getDragDictionary() { return StandIn::DragTypeDictionary; }

    // Define the data type for the drag operation.  These replace the use of
    // function pointers.  They're passed to addSupportedType and decoded in
    // decodeDragType
    enum {
	Modules,
	Interactors,
	Trash
    };

    //
    // Dragging a StandIn onto the tool selector means deleting the
    // standin.  The new Motif doesn't permit deleting an object involved
    // in a drag drop during the operation.  We used delete it in the
    // dropFinishCallback.  Now we create a work proc so that the Nodes
    // will be deleted right away but not until the Drag-n-Drop is over
    //
    int drag_drop_wpid;


    //
    // In order to allow keyboard events to percolate up to the workspace,
    // I'm breaking out low level window munging code from :createStandIn()
    // into a separate method because the code needs to be applied to the
    // io tabs as well.
    //
    static void ModifyButtonWindowAttributes(Widget button, int mask);

  protected:
    //
    // Protected member data:
    //
    static String  DefaultResources[];

    Node* node;
    virtual void  initialize();

    void       setButtonLabel();
    virtual    const char *getButtonLabel();
    virtual    void handleSelectionChange(boolean select);

    //
    // Drag and Drop related stuff
    //
    virtual int decideToDrag(XEvent *);
    virtual void dropFinish(long, int, unsigned char);
    virtual boolean decodeDragType (int, char *, XtPointer*, unsigned long*, long);

    virtual const char *getButtonLabelFont();

    //
    // Constructor:
    //
    StandIn(WorkSpace *w, Node *node);

    //
    // Set XmNuserData on the root widget so that we can track movement
    // in the workspace for the purpose of implementing undo
    //
    virtual void setRootWidget(Widget root, boolean standardDestroy = TRUE);

  public:


    //
    // Allocator that is installed in theSIAllocatorDictionary
    //
    static StandIn *AllocateStandIn(WorkSpace *w, Node *n);

    //
    // Destructor:
    //
    ~StandIn();

    void createStandIn();
    void addInput(int index);
    void removeLastInput();

    void addOutput(int index);
    void removeLastOutput();

    virtual void setLabelColor(Pixel color);
    virtual Pixel getLabelColor();

    void deleteArk(Ark *a);
    void drawArks(Node *dst_node);
    void drawTab(int paramIndex, boolean outputTab);

    void ioStatusChange(int index, boolean outputTab, 
					NodeParameterStatusChange status);

    static void ToggleHotSpots(EditorWindow* editor,
                               Node*    destnode,
                               boolean     on);

    void displayTabLabel(int index, boolean outputTab);
    void clearTabLabel();
    Tab *getInputParameterTab(int i);
    Tab *getOutputParameterTab(int i); 

    Tab *getParameterTab(int i, int input) 
        { return (input ? getInputParameterTab(i):
                          getOutputParameterTab(i)); }

    Widget getInputParameterLine(int i);
    Widget getOutputParameterLine(int i); 
    Widget getParameterLine(int i, int input) 
        { return (input ? getInputParameterLine(i):
                          getOutputParameterLine(i)); }

    void setInputParameterLine(int i, Widget w);
    void setOutputParameterLine(int i, Widget w);
    void setParameterLine(int i, Widget w, int input) 
        { input ? setInputParameterLine(i,w) :
                  setOutputParameterLine(i,w); }

    int getIOWidth();
    int getIOHeight();

    int getInputParameterTabX(int i);
    int getInputParameterTabY(int i);

    int getOutputParameterTabX(int i);
    int getOutputParameterTabY(int i);

    int getParameterTabY(int i,int input) 
        { return (input ? getInputParameterTabY(i):getOutputParameterTabY(i));}

    int getParameterTabX(int i,int input) 
        { return (input ? getInputParameterTabX(i):getOutputParameterTabX(i));}



    void setSelected(boolean s); 
    int isSelected()        { return this->selected; }

    void addArk(EditorWindow* editor, Ark *a);

    //
    // Print a representation of the stand in on a PostScript device.  We
    // assume that the device is in the same coordinate space as the parent
    // of this uicomponent (i.e. we send the current geometry to the 
    // PostScript output file).  We do not print the ArkStandIns, but do
    // print the Tabs.
    //
    boolean printPostScriptPage(FILE *f, boolean label_parameters = TRUE);
    static boolean PrintPostScriptSetup(FILE *f);
    virtual const char *getPostScriptLabelFont();


    //
    // This function can be called to notify a standin that its label
    // has changed.  By default, this standin just calls setButtonLabel() 
    // to reset the label. 
    virtual void notifyLabelChange();

    //
    // Now that StandIns can live in any WorkSpace, we need to query this.
    //
    WorkSpace *getWorkSpace() { return this->workSpace; }

    //
    // In addition to superclass work, also recreate an XmWorkspaceLine because
    // the start/end locations of the lines are fixed when they're created.
    // We need to do this on behalf of the new function that rearranges the
    // contents of the vpe based on connectivity.
    //
    virtual void setXYPosition (int x, int y);

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassStandIn;
    }
};

extern Pixel standInGetDefaultForeground();

#endif // _StandIn_h
