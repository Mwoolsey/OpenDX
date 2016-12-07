/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#include <Xm/Form.h>
#include <Xm/Frame.h>
#include <Xm/PushB.h>
#include <Xm/List.h>
#include <Xm/RowColumn.h>

#include "lex.h"
#include "DXStrings.h"
#include "ListEditor.h"


boolean ListEditor::ClassInitialized = FALSE;
String  ListEditor::DefaultResources[] =
{
    "*accelerators:		#augment\n"
#if 1
	"<Key>Return:			BulletinBoardReturn()",
#else
	"<Key>Return:			BulletinBoardReturn()\n"
	"<Btn2Down>,<Btn2Up>:		HELP!",
#endif

    // The following is good for most list interactors.
    "*frame*listEditorList.visibleItemCount: 	7",
    "*frame.topOffset:			5", 
    "*frame.leftOffset:			5",
    "*frame.rightOffset:		5",
    "*frame.bottomOffset:		5",
    "*frame.shadowThickness:		2",
    "*frame.marginWidth:		3",
    "*frame.marginHeight:		3",
    "*frame.resizable:			False",
    "*addButton.bottomOffset:		5",
    "*addButton.labelString:		Add",
    "*deleteButton.bottomOffset:	5",
    "*deleteButton.labelString:		Delete",
    //"*vScrollBar.width:			0",
    //"*vScrollBar.rightAttachment:	XmATTACH_FORM",
    "*listEditorList.listSizePolicy:	XmCONSTANT",

    NULL
};

#define UNSELECTED_INDEX -1

ListEditor::ListEditor(const char *name)  
{
    this->addButton	= NULL; 
    this->deleteButton	= NULL; 
    this->valueList	= NULL; 
    this->listFrame 	= NULL; 
    this->listSelection = UNSELECTED_INDEX;
}

ListEditor::~ListEditor()
{
}


//
// Build a DX style list out of the current list items. 
// The returned string must be deleted by the caller. 
// When the string is empty, NULL is returned. 
//
#define CHUNKSIZE 64 
char *ListEditor::buildDXList()
{
    int i, count = this->getListItemCount();
    char *list;


    if (count > 0) {
        int maxsize = CHUNKSIZE, size = 6;
	list = (char*)MALLOC(CHUNKSIZE*sizeof(char));
	strcpy(list,"{ ");
  	char *p = list;
	for (i=1 ; i<=count ; p+=STRLEN(p), i++) {
	    char *s = this->getListItem(i);
	    size += STRLEN(s);
	    if (size >= maxsize) {
		maxsize = size + CHUNKSIZE;
		list = (char*)REALLOC((void*)list, 
				      maxsize * sizeof(char));
		p = list;
	    }
	    if (s) {
		strcat(p,s);
		delete s;
	    }
	    if (i == count) 
	       strcat(p," ");
	    else
	       strcat(p,", ");
	    size += 2;
	}
	if (size >= maxsize) {
	    maxsize = size + 4;
	    list = (char*)REALLOC((void*)list, 
				      maxsize * sizeof(char));
	}
        strcat(list,"}");
    } else {
	list = NULL; 
    }
    return list;
}

//
// Set the currently selected item.
// Return FALSE if index is out of range (i.e 0 < index <= list item count).
//
boolean ListEditor::setSelectedItemIndex(int index)
{
     ASSERT(index >= 0);

     if (index > this->getListItemCount())
	return FALSE;

     XmListDeselectAllItems(this->valueList);
     this->listSelection = UNSELECTED_INDEX;
     XmListSelectPos(this->valueList,index, True); 

     return TRUE;
}
//
// Get the currently selected item.
// Returns zero if no item is selected. 
//
int ListEditor::getSelectedItemIndex()
{
    int count, *indices, selection;

    if (XmListGetSelectedPos(this->valueList,&indices,&count)) {
	// Things are set up so that only one selection is allowed 
        ASSERT(count == 1);	

	selection = indices[0];
	XtFree((char*)indices);
    } else {
	selection = 0;
    }

    return selection; 
}


//
// Get the number of items in the list.
//
int ListEditor::getListItemCount()
{
    int count;

    XtVaGetValues(this->valueList, XmNitemCount, &count, NULL);

    return count; 
}
//
// Get the text for the index'th item (1 based indexing)
// The returned string must be freed by the caller.
//
char *ListEditor::getListItem(int index)
{
    char *text;
    XmString *items;
     
    ASSERT(index > 0);

    XtVaGetValues(this->valueList, XmNitems, &items, NULL);
    XmStringGetLtoR(items[index-1], XmSTRING_DEFAULT_CHARSET, &text);
    return text;
}


