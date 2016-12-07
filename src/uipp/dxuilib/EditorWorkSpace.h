/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#ifndef _EditorWorkSpace_h
#define _EditorWorkSpace_h

#include "WorkSpace.h"
#include "DXDropSite.h"


//
// Class name definition:
//
#define ClassEditorWorkSpace	"EditorWorkSpace"

#define ORG_NONE                0       /* uninitialized origin value   */
#define ORG_SOURCE              1       /* source origin value          */
#define ORG_DESTINATION         2       /* destination origin value     */

class NodeDefinition;
class Node;
class Ark;
class PageTab;
class WorkSpaceInfo;

typedef struct _TrackingIndex
{
    Node*                 node;           /* node index                   */
    int                   param;          /* parameter index              */

} TrackingIndex;


//
// EditorWorkSpace class definition:
//				
class EditorWorkSpace : public WorkSpace, public DXDropSite
{
  private:
    //
    // Private member data:
    //
    static Boolean EditorWorkSpaceClassInitialized;
    static String  DefaultResources[];

    // 1 per class... Holds the data types thid drop site can swallow.
    static Dictionary *DropTypeDictionary;

    // Define constants for the data types we can swallow in a dnd operation.  Use
    // these instead of func pointers.  They're passed to addSupportedType and
    // decoded in decodeDropType.
    enum {
	Modules,
	ToolName,
	File,
	Text
    };
    static void SetOwner(void*);
    static void DeleteSelections(void*);
    static void Select(void*);


    //
    // Initialized to FALSE.  Set to TRUE, when the page has been filled with 
    // standins/decorators.  Reset to FALSE if any node is added to the group
    // represented in this page.
    //
    boolean members_initialized;

    //
    // Initialized to TRUE.  The user can turn it off.  Then editor won't have
    // to open the page in order to produce postscript output.  The user's mechanism
    // for turning this off is thru the PageSelector dialog.
    //
    boolean included_in_ps;

  protected:

    friend class StandIn;
    friend class EditorWindow;
    friend class PageTab; // really only needs PageTab::mergeNetElements90

    //
    // Protected member data:
    //

    EditorWindow    *editor;	    // Editor this workspace belongs to 
    Widget          io_tab;         /* source tab widget            */

    TrackingIndex   src;            /* source tab node & param.     */
    TrackingIndex   dst;            /* dest.  tab node & param.     */

    int             origin;         /* originating location         */

    boolean         (*notify)();    /* network change notification  */
                                        /*  callback                    */
    XtPointer       client_data;    /* client data for the callback */

    XtEventHandler  tracker;        /* mouse tracking routine       */

    GC              gc_xor;         /* workspace XOR GC             */
    GC              gc;             /* workspace  GC                */
    Cursor          cursor[MAX_CURSORS];
                                    /* workspace cursors            */

    boolean         first;          /* first line drawing?          */

    int             start_x;        /* line drawing coordinates     */
    int             start_y;
    int             first_x;
    int             first_y;
    int             last_x;
    int             last_y;

    Widget          source_spot;    /* source spot widget           */
    Widget          hot_spot;       /* hot spot widget              */
    Node            *hot_spot_node; /* hot spot node index          */
    int             remove_arcs;
    Ark*             arc;

    XmFontList      font_list;      /* For the small font of parameter names */
    Widget	    labeled_tab;    /* Tab whose label is currently displayed */
    StandIn	    *labeled_si;    /* SI that is currently displaying	*/
				    /* a parameter label		*/

    //
    // Called when the workspace background gets a double click.
    //
    void doDefaultAction(Widget w, XtPointer callData);

    //
    // Called when the workspace background gets a single click.
    //
    void doBackgroundAction(Widget w, XtPointer callData);

    //
    // Drag and Drop related stuff
    // setRootWidget is not really dnd stuff.  It exists only because it
    // provides a place to invoke this->setDropWidget().  This could be
    // done elsewhere.
    //
    virtual void setRootWidget(Widget root, boolean standardDestroy = TRUE);
    boolean mergeNetElements (Network *tmpnet, List *tmppanels, int x, int y);
    boolean compoundTextTransfer (char *, XtPointer, unsigned long, int, int);
    boolean decodeDropType (int, char *, XtPointer, unsigned long, int, int);

    virtual Dictionary *getDropDictionary(){ return EditorWorkSpace::DropTypeDictionary; }

    //
    // Constructor:
    //
    EditorWorkSpace(const char *name, Widget parent, WorkSpaceInfo *info, 
			EditorWindow *editor);
 
    //
    // Destructor:
    //
    ~EditorWorkSpace(); 

    //
    // Record scrollbar positions so that they can be recorded/reset when 
    // switching pages.
    //
    int scrollx;
    int scrolly;
    boolean recorded_positions;
    boolean record_positions;

    //
    // Track changes in the positions of things in the canvas so
    // that we can provide an undo.  This is needed due to the new
    // function that lays out the graph automatically.  The function
    // might make a bad layout that the user would want to undo.
    //
    virtual void doPosChangeAction (Widget, XtPointer);

  public:

    //
    // Record scrollbar positions so that they can be recorded/reset when 
    // switching pages.
    //
    void setRecordedScrollPos (int x, int y) {
	this->recorded_positions = TRUE;
	this->scrollx = x;
	this->scrolly = y;
    }
    void getRecordedScrollPos (int *x, int *y) {
	*x = this->scrollx;
	*y = this->scrolly;
    }
    boolean hasScrollBarPositions() { 
	return (this->recorded_positions && this->record_positions); 
    }
    void unsetRecordedScrollPos () { this->recorded_positions = FALSE; }
    void useRecordedPositions(boolean use_them) {
	this->unsetRecordedScrollPos();
	this->record_positions = use_them;
    }
    boolean getUsesRecordedPositions() { return this->record_positions; }
    void restorePosition();

    //
    // One time class initializations. 
    //
    void initializeRootWidget(boolean fromFirstIntanceOfADerivedClass = FALSE);
 
    //
    // Get the bounding box of the children which are selected
    //
    virtual void getSelectedBoundingBox (int *minx, int *miny, int *maxx, int *maxy);

    boolean membersInitialized() { return this->members_initialized; }
    void setMembersInitialized(boolean init=TRUE) { this->members_initialized = init; }

    virtual void manage();
    virtual void unmanage();

    void setPostscriptInclusion(boolean incl=TRUE) { this->included_in_ps = incl; }
    boolean getPostscriptInclusion() { return this->included_in_ps; }

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassEditorWorkSpace;
    }

};


#endif // _EditorWorkSpace_h
