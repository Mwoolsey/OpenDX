/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"


#include "ImageFormat.h"
#include "ImageNode.h"
#include "ImageFormatDialog.h"
#include "SymbolManager.h"
#include "XmUtility.h"
#if defined(DXD_WIN) || defined(OS2)
#define unlink _unlink
#endif
#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif
#include <Xm/Text.h>
#include <Xm/TextF.h>

String ImageFormat::DefaultResources[] = {
    NUL(char*)
};


ImageFormat::ImageFormat (const char *name, ImageFormatDialog *dialog) : UIComponent(name)
{
    this->menuButton = NUL(Widget);
    this->dialog = dialog;
    ImageNode *node = this->dialog->getNode();
    int junk;
    node->getResolution(this->width, junk);
    node->getAspect(this->aspect);
}

ImageFormat::~ImageFormat()
{
}

boolean ImageFormat::isMetric()
{
    return this->dialog->isMetric();
}

const char* ImageFormat::getRecordFormat()
{
static char formstr[128];
double gamma = this->dialog->getGamma();
const char *cp = this->paramString();

    if ((this->dialog->dirtyGamma() == 0) && (gamma == DEFAULT_GAMMA))
	if (this->dialog->getDelayedColors())
	    sprintf (formstr, "%s delayed=1", cp);
	else
	    strcpy (formstr, cp);
    else 
	if (this->dialog->getDelayedColors())
	    sprintf (formstr, "%s gamma=%lg delayed=1", cp, gamma);
	else
	    sprintf (formstr, "%s gamma=%lg", cp, gamma);

    return formstr;
}

boolean ImageFormat::useLocalFormat()
{
ImageNode *node = this->dialog->getNode();
    boolean formcon = node->isRecordFormatConnected();
    if (formcon) return FALSE;

    if (this->dialog->dirtyGamma()) return TRUE;
    return this->dialog->dirtyFormat();
}

//
// This only works for widgets whose normal foreground color is black.
//
void ImageFormat::setTextSensitive (Widget w, boolean sens)
{
    ASSERT ((XtClass(w) == xmTextWidgetClass) || (XtClass(w) == xmTextFieldWidgetClass));
    if (sens) {
	XtVaSetValues (w, 
	    XmNeditable, True,
	    RES_CONVERT(XmNforeground, "black"),
	NULL);
    } else {
	XtVaSetValues (w, 
	    XmNeditable, False,
	    RES_CONVERT(XmNforeground, "grey40"),
	NULL);
    }
}

void ImageFormat::eraseOutputFile (const char *fname)
{
    unlink (fname);
}


boolean ImageFormat::isA(Symbol classname)
{
    Symbol s = theSymbolManager->registerSymbol(ClassImageFormat);
    return (s == classname);
}
