/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"



#include <ctype.h>
#include <sys/types.h>
#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif

#include <Xm/Xm.h>

#include "dxl.h"

#include "XmUtility.h"

#include <Xm/Form.h>
#include <Xm/Frame.h>
#include <Xm/Label.h>
#include <Xm/PushB.h>
#include <Xm/CascadeB.h>
#include <Xm/RowColumn.h>
#include <Xm/Separator.h>
#include <Xm/ToggleB.h>
#include <Xm/RowColumn.h>
#include <Xm/List.h>
#include <Xm/TextF.h>
#ifdef DXD_WIN
#include <Xm/FileSB.h>
#endif
#include "../widgets/Stepper.h"
#include "ListIterator.h"
#include "Dictionary.h"
#include "FileContents.h"

extern Dictionary* theTypeChoiceDictionary;
extern void BuildTheTypeChoiceDictionary();

#include "CommandTextPopup.h"
#include "GARChooserWindow.h"
#include "GARApplication.h"
#include "GARMainWindow.h"
#include "ToggleButtonInterface.h"
#include "ButtonInterface.h"
#include "NoUndoChooserCommand.h"
#include "QuitCommand.h"
#include "WarningDialogManager.h"
#include "TypeChoice.h"
#include "ConfirmedQCommand.h"
#include "DataFileDialog.h"

boolean GARChooserWindow::ClassInitialized = FALSE;

String GARChooserWindow::DefaultResources[] = 
{
    ".title:				Data Prompter",
    ".iconName:				DX",
//    ".geometry:				+0+105",

    "*notebook.XmToggleButton.indicatorType:	XmONE_OF_MANY",
    "*notebook.XmToggleButton.indicatorSize:	18",
    "*notebook.XmToggleButton.spacing:		15",
    "*notebook.XmToggleButton.leftOffset:	10",
    "*notebook.XmToggleButton.rightOffset:	2",
    "*notebook.XmToggleButton.alignment:	XmALIGNMENT_BEGINNING",
    "*notebook.XmToggleButton.shadowThickness:	0",

    "*fnHeader.labelString:		Data file name",
    "*fnHeader.foreground:		#26a",
    "*helpHeader.labelString:		Hints",
    "*helpHeader.foreground:		#26a",
    "*popupButton.labelString:		...",
    "*fileMenu.labelString:		File",
    "*optionsMenu.labelString:		Options",
    "*showOption.labelString:		Open Message Window...",
    "*openPrompterOption.labelString:	Open General Array Importer...",

    "*quitOption.labelString:		Quit",
    "*quitOption.mnemonic:		Q",
    "*quitOption.accelerator:		Ctrl<Key>Q",
    "*quitOption.acceleratorText:	Ctrl+Q",

    "*dataFileOption.labelString:	Select Data File...",
    "*dataFileOption.mnemonic:		F",

    "*notebookHeader.labelString:	Select the format of your data:",
    "*notebookHeader.foreground:	#26a",

    //"*accelerators:             #augment\n"
    //"<Key>Return:                   BulletinBoardReturn()",


    NUL(char*)
};

