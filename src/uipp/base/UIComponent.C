/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include <defines.h>




#include <X11/Intrinsic.h>
#include <Xm/Xm.h>
#include <Xm/Label.h>
#include <Xm/Text.h>
#include <Xm/TextF.h>
#include <string.h>

#include "UIComponent.h"
#include "Application.h"
#include "WarningDialogManager.h"
#include "WarningDialogManager.h"
#include "DXStrings.h"

#if defined(HAVE_XINERAMA)
#include <X11/extensions/Xinerama.h>
#endif

// SUITS to allow turn-off of stippled drawing on slow devices
#define RES_CONVERT(res, str) XtVaTypedArg, res, XmRString, str, strlen(str)+1

//
// The following are constant understood by setGeometry() and used by
// setXYPosition() and setXYSize() and subclasses.
//
const int UIComponent::UnspecifiedPosition = 32767;
const int UIComponent::UnspecifiedDimension = 32767;

XtIntervalId UIComponent::BubbleTimer = 0;

extern "C" void UIComponent_WidgetDestroyCB(Widget    /* widget */,
					XtPointer  clientData,
					XtPointer /* callData */)
{
    UIComponent* component;

    ASSERT(clientData);

    //
    // Convert the client data into the UIComponent instance pointer it is.
    //
    component = (UIComponent*)clientData;

    //
    // Notify the component that its widget is being destroyed.
    //
    component->widgetDestroyed();
}

extern "C" void UIComponent_ComponentHelpCB(Widget    /* widget */,
					XtPointer  clientData,
					XtPointer /* callData */)
{
    UIComponent* component;

    ASSERT(clientData);

    //
    // Convert the client data into the UIComponent instance pointer it is.
    //
    component = (UIComponent*)clientData;

    //
    // Notify the component that its widget is being destroyed.
    //
    component->componentHelp();
}


UIComponent::UIComponent(const char* name)
{
    unsigned int i;

    ASSERT(name);
    for(i = 0; i < STRLEN(name); i++)
    {
	if( name[i] == ' '  || 
	    name[i] == '\t' || 
	    name[i] == '.'  || 
	    name[i] == '*')
	{
	    WarningMessage("UIComponent questionable name: %s", name);
	}
    }
    this->root   = NUL(Widget);
    this->name   = DuplicateString(name);
    this->active = TRUE;
    this->help_msg = NUL(char*);
    this->inactive_help_msg = NUL(char*);
    this->help_widget = NUL(Widget);
    this->deactivated = FALSE;
    this->xtt = NULL;
    this->fg = ((Pixel)-1);
}


UIComponent::~UIComponent()
{
    //
    // Remove destroy callback so Xt Intrinsics canot call the
    // callback with a pointer to an object that has already
    // been freed.
    //
    if (this->root)
    {
	XtRemoveCallback
	    (this->root,
	     XmNdestroyCallback,
	     (XtCallbackProc)UIComponent_WidgetDestroyCB,
	     (XtPointer)this);
    }

    //
    // Destroy the widget tree, if it still exists;
    // delete the name string.
    //
    if (this->root)
    {
	XtDestroyWidget(this->root);
    }

    if (this->name)
	delete[] this->name;

    if (this->help_msg)
	delete[] this->help_msg;

    if (this->inactive_help_msg)
	delete[] this->inactive_help_msg;
}

void UIComponent::clearRootWidget()
{
    this->root = NULL;
}
void UIComponent::setRootWidget(Widget root, boolean standardDestroy)
{
    if (this->root)
    {
	XtRemoveCallback
	    (this->root,
	     XmNdestroyCallback,
	     (XtCallbackProc)UIComponent_WidgetDestroyCB,
	     (XtPointer)this);
	this->removeComponentHelpCallback();
    }

    this->root = root;
        
    if (standardDestroy) {
	XtAddCallback
	    (this->root,
	     XmNdestroyCallback,
	     (XtCallbackProc)UIComponent_WidgetDestroyCB,
	     (XtPointer)this);
    }
    this->installComponentHelpCallback();
}

