/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#ifndef _MsgDialog_h
#define _MsgDialog_h


#include "Dialog.h"
#include <Xm/Xm.h>

//
// Class name definition:
//
#define ClassMsgDialog	"MsgDialog"

class TypeChoice;
class List;

//
// MsgDialog class definition:
//				

class MsgDialog : public Dialog
{
  private:
    //
    // Private member data:
    //
    static boolean ClassInitialized;

    Widget test_output;
    List* item_list;

  protected:
    //
    // Protected member data:
    //
    static String  DefaultResources[];

    TypeChoice *choice;

    //
    // Install the default resources for this class and then call the
    // same super class method to get the default resources from the
    // super classes.
    //
    virtual void installDefaultResources(Widget baseWidget);

    virtual void layoutChooser(Widget );
    virtual Widget createDialog (Widget );
    virtual boolean okCallback (Dialog* ); 

  public:

    //
    // Constructor:
    //
    MsgDialog(Widget parent, TypeChoice *choice);

    //
    // Destructor:
    //
    ~MsgDialog();

    void addItem (XmString xmstr, int pos);
    int  getMessageCount();

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassMsgDialog;
    }
};


#endif // _MsgDialog_h
