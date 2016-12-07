/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#if defined(DXD_WIN) || defined(OS2)  //SMH get correct name for unlink 
# include <stdio.h>      
# define unlink _unlink
#else
# include <unistd.h>     // For unlink()
#endif

#include <Xm/Label.h>
#include <Xm/Text.h>
#include <Xm/ToggleB.h>
#include <Xm/PushB.h>
#include <Xm/Separator.h>

#include <sys/stat.h>

#include "DXStrings.h"
#include "DXApplication.h"
#include "Network.h"
#include "EditorWindow.h"
#include "PrintProgramDialog.h"
#include "PrintProgramFileDialog.h"
#include "../base/QuestionDialogManager.h"
#include "../base/ErrorDialogManager.h"
#include "../base/XmUtility.h"


boolean PrintProgramDialog::ClassInitialized = FALSE;


String PrintProgramDialog::DefaultResources[] =
{
    "*dialogTitle:     			Print Program...",
    "*fileNameLabel.labelString:	File name:",
    "*toFileToggle.labelString:		Print To File Only",
    "*toFileToggle.shadowThickness:	0",
    "*labelParamsToggle.labelString:	Label Set Inputs (if scale allows)",
    "*labelParamsToggle.shadowThickness:0",
    "*printerLabel.labelString:		PostScript Printer Name:",
    "*fileDialogButton.labelString:	...",
    "*okButton.labelString:		OK",	
    "*cancelButton.labelString:		Cancel",	
        NULL
};


PrintProgramDialog::PrintProgramDialog(EditorWindow *e) : 
                       Dialog("printProgramDialog", e->getRootWidget())
{
    this->editor = e;
    this->printProgramFileDialog = NULL;

    if (NOT PrintProgramDialog::ClassInitialized)
    {
        PrintProgramDialog::ClassInitialized = TRUE;
	this->installDefaultResources(theApplication->getRootWidget());
    }
}
PrintProgramDialog::~PrintProgramDialog()
{
    if (this->printProgramFileDialog)
	delete this->printProgramFileDialog; 
}

void PrintProgramDialog::ConfirmationCancel(void *data)
{
}
void PrintProgramDialog::ConfirmationOk(void *data)
{
    PrintProgramDialog *dialog = (PrintProgramDialog*)data;
    dialog->printProgram();

}

