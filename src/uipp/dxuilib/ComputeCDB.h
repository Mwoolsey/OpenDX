/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#ifndef _ComputeCDB_h
#define _ComputeCDB_h


#include "ConfigurationDialog.h"
#include "List.h"


//
// Class name definition:
//
#define ClassComputeCDB	"ComputeCDB"

//
// XtCallbackProc (*CB), XtEventHandler (*EH) and XtActionProc (*AP)
// DialogCallback (*DCB), XtInputCallbackProc (*ICP), XtWorkProc (*WP)
// functions for this and derived classes
//
extern "C" void ComputeCDB_NameChangedCB(Widget, XtPointer, XtPointer);
extern "C" void ComputeCDB_ExpressionChangedCB(Widget, XtPointer, XtPointer);
extern "C" void ComputeCDB_ExpressionModifiedCB(Widget, XtPointer, XtPointer);

//
// Referenced classes
class Node;
class List;

//
// ComputeCDB class definition:
//				
class ComputeCDB : public ConfigurationDialog
{
  private:
    //
    // Private member data:
    //
    static boolean ClassInitialized;
    static String  DefaultResources[];

    //
    // A special modified bit for the expression field. Normally, a cdb uses a
    // CDBInput object for each input and that object keeps track of dirtiness.  
    // The expression input doesn't get one so we provide one here.
    //
    boolean expression_modified;
    void    setExpression (const char*);

    //
    // Various X callbacks called whenever either the expression or a name
    // changes.
    //
    friend void ComputeCDB_ExpressionChangedCB(Widget widget,
				  XtPointer clientData,
				  XtPointer);
    friend void ComputeCDB_NameChangedCB(Widget widget,
			    XtPointer clientData,
			    XtPointer);
    friend void ComputeCDB_ExpressionModifiedCB(Widget, XtPointer, XtPointer);

  protected:
    //
    // Protected member data:
    //
    // The compute expression.
    Widget expressionLabel;
    Widget expression;
    char   *initialExpression;

    //
    // Protected functions

    // See the ConfigurationDialog's description of these.  createInputs
    // replaces the standard inputs section with one that consists of
    // a text widget for each of the expression's parameters (and a source
    // field), and the expression itself.
    virtual Widget createDialog(Widget parent);
    virtual Widget createInputs(Widget parent, Widget top);
    boolean        widgetChanged(int i, boolean send = TRUE);
    virtual void   remanageInputs(boolean force);

    virtual boolean applyCallback(Dialog *d);
    virtual void restoreCallback(Dialog *d);


    //
    // Install the default resources for this class and then call the
    // same super class method to get the default resources from the
    // super classes.
    //
    virtual void installDefaultResources(Widget baseWidget);

  public:
    static ConfigurationDialog *AllocateConfigurationDialog(Widget parent,
							   Node *node);
    //
    // Constructor:
    //
    ComputeCDB(Widget parent, Node *node);

    //
    // Destructor:
    //
    ~ComputeCDB();

    // These are 1-based.
    virtual void changeInput(int i);
    virtual void newInput(int i);
    virtual void deleteInput(int i);
    virtual void manage();

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassComputeCDB;
    }
};


#endif // _ComputeCDB_h