void UIComponent::setBubbleHelp (const char *msg, Widget w, boolean active_help)
{
    if (active_help) {
	if (this->help_msg) delete[] this->help_msg;
	this->help_msg = NUL(char*);
	if ((!msg) || (!msg[0])) return ;
	this->help_msg = DuplicateString(msg);
    } else {
	if (this->inactive_help_msg) delete[] this->inactive_help_msg;
	this->inactive_help_msg = NUL(char*);
	if ((!msg) || (!msg[0])) return ;
	this->inactive_help_msg = DuplicateString(msg);
    }

    if (this->help_widget) {
	XtRemoveEventHandler (this->help_widget, EnterWindowMask|LeaveWindowMask,
	    True, (XtEventHandler)UIComponent_BubbleEH, (XtPointer)this);
    }
    this->help_widget = w;
    if (this->help_widget) {
	XtAddEventHandler (this->help_widget, EnterWindowMask|LeaveWindowMask,
	    False, (XtEventHandler)UIComponent_BubbleEH, (XtPointer)this);
    }
}


void UIComponent::widgetDestroyed()
{
    this->root = NUL(Widget);
}


void UIComponent::getResources(const XtPointer      resourceBase,
			       const XtResourceList resourceList,
			       const int            numResources)
{
    Widget parent;

    ASSERT(resourceBase);
    ASSERT(resourceList);
    ASSERT(numResources > 0);

    ASSERT(this->root);
    ASSERT(this->name);

    parent = XtParent(this->root);
    //
    // Use XtGetSubresources() to retrieve the resources
    // for a component with a parent.
    //
    XtGetSubresources
	(parent ? parent : this->root,
	 resourceBase,
	 this->name,
	 this->getClassName(),
	 resourceList,
	 numResources,
	 NUL(ArgList),
	 0);
}


void UIComponent::setDefaultResources(const Widget  widget,
				      const String* resourceSpec)
{
    XrmDatabase resourceDatabase;
    Display*    display;
    int         i;
    char        buffer[4096];
    char	prepend[1024];

    ASSERT(widget);
    ASSERT(resourceSpec);

    display          = XtDisplay(widget);
    resourceDatabase = XrmGetStringDatabase("");

    String name,classname;
    XtGetApplicationNameAndClass(display,&name,&classname);

    // Determine the base name of the application (i.e. ./dx -> dx)
#ifdef	DXD_NON_UNIX_DIR_SEPARATOR
    char *p = this->name;
    for (int j=strlen(this->name)-1; j>=0; j--) {
	if ((this->name[j] == '\\') || (this->name[j] == '/') || (this->name[j] == ':')) {
	    p = &this->name[j+1];
	    break;
	}
    }
#else
    char *p = strrchr(this->name,'/');
    p = ( !p ? this->name : p+1);
#endif

    if (EqualString(name,p)) { 		// Global resources
	// Replace the resource name with the class name when the resources
	// are for the top level component.
	strcpy(prepend,classname);
    } else {
	// Prepend the classname and the component's name 
	sprintf(prepend,"%s*%s",classname,this->name);
    }

    
    ASSERT(STRLEN(prepend) < 1024);

    i = 0;
    while(resourceSpec[i] != NUL(char*))
    {
	sprintf(buffer, "%s%s", prepend, resourceSpec[i++]);
	ASSERT(STRLEN(buffer) < 4096);
	XrmPutLineResource(&resourceDatabase, buffer);
    }

    //
    // Merge the rosources into the Xt database with lowest precedence.
    //
    if (resourceDatabase)
    {
#if defined XlibSpecificationRelease && XlibSpecificationRelease > 4
        XrmDatabase db = XrmGetDatabase(display);
        XrmCombineDatabase(resourceDatabase, &db, False);
#else
	XrmMergeDatabases(display->db, &resourceDatabase);
	display->db = resourceDatabase;
#endif
    }
}


