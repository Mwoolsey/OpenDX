/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include <defines.h>





#include "DynamicResource.h"
#include <Xm/Xm.h>
#include <Xm/CascadeB.h>
#include <Xm/CascadeBG.h>
#include <Xm/XmStrDefs.h>

//
// What's it do?
//
// Each object is a resource name,value pair and a list of widget names into
// which to make that setting.  Use installed XtTypeConverters in order to 
// accept string representations of resource values.  Use Xt functions to
// ensure that the resource name exists in the widget.  Supply print and parse
// functions in order to transfer things to and from files.
//

//
// Supply the root widget and/or the resourceName or neither.
//
DynamicResource::DynamicResource(const char *resourceName, Widget root)
{

    this->widgetNames = NUL(String *);
    this->nameCnt = 0;

    if ((resourceName)&&(resourceName[0])) {
	this->resourceName = new char [1+strlen(resourceName)];
	strcpy (this->resourceName, resourceName);
    } else
	this->resourceName = NUL(char*);
    this->valid_resource = FALSE;

    this->stringRepresentation = NUL(char *);
    this->storedData = FALSE;
    this->setting.default_addr = 0;

    this->root = NUL(Widget);
    if (root) this->setRootWidget(root);
}

DynamicResource::~DynamicResource()
{
int i;

    if (this->valid_resource) {
	delete this->setting.resource_name;
	delete this->setting.resource_class;
	delete this->setting.resource_type;
	delete this->setting.default_type;

	// FIXME:  The value in default_addr may point to something which
	// should be free-ed or deallocated in a special way i.e XmString.
	// delete this->setting->default_addr;
    }

    if (this->nameCnt) {
	for (i=0; i<this->nameCnt; i++) delete this->widgetNames[i];
	delete this->widgetNames;
    }

    if (this->resourceName) delete this->resourceName;
    if (this->stringRepresentation) delete this->stringRepresentation;
}


//void DynamicResource::initialize()
//{
//    if (!DynamicResource::DynamicResourceClassInitialized) {
//	DynamicResource::DynamicResourceClassInitialized = TRUE;
//    }
//}

//
// The root widget is changeable.  Whenever it is set, run thru the list of
// names, verifying that a widget has a resource and then setting the resource.
//
boolean DynamicResource::setRootWidget(Widget root)
{
int i;

    if (!root) {
	this->root = NUL(Widget);
	return TRUE;
    }

    if (!this->valid_resource) {
	Widget dest;
	boolean no_mistakes = TRUE;
	boolean any_found = FALSE;
	for (i=0; ((i<this->nameCnt) && (no_mistakes)); i++) {
	    if (!(dest = XtNameToWidget (root, this->widgetNames[i]))) 
		no_mistakes = FALSE;
	    else
	    	no_mistakes = DynamicResource::VerifyPresence 
		    (this->resourceName, dest, &this->setting);

	    if (no_mistakes) any_found = TRUE;
	}
	if ((any_found) && (no_mistakes)) this->valid_resource = TRUE;
    }
    
    if (this->root == root) return TRUE;
    this->root = root;

    // It's not bad for valid_resource to be FALSE here.  It just means that we
    // haven't built up the widgetName list enough to have found the resource.
    if (!this->valid_resource) {
	return FALSE;
    }

    boolean applyOK=TRUE;
    for (i=0; ((i<this->nameCnt)&&(applyOK)); i++) 
	applyOK = DynamicResource::applySetting (this->widgetNames[i]); 

    if (!applyOK) {
	return FALSE;
    }
    return TRUE;
}

//
// Produce a call to an installed resource converter.
// Assumes that the data can be held in an XtPointer (an int I suspect).  That will
// include most things.  For settings consisting of multiple bytes use 
// ::setDataDirectly.  It doesn't use a converter.
//
boolean DynamicResource::setData (const char *value)
{
int i;

    if (!this->valid_resource) return FALSE;

    this->storedData = FALSE;

    if (this->stringRepresentation) delete this->stringRepresentation;
    this->stringRepresentation = new char[1+strlen(value)];
    strcpy (this->stringRepresentation, value);

    // FIXME:  The value in default_addr may point to something which
    // should be free-ed or deallocated in a special way i.e XmString.
    // delete this->setting->default_addr;

    this->setting.default_addr = (XtPointer)this->stringRepresentation;

    if (!this->root) return TRUE;

    boolean applyOK = TRUE;
    for (i=0; ((i<this->nameCnt)&&(applyOK)); i++) 
	applyOK = DynamicResource::applySetting (this->widgetNames[i]);

    return applyOK;
}

