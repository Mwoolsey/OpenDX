/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"

 
 
#ifndef _TickLabel_h
#define _TickLabel_h
 
 
#include "UIComponent.h"

//
// Class name definition:
//
#define ClassTickLabel "TickLabel"

extern "C" void TickLabel_SelectEH(Widget, XtPointer, XEvent*, Boolean*);
extern "C" void TickLabel_DeleteOneCB(Widget, XtPointer, XtPointer);
extern "C" void TickLabel_AppendOneCB(Widget, XtPointer, XtPointer);
extern "C" void TickLabel_NumberCB(Widget, XtPointer, XtPointer);
extern "C" void TickLabel_NumberArmCB(Widget, XtPointer, XtPointer);
extern "C" void TickLabel_TextModifyCB(Widget, XtPointer, XtPointer);

class TickLabel;
typedef void (*TickSelectCB)(TickLabel*, void*);

#define DIRTY_TICK_NUMBER 1
#define DIRTY_TICK_TEXT 2

// Only used in TickLabelList.
class TickLabel : public UIComponent
{
  private:

    static boolean ClassInitialized;
    static String DefaultResources[];

    Widget 	number;
    Widget 	text;
    Widget 	form;

    String 	str;
    int 	dirty;
    double 	dval;
    int 	pos;
    boolean 	selected;
    boolean	ignoreVerifyCallback;

    TickSelectCB tscb;
    void*	clientData;

  protected:
    friend void TickLabel_SelectEH(Widget, XtPointer, XEvent*, Boolean*);
    friend void TickLabel_DeleteOneCB(Widget, XtPointer, XtPointer);
    friend void TickLabel_AppendOneCB(Widget, XtPointer, XtPointer);
    friend void TickLabel_NumberCB(Widget, XtPointer, XtPointer);
    friend void TickLabel_NumberArmCB(Widget, XtPointer, XtPointer);
    friend void TickLabel_TextModifyCB(Widget, XtPointer, XtPointer);

  public:

    void setNumber() { this->dirty&= ~DIRTY_TICK_NUMBER; };
    void setNumber (double dval);
    double getNumber() { return this->dval; };
    void setSelected (boolean set = TRUE, boolean callCallback = TRUE);
    boolean isSelected () { return this->selected; };

    void setText () { this->dirty&= ~DIRTY_TICK_TEXT; };
    void setText (const char *str);
    const char *getText();

    boolean isModified() { return (boolean)this->dirty; };
    boolean isNumberModified() { return this->dirty & DIRTY_TICK_NUMBER; };
    boolean isTextModified() { return this->dirty & DIRTY_TICK_TEXT; };

    virtual void initialize();
    void createLine(Widget parent);
    void destroyLine();
    void setPosition (int pos) { 
	ASSERT (this->form == NUL(Widget));
	this->pos = pos;
    }

    TickLabel (
	double dval, 
	const char *str, 
	int pos, 
	TickSelectCB tscb,
	void *clientData
    );

    ~TickLabel ();

    const char* getClassName() { return ClassTickLabel; };
};


#endif // _TickLabel_h
