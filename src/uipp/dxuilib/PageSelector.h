/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


#ifndef _PageSelector_h
#define _PageSelector_h

#include "UIComponent.h"
#include "Dictionary.h"

#define ClassPageSelector "PageSelector"

extern "C" void PageSelector_CommitNameChangeCB(Widget, XtPointer, XtPointer);
extern "C" void PageSelector_ModifyNameCB(Widget, XtPointer, XtPointer);
extern "C" void PageSelector_SelectCB(Widget, XtPointer, XtPointer);
extern "C" void PageSelector_ResizeHandlerEH(Widget, XtPointer, XEvent*, Boolean*);
extern "C" void PageSelector_RemoveGrabEH(Widget, XtPointer, XEvent*, Boolean*);
extern "C" void PageSelector_EllipsisEH(Widget, XtPointer, XEvent*, Boolean*);
extern "C" void PageSelector_ProcessOldEventEH(Widget , XtPointer , XEvent *, Boolean *);
extern "C" void PageSelector_UndrawTO(XtPointer , XtIntervalId* );
extern "C" void PageSelector_RedrawTO(XtPointer , XtIntervalId* );
extern "C" void PageSelector_GrabReleaseTO(XtPointer , XtIntervalId* );
extern "C" void PageSelector_PopupListAH 
    (Widget , XtPointer , String, XEvent*, String*, Cardinal*);

extern "C" void PageSelector_UngrabAP (Widget, XEvent*, String*, Cardinal* );
extern "C" void PageSelector_KeepingAP (Widget, XEvent*, String*, Cardinal* );
extern "C" void PageSelector_LosingAP (Widget, XEvent*, String*, Cardinal* );
extern "C" void PageSelector_MonitoringAP (Widget, XEvent*, String*, Cardinal* );
extern "C" Boolean PageSelector_PostNamePromptWP (XtPointer clientData);
extern "C" Boolean PageSelector_RemoveHookWP (XtPointer clientData);



class EditorWindow;
class EditorWorkSpace;
class List;
class PageGroupRecord;
class PageGroupManager;
class PageTab;
class VPERoot;
class Network;
class SetPageNameDialog;
class MoveNodesDialog;
class Dialog;

class PageSelector: public UIComponent, public Dictionary {

    friend class SetPageNameDialog;

  private:
    EditorWindow*	editor;
    Network*		net;
    VPERoot*		root;
    List*		page_buttons;
    Widget		parent;
    void		buildSelector();
    void		buildPageMenu();
    boolean		selecting_page;
    int			num_pages_when_empty;
    XtIntervalId	button_release_timer;
    Widget		page_name_prompt;
    Widget		diag_button;
    Widget		popupMenu;
    Widget		popupList;
    boolean		is_grabbed;
    XtActionHookId	action_hook;
    int			starting_button;
    Widget		vsb, hsb;
    boolean		is_button_release_grabbed;
    XEvent*		old_event;
    boolean		mouse_inside_name_prompt;
    EditorWorkSpace*	name_change_in_progress;
    SetPageNameDialog*  page_dialog;
    MoveNodesDialog*    move_dialog;
    XtWorkProcId	remove_hook_wpid;

    static 	String		DefaultResources[];
    static	boolean		ClassInitialized;
    static	String		PnpTranslationText;
    static	XtTranslations	PnpTranslations;
    static	XtActionsRec	PnpActions[];


    //
    // add a button to button_list in the order specified in the .net file,
    // updating the PageGroupRecord if necessary.
    //
    void	appendButton (PageTab* button);

    //
    // The the special menu-style list under the ... button (diag_button)
    //
    void grab(XEvent*);
    void ungrab();

    void postPageNamePrompt();

    PageGroupRecord* getRecordOf(const char*);

  protected:

    virtual void resizeCallback();
    virtual void addButton (const char* name, const void* );
    virtual void removeButton (const char* name);

