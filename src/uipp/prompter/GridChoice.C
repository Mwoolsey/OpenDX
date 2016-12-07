/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"



#include <sys/types.h>
#include <time.h>
#include <sys/stat.h>
#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif


#include "GridChoice.h"
#include "GARApplication.h"
#include "ToggleButtonInterface.h"
#include "NoUndoChoiceCommand.h"
#include "GARChooserWindow.h"
#include "GARMainWindow.h"
#include "WarningDialogManager.h"
#include "ErrorDialogManager.h"

#include <Xm/Xm.h>
#include <Xm/Label.h>
#include <Xm/Form.h>
#include <Xm/Separator.h>
#include <Xm/RowColumn.h>
#include "../widgets/Stepper.h"

#include "grid1.bm"
#include "grid2.bm"
#include "grid3.bm"
#include "grid4.bm"

boolean GridChoice::ClassInitialized = FALSE;

String GridChoice::DefaultResources[] = {
    "*smdForm*Offset:				10",
    "*smdForm*XmToggleButton.shadowThickness: 	0",
    "*smdForm*XmRowColumn.orientation:		XmHORIZONTAL",
    "*smdForm*titleLabel.labelString: 		Description of your data",
    "*smdForm*gridType.labelString: 		Grid type",
    "*smdForm*numVarLabel.labelString:		Number of variables",
    "*smdForm*numVarLabel.leftOffset:		30",
    "*smdForm*dimensionLabel.labelString:	Dimension:",
    "*smdForm*dimensionLabel.topOffset:		12",
    "*smdForm*dimensionStepper.topOffset:	12",
    "*smdForm*singleTimeStep.labelString:	Single time step",
    "*smdForm*dataOrg.topOffset:		13",
    "*smdForm*dataOrg.labelString:		Data organization:",
    "*smdForm*dataOrg.leftOffset:		30",
    "*smdForm*blockRB.marginRight:		30",
    "*smdForm*blockRB.labelString:		Block",
    "*smdForm*spreadsheetRB.labelString:	Columnar",

    "*gridRadio.orientation:			XmHORIZONTAL",
    "*gridRadio.spacing:			40",
    "*positionsOption.labelString:		Positions in data file",
    "*notebook*positionsOption.indicatorType:	XmN_OF_MANY",
    "*notebook*singleTimeStep.indicatorType:	XmN_OF_MANY",

    "*accelerators:             		#augment\n"
    "<Key>Return:                 		BulletinBoardReturn()",

    NUL(char*)
};

GridChoice::GridChoice (GARChooserWindow *gcw, Symbol sym) : 
    NonimportableChoice ("griddedData", TRUE, TRUE, TRUE, TRUE, gcw, sym)
{
    this->dimension_l = NUL(Widget);
    this->dimension_s = NUL(Widget);
    this->data_org_l = NUL(Widget);
    this->data_org_rb = NUL(Widget);

    this->recursive = FALSE;

    this->gridTypeCmd =
	new NoUndoChoiceCommand ("gridType", this->getCommandScope(),
	    TRUE, this, NoUndoChoiceCommand::SetGridType);

    this->positionsCmd =
	new NoUndoChoiceCommand ("positions", this->getCommandScope(),
	    TRUE, this, NoUndoChoiceCommand::Positions);

    this->positions_was_on = FALSE;
}

GridChoice::~GridChoice()
{
    if (this->gridTypeCmd) delete this->gridTypeCmd;
    if (this->positionsCmd) delete this->positionsCmd;
}

void GridChoice::initialize() 
{
    if (GridChoice::ClassInitialized) return ;
    GridChoice::ClassInitialized = TRUE;

    this->setDefaultResources
	(theApplication->getRootWidget(), TypeChoice::DefaultResources);
    this->setDefaultResources
	(theApplication->getRootWidget(), NonimportableChoice::DefaultResources);
    this->setDefaultResources
	(theApplication->getRootWidget(), GridChoice::DefaultResources);
}


