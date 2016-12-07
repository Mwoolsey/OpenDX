/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"



#ifndef _FindToolDialog_h
#define _FindToolDialog_h


#include "Dialog.h"

//
// Class name definition:
//
#define ClassFindToolDialog	"FindToolDialog"

//
// XtCallbackProc (*CB), XtEventHandler (*EH) and XtActionProc (*AP)
// DialogCallback (*DCB), XtInputCallbackProc (*ICP), XtWorkProc (*WP)
// functions for this and derived classes
//
extern "C" void FindToolDialog_TextVerifyCB(Widget, XtPointer, XtPointer);
extern "C" void FindToolDialog_DefaultCB(Widget, XtPointer, XtPointer);
extern "C" void FindToolDialog_SelectCB(Widget, XtPointer, XtPointer);
extern "C" void FindToolDialog_TextChangeCB(Widget, XtPointer, XtPointer);
extern "C" void FindToolDialog_RestoreCB(Widget, XtPointer, XtPointer);
extern "C" void FindToolDialog_CloseCB(Widget, XtPointer, XtPointer);
extern "C" void FindToolDialog_UndoCB(Widget, XtPointer, XtPointer);
extern "C" void FindToolDialog_RedoCB(Widget, XtPointer, XtPointer);
extern "C" void FindToolDialog_FindCB(Widget, XtPointer, XtPointer);
extern "C" void FindToolDialog_PopdownCB(Widget, XtPointer, XtPointer);

class ButtonInterface;
class CommandInterface;
class EditorWindow;
class ToolSelector;
class FindStack;
class WarningDialogManager;
class List;

//
// FindToolDialog class definition:
//				

class FindToolDialog : public Dialog
{
  private:
    //
    // Private member data:
    //
    static boolean ClassInitialized;
    static String  DefaultResources[];

    List   *moduleList;
    char   lastName[40];
    int    lastInstance;
    int    lastIndex;
    int    lastPos;
    int    maxInstance;
    boolean complete;
    FindStack *redoStack, *undoStack;

    Widget closebtn;
    Widget undobtn;
    Widget redobtn;
    Widget restorebtn;
    Widget findbtn;
    Widget text;
    Widget list;

    EditorWindow* editor;
    WarningDialogManager* warningDialog;
 
  protected:
    //
    // Protected member data:
    //

    friend void FindToolDialog_PopdownCB(Widget, XtPointer , XtPointer);
    friend void FindToolDialog_FindCB(Widget, XtPointer , XtPointer);
    friend void FindToolDialog_RedoCB(Widget, XtPointer , XtPointer);
    friend void FindToolDialog_UndoCB(Widget, XtPointer , XtPointer);
    friend void FindToolDialog_CloseCB(Widget, XtPointer , XtPointer);
    friend void FindToolDialog_RestoreCB(Widget, XtPointer , XtPointer);
    friend void FindToolDialog_TextChangeCB(Widget, XtPointer , XtPointer);
    friend void FindToolDialog_SelectCB(Widget, XtPointer , XtPointer);
    friend void FindToolDialog_DefaultCB(Widget, XtPointer , XtPointer);
    friend void FindToolDialog_TextVerifyCB(Widget, XtPointer , XtPointer);

    int selectNode(char* name, int instance, boolean newNode = TRUE);
    Widget createDialog(Widget);

    void	restoreSens(boolean);
    void	redoSens(boolean);
    void	undoSens(boolean);

    //
    // Install the default resources for this class and then call the
    // same super class method to get the default resources from the
    // super classes.
    //
    virtual void installDefaultResources(Widget baseWidget);

  public:

    //
    // Constructor:
    //
    FindToolDialog(EditorWindow*);

    //
    // Destructor:
    //
    ~FindToolDialog();

    virtual void    	manage();

    void changeList();

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassFindToolDialog;
    }
};


#endif // _FindToolDialog_h