/////////////////////////////////////////////////////////////////////////////
//
// Install the callback for the give widget.
// This is provided as a method so that 'this' is the right value (i.e.
// when the callback is installed from a derived class the value 'this'
// can be a pointer to the derived class and not a ListEditor).
//
void ListEditor::installValueEditorCallback(Widget w, String callback_name)
{
    ASSERT(w);
    ASSERT(callback_name);
    XtAddCallback(w,callback_name,(XtCallbackProc)ListEditor_ValueEditorCB, (XtPointer)this);
}
extern "C" void ListEditor_ValueEditorCB(Widget w, XtPointer clientData,
				XtPointer callData)
{
    ListEditor *le = (ListEditor*)clientData;
    le->valueEditorCallback(w, callData);
}
//
// If there is a change in value and there is a list item selected, 
// then display the new values in the selected list item.
//
void ListEditor::valueEditorCallback(Widget w, XtPointer cb)
{
    if (this->listSelection != UNSELECTED_INDEX)
        this->replaceAndSelectSelectedListItem(NULL);
}


/////////////////////////////////////////////////////////////////////////////
//
// If there is a change in selection, then display the new values
// in the value and label windows. 
//
extern "C" void ListEditor_ListCB(Widget w, XtPointer clientData,
				XtPointer callData)
{
    ListEditor *le = (ListEditor*)clientData;
    le->listCallback(w, callData);

}
void ListEditor::listCallback(Widget w, XtPointer cb)
{
    XmListCallbackStruct *list_data = (XmListCallbackStruct*)cb;
    char        *text;

    int position = list_data->item_position;

    if (position == this->listSelection)
    {
        /*
         * If the selected item has already been selected, deselect it.
         */
        XmListDeselectPos(this->valueList, position);
        this->listSelection = UNSELECTED_INDEX;
    }
    else
    {
        /*
         * Otherwise, set the current position in the list.
         */
        this->listSelection = position;

	// This seems to be necessary when deleteCallback() causes this 
	// callback to happen.
	// XmListSelectPos (this->valueList, position, False);

        /*
         * Copy the currently selected option value editor. 
         */
        XmStringGetLtoR(list_data->item, XmSTRING_DEFAULT_CHARSET, &text);
	this->updateValueEditor(text);
        XtFree(text);
    }
}
/////////////////////////////////////////////////////////////////////////////
extern "C" void ListEditor_AddCB(Widget w, XtPointer clientData,
				XtPointer callData)
{
    ListEditor *le = (ListEditor*)clientData;
    le->addCallback(w, callData) ;
}
//
// Add the values in the value/label windows to the scrolled list. 
//
void ListEditor::addCallback(Widget w, XtPointer cb)
{
    char position; 

    /*
     * If there is an item selected, add the new item just before it,
     * otherwise add it to the end of the list. 
     */
    if (this->listSelection != UNSELECTED_INDEX) {
	position = this->listSelection;
    } else {
        position = 0;
    }
    this->editListItem(NULL, position, FALSE, (position != 0));

    int count = this->getListItemCount();
    int visible;
    XtVaGetValues(this->valueList, XmNvisibleItemCount, &visible, NULL);
    if (count > visible) {
        this->getSelectedItemIndex();
	XmListSetBottomPos(this->valueList,position);	
    }
			
}
/////////////////////////////////////////////////////////////////////////////
extern "C" void ListEditor_DeleteCB(Widget w, XtPointer clientData,
				XtPointer callData)
{
    ListEditor *le = (ListEditor*)clientData;
    le->deleteCallback(w, callData);
}
//
// Remove the current selection from the scrolled list. 
//
void ListEditor::deleteCallback(Widget w, XtPointer callData)
{
    int count, position = this->listSelection;

    if (position != UNSELECTED_INDEX)
    {
	/*
	 * Delete the item in the selected position, taking care to adjust
	 * the selected position, if any, appropriately.
	 */
	XmListDeselectPos (this->valueList, position);
	XmListDeletePos (this->valueList, position);

        XtVaGetValues(this->valueList, XmNitemCount, &count, NULL);

	if (count > 0) {
	    if (position > count)
		position = count;

	    if (position > 0)
	    {
		this->listSelection = UNSELECTED_INDEX;
		XmListSelectPos (this->valueList, position, True);
	    } 
	} 
	else 
	{
	    this->listSelection = UNSELECTED_INDEX;
	}

#if 0
	//
	// Make sure the deleted item is visible
	//
	count = this->getItemCount();
	int visible;
	XtVaGetValues(this->valueList, XmNvisibleItemCount, &visible, NULL);
	if (count <= visible) {
	    selected = this->getSelectedItemIndex();
	    XmListSetTopItemPos(this->valueList,1);	
	}
#endif
			    
    }
}
//
// Display the options stored in the node. 
//
void ListEditor::replaceListItems(const char **items, int nitems)
{
    int prev_nitems, i;
    XmString *xmstrings;

    ASSERT(this->valueList);

#if 0
    /*
     * The following dummy code is to fix the vertical scrollbar problem.
     */
    Arg arg[4];
    int width, height;
    i = 0;
    XtSetArg(arg[i], XmNwidth,  &width); 	i++;
    XtSetArg(arg[i], XmNheight, &height); 	i++;
    XtGetValues(XtParent(this->valueList), arg, i);
    i = 0;
    XtSetArg(arg[i], XmNwidth,  width); 	i++;
    XtSetArg(arg[i], XmNheight, height); 	i++;
    XtSetValues(XtParent(this->valueList), arg, i);
#else
#if (XmVersion >= 1001)
    /*
     * Unmanage and re-manage the vertical scrollbar of the list
     * widget to force the list to re-size itself (in order to
     * work around a Motif1.1 List widget bug).
     */
    Widget vsb;
    XtVaGetValues(this->valueList,XmNverticalScrollBar, &vsb,
				   XmNvisibleItemCount, &prev_nitems,
					NULL);
    if (nitems > prev_nitems) {
	XtUnmanageChild(vsb);
	XtManageChild(vsb);
    }
#endif
#endif
    //
    // Delete all the current items.
    //
    XmListDeleteAllItems(this->valueList);
    this->listSelection = UNSELECTED_INDEX;

    //
    // Now reload the list. 
    //
    if (nitems != 0)
    {
	xmstrings = new XmString[nitems];
	for (i=0 ; i<nitems ; i++) {
	    xmstrings[i] = 
		XmStringCreateLtoR((char*)items[i],XmSTRING_DEFAULT_CHARSET);
	}
	XmListAddItems(this->valueList,xmstrings,nitems,1);
	while (nitems--)
	    XmStringFree(xmstrings[nitems]);
	delete xmstrings;
    } 
}

