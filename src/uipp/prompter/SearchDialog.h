/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"



#ifndef _SearchDialog_h
#define _SearchDialog_h


#include "../base/Dialog.h"

//
// Class name definition:
//
#define ClassSearchDialog	"SearchDialog"

//
// XtCallbackProc (*CB), XtEventHandler (*EH) and XtActionProc (*AP)
// DialogCallback (*DCB), XtInputCallbackProc (*ICP), XtWorkProc (*WP)
// functions for this and derived classes
//
extern "C" void SearchDialog_TextCB(Widget, XtPointer, XtPointer);
extern "C" void SearchDialog_CloseCB(Widget, XtPointer, XtPointer);
extern "C" void SearchDialog_PrevCB(Widget, XtPointer, XtPointer);
extern "C" void SearchDialog_NextCB(Widget, XtPointer, XtPointer);

class Browser;

//
// SearchDialog class definition:
//				

class SearchDialog : public Dialog
{

  private:
    //
    // Private member data:
    //
    static boolean ClassInitialized;
    static String  DefaultResources[];

    Widget 	mainform;
    Widget	close_pb;
    Widget	prev_pb;
    Widget	next_pb;
    Widget	text_label;
    Widget	text;
    Browser	*browser;
    int		last_search;
    enum{
		FORWARD,
		BACKWARD
	};

    friend void SearchDialog_NextCB(Widget, XtPointer, XtPointer);
    friend void SearchDialog_PrevCB(Widget, XtPointer, XtPointer);
    friend void SearchDialog_CloseCB(Widget, XtPointer, XtPointer);
    friend void SearchDialog_TextCB(Widget, XtPointer, XtPointer);
  protected:
    //
    // Protected member data:
    //
    Widget createDialog(Widget);

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
    SearchDialog(Widget parent, Browser*);

    //
    // Destructor:
    //
    ~SearchDialog();

    void    	mapRaise();
    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassSearchDialog;
    }
};


#endif // _SearchDialog_h