void UIComponent::manage()
{
    ASSERT(this->root);

    //
    // Check to see that a destroy callback has been installed.
    //
    ASSERT(XtHasCallbacks(this->root, XmNdestroyCallback) == XtCallbackHasSome);

    //
    // Manage the root widget.
    //
    XtManageChild(this->root);
}


boolean UIComponent::isManaged()
{
    if (!this->root)
	return FALSE;
    return XtIsManaged(this->root); 
}

void UIComponent::unmanage()
{
    ASSERT(this->root);
    XtUnmanageChild(this->root);
}


void UIComponent::activate()
{
    if (this->root)
    {
	if (!theApplication->bubbleHelpEnabled())
	    XtSetSensitive(this->root, TRUE);
	else {
	    if(deactivated)
	    {
		if(xtt)
		    XtAugmentTranslations(this->root, xtt);
		XtVaSetValues(this->root, XmNforeground, this->fg, NULL);
		deactivated = FALSE;
	    }
	}
    }
    this->active = TRUE;
    if (this->inactive_help_msg) {
	delete[] this->inactive_help_msg;
	this->inactive_help_msg = NUL(char*);
    }
}


void UIComponent::deactivate(const char * inactive_help)
{
    if (this->root)
    {
	if (!theApplication->bubbleHelpEnabled())
	    XtSetSensitive(this->root, FALSE);
	else {
	    if(!deactivated)
	    {
		XtVaGetValues(this->root, XmNtranslations, &xtt, NULL);
		XtVaGetValues(this->root, XmNforeground, &this->fg, NULL);
		XtUninstallTranslations(this->root);
		XtVaSetValues(this->root, 
		    RES_CONVERT(XmNforeground, "gray50"), NULL);
		deactivated = TRUE;
	    }
	}
    }
    this->active = FALSE;
    if ((this->inactive_help_msg) || (inactive_help))
	this->setBubbleHelp (inactive_help, this->help_widget, FALSE);
}

//
// Set the size of this component 
//
void UIComponent::setXYSize(int x, int y)
{
    this->setGeometry(UIComponent::UnspecifiedPosition,
			UIComponent::UnspecifiedPosition,
			x,y);
}
//
// Get the size of this component 
//
void UIComponent::getXYSize(int *x, int *y)
{
    int a, b;
    this->getGeometry(&a, &b, x, y);
}
//
// Get the position of this component 
//
void UIComponent::getXYPosition(int *x, int *y)
{
    int a, b;
    this->getGeometry(x,y, &a, &b);
}
//
// Set the position of this component 
//
void UIComponent::setXYPosition(int x, int y)
{
    this->setGeometry(x,y, UIComponent::UnspecifiedDimension,
                           UIComponent::UnspecifiedDimension);
}

//
// Get the size and dimensions. 
// Return TRUE if all return values are valid.
//
boolean UIComponent::getGeometry(int *x, int *y, int *width, int *height)
{
    Position a,b;
    Dimension w,h;

    ASSERT(this->root);

    XtVaGetValues(this->root,
        XmNx, &a,
        XmNy, &b,
        XmNwidth, &w,
        XmNheight, &h,
        NULL);

   *x = a;
   *y = b;
   *width = w;
   *height = h;

   return TRUE;
}
//
// Set the size and dimensions. 
//
void UIComponent::setGeometry(int x, int y, int width, int height)
{
    ASSERT(this->root);
    Arg wargs[4];
    int     n = 0;

#if 1
    if (x != UIComponent::UnspecifiedPosition)
	{ XtSetArg(wargs[n],XmNx,x); n++; }
    if (y != UIComponent::UnspecifiedPosition)
	{ XtSetArg(wargs[n],XmNy,y); n++; }
    if (width != UIComponent::UnspecifiedDimension)
	{ XtSetArg(wargs[n],XmNwidth,width); n++; }
    if (height != UIComponent::UnspecifiedDimension)
	{ XtSetArg(wargs[n],XmNheight,height); n++; }
    if (n > 0) 
        XtSetValues(this->root,wargs,n);
#else
    XtWidgetGeometry req;

    req.x = x; req.y = y; req.width = width; req.height = height;
    req.request_mode = 0;
    if (x != UIComponent::UnspecifiedPosition)  req.request_mode|= CWX;
    if (y != UIComponent::UnspecifiedPosition)  req.request_mode|= CWY;
    if (width != UIComponent::UnspecifiedDimension)  req.request_mode|= CWWidth;
    if (height != UIComponent::UnspecifiedDimension)  req.request_mode|= CWHeight;
    if (req.request_mode)
	XtMakeGeometryRequest (this->root, &req, NULL);
#endif
}



