/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"



#ifdef OS2
#include <stdlib.h>
#include <types.h>
#endif

#if defined(DXD_WIN) || defined(OS2)          //SMH get socket calls
#undef send
#undef FLOAT
#undef PFLOAT
#endif

#if defined(windows) && defined(HAVE_WINSOCK_H)
#include <winsock.h>
#elif defined(HAVE_CYGWIN_SOCKET_H)
#include <cygwin/socket.h>
#elif defined(HAVE_SYS_SOCKET_H)
#include <sys/socket.h>
#endif

#include "ImageNode.h"
#include "StandIn.h"
#include "Ark.h"
#include "Parameter.h"
#include "DXApplication.h"
#include "Network.h"
#include "PickNode.h"
#include "ImageWindow.h"
#include "ListIterator.h"
#include "ImageDefinition.h"
#include "WarningDialogManager.h"
#include "DXPacketIF.h"

#include "DXVersion.h"

#if WORKSPACE_PAGES
#include "ProcessGroupManager.h"
#endif

#ifdef ABS_IN_MATH_H
# define abs __Dont_define_abs
#endif
#include <math.h>
#ifdef ABS_IN_MATH_H
# undef abs
#endif

#include <ctype.h>


//
// Image Macro parameter numbers.
// n.b.  all AAxyz param numbers were remobed to ImageWindow.h
// so that they could be used in other .C files as well
//
#define OLD_USEVECTOR               1
#define OLD_OBJECT                  2
#define OLD_WHERE                   3
#define OLD_TO                      4
#define OLD_FROM                    5
#define OLD_WIDTH                   6
#define OLD_RESOLUTION              7
#define OLD_ASPECT                  8
#define OLD_UP                      9
#define OLD_OPTIONS                 10
#define OLD_AAENABLE                11
#define OLD_AALABELS                12
#define OLD_AATICKS                 13
#define OLD_AACORNERS               14
#define OLD_AAFRAME                 15
#define OLD_AAADJUST                16
#define OLD_AACURSOR                17
#define OLD_RECENABLE               18
#define OLD_RECFILE                 19
#define OLD_THROTTLE                20
#define OLD_RECFORMAT               21
#define OLD_PROJECTION              22
#define OLD_VIEWANGLE               23
#define OLD_BUTTON_STATE            24
#define OLD_BUTTON_UP_APPROX        25
#define OLD_BUTTON_DOWN_APPROX      26
#define OLD_BUTTON_UP_DENSITY       27
#define OLD_BUTTON_DOWN_DENSITY     28
#define OLD_RENDER_MODE             29
#define OLD_BACKGROUND              30
#define OLD_AAGRID                  31
#define OLD_AACOLORS                32
#define OLD_AAANNOTATION            33
#define OLD_AALABELSCALE            34
#define OLD_AAFONT                  35
#define OLD_DEFAULT_CAMERA          36
#define OLD_RESET_CAMERA            37

#define IMAGETAG		1
#define OBJECT                  2
#define WHERE                   3
#define USEVECTOR               4
#define TO                      5
#define FROM                    6
#define WIDTH                   7
#define RESOLUTION              8
#define ASPECT                  9
#define UP                      10
#define VIEWANGLE               11
#define PROJECTION              12
#define OPTIONS                 13
#define BUTTON_STATE            14
#define BUTTON_UP_APPROX        15
#define BUTTON_DOWN_APPROX      16
#define BUTTON_UP_DENSITY       17
#define BUTTON_DOWN_DENSITY     18
#define RENDER_MODE             19
#define DEFAULT_CAMERA          20
#define RESET_CAMERA            21
#define BACKGROUND              22
#define THROTTLE                23
#define RECENABLE               24
#define RECFILE                 25
#define RECFORMAT               26
#define RECRESOLUTION           27
#define RECASPECT               28
#define AAENABLE                29
#define AALABELS                30
#define AATICKS                 31
#define AACORNERS               32
#define AAFRAME                 33
#define AAADJUST                34
#define AACURSOR                35
#define AAGRID                  36
#define AACOLORS                37
#define AAANNOTATION            38
#define AALABELSCALE            39
#define AAFONT                  40
#define INTERACTIONMODE         41
#define IMAGENAME               42

//
// new AutoAxes parameters
//
#define AA_XTICK_LOCS           43
#define AA_YTICK_LOCS           44
#define AA_ZTICK_LOCS           45
#define AA_XTICK_LABELS         46
#define AA_YTICK_LABELS         47
#define AA_ZTICK_LABELS         48

//
// On behalf of Java Explorer
//
#define JAVA_OPTIONS		49

//
// These are input param numbers for the WebOptions macro.  They belong
// in WebOptionsNode.C but there isn't any way to make a subclass of Node
// just for a particular macro unless it's done in the style of ImageNode
// which seems like too much work in this case.  Since WebOptions is just
// a plain old macro created with the vpe, it's actually a MacroNode when
// the ui creates one.
//
#define WEB_FORMAT		1
#define WEB_FILE		2
#define WEB_ENABLED		3
#define WEB_COUNTERS		4
#define WEB_ORBIT		5
#define WEB_ID			6
#define WEB_IMGID		7
#define WED_ORBIT_DELTA		8

static int translateInputs[] =
{
    0,
    USEVECTOR,
    OBJECT,
    WHERE,
    TO,
    FROM,
    WIDTH,
    RESOLUTION,
    ASPECT,
    UP,
    OPTIONS,
    AAENABLE,
    AALABELS,
    AATICKS,
    AACORNERS,
    AAFRAME,
    AAADJUST,
    AACURSOR,
    RECENABLE,
    RECFILE,
    THROTTLE,
    RECFORMAT,
    PROJECTION,
    VIEWANGLE,
    BUTTON_STATE,
    BUTTON_UP_APPROX,
    BUTTON_DOWN_APPROX,
    BUTTON_UP_DENSITY,
    BUTTON_DOWN_DENSITY,
    RENDER_MODE,
    BACKGROUND,
    AAGRID,
    AACOLORS,
    AAANNOTATION,
    AALABELSCALE,
    AAFONT
};

ImageNode::ImageNode(NodeDefinition *nd, Network *net, int instnc) :
    DisplayNode(nd, net, instnc)
{
    ImageDefinition *imnd = (ImageDefinition*)nd;
    this->macroDirty = TRUE;
    this->translating = FALSE;

    //
    // Anything below here also belongs in this->setDefaultCfgState().
    //
    this->savedInteractionMode = NONE; 
    this->internalCache = imnd->getDefaultInternalCacheability();
}


ImageNode::~ImageNode()
{
}

boolean 
ImageNode::initialize()
{
    theApplication->setBusyCursor(TRUE);

    this->DisplayNode::initialize();

    if (! this->setMessageIdParameter(IMAGETAG))
	    return FALSE;

    this->enableVector(FALSE, FALSE);
    if (this->image != NULL)
	this->image->allowDirectInteraction(FALSE);

    this->height = -1;

    this->enableSoftwareRendering(TRUE, FALSE);
    this->setButtonUp(TRUE, FALSE);

    theApplication->setBusyCursor(FALSE);

    return TRUE;
}

void ImageNode::setDefaultCfgState()
{
    this->savedInteractionMode = NONE; 

    //
    // Before resetting internal caching, check the net version number.
    // If it's 3.1.1 or later, then proceed.  We're preventing this reset
    // for older nets because older versions of dx did not write internal cache
    // value into both .net and .cfg files.  They wrote it only into the .net file.
    // So if we reset here, then we'll be throwing out the saved value.  For current
    // versions of dxui, it's OK to reset because we'll find a cache value in the 
    // .cfg file.
    //
    Network* net = this->getNetwork();
    int net_major = net->getNetMajorVersion();
    int net_minor = net->getNetMinorVersion();
    int net_micro = net->getNetMicroVersion();
    int net_version = VERSION_NUMBER( net_major, net_minor, net_micro);
    int fixed_version = VERSION_NUMBER(3,1,1);
    if (net_version >= fixed_version) {
	ImageDefinition *imnd = (ImageDefinition*)this->getDefinition();
	this->internalCache = imnd->getDefaultInternalCacheability();
    }
}


