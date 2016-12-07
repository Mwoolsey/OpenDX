/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#ifndef _ConfigurationDialog_h
#define _ConfigurationDialog_h


#include "Dialog.h"
#include "List.h"
#include "InfoDialogManager.h"
#include "IBMApplication.h"


//
// Class name definition:
//
#define ClassConfigurationDialog	"ConfigurationDialog"

//
// XtCallbackProc (*CB), XtEventHandler (*EH) and XtActionProc (*AP)
// DialogCallback (*DCB), XtInputCallbackProc (*ICP), XtWorkProc (*WP)
// functions for this and derived classes
//
extern "C" void ConfigurationDialog_ActivateOutputCacheCB(Widget, XtPointer, XtPointer);
extern "C" void ConfigurationDialog_ValueChangedInputHideCB(Widget, XtPointer, XtPointer);
extern "C" void ConfigurationDialog_ValueChangedInputNameCB(Widget, XtPointer, XtPointer);
extern "C" void ConfigurationDialog_DescriptionCB(Widget, XtPointer, XtPointer);
extern "C" void ConfigurationDialog_CollapseCB(Widget, XtPointer, XtPointer);
extern "C" void ConfigurationDialog_ExpandCB(Widget, XtPointer, XtPointer);
extern "C" void ConfigurationDialog_RestoreCB(Widget, XtPointer, XtPointer);
extern "C" void ConfigurationDialog_ApplyCB(Widget, XtPointer, XtPointer);
extern "C" void ConfigurationDialog_SyntaxCB(Widget, XtPointer, XtPointer);
extern "C" void ConfigurationDialog_ResizeCB(Widget, XtPointer, XtPointer);

//
// Referenced classes
class Node;
class List;
class DescrDialog;
class TextPopup;


//
// ConfigurationDialog class definition:
//				
class ConfigurationDialog : public Dialog
{
  private:
    //
    // Private member data:
    //
    static boolean ClassInitialized;

    //
    // Various X Callbacks.  Apply and Restore are called upon pressing
    // the apply and restore buttons, and in turn call this->applyCallback
    // and this->restoreCallback.
    // The others are called when the input Value is either activated or
    // changed, or the input name is toggled.
    //
    friend void ConfigurationDialog_ApplyCB(Widget widget, XtPointer clientData, XtPointer);
    friend void ConfigurationDialog_RestoreCB(Widget widget, XtPointer clientData, XtPointer);
    friend void ConfigurationDialog_ExpandCB(Widget widget, XtPointer clientData, XtPointer);
    friend void ConfigurationDialog_CollapseCB(Widget widget, XtPointer clientData, XtPointer);
    friend void ConfigurationDialog_DescriptionCB(Widget widget, 
			XtPointer clientData, XtPointer);
    static void ActivateInputValueCB(TextPopup *, const char *, void *);	
    static void ValueChangedInputValueCB(TextPopup *, const char *, void *);	
    friend void ConfigurationDialog_ValueChangedInputNameCB(Widget widget, 
			XtPointer clientData, XtPointer);
    friend void ConfigurationDialog_ValueChangedInputHideCB(Widget widget, 
			XtPointer clientData, XtPointer);
    friend void ConfigurationDialog_ActivateOutputCacheCB(Widget widget, 
			XtPointer clientData, XtPointer);
    friend void ConfigurationDialog_ResizeCB(Widget widget, 
			XtPointer clientData, XtPointer);
    //
    // Initialize instance data for the constructors
    //
    void initInstanceData(Node *node);

    // hold the help string so that it's not leaked each time
    // it's requested.
    static char* HelpText;


  protected:
    //
    // Protected member data:
    //
    static String  DefaultResources[];
    Pixel   standardTopShadowColor;
    Pixel   standardBottomShadowColor;
    Dimension standardHighlightThickness;

    Node  *node;		// The node that this is for.

    Widget notation;		// The notation text widget.
    char   *initialNotation;	// the initial node comment.
    Widget apply;		// The Apply button
    Widget syntax;		// The Help (Syntax) button
    Widget restore;		// The Restore button
    Widget expand;		// The Expand button
    Widget collapse;		// The Collapse button

