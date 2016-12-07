/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
#include "Notebook.h"
#include "NotebookTab.h"
#include "ListIterator.h"
#include <Xm/PushB.h>
#include <Xm/Form.h>
#include <Xm/Frame.h>
#include <Xm/ScrolledW.h>
#include <Xm/ScrollBar.h>
#include "DXStrings.h"
#include "Application.h"

boolean Notebook::ClassInitialized = 0;
Notebook::Notebook(Widget parent) : UIComponent("notebook")
{
    if (!Notebook::ClassInitialized) {
	Notebook::ClassInitialized = TRUE;
    }

    Widget form = XtVaCreateManagedWidget (this->name, xmFormWidgetClass, parent, NULL);

    this->page_tab_form = XtVaCreateManagedWidget (
	    "pageTabForm", xmFormWidgetClass, form,
	    XmNtopAttachment, 		XmATTACH_FORM,
	    XmNleftAttachment, 		XmATTACH_FORM,
	    XmNrightAttachment, 	XmATTACH_NONE,
	    XmNbottomAttachment, 	XmATTACH_OPPOSITE_FORM,
	    XmNtopOffset, 		0,
	    XmNleftOffset, 		0,
	    XmNbottomOffset, 		-23,
	    XmNtraversalOn, 		FALSE,
	NULL);

    this->scrolled_window = XtVaCreateManagedWidget (
	    "swindow", xmScrolledWindowWidgetClass, form,
	    XmNtopAttachment,		XmATTACH_FORM,
	    XmNleftAttachment,		XmATTACH_FORM,
	    XmNrightAttachment,		XmATTACH_FORM,
	    XmNbottomAttachment,	XmATTACH_FORM,
	    XmNtopOffset,		22,
	    XmNleftOffset,		0,
	    XmNrightOffset,		0,
	    XmNbottomOffset,		0,
	    XmNscrollBarDisplayPolicy,	XmAS_NEEDED,
	    XmNscrollingPolicy,		XmAUTOMATIC,
	    XmNvisualPolicy,		XmVARIABLE,
	    XmNtraversalOn,		FALSE,
    NULL);

    this->manager = XtVaCreateManagedWidget ("manager", xmFormWidgetClass, 
	this->scrolled_window, XmNshadowThickness, 0, XmNtraversalOn, FALSE,
    NULL);

    // I don't think this is necessary but it does follow the doc.
    // XmNscrolledWindowChildType probably defaults
    // to XmWORK_AREA but you can't really tell by reading the man page.
    XtVaSetValues (this->scrolled_window, 
	XmNworkWindow, this->manager, 
#if (XmVersion>=2000)
	XmNscrolledWindowChildType, XmWORK_AREA,
#endif
    NULL);

    this->index_of_selection = -1;
    this->setRootWidget(form);

}

Notebook::~Notebook()
{
    ListIterator iter(this->names);
    char* name;
    while ((name = (char*)iter.getNext()) != NULL) {
	delete name;
    }
}

class NBTab : public NotebookTab {
    private:
	Notebook* notebook;
    protected:
	virtual void setState(boolean s=TRUE) {
	    this->NotebookTab::setState(s);
	    if (s) this->notebook->showPage(this->getLabel());
	}
    public:
       	NBTab(const char* name, Notebook* nb) : NotebookTab(name) {
	    this->notebook = nb;
	}
};

void Notebook::addPage (const char* name, Widget page)
{
    NotebookTab* nt = new NBTab (name, this);
    nt->createButton(this->page_tab_form);

    Widget tab = nt->getRootWidget();
    XtVaSetValues (tab, XmNwidth, 60, NULL);
    int tab_cnt = this->tabs.getSize();
    if (tab_cnt == 0) {
	XtVaSetValues (tab,
	    XmNtopAttachment,	XmATTACH_FORM,
	    XmNleftAttachment,	XmATTACH_FORM,
	    XmNrightAttachment, XmATTACH_NONE,
	    XmNbottomAttachment,XmATTACH_FORM,
	    XmNtopOffset, 0,
	    XmNleftOffset, 0,
	    XmNbottomOffset, 0,
	NULL);
    } else {
	NotebookTab* pnt = (NotebookTab*)this->tabs.getElement(tab_cnt);
	Widget prev = (Widget)pnt->getRootWidget();
	XtVaSetValues (tab,
	    XmNtopAttachment,	XmATTACH_FORM,
	    XmNleftAttachment,	XmATTACH_WIDGET,
	    XmNrightAttachment, XmATTACH_NONE,
	    XmNbottomAttachment,XmATTACH_FORM,
	    XmNleftWidget,	prev,
	    XmNtopOffset, 0,
	    XmNleftOffset, 0,
	    XmNbottomOffset, 0,
	NULL);
    }
    nt->manage();

    XtVaSetValues (page,
	XmNtopAttachment, XmATTACH_FORM,
	XmNleftAttachment, XmATTACH_FORM,
	XmNrightAttachment, XmATTACH_FORM,
	XmNbottomAttachment, XmATTACH_FORM,
    NULL);

    this->tabs.appendElement(nt);
    this->pages.appendElement(page);
    this->names.appendElement(DuplicateString(name));
    if (tab_cnt == 0) {
	nt->setState(TRUE);
    } else {
	if (XtIsManaged(page)) XtUnmanageChild(page);
    }
}

