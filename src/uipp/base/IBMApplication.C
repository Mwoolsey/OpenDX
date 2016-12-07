/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"

#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif

#if defined(HAVE_SYS_STAT_H)
#include <sys/stat.h>
#endif

#include "IBMApplication.h"
#include "HelpMenuCommand.h"
#include "MainWindow.h"
#include "../widgets/XmDX.h"
#include "../widgets/findcolor.h"
#include "HelpWin.h"
#include "ErrorDialogManager.h"
#include "WarningDialogManager.h"
#include "DXStrings.h"
#include "IBMVersion.h"
#include "lex.h"
#include "logo.h"
#include "icon50.h"
#include "ListIterator.h"
#include "StartWebBrowser.h"

#if defined (HAVE_XPM_H)
#include <xpm.h>
#endif
#if defined (HAVE_X11_XPM_H)
#include <X11/xpm.h>
#endif

IBMApplication *theIBMApplication = NULL;
IBMResource IBMApplication::resource;

XmColorProc IBMApplication::DefColorProc = NULL;

static
XrmOptionDescRec _IBMOptionList[] =
{
    {
	"-uiroot",
	"*UIRoot",
	XrmoptionSepArg,
	NULL
    },
    {
	"-wizard",
	"*wizard",
	XrmoptionNoArg,
	"True"
    },
    {
	"-dismissedWizards",
	"*dismissedWizards",
	XrmoptionSepArg,
	NULL
    },
};


static
XtResource _IBMResourceList[] =
{
    {
	"UIRoot",
	"Pathname",
	XmRString,
	sizeof(String),
	XtOffset(IBMResource*, UIRoot),
	XmRString,
	NULL
    },
    {
	"wizard",
	"Flag",
	XmRBoolean,
	sizeof(Boolean),
	XtOffset(IBMResource*, wizard),
	XmRImmediate,
	(XtPointer)False
    },
    {
	"dismissedWizards",
	"Pathname",
	XmRString,
	sizeof(String),
	XtOffset(IBMResource*, noWizardNames),
	XmRString,
	(XtPointer)NULL
    },

};

