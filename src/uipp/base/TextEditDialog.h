/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"



#ifndef _TextEditDialog_h
#define _TextEditDialog_h


#include "Dialog.h"

//
// Class name definition:
//
#define ClassTextEditDialog	"TextEditDialog"

//
// XtCallbackProc (*CB), XtEventHandler (*EH) and XtActionProc (*AP)
// DialogCallback (*DCB) functions for this and derived classes
//
extern "C" void TextEditDialog_ApplyCB(Widget, XtPointer, XtPointer);
extern "C" void TextEditDialog_RestoreCB(Widget, XtPointer, XtPointer);

class TextEditDialog;

//
// TextEditDialog class definition:
//				

class TextEditDialog : public Dialog
{
  private:
    //
    // Private member data:
    //

    friend void TextEditDialog_RestoreCB(Widget, XtPointer , XtPointer);
    friend void TextEditDialog_ApplyCB(Widget, XtPointer , XtPointer);


  protected:
    //
    // Protected member data:
    //
    static String  DefaultResources[];

    Widget editorText;
    int    dialogModality;
    boolean readOnly;
    boolean wizard;

    Widget createDialog(Widget);


    virtual void applyCallback(Widget    widget, XtPointer callData);
    virtual void restoreCallback(Widget    widget, XtPointer callData);
    virtual boolean okCallback(Dialog *d);

  
    //
    // Read the text in the text window and save it with saveText(). 
    // If there is no error then return TRUE. 
    //
    boolean saveEditorText();

    //
    // Get the the text that is to be edited. 
    // At this level we return NULL; the subclass may redefine this.
    //
    virtual const char *getText();

    //
    // Save the given text. 
    // At this level we do nothing; the subclass may redefine this.
    // If there is no error then return TRUE. 
    //
    virtual boolean saveText(const char *s);


    //
    // The title to be applied the newly managed dialog.
    // The returned string is freed by the caller (TextEditDialog::manage()).
    //
    virtual char *getDialogTitle();

    //
    // Constructor (for derived classes only):
    //
    TextEditDialog (const char *name, Widget parent, 
	boolean readonly = FALSE, boolean wizard = FALSE);

    //
    // Install the default resources for this class and then call the
    // same super class method to get the default resources from the
    // super classes.
    //
    virtual void installDefaultResources(Widget baseWidget);

  public:

    //
    // Destructor:
    //
    ~TextEditDialog();


    //
    // Get the the text that is in the text window.
    //
    const char *getEditorText();

    virtual void manage();

    //
    // Ask for the presumably new title from the derived class (usually) and 
    // install it.
    //
    void updateDialogTitle();

    //
    // Place the text returned by getText() into the text editor. 
    //
    void installEditorText(const char *s = NULL);

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassTextEditDialog;
    }
};


#endif // _TextEditDialog_h