Widget GridChoice::createBody (Widget parent, Widget top)
{
    Widget smd_form = 
	XtVaCreateManagedWidget("smdForm",
	    xmFormWidgetClass, 	parent,
	    XmNtopAttachment,	(top?XmATTACH_WIDGET:XmATTACH_FORM),
	    XmNtopWidget,	top,
	    XmNtopOffset,	(top?0:2),
	    XmNleftAttachment,	XmATTACH_FORM,
	    XmNleftOffset,	2,
	NULL);

    Display *d = XtDisplay(smd_form);
    Window win = XtWindow(theApplication->getRootWidget()); 
    Pixel bg, fg, bs;
    int depth;

    XtVaGetValues(smd_form, 
	XmNforeground, &fg, 
	XmNbackground, &bg, 
	XmNdepth,	&depth,
	XmNbottomShadowColor, &bs,
    NULL);


    Widget label2 = 
	XtVaCreateManagedWidget("gridType", xmLabelWidgetClass, smd_form,
	    XmNtopAttachment,	XmATTACH_FORM,
	    XmNtopOffset,	21,
	    XmNleftAttachment,	XmATTACH_FORM,
	NULL);

    Widget radioBox = XmCreateRadioBox (smd_form, "gridRadio", NULL, 0);
    XtManageChild (radioBox);
    XtVaSetValues (radioBox,
	XmNtopAttachment,	XmATTACH_FORM,
	XmNleftAttachment,	XmATTACH_WIDGET,
	XmNleftWidget,		label2,
    NULL);

    //
    // G R I D   T Y P E   1
    //
    Pixmap pix = XCreatePixmapFromBitmapData(
	d, win, (char *)grid1_bits, grid1_width, grid1_height,
	fg, bg, depth);
    Pixmap sel_pix = XCreatePixmapFromBitmapData(
	d, win, (char *)grid1_bits, grid1_width, grid1_height,
	fg, bs, depth);
    this->grid1Option = 
	new ToggleButtonInterface (radioBox, "grid1Option", this->gridTypeCmd,
	    TRUE, "Regular grid: data-point specified by origin-delta pairs");
    XtVaSetValues (this->grid1Option->getRootWidget(),
	XmNindicatorOn,		False,
	XmNlabelType,		XmPIXMAP,
	XmNlabelPixmap,		pix,
	XmNselectPixmap,	sel_pix,
	XmNshadowThickness, 	1,
	XmNspacing,		0,
	XmNmarginLeft,		0,
	XmNmarginRight,		0,
	XmNfillOnSelect,	True,
	XmNselectColor,		bs,
    NULL);

    //
    // G R I D   T Y P E   2
    //
    pix = XCreatePixmapFromBitmapData(
	d, win, (char *)grid2_bits, grid2_width, grid2_height,
	fg, bg, depth);
    sel_pix = XCreatePixmapFromBitmapData(
	d, win, (char *)grid2_bits, grid2_width, grid2_height,
	fg, bs, depth);
    this->grid2Option = 
	new ToggleButtonInterface (radioBox, "grid2Option", this->gridTypeCmd,
	    FALSE, "Partly regular: dimension(s) not describable by origin-delta pair");
    XtVaSetValues (this->grid2Option->getRootWidget(),
	XmNindicatorOn,		False,
	XmNlabelType,		XmPIXMAP,
	XmNlabelPixmap,		pix,
	XmNselectPixmap,	sel_pix,
	XmNshadowThickness, 	1,
	XmNspacing,		0,
	XmNmarginLeft,		0,
	XmNmarginRight,		0,
	XmNfillOnSelect,	True,
	XmNselectColor,		bs,
    NULL);

    //
    // G R I D   T Y P E   3
    //
    pix = XCreatePixmapFromBitmapData(
	d, win, (char *)grid3_bits, grid3_width, grid3_height,
	fg, bg, depth);
    sel_pix = XCreatePixmapFromBitmapData(
	d, win, (char *)grid3_bits, grid3_width, grid3_height,
	fg, bs, depth);
    this->grid3Option = 
	new ToggleButtonInterface (radioBox, "grid3Option", this->gridTypeCmd,
	    FALSE, "Warped regular grid: specify each position explicitly");
    XtVaSetValues (this->grid3Option->getRootWidget(),
	XmNindicatorOn,		False,
	XmNlabelType,		XmPIXMAP,
	XmNlabelPixmap,		pix,
	XmNselectPixmap,	sel_pix,
	XmNshadowThickness, 	1,
	XmNspacing,		0,
	XmNmarginLeft,		0,
	XmNmarginRight,		0,
	XmNfillOnSelect,	True,
	XmNselectColor,		bs,
    NULL);

    //
    // G R I D   T Y P E   4
    //
    pix = XCreatePixmapFromBitmapData(
	d, win, (char *)grid4_bits, grid4_width, grid4_height,
	fg, bg, depth);
    sel_pix = XCreatePixmapFromBitmapData(
	d, win, (char *)grid4_bits, grid4_width, grid4_height,
	fg, bs, depth);
    this->grid4Option = 
	new ToggleButtonInterface (radioBox, "grid4Option", this->gridTypeCmd,
	    FALSE, "Scattered data: no connections between data points");
    XtVaSetValues (this->grid4Option->getRootWidget(),
	XmNindicatorOn,		False,
	XmNlabelType,		XmPIXMAP,
	XmNlabelPixmap,		pix,
	XmNselectPixmap,	sel_pix,
	XmNshadowThickness, 	1,
	XmNspacing,		0,
	XmNmarginLeft,		0,
	XmNmarginRight,		0,
	XmNfillOnSelect,	True,
	XmNselectColor,		bs,
    NULL);

    this->number_var_l = 
	XtVaCreateManagedWidget("numVarLabel",
		xmLabelWidgetClass,     smd_form,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		radioBox,
		NULL);
    this->number_var_s = 
	XtVaCreateManagedWidget("numVarStepper",
		xmStepperWidgetClass,   smd_form,
		XmNleftAttachment,	XmATTACH_POSITION,
		XmNleftPosition,	50,
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		radioBox,
		XmNiMinimum,		1,
		XmNiMaximum,		1000,
		XmNiValueStep,		1,
		XmNiValue,		1,
		NULL);

    this->positionsOption =
	new ToggleButtonInterface (smd_form, "positionsOption", this->positionsCmd,
	    FALSE, "Does your data look like this: x1,y1,data1; x2,y2,data2; ...?");
    XtVaSetValues (this->positionsOption->getRootWidget(),
	XmNleftAttachment,	XmATTACH_FORM,
	XmNtopAttachment,	XmATTACH_WIDGET,
	XmNtopWidget,		this->number_var_s,
    NULL);
	
    this->dimension_l = 
	XtVaCreateWidget("dimensionLabel",
		xmLabelWidgetClass,     smd_form,
		XmNleftAttachment,	XmATTACH_POSITION,
		XmNleftPosition,	50,
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		this->number_var_s,
		NULL);
    this->dimension_s = 
	XtVaCreateWidget("dimensionStepper",
		xmStepperWidgetClass,   smd_form,
		XmNleftAttachment,	XmATTACH_WIDGET,
		XmNleftWidget,		this->dimension_l,
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		this->number_var_s,
		XmNiMinimum,		1,
		XmNiMaximum,		4,
		XmNiValueStep,		1,
		XmNiValue,		3,
		NULL);
    this->singleTimeStepOption = 
	new ToggleButtonInterface (smd_form, "singleTimeStep", this->gcw->getNullCmd(),
	    TRUE, "Does your data contain only one time step?");
    XtVaSetValues (this->singleTimeStepOption->getRootWidget(),
	XmNleftAttachment,	XmATTACH_FORM,
	XmNtopAttachment,	XmATTACH_WIDGET,
	XmNtopWidget,		this->positionsOption->getRootWidget(),
    NULL);

    this->data_org_l = 
	XtVaCreateManagedWidget("dataOrg",
		xmLabelWidgetClass,     smd_form,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		this->singleTimeStepOption->getRootWidget(),
		NULL);
    this->data_org_rb = 
	XmCreateRadioBox(smd_form, "data_org_rb", NULL, 0);
    XtVaSetValues(this->data_org_rb,
		XmNleftAttachment,	XmATTACH_POSITION,
		XmNleftPosition,	50,
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		this->singleTimeStepOption->getRootWidget(),
		XmNmarginWidth,		0,
#ifndef OS2
		XmNpacking,		XmPACK_TIGHT,
#else
                XmNorientation,         XmHORIZONTAL,
#endif
		NULL);
    XtManageChild(this->data_org_rb);

    this->blockOption = new ToggleButtonInterface 
	(this->data_org_rb, "blockRB", this->gcw->getNullCmd(), TRUE, 
	"Data Interleaving:      t[1], t[2], ..., t[n],         p[1], p[2], ..., p[n]");

    this->spreadSheetOption = new ToggleButtonInterface 
	(this->data_org_rb, "spreadsheetRB", this->gcw->getNullCmd(), FALSE, 
	"Data Interleaving:      t[1],p[1]     t[2],p[2]     ...     t[n],p[n]");

    return smd_form;
}

