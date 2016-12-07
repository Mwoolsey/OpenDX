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

#include <sys/stat.h>


#include "DXStrings.h"
#include "lex.h"
#include "CreateMacroDialog.h"

#include "DXApplication.h"
#include "Network.h"
#include "QuestionDialogManager.h"
#include "ErrorDialogManager.h"
#include "TextFile.h"
#include "EditorWindow.h"


#include "NodeDefinition.h"
#include "DictionaryIterator.h"
#include "XmUtility.h"

boolean CreateMacroDialog::ClassInitialized = FALSE;
String CreateMacroDialog::DefaultResources[] =
{
//    "*nameLabel.labelString:            Name:",
//    "*nameLabel.foreground:             SteelBlue",
//    "*categoryLabel.labelString:        Category:",
//   "*categoryLabel.foreground:         SteelBlue",
//  "*descriptionLabel.labelString:     Description:",
//  "*descriptionLabel.foreground:      SteelBlue",
    "*filenameLabel.labelString:  	Filename:",
    "*filenameLabel.foreground:      	SteelBlue",
    "*dialogTitle:			Create Macro...",
//    "*fsdbutton.labelString:  		...",
    "*okButton.labelString:             OK",
//    "*okButton.width:                   70",
//    "*cancelButton.labelString:         Cancel",
//    "*cancelButton.width:               70",
    NULL
};


CreateMacroDialog::CreateMacroDialog(Widget parent,
				     EditorWindow *e):
    SetMacroNameDialog("createMacroDialog", parent, e->getNetwork())
{
    this->editor = e;
    if (NOT CreateMacroDialog::ClassInitialized)
    {
        CreateMacroDialog::ClassInitialized = TRUE;
	this->installDefaultResources(theApplication->getRootWidget());
    }
    this->textFile = new TextFile();
}

//
// Install the default resources for this class.
//
void CreateMacroDialog::installDefaultResources(Widget  baseWidget)
{
    this->setDefaultResources(baseWidget, CreateMacroDialog::DefaultResources);
    this->SetMacroNameDialog::installDefaultResources( baseWidget);
}

CreateMacroDialog::~CreateMacroDialog()
{
    if (this->textFile)
       delete this->textFile; 
}

void CreateMacroDialog::ConfirmationCancel(void *data)
{
}

void CreateMacroDialog::ConfirmationOk(void *data)
{

    CreateMacroDialog *cmd = (CreateMacroDialog*)data;
    cmd->createMacro();
    
}


boolean CreateMacroDialog::okCallback(Dialog *d)
{

    /*CreateMacroDialog *cmd = (CreateMacroDialog*)d;*/

    char *name = SetMacroNameDialog::GetTextWidgetToken(this->macroName); 

    if (!this->verifyMacroName(name, TRUE)) {
	return FALSE;
	delete name;
    }

    char *filename = this->textFile->getText();
    if (IsBlankString(filename)) {
	ModalErrorMessage(this->getRootWidget(), "A file name must be given");
	delete name;
	XtFree(filename);
	return FALSE;
    }

    char *newname = Network::FilenameToNetname(filename);
    XtFree(filename);

    struct STATSTRUCT buffer;
	if(STATFUNC(newname, &buffer) == 0) {

    	theQuestionDialogManager->modalPost(
				this->parent,
				"Do you want to overwrite an existing file?",
				"Overwrite existing file",
				(void *)this,
				CreateMacroDialog::ConfirmationOk,
				CreateMacroDialog::ConfirmationCancel,
				NULL);
	delete name;
        delete newname; 

	return TRUE;
    }

    boolean worked = this->createMacro();

    delete name;
    delete newname; 

    return worked;

}



boolean CreateMacroDialog::createMacro()
{
   

    char *filename = this->textFile->getText();    
    char *name = SetMacroNameDialog::GetTextWidgetToken(this->macroName);
    char *cat = SetMacroNameDialog::GetTextWidgetToken(this->category);
    char *desc = XmTextGetString(this->description);

    boolean worked = this->editor->macroifySelectedNodes(name, cat, desc,
								 filename);
    delete name;
    delete cat;
    XtFree(desc);
    XtFree(filename);

    return worked;
}


void CreateMacroDialog::setFileName(const char *filename)
{
    XtVaSetValues(this->filename,XmNvalue,filename,NULL);

    //
    //  Right justify the text
    // 

    int len = STRLEN(filename);
    XmTextShowPosition(this->filename,len);
    XmTextSetInsertionPosition(this->filename,len);

}   



Widget CreateMacroDialog::createDialog(Widget parent)
{
    Arg arg[10];
    XtSetArg(arg[0], XmNdialogStyle, XmDIALOG_PRIMARY_APPLICATION_MODAL);
    XtSetArg(arg[1], XmNautoUnmanage, False);
//    Widget dialog = XmCreateFormDialog(parent, this->name, arg, 2);
    Widget dialog = this->CreateMainForm(parent, this->name, arg, 2);

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


    Widget filenameLabel = XtVaCreateManagedWidget(
        "filenameLabel", xmLabelWidgetClass, dialog,
        XmNtopAttachment   , XmATTACH_WIDGET,
        XmNtopWidget       , this->description,
        XmNtopOffset       , 10,
        XmNleftAttachment  , XmATTACH_FORM,
        XmNleftOffset      , 5,
        XmNrightAttachment , XmATTACH_POSITION,
        XmNrightPosition   , 25,
        XmNalignment       , XmALIGNMENT_BEGINNING,
	NULL);
    
    this->textFile->createTextFile(dialog);

    
    XtVaSetValues(this->textFile->getRootWidget(),
	XmNtopAttachment   , XmATTACH_WIDGET,
	XmNtopWidget	   , this->description,
	XmNtopOffset	   , 10,
	XmNleftAttachment  , XmATTACH_POSITION,
	XmNleftPosition	   , 25,
	XmNrightAttachment , XmATTACH_FORM,
	XmNrightOffset	   , 5,
	NULL); 
	
    
    Widget separator = XtVaCreateManagedWidget(
        "separator", xmSeparatorWidgetClass, dialog,
        XmNtopAttachment   , XmATTACH_WIDGET,
        XmNtopWidget       , this->textFile->getRootWidget(),
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

    this->textFile->enableCallbacks(TRUE);

    return dialog;
}

void CreateMacroDialog::manage()
{
    XmTextSetString(this->category, "Macros");
    XmTextSetString(this->macroName, "");
    XmTextSetString(this->description, "new macro");

    this->Dialog::manage();

    XmProcessTraversal(this->macroName, XmTRAVERSE_CURRENT);
    XmProcessTraversal(this->macroName, XmTRAVERSE_CURRENT);
}









