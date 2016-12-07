/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#include <Xm/Label.h>
#include "DXStrings.h"
#include "VPEAnnotator.h"
#include "ListIterator.h"
#include "WorkSpace.h"
#include "moduledrag.bm"
#include "moduledragmask.bm"
#include "DynamicResource.h"
#include "Dictionary.h"
#include "DXApplication.h"
#include "SetAnnotatorTextDialog.h"


boolean VPEAnnotator::VPEAnnotatorClassInitialized = FALSE;
boolean VPEAnnotator::DragDictionaryInitialized = FALSE;

Dictionary* VPEAnnotator::DragTypeDictionary = new Dictionary;
Widget VPEAnnotator::DragIcon = 0;

String VPEAnnotator::DefaultResources[] =
{
    ".lineInvisibility:		True",
    ".decorator.alignment:      XmALIGNMENT_BEGINNING",
    ".decorator.topOffset:      2",
    ".decorator.bottomOffset:   2",
    ".decorator.leftOffset:     2",
    ".decorator.rightOffset:    2",
    ".decorator.marginTop:      4",
    ".decorator.marginLeft:     4",
    ".decorator.marginRight:    4",
    ".decorator.marginBottom:   4",
    ".decorator.marginWidth:    0",
    ".decorator.marginHeight:   0",
    ".allowHorizontalResizing:  False",
    NUL(char*)
};

VPEAnnotator::VPEAnnotator(boolean developerStyle) : 
    LabelDecorator (developerStyle, "VPEAnnotator")
{
    this->layout_information = NUL(Base*);
}
VPEAnnotator::VPEAnnotator(boolean developerStyle, const char *name) : 
    LabelDecorator (developerStyle, name)
{
    this->layout_information = NUL(Base*);
}

VPEAnnotator::~VPEAnnotator()
{
    if (this->layout_information) delete this->layout_information;
}
 