GARChooserWindow::GARChooserWindow() : IBMMainWindow ("SMDName", TRUE)
{
    this->quitCmd =
	new ConfirmedQCommand("quit", this->commandScope, TRUE); 

    this->showMessagesCmd =
	new NoUndoChooserCommand ("showMessages",this->commandScope,
	    TRUE, this, NoUndoChooserCommand::ShowMessages);
	    
    this->openPrompterCmd =
	new NoUndoChooserCommand ("openPrompter",this->commandScope,
	    TRUE, this, NoUndoChooserCommand::OpenPrompter);
	    
    this->selectDataCmd =
	new NoUndoChooserCommand ("selectData",this->commandScope,
	    TRUE, this, NoUndoChooserCommand::SelectDataFile);
	    
    this->nullCmd = 
	new NoUndoChooserCommand ("donothing", this->commandScope,
	    TRUE, this, NoUndoChooserCommand::NoOp);

    //
    // Install the default resources for THIS class (not the derived classes)
    //
    if (NOT GARChooserWindow::ClassInitialized)
    {
	ASSERT(theGARApplication);
	GARChooserWindow::ClassInitialized = TRUE;
	this->installDefaultResources(theApplication->getRootWidget());
	BuildTheTypeChoiceDictionary();
    }

    this->text_file = new CommandTextPopup(this);
    this->file_search_dir = NUL(char*);
    this->connection = NUL(DXLConnection*);
    this->mismatch_complaint = FALSE;
    this->dfdialog = NUL(DataFileDialog*);

    TypeChoiceAllocator tca;
    int size = theTypeChoiceDictionary->getSize();
    int i;
    this->typeChoices = new List;
    for (i=1; i<=size; i++) {
	tca = (TypeChoiceAllocator)theTypeChoiceDictionary->getDefinition(i);
	Symbol sym = theTypeChoiceDictionary->getSymbol(i);
	TypeChoice *tc = tca(this, sym);
	this->typeChoices->appendElement((const void*)tc);
    }
    this->choice = NUL(TypeChoice*);
    this->unset_sync_wpid = NUL(XtWorkProcId);
    this->openPrompterCmd->deactivate(
	"You must create a .general file for gridded or scatterd data, first.");
}

//
// Install the default resources for this class.
//
void GARChooserWindow::installDefaultResources(Widget baseWidget)
{
    this->setDefaultResources(baseWidget, GARChooserWindow::DefaultResources);
    this->IBMMainWindow::installDefaultResources(baseWidget);
}


GARChooserWindow::~GARChooserWindow()
{
    delete this->nullCmd;

    delete this->text_file;
    ListIterator it(*this->typeChoices);
    TypeChoice *tc;
    while (tc = (TypeChoice*)it.getNext()) {
	delete tc;
    }
    delete this->typeChoices;
    if (this->dfdialog) delete this->dfdialog;
    if (this->file_search_dir) delete this->file_search_dir;

    if (this->unset_sync_wpid)
	XtRemoveWorkProc (this->unset_sync_wpid);

    if (this->connection)
	DXLExitDX(this->connection);
}


