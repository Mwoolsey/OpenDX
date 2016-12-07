/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"

 
 
#ifndef _TickLabelList_h
#define _TickLabelList_h
 
 
#include "UIComponent.h"
#include "List.h"

//
// Class name definition:
//
#define ClassTickLabelList "TickLabelList"

extern "C" void TickLabelList_ButtonEH(Widget, XtPointer, XEvent*, Boolean*);
extern "C" void TickLabelList_DeleteAllCB(Widget, XtPointer, XtPointer);
extern "C" void TickLabelList_AppendCB(Widget, XtPointer, XtPointer);
extern "C" void TickLabelList_ResizeTicksCB(Widget, XtPointer, XtPointer);

extern "C" void TickLabelList_AppendAfterSelCB(Widget, XtPointer, XtPointer);
extern "C" void TickLabelList_InsertAboveSelCB(Widget, XtPointer, XtPointer);
extern "C" void TickLabelList_DeleteSelCB(Widget, XtPointer, XtPointer);

extern "C" void TickLabelList_SortUpCB(Widget, XtPointer, XtPointer);
extern "C" void TickLabelList_SortDownCB(Widget, XtPointer, XtPointer);

extern "C" int TickLabelList_SortFunc (const void* e1, const void* e2);

class TickLabel;
class TickLabelList;
typedef void (*TickListModifyCB) (TickLabelList*, void*);

// Used only in the AutoAxesDialog
class TickLabelList : public UIComponent
{
  private:

    static boolean ClassInitialized;
    static String DefaultResources[];

    char *header;

    Widget header_label;
    Widget header_button;
    Widget listRC;

    Widget popupMenu;
    Widget aasButton; // append after selection
    Widget iasButton; // insert above selection
    Widget dsButton;  // delete selection
    Widget ntiButton; // new top item
    Widget daButton;  // delete all
#if UI_SHOULD_SORT_TICKS
    Widget suButton;  // sort in ascending order
    Widget sdButton;  // sort in descending order
#endif
    Widget labelLabel;

    List ticks;

    int highest_set_number;

    int dirty;

    double *oldDvals;
    String oldString;

    static void SelectCB (TickLabel*, void*);
    TickListModifyCB  tlmcb;
    void *clientData;

    void resizeCallback();

  protected:
    friend void TickLabelList_ButtonEH(Widget, XtPointer, XEvent*, Boolean*);
    friend void TickLabelList_DeleteAllCB(Widget, XtPointer, XtPointer);
    friend void TickLabelList_AppendCB(Widget, XtPointer, XtPointer);
    friend void TickLabelList_ResizeTicksCB(Widget, XtPointer, XtPointer);

    friend void TickLabelList_AppendAfterSelCB(Widget, XtPointer, XtPointer);
    friend void TickLabelList_InsertAboveSelCB(Widget, XtPointer, XtPointer);
    friend void TickLabelList_DeleteSelCB(Widget, XtPointer, XtPointer);
    friend void TickLabelList_SortUpCB(Widget, XtPointer, XtPointer);
    friend void TickLabelList_SortDownCB(Widget, XtPointer, XtPointer);

    friend int TickLabelList_SortFunc (const void* e1, const void* e2);

    void sortList(boolean up);

    static boolean SortUp;

  public:

    void setNumber (int pos, double dval);
    void setText (int pos, char *str);
    void setListSize (int size);

    void createList (Widget parent);
    void createLine (double dval, const char *str, int pos);

    void clear();

    boolean isModified();
    boolean isNumberModified();
    boolean isTextModified();
    void markClean();
    void markNumbersClean();
    void markTextClean();
   
    double * getTickNumbers();

    // should be const.  ImageNode is misdefined.
    /*const*/ char * getTickTextString();
    int getTickCount() { return this->ticks.getSize(); };

    virtual void initialize();

    TickLabelList (const char *header, TickListModifyCB, void *);
    ~TickLabelList ();

    const char* getClassName() { return ClassTickLabelList; };
};


#endif // _TickLabelList_h
