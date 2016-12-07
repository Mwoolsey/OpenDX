/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#include <Xm/Xm.h>
#include <Xm/Form.h>

#include "Application.h"
#include "ValueListNode.h"
#include "ValueListInteractor.h"
#include "ValueListInstance.h"
#include "SetAttrDialog.h"
#include "DXValue.h"
#include "List.h"
#include "ListIterator.h"
//#include "../widgets/WorkspaceW.h"
//#include "../widgets/XmDX.h"
//#include "ControlPanel.h"
#include "ErrorDialogManager.h"
//#include "WarningDialogManager.h"

boolean ValueListInteractor::ClassInitialized = FALSE;

#define EMPTY_LIST_DISPLAY  "NULL" // Displayed in interactor as empty list
#define EMPTY_LIST_VALUE    "NULL" // Value set in parameter as empty list

String ValueListInteractor::DefaultResources[] =
{
     ".AllowResizing:           True",
     "*Offset:		        5",
     "*pinTopBottom:		True",
     "*pinLeftRight:		True",
     "*wwLeftOffset:		0",
     "*wwRightOffset:		0",
     "*wwTopOffset:		0",
     "*wwBottomOffset:		0",
     NUL(char*)
};

ValueListInteractor::ValueListInteractor(const char * name, 
				InteractorInstance *ii) : 
				ValueInteractor(name, ii) , ListEditor(name)
{ 
    this->listForm = NULL;
}

ValueListInteractor::~ValueListInteractor()
{
}
//
// Allocate the interactor class and widget tree.
//
Interactor *ValueListInteractor::AllocateInteractor(const char *name,
					InteractorInstance *ii)
{
    ValueListInteractor *i = new ValueListInteractor(name,ii);
    return (Interactor*)i;
}

 
////////////////////////////////////////////////////////////////////////////////
///////////////////////////// ValueInteractor part /////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void ValueListInteractor::initialize()
{
    //
    // Initialize default resources (once only).
    //
    if (NOT ValueListInteractor::ClassInitialized)
    {
        ASSERT(theApplication);
        this->setDefaultResources(theApplication->getRootWidget(),
                                  ValueListInteractor::DefaultResources);
        this->setDefaultResources(theApplication->getRootWidget(),
                                  ListEditor::DefaultResources);
        this->setDefaultResources(theApplication->getRootWidget(),
                                  Interactor::DefaultResources);
        ValueListInteractor::ClassInitialized = TRUE;
    }
}

#if 0
//
// Get the current text value. For StringList and ValueList this is the
// currently selected list item. 
//
const char *ValueInteractor::getCurrentTextValue()
{
    static char  
    Node *n = this->getNode();
    if (pos = this->getSelectedItemIndex())
        this->getListItem(pos);
}
#endif


Widget ValueListInteractor::createInteractivePart(Widget form)
{
    if (!ValueListInteractor::ClassInitialized)
	this->initialize();

    //
    // Build a form for the list editor to reside in.
    //
    this->listForm =  form;

    //
    // Now ask the list editor to build itself. 
    //
    this->createListEditor(this->listForm);
   
    return this->listForm; 

}
void ValueListInteractor::completeInteractivePart()
{
int width, height;
InteractorInstance *ii = this->interactorInstance;

    this->passEvents(this->listForm, TRUE);
    this->ValueInteractor::completeInteractivePart();
    ii->getXYSize (&width, &height);
    if (!width && !height) {
//	this->setXYSize (200,200);
    }
}
//
// Update the displayed values for this interactor (i.e. the list).
//
void ValueListInteractor::updateDisplayedInteractorValue()
{
    char	*s, **items = NULL;
    int 	count, index;
    ValueListNode *vln = (ValueListNode*)this->interactorInstance->getNode();
    Type type = vln->getTheCurrentOutputType(1);

    const char *val = vln->getOutputValueString(1); 	// Get the list itself
    index = -1;
    count = 0;
    while ( (s = DXValue::NextListItem(val, &index, type, NULL)) ) {
	if ((count & 15) == 0)
	    items = (char**)REALLOC(items,(count + 16)*sizeof(char*));
	items[count++] = s;
    }
    if (count == 0) {
	items = (char**)REALLOC(items,sizeof(char*));
	items[count++] = DuplicateString(EMPTY_LIST_DISPLAY);
    }
    this->replaceListItems((const char**)items,count);

    while (count--)
	FREE((void*)items[count]);

    FREE((void*)items);
}
//
// ValueLists don't have any attributes
//
void ValueListInteractor::handleInteractivePartStateChange(
					InteractorInstance *src_ii,
					boolean major_change)
{
    this->updateDisplayedInteractorValue();
}
////////////////////////////////////////////////////////////////////////////////
///////////////////////////// ListEditor part //////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

//
// Build the interacive value editing part.
// This probably called by createEditingPart().
//
Widget ValueListInteractor::createValueEditor(Widget mainForm)
{
    Widget w = this->ValueInteractor::createTextEditor(mainForm);

    ASSERT(w);

    XtVaSetValues(w, 
	XmNtopAttachment, 	XmATTACH_NONE, 
	XmNleftAttachment,	XmATTACH_FORM,
	XmNrightAttachment, 	XmATTACH_FORM,
	XmNbottomAttachment,	XmATTACH_FORM,
	XmNbottomOffset,	0,
	NULL);

    this->installValueEditorCallback(w,XmNactivateCallback);

    return w;
}

//
// Called by this->listCallback() when a new item is selected in the list.
// This should update the the values displayed in the editor to match
// the values selected (i.e. listItem).
//
void ValueListInteractor::updateValueEditor(const char *listItem)
{
     this->installNewText(listItem); 
}

