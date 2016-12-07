/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#ifndef _ListEditor_h
#define _ListEditor_h


#include <Xm/Xm.h>

#include "Base.h"

//
// Class name definition:
//
#define ClassListEditor	"ListEditor"

//
// XtCallbackProc (*CB), XtEventHandler (*EH) and XtActionProc (*AP)
// DialogCallback (*DCB) functions for this and derived classes
//
extern "C" void ListEditor_ValueEditorCB(Widget, XtPointer, XtPointer);
extern "C" void ListEditor_ListCB(Widget, XtPointer, XtPointer);
extern "C" void ListEditor_DeleteCB(Widget, XtPointer, XtPointer);
extern "C" void ListEditor_AddCB(Widget, XtPointer, XtPointer);

//
// Referenced Classes
//

//
// ListEditor class definition:
//				
class ListEditor : virtual public Base 
{
  private:
    //
    // Private member data:
    //
    static boolean ClassInitialized;

    friend void ListEditor_AddCB(Widget	 widget,
		         XtPointer clientData,
		         XtPointer callData);
    friend void ListEditor_DeleteCB(Widget	 widget,
		         XtPointer clientData,
		         XtPointer callData);
    friend void ListEditor_ListCB(Widget	 widget,
		         XtPointer clientData,
		         XtPointer callData);

    friend void ListEditor_ValueEditorCB(Widget	 widget,
		         XtPointer clientData,
		         XtPointer callData);
  protected:
    //
    // Protected member data:
    //
    Widget	listFrame;
    Widget 	valueList;
    Widget	addButton, deleteButton;
    int		listSelection;	// Currently selected item in option list

    static String DefaultResources[];


    //
    // Install the callback for the give widget.
    // This is provided as a method so that 'this' is the right value (i.e.
    // when the callback is installed from a derived class the value 'this'
    // can be a pointer to the derived class and not a ListEditor).
    //
    void installValueEditorCallback(Widget w, String callback_name);


    //
    // By default this function creates the interactive value part
    // and the add/delete buttons in the given form. 
    //
    virtual void createEditingPart(Widget mainForm);


    //
    // By default this function add the add/delete buttons form 
    // containing the value list.  And is probably called by 
    // createEditingPart().
    // "above" is the widget above which these buttons will be created.
    //
    virtual void createEditingButtons(Widget mainForm, Widget above);

    //
    // Build the list portion of the ListEditor.
    // The result is used to set this->valueList.
    //
    Widget createList(Widget frame);

    //
    // Build the interacive value editing part.  
    // This probably called by createEditingPart().
    //
    virtual Widget createValueEditor(Widget mainForm) = 0;

    //
    // Called by this->listCallback() when a new item is selected in the list.
    // This should update the the values displayed in the editor to match
    // the values selected (i.e. listItem).
    //
    virtual void updateValueEditor(const char *listItem) = 0;

    //
    // Get the value in the value editor that should be added to the list.
    // The returned string must be freed by the caller.
    // Returns NULL if the value in the value editor is not valid and
    // can issue an error message in that case.
    //
    virtual char *getValueEditorValue() = 0;

    // 
    // Install a new list of items in the valueList. 
    //
    void replaceListItems(const char **items, int n); 

    //
    // Replace the currently selected list item with the given text and 
    // indicate the new list item as selected.
    // If no argument is provide the value is found with getValueEditorValue(). 
    //
    void replaceAndSelectSelectedListItem(const char *text = NULL);

    //
    // Insert the given text at after the current selection or at the end of
    // the list if there is no current selection.
    // If no argument is provide the value is found with getValueEditorValue().
    //
    void insertAndSelectListItem(const char *text);

    //
    // Insert or replace the given text at the given position.
    // The current selection (if there is one) is deselected.
    // If replace is FALSE...
    //     If position is given as 0, then the item is added at the end of the 
    //	   list.  If position is greater than 0, then the text is inserted at 
    //	   the given position.
    // If replace is TRUE...
    //     If position is given as 0, then the item at the end of the 
    //	   list is replaced.  If position is greater than 0, then the text 
    //     is replaces at the given position.
    // If text == NULL, the value is found with getValueEditorValue().
    // If there is an error, no items are left selected.
    //
    void editListItem(const char *text, int position, 
				boolean replace, boolean select);


    virtual void addCallback(Widget w, XtPointer callData);
    virtual void deleteCallback(Widget w, XtPointer callData);
    virtual void listCallback(Widget w, XtPointer callData);
    virtual void valueEditorCallback(Widget w, XtPointer callData);

  public:
    //
    // Constructor:
    //
    ListEditor(const char *name);

    //
    // Destructor:
    //
    ~ListEditor();

    //
    // Get the number of items in the list.
    //
    int getListItemCount();


    //
    // Get the currently selected item.
    // Returns zero if no item is selected.
    //
    int getSelectedItemIndex();

    //
    // Set the currently selected item.
    // Return FALSE if index is out of range (i.e 0 < index <= list item count).
    //
    boolean setSelectedItemIndex(int index);


    //
    // Build a DX style list out of the current list items.
    // The returned string must be deleted by the caller.
    // If the list is empty, NULL is returned.
    //
    char *buildDXList();



    //
    // Get the text for the index'th item (1 based indexing) 
    // The returned string must be freed by the caller.
    //
    char *getListItem(int index);
 

    //
    // Builds the list and add/delete buttons, but does not set this->root.
    // We don't set this->root as this class is intended to be used in
    // multiply inherited classes.  The list editor is expected to be 
    // created with a parent that is an XmForm.
    //
    virtual void createListEditor(Widget mainForm);

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassListEditor;
    }
};


#endif // _ListEditor_h
