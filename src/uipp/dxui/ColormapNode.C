/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"



#ifdef ABS_IN_MATH_H
# define abs __Dont_define_abs
#endif
#include <math.h>
#ifdef ABS_IN_MATH_H
# undef abs
#endif

#if HAS_RENAME
# ifdef sun4
#  include <unistd.h>   // For rename()
# else
#  include <stdio.h>    // For rename()
# endif
#endif  // HAS_RENAME


#include "lex.h"
#include "DXStrings.h"
#include "Parameter.h"
#include "DXApplication.h"
#include "ColormapNode.h"
#include "ColormapEditor.h"
#include "Command.h"
#include "Network.h"
#include "AttributeParameter.h"
#include "WarningDialogManager.h"
#include "ErrorDialogManager.h"
#include "DeferrableAction.h"

#include "../widgets/XmDX.h"

#define HMAP_PARAM_NUM 		1		// Private, used to store map
#define OLDHSVDATA_PARAM_NUM 	1	// Private, used to store map
#define SMAP_PARAM_NUM   	2	// Private, used to store map
#define OLDHSV_PARAM_NUM   	2	// Private, used to store map
#define VMAP_PARAM_NUM 		3	// Private, used to store map
#define OLDOPDATA_PARAM_NUM 	3	// Private, used to store map
#define OMAP_PARAM_NUM  	4	// Private, used to store map
#define OLDOP_PARAM_NUM  	4	// Private, used to store map
#define ID_PARAM_NUM		5	// Private id
#define DATA_PARAM_NUM		6
#define MIN_PARAM_NUM		7	// User set min
#define MAX_PARAM_NUM		8	// User set max
#define NHIST_PARAM_NUM 	9	// Number of bins in histogram 
#define CMAP_PARAM_NUM		10	// Colormap object
#define OPMAP_PARAM_NUM		11	// Opacity map object
#define ATTR_PARAM_NUM		12	// scalar list of min and max.
#define DEFHMAP_PARAM_NUM	13	// Stores default colormaps	
#define DEFSMAP_PARAM_NUM	14	//	"
#define DEFVMAP_PARAM_NUM	15	//	"
#define DEFOMAP_PARAM_NUM	16	//	"
#define DEFMIN_PARAM_NUM	17	// Stores default min
#define DEFMAX_PARAM_NUM	18	// Stores default max
#define LABEL_PARAM_NUM		19	

#define EXPECTED_INPUTS		LABEL_PARAM_NUM


ColormapNode::ColormapNode(NodeDefinition *nd, 
				Network *net, int instnc) :
    DrivenNode(nd, net, instnc)
{
    this->xpos = this->ypos = UIComponent::UnspecifiedPosition; 
    this->width = this->height = UIComponent::UnspecifiedDimension;
    this->title = NULL;
    this->netSavedCMFilename = NULL;
    this->histogram = NULL;
    this->colormapEditor = NUL(ColormapEditor*);
    this->updateMinMaxAttr = new DeferrableAction(UpdateAttrParam,
						(void *)this);
}

ColormapNode::~ColormapNode()
{
    if (this->title)
	delete title;
    if (this->netSavedCMFilename)
	delete netSavedCMFilename;
    if (this->histogram)
	delete histogram;

    if (this->colormapEditor) {
	delete this->colormapEditor;
	this->colormapEditor = NULL;
    }

    delete this->updateMinMaxAttr;
}