    friend  void PageSelector_ResizeHandlerEH(Widget, XtPointer, XEvent*, Boolean*);
    friend  void PageSelector_CommitNameChangeCB(Widget, XtPointer, XtPointer);
    friend  void PageSelector_ModifyNameCB(Widget, XtPointer, XtPointer);
    friend  void PageSelector_SelectCB(Widget, XtPointer, XtPointer);
    friend  void PageSelector_EllipsisEH(Widget, XtPointer, XEvent*, Boolean*);
    friend  void PageSelector_RemoveGrabEH(Widget, XtPointer, XEvent*, Boolean*);
    friend  void PageSelector_ProcessOldEventEH(Widget , XtPointer , XEvent *, Boolean *);
    friend  void PageSelector_GrabReleaseTO(XtPointer , XtIntervalId* );
    friend  void PageSelector_PopupListAH 
	(Widget , XtPointer , String, XEvent*, String*, Cardinal*);
    friend void PageSelector_UngrabAP (Widget, XEvent*, String*, Cardinal* );
    friend void PageSelector_KeepingAP (Widget, XEvent*, String*, Cardinal* );
    friend void PageSelector_LosingAP (Widget, XEvent*, String*, Cardinal* );
    friend void PageSelector_MonitoringAP (Widget, XEvent*, String*, Cardinal* );
    friend Boolean PageSelector_PostNamePromptWP (XtPointer clientData);
    friend Boolean PageSelector_RemoveHookWP (XtPointer clientData);

    void selectPage (Widget );
    void selectPage (PageTab* );

    virtual boolean verifyPageName(const char* , char* errMsg);
    virtual void    changePageName(EditorWorkSpace*, const char* );
    PageTab* getPageTabOf(const char* name);

    virtual void updateList();

    static Cursor GrabCursor;

    //
    // Get the text from a text widget and clip off the leading and
    // trailing white space.
    // The return string must be deleted by the caller.
    //
    static char *GetTextWidgetToken(Widget textWidget);

    void updatePageNameDialog();
    void updateMoveNodesDialog();

  public:

    PageSelector (EditorWindow* , Widget, Network*);

    ~PageSelector();

    void setRootPage(VPERoot* root) { this->root = root; }

    void postPageNamePrompt(PageTab* );
    void postPageNameDialog();
    void postMoveNodesDialog();
    void hidePageNamePrompt();
    void highlightTab (EditorWorkSpace*, int);
    void selectPage(EditorWorkSpace*);
    void togglePage (PageTab* );
    void updateDialogs();


    //
    // We won't intervene in dictionary operations in any way, we just use
    // these as a means of ensuring that the selector is always in sync with
    // the dictionary.
    //
    virtual boolean addDefinition(const char *name, const void *definition);
    virtual boolean addDefinition(Symbol key, const void *definition);
    virtual void    *removeDefinition(Symbol findkey);
    // looks like a c++ bug to me.  the const void* version supplants the const char*
    // version from the superclass.  Why?
    virtual void    *removeDefinition(const char* key) {
	return this->Dictionary::removeDefinition(key);
    }
    virtual void    *removeDefinition(const void* );

    void clear();

    int  getEmptySize() { return this->num_pages_when_empty; }


    //
    // Supports drag-n-drop in PageTab.
    //
    boolean changeOrdering (PageTab*, const char*, boolean dnd_operation=TRUE);

#if 0
    //
    // For use when dragging a page to the trash can.  Currently disconnected
    //
    void requestPageDeletion(const char*);
#endif

    EditorWindow* getEditor() { return this->editor; }

    //
    // Reorder the dictionary based on tab button position.
    //
    List* getSortedPages();

    //
    // the initial workspace should correspond to a PageTab whose GroupRecord
    // has a showing flag turned on.
    //
    EditorWorkSpace* getInitialWorkSpace();

    //
    // EditorWindow needs to know if the dialog is on the screen so that
    // it can do command activation properly.  If the dialog is up, then
    // EditorWindow will use its areSelectedNodesPagifiable() in setting
    // activation of its moveSelectedNodesCmd.
    //
    Dialog* getMoveNodesDialog();

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName() { return ClassPageSelector; }
};

#endif // _PageSelector_h
