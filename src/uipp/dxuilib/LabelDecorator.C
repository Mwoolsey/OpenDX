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


#include "XmUtility.h"
#include "DXStrings.h"
#include "LabelDecorator.h"
#include "DXApplication.h"
#include <Xm/Label.h>
#include "SetDecoratorTextDialog.h"
#include "WarningDialogManager.h"
#include "ListIterator.h"
#include "Dictionary.h"
#include "ntractor.bm"
#include "ntractormask.bm"
#include "Dictionary.h"
#include "DictionaryIterator.h"

#include "CommentStyleUser.h"

#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif

#if defined(HAVE_SYS_TYPES_H)
#include <sys/types.h>
#endif

#if defined(HAVE_CTYPE_H)
#include <ctype.h>
#endif

#if defined(HAVE_NETDB_H)
#include <netdb.h>
#endif

// gethostname is needed by decodeDragType 
#if defined(NEEDS_GETHOSTNAME_DECL)
extern "C" int gethostname(char *address, int address_len);
#endif

#if 0
#if defined(DXD_WIN)
#include <winsock.h>
#endif
#endif

boolean LabelDecorator::LabelDecoratorClassInitialized = FALSE;

Dictionary* LabelDecorator::CommentStyleDictionary = NUL(Dictionary*);

Dictionary* LabelDecorator::DragTypeDictionary = new Dictionary;
Widget LabelDecorator::DragIcon;

String LabelDecorator::DefaultResources[] =
{
  ".decorator.alignment:      XmALIGNMENT_CENTER",
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

  //
  // Added because of the change made to reduce the font sizes used thruout
  // the ui.  The fonts used in decorators must remain the same just as the
  // fonts used in the StandIn must remain the same.  Otherwise we have
  // trouble reading in existing nets since they're filled with position
  // and size information.
  //
#if defined(DXD_OS_NON_UNIX)
    "*fontList:\
-adobe-helvetica-bold-r-normal--14-*iso8859-1=bold,\
-adobe-helvetica-bold-r-normal--14-*iso8859-1=canvas,\
-adobe-helvetica-bold-r-normal--12-*=small_bold,\
-adobe-helvetica-bold-r-normal--12-*=small_canvas,\
-adobe-helvetica-bold-r-normal--18-*=big_bold,\
-adobe-helvetica-bold-r-normal--24-*=huge_bold,\
-adobe-helvetica-medium-r-normal--14-*=normal,\
-adobe-helvetica-medium-r-normal--12-*=small_normal,\
-adobe-helvetica-medium-r-normal--18-*=big_normal,\
-adobe-helvetica-medium-r-normal--24-*=huge_normal,\
-adobe-helvetica-medium-o-normal--12-*=small_oblique,\
-adobe-helvetica-medium-o-normal--18-*=big_oblique,\
-adobe-helvetica-medium-o-normal--24-*=huge_oblique,\
-adobe-helvetica-medium-o-normal--14-*=oblique",
#else
    "*fontList:\
-adobe-helvetica-bold-r-normal--14-*iso8859-1=bold,\
-adobe-helvetica-bold-r-normal--14-*iso8859-1=canvas,\
-adobe-helvetica-bold-r-normal--12-*iso8859-1=small_bold,\
-adobe-helvetica-bold-r-normal--12-*iso8859-1=small_canvas,\
-adobe-helvetica-bold-r-normal--18-*iso8859-1=big_bold,\
-adobe-helvetica-bold-r-normal--24-*iso8859-1=huge_bold,\
-adobe-helvetica-medium-r-normal--14-*iso8859-1=normal,\
-adobe-helvetica-medium-r-normal--12-*iso8859-1=small_normal,\
-adobe-helvetica-medium-r-normal--18-*iso8859-1=big_normal,\
-adobe-helvetica-medium-r-normal--24-*iso8859-1=huge_normal,\
-adobe-helvetica-medium-o-normal--12-*iso8859-1=small_oblique,\
-adobe-helvetica-medium-o-normal--18-*iso8859-1=big_oblique,\
-adobe-helvetica-medium-o-normal--24-*iso8859-1=huge_oblique,\
-adobe-helvetica-medium-o-normal--14-*iso8859-1=oblique",
#endif
//
// It might be nice to set these.  Everthing gets pinned top,left in user style
// panels but few things get pinned right and/or bottom.  Should label decorators?
// I think so.  The counter arg is that doing so prevents keeping labels and
// interactors lined up following a c.p. resize.
//  ".pinLeftRight:   	      True",
//  ".pinTopBottom:   	      True",

  NUL(char*)
};