boolean ColormapNode::initialize()
{
    if (this->getInputCount() != EXPECTED_INPUTS) {
        fprintf(stderr, 
           "Expected %d inputs for %s node, please check the mdf file.\n",
                        EXPECTED_INPUTS,
                        this->getNameString());
	return FALSE;
    }

    this->deferMinMaxUpdates();

    this->setMessageIdParameter(ID_PARAM_NUM);
    this->initMinimumValue(  "0.0");
    this->initMaximumValue("100.0");
    this->initNBinsValue("20");

    this->installTHEDefaultMaps(FALSE); 

    this->undeferMinMaxUpdates();

    return TRUE;

}
boolean ColormapNode::hasDefaultHSVMap()
{
    return !EqualString(this->getInputValueString(DEFHMAP_PARAM_NUM),"NULL") &&
	   !EqualString(this->getInputValueString(DEFSMAP_PARAM_NUM),"NULL") &&
	   !EqualString(this->getInputValueString(DEFVMAP_PARAM_NUM),"NULL");
}
//
// Install the currently defined default hsv colormap and send it 
// to the executive if requested.
//
void ColormapNode::installCurrentDefaultHSVMap(boolean send)
{
    const char *hmap = this->getInputValueString(DEFHMAP_PARAM_NUM);
    const char *smap = this->getInputValueString(DEFSMAP_PARAM_NUM);
    const char *vmap = this->getInputValueString(DEFVMAP_PARAM_NUM);


    if (EqualString(hmap,"NULL") || 
	EqualString(smap,"NULL") || 
	EqualString(vmap,"NULL")) {
	ErrorMessage("Default color map not defined");
    } else {
	//
	// Keep every setInputValue*() from updating the editor
	//
	this->deferVisualNotification();

	this->setInputValue(HMAP_PARAM_NUM,hmap,DXType::UndefinedType,FALSE);
	this->setInputValue(SMAP_PARAM_NUM,smap,DXType::UndefinedType,FALSE);
	this->setInputValue(VMAP_PARAM_NUM,vmap,DXType::UndefinedType,FALSE);

	this->undeferVisualNotification();

	if (send)
	    this->sendValues(FALSE);

    }

}
boolean ColormapNode::hasDefaultOpacityMap()
{
    return !EqualString(this->getInputValueString(DEFOMAP_PARAM_NUM),"NULL");
}
//
// Install the currently defined default opacity colormap and send it 
// to the executive if requested.
//
void ColormapNode::installCurrentDefaultOpacityMap(boolean send)
{
    const char *opmap = this->getInputValueString(DEFOMAP_PARAM_NUM);

    if (EqualString(opmap,"NULL")) { 
	ErrorMessage("Default opacity map not defined");
    } else {
	//
	// Keep every setInputValue*() from updating the editor
	//
	this->deferVisualNotification();
	this->setInputValue(OMAP_PARAM_NUM,opmap,DXType::UndefinedType,send);
	this->undeferVisualNotification();
	
    }
}
boolean ColormapNode::hasDefaultMin()
{
    return !EqualString(this->getInputValueString(DEFMIN_PARAM_NUM),"NULL"); 
}
boolean ColormapNode::hasDefaultMax()
{
    return !EqualString(this->getInputValueString(DEFMAX_PARAM_NUM),"NULL"); 
}
void ColormapNode::installCurrentDefaultMin(boolean send)
{
    this->installCurrentDefaultMinOrMax(TRUE, send);
}
void ColormapNode::installCurrentDefaultMax(boolean send)
{
    this->installCurrentDefaultMinOrMax(FALSE, send);
}
//
// Install the currently defined default min or max and send it 
// to the executive if requested.
//
void ColormapNode::installCurrentDefaultMinOrMax(boolean min, boolean send)
{
    const char *name;
    int pnum;

    if (min) {
	pnum = DEFMIN_PARAM_NUM;
	name = "minimum";
    } else {
	pnum = DEFMAX_PARAM_NUM;
	name = "maximum";
    }

    const char *s = this->getInputValueString(pnum);
    if (EqualString(s,"NULL")) 
	ErrorMessage("Default %s not defined",name);
    else {
	//
	// Keep every setInputValue*() from updating the editor
	//
	this->deferVisualNotification();

        this->deferMinMaxUpdates();
	if (min)
	    this->setMinimumValue(atof(s));
	else
	    this->setMaximumValue(atof(s));
	this->undeferMinMaxUpdates();

	if (send)
	    this->sendValues(FALSE);
	//
	// Setting the above inputs may/does not result in a  
	// a notifyVisuals to take place in 
	// DrivenNode::ioParameterStatusChange().  So lets make sure.
	//
	this->notifyVisualsOfStateChange();

	this->undeferVisualNotification();
    }
}
//
// Install the currently defined default min and max and send it 
// to the executive if requested.
//
void ColormapNode::installCurrentDefaultMinAndMax(boolean send)
{
    //
    // Keep every setInputValue*() from updating the editor
    //
    this->deferVisualNotification();

    this->deferMinMaxUpdates();
    this->installCurrentDefaultMin(FALSE);
    this->installCurrentDefaultMax(FALSE);
    this->undeferMinMaxUpdates();

    if (send)
	this->sendValues(FALSE);
    //
    // Setting the above inputs may/does not result in a  
    // a notifyVisuals to take place in DrivenNode::ioParameterStatusChange().
    // So lets make sure.
    //
    this->notifyVisualsOfStateChange();

    this->undeferVisualNotification();
}
//
// Install the THE default h/s/v/opacity colormap and send it to the executive
// if requested.
//
void ColormapNode::installTHEDefaultMaps(boolean send)
{
    int count = 2; 

#if defined(hp700) || defined(aviion)
    double hue[2];   hue[0] = 0.666667;   hue[1] =   0.0;
    double sat[2];   sat[0] = 1.0;  	  sat[1] =   1.0;
    double val[2];   val[0] = 1.0;  	  val[1] =   1.0;
    double op[2];     op[0] = 1.0;   	   op[1] =   1.0;
    double data[2]; data[0] = 0.0; 	 data[1] =   1.0;
#else
    double hue[] = {  .666667,  0.0 };
    double sat[] = { 1.0, 	1.0 };
    double val[] = { 1.0, 	1.0 };
    double op[]  = { 1.0, 	1.0 };
    double data[]= { 0.0,       1.0 };
#endif

    //
    // Keep every setInputValue*() from updating the editor
    //
    this->deferVisualNotification();

    this->deferMinMaxUpdates();
    this->setMinimumValue(0.0);
    this->setMaximumValue(100.0);
    this->undeferMinMaxUpdates();

    this->installNewMaps(count, data, hue, 
			count, data, sat, 
			count, data, val, 
			count, data, op,
			send);

    this->undeferVisualNotification();
}

//
// G/Set the 'minimum' attribute for the given component
//
boolean ColormapNode::initNBinsValue(const char *val)
{
    AttributeParameter *p = (AttributeParameter*) this->getInputParameter(
                                                NHIST_PARAM_NUM);
    return p->initAttributeValue(val);
}
boolean ColormapNode::setNBinsValue(double nbins)
{
    char value[256];
    AttributeParameter *p = (AttributeParameter*) this->getInputParameter(
                                                NHIST_PARAM_NUM);
    sprintf(value, "%d", (int)nbins);
    this->setInputValue(NHIST_PARAM_NUM, value);
    return p->setAttributeComponentValue(1, nbins);
}
double ColormapNode::getNBinsValue()
{
    AttributeParameter *p = (AttributeParameter*) this->getInputParameter(
                                                NHIST_PARAM_NUM);
    return p->getAttributeComponentValue(1);
}
//
// G/Set the 'minimum' attribute for the given component
//
boolean ColormapNode::initMinimumValue(const char *val)
{
    AttributeParameter *p = (AttributeParameter*) this->getInputParameter(
                                                MIN_PARAM_NUM);
    boolean r = p->initAttributeValue(val);
    return r;
}
boolean ColormapNode::setMinimumValue(double min)
{
    AttributeParameter *p = (AttributeParameter*) this->getInputParameter(
                                                MIN_PARAM_NUM);
    boolean r = p->setAttributeComponentValue(1, min);
    return r;
}
double ColormapNode::getMinimumValue()
{
    AttributeParameter *p = (AttributeParameter*) this->getInputParameter(
                                                MIN_PARAM_NUM);
    return p->getAttributeComponentValue(1);
}
//
// G/Set the 'maximum' attribute for the given component
//
boolean ColormapNode::initMaximumValue(const char *val)
{
    AttributeParameter *p = (AttributeParameter*) this->getInputParameter(
                                                MAX_PARAM_NUM);
    boolean r = p->initAttributeValue(val);
    return r;
}
boolean ColormapNode::setMaximumValue(double max)
{
    AttributeParameter *p = (AttributeParameter*) this->getInputParameter(
                                                MAX_PARAM_NUM);
    boolean r = p->setAttributeComponentValue(1, max);
    return r;
}
double ColormapNode::getMaximumValue()
{
    AttributeParameter *p = (AttributeParameter*) this->getInputParameter(
                                                MAX_PARAM_NUM);
    return p->getAttributeComponentValue(1);
}