const String IBMApplication::DefaultResources[] =
{
    "*background:              #b4b4b4b4b4b4",
    "*foreground:              black",
#if defined(DXD_OS_NON_UNIX)
    "*fontList:\
-adobe-helvetica-bold-r-normal--12-*iso8859-1=bold,\
-adobe-helvetica-bold-r-normal--14-*iso8859-1=canvas,\
-adobe-helvetica-bold-r-normal--10-*=small_bold,\
-adobe-helvetica-bold-r-normal--12-*=small_canvas,\
-adobe-helvetica-bold-r-normal--14-*=big_bold,\
-adobe-helvetica-bold-r-normal--20-*=huge_bold,\
-adobe-helvetica-medium-r-normal--12-*=normal,\
-adobe-helvetica-medium-r-normal--10-*=small_normal,\
-adobe-helvetica-medium-r-normal--14-*=big_normal,\
-adobe-helvetica-medium-r-normal--20-*=huge_normal,\
-adobe-helvetica-medium-o-normal--10-*=small_oblique,\
-adobe-helvetica-medium-o-normal--14-*=big_oblique,\
-adobe-helvetica-medium-o-normal--20-*=huge_oblique,\
-adobe-helvetica-medium-o-normal--12-*=oblique",
#else
    "*fontList:\
-adobe-helvetica-bold-r-normal--12-*iso8859-1=bold,\
-adobe-helvetica-bold-r-normal--14-*iso8859-1=canvas,\
-adobe-helvetica-bold-r-normal--10-*iso8859-1=small_bold,\
-adobe-helvetica-bold-r-normal--12-*iso8859-1=small_canvas,\
-adobe-helvetica-bold-r-normal--14-*iso8859-1=big_bold,\
-adobe-helvetica-bold-r-normal--20-*iso8859-1=huge_bold,\
-adobe-helvetica-medium-r-normal--12-*iso8859-1=normal,\
-adobe-helvetica-medium-r-normal--10-*iso8859-1=small_normal,\
-adobe-helvetica-medium-r-normal--14-*iso8859-1=big_normal,\
-adobe-helvetica-medium-r-normal--20-*iso8859-1=huge_normal,\
-adobe-helvetica-medium-o-normal--10-*iso8859-1=small_oblique,\
-adobe-helvetica-medium-o-normal--14-*iso8859-1=big_oblique,\
-adobe-helvetica-medium-o-normal--20-*iso8859-1=huge_oblique,\
-adobe-helvetica-medium-o-normal--12-*iso8859-1=oblique",
#endif
    "*keyboardFocusPolicy:     explicit",
    "*highlightOnEnter:	       false",
    "*highlightThickness:      0",
    "*XmPushButton*traversalOn:    false",
    "*XmPushButtonGadget*traversalOn:    false",
    "*XmToggleButton*traversalOn:  false",
    "*XmDial*width:                100",
    "*XmDial*height:	           100",
    "*XmDial*numMarkers:           60",
    "*XmDial*majorMarkerWidth:     10",
    "*XmDial*minorMarkerWidth:     5",
    "*XmDial*majorPosition:        5",
    "*XmDial*startingMarkerPos:    0",
    "*XmDial*shadePercentShadow:   0",
    "*XmDial*shading:              True",
    "*XmDial*shadowThickness:      0",
    "*XmDial*shadeIncreasingColor: #b4b4c9c9dddd",
    "*XmDial*shadeDecreasingColor: #7e7e7e7eb4b4",
    "*XmDial*majorMarkerColor:     Black",
    "*XmDial*minorMarkerColor:     Black",
    "*XmDial*indicatorColor:       Black",
    "*XmDial*indicatorWidth:       20",

    "*XmNumber.navigationType:     XmTAB_GROUP",

    "*XmScrollBar*foreground:      #b4b4b4b4b4b4",

    "*XmSlideBar*alignOnDrop:      True",

    "*XmToggleButton.selectColor:  #5F9EA0",
    "*XmToggleButton.indicatorSize:15",

    "*XmArrowButton.shadowThickness:         1",
    "*XmCascadeButton.shadowThickness:       1",
    "*XmCascadeButtonGadget.shadowThickness: 1",
    "*XmColorMapEditor*shadowThickness:      1",
    "*XmDrawnButton.shadowThickness:         1",
    "*XmForm.shadowThickness:                0",
    "*XmFrame.shadowThickness:               1",
    "*XmList.shadowThickness:                1",
    "*XmMenuBar.shadowThickness:             1",
    "*XmNumber*shadowThickness:              1",
    "*XmPushButton.shadowThickness:          1",
    "*XmPushButtonGadget.shadowThickness:    1",
    "*XmRowColumn.shadowThickness:           1",
    "*XmScrollBar.shadowThickness:           1",
    "*XmScrolledList.shadowThickness:        1",
    "*XmScrolledWindow.shadowThickness:      1",
    "*XmText.shadowThickness:                1",
    "*XmTextField.shadowThickness:           1",
    "*XmToggleButton.shadowThickness:        1",
    NULL
};

XtActionsRec IBMApplication::actions[] = {
    {"IBMButtonHelp", (XtActionProc)IBMApplication_IBMButtonHelpAP}
};


IBMApplication::IBMApplication(char* className): Application(className) 
{
    theIBMApplication = this;
    this->helpWindow = NULL;
    this->logo_pmap = XtUnspecifiedPixmap;
    this->icon_pmap = XtUnspecifiedPixmap;
    this->genericHelpCmd  = new HelpMenuCommand("genericAppHelp",
				NULL,
				TRUE,HelpMenuCommand::GenericHelp); 
    this->helpTutorialCmd = new HelpMenuCommand("helpTutorial",
				NULL, 
				TRUE,
				HelpMenuCommand::HelpTutorial);

    this->aboutAppString = NULL;

    this->noWizards = NUL(List*);
    this->techSupportString = NUL(char*);
}


IBMApplication::~IBMApplication()
{
    theIBMApplication = NULL;

#ifdef __PURIFY__
    delete this->genericHelpCmd;
    delete this->helpTutorialCmd;
    if (this->helpWindow)
        delete this->helpWindow;
    if (this->aboutAppString)
	delete[] this->aboutAppString;
    if (this->techSupportString)
	delete[] this->techSupportString;

    if (this->noWizards) {
	ListIterator it(*this->noWizards);
	char *nowiz;
	while ( (nowiz = (char*)it.getNext()) )
	    delete[] nowiz;
	delete this->noWizards;
	this->noWizards = NUL(List*);
    }
#endif
}

//
// Install the default resources for this class.
//
void IBMApplication::installDefaultResources(Widget baseWidget)
{
    this->setDefaultResources(baseWidget, IBMApplication::DefaultResources);
    this->Application::installDefaultResources(baseWidget);
}