Widget GARChooserWindow::createWorkArea (Widget parent)
{
    //
    // Create the outer frame and form.
    //
    Widget frame = XtVaCreateManagedWidget ("chooFrame",
	xmFrameWidgetClass,	parent,
	XmNshadowType,		XmSHADOW_OUT,
    NULL);
    Widget form = XtVaCreateManagedWidget ("chooForm",
	xmFormWidgetClass,	frame,
    NULL);


    //
    // Status area... includes file name, bubble help, and labeling
    //
    this->text_file->createTextPopup (form);
    Widget tf_root = this->text_file->getRootWidget();
    XtVaSetValues (tf_root,
	XmNleftAttachment,	XmATTACH_FORM,
	XmNleftOffset,		10,
	XmNrightAttachment,	XmATTACH_FORM,
	XmNrightOffset,		10,
	XmNtopAttachment,	XmATTACH_FORM,
	XmNtopOffset,		20,
	XmNbottomAttachment,	XmATTACH_OPPOSITE_FORM,
	XmNbottomOffset,	-60,
    NULL);

    Widget label = XtVaCreateManagedWidget ("fnHeader",
	xmLabelWidgetClass,	form,
	XmNrightAttachment,	XmATTACH_FORM,
	XmNrightOffset,		20,
	XmNbottomAttachment,	XmATTACH_WIDGET,
	XmNbottomWidget,	tf_root,
	XmNbottomOffset,	0,
    NULL);

    Widget help_frame = XtVaCreateManagedWidget ("helpFrame",
	xmFrameWidgetClass,	form,
	XmNshadowType,		XmSHADOW_IN,
	XmNleftAttachment,	XmATTACH_FORM,
	XmNrightAttachment,	XmATTACH_FORM,
	XmNbottomAttachment,	XmATTACH_FORM,
	XmNtopAttachment,	XmATTACH_OPPOSITE_FORM,
	XmNleftOffset,		4,
	XmNrightOffset,		4,
	XmNbottomOffset,	4,
	XmNtopOffset,		-23,
    NULL);
    XmString xmstr = XmStringCreateLtoR (" ", "small_normal");
    Widget help_viewer = XtVaCreateManagedWidget ("helpViewer",
	xmLabelWidgetClass,	help_frame,
	XmNlabelString,		xmstr,
	XmNrecomputeSize,	False,
    NULL);
    XmStringFree(xmstr);

    XtVaCreateManagedWidget ("helpHeader",
	xmLabelWidgetClass,	form,
	XmNrightAttachment,	XmATTACH_FORM,
	XmNrightOffset,		20,
	XmNbottomAttachment,	XmATTACH_WIDGET,
	XmNbottomWidget,	help_frame,
	XmNbottomOffset,	0,
    NULL);

    XtVaCreateManagedWidget ("sep",
	xmSeparatorWidgetClass,	form,
	XmNrightAttachment,	XmATTACH_FORM,
	XmNleftAttachment,	XmATTACH_FORM,
	XmNrightOffset,		0,
	XmNleftOffset,		0,
	XmNbottomAttachment,	XmATTACH_WIDGET,
	XmNbottomWidget,	help_frame,
	XmNbottomOffset,	4,
    NULL);


    Widget notelab = XtVaCreateManagedWidget ("notebookHeader",
	xmLabelWidgetClass,	form,
	XmNrightAttachment,	XmATTACH_FORM,
	XmNrightOffset,		20,
    NULL);

    Widget sep = XtVaCreateManagedWidget ("notesep",
	xmSeparatorWidgetClass,	form,
	XmNtopAttachment,	XmATTACH_FORM,
	XmNtopOffset,		70,
	XmNleftAttachment,	XmATTACH_FORM,
	XmNrightAttachment,	XmATTACH_FORM,
    NULL);
    XtVaSetValues (notelab, 
	XmNbottomAttachment,	XmATTACH_WIDGET,
	XmNbottomWidget,	sep,
	XmNbottomOffset,	-7,
	XmNtopAttachment,	XmATTACH_NONE,
    NULL);


    Widget notebook = XtVaCreateManagedWidget ("notebook",
	xmFormWidgetClass,	form,
	XmNtopAttachment,	XmATTACH_WIDGET,
	XmNtopWidget,		sep,
	XmNtopOffset,		2,
	XmNleftAttachment,	XmATTACH_FORM,
	XmNrightAttachment,	XmATTACH_FORM,
	XmNbottomAttachment,	XmATTACH_WIDGET,
	XmNleftOffset,		2,
	XmNrightOffset,		2,
	XmNbottomWidget,	help_frame,
    NULL);

    ListIterator it(*this->typeChoices);
    TypeChoice *tc;
    while (tc = (TypeChoice*)it.getNext()) {
	tc->createChoice (notebook);
    }


    theApplication->setHelpViewer(help_viewer);
    theApplication->enableBubbleHelp();

    this->showForms();

    theGARApplication->addHelpCallbacks(parent, this->name);

    this->setCommandActivation();

    return frame;
}

void GARChooserWindow::manage()
{
    this->IBMMainWindow::manage();
    XmUpdateDisplay(this->getRootWidget());

    //
    // If a data file name was specified on the command line, then insert the
    // name
    //
    const char *df = theGARApplication->getResourcesData();
    if ((df) && (df[0])) {
	this->text_file->setText(df);
	this->fileNameCB();
    } 
}


void GARChooserWindow::createMenus (Widget menu_bar)
{
    this->createFileMenu (menu_bar);
    this->createOptionsMenu (menu_bar);
    this->createBaseHelpMenu(menu_bar,TRUE,TRUE);
}