void
ColormapNode::openDefaultWindow(Widget)
{
    if (!this->colormapEditor)
    {
	this->colormapEditor = new ColormapEditor(this);
	if ((this->xpos != UIComponent::UnspecifiedPosition)  &&
            theDXApplication->applyWindowPlacements())
	    this->colormapEditor->setGeometry(this->xpos,this->ypos,
						this->width,this->height);
	this->colormapEditor->initialize();
    }

    this->colormapEditor->manage();
}

const char *
ColormapNode::getTitle()
{
    if (this->title)
	return this->title;

    const char *t1 = this->getInputSetValueString(LABEL_PARAM_NUM);

    if (!t1)
	return NULL;

    char *t2 = FindDelimitedString(t1,'"','"');
    
    if (!t2)
	return NULL;

    int t2len = STRLEN(t2);
    t2[t2len-1] = '\0';	// remove trailing double quote
    if (STRLEN(t2+1) > 0)	// original t2 != ""
	this->title = DuplicateString(t2+1);

    delete t2;
    return this->title;
}


void ColormapNode::setTitle(const char *title, boolean fromServer)
{
    if (this->title) {
	delete this->title;
	this->title = NULL;
    }
    if (!title)
	title = "";

    if (fromServer) 
	this->setInputAttributeFromServer(LABEL_PARAM_NUM,
			title,DXType::StringType);
    else
	this->setInputSetValue(LABEL_PARAM_NUM,title);

    ColormapEditor *editor = this->colormapEditor;
    if (editor)
	editor->setWindowTitle(title, TRUE);
}

boolean ColormapNode::isTitleVisuallyWriteable()
{
    return this->isInputDefaulting(LABEL_PARAM_NUM);
}

boolean 
ColormapNode::netParseComment(const char* comment,
                		   const char* filename, int lineno)
{
    if (this->DrivenNode::netParseComment(comment, filename, lineno)) 
    {
	int major = this->getNetwork()->getNetMajorVersion();
	if (major < 2) {
	    //
	    // The version 2 networks, have a Colormap that set the min/max 
 	    // parameters, but previous .net files did not have a min/max 
	    // parameter, so we must set the min/max parameters from the 
	    // first/last elements of the opacity data value list (input #3).  
	    // NOTE: we could also use the hsv data value list, but typically
	    //	     the opacity value list is shorter (no control points) so
	    //	     it will parse more quickly if we use the opacity data.
	    //
	    if (EqualSubstring(comment," input[3]",9)) {
		const char *vlist = this->getInputValueString(3);
		char buf[128], last[128];
		int	index = -1, first = TRUE;
		this->deferMinMaxUpdates();
		while (DXValue::NextListItem(vlist,&index,
						DXType::ScalarListType,buf,128)) {
		    if (first) {
			first = FALSE;
			this->setMinimumValue(atof(buf));
		    } else {
			strcpy(last,buf);
		    }
		}
		this->setMaximumValue(atof(last));
		this->undeferMinMaxUpdates();
	    }
	} else if ((major > 2) || 
		   (this->getNetwork()->getNetMinorVersion() > 0)) {
	    //
	    // For all networks after 2.0.x, which do not have a 'pathname ='
	    // auxiliary comment, we must do the setting of  the min/max that
	    // is normally done when parsing that comment.
	    //
	    if (EqualSubstring(comment," input[8]",9)) {
		//
		// This is the last comment, so lets do any clean ups here.
		// This means that we must sync the min/max attribute values 
		// with the parameter values since they are not synced 
		// automatically.
		this->deferMinMaxUpdates();
#if 00

		const char *s = this->getInputSetValueString(MIN_PARAM_NUM);
		this->setMinimumValue(atof(s));

		s = this->getInputSetValueString(MAX_PARAM_NUM);
		this->setMaximumValue(atof(s));
#else
	        this->syncAttributeParameters();
#endif

		this->undeferMinMaxUpdates();
	   } 
	}
	return TRUE;
    } else {
	return FALSE;
    }
}


boolean ColormapNode::netPrintAuxComment(FILE *f)
{
    if (!this->DrivenNode::netPrintAuxComment(f))
        return FALSE;

    if (!this->printCommonComments(f,"    "))
        return FALSE;

    return TRUE;
}
boolean 
ColormapNode::netParseAuxComment(const char* comment,
                		   const char* filename, int lineno)
{
#define PATH_COMMENT_LEN 22	// The length of " Colormap: pathname = "
    if (EqualSubstring(" colormap: pathname = ",comment,PATH_COMMENT_LEN) 
		||
             EqualSubstring(" Colormap: pathname = ",comment,PATH_COMMENT_LEN))
    {
  	//
  	// This is the last comment, so lets do any clean ups here.
  	// This means that we must sync the min/max attribute values with
  	// the parameter values since they are not synced automatically.
  	//
	this->deferMinMaxUpdates();
	const char *s = this->getInputSetValueString(MIN_PARAM_NUM);
	this->setMinimumValue(atof(s));

	s = this->getInputSetValueString(MAX_PARAM_NUM);
	this->setMaximumValue(atof(s));
	this->undeferMinMaxUpdates();

	int major = this->getNetwork()->getNetMajorVersion();
	int minor = this->getNetwork()->getNetMinorVersion();
	if (((major == 2) && (minor == 0)) || (major == 1))
	{
	    //
	    // After version 2.0.2 (next was 2.1) .net files the first four 
	    // inputs (unviewable) changed meaning.  They used to contain
	    // 		#1) hsvdata 	(scalar list) 
	    //		#2) hsv 	(3-vector list)
	    // 		#3) opdata	(scalar list) 
	    // 		#4) op		(scalar list) 
	    // With hsvdata in 1:1 correspondence with hsv and the same for
	    // opdata and op. 
	    // After version 2.0.2, we change the inputs as follows
	    // 		#1) huepts	(2-vector list) 
	    // 		#2) satpts	(2-vector list) 
	    // 		#3) valepts	(2-vector list) 
	    // 		#4) oppts	(2-vector list) 
	    // Each vector is a 2-vector representing the position of a control
	    // point in the color map where the first component is the data
	    // value and the 2nd is the h/s/v/opacity value.
	    // When reading the old nets we must try and convert these 
	    // old input values to the new ones.  We first try and read the
	    // existing .cm file and generate new inputs from the data containe
	    // there.  If the .cm is not found then generate the new inputs
	    // (control points) from the current inputs.  The latter method
	    // probably results in more control points than were in the .cm
	    // file, but this is better than going to the default map.
	    //
	    const char *netname = this->getNetwork()->getFileName();
	    char *temp = GetDirname(netname);
	    char *path = GetFullFilePath(temp);
	    char *cmfile = 
		    new char[STRLEN(&comment[PATH_COMMENT_LEN])+STRLEN(path)+8];
	    sprintf(cmfile , "%s/%s", path, &comment[PATH_COMMENT_LEN]);
	    delete temp;
	    delete path;
	    this->netSavedCMFilename = cmfile; 
	    this->convertOldInputs();
	}

	return TRUE;
    }
    else if (this->parseCommonComments(comment,filename,lineno))
        return TRUE;

    return FALSE;
}