void VPEAnnotator::initialize()
{
    //
    // Initialize default resources (once only).
    //
    if (!VPEAnnotator::VPEAnnotatorClassInitialized) {
	this->setDefaultResources(theApplication->getRootWidget(),
                                        VPEAnnotator::DefaultResources);
	this->setDefaultResources(theApplication->getRootWidget(),
                                        LabelDecorator::DefaultResources);
	this->setDefaultResources(theApplication->getRootWidget(),
                                        Decorator::DefaultResources);
	this->setDefaultResources(theApplication->getRootWidget(),
                                        WorkSpaceComponent::DefaultResources);
        VPEAnnotator::VPEAnnotatorClassInitialized = TRUE;

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

void
VPEAnnotator::completeDecorativePart()
{
DynamicResource *dr;
 
    this->setDragIcon (VPEAnnotator::DragIcon);
    this->setDragWidget (this->customPart);
      
    if (this->setResourceList) {
	  ListIterator it(*this->setResourceList);
	  while ( (dr = (DynamicResource*)it.getNext()) ) {
	      // don't bother checking return value from setRootWidget because it will
	      // always return FALSE for resources belonging to xmLabelWidgetClass if
	      // there is no XmLabel widget already in its list.  Not a problem.
	      dr->setRootWidget(this->getRootWidget());
	  }
    }
    this->setPrintType(PrintCut);

    if (!this->labelString) {
	XmString xmstr1 = XmStringCreate ("Visual program comment", "canvas");
	XmString xmstr2 = XmStringCreate ("        double click to edit", "small_bold");
	XmString xmstr3 = XmStringSeparatorCreate();
	XmString xmstr4 = XmStringConcat (xmstr1, xmstr3);
	XmString xmstr5 = XmStringConcat (xmstr4, xmstr2);
	XmStringFree(xmstr1);
	XmStringFree(xmstr2);
	XmStringFree(xmstr3);
	XmStringFree(xmstr4);
	XtVaSetValues (this->customPart, XmNlabelString, xmstr5, NULL);
	XmStringFree(xmstr5);
    }
}



Decorator*
VPEAnnotator::AllocateDecorator (boolean devStyle)
{
    return new VPEAnnotator (devStyle);
}

//
// Determine if this Component is of the given class.
//
boolean VPEAnnotator::isA(Symbol classname)
{
    Symbol s = theSymbolManager->registerSymbol(ClassVPEAnnotator);
    if (s == classname) return TRUE;
    return this->LabelDecorator::isA(classname);
}



void VPEAnnotator::openDefaultWindow()
{
    if (!this->setTextDialog)
	this->setTextDialog = new SetAnnotatorTextDialog 
	    (XtParent(this->getRootWidget()), FALSE, this);

    this->setTextDialog->post();
}



boolean VPEAnnotator::printPostScriptPage (FILE *f)
{
int xpos, ypos, xsize, ysize;

    //
    // The width is the same for all lines, but the total height must be shared
    // among all the lines.  The strategy is to generate postscript code which
    // will handle each line inside the VPEAnnotator.  You can't just output
    // the text of the VPEAnnotator complete with carriage returns and expect
    // it to show up properly.
    //
    this->getXYPosition (&xpos, &ypos);
    this->getXYSize (&xsize, &ysize);
    const char *label = this->getLabelValue();
    int lines = 1;
    int i = 0;
    while (label[i]) {
	if ((label[i] == '\n') && (i) && (label[i+1])) lines++;
	i++;
    }


    //
    // Now we know how many lines we're handling.  For each line calculate
    // line length.   We need to know the longest line for font scaling.
    //
    int* line_lengths = new int[1+lines];
    int next = 0;
    int line_length = 0;
    boolean line_waiting = FALSE;
    i = 0;
    while (label[i]) {
	if ((label[i] == '\n') && (i) && (label[i+1])) {
	    line_lengths[next] = line_length;
	    line_length = 0;
	    line_waiting = FALSE;
	    next++;
	} else {
	    line_length++;
	    line_waiting = TRUE;
	}
	i++;
    }
    if (line_waiting) {
	line_lengths[next] = line_length;
	next++;
    }
    line_lengths[next] = 0;


    //
    // Now we know the length of every line, the slot of the longest line.
    // Get space for each line.
    //
    String *line_buckets = new String[1+lines];
    for (i=0; i<lines; i++)
	line_buckets[i] = new char[1+(line_lengths[i]<<1)];
    line_buckets[i] = NUL(char*);


    //
    // Copy each individual line out of the source.  As the copy happens,
    // escape parens.
    //
    i = 0;
    next = 0;
    int slot = 0;
    line_waiting = FALSE;
    while (label[i]) {
	if (label[i] == '\n') {
	    line_buckets[next][slot] = '\0';
	    slot = 0;
	    next++;
	    line_waiting = FALSE;
	    if (!line_buckets[next]) break;
	} else {
	    switch (label[i]) {
		case '(':
		case ')':
		    line_buckets[next][slot++] = '\\';
		    line_buckets[next][slot++] = label[i];
		    break;
		default:
		    line_buckets[next][slot++] = label[i];
		    break;
	    }
	    line_waiting = TRUE;
	}
	i++;
    }
    if (line_waiting) {
	line_buckets[next][slot] = '\0';
    }


    int divisor = lines + 1;
    float font_scale = ysize/divisor;
    if (fprintf(f,"/%s findfont \n[ %f 0 0 -%f 0 0 ] makefont setfont\n",
	this->getPostScriptFont(), font_scale, font_scale) <= 0)
	return FALSE;

    int yofs = (int)font_scale;
    // 
    // increment the y coord because we're not using
    // justification and the text coords are for the base line of the character.
    ypos+= yofs;

    //
    // The the color of the current pen.
    //
    float red,green,blue;
    Pixel pixel;
    XtVaGetValues (this->customPart, XmNforeground, &pixel, NULL);
    VPEAnnotator::PixelToRGB (this->customPart, pixel, &red, &green, &blue);
    if (fprintf (f, "%f %f %f setrgbcolor\n", red, green, blue) <= 0)
	return FALSE;


    unsigned char align;
    XtVaGetValues (this->customPart, XmNalignment, &align, NULL);
    if ((align == XmALIGNMENT_CENTER) && (lines > 1)) {
	for (i=0; i<lines; i++) {
	    //
	    // This postscript code will leave x,y coords on the stack for 
	    // the moveto operation.  
	    // The y coord is calculated here in C (unlike the StandIn version).
	    // The x coord is munged according to the stringwidth.
	    //
	    if (fprintf(f, 
		"%d (%s) stringwidth pop\n"
		"neg %d add 2 div %d add %% x position\n"
		"2 1 roll\n"
		"moveto (%s) show\n",
		ypos + (i*yofs), line_buckets[i],
		xsize, xpos,
		line_buckets[i]) <= 0)
		return FALSE;
	}
    } else if ((align == XmALIGNMENT_BEGINNING) || (lines == 1)) {
	for (i=0; i<lines; i++) {
	    if (fprintf(f, "%d %d moveto (%s) show\n",
		xpos, ypos + (i*yofs), line_buckets[i]) <= 0) return FALSE;
	}
    } else {
	for (i=0; i<lines; i++) {
	    if (fprintf(f, 
		"(%s) stringwidth\n"
		"pop %d %% y position\n"
		"2 1 roll\n"
		"neg %d add 1 div %d add %% x position\n"
		"2 1 roll\n"
		"moveto (%s) show\n",
		line_buckets[i],
		ypos + (i*yofs),
		xsize, xpos,
		line_buckets[i]) <= 0)
		return FALSE;
	}
    }

    delete line_lengths;
    for (i=0; i<lines; i++) delete line_buckets[i];
    delete line_buckets;

    if (fprintf (f, "0.0 0.0 0.0 setrgbcolor\n") <= 0)
	return FALSE;

    return TRUE;
}

#if WORKSPACE_PAGES
boolean VPEAnnotator::printComment (FILE *f)
{
    if (!this->LabelDecorator::printComment(f)) return FALSE;
    if (!this->printGroupComment(f)) return FALSE;
    return TRUE;
}

boolean VPEAnnotator::parseComment (const char *comment, const char *file, int line)
{
    if (this->parseGroupComment(comment, file, line)  == TRUE) 
	return TRUE;
    return this->LabelDecorator::parseComment (comment, file, line);
}
#endif

const char* VPEAnnotator::getPostScriptFont()
{
    const char* fontname = this->getFont();
    if (fontname) {
	if (strstr(fontname, "bold"))
	    return "Helvetica-Bold";
	if (strstr(fontname, "oblique"))
	    return "Helvetica-Oblique";
	if (strstr(fontname, "normal"))
	    return "Helvetica";
    }
    return "Helvetica-Bold";
}
//
// FIXME:  This chunk of code is copied from StandIn.  It needs to live
// higher up the food chain - perhaps UIComponent - so that it can be
// shared.
//
void VPEAnnotator::PixelToRGB (Widget w, Pixel p, float *r, float *g, float *b)
{
    Colormap cmap = 0;
    XColor xcolor;
    Display *d = XtDisplay(w);
    xcolor.pixel = p;
    XtVaGetValues (w, XmNcolormap, &cmap, NULL); //XmNcolormap comes from Core
    ASSERT(cmap);
    XQueryColor(d,cmap,&xcolor);

    *r = xcolor.red   / 65535.0;
    *g = xcolor.green / 65535.0;
    *b = xcolor.blue  / 65535.0;
}


void VPEAnnotator::postTextGrowthWork()
{
    if (this->requiresLineReroutingOnResize() == FALSE) return ;
    WorkSpace* ws = this->getWorkSpace();
    if (!ws) return ;
    if (!ws->getRootWidget()) return ;
    ws->beginManyPlacements();
    ws->endManyPlacements();
}

void VPEAnnotator::setLayoutInformation (Base* info)
{
    if (this->layout_information)
	delete this->layout_information;
    this->layout_information = info;
}