//
// Insert or replace the given text at the given position.
// If replace is FALSE...
//     If position is given as 0, then the item is added at the end of the
//     list.  If position is greater than 0, then the text is inserted at
//     the given position.
// If replace is TRUE...
//     If position is given as 0, then the item at the end of the
//     list is replaced.  If position is greater than 0, then the text 
//     is replaced at the given position.
// If text == NULL, the value is found with getValueEditorValue().
// If there is an error, no items are left selected.
//
void ListEditor::editListItem(const char *text, int position, 
					boolean replace, boolean select)
{
    XmString string;
 
    ASSERT(position >= 0);

    if (!text) {
	char *newtext= this->getValueEditorValue();
	if (!newtext) {
	    return;	
	} 
	string = XmStringCreate((char*)newtext, XmSTRING_DEFAULT_CHARSET);
	delete newtext;
    } else {
	string = XmStringCreate((char*)text, XmSTRING_DEFAULT_CHARSET);
    }

    if ((this->listSelection != UNSELECTED_INDEX) &&
	(this->listSelection != position)) {
	XmListDeselectAllItems(this->valueList);
	this->listSelection = UNSELECTED_INDEX;
    }

    if (replace) {
	XmListReplaceItemsPos(this->valueList,&string,1,position);
    } else {
        XmListAddItem(this->valueList, string, position);
    }

    if (select) {
        XmListSelectPos (this->valueList, position, False);
	if (position == 0) {
	    this->listSelection = this->getListItemCount();
	} else {
	    this->listSelection = position ;
	}
    } else {
	this->listSelection = UNSELECTED_INDEX;
    }

    XmStringFree(string);
}