//
// Motif has opened up its scheme for calculating shadow colors.  The routine
// XmGetColors() tells what motif will use given a particaluar background
// pixel.  I have replaced Motif's own color calculation with ::ColorProc.
// As a result, if the normal color calculation proc (saved in ::DefColorProc)
// fails, then I use find_color to find replacements.  The alternative would
// have been to get stuck with crap (or probably just white for most color
// resources).
// So we do what Motif normally does.  Then error check the results the way Motif
// doesn't.  If the hoped-for colors aren't available then find colors that are.
// -Martin 5/25/95
//
extern "C"
void
IBMApplication_ColorProc (XColor *bgdef, XColor *fgdef,
        XColor *seldef, XColor *tsdef, XColor *bsdef)
{
Display *d = theApplication->getDisplay();
Widget rw = theApplication->getRootWidget();
XColor cdef, *defs[4];
int i;
Colormap colormap;
Status success;

    IBMApplication::DefColorProc (bgdef, fgdef, seldef, tsdef, bsdef);
    XtVaGetValues (rw, XmNcolormap, &colormap, NULL);
    ASSERT(colormap);
    defs[0] = fgdef; defs[1] = seldef;
    defs[2] = tsdef; defs[3] = bsdef;

    for (i=0; i<4; i++) {
	success = XAllocColor (d, colormap, defs[i]);
	if (!success) {
	    cdef.red = defs[i]->red; cdef.blue = defs[i]->blue;
	    cdef.green = defs[i]->green;
	    find_color (rw, &cdef);
	    success = XAllocColor(d, colormap, &cdef); 
	    if (success) {
		memcpy (defs[i], &cdef, sizeof(XColor));
	    } 
	}
    }
}


// String to Pixel type converter
extern "C" Boolean IBMApplication_String2Pixel (Display *d, XrmValue [], Cardinal ,
	XrmValue *from, XrmValue *to, XtPointer *closure)
{
static XColor cdef;
Colormap colormap;
Screen *screen;
int status;

    String colorname = (String)from[0].addr;
    Widget self = theApplication->getRootWidget();
    XtVaGetValues (self, XmNcolormap, &colormap, XmNscreen, &screen, NULL);
    ASSERT (colormap);
    ASSERT (screen);

    if (!strcmp(colorname, XtDefaultForeground)) colorname = "Black";
    else if (!strcmp(colorname, XtDefaultBackground)) colorname = "#b4b4b4b4b4b4";

    status = XParseColor (d, colormap, colorname, &cdef);
    if (!status) {
	char errmsg[128];
	sprintf (errmsg, "Unrecognized color entry: %s\n", colorname);
	XtWarning (errmsg);
	*closure = (XtPointer)False;
	return False;
    }

    status = XAllocColor (d, colormap, &cdef);
    if (!status) {
	find_color (self, &cdef);
	Pixel tmp = cdef.pixel;
	status = XAllocColor (d, colormap, &cdef);
	if (!status)
	    cdef.pixel = tmp;
    }

    *closure = (XtPointer)True;
    if (to->addr != NULL) {
	if (to->size < sizeof(Pixel)) {
	    to->size = sizeof(Pixel);
	    return False;
	}
	*(Pixel*)(to->addr) = cdef.pixel;
    } else {
	to->addr = (XPointer )&cdef.pixel;
    }
    to->size = sizeof(Pixel);
    return True;
}


boolean IBMApplication::initializeWindowSystem(unsigned int *argcp, char **argv)
{

    if (!this->Application::initializeWindowSystem(argcp, argv))
        return FALSE;

#if defined(DEBUG) && defined(BETA_VERSION)
    fprintf(stderr,"This is a BETA version. Undef BETA_VERSION"
			" (in IBMVersion.h) before shipping\n");
#endif


    XtAppSetTypeConverter (this->applicationContext, XmRString, XmRPixel,
	(XtTypeConverter)IBMApplication_String2Pixel, NULL, 0,
	XtCacheAll, NULL);

    // By setting Motif's color calculation proc, we can ensure that
    // we'll wind up using reasonable shadows when the shadows Motif
    // would normally use, aren't available.
    IBMApplication::DefColorProc =
	XmSetColorCalculation ((XmColorProc)IBMApplication_ColorProc);


    return TRUE;
}