#define SEND_NOW	0
#define NO_SEND		1
#define SEND_QUIET	2
//
// Install the given colormap and opacity map (control points) into the node.
// *_data are the data values (should be within min and max) that
// map onto the hue/sat/val or opacity maps in a 1 to 1 correspondence.
// hsv_count is the number of items in hsv_data/hue/sat/val and
// op_count is the number of items in op_data/opacity.
// Currently, hsv_count and op_count should be greater than 0
// for a change to take place to the respective maps.
// send_how is one of
//	1) SEND_NOW	; sends the value to the executive;
//	2) NO_SEND	; don't send the values at all
//	3) SEND_QUIET	; don't send the values without causing an execution 
//
//
void ColormapNode::installMaps(
		int h_count,  double  *h_level, double  *h_value,
		int s_count,  double  *s_level, double  *s_value,
		int v_count,  double  *v_level, double  *v_value,
		int op_count, double *op_level, double *op_value,
		boolean send_how)
{
    int      length;
    char*    value;

    int maxcount = h_count;
    if (s_count > maxcount) maxcount = s_count;
    if (v_count > maxcount) maxcount = v_count;
    if (op_count > maxcount) maxcount = op_count;
    
    //
    // Build the scalar list of hsv data values.
    //
    length = maxcount * 32 * 2;
    value = new char[length];
    ASSERT(value);

    //
    // Keep every setInputValue*() from updating the editor
    //
    this->deferVisualNotification();

    /*
     * Assign the value to the hue map parameter.
     */
    this->makeControlPointList(h_count, h_level, h_value, value);
    if (send_how == SEND_QUIET)
    	this->setInputValueQuietly(HMAP_PARAM_NUM, value, 
				DXType::UndefinedType);
    else
    	this->setInputValue(HMAP_PARAM_NUM, value, 
				DXType::UndefinedType, FALSE);
   
   /*
    * Assign the value to the saturation map parameter.
    */
    this->makeControlPointList(s_count, s_level, s_value, value);
    if (send_how == SEND_QUIET)
    	this->setInputValueQuietly(SMAP_PARAM_NUM, value, 
						DXType::UndefinedType);
    else
	this->setInputValue(SMAP_PARAM_NUM, value, 
		    		DXType::UndefinedType, FALSE);
   /*
    * Assign the value to the value map parameter.
    */
    this->makeControlPointList(v_count, v_level, v_value, value);
    if (send_how == SEND_QUIET)
    	this->setInputValueQuietly(VMAP_PARAM_NUM, value, 
						DXType::UndefinedType);
    else
	this->setInputValue(VMAP_PARAM_NUM, value, 
		    		DXType::UndefinedType, FALSE);

    /*
     * Assign the value to the correct parameter.
     */
    this->makeControlPointList(op_count, op_level, op_value, value);
    if (send_how == SEND_QUIET)
	this->setInputValueQuietly(OMAP_PARAM_NUM, value, 
						DXType::UndefinedType);
    else
	this->setInputValue(OMAP_PARAM_NUM, value, 
				DXType::UndefinedType, FALSE);

    delete value;

    if (send_how == SEND_NOW)
	this->sendValues(FALSE);

    this->undeferVisualNotification();
    
}
//
// Install the new control points, and update the executive quietly (i.e.
// with no executions when in execute-on-change mode).
//
void ColormapNode::installNewMapsQuietly(
		int h_count,  double  *h_level, double  *h_value,
		int s_count,  double  *s_level, double  *s_value,
		int v_count,  double  *v_level, double  *v_value,
		int op_count, double *op_level, double *op_value)
{
    this->installMaps(
		h_count,  h_level,  h_value,
		s_count,  s_level,  s_value,
		v_count,  v_level,  v_value,
		op_count, op_level, op_value,
		SEND_QUIET);
}
//
// Install the new control points, and if requested, send the values
// the to executive.
//
void ColormapNode::installNewMaps(
		int h_count,  double  *h_level, double  *h_value,
		int s_count,  double  *s_level, double  *s_value,
		int v_count,  double  *v_level, double  *v_value,
		int op_count, double *op_level, double *op_value,
		boolean send)
{
    this->installMaps(
		h_count,  h_level,  h_value,
		s_count,  s_level,  s_value,
		v_count,  v_level,  v_value,
		op_count, op_level, op_value,
		send ? SEND_NOW : NO_SEND);
}

