/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"


#ifndef _TextFile_h
#define _TextFile_h


#include "UIComponent.h"
#include "Dialog.h"

//
// TextFileCallback type definition:
// Must be carefull these don't collide with other Text* classes 
//
class TextFile;
typedef void (*TextFileSetTextCallback)(TextFile *tp, const char*, void*);
typedef void (*TextFileChangeTextCallback)(TextFile *tp, const char*, void*);
//
// Class name definition:
//
#define ClassTextFile	"TextFile"

//
// TextFileCallback (*DCB) functions for this and derived classes
//
extern "C" void TextFile_ActivateTextCB(Widget widget,
			   XtPointer clientData,
			   XtPointer);
extern "C" void TextFile_ValueChangedTextCB(Widget widget,
			   XtPointer clientData,
			   XtPointer);
extern "C" void TextFile_fsdButtonCB(Widget widget,
			   XtPointer clientData,
			   XtPointer);
//
// TextFile class definition:
// The line in CreateMacro Dialog box of 
// Filename: box ellipses 
// The only place its used that I can tell.
class TextFile : public UIComponent
{
  private:
    //
    // Private member data:
    //
    static String DefaultResources[];
    static boolean ClassInitialized;

    friend void TextFile_ActivateTextCB(Widget widget,
			   XtPointer clientData,
			   XtPointer);
    friend void TextFile_ValueChangedTextCB(Widget widget,
			   XtPointer clientData,
			   XtPointer);
    friend void TextFile_fsdButtonCB(Widget widget,
			   XtPointer clientData,
			   XtPointer);

    TextFileSetTextCallback    	setTextCallback;
    TextFileChangeTextCallback 	changeTextCallback;
    void		*callbackData;

    void initInstanceData();

  protected:
    //
    // Protected member data:
    //

    Widget fileName;
    Widget fsdButton;
    Dialog *textFileFileDialog;

    //
    // One time class initializations.
    //
    virtual void initialize();

    //
    // Called when the user hits a return in the text window.
    //

    virtual void activateTextCallback(const char *value, void *callData);

    //
    // Called when the characters in the text window changes
    //

    virtual void valueChangedTextCallback(const char *value, void *callData);


    //
    // Install the default resources for this class and then call the
    // same super class method to get the default resources from the
    // super classes.
    //
    virtual void installDefaultResources();

    //
    // Constructor:
    //
    TextFile(const char *name);

  public:
    //
    // Constructor:
    //
    TextFile();

    //
    // Destructor:
    //
    ~TextFile();

    virtual Widget createTextFile(Widget parent, 
				TextFileSetTextCallback stc = NULL,
				TextFileChangeTextCallback ctc = NULL,
				void *callbackData = NULL);

    //
    // De/Install callbacks for the text widget. 
    //
    void enableCallbacks(boolean enable = TRUE);
    void disableCallbacks() { this->enableCallbacks(FALSE); }

    //
    //	To be called by the TextFileFileDialog when a file is selected
    //

    void fileSelectCallback(const char *value);

    //
    // Post the FileSelector Dialog
    virtual void postFileSelector();



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
	return ClassTextFile;
    }
};


#endif // _TextFile_h