boolean GridChoice::prompter()
{
    unsigned long mode = 0;
    mode = GMW_FULL;

    GARMainWindow *gmw = theGARApplication->getMainWindow();
    if (gmw) {
	Command *fullCmd = gmw->getFullCmd();
	if ((gmw->isManaged()) && (fullCmd->isActive()))
	    fullCmd->execute();
    }


    if (this->grid4Option->getState())
	    mode |= GMW_POINTS;

    if (!this->singleTimeStepOption->getState())
	mode |= GMW_SERIES;

    if (this->spreadSheetOption->getState())
	mode |= GMW_SPREADSHEET;
    gmw = theGARApplication->getMainWindow();
    if (!gmw) {
	theGARApplication->postMainWindow(mode);
	gmw = theGARApplication->getMainWindow();
    } else {
	gmw->newGAR(mode);
	gmw->manage();
    }
    ASSERT(gmw);


    if (this->positionsOption->getState()) {
	int dim;
	XtVaGetValues(this->dimension_s, XmNiValue, &dim, NULL);
	char structure[256];
	sprintf(structure, "%d-vector", dim);
	gmw->addField("locations", structure);
    }

    //
    // Add the variables
    //
    int i, vars;
    XtVaGetValues(this->number_var_s, XmNiValue, &vars, NULL);
    for(i = 0; i < vars; i++)
    {
	char fieldname[256];
	sprintf(fieldname, "field%d", i);
	gmw->addField(fieldname, "scalar");
    }
    return TRUE;
}