boolean ImageNode::useVector()
{
    Parameter *p = this->getInputParameter(USEVECTOR);
    const char *s = p->getValueOrDefaultString();
    return *s != '0';
}

boolean ImageNode::useAutoAxes()
{
    Parameter *p = this->getInputParameter(AAENABLE);
    const char *s = p->getValueOrDefaultString();
    return *s != '0';
}

boolean ImageNode::useSoftwareRendering()
{
    Parameter *p = this->getInputParameter(RENDER_MODE);
    const char *s = p->getValueOrDefaultString();
    return EqualString(s, "0") || EqualString(s, "\"software\"");
}

    


Type ImageNode::setInputValue(int index,
			      const char *value,
			      Type t,
			      boolean send)
{
    Type result;

    int translating = this->translating;
    this->translating = 0;

    switch(index) {
    case USEVECTOR:
	if (translating)
	{
	    if (*value == '1')
		value = "0";
	    else if (*value == '2')
		value = "1";
	}
	if (EqualString(value, "NULL"))
	    value = "0";
	this->notifyUseVectorChange(*value != '0');
	break;

    case AAENABLE:
	if (translating)
	{
	    if (*value == '1')
		value = "0";
	    else if (*value == '2')
		value = "1";
	}
	break;

    case RECENABLE:
	if (translating)
	{
	    if (*value == '1')
		value = "0";
	    else if (*value == '2')
		value = "1";
	}
	break;

    case PROJECTION:
	this->notifyProjectionChange(*value != '0');
	break;

    case RENDER_MODE:
	if (EqualString(value, "\"hardware\"") ||
	    EqualString(value, "hardware"))
	    value = "1";
	else if (EqualString(value, "\"software\"") ||
	    EqualString(value, "software"))
	    value = "0";
	else if (translating)
	{
	    if (*value == '1')
		value = "0";
	    else if (*value == '2')
		value = "1";
	}
	if (t == DXType::StringType)
	    t = DXType::FlagType;
	break;
    case BUTTON_UP_APPROX:
    case BUTTON_DOWN_APPROX:
	if (EqualString(value, "\"flat\""))
	    value = "\"none\"";
	break;
    }

    result = this->DisplayNode::setInputValue(index, value, t, send);

    this->translating = translating;

    return result;
}

Type ImageNode::setInputSetValue(int index,
			      const char *value,
			      Type t,
			      boolean send)
{
    Type result;

    int translating = this->translating;
    this->translating = 0;

    switch(index) {
    case USEVECTOR:
	if (translating)
	{
	    if (*value == '1')
		value = "0";
	    else if (*value == '2')
		value = "1";
	}
	if (EqualString(value, "NULL"))
	    value = "0";
	this->notifyUseVectorChange(*value != '0');
	break;

    case AAENABLE:
	if (translating)
	{
	    if (*value == '1')
		value = "0";
	    else if (*value == '2')
		value = "1";
	}
	break;

    case RECENABLE:
	if (translating)
	{
	    if (*value == '1')
		value = "0";
	    else if (*value == '2')
		value = "1";
	}
	break;

    case PROJECTION:
	this->notifyProjectionChange(*value != '0');
	break;

    case RENDER_MODE:
	if (EqualString(value, "\"hardware\"") ||
	    EqualString(value, "hardware"))
	    value = "1";
	else if (EqualString(value, "\"software\"") ||
	    EqualString(value, "software"))
	    value = "0";
	else if (translating)
	{
	    if (*value == '1')
		value = "0";
	    else if (*value == '2')
		value = "1";
	}
	if (t == DXType::StringType)
	    t = DXType::FlagType;

	if (this->image)
	    this->image->setSoftware(*value == '0');

	break;
    case BUTTON_UP_APPROX:
    case BUTTON_DOWN_APPROX:
	if (EqualString(value, "\"flat\""))
	    value = "\"none\"";
	break;
    }

    result = this->DisplayNode::setInputSetValue(index, value, t, send);

    this->translating = translating;

    return result;
}

boolean ImageNode::sendValues(boolean ignoreDirty)
{
	ListIterator li(*this->getNetwork()->getImageList());
	ImageWindow *w;
	boolean sendMacro = TRUE;	// This shouldn't be necessary, 
	// the loop should catch all cases.
	while( (w = (ImageWindow*)li.getNext()) )
		if (w == this->image)
		{
			sendMacro = TRUE;
			break;
		}
		else if (w->getAssociatedNode() != NULL &&
			w->getAssociatedNode()->isA(ClassImageNode))
		{
			sendMacro = FALSE;
			break;
		}

	if (sendMacro && (ignoreDirty || this->macroDirty))
	{
		DXPacketIF *pif = theDXApplication->getPacketIF();
		if (pif == NULL || !this->sendMacro(pif))
			return FALSE;
		this->macroDirty = FALSE;
	}
	return this->DisplayNode::sendValues(ignoreDirty);
}

//
// FIXME:  This code is supposed to ensure that 1 and only 1 copy
// of the image macro is written to the .net file.  This code doesn't
// work.  If you never open an image window then each image tool will
// write out an image macro.  This should really be done in Network.
//
boolean ImageNode::printValues(FILE *f, const char *prefix, PrintType dest)
{
    ListIterator li(*this->getNetwork()->getImageList());
    ImageWindow *w;
    boolean sendMacro=TRUE;
    while( (w = (ImageWindow*)li.getNext()) )
	if (w == this->image)
	{
	    sendMacro = TRUE;
	    break;
	}
	else if ((w->getAssociatedNode())&&(w->getAssociatedNode()->isA(ClassImageNode)))
	{
	    sendMacro = FALSE;
	    break;
	}
    if (sendMacro)
	if (!this->printMacro(f))
	    return FALSE;
   return DisplayNode::printValues(f, prefix, dest);
}


#define FLOAT_LENGTH  14
#define VECTOR_LENGTH (FLOAT_LENGTH*3+10)
#define INT_LENGTH    11

void ImageNode::notifyUseVectorChange(boolean newUse)
{
    if (newUse == this->useVector())
	return;

    DXPacketIF *pif = theDXApplication->getPacketIF(); 
    if (pif != NUL(DXPacketIF*)) 
    {
	if (newUse)
	{
	    this->useAssignedInputValue(TO, FALSE);
	    this->useAssignedInputValue(FROM, FALSE);
	    this->useAssignedInputValue(WIDTH, FALSE);
	    this->useAssignedInputValue(UP, FALSE);
	    this->useAssignedInputValue(VIEWANGLE, FALSE);
	    this->useAssignedInputValue(PROJECTION, FALSE);
	    this->setInputDirty(TO);
	    this->setInputDirty(FROM);
	    this->setInputDirty(WIDTH);
	    this->setInputDirty(UP);
	    this->setInputDirty(VIEWANGLE);
	    this->setInputDirty(PROJECTION);
	}
	else
	{
	    this->useDefaultInputValue(TO, FALSE);
	    this->useDefaultInputValue(FROM, FALSE);
	    this->useDefaultInputValue(WIDTH, FALSE);
	    this->useDefaultInputValue(UP, FALSE);
	    this->useDefaultInputValue(VIEWANGLE, FALSE);
	}
    }
}
void ImageNode::enableVector(boolean use, boolean send)
{
    if (use == this->useVector())
	return;

    this->setInputValue(USEVECTOR, use? "1": "0", DXType::IntegerType, send);
    Parameter *p = this->getInputParameter(USEVECTOR);
    p->setDirty();
}
void ImageNode::setTo(double *to, boolean send)
{
    char s[100];
    sprintf(s, "[%g %g %g]", to[0], to[1], to[2]);
    this->setInputValue(TO, s, DXType::VectorType, send);
}
void ImageNode::setFrom(double *from, boolean send)
{
    char s[100];
    sprintf(s, "[%g %g %g]", from[0], from[1], from[2]);
    this->setInputValue(FROM, s, DXType::VectorType, send);
}
void ImageNode::setResolution(int x, int y, boolean send)
{
    char s[100];
    sprintf(s, "%d", x);
    this->setInputValue(RESOLUTION, s, DXType::IntegerType, send);
    this->height = y;
}
void ImageNode::setWidth(double w, boolean send)
{
    char s[100];
    sprintf(s, "%g", w);

    boolean persp;
    this->getProjection(persp);

    if (persp)
	this->setInputSetValue(WIDTH, s, DXType::ScalarType, send);
    else
	this->setInputValue(WIDTH, s, DXType::ScalarType, send);
}
void ImageNode::setAspect(double a, boolean send)
{
    char s[100];
    sprintf(s, "%g", a);
    this->setInputValue(ASPECT, s, DXType::ScalarType, send);
}
void ImageNode::setThrottle(double a, boolean send)
{
    char s[100];
    sprintf(s, "%g", a);
    this->setInputValue(THROTTLE, s, DXType::ScalarType, send);
}