boolean IBMApplication::initialize(unsigned int* argcp,
				   char**        argv)
{
    if (!this->Application::initialize(argcp,argv))
        return FALSE;

    this->parseCommand(argcp, argv, _IBMOptionList, XtNumber(_IBMOptionList));

    //
    // Get application resources.
    //
    this->getResources((XtPointer)&IBMApplication::resource,
		       _IBMResourceList,
		       XtNumber(_IBMResourceList));

    if (IBMApplication::resource.UIRoot == NULL) {
	char *s = getenv("DXROOT");
	if (s) 
	    // POSIX says we better copy the result of getenv(), so...
	    // This will show up as a memory leak, not worth worrying about
	    IBMApplication::resource.UIRoot = DuplicateString(s); 
	else
	    IBMApplication::resource.UIRoot =  "/usr/local/dx";
    }

    this->initIcon();

    this->parseNoWizardNames();

    return TRUE;
}

HelpWin *IBMApplication::newHelpWindow()
{
    return new HelpWin();
}
void IBMApplication::helpOn(const char *topic)
{
    if (!this->helpWindow) {
        this->helpWindow = this->newHelpWindow();
    }
    this->helpWindow->helpOn(topic);
}

void IBMApplication::addActions()
{
    this->Application::addActions();
    XtAppAddActions(this->applicationContext, IBMApplication::actions,
        sizeof(IBMApplication::actions)/sizeof(IBMApplication::actions[0]));
}

extern "C" void IBMApplication_IBMButtonHelpAP(
    Widget   widget,
    XEvent*   /* event */,
    String*   /* params */,
    Cardinal* /* num_params */)
{

    MainWindowHelpCallbackStruct callData;

#if FIXME
    if (!theDXApplication->appAllowsDXHelp())
        return;
#endif

    while (widget)
    {
        if (XtHasCallbacks(widget, XmNhelpCallback) == XtCallbackHasSome)
        {
            callData.reason = DxCR_HELP;
            callData.event  = NULL;
            callData.widget = widget;
            XtCallCallbacks
                (widget,
                 XmNhelpCallback,
                 (XtPointer)&callData);
            break;
        }
        else
        {
            widget = XtParent(widget);
        }
    }
}


//
// Get the command string that will start the tutorial.
//
const char *IBMApplication::getStartTutorialCommandString()
{
    return "dx -tutor";
}

//
// Start the tutorial on behalf of the application.
//
boolean IBMApplication::startTutorial()
{
    char *url = new char[strlen(getUIRoot()) + 35];
    strcpy(url, "file://");
    strcat(url, getUIRoot());
    strcat(url, "/html/pages/qikgu011.htm");
    if(!_dxf_StartWebBrowserWithURL(url))
	system("dx -tutor");
    delete[] url;

    return TRUE;
}
void IBMApplication::handleXtWarning(char *message)
{
    this->Application::handleXtWarning(message);
    if(strstr(message, "Could not convert X KEYSYM to a keycode"))
    {
	fprintf(stderr, "XtWarning: %s\n", message);
	fprintf(stderr, "Please see the README file for Data Explorer for a work-around.\n\n");
    }
}

//
// If there is a copyright notice, we create the Logo and Icon data structures
// and then call the super class method,  otherwise, we just return.
//
void IBMApplication::postCopyrightNotice()
{
    const char *s = this->getCopyrightNotice();
    if (s) 	
	this->initLogo();

    this->Application::postCopyrightNotice();
}