//
// Install the default resources for this class.
//
void PrintProgramDialog::installDefaultResources(Widget  baseWidget)
{
    this->setDefaultResources(baseWidget, PrintProgramDialog::DefaultResources);
    this->Dialog::installDefaultResources( baseWidget);
}
Widget PrintProgramDialog::createDialog(Widget parent)
{
    Arg arg[10];
    XtSetArg(arg[0], XmNdialogStyle, XmDIALOG_PRIMARY_APPLICATION_MODAL);
    XtSetArg(arg[1], XmNautoUnmanage, False);
    XtSetArg(arg[2], XmNwidth, 350);
    XtSetArg(arg[3], XmNheight, 186);
    Widget dialog = this->CreateMainForm(parent, this->name, arg, 4);

    Widget printerLabel = XtVaCreateManagedWidget(
        "printerLabel", xmLabelWidgetClass, dialog,
        XmNtopAttachment   , XmATTACH_FORM,
        XmNtopOffset       , 12,
        XmNleftAttachment  , XmATTACH_FORM,
        XmNleftOffset      , 5,
        NULL);

    this->printerName = XtVaCreateManagedWidget(
        "printerName", xmTextWidgetClass, dialog,
        XmNtopAttachment   , XmATTACH_FORM,
        XmNtopOffset       , 8,
        XmNleftAttachment  , XmATTACH_WIDGET,
        XmNleftWidget	   , printerLabel,
        XmNrightAttachment , XmATTACH_FORM,
        XmNrightOffset     , 5,
        XmNeditMode        , XmSINGLE_LINE_EDIT,
        XmNeditable        , True,
        NULL);
    XmTextSetString(this->printerName,"ps");



    this->fileSelectButton = XtVaCreateManagedWidget("fileDialogButton",
                        xmPushButtonWidgetClass,     dialog,
                        XmNrightAttachment,  XmATTACH_FORM,
                        XmNrightOffset,  5,
			XmNtopAttachment,   XmATTACH_WIDGET,
			XmNtopWidget    , this->printerName,
			XmNtopOffset    , 12,
                        NULL);

    Widget fileNameLabel = XtVaCreateManagedWidget(
        "fileNameLabel", xmLabelWidgetClass, dialog,
	XmNtopAttachment,   XmATTACH_WIDGET,
	XmNtopWidget    , this->printerName,
        XmNtopOffset    , 12,
        XmNleftAttachment  , XmATTACH_FORM, 
        XmNleftOffset      , 5,
        NULL);

    this->fileName = XtVaCreateManagedWidget(
        "fileName", xmTextWidgetClass, dialog,
	XmNtopAttachment,   XmATTACH_WIDGET,
	XmNtopWidget    , this->printerName,
        XmNtopOffset    , 8,
        XmNleftAttachment  , XmATTACH_WIDGET,
        XmNleftWidget	   , fileNameLabel,
        XmNleftOffset     , 10,
        XmNrightAttachment  , XmATTACH_WIDGET,
        XmNrightWidget	   , this->fileSelectButton,
        XmNrightOffset     , 10,
        XmNeditMode        , XmSINGLE_LINE_EDIT,
        XmNeditable        , True,
        NULL);

    this->toFileToggle = XtVaCreateManagedWidget(
        "toFileToggle", xmToggleButtonWidgetClass, dialog,
        XmNtopAttachment, XmATTACH_WIDGET,
        XmNtopWidget    , this->fileName,
        XmNtopOffset    , 10,
        XmNleftAttachment  , XmATTACH_FORM,
        XmNleftOffset      , 5,
        NULL);


    this->labelParamsToggle= XtVaCreateManagedWidget(
        "labelParamsToggle", xmToggleButtonWidgetClass, dialog,
        XmNtopAttachment, XmATTACH_WIDGET,
        XmNtopWidget    , this->toFileToggle,
        XmNtopOffset    , 10,
        XmNleftAttachment  , XmATTACH_FORM,
        XmNleftOffset      , 5,
        XmNset		   , True,
        NULL);

#if 0
    XtVaSetValues(this->fileSelectButton, 
			XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
			XmNtopWidget,     this->fileName,
			NULL); 
#endif
			
    //
    // The OK and CANCEL buttons.
    //
    this->ok =  XtVaCreateManagedWidget(
        "okButton", xmPushButtonWidgetClass, dialog,
                        XmNleftAttachment  , XmATTACH_FORM,
                        XmNleftOffset      , 5,
                        XmNbottomAttachment, XmATTACH_FORM,
                        XmNbottomOffset    , 10,
                        XmNrecomputeSize   , False,
                        XmNwidth           , 75,
                        NULL);


    this->cancel =  XtVaCreateManagedWidget(
        "cancelButton", xmPushButtonWidgetClass, dialog,
                        XmNrightAttachment , XmATTACH_FORM,
                        XmNrightOffset     , 5,
                        XmNbottomAttachment, XmATTACH_FORM,
                        XmNbottomOffset    , 10,
                        XmNrecomputeSize   , False,
                        XmNwidth           , 75,
                        NULL);

    Widget separator = XtVaCreateManagedWidget(
        "separator", xmSeparatorWidgetClass, dialog,
                        XmNbottomAttachment, XmATTACH_WIDGET,
                        XmNbottomWidget    , this->ok,
                        XmNbottomOffset    , 10,
                        XmNleftAttachment  , XmATTACH_FORM,
                        XmNleftOffset      , 2,
                        XmNrightAttachment , XmATTACH_FORM,
                        XmNrightOffset     , 2,
                        NULL);


    XtAddCallback(this->fileSelectButton,
                        XmNactivateCallback,
                        (XtCallbackProc)PrintProgramDialog_ButtonCB,
                        (XtPointer)this);

    XtAddCallback(this->toFileToggle,
                        XmNvalueChangedCallback,
                        (XtCallbackProc)PrintProgramDialog_ToFileToggleCB,
                        (XtPointer)this);


    return dialog;
}


//
// Set the name of the file as displayed in the fileName text widget.
// Text is always right justified.
//
void PrintProgramDialog::setFileName(const char *filename)
{
    XtVaSetValues(this->fileName, XmNvalue, filename, NULL);

    //
    // Right justify the text
    //
    int len = STRLEN(filename);
    XmTextShowPosition(this->fileName,len);
    XmTextSetInsertionPosition(this->fileName,len);

}
//
// Call the superclass method and then install a default file name
// based on the name of the network associated with our editor.
//
void PrintProgramDialog::manage()
{
   this->Dialog::manage();

   Network *n = this->editor->getNetwork();

   const char *file = n->getFileName();

   if (file) {
        char *fileA = GetFileBaseName(file,".net"); 
	char *fileB = new char[STRLEN(fileA) + 4];
	sprintf(fileB, "%s.ps",fileA);
	delete fileA;
	this->setFileName(fileB);
	delete fileB;
   }

   this->setSensitivity();
}

boolean PrintProgramDialog::okCallback(Dialog *d)
{


    if (this->isPrintingToFile()) {
	char *filename = XmTextGetString(this->fileName);
	if (IsBlankString(filename)) {
	    ModalErrorMessage(this->getRootWidget(),
			"A filename must be given");
	    XtFree(filename);
	    return FALSE;
	}

	struct STATSTRUCT buffer;

	if (STATFUNC(filename, &buffer) == 0) {
	    theQuestionDialogManager->modalPost(
		this->parent,
		"Do you want to overwrite an existing file?",
		"Overwrite existing file",
		(void *)this,
		PrintProgramDialog::ConfirmationOk,
		PrintProgramDialog::ConfirmationCancel,
		NULL);
	    XtFree(filename);
	    return TRUE;
	} 
	XtFree(filename);
    } 
    return this->printProgram();
}

