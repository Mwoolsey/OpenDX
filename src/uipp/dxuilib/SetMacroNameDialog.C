/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#include "Xm/Form.h"
#include "Xm/Label.h"
#include "Xm/PushB.h"
#include "Xm/Separator.h"
#include "Xm/Text.h"

#include "DXStrings.h"
#include "lex.h"
#include "SetMacroNameDialog.h"

#include "DXApplication.h"
#include "Network.h"
#include "ErrorDialogManager.h"
#include "Dialog.h"

#include "NodeDefinition.h"
#include "DictionaryIterator.h"
#include "XmUtility.h"

boolean SetMacroNameDialog::ClassInitialized = FALSE;
String SetMacroNameDialog::DefaultResources[] =
{
    "*nameLabel.labelString:            Name:",
    "*nameLabel.foreground:             SteelBlue",
    "*categoryLabel.labelString:        Category:",
    "*categoryLabel.foreground:         SteelBlue",
    "*descriptionLabel.labelString:     Description:",
    "*descriptionLabel.foreground:      SteelBlue",
    "*okButton.labelString:             Set",
    "*okButton.width:                   70",
    "*cancelButton.labelString:         Cancel",
    "*cancelButton.width:               70",
    NULL
};


SetMacroNameDialog::SetMacroNameDialog(const char *name, Widget parent, Network *n):
    Dialog(name, parent)
{
    this->network = n;

}

SetMacroNameDialog::SetMacroNameDialog(Widget parent, Network *n):
    Dialog("setMacroNameDialog", parent)
{
    this->network = n;
    if (NOT SetMacroNameDialog::ClassInitialized)
    {
        SetMacroNameDialog::ClassInitialized = TRUE;
	this->installDefaultResources(theApplication->getRootWidget());
    }
}

//
// Install the default resources for this class.
//
void SetMacroNameDialog::installDefaultResources(Widget  baseWidget)
{
    this->setDefaultResources(baseWidget, SetMacroNameDialog::DefaultResources);
    this->Dialog::installDefaultResources( baseWidget);
}

SetMacroNameDialog::~SetMacroNameDialog()
{
}


//
// Check the given name to see if it is a valid macro name.
// If realMacro is TRUE, then don't allow the name to be 'main'.
// If it is return TRUE, othewise return FALSE and issue an error
// message.
//
boolean SetMacroNameDialog::verifyMacroName(const char *name, boolean realMacro)
{
    int i = 0;
    if (realMacro && IsToken(name,"main",i) &&
               (!name[i] || IsWhiteSpace(name,i))) 
    {
	ModalErrorMessage(this->getRootWidget(),
		"Macro name cannot be \"main\".");
	return FALSE;
    }
    i = 0;
    if (!IsRestrictedIdentifier(name, i) )
    {
	ModalErrorMessage(this->getRootWidget(),
		"Macro name `%s' must start with a letter and consist "
		     "of only letters and numbers", name);
	return FALSE;
    }
    SkipWhiteSpace(name, i);
    if (name[i] != '\0')
    {
	ModalErrorMessage(this->getRootWidget(),
		"Macro name `%s' must start with a letter and consist "
		     "of only letters and numbers", name);
	return FALSE;
    }

    if (IsReservedScriptingWord(name)) {
        ErrorMessage("Macro name \"%s\" is a reserved word", name);
        return FALSE;
    }


    NodeDefinition *nd;
    DictionaryIterator di(*theNodeDefinitionDictionary);
    Symbol nameSym = theSymbolManager->registerSymbol(name);
    while ( (nd = (NodeDefinition*)di.getNextDefinition()) )
    {
	if (nd->getNameSymbol() == nameSym)
	{ 
            if (!nd->isDerivedFromMacro())
            {
                ModalErrorMessage(this->getRootWidget(),
                    "Macro name `%s' is the name of a Data Explorer module.\n"
                    "Macro names must not be the same as a module name.",
                         name);
                return FALSE;
            } 
	    else if (this->network->getDefinition() != 
				(MacroDefinition*)nd) // && is Macro 
	    {
                ModalErrorMessage(this->getRootWidget(),
                    "Macro name `%s' is the name of an already loaded macro.",
                         name);
                return FALSE;
            }
	}
    }

    return TRUE;
}