void UIComponent::installComponentHelpCallback(Widget widget)
{

    if (!widget) 
	widget = this->root;
    ASSERT(widget);

    if (XtHasCallbacks(widget, XmNhelpCallback) != XtCallbackNoList)
    {
	// Ensure that only one copy of the help callback is installed.
	XtRemoveCallback
	    (widget,
	     XmNhelpCallback,
	     (XtCallbackProc)UIComponent_ComponentHelpCB,
	     (XtPointer)this);
	XtAddCallback
	    (widget,
	     XmNhelpCallback,
	     (XtCallbackProc)UIComponent_ComponentHelpCB,
	     (XtPointer)this);
    }
}

void UIComponent::removeComponentHelpCallback(Widget widget)
{

    if (!widget) 
	widget = this->root;
    ASSERT(widget);

    if (XtHasCallbacks(widget, XmNhelpCallback) != XtCallbackNoList)
	XtRemoveCallback
	    (widget,
	     XmNhelpCallback,
	     (XtCallbackProc)UIComponent_ComponentHelpCB,
	     (XtPointer)this);
}


const char *UIComponent::getComponentHelpTopic()
{
    return this->name;
}
void UIComponent::componentHelp()
{
    theApplication->helpOn(this->getComponentHelpTopic());
}
void UIComponent::setLocalData(void *data)
{
    ASSERT(this->root);
    Arg arg[1];
    XtSetArg(arg[0], XmNuserData, (XtPointer)data);
    XtSetValues(this->root, arg,1); 
}

void *UIComponent::getLocalData()
{
    void *data;
    ASSERT(this->root);
    Arg arg[1];
    XtSetArg(arg[0], XmNuserData, (XtPointer)&data);
    XtGetValues(this->root, arg,1); 
    return data;
}

/* 
    ParseGeometryComment actually performs the Window placement and
    Window sizing for shell windows such as the ImageWindow and 
    Control Panels. The network saves the placement as a normalized
    vector of where and what size it was on the authors screen 
    and then tries to replicate that to fit the screen of others. 
    This becomes a real problem when Xinerama comes into play. For example,
    the width/height ratio changes significantly and makes ImageWindows
    very wide when reconstructing them if the Xinerama environment is
    in place. For example DisplayWidth may be 2560 and Height may only
    be 1024. But the original developer was on a system of 1280 x 1024.
    The resulting image doesn't look anything like the authors.

    The fix for placement is to check for Xinerama and replicate on
    the dominate screen. Then this also needs to be fixed in PrintGeometryComment
    as well.
    
    It will not be possible to replicate the multiple screen geometry
    unless the support for Xinerama is turned off. For example: two
    identical screens side by side. If the user builds with the VPE on
    the primary screen, and puts the Image Window on the secondary screen,
    when the Editor is re-opened and executed, the Image Window will
    appear on the primary screen. The only way to fix this would be
    to append the screen number in the GeometryComment
 */

