/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"


#ifndef _InteractorInstance_h
#define _InteractorInstance_h



#include "DXStrings.h"
#include "Base.h"
#include "InteractorNode.h"


class List;
class Interactor;
class Node;
class Network;
class ControlPanel;
class InteractorStyle;
class SetAttrDialog;

//
// Class name definition:
//
#define ClassInteractorInstance	"InteractorInstance"


//
// Describes an instance of an interactor in a control Panel.
//
class InteractorInstance : public Base {

    friend class Interactor;
    friend void InteractorNode::openDefaultWindow(Widget w);

  private:


  protected:

    char		*localLabel;
    Interactor		*interactor;
    ControlPanel	*controlPanel;
    int			xpos, ypos;
    int			width, height;
    InteractorStyle	*style;
    InteractorNode	*node;
    boolean		selected;
    boolean		verticalLayout;

    //
    // The dialog for setting attributes of the interactor.
    // This set in this->openDefaultWindow()
    //
    SetAttrDialog *setAttrDialog;

    //
    // Create the set attributes dialog.
    // This should also perform the function of this->loadSetAttrDialog().
    //
    virtual SetAttrDialog *newSetAttrDialog(Widget parent);

    //
    // Make sure the given output's current value complies with any attributes.
    // This is called by InteractorInstance::setOutputValue() which is
    // intern intended to be called by the Interactors.
    // If verification fails (returns FALSE), then a reason is expected to
    // placed in *reason.  This string must be freed by the caller.
    // At this level we always return TRUE (assuming that there are no
    // attributes) and set *reason to NULL.
    //
    virtual boolean verifyValueAgainstAttributes(int output, 
					 	const char *val,
						Type t,
						char **reason);
    virtual boolean defaultVertical() { return TRUE; }

    static int CountLines(const char*);
    static const char* FetchLine(const char*, int);

    char* java_var_name;
    virtual const char* javaName() { return "tfld"; }

  public:
    InteractorInstance(InteractorNode *n);
	
    ~InteractorInstance(); 

    void 	setPanel(ControlPanel *panel) 
				{ this->controlPanel= panel; }
    void 	setLocalLabel(const char *label); 

    void 	setXYPosition(int x, int y);
    void	setXYSize(int width, int height);

    void 	setStyle(InteractorStyle *is) 
				{ this->style = is; }
 
    boolean 	isSelected()  { return this->selected ; }

    void 	setSelected(boolean select);

    void 	setSelected() { this->setSelected(TRUE); } 
    void 	clrSelected() { this->setSelected(FALSE); } 
    virtual void setVerticalLayout(boolean val = TRUE); 
    boolean	isVerticalLayout()  { return this->verticalLayout; }

    //
    // Ask the InteractorStyle to build an Interactor for this instance.
    //
    void	createInteractor();

    const char 		*getInteractorLabel();
    const char 		*getLocalLabel() { return this->localLabel; }
    void 		getXYPosition(int *x, int *y);	

    //
    // If the interactor for this instance exists, then return TRUE
    // and the width and height in *x and *y respectively.
    // If the interactor does not exists return FALSE and set *x and *y to 0.
    //
    boolean getXYSize(int *x, int *y);


    InteractorStyle 	*getStyle() { return this->style; }
    ControlPanel 	*getControlPanel() { return this->controlPanel; }
    Widget		getParentWidget();
				
    Interactor		*getInteractor() { return this->interactor; }
    Node		*getNode() 	{ return this->node; }
    Network		*getNetwork();

    void                changeStyle(InteractorStyle* style);    

    //
    // Delete an interactor in such a way that we could create another one
    // with createInteractor().
    //
    void uncreateInteractor();

    //
    // Send notification to this->interactor that the attributes may have 
    // changed and to reflect these changes in the display Interactor. 
    // Some changes can be ignored if 'this != src_ii'.  This is typically
    // called by InteractorNode::notifyInstancesOfStateChange().
    //
    void		handleInteractorStateChange(
				InteractorInstance *src_ii, 
				boolean unmanage = FALSE);

    boolean isDataDriven() { InteractorNode *n = 
					(InteractorNode*)this->getNode();
			     return n->isDataDriven();
			   } 


    //
    // Return TRUE/FALSE indicating if this class of interactor instance has
    // a set attributes dialog (i.e. this->newSetAttrDialog returns non-NULL).
    // At this level in the class hierarchy, we return FALSE.
    //
    virtual boolean hasSetAttrDialog();

    //
    // Open the set attributes dialog for this Interactor.
    //
    void openSetAttrDialog();

    //
    // Make sure the given value complies with any attributes and if so
    // set the value in the Node.  This should generally be called from
    // interactors that can't directly enforce  attributes.  For example,
    // the Text style versus the Stepper style Vector interactor.  The
    // Stepper style enforces its attributes itself, but the Text style
    // accepts any value and then must have the value checked for type
    // and attribute compliance.
    // If we fail because attribute verification fails, then *reason contains
    // the reason (as passed back by verifyValueAgainstAttributes()) for
    // failure.  This string is expected to be freed by the caller.
    //
    boolean setAndVerifyOutput(int index, const char *val, 
					Type type, boolean send,
					char **reason);

    //
    // Called from ControlPanel in the process of performing cut/copy + paste
    // of interactors within the control panels of one executable.  If possible
    // change the pointers so that this instance no long belongs to a temp net
    // but to the real network (passwd in in newnet). Return True on success.
    //
    boolean switchNets (Network *newnet);

    virtual boolean printAsJava(FILE* );
    virtual const char* getJavaVariable();

    const char *getClassName() 
	{ return ClassInteractorInstance; }
};

#endif // _InteractorInstance_h