void ImageNode::setUp(double *up, boolean send)
{
    char s[100];
    sprintf(s, "[%g %g %g]", up[0], up[1], up[2]);
    this->setInputValue(UP, s, DXType::VectorType, send);
}
void ImageNode::setBox(double box[4][3], boolean send)
{
    int i;
    int j;
    for (i = 0; i < 3; ++i)
	for (j = 0; j < 4; ++j)
	    this->boundingBox[j][i] = box[j][i];
}
void ImageNode::notifyProjectionChange(boolean newPersp)
{
    int parameter;
    if (newPersp)
	parameter = WIDTH;
    else
	parameter = VIEWANGLE;
    this->useDefaultInputValue(parameter, FALSE);
    if (newPersp)
	parameter = VIEWANGLE;
    else
	parameter = WIDTH;
    this->useAssignedInputValue(parameter, FALSE);
}
void ImageNode::setProjection(boolean persp, boolean send)
{
    char s[100];
    sprintf(s, "%d", persp);
    this->setInputValue(PROJECTION, s, DXType::IntegerType, send);
}

void ImageNode::setViewAngle(double angle, boolean send)
{
    char s[100];
    sprintf(s, "%g", angle);

    boolean persp;
    this->getProjection(persp);

    if (persp)
	this->setInputValue(VIEWANGLE, s, DXType::ScalarType, send);
    else
	this->setInputSetValue(VIEWANGLE, s, DXType::ScalarType, send);
}
void ImageNode::setButtonUp(boolean up, boolean send)
{
    char s[100];
    sprintf(s, "%d", up? 1: 2);
    this->setInputValue(BUTTON_STATE, s, DXType::IntegerType, send);
}
void ImageNode::setApprox(boolean up, const char *approx, boolean send)
{
    int param = up? BUTTON_UP_APPROX: BUTTON_DOWN_APPROX;
    this->setInputValue(param, approx, DXType::StringType, send);
}
void ImageNode::setDensity(boolean up, int density, boolean send)
{
    char s[100];
    sprintf(s, "%d", density);
    int param = up? BUTTON_UP_DENSITY: BUTTON_DOWN_DENSITY;

    this->setInputValue(param, s, DXType::IntegerType, send);
}

void ImageNode::setBackgroundColor(const char *color, boolean send)
{
    if (color == NULL)
	this->useDefaultInputValue(BACKGROUND, send);
    else
	this->setInputValue(BACKGROUND, color, DXType::UndefinedType, send);
}


void ImageNode::enableSoftwareRendering(boolean enable, boolean send)
{
    const char *s = enable? "0": "1";

    if (enable)
    {
	if (!this->isInputDefaulting(BUTTON_UP_DENSITY))
	    this->useDefaultInputValue(BUTTON_UP_DENSITY, FALSE);
	if (!this->isInputDefaulting(BUTTON_DOWN_DENSITY))
	    this->useDefaultInputValue(BUTTON_DOWN_DENSITY, FALSE);
    }
    else
    {
	if (this->isInputSet(BUTTON_UP_DENSITY) &&
		this->isInputDefaulting(BUTTON_UP_DENSITY))
	    this->useAssignedInputValue(BUTTON_UP_DENSITY, FALSE);
	if (this->isInputSet(BUTTON_DOWN_DENSITY) &&
		this->isInputDefaulting(BUTTON_DOWN_DENSITY))
	    this->useAssignedInputValue(BUTTON_DOWN_DENSITY, FALSE);
    }
    this->setInputValue(RENDER_MODE, s, DXType::StringType, send);
}

void ImageNode::getTo(double *to)
{
    const char *s;
    if (this->isInputDefaulting(TO))
	s = this->getInputDefaultValueString(TO);
    else
	s = this->getInputSetValueString(TO);

    if(!s || EqualString(s,"NULL"))
	to[0] = to[1] = to[2] = 0.0;
    else
	sscanf(s, "[%lg %lg %lg]", &to[0], &to[1], &to[2]);
}
void ImageNode::getFrom(double *from)
{
    const char *s;
    if (this->isInputDefaulting(FROM))
	s = this->getInputDefaultValueString(FROM);
    else
	s = this->getInputSetValueString(FROM);

    if(!s || EqualString(s,"NULL"))
    {
	from[0] = 0.0;
	from[1] = 0.0;
	from[2] = 1.0;
    }
    else
	sscanf(s, "[%lg %lg %lg]", &from[0], &from[1], &from[2]);
}
void ImageNode::getThrottle(double &a)
{
    const char *s;
    
    a = 0.0;
    if ((s = this->getInputValueString(THROTTLE)) && !EqualString(s,"NULL")) {
        sscanf(s, "%lg", &a);
    } else if ((s = this->getInputDefaultValueString(THROTTLE)) && 
		(*s != '(')) /* Not a descriptive value */ {
        sscanf(s, "%lg", &a);
    } 
}

void ImageNode::getUp(double *up)
{
    const char *s;
    if (this->isInputDefaulting(UP))
	s = this->getInputDefaultValueString(UP);
    else
	s = this->getInputSetValueString(UP);

    if(!s || EqualString(s,"NULL"))
    {
	up[0] = 0.0;
	up[1] = 1.0;
	up[2] = 0.0;
    }
    else
	sscanf(s, "[%lg %lg %lg]", &up[0], &up[1], &up[2]);
}
void ImageNode::getBox(double box[4][3])
{
    int i;
    int j;
    for (i = 0; i < 3; ++i)
	for (j = 0; j < 4; ++j)
	    box[j][i] = this->boundingBox[j][i];
}
void ImageNode::getProjection(boolean &persp)
{
    const char *s;
    int ii;

    if (this->isInputDefaulting(PROJECTION))
	s = this->getInputDefaultValueString(PROJECTION);
    else
	s = this->getInputValueString(PROJECTION);

    if(!s || EqualString(s,"NULL"))
	persp = 0;
    else
	sscanf(s, "%d", &ii);

    persp = ii != 0;
}
void ImageNode::getViewAngle(double &angle)
{
    const char *s;
    if (this->isInputDefaulting(VIEWANGLE))
	s = this->getInputDefaultValueString(VIEWANGLE);
    else
	s = this->getInputValueString(VIEWANGLE);

    if(!s || EqualString(s,"NULL"))
	angle = 30.0;
    else
	sscanf(s, "%lg", &angle);
}
boolean ImageNode::isButtonUp()
{
    int value;
    const char *s = this->getInputValueString(BUTTON_STATE);
    sscanf(s, "%d", &value);
    return value == 1;
}
void ImageNode::getApprox(boolean up, const char *&approx)
{
    int param = up? BUTTON_UP_APPROX: BUTTON_DOWN_APPROX;
    if (this->isInputDefaulting(param))
	approx = this->getInputDefaultValueString(param);
    else
	approx = this->getInputValueString(param);
}
void ImageNode::getDensity(boolean up, int &density)
{
    int param = up? BUTTON_UP_DENSITY: BUTTON_DOWN_DENSITY;

    const char *s;
    if (this->isInputDefaulting(param))
	s = this->getInputDefaultValueString(param);
    else
	s = this->getInputValueString(param);
    sscanf(s, "%d", &density);
}