boolean UIComponent::PrintGeometryComment(FILE *f, int xpos, int ypos,
                                int xsize, int ysize, const char *tag,
                                const char *indent)
{
    float norm_xsize, norm_ysize, norm_xpos, norm_ypos;

    if (!tag)
        tag = "window";

    if (!indent)
        indent = "";

	Display *disp = theApplication->getDisplay();
    int width = DisplayWidth(disp,0);
    int height = DisplayHeight(disp,0);
    int x=0, y=0;
    int screen = -1;

 #if defined(HAVE_XINERAMA)
			// Do some Xinerama Magic to use the largest main screen.
			int dummy_a, dummy_b;
			int screens;
			XineramaScreenInfo   *screeninfo = NULL;
	
			if ((XineramaQueryExtension (disp, &dummy_a, &dummy_b)) &&
				(screeninfo = XineramaQueryScreens(disp, &screens))) {
				// Xinerama Detected
		
				if (XineramaIsActive(disp)) {
					width = 0; height = 0;
					int i = dummy_a;
					while ( i < screens ) {
						if(xpos > screeninfo[i].x_org && 
							xpos < screeninfo[i].x_org+screeninfo[i].width &&
							ypos > screeninfo[i].y_org &&
							ypos < screeninfo[i].y_org+screeninfo[i].height ) {
							screen = i;
							width = screeninfo[i].width;
							height = screeninfo[i].height;
							x = screeninfo[i].x_org;
							y = screeninfo[i].y_org;
						}
						i++;
					}
				}
			}
#endif

    norm_xsize = (float) xsize/width;
    norm_ysize = (float) ysize/height;
    norm_xpos  = (float) (xpos - x)/width;
    norm_ypos  = (float) (ypos - y)/height;
    if(screen != -1) {
    	if (fprintf(f,
          "%s// %s: position = (%6.4f,%6.4f), size = %6.4fx%6.4f, screen = %d\n",
                indent, tag, norm_xpos,norm_ypos,norm_xsize,norm_ysize,screen) < 0)
        return FALSE;
    } else {
    	if (fprintf(f,
          "%s// %s: position = (%6.4f,%6.4f), size = %6.4fx%6.4f\n",
                indent, tag, norm_xpos,norm_ypos,norm_xsize,norm_ysize) < 0)
        return FALSE;
    }

    return TRUE;
}
  
boolean UIComponent::ParseGeometryComment(const char *line, const char *file,
                                int lineno, int *xpos, int *ypos,
                                int *xsize, int *ysize, const char *tag)
{
    char format[1024];
    int items;
    float norm_xsize, norm_ysize, norm_xpos, norm_ypos;
    int screen = -1;
    int display_xsize, display_ysize;

    if (!tag)
        tag = "window";

    int taglen = STRLEN(tag);
    sprintf(format," %s: position = (%%f,%%f), size = %%fx%%f, screen = %%d",tag);

    // 15 = strlen(" ") + strlen(": position = (")
    if (!EqualSubstring(line,format,taglen+15))
        return FALSE;

    items = sscanf(line,format,&norm_xpos,&norm_ypos,&norm_xsize,&norm_ysize,&screen);

    if (items == 4 || items == 5) {
        // If the size when printed was UIComponent::UnspecifiedPosition
        // then the normalized value will be greater than 3, so ignore it.
        if ((norm_xsize < 3) && (norm_ysize < 3)) {
 
			int x=0, y=0, width=0, height=0;
 			Display *disp = theApplication->getDisplay();
            width = DisplayWidth(disp,0);
            height = DisplayHeight(disp,0);
	
 #if defined(HAVE_XINERAMA)
			// Do some Xinerama Magic to use the largest main screen.
			int dummy_a, dummy_b;
			int screens;
			XineramaScreenInfo   *screeninfo = NULL;
	
			if ((XineramaQueryExtension (disp, &dummy_a, &dummy_b)) &&
				(screeninfo = XineramaQueryScreens(disp, &screens))) {
				// Xinerama Detected
		//fprintf(stderr, "screens = %d, screen = %d, \nline = %s\n", screens, screen, line);
				if (XineramaIsActive(disp)) {
					width = 0; height = 0;
					int i = dummy_a;
					if(screen != -1 && screen <= screens) {
						width = screeninfo[screen].width;
						height = screeninfo[screen].height;
						x = screeninfo[screen].x_org;
						y = screeninfo[screen].y_org;
					} else
					while ( i < screens ) {
						if(screeninfo[i].width > width) {
							width = screeninfo[i].width;
							height = screeninfo[i].height;
							x = screeninfo[i].x_org;
							y = screeninfo[i].y_org;
						}
						i++;
					}
				}
			}
#endif
            *xpos  = (int) (width * norm_xpos  + .5 + x);
            *ypos  = (int) (height * norm_ypos  + .5 + y);
            *xsize = (int) (width * norm_xsize + .5);
            *ysize = (int) (height * norm_ysize + .5);
        } else {
            *xpos = UIComponent::UnspecifiedPosition;
            *ypos = UIComponent::UnspecifiedPosition;
            *xsize = UIComponent::UnspecifiedDimension;
            *ysize = UIComponent::UnspecifiedDimension;
        }
    } else {
        WarningMessage("Bad comment found in file '%s' line %d (ignoring)",
                                        file,lineno);
    }

    return TRUE;
}

