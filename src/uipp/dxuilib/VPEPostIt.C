/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#include "VPEPostIt.h"
#include "DXApplication.h"
#include "moduledrag.bm"
#include "moduledragmask.bm"
#include "DXStrings.h"
#include "postit.bm"
#include "WorkSpace.h"

boolean VPEPostIt::VPEPostItClassInitialized = FALSE;

String VPEPostIt::DefaultResources[] =
{
  ".allowVerticalResizing:      False",
  ".allowHorizontalResizing:    False",
  ".lineInvisibility:		False",
  ".decorator.topOffset:      	2",
  ".decorator.bottomOffset:   	2",
  ".decorator.leftOffset:     	2",
  ".decorator.rightOffset:    	2",
  ".decorator.marginWidth:    	0",
  ".decorator.marginHeight:   	0",
  ".decorator.marginTop:    	0",
  ".decorator.marginLeft:   	0",
  ".decorator.marginRight:    	0",
  ".decorator.marginBottom:   	0",
  ".decorator.highlightThickness:   	0",
  ".decorator.shadowThickness: 		0",

  NUL(char*)
};

//
// This causes the vpe annotation to use a special color when the color menu
// is set back to default.  Without this, the annoation would use black and
// it's difficult to make the pixmap look decent when the color is set to black
// so I'll do it this way.  If you change this color, then look in Decorator.C
// for the list of colors already provided.  It might be better to use one
// of those to go a little easier on the colormap.
//
#define PRIVATE_DEFAULT "#505050"

VPEPostIt::VPEPostIt(boolean developerStyle) : VPEAnnotator (developerStyle, "VPEPostIt")
{
    this->bg_pixmap = NUL(Pixmap);
}

VPEPostIt::~VPEPostIt()
{
    if (this->bg_pixmap) {
	Display* d = theDXApplication->getDisplay();
	XFreePixmap(d, this->bg_pixmap);
	this->bg_pixmap = NUL(Pixmap);
    }
}

void
VPEPostIt::initialize()
{
    if (!VPEPostIt::VPEPostItClassInitialized) {
	this->setDefaultResources(theApplication->getRootWidget(),
		VPEPostIt::DefaultResources);
	VPEPostIt::VPEPostItClassInitialized = TRUE;
    }

    if (!VPEAnnotator::DragDictionaryInitialized) {
	if ((theDXApplication->appAllowsSavingNetFile()) &&
	    (theDXApplication->appAllowsSavingCfgFile()) &&
	    (theDXApplication->appAllowsEditorAccess())) {
	    this->addSupportedType (Decorator::Trash, DXTRASH, FALSE);
	    this->addSupportedType (Decorator::Modules, DXMODULES, FALSE);
	}
	// Don't use text because Shift+drag breaks
	//this->addSupportedType (Decorator::Text, "TEXT", FALSE);
 
	VPEAnnotator::DragIcon = this->createDragIcon (moduledrag_width,
	    moduledrag_height, (char *)moduledrag_bits, (char *)moduledragmask_bits);
	VPEAnnotator::DragDictionaryInitialized = TRUE;
    }

}
 

Decorator*
VPEPostIt::AllocateDecorator (boolean devStyle)
{
    return new VPEPostIt (devStyle);
}

//
// Determine if this Component is of the given class.
//
boolean VPEPostIt::isA(Symbol classname)
{
    Symbol s = theSymbolManager->registerSymbol(ClassVPEPostIt);
    if (s == classname) return TRUE;
    return this->VPEAnnotator::isA(classname);
}


void VPEPostIt::setLabel(const char *newStr, boolean)
{
    if (this->labelString) XmStringFree(this->labelString);
    this->labelString = 0;

    char *filtered =  WorkSpaceComponent::FilterLabelString(newStr);
    int len = strlen(filtered);
 
    // remove trailing whitespace because of Motif bug in XmString.
    int i;
    for (i=len-1; i>=0; i--) {
	if ((filtered[i] == ' ')||(filtered[i]=='\t')||(filtered[i]=='\n'))
	    filtered[i] = '\0';
	else break;
    }
    char *font = DuplicateString(this->getFont());
    this->labelString = XmStringCreateLtoR (filtered, font);
    delete filtered;
    delete font;
}

