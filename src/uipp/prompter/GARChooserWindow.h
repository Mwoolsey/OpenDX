/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


#ifndef _GARChooserWindow_h_
#define _GARChooserWindow_h_

#include "IBMMainWindow.h"
#include "dxl.h"

class ToggleButtonInterface;
class Command;
class CommandTextPopup;
class TypeChoice;
class DataFileDialog;

extern "C" void TypeChoice_BrokenConnCB (DXLConnection *, void *);
extern "C" void TypeChoice_HandleMessagesCB (XtPointer , int*, XtInputId* );
extern "C" Boolean GARChooserWindow_SyncWP (XtPointer );


#define ClassGARChooserWindow  "GARChooserWindow"

class GARChooserWindow: public IBMMainWindow
{
  private:
    CommandTextPopup *text_file;

    friend class TypeChoice;
    friend void TypeChoice_BrokenConnCB(DXLConnection*, void*);
    friend void TypeChoice_HandleMessagesCB (XtPointer , int*, XtInputId* );
    friend Boolean GARChooserWindow_SyncWP (XtPointer );

    static String DefaultResources[];
    static boolean ClassInitialized;

    char *		getFileReady();
    char *		findExtension(const char *);
    char *		file_search_dir;

    DXLConnection*	connection;

    boolean		mismatch_complaint;

    DataFileDialog*	dfdialog;

    XtWorkProcId	unset_sync_wpid;

  protected:

    virtual Widget createWorkArea (Widget menu_bar);
    virtual void   createMenus (Widget menu_bar);
    virtual void   createFileMenu (Widget menu_bar);
    virtual void   createOptionsMenu (Widget menu_bar);

    //
    // Commands
    //
    Command*	quitCmd;
    Command*	showMessagesCmd;
    Command*	selectDataCmd;
    Command*	openPrompterCmd;
    Command*	nullCmd;

    TypeChoice*	choice;
    List*	typeChoices;

    void 	showForms();
    void	controlDimension();

    //
    // Install the default resources for this class and then call the
    // same super class method to get the default resources from the
    // super classes.
    //
    virtual void installDefaultResources(Widget baseWidget);

  public:

    GARChooserWindow ();
    ~GARChooserWindow();

    void	setCommandActivation();
    void	setDataFile(const char *);
    void	fileNameCB();
    void	setChoice(TypeChoice *new_choice);
    TypeChoice* getChoice() { return this->choice; }

    virtual	void manage();
    virtual	void closeWindow();

    boolean	showMessages ();
    boolean	openExistingPrompter();
    boolean 	postDataFileSelector();
    boolean	nullFunc() { return TRUE; }

    Command*	getNullCmd() { return this->nullCmd; }

    const char *getDataFilename();

    DXLConnection* getConnection() { return this->connection; }

    void setFileSearchDir(const char *);

    void loadAndSet (DXLConnection*, const char* net_file, char* argv[], int argc);

    CommandTextPopup* getCommandText() { return this->text_file; }

    //
    // So that CommandTextPopup can post the file selector dialog
    Command* getSelectDataCmd() { return this->selectDataCmd; }

    virtual const char *getClassName()
    {
	return ClassGARChooserWindow;
    }
};

#endif