// LloydT demoed a user style panel on a sgi screen where a font turned out
// to be a different size than when he had saved the .cfg.  So define a value
// which will be used for margin{Left,Right} on the XmLabel and set to 0 in
// case of :
// 	running in user style &&
//	the label is bigger than the space available.
#define LLOYDS_SAFETY_MARGIN 4

LabelDecorator::LabelDecorator(boolean developerStyle, const char *name) : 
	Decorator (xmLabelWidgetClass, name, developerStyle)
{
    this->labelString = 0;
    this->setTextDialog = 0;
    this->font[0] = '\0';  // equals XmSTRING_DEFAULT_CHARSET
    this->otherStrings = NUL(Dictionary*);
    LabelDecorator::BuildTheCommentStyleDictionary();
}
LabelDecorator::LabelDecorator(boolean developerStyle) : 
	Decorator (xmLabelWidgetClass, "LabelDecorator", developerStyle)
{
    this->labelString = 0;
    this->setTextDialog = 0;
    this->font[0] = '\0';  // equals XmSTRING_DEFAULT_CHARSET
    this->otherStrings = NUL(Dictionary*);
    LabelDecorator::BuildTheCommentStyleDictionary();
}

void LabelDecorator::BuildTheCommentStyleDictionary()
{
    if (!LabelDecorator::CommentStyleDictionary) {
	LabelDecorator::CommentStyleDictionary = new Dictionary;
	CommentStyle* csty;

	//
	// Add individual entries... 1 for each type of comment.
	//
	csty = new CommentStyleUser;
	LabelDecorator::CommentStyleDictionary->addDefinition(csty->getKeyword(), csty);

	//
	// At this point, create/add comment style classes for other purposes like
	// saving .net/.cfg contents.  See EditorWindow::setCopiedNet().
	//
	//csty = new CommentStyleNet;
	//LabelDecorator::CommentStyleDictionary->addDefinition(csty->getKeyword(), csty);

	//csty = new CommentStyleCfg;
	//LabelDecorator::CommentStyleDictionary->addDefinition(csty->getKeyword(), csty);

    }
}

LabelDecorator::~LabelDecorator()
{
    if (this->labelString) {
	XmStringFree(this->labelString);
	this->labelString = 0;
    }
    if (this->setTextDialog) {
	delete this->setTextDialog;
	this->setTextDialog = 0;
    }
    if (this->otherStrings) {
	DictionaryIterator di(*this->otherStrings);
	char* other_string;
	while ( (other_string = (char*)di.getNextDefinition()) )
	    delete other_string;
	delete this->otherStrings;
	this->otherStrings = NUL(Dictionary*);
    }
}

const char* LabelDecorator::getOtherString(const char* keyword)
{
    if (!this->otherStrings) return NUL(char*);
    return (const char*)this->otherStrings->findDefinition(keyword);
}

void LabelDecorator::setOtherString(const char* keyword, const char* value)
{
    if (!this->otherStrings)
	this->otherStrings = new Dictionary;

    char* cp = NUL(char*);
    char* new_value = DuplicateString(value);
    this->otherStrings->replaceDefinition(keyword, (void*)new_value, (void**)&cp);
    if (cp) delete cp;
}

 
void LabelDecorator::initialize()
{
    //
    // Initialize default resources (once only).
    //
    if (NOT LabelDecorator::LabelDecoratorClassInitialized)
    {
	this->setDefaultResources(theApplication->getRootWidget(),
                                        LabelDecorator::DefaultResources);
	this->setDefaultResources(theApplication->getRootWidget(),
                                        Decorator::DefaultResources);
	this->setDefaultResources(theApplication->getRootWidget(),
                                        WorkSpaceComponent::DefaultResources);
        LabelDecorator::LabelDecoratorClassInitialized = TRUE;

	LabelDecorator::DragIcon = this->createDragIcon 
	 (ntractor_width,ntractor_height,(char *)ntractor_bits,(char *)ntractormask_bits);
	this->addSupportedType (Decorator::Modules, DXINTERACTORS, TRUE);
	this->addSupportedType (Decorator::Trash, DXTRASH, FALSE);
	// Don't use text because Shift+drag breaks
	//this->addSupportedType (Decorator::Text, "TEXT", FALSE);
    }
}