void IBMApplication::initLogo()
{
int		i,k;
unsigned long	ndx;
int		x,y;
Display 	*d;
Colormap	cmap;
XColor		xcolor2[256];
unsigned long	rmult, gmult, bmult;
XImage 		*ximage;
GC		gc;
unsigned char	*logo_data;
FILE	*fp;
size_t  size;
char	s[256];

#if defined(HAVE_LIBXPM)
    Widget w;
    int err;
    w = theIBMApplication->getRootWidget();

    sprintf(s, "%s/ui/logo.xpm", theIBMApplication->getUIRoot());
    err = XpmReadFileToPixmap(XtDisplay(w),  
        RootWindowOfScreen(XtScreen(w)),
	s, &this->logo_pmap, NULL, NULL);
    if (err != XpmSuccess) {

#endif
    size = LOGO_WIDTH*LOGO_HEIGHT;
    logo_data = (unsigned char *)XtMalloc(size);
    sprintf(s,"%s/ui/logo.dat", this->getUIRoot());

    if((fp = fopen(s, "r")) == NULL)
    {
	WarningMessage("Unable to open logo data");
	XtFree((char *)logo_data);
	return;
    }
    if(fread((char *)logo_data, sizeof(char), size, fp) != size)
    {
	WarningMessage("Unable to read logo data");
	XtFree((char *)logo_data);
	return;
    }
    fclose(fp);
    d = XtDisplay(this->getRootWidget());
    XtVaGetValues (this->getRootWidget(), 
	XmNcolormap, &cmap, 
    NULL);

    this->logo_pmap = 
	XCreatePixmap(d, RootWindowOfScreen(XtScreen(this->getRootWidget())), 
	    LOGO_WIDTH, LOGO_HEIGHT, 
	    DefaultDepthOfScreen(XtScreen(this->getRootWidget())));
    ximage = XGetImage(d, this->logo_pmap, 0, 0, 
	LOGO_WIDTH, LOGO_HEIGHT, AllPlanes, ZPixmap);

    Visual *xvi = DefaultVisualOfScreen(XtScreen(this->getRootWidget()));
    XVisualInfo vinfotemplate;
    vinfotemplate.visualid = XVisualIDFromVisual(xvi);
    int nReturn;
    XVisualInfo *xvinfo = XGetVisualInfo(XtDisplay(this->getRootWidget()),
                              VisualIDMask, &vinfotemplate, &nReturn);
 
    if (xvinfo->c_class == PseudoColor || xvinfo->c_class == TrueColor)
    {
	unsigned char *l;
	Boolean failed = False;
	this->num_colors = 0;
	for(i = 0; i < LUT_SIZE; i++)
	{
	    if(XAllocColor(d, cmap, &logo_lut[1][i]) && !failed)
	    {
		this->num_colors++;
	    }
	    else
	    {
		failed = True;
		find_color(this->getRootWidget(), &logo_lut[1][i]);
	    }
	}
	for (i = 0; i < LUT_SIZE; i++)
	{
	    ndx = logo_lut[0][i].pixel;
	    xcolor2[ndx] = logo_lut[1][i];
	}
	for(y = 0, l = logo_data; y < LOGO_HEIGHT; y++ )
	    for(x = 0; x < LOGO_WIDTH; x++ )
		XPutPixel(ximage, x, y, xcolor2[*l++].pixel);
    }
    else if(xvinfo->c_class == DirectColor)
    {
	Visual *xvi = DefaultVisualOfScreen(XtScreen(this->getRootWidget()));
	rmult = xvi->red_mask & (~xvi->red_mask+1);
	gmult = xvi->green_mask & (~xvi->green_mask+1);
	bmult = xvi->blue_mask & (~xvi->blue_mask+1);

	for (i = 0; i < LUT_SIZE; i++)
	{
	    ndx = logo_lut[0][i].pixel;
	    xcolor2[ndx].red =   logo_lut[0][i].red >> 8;
	    xcolor2[ndx].green = logo_lut[0][i].green >> 8;
	    xcolor2[ndx].blue =  logo_lut[0][i].blue >> 8;
	    xcolor2[ndx].pixel = 
				xcolor2[ndx].red * rmult +
				xcolor2[ndx].green * gmult +
				xcolor2[ndx].blue * bmult;
	}

	k = 0;
	for(y = 0; y < LOGO_HEIGHT; y++)
	{
	    for(x = 0; x < LOGO_WIDTH; x++)
	    {
		ndx = logo_data[k++];
		XPutPixel(ximage, x, y, xcolor2[ndx].pixel);
	    }
	}
    }
    gc = XCreateGC(d, this->logo_pmap, 0, NULL);

    XPutImage(d, this->logo_pmap, gc, ximage, 0, 0, 0, 0, 
	LOGO_WIDTH, LOGO_HEIGHT);

    XtFree((char *)logo_data);
    XFreeGC(d, gc);
    XDestroyImage(ximage);
#if defined(HAVE_LIBXPM)
    }
#endif
}
void IBMApplication::cleanupLogo()
{
    Display *d;
    Colormap cmap;
    int     depth, i;
    unsigned long   pix[256];

    if (this->logo_pmap == XtUnspecifiedPixmap)
	return;

    d = XtDisplay(this->getRootWidget());

    XtVaGetValues (this->getRootWidget(), 
	XmNcolormap, &cmap, 
	XmNdepth, &depth,
    NULL);
    if (depth == 8)
    {

        for(i = 0; i < this->num_colors; i++)
            pix[i] = logo_lut[1][i].pixel;

        XFreeColors(d, cmap, &(pix[0]), this->num_colors, 0);
    }
    XFreePixmap(d, this->logo_pmap);
}