boolean PrintProgramDialog::printProgram()
{
    boolean toFile = this->isPrintingToFile();
    char *filename;

    if (toFile) {
	//
	// Get the name of the file
	//
	char *p;
        filename = XmTextGetString(this->fileName);
#ifdef DXD_NON_UNIX_DIR_SEPARATOR
	for (int j=0; j<strlen(filename); j++) {
	    if (filename[j] == '\\')
		filename[j] = '/';
	}
#endif
	p = strrchr(filename,'/');
	if (p) {
	    p++;
	    if (!*p) {
		XtFree(filename);
		ModalErrorMessage(this->getRootWidget(),
				"A valid file name must be given.");
		return FALSE;
	    }
	}
#define FORCE_PS_EXT 0
#if FORCE_PS_EXT 	// Forces .ps extension
	p = (char*)strrstr(filename,".ps");
	if (!p || *(p+3)) {	// Check to see if '.ps' is last thing in name
	    p = new char[STRLEN(filename) + 3];
	    sprintf(p,"%s.ps",filename);
	} else {
	    p = DuplicateString(filename);
	}
	XtFree(filename);
	filename = p; 
#endif
		
    } else {
	//
	// Make a temporary filename 
	//
	filename = UniqueFilename(theDXApplication->getTmpDirectory());
	if (!filename)
	    filename = UniqueFilename("/usr/tmp");
	if (!filename) {
	    ModalErrorMessage(this->getRootWidget(),
				"Could not create temporary file");
	    return FALSE;
	}
    }

    //
    // Write the PostScript file
    //
    if (!this->editor->printVisualProgramAsPS(filename,
			8.5,11.0, this->isLabelingParams())) {
	unlink(filename);	
	goto error;
    }
   
    //
    // If printing, then send the (temporary) file to the printer. 
    //
    if (!toFile) {
        char *printer = XmTextGetString(this->printerName);
	char *cmd = new char[STRLEN(filename) + STRLEN(printer) + 128];
	sprintf(cmd,"(lpr -P%s %s; rm -f %s) &", printer, filename, filename);
	system(cmd);
	XtFree(printer);
	delete cmd;
    }

#if !FORCE_PS_EXT 
    if (toFile)
	XtFree(filename);
    else
#endif
	delete filename;
    return TRUE;

error:
#if !FORCE_PS_EXT 
    if (toFile)
	XtFree(filename);
    else
#endif
	delete filename;
    return FALSE;

}
//
// Handle this->toFileToggle value changes.
//
extern "C" void PrintProgramDialog_ToFileToggleCB(Widget w,
                                XtPointer clientData,
                                XtPointer callData)
{
      PrintProgramDialog *ppd= (PrintProgramDialog*) clientData;
      ppd->setSensitivity();
}
//
// Set the sensitivity of the widgets based on the toFileToggle
// state. 
//
void PrintProgramDialog::setSensitivity()
{
    Boolean toFile = this->isPrintingToFile() ? True : False;

    SetTextSensitive(this->printerName, !toFile,
                     !toFile ? theDXApplication->getForeground()
                             : theDXApplication->getInsensitiveColor());
    SetTextSensitive(this->fileName, toFile,
                     toFile ? theDXApplication->getForeground()
                             : theDXApplication->getInsensitiveColor());
    XtSetSensitive(this->fileSelectButton, toFile);
}
//
// Return TRUE if the labelParamsToggle is set indicating that we
// should include parameter labels. 
//
boolean PrintProgramDialog::isLabelingParams()
{
    Boolean set;
    Arg wargs[2];

    ASSERT(this->getRootWidget());

    XtSetArg(wargs[0], XmNset, &set); 
    XtGetValues(this->labelParamsToggle, wargs, 1);
    return (set == True ? TRUE : FALSE);
}
//
// Return TRUE if the toFileToggle is set indicating that we
// should print to a file instead of the printer.
//
boolean PrintProgramDialog::isPrintingToFile()
{
    Boolean set;
    Arg wargs[2];

    ASSERT(this->getRootWidget());

    XtSetArg(wargs[0], XmNset, &set); 
    XtGetValues(this->toFileToggle, wargs, 1);
    return (set == True ? TRUE : FALSE);
}

//
// Handles the fileSelectButton callbacks and causes the 
// printProgramFileDialog  to be created if necessary and then posted.
//
extern "C" void PrintProgramDialog_ButtonCB(Widget w,
                                XtPointer clientData,
                                XtPointer callData)
{
      PrintProgramDialog *ppd = (PrintProgramDialog*) clientData;
      ppd->postFileSelector();
}
//
// Opens the PostScript file selection dialog. 
//
void PrintProgramDialog::postFileSelector()
{
    if (!this->printProgramFileDialog) {
        this->printProgramFileDialog = new PrintProgramFileDialog(this,
					this->editor->getNetwork());
    }

    this->printProgramFileDialog->post();
}

