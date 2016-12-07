/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


#ifndef _TextSelector_h
#define _TextSelector_h

#include <X11/Intrinsic.h>

#include "UIComponent.h"
#include "List.h"

#define ClassTextSelector "TextSelector"

extern "C" void TextSelector_GrabReleaseTO(XtPointer , XtIntervalId* );
extern "C" void TextSelector_PopupListAH 
    (Widget , XtPointer , String, XEvent*, String*, Cardinal*);
extern "C" void TextSelector_EllipsisEH(Widget, XtPointer, XEvent*, Boolean*);
extern "C" void TextSelector_UngrabAP (Widget, XEvent*, String*, Cardinal* );
extern "C" void TextSelector_KeepingAP (Widget, XEvent*, String*, Cardinal* );
extern "C" void TextSelector_LosingAP (Widget, XEvent*, String*, Cardinal* );
extern "C" void TextSelector_MonitoringAP (Widget, XEvent*, String*, Cardinal* );
extern "C" Boolean TextSelector_RemoveHookWP (XtPointer clientData);
extern "C" void TextSelector_SelectCB(Widget, XtPointer, XtPointer);
extern "C" void TextSelector_ModifyCB(Widget, XtPointer, XtPointer);
extern "C" void TextSelector_ResolveCB(Widget, XtPointer, XtPointer);
extern "C" void TextSelector_ProcessOldEventEH(Widget , XtPointer , XEvent *, Boolean *);
extern "C" Boolean TextSelector_FillWP (XtPointer clientData);
extern "C" void TextSelector_RemoveGrabEH(Widget, XtPointer, XEvent*, Boolean*);

//
// Class TextSelector:
// Used in Moved Nodes to Page... dialog
// and SelectorPulldownInteractor
// Just a pulldown menu.
// 
class TextSelector: public UIComponent {

  private:

    Widget		parent;
    XtIntervalId	button_release_timer;
    Widget		diag_button;
    Widget		popupMenu;
    Widget		popupList;
    boolean		is_grabbed;
    XtActionHookId	action_hook;
    int			starting_button;
    Widget		vsb, hsb;
    Widget		textField;
    boolean		is_button_release_grabbed;
    XEvent*		old_event;
    XtWorkProcId	remove_hook_wpid;
    XtWorkProcId	auto_fill_wpid;

    static 	String		DefaultResources[];
    static	boolean		ClassInitialized;
    static	TextSelector*	GrabOwner;

    List	item_list;
    int*	selected_items;
    int		selected_cnt;


    //
    // The the special menu-style list under the ... button (diag_button)
    //
    void grab(XEvent*);
    void ungrab();

  protected:

    friend  void TextSelector_EllipsisEH(Widget, XtPointer, XEvent*, Boolean*);
    friend  void TextSelector_GrabReleaseTO(XtPointer , XtIntervalId* );
    friend  void TextSelector_PopupListAH 
	(Widget , XtPointer , String, XEvent*, String*, Cardinal*);
    friend void TextSelector_UngrabAP (Widget, XEvent*, String*, Cardinal* );
    friend void TextSelector_KeepingAP (Widget, XEvent*, String*, Cardinal* );
    friend void TextSelector_LosingAP (Widget, XEvent*, String*, Cardinal* );
    friend void TextSelector_MonitoringAP (Widget, XEvent*, String*, Cardinal* );
    friend void TextSelector_SelectCB(Widget, XtPointer, XtPointer);
    friend void TextSelector_ModifyCB(Widget, XtPointer, XtPointer);
    friend void TextSelector_ResolveCB(Widget, XtPointer, XtPointer);
    friend void TextSelector_RemoveGrabEH(Widget, XtPointer, XEvent*, Boolean*);
    friend void TextSelector_ProcessOldEventEH(Widget , XtPointer , XEvent *, Boolean *);
    friend Boolean TextSelector_RemoveHookWP (XtPointer clientData);
    friend Boolean TextSelector_FillWP (XtPointer clientData);

    virtual void updateList();

    static Cursor GrabCursor;

    //
    // Get the text from a text widget and clip off the leading and
    // trailing white space.
    // The return string must be deleted by the caller.
    //
    static char *GetTextWidgetToken(Widget textWidget);

    void enableModifyCB(boolean enab = TRUE);
    virtual boolean autoFill(boolean any_match=FALSE);
    virtual boolean autoFill(boolean any_match, boolean& unique_match);

    Widget getTextField() { return this->textField; }

    boolean case_sensitive;
    boolean equalString(const char*, const char*);
    boolean equalSubstring(const char*, const char*, int n);

  public:

    TextSelector ();

    ~TextSelector();

    void createTextSelector (Widget,XtCallbackProc,XtPointer,boolean use_button=TRUE);
    void setItems (char* items[], int nitems);
    void setSelectedItem (int );
    int  getSelectedItem(char* item = NUL(char*));

    //
    // Needed in order to implement WorkSpaceComponent::setAppearance.
    // Normally menu panes in the Motif can be accessed thru subMenuId in a
    // a CascadeButtonWidget, but not in this case.
    //
    void setMenuColors (Arg[], int);

    void setCaseSensitive(boolean cs=TRUE) { this->case_sensitive = cs; }

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName() { return ClassTextSelector; }
};

#endif

