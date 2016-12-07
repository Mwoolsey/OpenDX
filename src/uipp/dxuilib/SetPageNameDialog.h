/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#ifndef _SetPageNameDialog_h
#define _SetPageNameDialog_h


#include "Dialog.h"

extern "C" void SetPageNameDialog_ApplyCB (Widget, XtPointer clientData, XtPointer);
extern "C" void SetPageNameDialog_ModifyNameCB (Widget , XtPointer , XtPointer );


//
// Class name definition:
//
#define ClassSetPageNameDialog	"SetPageNameDialog"

class PageSelector;
class PageTab;
class EditorWorkSpace;

//
// SetPageNameDialog class definition:
//				
class SetPageNameDialog : public Dialog
{
  private:
    //
    // Private member data:
    //
    boolean incl;
    boolean auto_scroll;
    char* page_name;
    int position;
    int max;

    Widget page_name_widget;
    Widget ps_toggle;
    Widget auto_scroll_toggle;
    Widget position_number;
    Widget apply;

    static boolean ClassInitialized;

    EditorWorkSpace* workspace;
    PageSelector* selector;

    boolean stop_updates;
    boolean applyCallback(Dialog *d);

  protected:
    //
    // Protected member data:
    //
    Widget pageName;

    static String DefaultResources[];

    virtual boolean okCallback(Dialog *d);
    virtual Widget createDialog(Widget parent);

    //
    // Install the default resources for this class and then call the
    // same super class method to get the default resources from the
    // super classes.
    //
    virtual void installDefaultResources(Widget baseWidget);

    //
    // Check the given name to see if it is a valid page name.
    // If it is return TRUE, othewise return FALSE and issue an error
    // message.
    //
    boolean verifyPageName(const char *name);

    void update();

    friend void SetPageNameDialog_ApplyCB (Widget, XtPointer clientData, XtPointer);
    friend void SetPageNameDialog_ModifyNameCB (Widget , XtPointer , XtPointer );

  public:
    //
    // Constructor:
    //
    SetPageNameDialog(Widget parent, PageSelector* selector);

    //
    // Destructor:
    //
    ~SetPageNameDialog();

    void setWorkSpace(EditorWorkSpace* , const char* , int , int );

    virtual void manage();

    //
    // Returns a pointer to the class name.
    //
    virtual const char* getClassName()
    {
	return ClassSetPageNameDialog;
    }
};


#endif // _SetPageNameDialog_h