void ImageNode::handleImageMessage(int id, const char *line)
{
	double to[3];
	double from[3];
	double up[3];
	double w;
	double a;
	int x, y;
	double box[4][3];
	double aamat[4][4];
	char s[100];
	ImageWindow *iw = this->image;
	int button, ddcamera, n;

	ASSERT(iw);

	aamat[0][3] = 0.0;
	aamat[1][3] = 0.0;
	aamat[2][3] = 0.0;
	aamat[3][3] = 1.0;


	// FIXME: Greg added aamat to the message on 5/14 and to preserve
	// runability I'm doing 2 sscanfs.  One of these should go away
	// after the next successful nightly build.
	n = sscanf(line,
		"%*d:  IMAGE:  ##%*d %dx%d %[^=]=%lg "
		"aspect=%lf from=(%lf,%lf,%lf) to=(%lf,%lf,%lf) up=(%lf,%lf,%lf) "
		"box=(%lf,%lf,%lf)(%lf,%lf,%lf)(%lf,%lf,%lf)(%lf,%lf,%lf) "
		"aamat=(%lf,%lf,%lf)(%lf,%lf,%lf)(%lf,%lf,%lf)(%lf,%lf,%lf) "
		"ddcamera=%d button=%d",
		&x, &y,
		s,
		&w, &a,
		&from[0], &from[1], &from[2],
		&to[0], &to[1], &to[2],
		&up[0], &up[1], &up[2],
		&box[3][0], &box[3][1], &box[3][2],
		&box[2][0], &box[2][1], &box[2][2],
		&box[1][0], &box[1][1], &box[1][2],
		&box[0][0], &box[0][1], &box[0][2],
		&aamat[0][0], &aamat[0][1], &aamat[0][2],
		&aamat[1][0], &aamat[1][1], &aamat[1][2],
		&aamat[2][0], &aamat[2][1], &aamat[2][2],
		&aamat[3][0], &aamat[3][1], &aamat[3][2],
		&ddcamera, &button);
	if (n != 40)
	{
		n = sscanf(line,
			"%*d:  IMAGE:  ##%*d %dx%d %[^=]=%lg "
			"aspect=%lf from=(%lf,%lf,%lf) to=(%lf,%lf,%lf) up=(%lf,%lf,%lf) "
			"box=(%lf,%lf,%lf)(%lf,%lf,%lf)(%lf,%lf,%lf)(%lf,%lf,%lf) "
			"ddcamera=%d button=%d",
			&x, &y,
			s,
			&w, &a,
			&from[0], &from[1], &from[2],
			&to[0], &to[1], &to[2],
			&up[0], &up[1], &up[2],
			&box[3][0], &box[3][1], &box[3][2],
			&box[2][0], &box[2][1], &box[2][2],
			&box[1][0], &box[1][1], &box[1][2],
			&box[0][0], &box[0][1], &box[0][2],
			&ddcamera, &button);

		aamat[0][0] = 1.0;
		aamat[0][1] = 0.0;
		aamat[0][2] = 0.0;
		aamat[0][3] = 0.0;
		aamat[1][0] = 0.0;
		aamat[1][1] = 1.0;
		aamat[1][2] = 0.0;
		aamat[1][3] = 0.0;
		aamat[2][0] = 0.0;
		aamat[2][1] = 0.0;
		aamat[2][2] = 1.0;
		aamat[2][3] = 0.0;
		aamat[3][0] = 0.0;
		aamat[3][1] = 0.0;
		aamat[3][2] = 0.0;
		aamat[3][3] = 1.0;
	}


	if ((n == 28) || (n == 40))
	{
		int persp;
		double viewAngle;

		if (EqualString(s, "width"))
			persp = FALSE;
		else
			persp = TRUE;

		double xdiff = from[0] - to[0];
		double ydiff = from[1] - to[1];
		double zdiff = from[2] - to[2];
		double dist = sqrt(xdiff * xdiff + ydiff * ydiff + zdiff * zdiff);

		if (persp)
		{
			//
			// fov = width/dist;
			//
			viewAngle = atan(w / 2) * 360 / 3.1415926;
			w = dist * w;
		}
		else
		{
			viewAngle = atan((w / 2) / dist) * 360 / 3.1415926;
		}

		enum DirectInteractionMode imode = this->image->getInteractionMode();

		if (ddcamera == 1 || button == 1)
		{
			/*
			* If in navigate mode, then to, from and up are
			* set elsewhere.
			*/
			if (imode != NAVIGATE || ddcamera == 1)
			{
				this->setTo(to, FALSE);
				this->setFrom(from, FALSE);
				this->setUp(up, FALSE);
			}

			this->setResolution(x, y, FALSE);
			this->setWidth(w, FALSE);
			this->setAspect(a, FALSE);
			this->setBox(box, FALSE);
			this->setProjection(persp, FALSE);
			this->setViewAngle(viewAngle, FALSE);
			this->enableVector(TRUE, FALSE);
			this->sendValuesQuietly();
			iw->newCamera(box, aamat, from, to, up, x, y, w, persp, viewAngle);
		}
		else if (!iw->cameraIsInitialized() || imode == NAVIGATE)
			iw->newCamera(box, aamat, from, to, up, x, y, w, persp, viewAngle);

		iw->allowDirectInteraction(TRUE);

		if (this->savedInteractionMode != NONE)  {
			iw->setInteractionMode(this->savedInteractionMode);
			this->savedInteractionMode = NONE;
		}
	}
	else if (sscanf(line,
		"%*d:  IMAGE:  ##%*d %dx%d",
		&x, &y) == 2)
	{
		this->setResolution(x, y, FALSE);

		iw->allowDirectInteraction(FALSE);

		iw->clearFrameBufferOverlay();
	}
}

boolean ImageNode::associateImage(ImageWindow *w)
{
    boolean result = this->DisplayNode::associateImage(w);

    if (result && w)
	w->allowDirectInteraction(FALSE);

    return result;
}

//
// The next two functions do the same thing ... They prevent "RECENABLE"
// from being printed.
char *ImageNode::inputValueString(int i, const char *prefix)
{
    if (i != RECENABLE)
	return this->DisplayNode::inputValueString(i, prefix);
    else
	return NULL;
}

boolean ImageNode::printIOComment(FILE *f, boolean input, int index, 
					const char *indent, boolean valueOnly)
{
    if (index == RECENABLE && input)
	return TRUE;
    else
	return this->DisplayNode::printIOComment(f, input, index, 
					indent,valueOnly);
}

void ImageNode::openDefaultWindow(Widget w)
{
    this->openImageWindow(TRUE);
}

//
// Determine if this node is of the given class.
//
boolean ImageNode::isA(Symbol classname)
{
    Symbol s = theSymbolManager->registerSymbol(ClassImageNode);
    if (s == classname)
	return TRUE;
    else
	return this->DisplayNode::isA(classname);
}


char *ImageNode::netEndOfMacroNodeString(const char *prefix)
{
    char buf[1025];
#if WORKSPACE_PAGES
    const char *gpn  = this->getGroupName(theSymbolManager->getSymbol(PROCESS_GROUP));
#else
    const char *gpn  = this->getGroupName();
#endif
    if (gpn)
	sprintf(buf, "CacheScene(%sImage_%d_in_%d, "
		   "%sImage_%d_out_1, %sImage_%d_out_2)[group: \"%s\"];\n",
		   prefix, this->getInstanceNumber(), IMAGETAG,
		   prefix, this->getInstanceNumber(),
		   prefix, this->getInstanceNumber(),
		   gpn);
    else
	sprintf(buf, "CacheScene(%sImage_%d_in_%d, "
		   "%sImage_%d_out_1, %sImage_%d_out_2);\n",
		   prefix, this->getInstanceNumber(), IMAGETAG,
		   prefix, this->getInstanceNumber(),
		   prefix, this->getInstanceNumber());

    char *s = new char[1+STRLEN(buf)];
    strcpy (s, buf);
    return s;
}