void IBMApplication::initIcon()
{
Display 	*d;
Colormap	cmap;
XImage		*ximage;
GC		gc;
unsigned char	*icon_data;
FILE		*fp;
size_t 		size;
int		k;
int		x,y;
char		s[256];

#if defined(HAVE_LIBXPM)
    Widget w;
    int err;
    w = theIBMApplication->getRootWidget();

    sprintf(s, "%s/ui/icon50.xpm", theIBMApplication->getUIRoot());
    err = XpmReadFileToPixmap(XtDisplay(w),  
        RootWindowOfScreen(XtScreen(w)),
	s, &this->icon_pmap, NULL, NULL);
    if (err != XpmSuccess) {

#endif
    size = ICON_WIDTH*ICON_HEIGHT;
    icon_data = (unsigned char *)XtMalloc(size);

    sprintf(s,"%s/ui/icon50.dat", theIBMApplication->getUIRoot());
    if((fp = fopen(s, "r")) == NULL)
    {
        WarningMessage("Unable to open icon data\n");
	XtFree((char *)icon_data);
        return;
    }
    if(fread((char *)icon_data, sizeof(char), size, fp) != size)
    {
        WarningMessage("Unable to read icon data\n");
	XtFree((char *)icon_data);
        return;
    }
    fclose(fp);

    d = XtDisplay(theIBMApplication->getRootWidget());
    XtVaGetValues (this->getRootWidget(), XmNcolormap, &cmap, NULL);

    this->icon_pmap = 
	XCreatePixmap(d, RootWindowOfScreen(XtScreen(this->getRootWidget())), 
	    ICON_WIDTH, ICON_HEIGHT, 1);
    ximage = 
	XGetImage(d, this->icon_pmap, 0, 0, 
	    ICON_WIDTH, ICON_HEIGHT, AllPlanes, ZPixmap);

    k = 0;
    for(y = 0; y < ICON_HEIGHT; y++)
    {
	for(x = 0; x < ICON_WIDTH; x++)
	{
	    XPutPixel(ximage, x, y, icon_data[k++]);
	}
    }

    gc = XCreateGC(d, this->icon_pmap, 0, NULL);

    XPutImage(d, this->icon_pmap, gc, ximage, 
	      0, 0, 0, 0, ICON_WIDTH, ICON_HEIGHT);

    XFreeGC(d, gc);
    XDestroyImage(ximage);
    XtFree((char *)icon_data);
#if defined(HAVE_LIBXPM)
    }
#endif
}
void IBMApplication::getVersionNumbers(int *maj, int *min, int *mic)
{
   *maj = IBM_MAJOR_VERSION;
   *min = IBM_MINOR_VERSION;
   *mic = IBM_MICRO_VERSION;
}
//
// Get the 'about' string as reported from 'About...' menu option
//
const char *IBMApplication::getAboutAppString()
{

    if (!this->aboutAppString) {
	int a,b,c;
	this->getVersionNumbers(&a,&b,&c);
        this->aboutAppString = new char[1024];
        sprintf(this->aboutAppString,
		"Product : %s \n"
#if defined(BETA_VERSION)
		"Version : %d.%d.%d Beta\n"
#else
		"Version : %d.%d.%d\n"
#endif
		"Dated   : %s",
                        this->getFormalName(),
			a,b,c,	
                        __DATE__);
    }
    return this->aboutAppString;
}

//
// Get the 'technical support' string as reported from 'Technical Support...' 
// menu option
//
const char *IBMApplication::getTechSupportString()
{
    if (!this->techSupportString) {
	const char *dxroot = this->getUIRoot();
	const char *nosup = "No Technical Support Available";
	if (!dxroot) {
	    this->techSupportString = DuplicateString(nosup);
	    return this->techSupportString;
	}

	char supfile[1024];
	sprintf(supfile,"%s/ui/support.txt",dxroot);
	FILE *fp;
	int helpsize;

	struct STATSTRUCT buf;
       
	if (STATFUNC(supfile, &buf) != 0) {
	    this->techSupportString = DuplicateString(nosup);
	    return this->techSupportString;
	}
	helpsize = buf.st_size;
	
	if (!(fp = fopen(supfile,"r"))) {
	    this->techSupportString = DuplicateString(nosup);
	    return this->techSupportString;
	}

	this->techSupportString = new char[helpsize + 1];
	if (!this->techSupportString) {
	    this->techSupportString = DuplicateString(nosup);
	    return this->techSupportString;
	}
	fread(this->techSupportString,1,helpsize,fp);
	this->techSupportString[helpsize] = '\0';
	fclose(fp);
    }

    return this->techSupportString;
}