//
// Get the text from a text widget and clip off the leading and
// trailing white space.
// The return string must be deleted by the caller.
//
char *SetMacroNameDialog::GetTextWidgetToken(Widget textWidget)
{
    char *name = XmTextGetString(textWidget);
    ASSERT(name);
    int i,len = STRLEN(name);

    for (i=len-1 ; i>=0 ; i--) {
	if (IsWhiteSpace(name,i))
	    name[i] = '\0';
	else
	    break;
    }

    i=0; SkipWhiteSpace(name,i);
     
    char *s = DuplicateString(name+i);
    XtFree(name);
    return s;

}

boolean SetMacroNameDialog::okCallback(Dialog *d)
{
    char *name = SetMacroNameDialog::GetTextWidgetToken(this->macroName);

    //
    // gresh889.  Set the network's name only if we're really changing it.
    // This allows the user to change the cat or descr by themselves.
    //
    if (!EqualString(name, this->network->getNameString())) {
	if (!this->verifyMacroName(name, this->network->isMacro())) {
	    delete name;
	    return FALSE;
	}

	this->network->setName(name);
    }
    delete name;

    char *cat = SetMacroNameDialog::GetTextWidgetToken(this->category);
    this->network->setCategory(cat);
    delete cat;
    char *desc = XmTextGetString(this->description);
    this->network->setDescription(desc);
    XtFree(desc);

    return TRUE;
}