const char *ImageNode::getPickIdentifier()
{
    return this->getModuleMessageIdString();
}

boolean ImageNode::netParseComment(const char* comment,
				   const char *file, int lineno)
{
	int major;
	int minor;
	int micro;
	char buf[1024];

	if (this->getNetwork()->getDXMajorVersion() < 3)
	{
		const char *n = strstr(comment, "input[");
		if (n)
		{
			int in, len = (n - comment) + 6;
			memcpy(buf, comment, len);
			sscanf(comment+len, "%d", &in);
			sprintf(buf+len, "%d%s",
				translateInputs[in], strchr(comment+len, ']'));
			comment = (const char *)buf;
		}
	}

	this->getNetwork()->getVersion(major, minor, micro);

	//
	// version 2 net files after 2.0.0 require translation of 
	// flags from 1/2 to 0/1 
	//
	if (major == 2 && minor >= 0 && micro >= 1)
		this->translating = TRUE;
	boolean res =
		this->DisplayNode::netParseComment(comment, file, lineno);
	this->translating = FALSE;

	return res;
}

boolean ImageNode::netPrintAuxComment(FILE *f)
{
    if (!this->DisplayNode::netPrintAuxComment(f))
        return FALSE;

    if (!this->printCommonComments(f,"    "))
        return FALSE;


    return TRUE;
}

boolean ImageNode::netParseAuxComment(const char* comment,
                                   const char* file, int lineno)
{
    return this->DisplayNode::netParseAuxComment(comment,file,lineno) ||
	    this->parseCommonComments(comment,file,lineno);
}


boolean ImageNode::cfgPrintNode(FILE *f, PrintType dest)
{
    if (!this->DisplayNode::cfgPrintNode(f,dest))
	return FALSE;

    //
    // Print the inputs that have values
    //
    int i, num = this->getInputCount();
    for (i=1 ; i<=num ; i++) {
        if (!this->printIOComment(f, TRUE, i,NULL,TRUE))
            return FALSE;
    }

    if (!this->printCommonComments(f))
	return FALSE;

    return TRUE;
}

boolean ImageNode::cfgParseComment(const char *comment, const char *file,
                                int lineno)
{
    if (this->DisplayNode::cfgParseComment(comment,file,lineno))
	return TRUE;

    if (this->parseIOComment(TRUE, comment,file,lineno,TRUE))
	return TRUE;

    if (this->parseCommonComments(comment,file,lineno))
	return TRUE;

    return FALSE;

}

boolean ImageNode::printCommonComments(FILE *f, const char *indent)
{

    if (!indent)
	indent = "";

    //
    // Print internal cacheability
    //
    if(fprintf (f, "%s// internal caching: %d\n", indent,
			this->getInternalCacheability()) <= 0)
	return FALSE;

    if (!this->image)
	return TRUE;

    //
    // Only print the 'interaction mode' comment if the relevent input 
    // is not set or connected.
    // NOTE: this should be the last comment for Image nodes in both the .net
    // and .cfg (see parseCommonComments()).
    //
    if (this->isInputDefaulting(INTERACTIONMODE)) {

	enum DirectInteractionMode imode = this->image->getInteractionMode();
	const char *mode = "";
	switch (imode) {
	    case CAMERA: 	mode = "CAMERA"; break;
	    case CURSORS: 	mode = "CURSORS"; break;
	    case PICK: 		mode = "PICK"; break;
	    case NAVIGATE: 	mode = "NAVIGATE"; break;
	    case PANZOOM: 	mode = "PANZOOM"; break;
	    case ROAM: 		mode = "ROAM"; break;
	    case ROTATE: 	mode = "ROTATE"; break;
	    case ZOOM: 		mode = "ZOOM"; break;
	    default: 		mode = "NONE"; break;
	}
	if (fprintf(f,"%s// interaction mode = %s\n",indent,mode) <= 0)
	    return FALSE;
    }


    return TRUE;

}

boolean ImageNode::setInteractionMode(const char *mode)
{
	int i, n;
	char imode[32], arg[256];

	n = sscanf(mode, "%s %s", imode, arg);

	for (i = 0; i < 31 && mode[i]; i++)
	{
		char c = imode[i];

		if (isalpha(c) && isupper(c))
			imode[i] = tolower(c);
		else
			imode[i] = c;
	}

	imode[i] = '\0';

	enum DirectInteractionMode interactionMode;

	if (EqualString(imode,"camera"))
		interactionMode = CAMERA;
	else if (EqualString(imode,"cursors"))
		interactionMode = CURSORS;
	else if (EqualString(imode,"pick"))
		interactionMode = PICK;
	else if (EqualString(imode,"navigate"))
		interactionMode = NAVIGATE;
	else if (EqualString(imode,"panzoom"))
		interactionMode = PANZOOM;
	else if (EqualString(imode,"roam"))
		interactionMode = ROAM;
	else if (EqualString(imode,"rotate"))
		interactionMode = ROTATE;
	else if (EqualString(imode,"zoom"))
		interactionMode = ZOOM;
	else
		interactionMode = NONE;

	ImageWindow *img = this->image;
	if (img)
	{
		if (n > 1)
		{
			char *clss;

			if (interactionMode == CURSORS)
				clss = ClassProbeNode;
			else if (interactionMode == PICK)
				clss = ClassPickNode;
			else
				goto no_arg;

			List *pl = 
				theDXApplication->network->makeClassifiedNodeList(clss, TRUE);

			if (pl)
			{
				char *arg0;

				if (EqualSubstring(arg, "label=", 6))
					arg0 = arg + 6;
				else 
					arg0 = arg;

				ListIterator li(*pl);
				Node *nd;
				int inum = -1;

				while (NULL != (nd = (Node *)li.getNext()))
					if (EqualString(arg0, nd->getLabelString()))
					{
						inum = nd->getInstanceNumber();
						break;
					}

					if (inum > 0)
					{
						if (interactionMode == CURSORS)
							img->setCurrentProbe(inum);
						else
							img->setCurrentPick(inum);
					}

					delete pl;
			}
		}
		img->allowDirectInteraction(TRUE);
		img->setInteractionMode(interactionMode);
	} else
		this->savedInteractionMode = interactionMode;

no_arg:
	return TRUE;
}

void ImageNode::setInteractionModeParameter (DirectInteractionMode mode)
{
char *cp = 0;

    switch (mode) {
	case CAMERA: cp = "camera"; break;
	case CURSORS: cp = "cursors"; break;
	case PICK: cp = "pick"; break;
	case NAVIGATE: cp = "navigate"; break;
	case PANZOOM: cp = "panzoom"; break;
	case ROAM: cp = "roam"; break;
	case ROTATE: cp = "rotate"; break;
	case ZOOM: cp = "zoom"; break;
	case NONE: cp = "none"; break;

	//
	// USER Error? (probably not possible in this case)
	//
	default:
	    cp = NUL(char*);
    }
    if (cp) this->setInputValue(INTERACTIONMODE, cp, DXType::StringType, FALSE);
}

boolean ImageNode::parseCommonComments(const char *comment, const char *file,
                                int lineno)
{
	char mode[128];
	char *cp = " internal caching:";

	if (sscanf(comment," interaction mode = %[^\n]",mode) == 1) 
	{
		this->setInteractionMode((const char *)mode);

		//
		// Now since this is the last comment, ask the ImageWindow 
		// to refresh its state from our new cfg state. 
		//
		ImageWindow *img = this->image;
		if (img) 
			img->updateFromNewCfgState();

		return TRUE;
	} else if (EqualSubstring(comment,cp,strlen(cp))) {
		int cacheval, items;
		items = sscanf (comment, " internal caching: %d", &cacheval);
		if (items == 1) {
			switch (cacheval) {
		case 0:
			this->setInternalCacheability (InternalsNotCached);
			break;
		case 1:
			this->setInternalCacheability (InternalsFullyCached);
			break;
		case 2:
			this->setInternalCacheability (InternalsCacheOnce);
			break;
		default:
			return FALSE; 
			break;
			}
			return TRUE;
		}
	}

	return FALSE;
}



