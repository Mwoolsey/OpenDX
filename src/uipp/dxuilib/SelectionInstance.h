/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#ifndef _SelectionInstance_h
#define _SelectionInstance_h



//#include "DXStrings.h"
#include "SelectionNode.h"
#include "InteractorInstance.h"

class SelectionInteractor;


//
// Class name definition:
//
#define ClassSelectionInstance	"SelectionInstance"


//
// Describes an instance of an interactor in a control Panel.
//
class SelectionInstance : public InteractorInstance {

    friend class SelectionAttrDialog;

  private:


  protected:


    boolean appendOptionPair(const char *value, const char *name);

    //
    // Create the default  set attributes dialog box for this class of
    // Interactor.
    //
    virtual SetAttrDialog *newSetAttrDialog(Widget parent);

  public:
    SelectionInstance(InteractorNode *node) : InteractorInstance(node) {}
	
    ~SelectionInstance()  {}

    int 	getOptionCount();
    int 	getSelectedOptionIndex();
    const char	*getValueOptionsAttribute();
    char	*getOptionValueString(int optind);
    char	*getOptionNameString(int optind, boolean keep_quotes = FALSE);

    virtual boolean hasSetAttrDialog();

    const char *getClassName() 
		{ return ClassSelectionInstance; }
};

#endif // _SelectionInstance_h