Widget SetMacroNameDialog::createDialog(Widget parent)
{
    Arg arg[10];
    XtSetArg(arg[0], XmNdialogStyle, XmDIALOG_PRIMARY_APPLICATION_MODAL);
    XtSetArg(arg[1], XmNautoUnmanage, False);
//    Widget dialog = XmCreateFormDialog(parent, this->name, arg, 2);
    Widget dialog = this->CreateMainForm(parent, this->name, arg, 2);

    XtVaSetValues(XtParent(dialog), XmNtitle, "Name...", NULL);

    Widget nameLabel = XtVaCreateManagedWidget(
	"nameLabel", xmLabelWidgetClass, dialog,
        XmNtopAttachment   , XmATTACH_FORM,
        XmNtopOffset       , 10,
        XmNleftAttachment  , XmATTACH_FORM,
        XmNleftOffset      , 5,
        XmNrightAttachment , XmATTACH_POSITION,
        XmNrightPosition   , 25,
        XmNalignment       , XmALIGNMENT_BEGINNING,
	NULL);

    this->macroName = XtVaCreateManagedWidget(
	"name", xmTextWidgetClass, dialog,
        XmNtopAttachment   , XmATTACH_FORM,
        XmNtopOffset       , 10,
        XmNleftAttachment  , XmATTACH_POSITION,
        XmNleftPosition    , 25,
        XmNrightAttachment , XmATTACH_FORM,
        XmNrightOffset     , 5,

        XmNeditMode        , XmSINGLE_LINE_EDIT,
        XmNeditable        , True,
	NULL);

    Widget categoryLabel = XtVaCreateManagedWidget(
        "categoryLabel", xmLabelWidgetClass, dialog,
        XmNtopAttachment   , XmATTACH_WIDGET,
        XmNtopWidget       , this->macroName,
        XmNtopOffset       , 10,
        XmNleftAttachment  , XmATTACH_FORM,
        XmNleftOffset      , 5,
        XmNrightAttachment , XmATTACH_POSITION,
        XmNrightPosition   , 25,
        XmNalignment       , XmALIGNMENT_BEGINNING,
	NULL);

    this->category = XtVaCreateManagedWidget(
        "category", xmTextWidgetClass, dialog,
        XmNtopAttachment   , XmATTACH_WIDGET,
        XmNtopWidget       , this->macroName,
        XmNtopOffset       , 10,
        XmNleftAttachment  , XmATTACH_POSITION,
        XmNleftPosition    , 25,
        XmNrightAttachment , XmATTACH_FORM,
        XmNrightOffset     , 5,
        XmNeditMode        , XmSINGLE_LINE_EDIT,
        XmNeditable        , True,
	NULL);

    Widget descriptionLabel = XtVaCreateManagedWidget(
        "descriptionLabel", xmLabelWidgetClass, dialog,
        XmNtopAttachment   , XmATTACH_WIDGET,
        XmNtopWidget       , this->category,
        XmNtopOffset       , 10,
        XmNleftAttachment  , XmATTACH_FORM,
        XmNleftOffset      , 5,
        XmNrightAttachment , XmATTACH_POSITION,
        XmNrightPosition   , 25,
        XmNalignment       , XmALIGNMENT_BEGINNING,
	NULL);

    this->description = XtVaCreateManagedWidget(
        "description", xmTextWidgetClass, dialog,
        XmNtopAttachment   , XmATTACH_WIDGET,
        XmNtopWidget       , this->category,
        XmNtopOffset       , 10,
        XmNleftAttachment  , XmATTACH_POSITION,
        XmNleftPosition    , 25,
        XmNrightAttachment , XmATTACH_FORM,
        XmNrightOffset     , 5,
        XmNeditMode        , XmSINGLE_LINE_EDIT,
	NULL);

    Widget separator = XtVaCreateManagedWidget(
        "separator", xmSeparatorWidgetClass, dialog,
        XmNtopAttachment   , XmATTACH_WIDGET,
        XmNtopWidget       , this->description,
        XmNtopOffset       , 10,
        XmNleftAttachment  , XmATTACH_FORM,
        XmNleftOffset      , 2,
        XmNrightAttachment , XmATTACH_FORM,
        XmNrightOffset     , 2,
	NULL);

    this->ok = XtVaCreateManagedWidget(
        "okButton", xmPushButtonWidgetClass, dialog,
        XmNtopAttachment   , XmATTACH_WIDGET,
        XmNtopWidget       , separator,
        XmNtopOffset       , 10,
        XmNleftAttachment  , XmATTACH_FORM,
        XmNleftOffset      , 5,
        XmNbottomAttachment, XmATTACH_FORM,
        XmNbottomOffset    , 5,
	NULL);

    this->cancel = XtVaCreateManagedWidget(
        "cancelButton", xmPushButtonWidgetClass, dialog,
        XmNtopAttachment   , XmATTACH_WIDGET,
        XmNtopWidget       , separator,
        XmNtopOffset       , 10,
        XmNrightAttachment , XmATTACH_FORM,
        XmNrightOffset     , 5,
        XmNbottomAttachment, XmATTACH_FORM,
        XmNbottomOffset    , 5,
	NULL);

    return dialog;
}

void SetMacroNameDialog::manage()
{
    Network *n = this->network;
    char *cat = (char *)n->getCategoryString();

    if(cat)
    	XmTextSetString(this->category, cat);
    else
    	XmTextSetString(this->category, "Macros");

    XmTextSetString(this->macroName, (char *)n->getNameString());
    XmTextSetString(this->description, (char *)n->getDescriptionString());

    boolean sens = (this->network == theDXApplication->network);
    Pixel color = sens ? theDXApplication->getForeground()
		       : theDXApplication->getInsensitiveColor();
    SetTextSensitive(this->macroName, sens, color); 
    SetTextSensitive(this->category,  sens, color);
    SetTextSensitive(this->description,  sens, color);

    this->Dialog::manage();

    if(sens)
    {
   	XmProcessTraversal(this->description, XmTRAVERSE_CURRENT);
   	XmProcessTraversal(this->description, XmTRAVERSE_CURRENT);
    }
}