boolean GridChoice::simplePrompter()
{
    unsigned long mode = 0;
    GARMainWindow *gmw = theGARApplication->getMainWindow();
    if (gmw) {
	theGARApplication->mainWindow = NUL(GARMainWindow*);
	delete gmw;
	gmw = NUL(GARMainWindow*);
    }

    if (!this->singleTimeStepOption->getState()) {
       if (this->grid4Option->getState())
 	  mode = GMW_FULL | GMW_SERIES | GMW_POINTS;
       else
 	  mode = GMW_FULL | GMW_SERIES;
    } else if (this->grid1Option->getState()) {
	    mode = GMW_GRID | GMW_POSITIONS | GMW_SIMP_POSITIONS;
    } else if (this->grid2Option->getState()) {
	    mode = GMW_GRID | GMW_POSITIONS | GMW_POSITION_LIST;
    }
    else if (this->grid3Option->getState()) {
	    mode = GMW_GRID;
    }
    else if (this->grid4Option->getState()) {
       if (this->positionsOption->getState())
	    mode = GMW_POINTS;
       else
	    mode |= (GMW_POINTS | GMW_POSITIONS);
    }

    if (this->spreadSheetOption->getState()) 
	mode |= GMW_SPREADSHEET;

    if (this->positionsOption->getState()) {
	//
	// Turn off the positions section
	//
	mode &= ~(GMW_POSITIONS | GMW_SIMP_POSITIONS);
    }


    theGARApplication->postMainWindow(mode);
    gmw = theGARApplication->getMainWindow();
    ASSERT(gmw);


    if (this->positionsOption->getState()) {
	int dim;
	XtVaGetValues(this->dimension_s, XmNiValue, &dim, NULL);
	char structure[256];
	sprintf(structure, "%d-vector", dim);
	gmw->addField("locations", structure);
    }

    //
    // Add the variables
    //
    int i, vars;
    XtVaGetValues(this->number_var_s, XmNiValue, &vars, NULL);
    for(i = 0; i < vars; i++)
    {
	char fieldname[256];
	sprintf(fieldname, "field%d", i);
	gmw->addField(fieldname, "scalar");
    }
    return TRUE;
}