void LabelDecorator::setArgs (Arg *args, int *n)
{
int i = *n;

    if (this->labelString) {
	XtSetArg (args[i], XmNlabelString, this->labelString); i++;
    }
    *n = i;
}

#define NO_TEXT_SYMBOL "<NULL>"
//
// format:
//	// decorator LabelDecorator\tpos=(%d,%d), size=%dx%d, alignment=%d, value = %s
//	// decorator LabelDecorator\tpos=(%d,%d), size=%dx%d, font=%s, value = %s
//	// decorator LabelDecorator\tpos=(%d,%d), size=%dx%d, value = %s
//
boolean LabelDecorator::printComment (FILE *f)
{
    if (!this->Decorator::printComment (f)) return FALSE;
    boolean printOK=TRUE;

    //
    // Starting in 3.1.4, print out just config info, then put the actual text
    // on following lines.
    //
    const char* cp = NUL(char*);
    if ((!cp) || (!cp[0])) cp = NO_TEXT_SYMBOL;
    if (this->font[0]) {
	if (fprintf (f, ", font=%s, value = %s\n", this->font, cp) < 0) 
	    return FALSE;
    } else {
	if (fprintf (f, ", value = %s\n", cp) < 0) 
	    return FALSE;
    }

    Dictionary* csty_dict = this->getCommentStyleDictionary();
    DictionaryIterator di(*csty_dict);
    CommentStyle* csty;
    while ((printOK) && (csty = (CommentStyle*)di.getNextDefinition()))
	printOK = csty->printComment(this, f);

    if (this->setResourceList) {
	ListIterator it(*this->setResourceList);
	DynamicResource *dr;
	while ((printOK)&&(dr = (DynamicResource *)it.getNext())) {
	    printOK = dr->printComment(f);
	}
    }

    return printOK;
}

//
// The 3.0 beta used "alignment = %d" on this comment line.  It was removed when
// DynamicResource came into being.  All such settings were moved to separate lines.
//
boolean LabelDecorator::parseComment (const char *comment, const char *file, int l)
{
int items_parsed;
char text[1024];
char font[64];

    //
    // As of 3.1.4, we want to print the text of the decorator on multiple lines
    // using statefullness already implemented in ControlPanel and now in Network
    // as well, with the addition of vpe pages. 
    //
    if (EqualSubstring(" annotation",comment,11)) {
	char keyword[64];
	boolean parsed = FALSE;
	items_parsed = sscanf (comment, " annotation %[^:]", keyword);
	if (items_parsed == 1) {
	    Dictionary* csty_dict = this->getCommentStyleDictionary();
	    DictionaryIterator di(*csty_dict);
	    CommentStyle* csty;
	    while ( (csty = (CommentStyle*)di.getNextDefinition()) ) {
		if (csty->parseComment(this, comment, file, l)) {
		    parsed = TRUE;
		    break;
		}
	    }
	} 
	if (!parsed) 
	    WarningMessage ("Unrecognized text in LabelDecorator "
			    "Comment (file %s, line %d)", file, l);
	return TRUE;
    }



    if (!this->Decorator::parseComment (comment, file, l)) return FALSE;

    const char *cp = strstr (comment, ", value =");
    if (!cp) {
	WarningMessage ("Unrecognized text in LabelDecorator Comment (file %s, line %d)",
		file, l);
	return TRUE;
    }

    items_parsed = sscanf(cp, ", value = %[^\n]", text);

    if (items_parsed != 1) {
	WarningMessage ("Unrecognized text in LabelDecorator Comment (file %s, line %d)",
		file, l);
	return TRUE;
    }

    if (EqualString(text, NO_TEXT_SYMBOL))
	text[0] = '\0';

    const char *fontcp = strstr (comment, ", font=");
    if ((fontcp)&&(fontcp<cp)) {
	items_parsed = sscanf (fontcp, ", font=%[^,]", font);
	if (items_parsed != 1) {
	    WarningMessage 
	     ("Unrecognized text in LabelDecorator Comment (file %s, line %d)", file, l);
	    return TRUE;
	}
	this->setFont(font);
    }

    // set font before text because setting font results in remaking the text.
    if (text[0])
	this->setLabel(text); 

    return TRUE;
}


