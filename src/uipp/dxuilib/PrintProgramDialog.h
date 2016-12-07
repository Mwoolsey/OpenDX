/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#ifndef _PrintProgramDialog_h
#define _PrintProgramDialog_h


#include "Dialog.h"
//#include "Application.h"


//
// Class name definition:
//
#define ClassPrintProgramDialog	"PrintProgramDialog"

class EditorWindow;
class Dialog;

extern "C" void PrintProgramDialog_ButtonCB(Widget w,
				    XtPointer clientData,
				    XtPointer callData);
extern "C" void PrintProgramDialog_ToFileToggleCB(Widget w,
				    XtPointer clientData,
				    XtPointer callData);
//
// PrintProgramDialog class definition:
//				
class PrintProgramDialog : public Dialog
{
   private:

    static boolean ClassInitialized;
    static void ConfirmationCancel(void *data);
    static void ConfirmationOk(void *data);


    //
    // Handle this->toFileToggle value changes.
    //
    friend void PrintProgramDialog_ToFileToggleCB(Widget w,
				    XtPointer clientData,
				    XtPointer callData);
    //
    // Handles the fileSelectButton callbacks and causes the 
    // printProgramFileDialog  to be created if necessary and then posted.
    //
    friend void PrintProgramDialog_ButtonCB(Widget w,
				    XtPointer clientData,
				    XtPointer callData);

  protected:
    static String  DefaultResources[];

    EditorWindow *editor;
    Dialog	*printProgramFileDialog; 
    Widget	printerName;
    Widget	labelParamsToggle;
    Widget	toFileToggle;
    Widget	fileName;
    Widget	fileSelectButton;

    virtual Widget createDialog(Widget parent);
    virtual boolean okCallback(Dialog *d);

    //
    // Set the sensitivity of the widgets based on the toFileToggle
    // state.
    //
    void setSensitivity();

    //
    // Opens the PostScript file selection dialog.
    //
    void postFileSelector();

    boolean printProgram(); 

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
    PrintProgramDialog( EditorWindow *e);


    //
    // Destructor:
    //
    ~PrintProgramDialog();

    //
    // Call the superclass method and then install a default file name
    // based on the name of the network associated with our editor.
    //
    virtual void manage();

    //
    // Set the name of the file as displayed in the fileName text widget.
    // Text is always right justified.
    //
    void setFileName(const char *filename);

    //
    // Return TRUE if the toFileToggle is set indicating that we
    // should print to a file instead of the printer.
    //
    boolean isPrintingToFile();

    //
    // Return TRUE if the labelParamsToggle is set indicating that we
    // should include parameter labels.
    //
    boolean isLabelingParams();

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassPrintProgramDialog;
    }
};


#endif // _PrintProgramDialog_h