boolean GridChoice::usePositions()
{
    if (this->grid4Option->getState())
	this->positions_was_on = this->positionsOption->getState();
    if (this->positionsOption->getState()) {
	XtManageChild (this->dimension_l);
	XtManageChild (this->dimension_s);
    } else {
	XtUnmanageChild (this->dimension_l);
	XtUnmanageChild (this->dimension_s);
    }
    return TRUE;
}




boolean GridChoice::setGridType()
{
    this->setCommandActivation();
    return this->usePositions();
}


//
// file_checked means that the caller has looked for the file and found
// that is exists.
//
void GridChoice::setCommandActivation(boolean file_checked)
{
boolean file_ready = FALSE;
char* fname = NUL(char*);

    if (this->recursive) return ;

    boolean is_general;
    char *gen_ext = ".general";
    int gen_len = strlen(gen_ext);
    char *general_file = NUL(char*);

    //
    // Does the text widget at the top contain a valid data file name?
    //
    if (!file_checked) {
	const char *cp = this->gcw->getDataFilename();
	if ((cp) && (cp[0])) {
	    const char *file_name = fname = 
		theGARApplication->resolvePathName(cp, this->getFileExtension());
	    if (file_name) {
		file_ready = TRUE;
		int len = strlen(cp);
		if (len > gen_len) {
		    is_general = (strcmp (gen_ext, &cp[len-gen_len]) == 0);
		    general_file = DuplicateString(file_name);
		    if (is_general) {
			this->recursive = TRUE;
			theGARApplication->openGeneralFile(general_file);
			this->recursive = FALSE;
		    }
		}
	    }
	}
    } else
	file_ready = TRUE;

    char msg[128];
    GARMainWindow *gmw = theGARApplication->getMainWindow();
    if (file_ready) {
        if (this->isBrowsable()) {
            this->browseCmd->activate();
        } else {               
            sprintf (msg, "The file is available but is not ascii.");
            this->browseCmd->deactivate(msg);
        }
        if (this->isTestable()) {
            if ((!gmw) || (theGARApplication->isDirty() == TRUE)) {
                if (!gmw)
                    sprintf (msg,
		       "You must create a .general file, first.  Use the full prompter.");
                else
                    sprintf (msg, 
			"You must save the .general file you have already started.");
                this->verifyDataCmd->deactivate(msg);
            } else {
                this->verifyDataCmd->activate();
            }
        } else {
            sprintf (msg, "Only files which the import module can read can be tested.");
            this->verifyDataCmd->deactivate(msg);
        }

        if (this->visualizable) {
            if  ((!gmw) || (theGARApplication->isDirty() == TRUE)) {
                if (!gmw)
                    sprintf (msg,
		       "You must create a .general file, first.  Use the full prompter.");
                else
                    sprintf (msg, 
			"You must save the .general file you have already started.");
                this->visualizeCmd->deactivate(msg);
            } else {
		this->visualizeCmd->activate();
            }
        } else {
            sprintf (msg, "Only files which the import module can read can be tested.");
            this->visualizeCmd->deactivate(msg);
        }
	//
	// If the file changed, then toss out net_to_run
	//
	ASSERT(fname);

	boolean file_changed = theGARApplication->isDirty();
	if (!this->previous_data_file)
	    file_changed = TRUE;
	else if (!EqualString (this->previous_data_file, fname))
	    file_changed = TRUE;

	if (file_changed) {
	    if (this->net_to_run) delete this->net_to_run;
	    this->net_to_run = NUL(char*);
	    if (this->previous_data_file) delete this->previous_data_file;
	    this->previous_data_file = NUL(char*);
	}
    } else {
	if (this->net_to_run) {
	    delete this->net_to_run;
	    this->net_to_run = NUL(char*);
	    if (this->previous_data_file) delete this->previous_data_file;
	    this->previous_data_file = NUL(char*);
	}

	if (this->isBrowsable()) 
	    this->browseCmd->deactivate ("You must specify a data file name, first.");
	if (this->isTestable())
	    this->verifyDataCmd->deactivate ("You must specify a data file name, first.");
	if (this->isVisualizable())
	    this->visualizeCmd->deactivate ("You must specify a data file name, first.");
    }

    if ((this->grid1Option->getState()) || (this->grid2Option->getState())) {
	this->positionsOption->setState (FALSE, TRUE);
	this->positionsCmd->deactivate("Used only for warped-regular or scattered data");
    } else if (this->grid3Option->getState()) {
	this->positionsOption->setState (TRUE, TRUE);
	this->positionsCmd->deactivate("Your data must look like this: x1,y1,data1; x2,y2,data2; ...");
    } else {
	this->positionsOption->setState (this->positions_was_on, TRUE);
	this->positionsCmd->activate();
    }

    if (fname) {
	if (this->previous_data_file) delete this->previous_data_file;
	this->previous_data_file = DuplicateString(fname);
	delete fname;
    }

}