void GARChooserWindow::createFileMenu (Widget menu_bar)
{
    //
    // Create "File" menu and options.
    //
    Widget pulldown =
	XmCreatePulldownMenu(menu_bar, "fileMenuPulldown", NUL(ArgList), 0);

    XtVaCreateManagedWidget ("fileMenu",
	 xmCascadeButtonWidgetClass, menu_bar,
	 XmNsubMenuId, 		pulldown,
    NULL);
 
    new ButtonInterface(pulldown,"dataFileOption", this->selectDataCmd);
    XtVaCreateManagedWidget ("sep", xmSeparatorWidgetClass, pulldown, NULL);
    new ButtonInterface(pulldown,"quitOption", this->quitCmd);
}
void GARChooserWindow::createOptionsMenu (Widget menu_bar)
{
    //
    // Create "Options" menu and options.
    //
    Widget pulldown =
	XmCreatePulldownMenu(menu_bar, "optionsMenuPulldown",
                                        NUL(ArgList), 0);
    XtVaCreateManagedWidget ("optionsMenu",
	 xmCascadeButtonWidgetClass, menu_bar,
	 XmNsubMenuId, 		pulldown,
    NULL);
 
    new ButtonInterface(pulldown,"showOption",this->showMessagesCmd,
	"Messages from the DX executive");

    new ButtonInterface(pulldown,"openPrompterOption",this->openPrompterCmd,
	"The description in your General Array Format file.");
}





void GARChooserWindow::setChoice (TypeChoice *new_choice)
{
    //
    // We have the old type saved.  Pull out the button.
    //
    if (this->choice != new_choice) {
	if (this->choice) this->choice->setSelected(FALSE);
	this->choice = new_choice;
	if (this->choice) this->choice->setSelected(TRUE);
    } 
    if ((this->choice) && (this->choice->selected() == FALSE))
	this->choice = NUL(TypeChoice*);

    this->setCommandActivation();
    this->showForms();

    //
    // If the new choice dislikes having the prompter open and the
    // prompter is already open, then get rid of it.
    //
    if ((this->choice) && (this->choice->usesPrompter()==FALSE)) {
	IBMMainWindow* gmw = theGARApplication->getMainWindow(FALSE);
	if ((gmw) && (gmw->isManaged()))
	    theGARApplication->destroyMainWindow();
	    this->openPrompterCmd->deactivate(
		"You must create a .general file for gridded or scatterd data, first.");
    }


    if ((this->choice==NUL(TypeChoice*)) || (this->choice->selected() == FALSE)) {
	return ;
    }


    //
    // Now look for a mismatch between the file name extension and the
    // data type selected.  ...but only complain 1 time per file.
    //
    const char *fext = this->choice->getFileExtension();
    if ((!this->mismatch_complaint) && 
	(fext) && (fext[0])) {
	    boolean mismatch = FALSE;
	    char *file_name = this->getFileReady();
	    char *ext = NUL(char*);
	    if (file_name) ext = this->findExtension(file_name);
	    char *cp = DuplicateString(fext);
	    int i,len;
	    len = strlen(cp);
	    for (i=0; i<len; i++) cp[i] = toupper(cp[i]);
	    if ((ext) && (ext[0]) && (this->choice->canHandle(ext) == FALSE)) {
		if (cp) mismatch = (!EqualString(ext, cp));
	    } else if ((cp) && (cp[0]))
		mismatch = TRUE;
	    if (cp) delete cp;
	    if (ext) delete ext;

	    if (file_name) {
		if (mismatch) {
		    char msg[256];
		    sprintf (msg, "File extension / Data type mismatch.");
		    WarningMessage (msg);
		    this->mismatch_complaint = TRUE;
		}
		delete file_name;
	    }
	}

    return ;
}