// The messages we parse can contain one or more of the following...
//
// 'min=%g' 'max=%g' 'histogram={ %g ... }' 
//
// If any input or output values are to be changed, don't send them
// because the module backing up the interactor will have just executed
// and if the UI is in 'execute on change' mode then there will be an
// extra execution.
//
// Returns the number of attributes parsed.
// Handles messages with 
//   'min=%g' 'max=%g' 'histogram={%d %d ... }' or 'histogram=NULL'
// Returns the number of the above found in the given message.
//
int ColormapNode::handleNodeMsgInfo(const char *line)
{
    int index, values = 0;
    char *p, *buf = NULL;

    this->deferMinMaxUpdates();

    //
    // Handle the 'title=%s\n' part of the message.
    //
    if ( (p = strstr((char*)line,"title=")) ) {
        while (*p != '=') p++;
        p++;
	if (EqualString(p,"NULL"))
	    p = NULL;
	this->setTitle(p,TRUE);
    }

    //
    // Handle the 'min=%g' part of the message.
    //
    if ( (p = strstr((char*)line,"min=")) ) {
        while (*p != '=') p++;
        p++;
	if (EqualSubstring(p,"NULL",4)) {
	    values++;
	    this->setInputValueQuietly(DEFMIN_PARAM_NUM,"NULL",
					DXType::UndefinedType);
	} else {
	    buf = DuplicateString(p);
	    index = 0;
	    if (IsScalar(buf, index)) {
		/*update_attr = TRUE;*/
		values++;
		buf[index] = '\0';
		this->setInputAttributeFromServer(MIN_PARAM_NUM,buf,
						    DXType::UndefinedType);
		this->setInputValueQuietly(DEFMIN_PARAM_NUM,buf,
						DXType::UndefinedType);
	    }
	    delete buf;
        }
    }
    //
    // Handle the 'max=%g' part of the message.
    //
    if ( (p = strstr((char*)line,"max=")) ) {
        while (*p != '=') p++;
        p++;
	if (EqualSubstring(p,"NULL",4)) {
	    values++;
	    this->setInputValueQuietly(DEFMAX_PARAM_NUM,"NULL",
					DXType::UndefinedType);
	} else {
	    buf = DuplicateString(p);
	    index = 0;
	    if (IsScalar(buf, index)) {
		/*update_attr = TRUE;*/
		values++;
		buf[index] = '\0';
		this->setInputAttributeFromServer(MAX_PARAM_NUM,buf,
						    DXType::UndefinedType);
		this->setInputValueQuietly(DEFMAX_PARAM_NUM,buf,
						    DXType::UndefinedType);
	    }
	    delete buf;
	}
    }
    //
    // Handle the 'histogram={%d %d ... }' or 'histogram=NULL' part of the 
    // message.
    // Backwards compatibility for versions 2.0.0 -> 2.0.2.
    //
    if ( (p = strstr((char*)line,"histogram=")) ) {
	values++;
        while (*p != '=') p++;
        p++;
	if (this->histogram) 
	    delete this->histogram;
	if (EqualSubstring(p,"NULL",4)) {
	    this->histogram = NULL;
	} else {
	    this->histogram = DuplicateString(p);
	}
    }

    //
    // Handle the '*map={%d %d ... }' part of the message.
    // If we get a NULL value for a map, that means the default map is
    // not available so don't install it in the current map (i.e. one of
    // the first four inputs), otherwise do install it.
    //
    //
    if ( (p = strstr((char*)line,"map=")) ) {
        char *buf = new char [STRLEN(p)];
   	/*update_attr = FALSE;*/
	do {
	    boolean null_default;
	    int i1, i2;
	    if (EqualSubstring(p+4,"NULL",4)) {
		null_default = TRUE; 
		strcpy(buf,"NULL");
	    } else if (FindDelimitedString(p+4,'{','}', buf))
		null_default = FALSE; 
	    else
		continue;	// I don't expect to get here
	    switch (*(p-1)) { 		// Get character just before 'map='
		case 'e':		// "huemap=..."
		    i1 = HMAP_PARAM_NUM; 
		    i2 = DEFHMAP_PARAM_NUM;
		    break;
		case 't':		// "satmap=..."
		    i1 = SMAP_PARAM_NUM; 
		    i2 = DEFSMAP_PARAM_NUM;
		    break;
		case 'l':		// "valmap=..."
		    i1 = VMAP_PARAM_NUM; 
		    i2 = DEFVMAP_PARAM_NUM;
		    break;
		case 'p':		// "opmap=..."
		    i1 = OMAP_PARAM_NUM; 
		    i2 = DEFOMAP_PARAM_NUM;
		    break;
		default:
		    i1 = i2 = 0;
		    break;
	    }
	    if (i1 && !null_default) {
		values++;
		this->setInputValueQuietly(i1,buf, DXType::VectorListType);
	    }
	    if (i2) {
		values++;
		this->setInputValueQuietly(i2,buf, DXType::VectorListType);
	    }
	    p += 4; 
	} while ( (p = strstr((char*)p,"map=")) );
	delete buf;
    }
    
    this->undeferMinMaxUpdates();
 
    return values;
}
//
// Update any UI visuals that may be based on the state of this
// node.   Among other times, this is called after receiving a message
// from the executive.  This ignores "unmanaged".
//
void ColormapNode::reflectStateChange(boolean /* unmanaged */ ) 
{
   ColormapEditor *editor = this->colormapEditor;

   if (editor)
	editor->handleStateChange();
}


//
// Returns the array of histogram values with the size of the array returned
// in nhist.  The returned array must be freed by the caller.
//
int *ColormapNode::getHistogram(int *nhist)
{
    const char *s = this->histogram;
    int count = 0, *hist = NULL;
 
    if (s) {
        char buf[64];
	int index = -1;
	while (DXValue::NextListItem(s, &index, DXType::ScalarListType, buf,64)) {
	    if ((count & 31) == 0)
		hist = (int*)REALLOC(hist,(count+32) * sizeof(int));
	    hist[count++] = atoi(buf);
	}
    }
    *nhist = count;
    return hist;
}

boolean ColormapNode::isNBinsAlterable()
{
     return this->isInputConnected(DATA_PARAM_NUM) &&
            !this->isInputConnected(NHIST_PARAM_NUM);
}

//
// Clear the histogram data if we disconnect
//
void ColormapNode::ioParameterStatusChanged(boolean input, int index,
                                         NodeParameterStatusChange status )
{
    if (input) {
	if ((index == DATA_PARAM_NUM) && 
	    (status == Node::ParameterArkRemoved)) {
	    if (this->histogram) {
		delete histogram;
	        this->histogram = NULL;
	    }
	} else if (((index == MIN_PARAM_NUM) || (index == MAX_PARAM_NUM)) &&
		   (status & PARAMETER_VALUE_CHANGED) &&
		   (status != Node::ParameterSetValueToDefaulting)) {
	    this->updateMinMaxAttr->requestAction(NULL);
	}
	switch (index) {
	    case HMAP_PARAM_NUM:
	    case SMAP_PARAM_NUM:
	    case VMAP_PARAM_NUM:
	    case OMAP_PARAM_NUM:
	    case DEFHMAP_PARAM_NUM:
	    case DEFSMAP_PARAM_NUM:
	    case DEFVMAP_PARAM_NUM:
	    case DEFOMAP_PARAM_NUM:
		//
		// The above inputs are not viewable and therefore may/will 
		// not cause a notifyVisuals to take place in 
		// DrivenNode::ioParameterStatusChange().  
		//
		this->notifyVisualsOfStateChange();
		break;
	    default:
		break;
	}
    }

    //
    // This calls this->reflectStateChange() for input changes.
    //
    this->DrivenNode::ioParameterStatusChanged(input, index, status);
}

