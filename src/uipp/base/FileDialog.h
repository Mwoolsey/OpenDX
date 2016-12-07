/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"



#ifndef _FileDialog_h
#define _FileDialog_h


#include "Dialog.h"


//
// Class name definition:
//
#define ClassFileDialog	"FileDialog"

//
// FileDialog class definition:
//				

class FileDialog : public Dialog
{
  private:
    //
    // Private member data:
    //
    static boolean ClassInitialized;

  protected:
    //
    // Protected member data:
    //
    static String  DefaultResources[];
    boolean 	hasCommentButton;
    char	*readOnlyDirectory;

    Widget shell;
    Widget fsb;

    virtual Widget createDialog(Widget parent);

    virtual boolean okCallback(Dialog *d);

    //
    // Create the file selection box (i.e. the part with the filter, directory
    // and file list boxes, and the 4 buttons, ok, comment, cancel, apply).
    // The four buttons are set the have a width of 120
    //
    Widget createFileSelectionBox(Widget parent, const char *name);

    //
    // Called by the okCallback() after extracting the string from
    // the file selection box.
    //
    virtual void okFileWork(const char *string) = 0;

    //
    // If 'limit' is true, change the dialog box so that it operates in 
    // limited mode, that is, the user can only select file from the 
    // current directory specified by XmNdirectory.
    //
    virtual void displayLimitedMode(boolean limit);

    //
    // Set the name of the current file.
    //
    void setFileName(const char *file);

    // 
    // Get the default file name that is to be placed in the filename text
    // each time this is FileDialog::manage()'d.  By default this returns
    // NULL and nothing happens.  Subclasses can implement this to acquire
    // the described behavior.  The returned string will be deleted in
    // FileDialog::manage().
    //
    virtual char *getDefaultFileName();

    //
    // Constructor: protected since this is an abstract class
    //
    FileDialog(const char *name, Widget parent);

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
    ~FileDialog();

    Widget getFileSelectionBox() { return this->fsb; }

    // Redefine manage to make sure the fsb is updated before it's managed.
    virtual void post();
    virtual void manage();

    //
    // If dirname is not NULL, then set the dialog so that the directory is
    // fixed.  If dirname is NULL, then the fixed directory is turned off. 
    //
    void setReadOnlyDirectory(const char *dirname);

    //
    // Get the name of the currently selected file.
    // The returned string must be freed by the caller.
    // If a filename is not currently selected, then return NULL.
    //
    char *getSelectedFileName();

    //
    // Get the name of the current directory (without a trailing '/').
    // The returned string must be freed by the caller.
    //
    char *getCurrentDirectory();


    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassFileDialog;
    }
};


#endif // _FileDialog_h
