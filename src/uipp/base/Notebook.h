/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
#ifndef _NOTEBOOK_H_
#define _NOTEBOOK_H_

#include "UIComponent.h"
#include "List.h"


#define ClassNotebook "Notebook"

// Notebook is the Panel area including tabs and
// pages where modules are placed.
class Notebook : public UIComponent
{
    private:
	int index_of_selection;

	static boolean ClassInitialized;

    protected:

	Widget page_tab_form;
	Widget scrolled_window;
	Widget manager;

	// list of NotebookTab objects
	List tabs;

	// list of form widgets
	List pages;

	// list of names of the tabs
	List names;

    public:
	Notebook (Widget parent);

	virtual ~Notebook();

	Widget getTabManager() { return this->page_tab_form; }
	Widget getPageManager() { return this->manager; }

	virtual void addPage (const char* name, Widget page);
	virtual void showPage (int page);
	virtual void showPage (const char* name);
	virtual Widget getPage(const char* name);
	
	void showRectangle(int x1, int y1, int x2, int y2);

	virtual Widget getSelectedPage();

	//
	// Returns a pointer to the class name.
	//
	const char* getClassName()
	{
	    return ClassNotebook;
	}
};

#endif //_NOTEBOOK_H_