#define PARSEPARAM(str, type, index, flag)			\
{								\
    while(*p == ' ') p++;					\
    if (*p && !strncmp(p, str, strlen(str))) 			\
    {								\
	p += strlen(str);					\
	q = argbuf;						\
	for (r = *p++; ((r != ';')&&(r != '\0')); r = *p++)	\
	    *q++ = r;						\
	*q = '\0';						\
	this->setInputSetValue(index, argbuf, DXType::type, 0);	\
	flag = True;						\
    }								\
}

#define PARSECOMMAND(str, ccode)                     		\
{                                                             	\
    while(*p == ' ') p++;                                     	\
    if (*p && !strncmp(p, str, strlen(str)))                  	\
    {                                                         	\
	p += strlen(str);                                       \
	q = argbuf;                                             \
	for (r = *p++; ((r != ';')&&(r != '\0')); r = *p++)     \
	    *q++ = r;                                           \
	*q = '\0';                                              \
	ccode;                                      \
    }                                                         	\
}

int ImageNode::handleNodeMsgInfo(const char *line)
{
    boolean save = FALSE, throttle = FALSE, bckgnd = FALSE,
	    ititle = FALSE, autoaxes = FALSE, rdropt = FALSE;
    char *p = (char *)line, *q, r, *argbuf;

    if (! line)
	return 0;

    argbuf = new char [1 + strlen(line) ];

    // Skip over first two fields: processor id and image id
    while(*p != ':' && *p != '\0') p++;
    if (*p == ':') p++;
    while(*p != ':' && *p != '\0') p++;
    if (*p == ':') p++;
    
    PARSEPARAM("backgroundColor=",  UndefinedType,   BACKGROUND,           bckgnd);
    PARSEPARAM("throttle=",         ScalarType,      THROTTLE,	          throttle);
    PARSEPARAM("recenable=",        FlagType,        RECENABLE,	          save);
    PARSEPARAM("recfile=",          StringType,      RECFILE,	          save);
    PARSEPARAM("recformat=",        StringType,      RECFORMAT,	          save);
    PARSEPARAM("recresolution=",    IntegerType,     RECRESOLUTION,       save);
    PARSEPARAM("recaspect=",        ScalarType,      RECASPECT,	          save);
    PARSEPARAM("aaenabled=",        FlagType,        AAENABLE,	          autoaxes);
    PARSEPARAM("aalabel=",          StringType,      AALABELS,	          autoaxes);
    PARSEPARAM("aalabellist=",      StringListType,  AALABELS,	          autoaxes);
    PARSEPARAM("aaticklist=",       IntegerListType, AATICKS,	          autoaxes);
    PARSEPARAM("aatick=",           IntegerType,     AATICKS,	          autoaxes);
    PARSEPARAM("aacorners=",        VectorListType,  AACORNERS,	          autoaxes);
    PARSEPARAM("aaframe=",          FlagType,        AAFRAME,	          autoaxes);
    PARSEPARAM("aaadjust=",         FlagType,        AAADJUST,	          autoaxes);
    PARSEPARAM("aacursor=",         VectorType,      AACURSOR,	          autoaxes);
    PARSEPARAM("aagrid=",           FlagType,        AAGRID,	          autoaxes);
    PARSEPARAM("aacolor=",          StringType,      AACOLORS,	          autoaxes);
    PARSEPARAM("aacolorlist=",      StringListType,  AACOLORS,	          autoaxes);
    PARSEPARAM("aaannotation=",     StringType,     AAANNOTATION,         autoaxes);
    PARSEPARAM("aaannotationlist=", StringListType, AAANNOTATION,         autoaxes);
    PARSEPARAM("aalabelscale=",     ScalarType,      AALABELSCALE,        autoaxes);
    PARSEPARAM("aafont=",           StringType,      AAFONT,	          autoaxes);
    PARSEPARAM("aaxticklocs=",      ScalarListType,  AA_XTICK_LOCS,       autoaxes);
    PARSEPARAM("aayticklocs=",      ScalarListType,  AA_YTICK_LOCS,       autoaxes);
    PARSEPARAM("aazticklocs=",      ScalarListType,  AA_ZTICK_LOCS,       autoaxes);
    PARSEPARAM("aaxticklabels=",    StringListType,  AA_XTICK_LABELS,     autoaxes);
    PARSEPARAM("aayticklabels=",    StringListType,  AA_YTICK_LABELS,     autoaxes);
    PARSEPARAM("aazticklabels=",    StringListType,  AA_ZTICK_LABELS,     autoaxes);
    PARSEPARAM("recmode=",          IntegerType,     RENDER_MODE,         rdropt);
    PARSEPARAM("buappx=",           StringType,      BUTTON_UP_APPROX,    rdropt);
    PARSEPARAM("bdappx=",           StringType,      BUTTON_DOWN_APPROX,  rdropt);
    PARSEPARAM("buden=",            IntegerType,     BUTTON_UP_DENSITY,   rdropt);
    PARSEPARAM("bdden=",            IntegerType,     BUTTON_DOWN_DENSITY, rdropt);


#ifdef alphax
    //
    // WARNING, This is a HACK!
    // When there is no bounding box, but the interactionMode tab is set to
    // "rotate", the alpha is dieing in Picture.c's inverse() function
    // because the determinate of the matrix is 0 (see bug PAULA82).
    // The 6000 and sgi do not dump core for this.
    // To generate the dump, connect Collect to Image and set Image's
    // interaction mode input to "rotate", open the image window and then
    // execute.
    // This fixes this problem, because if there is a camera-less IMAGE message
    // then direct interaction has been turned off.
    //
    // I would like to see this fixed with a 2nd defaulting arg to
    // setInteractionMode() that indicates whether or not to enable 
    // interaction mode.  It would default to TRUE.
    //
    if (this->image && this->image->directInteractionAllowed())
#endif
    PARSECOMMAND("interactionmode=", this->setInteractionMode(argbuf));

    //
    // FIXME: I don't believe the PARSECOMMAND macro is a good thing.  You're
    // supposed to deal with parameter changes in :ioParameterStatusChanged().
    //
#if 0
    PARSECOMMAND("title=", this->setTitle(argbuf,TRUE));
#else
    PARSEPARAM("title=",           StringType,      IMAGENAME,	   ititle);
#endif

    this->deferVisualNotification();

    if (this->image)
    {
	if (autoaxes)
	    this->image->updateAutoAxesDialog();

	if (rdropt)
	    this->image->updateRenderingOptionsDialog();
    }

    this->undeferVisualNotification();

    delete argbuf;

    return 0;
}

#undef PARSEPARAM

void ImageNode::reflectStateChange(boolean) { }

boolean ImageNode::isDataDriven()
{
    int i, icnt = this->getInputCount();
    boolean driven = FALSE;

    for (i = 2; !driven && i <= icnt; i++)
	driven = !this->isInputDefaulting(i);
    
    return driven;
}