    Widget inputForm;		// form for inputs' labels and scrolledWindow.
    Widget inputTitle;		// title for all inputs
    Widget inputNameLabel;	// Top widget of the input's name column
    Widget inputHideLabel;	// Top widget of the input's hide column
    Widget inputTypeLabel;	// Top widget of the input's type column
    Widget inputSourceLabel;	// Top widget of the input's source column
    Widget inputValueLabel;	// Top widget of the input's value column
    Widget inputScroller;	// ScrolledWindow for inputs
    Widget scrolledInputForm;	// Form containing only inputs and no labels
    Widget buttonSeparator;

    Widget outputForm;		// form for all outputs.
    Widget outputTitle;		// title for all outputs
    Widget outputNameLabel;	// Top widget of the output's name column
    Widget outputTypeLabel;	// Top widget of the output's type column
    Widget outputDestLabel;	// Top widget of the output's destination column
    Widget outputCacheLabel;	// Top widget of the output's cache column

    DescrDialog *descriptionDialog;

    List   inputList;		// CDBInputs
    List   outputList;		// CDBOutputs

    //
    // Protected functions
    //
    // Functions to create and clean up the dialog.  These are virtual in 
    // the Dialog class.  CreateDialog by default sets up the Notation, 
    // the buttons at the bottom, and the body by calling the virtual function
    // createBody.
    Widget createDialog(Widget parent);

    //
    // createBody sets up the part of the dialog between the Notation and the
    // buttons.  By default it calls createInputs and createOutputs, which,
    // by default, make use of typeWidth to set the width of these two parts.
    // All of these member functions take a "top" widget that should be made
    // XmNtopAttachment = XmATTACH_WIDGET, XmNtopWidget = top.  They should
    // return a widget that the next thing down topAttaches to.  If any returns
    // NULL, the next thing down will attach to the widget passed in as top.
    int typeWidth;
    virtual Widget createBody(Widget parent);
    virtual Widget createInputs(Widget parent, Widget);
    virtual Widget createOutputs(Widget parent, Widget top);

    //
    // widgetChanged is called to allow the CDB to update the Node's Parameter
    // based on new stuff.  Returns FALSE if there's an error.
    virtual boolean   widgetChanged(int i, boolean send = TRUE);

    //
    // redraw all inputs, and make them visible either based on the parameter's
    // visiblility (!force), or force them all to be visible.
    virtual void   remanageInputs(boolean force);

    //
    // These are called by pushing the various buttons.
    // OK and Apply return FALSE on error indicating not to unmanage
    // the dialog (a non-sequitor for apply).
    virtual boolean okCallback(Dialog *d);
    virtual boolean applyCallback(Dialog *d);
    virtual void helpCallback(Dialog *d);
    virtual void cancelCallback(Dialog *d);
    virtual void restoreCallback(Dialog *d);
    virtual void collapseCallback(Dialog *d);
    virtual void expandCallback(Dialog *d);
    virtual void resizeCallback();

    //
    // Constructor (for derived classes):
    //
    ConfigurationDialog(const char *name, Widget parent, Node *node);

    //
    // Install the default resources for this class and then call the
    // same super class method to get the default resources from the
    // super classes.
    //
    virtual void installDefaultResources(Widget baseWidget);

  public:
    static ConfigurationDialog* AllocateConfigurationDialog(Widget parent,
						    Node *node);
    //
    // Constructor (for instances of THIS class).  Should not be
    // called by the derived classes (see the other Constructor).
    //
    ConfigurationDialog(Widget parent, Node *node);

    //
    // Destructor:
    //
    ~ConfigurationDialog();

    virtual void post();
    virtual void unmanage();
    virtual void manage();

    const char *getHelpSyntaxString();

    // These functions are called by anyone when the Node gets reconfigured.
    // changeIn/Output are called when parameter i has changed status or
    // value or connection.  newIn/Output are called whenever a parameter
    // is added, and deleteIn/Output are called whenever a parameter is
    // deleted.  "i" in these is 1-based.
    virtual void changeInput(int i);
    virtual void changeOutput(int i);
    virtual void newInput(int i);
    virtual void newOutput(int i);
    virtual void deleteInput(int i);
    virtual void deleteOutput(int i);

    virtual void changeLabel();

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassConfigurationDialog;
    }
};


#endif // _ConfigurationDialog_h