boolean DynamicResource::setData ()
{
int i;
    if (!this->valid_resource) return FALSE;
    if (!this->storedData) return FALSE;

    boolean applyOK=TRUE;
    for (i=0; ((i<this->nameCnt)&&(applyOK)); i++) 
	applyOK = DynamicResource::applySetting (this->widgetNames[i]);

    return applyOK;
}

//
// Don't attempt to use a resource converter.  This would be for setting a pixmap
// resource if you had already created the pixmap yourself or for setting the
// text inside a text widget.
//
boolean DynamicResource::setDataDirectly (void *value)
{
    this->storedData = TRUE;
    this->stringRepresentation = NUL(char*);
    this->setting.default_addr = (XtPointer)value;
    return this->setData();
}

//
// When adding a widget to the list, is it good to verify that 
// this->setting.resource_name is in the set of resources held by the 
// widget of interest?  The name used will be ...XtName(XtParent(dest)).XtName(dest)
//
// If allIntermediates is set to TRUE, then automatically include all widgets
// which are both children of root and ancestors of dest.  This facilitates
// setting colors resources in objects made of several widgets.
//
boolean DynamicResource::addWidgetToNameList (Widget dest, boolean allAncestors)
{
boolean applyOK=TRUE;

    if (this->inWidgetList (dest)) return TRUE;

    ASSERT(this->resourceName && this->resourceName[0]);
    if (!this->valid_resource)  {
	this->valid_resource = 
	    DynamicResource::VerifyPresence (this->resourceName, dest, &this->setting);
	if (!this->valid_resource) return FALSE;
    }

    ASSERT(this->root);

    char *cp = DynamicResource::QualifiedWidgetName(this->root, dest);
    this->widgetNames = 
	DynamicResource::GrowNameList (this->widgetNames, &this->nameCnt, cp);

    if ((this->storedData) || ((this->root)&&(this->stringRepresentation)))
	applyOK = this->applySetting (cp);

    if ((allAncestors)&&(applyOK)) {
	if (dest != this->root) {
	    applyOK = this->addWidgetToNameList (XtParent(dest), allAncestors);
	}
    }
    delete cp;
    return applyOK;
}

//
// Add dest and all decendants thereof.
// ...used for making a setting once and have it trickle down automatically.
//
boolean DynamicResource::addWidgetsToNameList (Widget dest, boolean )
{
Widget menu, *kids;
int i, nkids;
boolean applyOK=TRUE;

    if (!this->addWidgetToNameList (dest, FALSE)) return FALSE;

    if ((XtIsSubclass (dest, xmCascadeButtonWidgetClass))||
        (XtIsSubclass (dest, xmCascadeButtonGadgetClass))) {
        menu = 0;
        XtVaGetValues (dest, XmNsubMenuId, &menu, NULL);
        if (!menu) return TRUE;
	applyOK = this->addWidgetsToNameList (menu, FALSE);
    }
    if (!XtIsComposite (dest)) return applyOK;

    XtVaGetValues (dest, XmNchildren, &kids, XmNnumChildren, &nkids, NULL);
    for (i=0; ((i<nkids)&&(applyOK)); i++) {
        if (XtIsShell (kids[i])) continue ;
        applyOK = this->addWidgetsToNameList (kids[i], FALSE);
    }
    return applyOK;
}

//
// Make the results of our investigations available.  Memory is owned locally.
//
void *
DynamicResource::getData()
{
    if (!this->valid_resource) return 0 ;
    if (!this->storedData) return 0;

    return (void *)this->setting.default_addr;
}

//
// Form the name w would get if it were added, then check the list of names
// to see if that string is present.
//
boolean
DynamicResource::inWidgetList (Widget w)
{
int i;

    if (!this->valid_resource) return FALSE;

    char *wname = DynamicResource::QualifiedWidgetName (this->root, w);
    for (i=0; i<this->nameCnt; i++) {
	if (!strcmp(this->widgetNames[i], wname)) return TRUE;
    }
    return FALSE;
}

// P R I N T   A N D   P A R S E      P R I N T   A N D   P A R S E      
// P R I N T   A N D   P A R S E      P R I N T   A N D   P A R S E      
// P R I N T   A N D   P A R S E      P R I N T   A N D   P A R S E      
// P R I N T   A N D   P A R S E      P R I N T   A N D   P A R S E      