void GARChooserWindow::showForms()
{
//
// minHeight should be initialized to whatever space is required
// to show just the locally created widgets.  It's assumed that the outliner
// expands only vertically.   That means that minWidth will be set to the
// whatever is requested by an expanded TypeChoice, whereas minHeight will
// be a cumulative sum.  minHeight will automatically be larger if there
// is a new subclass off TypeChoice.
//
Dimension minHeight = 150;
Dimension minWidth = 410;

    ListIterator it(*this->typeChoices);
    TypeChoice *tc;
    boolean first = TRUE;
    Widget prev_widget = NUL(Widget);
    TypeChoice* unmanage_guy = NUL(TypeChoice*);
    TypeChoice* manage_guy = NUL(TypeChoice*);
    while (tc = (TypeChoice*)it.getNext()) {
	if (first) {
	    XtVaSetValues (tc->getOptionWidget(),
		XmNtopAttachment, XmATTACH_FORM,
		XmNtopOffset,	  4,
	    NULL);
	    first = FALSE;
	} else {
	    ASSERT(prev_widget);
	    XtVaSetValues (tc->getOptionWidget(),
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget,	  prev_widget,
		XmNtopOffset,	  4,
	    NULL);
	    if (unmanage_guy) unmanage_guy->unmanage();
	}
	unmanage_guy = NUL(TypeChoice*);
	if ((!tc->selected()) && (tc->isManaged())) {
	    unmanage_guy = tc;
	} else if ((tc->selected()) && (!tc->isManaged())) {
	    manage_guy = tc;
	}
#if VERTICAL_LAYOUT
	if (!tc->selected()) {
	    prev_widget = tc->getOptionWidget();
	} else {
	    prev_widget = tc->getRootWidget();
	    minWidth = tc->getMinWidth();
	}
#else
	prev_widget = tc->getOptionWidget();
	XtVaSetValues (tc->getRootWidget(),
	    XmNtopAttachment, 	XmATTACH_FORM,
	    XmNleftAttachment,	XmATTACH_WIDGET,
	    XmNrightAttachment,	XmATTACH_FORM,
	    XmNleftWidget,	tc->getOptionWidget(),
	    XmNtopOffset,	4,
	    XmNleftOffset,	4,
	    XmNrightOffset,	4,
	NULL);	
	if (tc->selected()) minWidth = tc->getMinWidth();
#endif
	minHeight+= tc->getHeightContribution();
    }
    if (unmanage_guy) unmanage_guy->unmanage();
    Dimension oldHeight, oldWidth;
    XtVaGetValues  (this->getRootWidget(), 
	XmNheight,	&oldHeight,
	XmNwidth,	&oldWidth,
    NULL);
    int n = 0;
    Arg args[9];
    XtSetArg (args[n], XmNminHeight, minHeight); n++;
    XtSetArg (args[n], XmNminWidth, minWidth); n++;
    //
    // This constant controls if or not the window will automatically resize
    // itself every time you pick a new data type choice, or if it will only
    // resize itself (if growing)||(if all choices unselected)
    //
#define SHRINK_WRAP 1
    boolean shrink_behavior = (oldHeight < minHeight) || SHRINK_WRAP;
    if ((shrink_behavior) || (!this->choice)) {
	XtSetArg (args[n], XmNheight, minHeight); n++;
    }
    shrink_behavior = (oldWidth < minWidth) || SHRINK_WRAP;
    if ((shrink_behavior) || (!this->choice)) {
	XtSetArg (args[n], XmNwidth, minWidth); n++;
    }
    XtSetValues (this->getRootWidget(), args, n);

    if (manage_guy) manage_guy->manage();
}

boolean GARChooserWindow::showMessages()
{
    this->choice->showList();
    return TRUE;
}

boolean GARChooserWindow::openExistingPrompter()
{
    GARMainWindow* gmw = theGARApplication->getMainWindow(FALSE);
    if (gmw == NUL(GARMainWindow*)) {
	this->openPrompterCmd->deactivate(
	    "You must create a .general file for gridded or scatterd data, first.");
	return FALSE;
    }
    gmw->manage();
    return TRUE;
}


void GARChooserWindow::setCommandActivation()
{
    if (this->choice) this->choice->setCommandActivation();
    if (!this->choice) 
	this->showMessagesCmd->deactivate("You must select an import type, first.");
    else 
	this->showMessagesCmd->activate();

    GARMainWindow* gmw = theGARApplication->getMainWindow(FALSE);
    if (gmw == NUL(GARMainWindow*))
	this->openPrompterCmd->deactivate(
	    "You must create a .general file for gridded or scatterd data, first.");
    else
	this->openPrompterCmd->activate();
}