void
LabelDecorator::completeDecorativePart()
{
    this->setDragIcon (LabelDecorator::DragIcon);
    this->setDragWidget (this->customPart);

    if (!this->labelString) {
	XmString xmstr1 = XmStringCreate ("ControlPanel comment\n", "bold");
	XmString xmstr2 = XmStringCreate ("double click to edit", "small_bold");
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

void
LabelDecorator::uncreateDecorator()
{
    if (this->setTextDialog) {
	delete this->setTextDialog;
	this->setTextDialog = 0;
    }
    this->Decorator::uncreateDecorator();
}

Decorator*
LabelDecorator::AllocateDecorator (boolean devStyle)
{
    return new LabelDecorator (devStyle);
}

//
// Determine if this Component is of the given class.
//
boolean LabelDecorator::isA(Symbol classname)
{
    Symbol s = theSymbolManager->registerSymbol(ClassLabelDecorator);
    if (s == classname) return TRUE;
    return this->Decorator::isA(classname);
}

void
LabelDecorator::setFont (const char *font)
{
const char *cp;

    if ((font)&&(font[0]))
	strcpy (this->font, font);
    else
	this->font[0] = '\0'; // same as XmSTRING_DEFAULT_CHARSET

    if (this->labelString) {

	cp = this->getLabelValue();

	if ((this->customPart)&&(this->labelString)) {
	    XmString xmstr = XmStringCreateLtoR ((char *)cp, this->font);

	    Dimension   mw,mh,mt,mb,ml,mr; // margin sizes
	    Dimension   ht,st; // thicknesses
	    Dimension   width, height;
	    Dimension   neww, newh;
	    XmFontList  xmfl;
	    XtWidgetGeometry req;

	    mw = mh = mt = mb = ml = mr = ht = st = 0;
	    XtVaGetValues (this->customPart, XmNfontList, &xmfl, XmNmarginWidth, &mw,
		XmNmarginHeight, &mh, XmNmarginWidth, &mw, XmNmarginHeight, &mh,
		XmNmarginLeft, &ml, XmNmarginRight, &mr, XmNmarginTop, &mt,
		XmNmarginBottom, &mb, XmNshadowThickness, &st, XmNhighlightThickness, &ht,
		XmNwidth, &width, XmNheight, &height, NULL);
	    XmStringExtent (xmfl, xmstr, &neww, &newh);
	    req.width = neww + (2*(mw + ht + st)) + ml + mr;
	    req.height = newh + (2*(mh + ht + st)) + mt + mb;
	    req.request_mode = 0;
	    if ((req.width > width) || (this->resizeOnUpdate()))
		req.request_mode|= CWWidth;
	    req.request_mode|= CWHeight;
	    if (req.request_mode) XtMakeGeometryRequest (this->customPart, &req, NULL);
	    XmStringFree (xmstr);
	}
	this->setLabel(cp);
    }
}

void LabelDecorator::setLabel (const char *newStr, boolean)
{ 
XmStringContext cxt;
char *text, *tag;
XmStringDirection dir;
Boolean sep;
unsigned char rp;
int i, linesNew, linesOld;
XmString oldXmStr = NULL;


    if (this->labelString) XmStringFree (this->labelString);
    this->labelString = 0;

    linesNew = linesOld = 1;
    char *filtered =  WorkSpaceComponent::FilterLabelString(newStr);
    int len = strlen(filtered);

    // remove trailing whitespace (but not \t) because of Motif bug in XmString.
    for (i=len-1; i>=0; i--) {
	if ((filtered[i] == ' ')||(filtered[i]=='\n'))
	    filtered[i] = '\0';
	else break;
    }
    for (i=0; filtered[i]!='\0'; i++) if (filtered[i]=='\n') linesNew++;
    this->labelString = XmStringCreateLtoR (filtered, this->font);
    delete filtered;

    if (!this->customPart) return ;

    XtVaGetValues (this->customPart, XmNlabelString, &oldXmStr, NULL);
    ASSERT(oldXmStr);
    if (XmStringInitContext (&cxt, oldXmStr)) {
        while (XmStringGetNextSegment (cxt, &text, &tag, &dir, &sep)) {
            if (sep) linesOld++;
	    // unsure about this line. The motif manual doesn't do this
	    // but purify complains about memory loss
	    if (tag) XtFree(tag);
            XtFree(text);
        }
        XmStringFreeContext(cxt);
    }
    XmStringFree(oldXmStr);

    // toggle XmNresizePolicy so that the decorator doesn't snap back
    // to a reasonable size.  If the old string is taller/shorter than the new string
    // then let it change. 
    // Compares linesNew > 1 because we're going to disable the alignment resource
    // setting for single line decorators and we want them to be shrink-wrapped.
    XtVaGetValues (this->getRootWidget(), XmNresizePolicy, &rp, NULL);
    if ((linesNew == linesOld) && (linesNew > 1) && (this->resizeOnUpdate() == FALSE)) {
        XtVaSetValues (this->getRootWidget(), XmNresizePolicy, XmRESIZE_GROW, NULL);
    } else {
        XtVaSetValues (this->getRootWidget(), XmNresizePolicy, XmRESIZE_ANY, NULL);
    }

    XtVaSetValues (this->customPart, XmNlabelString, this->labelString, NULL);
    XtVaSetValues (this->getRootWidget(), XmNresizePolicy, rp, NULL);


//#define HACKING_WITH_RESOURCES 1
#if HACKING_WITH_RESOURCES
    // Look for 2 lines each containing 1 word.
    // If that's what you find, then the 1st line is a resource name, the
    // second line is a string representation of the resource value.
    if (linesNew == 2) {
	char line1[128], line2[128];
	char *filtered =  WorkSpaceComponent::FilterLabelString(newStr);
	line2[0] = line1[0] = '\0';
	for (i=0; filtered[i]!='\0'; i++) {
	    if (filtered[i]=='\n') {
		filtered[i] = '\0';
		strcpy (line1, filtered);
		strcpy (line2, &filtered[i+1]);
		break;
	    }
	}
	delete filtered;

	DynamicResource *dr;
	if (!this->setResourceList) {
	    this->setResourceList = new List;
	    dr = NUL(DynamicResource*);
	} else {
	    ListIterator it(*this->setResourceList);
	    while (dr = (DynamicResource *)it.getNext()) {
		if (!strcmp(dr->getResourceName(), line1)) {
		    break;
		}
	    }
	}
	if (!dr) {
	    dr = new DynamicResource(line1, this->getRootWidget());
	    this->setResourceList->appendElement((void *)dr);
	}

	// automatically checks for set contains before adding to set.
	boolean applyOK;
	if (applyOK = dr->addWidgetToNameList (this->customPart)) {
	    applyOK = dr->setData(line2); 
	}
	if (!applyOK) {
	    this->setResourceList->removeElement((void*)dr);
	    delete dr;
	}
    }
#endif
}

//
// null-terminated char * -- the XmNlabelStinrg
//
const char *
LabelDecorator::getLabelValue()
{
XmStringContext cxt;
char *text, *tag;
XmStringDirection dir;
int os;

static char *label_buf = NUL(char*);
static int label_max_size = 0;

    if (!this->labelString) return "";
    if (!XmStringInitContext (&cxt, this->labelString)) {
	XtWarning ("Can't convert compound string.");
	return "";
    }

    int lines = XmStringLineCount(this->labelString);
    int bytes = XmStringLength (this->labelString);
    int required_space = bytes + lines + 1;

    if (required_space >= label_max_size) {
	label_max_size = required_space + 512;
	if (label_buf) delete label_buf;
	label_buf = new char[label_max_size];
    }

    os = 0;
    XmStringComponentType uktag = 0;
    unsigned short ukl = 0;
    unsigned char* ukv = 0;
    tag = NUL(char*);
    while (1) {
	XmStringComponentType retVal = 
	    XmStringGetNextComponent (cxt, &text, &tag, &dir, &uktag, &ukl, &ukv);
	if (retVal == XmSTRING_COMPONENT_END) break;
	switch (retVal) {
	    case XmSTRING_COMPONENT_TEXT:
		strcpy (&label_buf[os], text);
		os+= strlen(text);
		XtFree(text),text = NUL(char*);
		break;
	    case XmSTRING_COMPONENT_SEPARATOR:
		label_buf[os++] = '\n';
		label_buf[os] = '\0';
		break;
	    default:
		if (tag) XtFree(tag),tag=NUL(char*);
		break;
	}
    }
    label_buf[os] = '\0';

    //
    // Add an extra \ before instances of '\' 'n' or the like
    // UnFilter returns NUL(char*) if no change was needed.
    //
    char *re_escaped_label_buf = 
	LabelDecorator::UnFilterString(label_buf, &required_space);
    if (re_escaped_label_buf) {
	if (required_space >= label_max_size) {
	    label_max_size = required_space + 512;
	    if (label_buf) delete label_buf;
	    label_buf = new char[label_max_size];
	}
	strcpy (label_buf, re_escaped_label_buf);
	delete re_escaped_label_buf;
    }

    ASSERT (os < label_max_size);
    XmStringFreeContext(cxt);
    return label_buf;
}


char*
LabelDecorator::UnFilterString(char* oldBuf, int* space)
{
char c2,c3;

    if ((!oldBuf) || (!oldBuf[0])) return NUL(char*);
    int i;
    int len = strlen(oldBuf);
    List addrs;

    //
    // Count up the instances.
    //
    int lenm1 = len-1;
    int lenm3 = len-3;
    for (i=0; i<lenm1; i++) {
	if (oldBuf[i] == '\\') {
	    char c = oldBuf[i+1];
	    switch (c) {
		case 'n':
		    addrs.appendElement((void*)&oldBuf[i]);
		    break;
		case 't':
		    addrs.appendElement((void*)&oldBuf[i]);
		    break;
		case 'r':
		    addrs.appendElement((void*)&oldBuf[i]);
		    break;
		case 'f':
		    addrs.appendElement((void*)&oldBuf[i]);
		    break;
		case 'b':
		    addrs.appendElement((void*)&oldBuf[i]);
		    break;
		case '0':
		    addrs.appendElement((void*)&oldBuf[i]);
		    break;
		default:
		    //
		    // If followed by 3 digits <= 7, assume escaped octal
		    //
		    c2 = oldBuf[i+2];
		    c3 = oldBuf[i+3];
		    if ((i<=lenm3) && (isdigit(c)) && (isdigit(c2)) && (isdigit(c3))) {
			addrs.appendElement((void*)&oldBuf[i]);
		    }
		    break;
	    }
	}
    }
    if (addrs.getSize() == 0) return NUL(char*);

    //
    // Copy over.
    //
    char* newbuf = new char[len+1+addrs.getSize()];
    int ns = 1;
    int j = 0;
    int list_size = addrs.getSize();
    char* next_slash = (char*)addrs.getElement(ns);
    char* cp;
    for (i=0; i<len; i++) {
	cp = &oldBuf[i];
	if ((cp >= next_slash) && (next_slash)) {
	    newbuf[j++] = '\\';
	    ns++;
	    next_slash = (ns > list_size?NUL(char*):(char*)addrs.getElement(ns));
	}
	newbuf[j++] = *cp;
    }
    newbuf[j] = '\0';

    *space = j;
    return newbuf;
}

void LabelDecorator::openDefaultWindow()
{
    ASSERT(this->getRootWidget());
    if (!this->setTextDialog) {
	Widget parent = XtParent(this->getRootWidget());
	this->setTextDialog = 
	    new SetDecoratorTextDialog (parent, FALSE, this);
    }

    this->setTextDialog->post();
}


boolean LabelDecorator::decodeDragType (int tag,
	char * a, XtPointer *value, unsigned long *length, long operation)
{
boolean retVal;
const char *cp;

    switch (tag) {
	case Decorator::Modules:
	    retVal = this->convert(this->getNetwork(), a, value, length, operation);
	    break;

	case Decorator::Trash:
	    retVal = TRUE;
	    // dummy pointer
	    *value = (XtPointer)malloc(4);
	    *length = 4;
	    break;

	case Decorator::Text:
	    retVal = TRUE;
	    cp = this->getLabelValue();
	    *length = strlen(cp);
	    *value = (XtPointer)malloc(1+*length);
	    strcpy ((char *)*value, cp);
	    break;

	// The only data needed is hostname:pid.  That string is used to verify that
	// this operation is taking place in intra-executable fashion and not between
	// executables.  This is necessary because placing an interactor in a panel
	// depends on what is in the net.
	case Decorator::Interactors:
	    char * hostname;
	    int len;
	    hostname = new char[MAXHOSTNAMELEN + 16];
	    if (gethostname (hostname, MAXHOSTNAMELEN) == -1) {
		retVal = FALSE;
		break;
	    }
	    len = strlen(hostname);
	    sprintf (&hostname[len], ":%d", getpid());
 
	    *value = hostname;
	    *length = strlen(hostname);
	    retVal = TRUE;
	    break;
 
	default:
	    retVal = FALSE;
	    break;
    }

    return retVal;
}


//
// In addition to the superclass activities, check to see if the text fits in
// the box when going into user style.  If not, the set margins to 0.  When
// returning to developer style, reset margins to a constant defined above.
//
// Toggling XmNresizePolicy on root widget is done because any change to the
// margins might make the label widget recalc its desired size and the parent
// might agree.  Want to prevent that because changing one of vertical,horizontal
// dimensions shouldn't affect the other.
//
void LabelDecorator::setAppearance (boolean developerStyle)
{
int n = 0;
Arg args[5];
Dimension   mw,mh,mt,mb,ml,mr; // margin sizes
Dimension   ht,st; // thicknesses
Dimension   neww, newh;
int	    to,bo,lo,ro;

    this->Decorator::setAppearance (developerStyle);

    if (!this->labelString) return ;
    if (!this->customPart) return ;

    if (developerStyle) {
	XtVaGetValues (this->customPart, 
	    XmNmarginTop, &mt,
	    XmNmarginLeft, &ml,
	    XmNmarginRight, &mr,
	    XmNmarginBottom, &mb,
	NULL);
	if (mt == 0) { XtSetArg (args[n], XmNmarginTop, LLOYDS_SAFETY_MARGIN); n++; }
	if (ml == 0) { XtSetArg (args[n], XmNmarginLeft, LLOYDS_SAFETY_MARGIN); n++; }
	if (mr == 0) { XtSetArg (args[n], XmNmarginRight, LLOYDS_SAFETY_MARGIN); n++; }
	if (mb == 0) { XtSetArg (args[n], XmNmarginBottom, LLOYDS_SAFETY_MARGIN); n++; }
	if (n) {
	    XtVaSetValues (this->getRootWidget(), 
		XmNresizePolicy, XmRESIZE_ANY, 
	    NULL);
	    XtSetValues (this->customPart, args, n);
	}
    } else {
	int required_width, required_height;
	XmFontList xmfl;
	//
	// If in user mode and the string will occupy more room than is available,
	// then as an emergency measure, set XmNmargin{Left,Right} to 0.  They
	// will be set back to their normal value if mode is switched to devel.
	//
	XtVaGetValues (this->customPart, 
	    XmNfontList, &xmfl, 
	    XmNmarginHeight, &mh,
	    XmNmarginWidth, &mw,
	    XmNmarginTop, &mt, 
	    XmNmarginLeft, &ml, 
	    XmNmarginRight, &mr, 
	    XmNmarginBottom, &mb, 
	    XmNshadowThickness, &st, 
	    XmNhighlightThickness, &ht,
	    XmNtopOffset, &to,
	    XmNbottomOffset, &bo,
	    XmNleftOffset, &lo,
	    XmNrightOffset, &ro,
	NULL);
	if ((ml == 0) && (mr == 0)) { /* do nothing */ }
	else {
	    XmStringExtent (xmfl, this->labelString, &neww, &newh);
	    required_width = lo + ro + neww + (2*(mw + ht + st)) + ml + mr;
	    required_height = to + bo + newh + (2*(mh + ht + st)) + mt + mb;
	    Arg args[5];
	    n = 0;
	    if (required_width > this->width) {
		XtSetArg (args[n], XmNmarginLeft, 0); n++;
		XtSetArg (args[n], XmNmarginRight, 0); n++;
	    }
	    if (required_height > this->height) {
		XtSetArg (args[n], XmNmarginTop, 0); n++;
		XtSetArg (args[n], XmNmarginBottom, 0); n++;
	    }
	    if (n) {
		XtVaSetValues (this->getRootWidget(), 
		    XmNresizePolicy, XmRESIZE_NONE,
		NULL);
		XtSetValues (this->customPart, args, n);
	    }
	}
    }
}

void LabelDecorator::associateDialog (Dialog* diag)
{
    SetDecoratorTextDialog* old_diag = this->setTextDialog;
    this->setTextDialog = (SetDecoratorTextDialog*)diag;

    if (old_diag) old_diag->setDecorator(NUL(LabelDecorator*));

    if (this->setTextDialog) this->setTextDialog->setDecorator(this);
}


//
// Used by SetDecoratorTextDialog for the reformatCallback.
//
XmFontList LabelDecorator::getFontList()
{
    if (!this->customPart) return NUL(XmFontList);
    XmFontList xmfl;
    XtVaGetValues (this->customPart, XmNfontList, &xmfl, NULL);
    return xmfl;
}

boolean LabelDecorator::printAsJava (FILE* jf, const char* var_name, int instance_no)
{
    const char* indent = "        ";
    int x,y,w,h;
    this->getXYSize (&w,&h);
    this->getXYPosition (&x,&y);

    const char* labtext = this->getLabel();
    int line_count = Decorator::CountLines(labtext);

    char lvar[128];
    sprintf (lvar, "%s_label_%d", var_name, instance_no);

    fprintf (jf, "\n"); 
    fprintf (jf, "%sLabelGroup %s = new LabelGroup(%d);\n", indent, lvar, line_count);
    int i;
    for (i=1; i<=line_count; i++) {
	const char* cp = Decorator::FetchLine(labtext,i);
	fprintf (jf, "%s%s.setText(\"%s\");\n", indent, lvar, cp);
    }
    if (this->printJavaResources(jf, indent, lvar) == FALSE)
	return FALSE;
    fprintf (jf, "%s%s.init();\n", indent, lvar);
    if (fprintf (jf, "%s%s.setBounds (%d,%d,%d,%d);\n", indent,lvar,x,y,w,h) <= 0)
	return FALSE;
    fprintf (jf, "%s%s.addDecorator(%s);\n", indent, var_name, lvar);


    return TRUE;
}

boolean LabelDecorator::printJavaResources(FILE* jf, const char* indent, const char* var)
{
    if (this->Decorator::printJavaResources(jf, indent, var) == FALSE)
	return FALSE;

    if (this->setResourceList) {
	ListIterator it(*this->setResourceList);
	DynamicResource* dr;
	while ( (dr = (DynamicResource*)it.getNext()) ) {
	    if (EqualString(dr->getResourceName(), XmNalignment))  {
		const char* clr = dr->getStringRepresentation();
		if (!clr) continue;

		if (EqualString(clr, "XmALIGNMENT_BEGINNING")) {
		    fprintf (jf, "%s%s.setAlignment(Label.LEFT);\n", indent, var);
		} else if (EqualString(clr, "XmALIGNMENT_END")) {
		    fprintf (jf, "%s%s.setAlignment(Label.RIGHT);\n", indent, var);
		} else if (EqualString(clr, "XmALIGNMENT_CENTER")) {
		    fprintf (jf, "%s%s.setAlignment(Label.CENTER);\n", indent, var);
		}
	    }
	}
    }

    //
    // These fonts are a notch smaller than the ones in IBMApplication.C
    //
    if ((this->font) && (this->font[0])) {
	char* size_str;
	char* font_str;
	if (strstr (this->font, "small"))
	    size_str = "10";
	else if (strstr (this->font, "big"))
	    size_str = "14";
	else if (strstr (this->font, "huge"))
	    size_str = "18";
	else
	    size_str = "12";
	if (strstr (this->font, "bold"))
	    font_str = "Font.BOLD";
	else if (strstr (this->font, "normal"))
	    font_str = "Font.PLAIN";
	else if (strstr (this->font, "oblique"))
	    font_str = "Font.ITALIC";
	else
	    font_str = "";
	if ((font_str) && (size_str))
	    fprintf (jf, "%s%s.setFont(new Font(\"Helvetica\", %s, %s));\n",
		indent, var, font_str, size_str);
    }

    return TRUE;
}


//
// ...ready for printing in the panel comments -- no '\n' chars
//
// I copied this out of rcs 8.8.  I had erased it when I changed the print
// parse strategy because it was no longer needed.  Now I need it again in
// order to print java.
//
static char *label_buf = NUL(char*);
static int label_max_size = 0;

const char *
LabelDecorator::getLabel()
{
XmStringContext cxt;
char *text, *tag;
XmStringDirection dir;
Boolean sep;
int os;
boolean separator_needed;

    if (!this->labelString) return "";
    if (!XmStringInitContext (&cxt, this->labelString)) {
	XtWarning ("Can't convert compound string.");
	return "";
    }

    int required_space = XmStringLength (this->labelString) +
	(2*XmStringLineCount(this->labelString)) + 1;

    if (required_space >= label_max_size) {
	label_max_size = required_space + 512;
	if (label_buf) delete label_buf;
	label_buf = new char[label_max_size];
    }


    separator_needed = FALSE;
    os = 0;
    while (XmStringGetNextSegment (cxt, &text, &tag, &dir, &sep)) {
	if (separator_needed) {
	    label_buf[os++] = '\\';
	    label_buf[os++] = 'n';
	}
	strcpy (&label_buf[os], text);
	os+= strlen(text);
	XtFree(text);
	// unsure about this line. The motif manual doesn't do this
	// but purify complains about memory loss
	if (tag) XtFree(tag);
	separator_needed = TRUE;
    }
    label_buf[os] = '\0';
    ASSERT(os < label_max_size);
    XmStringFreeContext(cxt);
    return label_buf;
}