//
// %s.%s:%s\n, with widget name, resource name, and resource value
// There is 1 resource spec per line.  
//
boolean
DynamicResource::printComment (FILE *f)
{
int i;

    if ((!this->valid_resource) && (this->root != NUL(Widget))) return TRUE;
    if (!this->stringRepresentation) return TRUE;

    for (i=0; i<this->nameCnt; i++) {
	if (fprintf (f, "    // resource %s.%s:%s\n", this->widgetNames[i],
		this->resourceName, this->stringRepresentation) < 0)
	    return FALSE;
    }

    return TRUE;
}

boolean
DynamicResource::parseComment (const char *comment, 
				const char * /* file */, int /* line */)
{
char *full_resource, *resource_value, *rname;
int i;

    full_resource = new char[strlen(comment)];
    resource_value = new char[strlen(comment)];

    if (sscanf (comment, " resource %[^:]:%[^\n]", full_resource, resource_value) != 2) {
	delete full_resource;
	delete resource_value;
        return FALSE;
    }

    // break full_resource into widget name and resource name by pulling the
    // resource name off the end.
    int len = strlen(full_resource);
    rname = new char[1+len];
    int j = 0;
    for (i=len-1; i>=0; i--) {
        if (full_resource[i] == '.') {
	    full_resource[i] = '\0';
	    break;
        }
        rname[j++] = full_resource[i];
    }
    rname[j] = '\0';

    char *widgetName = new char[1+strlen(full_resource)];
    strcpy (widgetName, full_resource);

    resourceName = new char[1+j];
    int k = 0;
    for (i=j-1; i>=0; i--) resourceName[k++] = rname[i];
    resourceName[k] = '\0';

    // Now we have the XmN... into resourceName and the qualified widget name
    // in widgetName.  Add them to this->widgetNames.  
    this->resourceName = resourceName;
    this->widgetNames = 
	DynamicResource::GrowNameList (this->widgetNames, &this->nameCnt, widgetName);

    this->setting.default_addr = this->stringRepresentation = resource_value;
    this->storedData = FALSE;
    this->valid_resource = FALSE;

    delete widgetName;
    delete rname;
    delete full_resource;

    return TRUE;
}

// S T A T I C   U T I L S    S T A T I C   U T I L S    S T A T I C   U T I L S    
// S T A T I C   U T I L S    S T A T I C   U T I L S    S T A T I C   U T I L S    
// S T A T I C   U T I L S    S T A T I C   U T I L S    S T A T I C   U T I L S    
// S T A T I C   U T I L S    S T A T I C   U T I L S    S T A T I C   U T I L S    

//
// Produce the string which represents the name of the widget and parents
// from root on down.  The string will be passed to XtNameToWidget().
// The string will contain at most MAXLEVELS names.
//
#define MAXLEVELS 3
char *
DynamicResource::QualifiedWidgetName (Widget root, Widget dest)
{
char *names_of_dest[MAXLEVELS+1];
int i;

    if (dest == root) {
	char *cp = new char[2];
	cp[0] = '.'; cp[1] = '\0';
	return cp;
    }

    // compute the length of the new widget name.
    Widget w = dest;
    ASSERT(w != root);
    int howmany = 0;
    int totlen = 0;
    char *cp;

    do {
	cp = XtName(w); totlen+= 1+strlen(cp);
	names_of_dest[howmany++] = cp;
	w = XtParent(w);
	ASSERT(w);  // would fail if dest were not a descendant of this->root
    } while ((w != root)&&(howmany<MAXLEVELS)) ;

    ASSERT(totlen);
    char *newname = new char[1+totlen];
    newname[0] = '*';
    int os = 1;
    for (i=howmany-1; i>=0; i--) {
	strcpy (&newname[os], names_of_dest[i]);
	os+= strlen(names_of_dest[i]);

	// don't tack on the separator the last time around
	if (i != 0) {
	    strcpy (&newname[os], "."); os++;
	}
    }
    return newname;
}