boolean GridChoice::verify(const char *seek)
{
    //
    // Before doing anything, check for a filename.  Normally, the visualize,
    // and verify buttons are grey if there isn't a .general file to work on
    // however, if the user presses File/New, then the prompter is clean and
    // has no file.  If that's the case, then just issue an error.
    //
    GARMainWindow *gmw = theGARApplication->getMainWindow();
    ASSERT(gmw);
    const char *cp = gmw->getFilename();
    if ((!cp) || (!cp[0])) {
	ErrorMessage("You must save a valid general file, first.");
	return FALSE;
    }

    if (!this->connect(TypeChoice::ImageMode)) return FALSE;
    const char *net_dir = theGARApplication->getResourcesNetDir();
    char net_file[512];
    sprintf (net_file, "%s/ui/decision.net", net_dir);
    DXLConnection* conn = this->gcw->getConnection();
    DXLLoadVisualProgram (conn, net_file);

    DXLSetString (conn, "test_file", cp);
    DXLSetString (conn, "test_format", this->getImportType());
    if (!seek) {
        DXLSetValue (conn, "DescribeOrVisualize", "2");
	//
	// Commented out because wer're in ImageMode and dxui will get the
	// messages. They won't be shipped here.
        //this->gcw->showMessages();
    } else {
        DXLSetValue (conn, "DescribeOrVisualize", seek);
    }

    DXLExecuteOnce (conn);

    return TRUE;
}


//
// Decision tree for picking a net to run:
//
boolean GridChoice::visualize()
{
    char net_file[512];
    const char *net_dir = theGARApplication->getResourcesNetDir();
    if (!this->net_to_run) {
        return this->verify("1");
    }

    if (!this->connect(TypeChoice::ImageMode)) return FALSE;
    DXLConnection* conn = this->gcw->getConnection();

    GARMainWindow *gmw = theGARApplication->getMainWindow();
    ASSERT(gmw);

    sprintf (net_file, "%s/ui/%s", net_dir, this->net_to_run);
    char *cp = GARApplication::FileFound(net_file, NUL(char*));
    if ((cp) && (cp[0])) {
        char* args[4];
        args[0] = "_filename_";
        args[1] = DuplicateString(gmw->getFilename());
        args[2] = "_filetype_";
        args[3] = DuplicateString(this->getImportType());
        this->gcw->loadAndSet (conn, net_file, args, 4);
        delete args[1];
        delete args[3];
        uiDXLOpenAllImages (conn);

        this->hideList();
        DXLExecuteOnce(conn);
        delete cp;
        return TRUE;
    } else {
	WarningMessage ("Could not find %s", net_file);
	return FALSE;
    }
}


boolean GridChoice::canHandle (const char *ext)
{
    if ((!ext) || (!ext[0])) return FALSE;

    if (EqualString(ext, ".GENERAL")) return TRUE;
    if (EqualString(ext, ".general")) return TRUE;

    return FALSE;
}

boolean GridChoice::retainsControl(const char* new_file)
{
    GARMainWindow *gmw = theGARApplication->getMainWindow();
    if (!gmw) return FALSE;
    return TRUE;
}