void Notebook::showPage (int pageId)
{
    // if pageId == this->index_of_selection, then we
    // still have to keep processing because the user may
    // have tried to unselect the tab of the current page
    // which has to be prevented.
    int cnt = this->tabs.getSize();
    Widget page_to_manage=0;
    Widget page_to_unmanage=0;
    for (int i=1; i<=cnt; i++) {
	NotebookTab* nt = (NotebookTab*)this->tabs.getElement(i);
	Widget tab = nt->getRootWidget();
	Widget page = (Widget)this->pages.getElement(i);
	if (i==pageId) {
	    if (!nt->getState())
		nt->setState(TRUE);
	    page_to_manage = page; 
	} else {
	    if (nt->getState())
		nt->setState(FALSE);
	    if (XtIsManaged(page))
		page_to_unmanage = page;
	}
	if ((page_to_manage)&&(page_to_unmanage)) break;
    }

    //
    // bug here:  I unmanage the page going away and manage the page
    // arriving.  Problem is that the parent of the 2 pages collapses
    // itself to 0 width and height.  I use setValues to return the
    // parent to its proper size.
    //
    if (pageId != this->index_of_selection) {
	if (this->index_of_selection != -1) {
	    ASSERT(page_to_unmanage);
	    XtUnmanageChild(page_to_unmanage);
	}
	Dimension w,h;
	XtVaGetValues (page_to_manage, XmNwidth,&w,XmNheight,&h,NULL);
	ASSERT(page_to_manage);
	ASSERT(page_to_manage != page_to_unmanage);
	XtVaSetValues (this->manager, XmNwidth,w,XmNheight,h,NULL);
	XtManageChild(page_to_manage);
    }
    this->index_of_selection = pageId;
}

Widget Notebook::getSelectedPage()
{
    return (Widget)this->pages.getElement(this->index_of_selection);
}

Widget Notebook::getPage (const char* name)
{
    ListIterator iter(this->names);
    const char* cp;
    int i = 1;
    while (cp=(const char*)iter.getNext()) {
	if (EqualString(cp, name)) 
	    return (Widget)this->pages.getElement(i);
	i++;
    }
    return NUL(Widget);
}

void Notebook::showPage (const char* name)
{
    ListIterator iter(this->names);
    const char* cp;
    int ndx = 1;
    int selection = -1;
    while (cp=(const char*)iter.getNext()) {
	if (EqualString(cp, name)) {
	    selection = ndx;
	    break;
	}
	ndx++;
    }
    ASSERT(selection != -1);
    this->showPage(selection);
}

void Notebook::showRectangle(int x1, int y1, int x2, int y2)
{
    Widget vbar, clip;
    XtVaGetValues (this->scrolled_window,
	XmNverticalScrollBar, &vbar,
	XmNclipWindow, &clip,
    NULL);
    ASSERT(vbar);
    ASSERT(clip);

    Dimension clip_height;
    XtVaGetValues (clip, XmNheight, &clip_height, NULL);
    int value, slider_size, min, max, incr, page_incr;
    boolean adjusted = FALSE;

    XtVaGetValues (vbar, 
	XmNsliderSize, &slider_size,
	XmNminimum, &min,
	XmNmaximum, &max,
	XmNvalue, &value,
	XmNincrement, &incr,
	XmNpageIncrement, &page_incr,
    NULL);


    if (y2 > (value + clip_height)) {
	adjusted = TRUE;
	value = y2-clip_height;
    } else if (y1 < value) {
	adjusted = TRUE;
	value = y1;
    }

    if (adjusted)
	XmScrollBarSetValues (vbar, value, slider_size, incr, page_incr, TRUE);
}