boolean VPEPostIt::printPostScriptPage (FILE *f)
{
Pixel pixel;
int xpos, ypos, xsize, ysize;
float red,green,blue;

    this->getXYPosition (&xpos,&ypos);
    this->getXYSize (&xsize, &ysize);
    XtVaGetValues (this->customPart, XmNforeground, &pixel, NULL);
    VPEAnnotator::PixelToRGB (this->customPart, pixel, &red, &green, &blue);
    if (fprintf(f,"%f %f %f %d %d %d %d Box\n",
	    red,green,blue,
	    xpos, ypos, xsize, ysize) < 0)
	return FALSE;

    return TRUE;
}

void VPEPostIt::completeDecorativePart()
{

#if defined(PRIVATE_DEFAULT)
    //
    // Make the foreground yellow by default.
    //
    boolean yellow_by_default = FALSE;
    if (!this->isResourceSet(XmNforeground)) {
	this->setResource (XmNforeground, PRIVATE_DEFAULT);
	yellow_by_default = TRUE;
    }
#endif

    this->VPEAnnotator::completeDecorativePart();
    this->makePixmap();

#if defined(PRIVATE_DEFAULT)
    if (yellow_by_default)
	this->setResource (XmNforeground, NUL(const char *));
#endif
}

void VPEPostIt::makePixmap()
{
    //
    // Install the bitmap and set width and height accordingly.
    // N.B.  If you create a pixmap each time you create this decorator,
    // then you've created lots of pixmaps and must destroy each one.
    // If you create only one, then you can't change the color.
    //
    Display *d = theDXApplication->getDisplay();
    Window w = XtWindow(this->getWorkSpace()->getRootWidget());
    int depth;
    Pixel fg, bg, ts, bs;
    Screen* scr;
    Colormap cmap;
    XtVaGetValues (this->customPart,
	XmNdepth, &depth, 
	XmNforeground, &fg,
	XmNbackground, &bg,
	XmNscreen, &scr,
	XmNcolormap, &cmap,
    NULL);
    //
    // Want a slightly lighter background color, so use Motif's color calculation
    // to get topShadowColor.  Use that as background and we won't be taking an
    // extra slot in the color map.
    //
    XmGetColors (scr, cmap, bg, NUL(Pixel*), &ts, &bs, NUL(Pixel*));
    float red, green, blue;
    VPEAnnotator::PixelToRGB (this->customPart, fg, &red, &green, &blue);
    float light = (red*0.30) + (0.59*green) + (0.11*blue);
    boolean extreme_color = FALSE;
    if (light >= 0.55)
	extreme_color = TRUE;
    else if (light == 0)
	extreme_color = TRUE;
    bg = (extreme_color? bs : ts);
    if (this->bg_pixmap) {
	XFreePixmap(d, this->bg_pixmap);
	this->bg_pixmap = NUL(Pixmap);
    }
    this->bg_pixmap = XCreatePixmapFromBitmapData(d, w, postit_bits,
	postit_width, postit_height, fg, bg, depth);
    XtVaSetValues (this->customPart,
	XmNlabelType, XmPIXMAP,
	XmNlabelPixmap, this->bg_pixmap,
	XmNwidth, postit_width,
	XmNheight, postit_height,
	XmNmarginTop, 0,
	XmNmarginLeft, 0,
	XmNmarginRight, 0,
	XmNmarginBottom, 0,
    NULL);
    XtVaSetValues (this->getRootWidget(),
	XmNwidth, 0,
	XmNheight, 0,
    NULL);
}

void VPEPostIt::setFont (const char *font)
{
    if ((font)&&(font[0]))
	strcpy (this->font, font);
    else
	this->font[0] = '\0'; // same as XmSTRING_DEFAULT_CHARSET
}


//
// This implements a different default foreground color, i.e. yellow
// It also recreates the pixmap whenever foreground changes.  Otherwise,
// the pixmap is a static picture.
//
void VPEPostIt::setResource (const char *res, const char *val)
{
    if (EqualString(res, XmNforeground)) {
	boolean yellow_by_default = FALSE;

#if defined(PRIVATE_DEFAULT)
	//
	// Make the foreground yellow by default.
	//
	if ((!val) || (!val[0])) {
	    this->VPEAnnotator::setResource (XmNforeground, PRIVATE_DEFAULT);
	    yellow_by_default = TRUE;
	} else 
#endif
	    this->VPEAnnotator::setResource (res, val);

	this->makePixmap();

	if (yellow_by_default)
	    this->VPEAnnotator::setResource (XmNforeground, NUL(const char *));
    } else 
	this->VPEAnnotator::setResource (res, val);
}