void ImageNode::ioParameterStatusChanged(boolean input, int index,
				 NodeParameterStatusChange status)
{
    switch (index)
    {
	case THROTTLE:
	    if (this->image)
	    {
		if (status == ParameterArkAdded ||
				status == ParameterArkRemoved)
	        {
		    boolean status = this->isInputDefaulting(THROTTLE);
		    this->image->sensitizeThrottleDialog(status);
		}
		else if (status == ParameterValueChanged ||
			 status == ParameterSetValueChanged)
		{
		    double t;

		    this->getThrottle(t);
		    this->image->updateThrottleDialog(t);
		}
	    }
	    break;
	    
	case RECENABLE:
	case RECFILE:
	case RECFORMAT:
	case RECRESOLUTION:
	case RECASPECT:
	    if (this->image)
	    {
		// This used to say isRecFileInputSet() instead of TRUE
		// but it always worked because isRecFileInputSet always returns
		// the wrong value.  Nowdays, we only want to revisit the sensitivity
		// of the dialogs, not the sensitivity of the command anyway.
		this->image->sensitizePrintImageDialog(TRUE);
		this->image->sensitizeSaveImageDialog(TRUE);
	    }
	    break;

	case BACKGROUND:

	    if (this->image)
	    {
		if (status & ParameterArkAdded || status & ParameterArkRemoved)
		{
		    this->image->sensitizeBackgroundColorDialog(
				!this->isBGColorConnected());
		}
		else if (status & ParameterValueChanged ||
				 status & ParameterSetValueChanged)
		{
		    const char *color;
		    this->getBackgroundColor(color);
		    this->image->updateBGColorDialog(color);
		}
	    }

	    break;

	case AAENABLE:
	    if (this->image)
		this->image->setAutoAxesDialogEnable();
	    break;

	case AALABELS:
	    if (this->image)
		this->image->setAutoAxesDialogLabels();
	    break;

	case AATICKS:
	    if (this->image)
		this->image->setAutoAxesDialogTicks();
	    break;

	case AA_XTICK_LOCS:
	    if (this->image)
		this->image->setAutoAxesDialogXTickLocs();
	    break;

	case AA_YTICK_LOCS:
	    if (this->image)
		this->image->setAutoAxesDialogYTickLocs();
	    break;

	case AA_ZTICK_LOCS:
	    if (this->image)
		this->image->setAutoAxesDialogZTickLocs();
	    break;

	case AA_XTICK_LABELS:
	    if (this->image)
		this->image->setAutoAxesDialogXTickLabels();
	    break;

	case AA_YTICK_LABELS:
	    if (this->image)
		this->image->setAutoAxesDialogYTickLabels();
	    break;

	case AA_ZTICK_LABELS:
	    if (this->image)
		this->image->setAutoAxesDialogZTickLabels();
	    break;

	case AACORNERS:
	    if (this->image)
		this->image->setAutoAxesDialogCorners();
	    break;

	case AAFRAME:
	    if (this->image)
		this->image->setAutoAxesDialogFrame();
	    break;

	case AAADJUST:
	    if (this->image)
		this->image->setAutoAxesDialogAdjust();
	    break;

	case AACURSOR:
	    if (this->image)
		this->image->setAutoAxesDialogCursor();
	    break;

	case AAGRID:
	    if (this->image)
		this->image->setAutoAxesDialogGrid();
	    break;

	case AACOLORS:
	    if (this->image)
		this->image->setAutoAxesDialogAnnotationColors();
	    break;

	case AAANNOTATION:
	    if (this->image)
		this->image->setAutoAxesDialogAnnotationColors();
	    break;

	case AALABELSCALE:
	    if (this->image)
		this->image->setAutoAxesDialogLabelScale();
	    break;

	case AAFONT:
	    if (this->image)
		this->image->setAutoAxesDialogFont();
	    break;
	
	case INTERACTIONMODE:
	    if (this->image)
	    {
		boolean status = this->isInteractionModeConnected();
		this->image->sensitizeViewControl(!status);
	    }
	    break;

	case IMAGENAME:
	    if (this->image)
	    {
		boolean status = this->isInputConnected(IMAGENAME);
		if ((!status) && (this->isInputDefaulting(IMAGENAME)))
		    this->setTitle(NUL(char*));
		else
		    this->setTitle(this->getInputValueString (IMAGENAME));
		this->image->sensitizeChangeImageName(!status);
	    }
	    break;

        case RENDER_MODE:
            if (this->image)
            {
                if (status == ParameterArkAdded)
                {
                    this->image->sensitizeRenderMode(False);
                }
                else if (status == ParameterArkRemoved)
                {
                    this->image->sensitizeRenderMode(True);
                }
                else if (status &
                        (ParameterValueChanged | ParameterSetValueChanged))
                {
                    this->image->updateRenderingOptionsDialog();
                }
            }
            break;
  
        case BUTTON_UP_APPROX:
            if (this->image)
            {
                if (status == ParameterArkAdded)
                {
                    this->image->sensitizeButtonUpApprox(False);
                }
                else if (status == ParameterArkRemoved)
                {
                    this->image->sensitizeButtonUpApprox(True);
                }
                else if (status &
                        (ParameterValueChanged | ParameterSetValueChanged))
                {
                    this->image->updateRenderingOptionsDialog();
                }
            }
            break;
  
        case BUTTON_DOWN_APPROX:
            if (this->image)
            {
                if (status == ParameterArkAdded)
                {
                    this->image->sensitizeButtonDownApprox(False);
                }
                else if (status == ParameterArkRemoved)
                {
                    this->image->sensitizeButtonDownApprox(True);
                }
                else if (status &
                        (ParameterValueChanged | ParameterSetValueChanged))
                {
                    this->image->updateRenderingOptionsDialog();
                }
            }
            break;
  
        case BUTTON_UP_DENSITY:
            if (this->image)
            {
                if (status == ParameterArkAdded)
                {
                    this->image->sensitizeButtonUpDensity(False);
                }
                else if (status == ParameterArkRemoved)
                {
                    this->image->sensitizeButtonUpDensity(True);
                }
                else if (status &
                        (ParameterValueChanged | ParameterSetValueChanged))
                {
                    this->image->updateRenderingOptionsDialog();
                }
            }
            break;
  
        case BUTTON_DOWN_DENSITY:
            if (this->image)
            {
                if (status == ParameterArkAdded)
                {
                    this->image->sensitizeButtonDownDensity(False);
                }
                else if (status == ParameterArkRemoved)
                {
                    this->image->sensitizeButtonDownDensity(True);
                }
                else if (status &
                        (ParameterValueChanged | ParameterSetValueChanged))
                {
                    this->image->updateRenderingOptionsDialog();
                }
            }
            break;
	
	default:
	    break;
    }

    DisplayNode::ioParameterStatusChanged(input, index, status);
}


//
// FIXME!: this should really be being done through ImageWindow via
// ImageNode::reflectStateChange().  ImageNode has no business setting
// senstivities of another object.  That is ImageWindow's decision.
//
void  ImageNode::updateWindowSensitivities() { }
void  ImageNode::openImageWindow(boolean manage)
{
    this->DisplayNode::openImageWindow(manage);
}

int ImageNode::getMessageIdParamNumber() { return IMAGETAG; }


//
// If the param is driven by the server, then don't change the defaulting
// status. For both setTitle() and getTitle(), it's important to strip
// surrounding quotes becuase the value will be used in a .net file and one
// the window mgr border and quotes are not acceptable in those situations.
//
void ImageNode::setTitle(const char *title, boolean fromServer)
{
    //
    // Hands off the param if it's connected.
    //
	if (!this->isInputConnected(IMAGENAME)) {
		if ((!title) || (!title[0])) {
			this->useDefaultInputValue(IMAGENAME, FALSE);
		} else {
			if ((fromServer) && (this->isInputDefaulting(IMAGENAME))) {
				this->setInputAttributeFromServer (IMAGENAME,title,DXType::StringType);
			} else {
				const char *v = this->getInputValueString (IMAGENAME);
				if (!EqualString (v, title))
					this->setInputValue (IMAGENAME, title, DXType::StringType, FALSE);
			}
		}
	}
	if ((title) && (title[0]=='\"')) {
		char *tmpbuf = DuplicateString(&title[1]);
		if (tmpbuf[strlen(tmpbuf)-1] == '\"') tmpbuf[strlen(tmpbuf)-1] = '\0';
		this->DisplayNode::setTitle(tmpbuf, fromServer);
		delete tmpbuf;
	} else {
		this->DisplayNode::setTitle(title, fromServer);
	}
}
 