//
// Convert a name to a Widget, then insert the resource spec.
//
boolean DynamicResource::applySetting (const char *name)
{
XrmValue toinout, from;
unsigned char 	uchar_space;
int 		int_space;
short 		short_space;
long 		long_space;
Widget		w;

    if (!strcmp(name, ".")) w = this->root;
    else w = XtNameToWidget (this->root, name);
    ASSERT(w);

    if (!this->storedData) {

	ASSERT (from.addr = (XPointer)this->setting.default_addr);
	from.size = 1+strlen((char *)from.addr);

	//
	// To get the data for use in XtSetValues, you have to
	// dereference a pointer.  That requires the appropriate cast.  Noteworthy
	// in its absence are the types float and double.  I'm not sure how to
	// move them around in an ARCH-independent way.  I suspect it's not necessary
	// to use various chunks of memory for this.  One chunk would suffice if it's
	// guarranteed to be large enough and aligned properly.
	//
	toinout.size = this->setting.resource_size;
	if (this->setting.resource_size == sizeof(unsigned char)) {
	    toinout.addr = (XPointer)&uchar_space;
	} else if (this->setting.resource_size == sizeof(short)) {
	    toinout.addr = (XPointer)&short_space;
	} else if (this->setting.resource_size == sizeof(int)) {
	    toinout.addr = (XPointer)&int_space;
	} else if (this->setting.resource_size == sizeof(long)) {
	    toinout.addr = (XPointer)&long_space;
	} else
	    ASSERT(0);

	// This would fail if there were no installed type converter.
	// It also fails for unrecognized data for some resources.
	Boolean resOK =
	    XtConvertAndStore(w, XmRString, &from, this->setting.resource_type, &toinout);
	if (resOK) {

	    if (this->setting.resource_size == sizeof(unsigned char)) {
		this->setting.default_addr = (XtPointer)*((unsigned char *)toinout.addr);
	    } else if (this->setting.resource_size == sizeof(short)) {
		this->setting.default_addr = (XtPointer)*((short *)        toinout.addr);
	    } else if (this->setting.resource_size == sizeof(int)) {
		this->setting.default_addr = (XtPointer)*((int *)          toinout.addr);
	    } else if (this->setting.resource_size == sizeof(long)) {
		this->setting.default_addr = (XtPointer)*((long *)         toinout.addr);
	    } 

	    this->storedData = TRUE;
	} else {
	    if (this->stringRepresentation) delete this->stringRepresentation;
	    this->setting.default_addr = 0;
	    this->stringRepresentation = NULL;
	    return FALSE;
	}
    } 
    XtVaSetValues (w, this->setting.resource_name, this->setting.default_addr, NULL);
    return TRUE;
}


//
// TRUE if resourceName names a resource held by Widget w.  The XtResource from that
// widget is copied into *setting.  (Assumes XmNlabelString means the same thing
// for anyone who has one.  I believe that's safe.)
//
// It would be good to check for the types we can handle, but
// when the XtConvertAndStore happens we'll check the return value.  If this stuff
// were exposed to users, then you would have to handle errors, but I suspect that
// will never happen.
// This code should be able to handle any type which editres can.
//
boolean
DynamicResource::VerifyPresence (const char *resourceName, Widget w, XtResource *setting)
{
XtResourceList reslist;
Cardinal i, rescnt;

    // Not an error because we want to go thru the constructor OK even
    // if the resource name is not yet know.  This is because of printing, parsing.
    if ((!resourceName) || (!resourceName[0])) return FALSE;

    XtGetResourceList (XtClass(w), &reslist, &rescnt);

    for (i=0; i<rescnt; i++) {
	if (!strcmp(resourceName, reslist[i].resource_name)) {
	    DynamicResource::CopyResource (&reslist[i], setting);
	    XtFree((char *)reslist);
	    return TRUE;
	}
    }
    if (reslist) XtFree((char *)reslist);
    XtGetConstraintResourceList (XtClass(XtParent(w)), &reslist, &rescnt);
    for (i=0; i<rescnt; i++) {
        if (!strcmp(resourceName, reslist[i].resource_name)) {
	    DynamicResource::CopyResource (&reslist[i], setting);
	    XtFree((char *)reslist);
	    return TRUE;
        }
    }
    if (reslist) XtFree((char *)reslist);

    return FALSE;
}


void DynamicResource::CopyResource (XtResource *src, XtResource *dest)
{
    dest->resource_name = new char[1+strlen(src->resource_name)];
    strcpy (dest->resource_name, src->resource_name);
    dest->resource_class = new char[1+strlen(src->resource_class)];
    strcpy (dest->resource_class, src->resource_class);
    dest->resource_type = new char[1+strlen(src->resource_type)];
    strcpy (dest->resource_type, src->resource_type);
    dest->default_type = new char[1+strlen(src->default_type)];
    strcpy (dest->default_type, src->default_type);
    dest->resource_size = src->resource_size;
    dest->resource_offset = src->resource_offset;
}

String *
DynamicResource::GrowNameList (String *widgetNames, int *n, const char *newname)
{
int nameCnt = *n;
String *newlist;
int i;

    // increase the size of this->widgetNames
    newlist = new String[nameCnt + 1];
    for (i=0; i<nameCnt; i++) {
	newlist[i] = widgetNames[i];
    }

    newlist[i] = new char[1+strlen(newname)];
    strcpy (newlist[i], newname);

    nameCnt++;
    if (widgetNames) delete widgetNames;

    *n = nameCnt;
    return newlist;
}