//
// Get the name of the directoy that contains the help files.
// This returns $UIRoot/help
//
const char *IBMApplication::getHelpDirectory()
{
    static char *helpDir = NULL;

    if (!helpDir)
    {
        const char *root = this->getUIRoot();
        helpDir = new char[STRLEN(root) + strlen("/help") + 1];
        sprintf(helpDir, "%s/help", root);
    }
    return helpDir;
}
const char *IBMApplication::getHTMLDirectory()
{
    static char *htmlDir = NULL;

    if (!htmlDir)
    {
        const char *root = this->getUIRoot();
        htmlDir = new char[STRLEN(root) + strlen("/html") + 1];
        sprintf(htmlDir, "%s/html", root);
    }
    return htmlDir;
}
const char *IBMApplication::getTmpDirectory(boolean bList)
{
#ifndef MAX_DIR_PATH_LIST
#define MAX_DIR_PATH_LIST  255
#endif
    static char *tmpDir = NULL, tmpDirList[MAX_DIR_PATH_LIST];
    static int beingHere = 0;
    char	*p;

#ifdef DXD_OS_NON_UNIX
    if(!beingHere) {
	strcpy(tmpDirList, "./;");
	tmpDir = "./";

	p = getenv("TEMP");	//	2nd Pref
	if(p) {
	    tmpDir = p;
	    strcat(tmpDirList, p);
	    strcat(tmpDirList, ";");
	}

	p = getenv("TMP");	//	1st Pref.
	if(p) {
	    tmpDir = p;
	    strcat(tmpDirList, p);
	    strcat(tmpDirList, ";");
	}
	beingHere = 1;
	DOS2UNIXPATH(tmpDir);
	DOS2UNIXPATH(tmpDirList);
    }
    if (bList) {
	return tmpDirList;
    }
    else {
	return tmpDir;
    }
#else
    if(!beingHere) {
	strcpy(tmpDirList, "/tmp;");
	tmpDir = "/tmp";

	p = getenv("TMP");	//	2nd Pref
	if(p) {
	    tmpDir = p;
	    strcat(tmpDirList, p);
	    strcat(tmpDirList, ";");
	}

	p = getenv("TMPDIR");	//	1st Pref
	if(p) {
	    tmpDir = p;
	    strcat(tmpDirList, p);
	    strcat(tmpDirList, ";");
	}
	beingHere = 1;
    }
    if (bList)
	return tmpDirList;
    else
	return tmpDir;
#endif
}

//
// Application Resources
//
boolean IBMApplication::getApplicationDefaultsFileName(char* res_file, boolean create)
{
    //
    // Print the resource database to the file.
    //
#ifdef DXD_OS_NON_UNIX
const char* class_name = this->getApplicationClass();
    char* home = (char*)getenv("XAPPLRESDIR");
    if (!home || !strlen(home)) {
	home = (char*)this->resource.UIRoot;
	if (!home || !strlen(home)) {
	    sprintf(res_file, "/%s-AD", class_name);
	} else {
	    sprintf(res_file, "%s/ui/%s-AD", home, class_name);
	}
    } else {
	sprintf(res_file, "%s/ui/%s-AD", home, class_name);
    }

    return this->isUsableDefaultsFile(res_file, create);
#else
    return this->Application::getApplicationDefaultsFileName(res_file, create);
#endif
}

//
// Open $HOME/DX, search for DX*dismissedWizards:.   If found, replace it,
// otherwise append the DX*dismissedWizards: and the contents of noWizards.
//
void IBMApplication::printResource(const char* resource, const char* value)
{
const char* class_name = this->getApplicationClass();

    //
    // Create one line which specifies the new resource setting.
    //
    int totlen = 0;
    char resource_line[4096];
    sprintf (resource_line, "%s*%s: %s\n", class_name, resource, value);

    char res_file[256];
    if (this->getApplicationDefaultsFileName(res_file, TRUE)) {
	// Here, it would be nice to use a function like
	// XrmRemoveLineResource() but I can't find any
	// such beast.  If one were available, then when
	// there is no value for a resource, the spec
	// could be removed from the file.  This way
	// we store a nothing spec in the file.
	XrmDatabase db = XrmGetFileDatabase(res_file);
	XrmPutLineResource (&db, resource_line);
	XrmPutFileDatabase (db, res_file);
	XrmDestroyDatabase(db);
    }
}