void GARChooserWindow::setDataFile (const char *file)
{
    this->text_file->setText(file, TRUE);
}

void GARChooserWindow::fileNameCB()
{
    char *full_path = this->getFileReady();
    if ((full_path) && (full_path[0])) {

	char *ext = this->findExtension(full_path);

	//
	// If the file was found using a search path, then install the full pathname
	//
	const char *short_path = this->text_file->getText();
	if (!EqualString(full_path, short_path)) {
	    this->text_file->setText(full_path);
	}

	//
	// Find out if the currently selected data type wants to retain control
	// over the new data file name.  This happens when the type is GridChoice
	// and the data file name inside the .general file has an extension which
	// matches one of the other type choices.  image.general
	//
	if ((this->choice) && (this->choice->retainsControl(full_path))) {
	} else {
	    ListIterator it(*this->typeChoices);
	    TypeChoice *tc;
	    if (ext) {
		while (tc = (TypeChoice*)it.getNext()) {
		    const char *fext = tc->getFileExtension();
		    if (tc->canHandle(ext)) {
			if (tc->selected() == FALSE) 
			    this->setChoice(tc);
			break;
		    } else if (fext) {
			char *cp = DuplicateString(fext);
			int i, len;
			len = strlen(cp);
			for (i=0; i<len; i++) cp[i] = toupper(cp[i]);
			if (EqualString(cp, ext)) {
			    if (tc->selected() == FALSE) 
				this->setChoice(tc);
			    if (cp) delete cp;
			    break;
			}
			if (cp) delete cp;
		    }
		}

		delete ext;
	    }

	    it.setList(*this->typeChoices);
	    while (tc = (TypeChoice*)it.getNext()) 
		tc->hideList();

	    this->mismatch_complaint = FALSE;
	}
    }

    if (full_path) delete full_path;

    this->setCommandActivation();
}

char *GARChooserWindow::getFileReady()
{
    const char *fext = NUL(char*);
    if (this->choice) fext = this->choice->getFileExtension();
    //
    // Does the text widget at the top contain a valid data file name?
    //
    char *cp = NULL;
    char *file_name = NUL(char*);
    if (this->text_file->getRootWidget())  
	cp = this->text_file->getText();
    if ((cp) && (cp[0])) {
	file_name = theGARApplication->resolvePathName(cp, fext);
    }
    if (cp) XtFree(cp);
    return file_name;
}

char *GARChooserWindow::findExtension(const char *cp)
{
    if ((!cp) || (!cp[0])) return NUL(char*);

    //
    // Count backwards from the end of the file name looking for the 
    // file extension.  It will automatically determine a data type.
    //
    int len = strlen(cp);
    int i;
    for (i=len-1; i>=0; i--) {
	if (cp[i] == '.') break;
    }
    if ((i<0) || (cp[i] != '.'))
	return NUL(char*);

    char *ext = new char[1+len-i];
    strcpy (ext, &cp[i]);

    int j,t = strlen(ext);
    for (j=0; j<t; j++) ext[j] = toupper(ext[j]);

    return ext;
}


const char *
GARChooserWindow::getDataFilename()
{
static char tbuf[512];

    if (!this->text_file) return NUL(char*);
    char *cp = this->text_file->getText();
    if (!cp) return NUL(char*);
    if (!cp[0]) {
	XtFree(cp);
	return NUL(char*);
    }
    strcpy (tbuf, cp);
    XtFree(cp);
    return tbuf;
}

void GARChooserWindow::closeWindow()
{
    this->quitCmd->execute();
}

boolean GARChooserWindow::postDataFileSelector()
{
    if (!this->dfdialog) 
	this->dfdialog = new DataFileDialog (this->getRootWidget(), this);

    this->dfdialog->post();
    if (this->file_search_dir) {
	char* cp = DuplicateString(this->file_search_dir);
	this->setFileSearchDir(cp);
	if (cp) delete cp;
    }
    return TRUE;
}