//
// Determine if the text in the ValueEditor can be added to the current
// list of items.   If the current list is size is 1 and the item is
// selected, then the text can match any of the outputs, otherwise it
// must match the item type of current list of items. 
// Returns NULL if it does not type match, otherwise a string that must
// be freed by the caller.  The returned string may be different from the
// valueEditor text if the text was coerced to the expected type.
// An error message is issued if the text is not valid.
//
char *ValueListInteractor::verifyValueEditorText()
{
    int itemCount = this->getListItemCount();
    InteractorNode *node= (InteractorNode*)this->interactorInstance->getNode();
    Type itemType;
    List typeList;

    if ((itemCount == 1) && (this->getSelectedItemIndex() == 0)) {
	const char* const *types = node->getOutputTypeStrings(1);
	int j;
	ASSERT(types);
	for (j=0 ; types[j] ; j++) {
	    itemType = DXType::StringToType(types[j]) & DXType::ListTypeMask; 
	    if (itemType != DXType::UndefinedType)
		typeList.appendElement((void*)itemType);
	}	
    } else {
	itemType = node->getTheCurrentOutputType(1) & DXType::ListTypeMask;
	typeList.appendElement((void*)itemType);
    }
    ASSERT(typeList.getSize() > 0);

    //
    // Make sure the value entered is valid value
    //
    ListIterator i(typeList);
    char *s = this->getDisplayedText(); 
    boolean matched = FALSE;
    while ( (itemType = (Type)i.getNext()) ) {
	if (!DXValue::IsValidValue(s,itemType)) {
	    char *news;
	    if ( (news = DXValue::CoerceValue(s,itemType)) ) {
		if (s) delete s;
		s = news;
	    	matched = TRUE;
		break;
	    } 
	} else {
	    matched = TRUE;
	    break;
	}
    }
    if (!matched) {
	if (typeList.getSize() == 1) {
    	    itemType = (Type)typeList.getElement(1); 
	    ErrorMessage("'%s' is not a valid %s", 
			s,DXType::TypeToString(itemType));
	}
	else
	    ErrorMessage("'%s' is not a valid type", s);
	
	s = NULL;
    } 
    return s;

}
//
// Get the value in the value editor that should be added to the list.
// The returned string must be freed by the caller.
// Returns NULL if the value in the value editor is not valid and
// can issue an error message in that case.
//
char *ValueListInteractor::getValueEditorValue()
{
    char *s;
    //
    // Make sure the value entered is valid value
    // We do this here as well as in valueEditorCallback, because the
    // user can use the Add button without hitting return in the text
    // window, thereby bypassing valueEditorCallback().
    //
    if ( (s = this->verifyValueEditorText()) ) 
	this->installNewText(s);
    return s;
}
//
// Read the displayed values and set the output from them.
//
boolean ValueListInteractor::updateOutputFromDisplay()
{
    InteractorNode *node = (InteractorNode*)this->interactorInstance->getNode();
    char *s = NULL; 
    boolean r = TRUE;

    //
    // get the current list value. 
    //
    switch (this->getListItemCount()) {
	case 0: s = NULL;
		break;
	case 1:
		s = this->getListItem(1);
		if (s && EqualString(s,EMPTY_LIST_DISPLAY)) {
		    delete s;
		    s = NULL;
		    break;
		}
		if (s) delete s;
	default:
		s = this->buildDXList();
		break;
    }

 
    if (!s) {
	if (node->setOutputValue(1,EMPTY_LIST_VALUE) == DXType::UndefinedType)
	    r = FALSE;
    } else {
	if (node->setOutputValue(1,s) == DXType::UndefinedType)
	    r = FALSE;
    }
    if (s) delete s;
    return r;
}

//
// Before calling the superclass method, we check and massage the entered 
// value to make sure it matches the output type for this module.
//
void ValueListInteractor::valueEditorCallback(Widget w, XtPointer cb)
{
    char *s;
    //
    // Make sure the value entered is valid value
    //
    if ( (s = this->verifyValueEditorText()) ) 
	this->installNewText(s);

    this->ListEditor::valueEditorCallback(w,cb);

    if (s) {
	delete s;
        this->updateOutputFromDisplay();
    }
}
  
//
// After calling the superclass method to update the list, we set the 
// current output to a DX style list containing the listed items. 
//
void ValueListInteractor::addCallback(Widget w, XtPointer cb)
{
    const char *item1 = this->getListItem(1);
    char *valueText = this->getDisplayedText();

    //
    // If the current value text is the empty list indicator, ignore this.
    //
    if (EqualString(valueText,EMPTY_LIST_DISPLAY)) {
	delete valueText;
	return;
    }
    delete valueText;
 
    // 
    // If the list is displayed as empty, clear it.
    //
    if ((this->getListItemCount() == 1) && 
	EqualString(item1,EMPTY_LIST_DISPLAY)) {
	this->replaceListItems(NULL,0);
    }

     
    this->ListEditor::addCallback(w,cb);
    int position = this->getSelectedItemIndex();
    this->updateOutputFromDisplay();

    if (position)
	this->setSelectedItemIndex(position);

}
//
// After calling the superclass method to update the list, we set the 
// current output to a DX style list containing the listed items. 
//
void ValueListInteractor::deleteCallback(Widget w, XtPointer cb)
{
    // 
    // If the list is displayed as empty, ignore callback.
    //
    if (this->getListItemCount() == 1)  {
	char *s = this->getListItem(1);
	if (s && EqualString(s,EMPTY_LIST_DISPLAY)) {
	    delete s;
	    return;
	}
	if (s) delete s;
    }

    this->ListEditor::deleteCallback(w,cb);
    int position = this->getSelectedItemIndex();

    this->updateOutputFromDisplay();

    if (position)
	this->setSelectedItemIndex(position);
}