//
// W I Z A R D S     W I Z A R D S     W I Z A R D S
// W I Z A R D S     W I Z A R D S     W I Z A R D S
// W I Z A R D S     W I Z A R D S     W I Z A R D S
//
#define NAME_SEP ','

//
// The wizard name list is a colon separated list of names.
//
void IBMApplication::parseNoWizardNames()
{
    if (this->resource.noWizardNames == NUL(char*)) return ;

    this->noWizards = new List;

    //
    // Change the separators to \0
    //
    char* nwn = DuplicateString(this->resource.noWizardNames);
    int len = strlen(nwn);
    int names = 0;
    int i;
    boolean trailing_name_sep = FALSE;
    for (i=0; i<len; i++) {
	if (nwn[i] == NAME_SEP) {
	    if (i == (len-1)) 
		trailing_name_sep = TRUE;
	    nwn[i] = '\0';
	    names++;
	}
    }
    if (!trailing_name_sep) names++;

    //
    // Append each name in the big string to a list.
    //
    char *cp = nwn;
    for (i=0; i<names; i++) {
	char *name = DuplicateString(cp);
	this->noWizards->appendElement((void*)name);
	cp+= 1+strlen(name);
    }

    delete[] nwn;
}

//
// Add 1 name to the list and update the resource value in $HOME/DX file
// dxui will automatically read the contents of $HOME/DX when it starts up thereby
// setting the list of names which should get no wizard.
//
void IBMApplication::appendNoWizardName(const char* nowiz)
{
    char* name = DuplicateString(nowiz);
    if (!this->noWizards)
	this->noWizards = new List;
    this->noWizards->appendElement((void*)name);
    this->printNoWizardNames();
}


//
// Open $HOME/DX, search for DX*dismissedWizards:.   If found, replace it,
// otherwise append the DX*dismissedWizards: and the contents of noWizards.
//
void IBMApplication::printNoWizardNames()
{
const char* class_name = this->getApplicationClass();

    char res_file[256];
    if (!this->getApplicationDefaultsFileName(res_file,TRUE)) {
	//
	// Probably should complain somehow, but the user is not in a position
	// to deal with this sort of complaint.  This happens when the user
	// clicks the 'don't show this help message anymore' button that only
	// appears as a result of running with -wizard or thru startupui.
	//
	return ;
    }

    //
    // Create one line which specifies the new resource setting.
    //
    int totlen = 0;
    char *resource_fmt = "%s*dismissedWizards: %s";
    char* name;
    ListIterator it(*this->noWizards);
    while ( (name = (char*)it.getNext()) ) {
	totlen+= 1+strlen(name);
    }

    totlen+= 32; 
    char *name_list = new char[totlen];

    totlen+= strlen(resource_fmt);

    char *resource_line = new char[totlen];

    it.setList(*this->noWizards);
    int nl_os = 0;
    while ( (name = (char*)it.getNext()) ) {
	strcpy (&name_list[nl_os], name);
	nl_os+= strlen(name);
	name_list[nl_os++] = NAME_SEP;
	name_list[nl_os] = '\0';
    }
    if (name_list[nl_os-1] == NAME_SEP) {
	nl_os--;
	name_list[nl_os] = '\0';
    }
    sprintf (resource_line, resource_fmt, class_name, name_list);

    //
    // Print the resource database to the file.
    //

    XrmDatabase db = XrmGetFileDatabase(res_file);
    XrmPutLineResource (&db, resource_line);
    XrmPutFileDatabase (db, res_file);
    XrmDestroyDatabase(db);
    delete[] resource_line;
}

boolean IBMApplication::isWizardWindow(const char* name)
{
    if (this->inWizardMode() == FALSE) return FALSE;

    if (!this->noWizards) return TRUE;

    ListIterator it(*this->noWizards);
    char *wiz;
    while ( (wiz = (char*)it.getNext()) ) {
	if (EqualString(wiz, name)) return FALSE;
    }

    return TRUE;
}

Pixmap IBMApplication::getLogoPixmap(boolean create_if_necessary)
{
    if ((this->logo_pmap == XtUnspecifiedPixmap) && (create_if_necessary == TRUE))
	this->initLogo();
    return this->logo_pmap;
}
