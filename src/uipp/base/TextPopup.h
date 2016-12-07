/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"


#ifndef _TextPopup_h
#define _TextPopup_h


#include "UIComponent.h"


//
// TextPopupCallback type definition:
//
class TextPopup;
typedef void (*SetTextCallback)(TextPopup *tp, const char*, void*);
typedef void (*ChangeTextCallback)(TextPopup *tp, const char*, void*);
//
// Class name definition:
//
#define ClassTextPopup	"TextPopup"

//
// XtCallbackProc (*CB), XtEventHandler (*EH) and XtActionProc (*AP)
// TextPopupCallback (*DCB) functions for this and derived classes
//
extern "C" void TextPopup_ItemSelectCB(Widget widget,
			   XtPointer clientData,
			   XtPointer);
extern "C" void TextPopup_ActivateTextCB(Widget widget,
			   XtPointer clientData,
			   XtPointer);
extern "C" void TextPopup_ValueChangedTextCB(Widget widget,
			   XtPointer clientData,
			   XtPointer);
extern "C" void TextPopup_ButtonEH(Widget widget,
			   XtPointer clientData,
			   XEvent *e, Boolean *ctd);
//
// TextPopup class definition:
// Contains the TextField followed by a popup button for
// attaching a menu. Only used in AutoAxes Configuration
// for the Font selection.
//				
class TextPopup : public UIComponent
{
  private:
    //
    // Private member data:
    //
    static boolean ClassInitialized;

    friend void TextPopup_ItemSelectCB(Widget widget,
			   XtPointer clientData,
			   XtPointer);
    friend void TextPopup_ActivateTextCB(Widget widget,
			   XtPointer clientData,
			   XtPointer);
    friend void TextPopup_ValueChangedTextCB(Widget widget,
			   XtPointer clientData,
			   XtPointer);
    friend void TextPopup_ButtonEH(Widget widget,
			   XtPointer clientData,
			   XEvent *e, Boolean *ctd);

    SetTextCallback    	setTextCallback;
    ChangeTextCallback 	changeTextCallback;
    void		*callbackData;

    boolean             callbacksEnabled;

    void initInstanceData();

  protected:
    //
    // Protected member data:
    //

    static String DefaultResources[];
    Widget textField;
    Widget popupButton;
    Widget popupMenu;

    //
    // One time class initializations.
    //
    virtual void initialize();

    //
    // Called when an item in the pop up list is selected. 
    // This method installs the text in the text window and calls the
    // activateCallback().
    //
    virtual void itemSelectCallback(const char *value);
    //
    // Called when the user hits a return in the text window.
    //
    virtual void activateTextCallback(const char *value, void *callData);
    //
    // Called when the characters in the text window changes
    //
    virtual void valueChangedTextCallback(const char *value, void *callData);

    //
    // Constructor:
    //
    TextPopup(const char *name);

  public:
    //
    // Constructor:
    //
    TextPopup();

    //
    // Destructor:
    //
    ~TextPopup();

    virtual Widget createTextPopup(Widget parent, 
				const char **items, int n,
				SetTextCallback stc = NULL,
				ChangeTextCallback ctc = NULL,
				void *callbackData = NULL);

    //
    // De/Install callbacks for the text widget. 
    //
    boolean  enableCallbacks(boolean enable = TRUE);
    boolean  disableCallbacks() { return this->enableCallbacks(FALSE); }

    //
    // Get the current text in the text box.  The return string must
    // be freed by the caller.
    //
    char *getText();
    //
    // Set the current text in the text box. 
    //
    void setText(const char *value, boolean doActivate = FALSE);

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassTextPopup;
    }
};


#endif // _TextPopup_h