//
// Insert the given text at after the current selection or at the end of
// the list if there is no current selection. 
// If no argument is provide the value is found with getValueEditorValue().
//
void ListEditor::insertAndSelectListItem(const char *text)
{
    int position = this->listSelection;

    if (position != UNSELECTED_INDEX) {
	this->replaceAndSelectSelectedListItem(text);
    } else {
	this->editListItem(text,0, FALSE, TRUE);
    }

    
}
//
// Replace the currently selected list item with the given text and
// indicate the new list item as selected.
// If no argument is provide the value is found with getValueEditorValue().
//
void ListEditor::replaceAndSelectSelectedListItem(const char *text)
{
    int position = this->listSelection;

    ASSERT(position != UNSELECTED_INDEX);

    this->editListItem(text,position,TRUE, TRUE);
}


void ListEditor::createListEditor(Widget mainForm)
{

    /////////////////////////////////////////////////////////////
    // Create the widgets from left to right and top to bottom.//
    /////////////////////////////////////////////////////////////

    // This is so the delete/add buttons don't cause it to go away
    XtVaSetValues(mainForm,XmNautoUnmanage, False, NULL);

    this->listFrame = XtVaCreateManagedWidget("frame",
			xmFrameWidgetClass,     mainForm,
			XmNtopAttachment, 	XmATTACH_FORM,
			XmNleftAttachment,	XmATTACH_FORM,
			XmNrightAttachment,	XmATTACH_FORM,
			NULL);
 
    //
    // Create the list (items are added later).
    //
    this->valueList = this->createList(this->listFrame);

    //
    // The Add/Delete buttons and the value editor
    //
    this->createEditingPart(mainForm);
}

//
// Build the list portion of the ListEditor (with callbacks).
//
Widget ListEditor::createList(Widget frame)
{
    //
    // The scrolled list of options ( list items are added later) 
    //

    Widget valueList = XmCreateScrolledList(frame,"listEditorList",NULL,0);

    XtVaSetValues(valueList, XmNselectionPolicy, XmBROWSE_SELECT, NULL);

    XtAddCallback(valueList, XmNbrowseSelectionCallback,
	(XtCallbackProc)ListEditor_ListCB, (XtPointer)this);
    XtAddCallback(valueList, XmNdefaultActionCallback,
	(XtCallbackProc)ListEditor_ListCB, (XtPointer)this);

    XtManageChild(valueList);

    return valueList;
}

//
// Build the value editor and the add/delete buttons 
//
void ListEditor::createEditingPart(Widget mainForm)
{
    //
    // The interactive value editing part 
    //
    Widget valueEditorTop = this->createValueEditor(mainForm); 

    //
    // The Add/Delete buttons
    //
    this->createEditingButtons(mainForm, valueEditorTop);

    XtVaSetValues(this->listFrame,
	XmNbottomAttachment, XmATTACH_WIDGET,
	XmNbottomWidget,     this->addButton,
	NULL);
}
//
// Build the add/delete buttons and attach them to the form and
// the list frame.
//
void ListEditor::createEditingButtons(Widget mainForm, Widget above)
{

    this->addButton = XtVaCreateManagedWidget("addButton",
			xmPushButtonWidgetClass , mainForm,
			XmNbottomAttachment, 	XmATTACH_WIDGET,
			XmNbottomWidget, 	above,
			XmNrightAttachment,	XmATTACH_POSITION,
			XmNleftAttachment,	XmATTACH_FORM,
			XmNleftOffset,		10,
			XmNrightOffset,		0,
			XmNrightPosition,	40,
			NULL); 


    XtAddCallback(this->addButton, XmNactivateCallback,
	(XtCallbackProc)ListEditor_AddCB, (XtPointer)this);

    //
    // The Delete button
    //
    this->deleteButton = XtVaCreateManagedWidget("deleteButton",
			xmPushButtonWidgetClass,mainForm,
			XmNbottomAttachment, 	XmATTACH_WIDGET,
			XmNbottomWidget, 	above,
			XmNleftAttachment, 	XmATTACH_POSITION,
			XmNrightAttachment, 	XmATTACH_FORM,
    			XmNrightOffset, 	10,
    			XmNleftOffset, 		0,
    			XmNleftPosition,	60,
			NULL); 

    XtAddCallback(this->deleteButton, XmNactivateCallback,
	(XtCallbackProc)ListEditor_DeleteCB, (XtPointer)this);

}