//
// Determine if this node is of the given class.
//
boolean ColormapNode::isA(Symbol classname)
{
    Symbol s = theSymbolManager->registerSymbol(ClassColormapNode);
    if (s == classname)
	return TRUE;
    else
	return this->DrivenNode::isA(classname);
}
//
// Rescale the values from the old min and max to the new ones. 
//
void ColormapNode::RescaleDoubles(double *values, int count,
				double newmin, double newmax)
{
    int i;
    double rel_slope, min=0, max=0;

    for (i=0 ; i<count ; i++) {
	double val = values[i];
	if (i == 0) {
	    min = max = val;
	} else if (val > max) {
	    max = val;
	} else if (val < min) {
	    min = val;
	}
    }
    if (min == max) {
	values[i] = newmin; 
    } else {
	rel_slope = (newmax - newmin) / (max - min);
	for (i=0 ; i<count ; i++) {
	    double v = values[i];
	    values[i] = newmin + rel_slope * (v - min);
	}
    }
    
}
const char *ColormapNode::getHueMapList()
{
    return this->getInputValueString(HMAP_PARAM_NUM);
}
const char *ColormapNode::getSaturationMapList()
{
    return this->getInputValueString(SMAP_PARAM_NUM);
}
const char *ColormapNode::getValueMapList()
{
    return this->getInputValueString(VMAP_PARAM_NUM);
}
const char *ColormapNode::getOpacityMapList()
{
    return this->getInputValueString(OMAP_PARAM_NUM);
}
//
// Read the .cm file and set the 1st - 4th inputs and the min/max with 
// the values found there.  If send is TRUE, then update the executive.
//
boolean ColormapNode::cmOpenFile(const char *cmapfile, boolean send,
						boolean installDefaultOnError)
{
    boolean r = this->cmAccessFile(cmapfile, TRUE, installDefaultOnError);

    if (send)
	this->sendValues(FALSE);

    return r;
}
//
// Save the .cm file from the 1st - 4th inputs and the min/max with 
// the values found there. 
//
boolean ColormapNode::cmSaveFile(const char *cmapfile)
{
    return this->cmAccessFile(cmapfile, FALSE);
}
//
// Save or read a .cm file which contains control point information about
// the colormap editor.  If an error occurs  and FALSE is returned, 
// otherwise TRUE.  If a failure occurs while reading
// the .cm file and useDefaultOnOpenError is TRUE,  then the default colormap 
// is installed (without being sent to the executive). 
//
boolean ColormapNode::cmAccessFile(const char *cmapfile, 
					boolean opening,
					boolean useDefaultOnOpenError)
{
    char *map, *cmname, *name,*path,*temp;
    double min,max;
    FILE *fp;

    temp = GetDirname(cmapfile);
    path = GetFullFilePath(temp);
    name = GetFileBaseName(cmapfile, ".cm");
    cmname = new char[STRLEN(path)+STRLEN(name)+5];
    sprintf(cmname, "%s/%s.cm", path, name);

    delete temp;
    delete path;
    delete name;

    //
    // Keep every setInputValue*() from updating the editor
    //
    this->deferVisualNotification();

    if (opening) {
	// Open file here
	char buffer[128];
	fp = fopen(cmname,"r");
	if (!fp) 
	    goto error; 
	
	// Get the line containing the min
	if (!fgets(buffer,128, fp) || sscanf(buffer,"%lg", &min) < 0)  
	    goto error;
	// Get the line containing the max 
	if (!fgets(buffer,128, fp) || sscanf(buffer,"%lg", &max) < 0) 
	    goto error;

	this->deferMinMaxUpdates();
	this->setMinimumValue(min);
	this->setMaximumValue(max);
	this->undeferMinMaxUpdates();

	//
	// Parse the hue map
	//
	map = this->cmParseMap(fp, min,max);
	if (!map) 
	    goto error;
	this->setInputValue(HMAP_PARAM_NUM,map,DXType::VectorListType,FALSE);
	delete map;
	//
	// Parse the saturation map
	//
	map = this->cmParseMap(fp, min,max);
	if (!map)
	    goto error;
	this->setInputValue(SMAP_PARAM_NUM,map,DXType::VectorListType,FALSE);
	delete map;
	//
	// Parse the value map
	//
	map = this->cmParseMap(fp, min,max);
	if (!map)
	    goto error;
	this->setInputValue(VMAP_PARAM_NUM,map,DXType::VectorListType,FALSE);
	delete map;
	//
	// Parse the opacity map
	//
	map = this->cmParseMap(fp, min,max);
	if (!map) 
	    goto error;
	this->setInputValue(OMAP_PARAM_NUM,map,DXType::VectorListType,FALSE);
	delete map;

    } else {    // Saving
        char  *file, *tmpfile = UniqueFilename(cmname);
#if HAS_RENAME
        tmpfile = UniqueFilename(cmname);
#else
	tmpfile = NULL;
#endif
        if (tmpfile)
            file = tmpfile;
        else
            file = cmname;

	fp = fopen(file,"w");
	if (!fp) 
	    goto error; 
	min = this->getMinimumValue();
	max = this->getMaximumValue();
	if ((fprintf(fp,"%g\n%g\n",min,max) <= 0) 		||
	    !this->cmPrintMap(fp, min,max,this->getHueMapList()) 	||
	    !this->cmPrintMap(fp, min,max,this->getSaturationMapList()) ||
	    !this->cmPrintMap(fp, min,max,this->getValueMapList())	||
	    !this->cmPrintMap(fp, min,max,this->getOpacityMapList())) 
	    goto error;

        if (tmpfile) {
#if HAS_RENAME
#ifdef DXD_WIN
	    if (fp)
		fclose(fp);
	    fp = NULL;
	    _unlink(cmname);
#endif
            rename(tmpfile,cmname);
#else
	    ASSERT(0); 
#endif
            delete tmpfile;
        }
    }

    if (fp) {
	if (fclose(fp) != 0) {
	    this->undeferVisualNotification();
	    return FALSE; 
	}
    }
    if (cmname) delete cmname;

    this->undeferVisualNotification(); 
    return TRUE;

error:
    if (cmname) delete cmname;
    if (fp) fclose(fp);
    if (fp && opening && useDefaultOnOpenError)
	this->installTHEDefaultMaps(FALSE);
    this->undeferVisualNotification(); 
    return FALSE;
}
//
// This method implements backwards compatibility for v 2.0.2 nets
// and older.  Upon entering this method, it is assumed that the first
// 4 inputs (private and hidden) contain old style output values.
// After version 2.0.2 (next was 2.1) .net files the first four
// inputs (unviewable) changed meaning.  They used to contain
//          #1) hsvdata     (scalar list)
//          #2) hsv         (3-vector list)
//          #3) opdata      (scalar list)
//          #4) op          (scalar list)
// With hsvdata in 1:1 correspondence with hsv and the same for
// opdata and op.
// After version 2.0.2, we change the inputs as follows
//          #1) huepts      (2-vector list)
//          #2) satpts      (2-vector list)
//          #3) valepts     (2-vector list)
//          #4) oppts       (2-vector list)
// Each vector is a 2-vector representing the position of a control
// point in the color map where the first component is the data
// value and the 2nd is the h/s/v/opacity value.
// 
// This method converts the old values to the new values. Note, that
// it may create more control points that were in the original map.
//
void ColormapNode::convertOldInputs()
{
    const char *value; 
    double *hsvdata, *opdata, *h, *s=NULL, *v=NULL, *op; 
    int datacnt, hsvcnt, opcnt, i,j, tuples;

    //
    // Extract the h/s/v and corresponding data levels.
    //
    value = this->getInputValueString(OLDHSVDATA_PARAM_NUM);
    datacnt = DXValue::GetDoublesFromList(value, DXType::ScalarListType,
					&hsvdata,  &tuples);
    if (datacnt > 0) {
	//
	// Scale the data values into a normalized range
	//
 	this->RescaleDoubles(hsvdata, datacnt, 0.0, 1.0);

	double *hsv_values;
        value = this->getInputValueString(OLDHSV_PARAM_NUM);
    	hsvcnt = DXValue::GetDoublesFromList(value, DXType::VectorListType,
					&hsv_values,  &tuples);
	ASSERT(tuples == 3);
	ASSERT(datacnt == hsvcnt);
	h = new double[3 * hsvcnt];	
	s = &h[hsvcnt];
	v = &h[2*hsvcnt];
	for (j=i=0 ; i<hsvcnt ; i++ ) {
	    h[i] = hsv_values[j++];
	    s[i] = hsv_values[j++];
	    v[i] = hsv_values[j++];
	}
	delete hsv_values;
    } else {
	hsvcnt = 0;
	h = NULL;
    } 

    //
    // Extract the opacity and corresponding data levels.
    //
    value = this->getInputValueString(OLDOPDATA_PARAM_NUM);
    datacnt = DXValue::GetDoublesFromList(value , DXType::ScalarListType,
					&opdata,  &tuples);
    if (datacnt > 0) {
	//
	// Scale the data values into a normalized range
	//
 	this->RescaleDoubles(opdata, datacnt, 0.0, 1.0);

        value = this->getInputValueString(OLDOP_PARAM_NUM);
    	opcnt = DXValue::GetDoublesFromList(value, DXType::ScalarListType,
					&op,  &tuples);
    } else {
	opcnt = 0;
	op = NULL;
    } 


    //
    // And now finally install the new maps from the old inputs.
    //
    this->installNewMaps(hsvcnt,hsvdata, h,
			hsvcnt, hsvdata, s,
			hsvcnt, hsvdata, v,
			opcnt,  opdata,  op,
			FALSE);

    if (hsvdata) delete hsvdata;
    if (h) delete h;
    if (opdata) delete opdata;
    if (op) delete op;
}
//
// Given two arrays of data levels and h/s/v/opacity value, construct
// the vector list representing the given values.  Each 2-vector contains
// a level and a corresponding value with the level in the 1st component.
// If count is given as 0, the string value "NULL" is returned.
// If valstr is provided, then it is used to place the vector list in and
// is used as the return value.
// If not, then a new string which must be freed by the caller is 
// returned.
//
char *ColormapNode::makeControlPointList(int count, 
			double *level, double *value, 
			char *valstr)
{
    if (count <= 0) { 
	if (valstr)
	    strcpy(valstr,"NULL");
	else
	    valstr = DuplicateString("NULL");
    } else {
	int i;
	char *p;
	if (!valstr)
	    valstr =  new char[32 * 2 * count];
	strcpy(valstr,"{ ");
	p = valstr;
	for (i=0, p += STRLEN(p) ; i<count ; i++, p+=STRLEN(p)) {
	    char v1[128], v2[128];
	    DXValue::FormatDouble(level[i],v1);
	    DXValue::FormatDouble(value[i],v2);
	    sprintf(p, "[%s %s] ", v1,v2); 
	} 
	strcat(p,"}");
    }
    return valstr;
}
//
// Parse lines out of the color map file that describe a particular
// map (either hue/sat/val/opacity).  The map is parsed into a VectorList
// and returned.  The returned string must be freed by the caller.  If
// there are any errors parsing the file, then NULL is returned.
//
char *ColormapNode::cmParseMap(FILE *fp, double min, double max)
{
#define BUFLEN 1024
    double *values = NULL, *levels = NULL; 
    char *map=NULL, buffer[BUFLEN];
    int count;

    if (!fgets(buffer,BUFLEN, fp))
	goto error;

    if (sscanf(buffer,"%d", &count) != 1)
	goto error;

    if (count > 0) {
	levels = new double[2 * count];
	values = &levels[count]; 
	int i;
	
	for (i=0 ; i<count ; i++) {
	    if (!fgets(buffer,BUFLEN, fp))
		goto error;
	    if (sscanf(buffer,"%lg %lg", &levels[i], &values[i]) != 2)
		goto error;
	}
    } 
    map = this->makeControlPointList(count, levels, values);
    
    if (levels) delete levels;
    return map;

error:
    if (levels) delete levels;
    if (map) delete map;
    return NULL;
}
//
// Print lines to the color map file that describe a particular
// map (either hue/sat/val/opacity).
// No error message is issued if an error occurs.
//
boolean ColormapNode::cmPrintMap(FILE *fp, double min, double max, 
						const char *map)
{
    double *values = NULL;
    int offset, i, tuples, count;


    count = DXValue::GetDoublesFromList(map, DXType::VectorListType, 
					&values, &tuples);

    if (fprintf(fp,"%d\n",count) <= 0)
	goto error;
    
    for (offset=i=0 ; i<count ; i++) {
	// Print out the (normalized) level and value for this map.
	// The 3 trailing zeros are included for backwards compatibility
	double l = values[offset];
	if (fprintf(fp,"%.20f   %.20f  0  0  0\n",l,values[offset+1]) <= 0)
	    goto error;
	offset += 2;
    } 

    if (values) delete values;
    return TRUE;

error:
    if (values) delete values;
    return FALSE;
}
void ColormapNode::UpdateAttrParam(void *data, void *request_data)
{
    ColormapNode *cmap = (ColormapNode*)data;
    double min = cmap->getMinimumValue();
    double max = cmap->getMaximumValue();
    char buffer[1024];
    char v1[128], v2[128];
    DXValue::FormatDouble(min,v1);
    DXValue::FormatDouble(max,v2);
    sprintf(buffer, "{%s %s}", v1,v2); 
    cmap->setInputValueQuietly(ATTR_PARAM_NUM,buffer);
}
//
// The colormap node is always present in the network (i.e. there is a
// module that executes on its behalf) and may receive messages even
// when it does not have input connections.  We BELIEVE that the only
// time it will receive messages while not having any connections is
// when the network is marked dirty (i.e. the first execution of a new
// network).
//
boolean ColormapNode::expectingModuleMessage()
{
    return this->getNetwork()->isDirty() || 
		this->DrivenNode::expectingModuleMessage();
}
boolean ColormapNode::hasCfgState()
{
    return TRUE;
}
boolean ColormapNode::cfgPrintNode(FILE *f, PrintType dest)
{
    if (!this->cfgPrintNodeLeader(f))
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

boolean ColormapNode::cfgParseComment(const char *comment, const char *file,
                                int lineno)
{
    if (this->cfgParseNodeLeader(comment,file,lineno))
        return TRUE;

    if (this->parseIOComment(TRUE, comment,file,lineno,TRUE))
        return TRUE;

    if (this->parseCommonComments(comment,file,lineno))
        return TRUE;

    return FALSE;
}
boolean ColormapNode::printCommonComments(FILE *f, const char *indent)
{
    int x, y, w, h;

    if (!indent)
	indent = ""; 

    if (this->colormapEditor) {
	this->colormapEditor->getGeometry(&x,&y,&w,&h);
    } else { 
	x = this->xpos;
	y = this->ypos;
	w = this->width;
	h = this->height;
    } 

    const char *title = this->getTitle();
    if (title)
        if (fprintf(f, "%s// Colormap: title = %s\n", indent, title) < 0)
            return FALSE;;

    //
    // parseCommonComments() assumes this is the last comment
    //
    if (!UIComponent::PrintGeometryComment(f, x,y,w,h,NULL,indent))
	return FALSE;


    return TRUE;
}
boolean ColormapNode::parseCommonComments(const char *comment, const char *file,
                                int lineno)
{

#define TITLE_COMMENT_LEN 19	// The length of " Colormap: title = "
    if (EqualSubstring(" Colormap: title = ",comment,TITLE_COMMENT_LEN)) {
	this->setTitle(comment + TITLE_COMMENT_LEN); 
	return TRUE;
    } else if (UIComponent::ParseGeometryComment(comment,file,lineno,
			&this->xpos, &this->ypos, 
			&this->width, &this->height))  {
	if (this->colormapEditor) { 
 	    if ((this->xpos != UIComponent::UnspecifiedPosition)  &&
		theDXApplication->applyWindowPlacements())
		this->colormapEditor->setGeometry(this->xpos,this->ypos,
						    this->width,this->height);
	    //
	    // This is the last comment, we can now apply all the .cfg
	    // information to the cmap editor window.
	    //
	    this->syncAttributeParameters();
	    this->colormapEditor->handleStateChange();
	}
	return TRUE;
    } 

    return FALSE;
}

int ColormapNode::getMessageIdParamNumber() { return ID_PARAM_NUM; }

//
// In addition to superclass work, sendValues to the exec.  Sending values was
// held off during node creation and initialization if the node's network !=
// theDXApplication->network.  Reason:  ColormapNode initializes himself in
// a noisy way using its instance number which in the case of mergeNetwork
// operations is the wrong network.  The instance number will be updated
// by switching networks at which time its safe to go ahead and spout off.
//
void ColormapNode::switchNetwork(Network *from, Network *to, boolean silently)
{
    this->DrivenNode::switchNetwork(from, to);

    //
    // Loop over params, sending the dirty ones quietly.
    //
    if (this->getNetwork() == theDXApplication->network) {
	this->updateMinMaxAttr->undeferAction();
	this->sendValuesQuietly();
    }
}

//
// Control deferring updates on min/max params based on membership in the
// real network.  (Added just before 3.1.4) We'll behave as we always have
// in most situtations.  But in the case of membership in a temporary network
// we don't want to allow updates to be sent to the exec because we haven't
// received the proper instance number yet.
// This new deferMinMaxUpdates method really isn't necessary.  I just want you
// to see that you'll have to change code if you want to put Colormaps into
// macros.  
//
void ColormapNode::deferMinMaxUpdates()
{
    NodeDefinition* nd = this->getDefinition();
    ASSERT(nd->isAllowedInMacro() == FALSE);
    if (this->getNetwork() == theDXApplication->network)
	this->updateMinMaxAttr->deferAction();
    else if (this->updateMinMaxAttr->isActionDeferred() == FALSE)
	this->updateMinMaxAttr->deferAction();
}

void ColormapNode::undeferMinMaxUpdates()
{
    NodeDefinition* nd = this->getDefinition();
    ASSERT(nd->isAllowedInMacro() == FALSE);
    if (this->getNetwork() == theDXApplication->network)
	this->updateMinMaxAttr->undeferAction();
}