//
// FIXME: I am confused.  (At last a genuine FIXME comment.)
// If the param is defaulting, then rely on the superclass.  If the param isn't
// defaulting then fetch the value from the node.  If the node has a null value,
// then rely on the superclass.  Should there ever be a difference between
// what the superclass thinks and what this thinks?  I don't think so.
// Wierdness: this and the the superclass support {set,get}Title,
// but the superclass versions use an ioComment whereas this uses an input
// parameter.
//
const char *ImageNode::getTitle()
{
    //
    // Using the node's value if the input is defaulting would prevent us
    // from using "nodename: filename".
    //
    if (this->isInputDefaulting(IMAGENAME)) 
	return this->DisplayNode::getTitle();

    //
    // Make sure the value is a valid string and strip off the quotes
    // and use it, if it is.
    //
	if (this->getInputSetValueType(IMAGENAME) == DXType::StringType) {
		const char *v = this->getInputValueString (IMAGENAME);
		if (!v || !v[0] || EqualString(v,"NULL"))
			return this->DisplayNode::getTitle();

		char *t = FindDelimitedString(v,'"','"');
		if (!t) 
			return this->DisplayNode::getTitle();

		if (this->title)
			delete this->title;

		int tlen = STRLEN(t);
		t[tlen - 1] = '\0';			// Strip last quote
		this->title = DuplicateString(t + 1);	// Copy and strip first quote
		delete t;
		return this->title;
	} else 
		return this->DisplayNode::getTitle();
}

boolean ImageNode::hardwareMode()
{
    ImageWindow* iw = this->image;
    if (!iw) return FALSE;

    return iw->hardwareMode();
}

void ImageNode::enableJava(const char* filename, boolean send)
{
    Node* webOptions = this->getWebOptionsNode();

    //
    // Set enable flag and filename inputs to the WebOptions tool
    // Other inputs can default.
    //
    webOptions->setInputValue (WEB_FILE, filename, DXType::StringType, FALSE);
    webOptions->setInputValue (WEB_ENABLED, "1", DXType::FlagType, TRUE);
}

void ImageNode::disableJava(boolean send)
{

    Node* webOptions = this->getWebOptionsNode();
    //
    // Set enable flag and filename inputs to the WebOptions tool
    // Other inputs can default.
    //
    webOptions->setInputValue (WEB_ENABLED, "0", DXType::FlagType, TRUE);
}

Node* ImageNode::getWebOptionsNode()
{
    Node* webOptions = NUL(Node*);
    List* ia = (List*)this->getInputArks(JAVA_OPTIONS);
    ASSERT(ia);
    ASSERT(ia->getSize() == 1);
    Ark* a = (Ark*)ia->getElement(1);
    int dummy = 0;
    webOptions = a->getSourceNode(dummy);
    ASSERT(webOptions);
    return webOptions;
}

//
// Flip webOptions input tab-up
//
void ImageNode::unjavifyNode()
{
    Parameter* p = this->getInputParameter(JAVA_OPTIONS);
    if (p->isConnected()) 
	p->disconnectArks();

    this->useDefaultInputValue(JAVA_OPTIONS, TRUE);
    this->setInputVisibility (JAVA_OPTIONS, FALSE);
}

//
// Connect webOptions output 1 to this input JAVA_OPTIONS
// Connection seq_rcvr output 1 to webOptions input ???
//
// The caller must pass in a new webOptions  macro node and a new
// counters receiver OR the arcs must be in place already.
//
void ImageNode::javifyNode (Node* w, Node* s)
{
    Node* seq_rcvr = s;
    Node* webOptions = w;
    Network* net = this->getNetwork();
    EditorWindow* ewin = net->getEditor();
    if (webOptions == NUL(Node*)) {
	webOptions = this->getWebOptionsNode();
    }
    ASSERT(webOptions);

    if (this->isInputConnected(JAVA_OPTIONS) == FALSE) {
	this->useDefaultInputValue(JAVA_OPTIONS, FALSE);
	this->setInputVisibility (JAVA_OPTIONS, TRUE);
	StandIn* wosi = webOptions->getStandIn();
	StandIn* si = this->getStandIn();
	if ((si) && (wosi)) {
	    si->addArk(ewin, new Ark(webOptions, 1, this, JAVA_OPTIONS));
	} else {
	    new Ark(webOptions, 1, this, JAVA_OPTIONS);
	}
    }

    if (seq_rcvr == NUL(Node*)) {
	List* ia = (List*)webOptions->getInputArks(WEB_COUNTERS);
	ASSERT(ia);
	ASSERT(ia->getSize() == 1);
	Ark* a = (Ark*)ia->getElement(1);
	int dummy = 0;
	seq_rcvr = a->getSourceNode(dummy);
    }

    //
    // ASSERT (seq_rcvr);
    // At this point we should have the receiver node.  It would be
    // missing if the user manually disconnected it.  If we don't have
    // it, then the network won't be javified properly.  We'll put up
    // an error message but we won't prevent the user from doing this.
    //
    if (webOptions->isInputConnected(WEB_COUNTERS) == FALSE) {
	if (seq_rcvr == NUL(Node*)) {
	    WarningMessage (
		"WebOptions macro of Image tool (instance %d)\n"
		"was disconnected from its sequence counter.\n"
		"This visual program may not function properly\n"
		"when run under control of DXServer.",
		this->getInstanceNumber());
	} else {
	    webOptions->useDefaultInputValue(WEB_COUNTERS, FALSE);
	    StandIn* wsi = webOptions->getStandIn();
	    StandIn* rsi = seq_rcvr->getStandIn();
	    if ((wsi) && (rsi)) {
		rsi->addArk(ewin, new Ark(seq_rcvr, 1, webOptions, WEB_COUNTERS));
	    } else {
		new Ark(seq_rcvr, 1, webOptions, WEB_COUNTERS);
	    }
	}
    }
}

boolean ImageNode::isJavified()
{
    return this->isInputConnected(JAVA_OPTIONS);
}

boolean ImageNode::isJavified(Node* webOptions)
{
    return webOptions->isInputConnected(WEB_COUNTERS);
}

const char* ImageNode::getWebOptionsFormat()
{
    Node* webOptions = this->getWebOptionsNode();
    const char* fmt = NUL(char*);
    if (webOptions->isInputConnected(WEB_FORMAT) == FALSE) 
	fmt = webOptions->getInputValueString(WEB_FORMAT);
    if (fmt) {
    	if (EqualString(fmt, "\"NULL\"")) fmt = NUL(char*);
	else if (EqualString(fmt, "NULL")) fmt = NUL(char*);
    }
    return fmt;
}


boolean ImageNode::isWebOptionsOrbit()
{
    boolean retval = FALSE;
    Node* webOptions = this->getWebOptionsNode();
    const char* orbit = NUL(char*);
    if (webOptions->isInputConnected(WEB_ORBIT) == FALSE) 
	orbit = webOptions->getInputValueString(WEB_ORBIT);
    if ((orbit) && (EqualString(orbit, "1"))) 
	retval = TRUE;
    return retval;
    
}


boolean ImageNode::printAsJava(FILE* f)
{
    if (this->DisplayNode::printAsJava(f) == FALSE)
	return FALSE;

    Node* webOptions = this->getWebOptionsNode();
    const char* wns = webOptions->getNameString();
    int winstno = webOptions->getInstanceNumber();

    const char* indent = "        ";
    const char* ns = this->getNameString();
    int instno = this->getInstanceNumber();

    if (fprintf (f,
	"%s%s_%d.addInputArc (%d, %s_%d, %d);\n",
	indent, ns, instno, JAVA_OPTIONS, wns, winstno, 1) 
	<= 0) return FALSE;

    return TRUE;
}

boolean ImageNode::isWebOptionsImgIdConnected()
{
    Node* webOptions = this->getWebOptionsNode();
    return webOptions->isInputConnected(WEB_ID);
}

boolean ImageNode::printInputAsJava(int input)
{
    boolean retval = FALSE;
    switch (input) {
	case FROM:
	case VIEWANGLE:
	case WIDTH:
	case TO:
	case UP:
	case IMAGENAME:
	case INTERACTIONMODE:
	    retval = TRUE;
	    break;
    }
    return retval;
}