extern "C" void
UIComponent_InstallBubbleTP (XtPointer cData, XtIntervalId *id)
{
    UIComponent *uic = (UIComponent*)cData;
    uic->showBubbleHelp();
    UIComponent::BubbleTimer = 0;
}

extern "C" void 
UIComponent_BubbleEH (Widget w, XtPointer cData, XEvent* xev, Boolean* doit)
{
    *doit = True;
    UIComponent *uic = (UIComponent*)cData;
    if (xev->type == EnterNotify) {
	if (UIComponent::BubbleTimer)
	    XtRemoveTimeOut (UIComponent::BubbleTimer);
	UIComponent::BubbleTimer = 
	    XtAppAddTimeOut (theApplication->getApplicationContext(),
		(unsigned long)100, (XtTimerCallbackProc)UIComponent_InstallBubbleTP,
		(XtPointer)uic);
	//uic->showBubbleHelp();
    } else if (xev->type == LeaveNotify) {
	if (UIComponent::BubbleTimer)
	    XtRemoveTimeOut (UIComponent::BubbleTimer);
	else
	    uic->eraseBubbleHelp();
    }
}

void
UIComponent::showBubbleHelp()
{
    if (!theApplication->bubbleHelpEnabled())  return ;

    Widget help_viewer = theApplication->getHelpViewer();
    if (!help_viewer) return ;

    char *help_msg;
    if ((this->inactive_help_msg) && (!this->active))
	help_msg = this->inactive_help_msg;
    else 
	help_msg = this->help_msg;
    if (!help_msg) return ;

    if (XtIsSubclass(help_viewer, xmLabelWidgetClass)) {
	XmString xmstr = XmStringCreateLtoR (help_msg, "small_normal");
	XtVaSetValues (help_viewer, XmNlabelString, xmstr, NULL);
	XmStringFree(xmstr);
    } else if (XtIsSubclass(help_viewer, xmTextWidgetClass)) {
	XtVaSetValues (help_viewer, XmNvalue, help_msg, NULL);
    } else if (XtIsSubclass(help_viewer, xmTextFieldWidgetClass)) {
	XtVaSetValues (help_viewer, XmNvalue, help_msg, NULL);
    } else ASSERT(0);
}

void
UIComponent::eraseBubbleHelp()
{
    if (!theApplication->bubbleHelpEnabled())  return ;

    Widget help_viewer = theApplication->getHelpViewer();
    if (!help_viewer) return ;

    if (XtIsSubclass(help_viewer, xmLabelWidgetClass)) {
	XmString xmstr = XmStringCreateLtoR ("", "small_normal");
	XtVaSetValues (help_viewer, XmNlabelString, xmstr, NULL);
	XmStringFree(xmstr);
    } else if (XtIsSubclass(help_viewer, xmTextWidgetClass)) {
	XtVaSetValues (help_viewer, XmNvalue, "", NULL);
    } else if (XtIsSubclass(help_viewer, xmTextFieldWidgetClass)) {
	XtVaSetValues (help_viewer, XmNvalue, "", NULL);
    } else ASSERT(0);
}