void GARChooserWindow::setFileSearchDir(const char *value)
{
    if (this->file_search_dir) delete this->file_search_dir;
    this->file_search_dir = NUL(char*);

    this->file_search_dir = DuplicateString(value);

    if (!this->dfdialog) return;
    if (!this->file_search_dir) return;

    Widget fsb = this->dfdialog->getFileSelectionBox();
    if (!fsb) return ;

    const char *ext = (this->choice?this->choice->getFileExtension():NUL(char*));
    int extlen = 0;
    if (ext) extlen = strlen(ext);
    char *dirspec = new char[strlen(this->file_search_dir) + extlen + 3];
    strcpy(dirspec, this->file_search_dir);
#if defined(intelnt)
/* Convert to Unix if DOS */
    for(int i=0; i<strlen(dirspec); i++)
        if(dirspec[i] == '/') dirspec[i] = '\\';
    strcat(dirspec, "\\*");
#else
    strcat(dirspec, "/*");
#endif
    if (extlen) 
	strcat(dirspec, ext);
    XmString xmstr = XmStringCreateLtoR (dirspec, "bold");
#ifdef DXD_WIN
    Widget filt = XmFileSelectionBoxGetChild(fsb, XmDIALOG_FILTER_TEXT);
    XmTextSetString(filt, dirspec);
#endif
    XtVaSetValues (fsb, XmNdirMask, xmstr, NULL);
    XmStringFree(xmstr);
    delete dirspec;
}


//
// Make a copies of the net and cfg files replacing things listed in args.  
// This is used as an alternative to DXLinkInputs.  Nancy and Donna wanted this
// in order to improve the appearance of the ezstart nets.  A potential problem
// is that you have to munge disk files and reload a program in order to change
// a value where you used to have to simply send down new values using DXLink.
//
void GARChooserWindow::loadAndSet (DXLConnection* conn, const char* net_file,
    char* argv[], int argc)
{
int j;

    ASSERT ((net_file) && (net_file[0]));
    ASSERT (conn);

    //
    // The number of args must be an even number.  For each pattern there is a 
    // replacement.
    //
    ASSERT ((argc&0x1) == 0);

    char* cfg_file = DuplicateString(net_file);
    int cfg_spot = strlen(net_file) - 4;
    ASSERT (!strcmp(&cfg_file[cfg_spot], ".net"));
    strcpy (&cfg_file[cfg_spot], ".cfg");

    FileContents net_to_load(net_file);
    FileContents cfg_to_load(cfg_file);
    delete cfg_file;
    if (net_to_load.initialize()) {
	const char* base_name = net_to_load.sansExtension();
	cfg_to_load.initialize(base_name);

	for (j=0; j<argc; j+=2) {
	    char* pattern = argv[j];
	    char* replacement = argv[j+1];
	    net_to_load.replace (pattern, replacement);
	    cfg_to_load.replace (pattern, replacement);
	}
	net_to_load.close();
	cfg_to_load.close();

	//
	// Loading this visual program should get both .net and .cfg files.  So
	// it shouldn't be necessary to explicitly request a load of the .cfg file.
	//
	DXLSetSynchronization(conn, 1);
	DXLLoadVisualProgram (conn, net_to_load.getFileName());
	XtAppContext apcxt = theApplication->getApplicationContext();
	if (this->unset_sync_wpid == NUL(XtWorkProcId))
	    XtAppAddWorkProc (apcxt, (XtWorkProc)
		GARChooserWindow_SyncWP, (XtPointer)this);
	
    } else {
	net_to_load.close();
	cfg_to_load.close();
	WarningMessage (
	    "Unable to write to temporary directory.\nTry setting TMPDIR and restarting."
	);
    }
}


extern "C" Boolean GARChooserWindow_SyncWP (XtPointer clientData)
{
    GARChooserWindow* gcw = (GARChooserWindow*)clientData;
    ASSERT(gcw);
    //
    // connection can be 0 here if the user picked File/Quit while the
    // visual program was executing.
    //
    if (gcw->connection)
	DXLSetSynchronization(gcw->connection, 0);
    gcw->unset_sync_wpid = NUL(XtWorkProcId);
    return True;
}
