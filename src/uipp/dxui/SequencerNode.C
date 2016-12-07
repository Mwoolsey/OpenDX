/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"


#include "lex.h"
#include "Parameter.h"
#include "DXApplication.h"
#include "SequencerNode.h"
#include "SequencerWindow.h"
#include "DXStrings.h"
#include "ErrorDialogManager.h"
#include "Network.h"
#include "Command.h"
#include "WarningDialogManager.h"
#include "IBMVersion.h"

#include "../widgets/XmDX.h"

#define ID_PARAM_NUM	1
#define DATA_PARAM_NUM 	2
#define FRAME_PARAM_NUM 3
#define MIN_PARAM_NUM 	4
#define MAX_PARAM_NUM 	5
#ifndef HAS_START_STOP
#define DELTA_PARAM_NUM 6
#define ATTR_PARAM_NUM  7
#define EXPECTED_INPUTS ATTR_PARAM_NUM
#else
#define START_PARAM_NUM 6
#define STOP_PARAM_NUM 	7
#define DELTA_PARAM_NUM 8
#define EXPECTED_INPUTS DELTA_PARAM_NUM
#endif


SequencerNode::SequencerNode(NodeDefinition *nd, Network *net, int instnc) :
    ShadowedOutputNode(nd, net, instnc)
{
    this->seq_window = NULL;
    this->wasExecuted = FALSE;
    this->xpos = this->ypos = UIComponent::UnspecifiedPosition;
    this->width = this->height = UIComponent::UnspecifiedDimension;

    //
    // Initialize the startup value based on the net version number.  This preserves
    // an old behvior: the sequencer always appears when you startup in image mode.
    // It also provides a new behavior: you can mark the vcr as non-startup window
    // so that it won't automatically appear when loading the .net in image mode.
    // The behavior for new nets will be to mark the vcr as a startup window if 
    // it's startup flag was set.  The startup flag will be set thru the vcr's
    // manage/unmange methods rather than a toggle button.
    //
    int major = net->getNetMajorVersion();
    int minor = net->getNetMinorVersion();
    int micro = net->getNetMicroVersion();
    if (VERSION_NUMBER (major, minor, micro) < VERSION_NUMBER (3,1,4))
	this->startup = TRUE;
    else
	this->startup = FALSE;
}

SequencerNode::~SequencerNode()
{
    theDXApplication->openSequencerCmd->deactivate();
#if 00
    theDXApplication->network->sequencer = NUL(SequencerNode*);
#endif

    if (this->seq_window) delete this->seq_window;
}

boolean SequencerNode::initialize()
{
    this->seq_window = NUL(SequencerWindow*);

    if (this->getInputCount() != EXPECTED_INPUTS) {
        fprintf(stderr,
           "Expected %d inputs for %s node, please check the mdf file.\n",
                        EXPECTED_INPUTS,
                        this->getNameString());
	return FALSE;
    }

    //
    // This keeps updateAttributes() from sending messages until
    // we're done.
    //
    this->deferVisualNotification();

    if (!this->setMessageIdParameter(ID_PARAM_NUM) 	||
    	!this->initMinimumValue(1)			||
	!this->initMaximumValue(100)			||
	!this->initDeltaValue(    1)			||
	!this->initStartValue(    1)			||
	!this->initStopValue(    100)) {
	ErrorMessage("Error setting default attributes for %s",
                        this->getNameString());
	return FALSE;
    }

    this->undeferVisualNotification();

    this->current         = 0;
    this->next            = 1;
    this->previous        = 1;
    this->step            = FALSE;
    this->loop            = FALSE;
    this->palindrome      = FALSE;
    this->current_defined = FALSE;
    this->ignoreFirstFrameMsg = FALSE;

    this->transmitted     = False;

#if 00
    theDXApplication->network->sequencer = this;
#endif

    theDXApplication->openSequencerCmd->activate();

    return TRUE;
}

//
// If the min or max input has changed, update the attribute parameter
// (integer list of min and max) and then call the super class method
// if the input is not the attribute parameter.
//
void SequencerNode::ioParameterStatusChanged(boolean input, int index,
                                NodeParameterStatusChange status)
{
    if (input && (status & Node::ParameterValueChanged) &&
	((index == MIN_PARAM_NUM)   || 
	 (index == DELTA_PARAM_NUM) ||
	 (index == MAX_PARAM_NUM))) {
	//
	// Each time the min or max attribute values (which are stored
	// in AttributeParamters and so we come here each time they are 
	// changed) update the attribute parameter which is an integer
	// list of the min and max values.
	// Note that the attribute parameter is expected to have 
 	// InputDoesNotDeriveOutputCacheTag cacheability, so that setting 
	// this over and over again will not cause executions.
	//
	this->updateAttributes();
    }
    if (!input || (index != ATTR_PARAM_NUM)) 
	ShadowedOutputNode::ioParameterStatusChanged(input,index,status);
}
void SequencerNode::openDefaultWindow(Widget parent)
{
    if (this->seq_window)
	this->seq_window->manage();
    else
    {
	this->seq_window = new SequencerWindow(this);
        if ((this->xpos != UIComponent::UnspecifiedPosition)  &&
            theDXApplication->applyWindowPlacements())
	    this->seq_window->setGeometry(this->xpos,this->ypos,
					this->width,this->height);
  	this->seq_window->initialize();
	this->seq_window->manage();

	if (theDXApplication->getExecCtl()->isExecuting())
	    this->disableFrameControl();
	else
	    this->enableFrameControl();
    }
}

char* SequencerNode::netNodeString(const char *prefix)
{
    char *s;

    if (!this->isDataDriven()) {
        int inst = this->getInstanceNumber();
        s = new char[96];
    	sprintf(s, "    %sSequencer_%d_out_1 = @frame;\n", prefix, inst);
    } else {
	//
	// Sequencer_%d_in_(FRAME_PARAM_NUM) = @frame;
	// Sequencer_%d_out_1 = Sequencer(xxx, xxx, 
 	//		 	    Sequencer_%d_in_(FRAME_PARAM_NUM)....);
	// 
	char buf[128];
	char *call = this->ShadowedOutputNode::netNodeString(prefix);
	this->getNetworkInputNameString(FRAME_PARAM_NUM,buf);
	int len = STRLEN(call) + STRLEN(buf) + 64;
	s = new char[len];
	sprintf(s,"    %s = @frame;\n%s", buf, call);
	delete call;
    }
    return s;
}

boolean SequencerNode::netPrintAuxComment(FILE *f)
{
    this->wasExecuted = TRUE;

    if (!this->DrivenNode::netPrintAuxComment(f))
	return FALSE;

    if (!this->printCommonComments(f,"    "))
        return FALSE;

    return TRUE;

}
boolean SequencerNode::netParseAuxComment(const char* comment,
                            		const char *file, 
                            		int lineno)
{
    this->wasExecuted = TRUE;

    if (this->DrivenNode::netParseAuxComment(comment,file,lineno))
	return TRUE;

    if (this->parseCommonComments(comment,file,lineno))
        return TRUE;

    return FALSE;

}

boolean SequencerNode::cfgPrintNode(FILE *f, PrintType dest)
{
    if (!this->cfgPrintNodeLeader(f))
        return FALSE;

    if (!this->printCommonComments(f))
	return FALSE;

    //
    // Print startup mode into the cfg file.
    // Do this only when saving a file because Network will need to manage
    // the sequencer window if the startup flag is turned on.  That's unwanted
    // behavior during drag-n-drop and not relevant when printing to the exec.
    //
    if (fprintf (f, "// startup = %d\n", (dest == PrintFile ? this->startup: 0)) < 0)
	return FALSE;

    return TRUE;
}
boolean SequencerNode::cfgParseComment(const char* comment,
                            		const char *file, 
                            		int lineno)
{
    if (this->cfgParseNodeLeader(comment,file,lineno)) 
        return TRUE;

    if (this->parseCommonComments(comment,file,lineno))
        return TRUE;

    //
    // Parse the startup mode from the cfg file.
    //
    const char* substr = " startup";
    if (EqualSubstring (comment, substr, strlen(substr))) {
	int startup_flag;
	int items_parsed = sscanf (comment, " startup = %d", &startup_flag);
	if (items_parsed == 1) {
	    this->setStartup(startup_flag);
	    return TRUE;
	}
    }

    return FALSE;
} 

boolean SequencerNode::printCommonComments(FILE *f, const char *indent)
{
    if (!indent)
	indent = ""; 

    //
    // Include the instance number for forward compatibility reasons 
    // since we don't enforce sequencer instance numbers to be 1.
    //
    if (fprintf(f, "%s//" 
           " vcr[%d]: min = %d, max = %d, beg = %d, end = %d, cur = %d, "
		"inc = %d, loop = %s, step = %s, pal = %s\n",
	   indent,
           this->getInstanceNumber(), 
           this->getMinimumValue(), 
	   this->getMaximumValue(), 
	   this->getStartValue(), 
	   this->getStopValue(), 
	   this->next, 
	   this->getDeltaValue(),
           this->loop ? "on" : "off", 
	   this->isStepMode() ? "on" : "off", 
	   this->palindrome ? "on" : "off") < 0)
	return FALSE;

    int x,y,w,h;
    if (this->seq_window) {
	this->seq_window->getGeometry(&x,&y,&w,&h);
    } else {
	x = this->xpos; y = this->ypos;
	w = this->width; h = this->height;
    }
    
    if (x != UIComponent::UnspecifiedPosition) {
	if (!UIComponent::PrintGeometryComment(f,x,y,w,h,NULL,indent))
	    return FALSE;
    }
    return TRUE;
}


boolean SequencerNode::parseCommonComments(const char* comment,
                            		const char *file, 
                            		int lineno)
{
    int  expected_items, items, instance;
    int  min, max, beg, end, cur, inc;
    char loop[64];
    char step[64];
    char pal[64];

    if (EqualSubstring(comment," vcr",4)) {

	//
	// See comment in :cfgPrint.  This is to preserve behavior of existing
	// nets.
	//
	this->startup = TRUE;

	if (EqualSubstring(comment," vcr[",5)) {
	    expected_items = 10;
	    items = sscanf(comment,
	       " vcr[%d]: min = %d, max = %d, beg = %d, end = %d, cur = %d,"
	       " inc = %d, loop = %[^,], step = %[^,], pal = %[^\n]",
	       &instance, // Drop this on the floor until we support >1 Sequencers
	       &min, &max, &beg, &end, &cur, &inc,
	       loop, step, pal);
	} else {
	    //
	    // Pre 2.0.0 nets
	    //
	    expected_items = 9;
	    items = sscanf(comment,
	       " vcr: min = %d, max = %d, beg = %d, end = %d, cur = %d, inc = %d,"
	       " loop = %[^,], step = %[^,], pal = %[^\n]",
	       &min, &max, &beg, &end, &cur, &inc,
	       loop, step, pal);
	}

	if(items < expected_items)
	{
	    ErrorMessage("Malformed comment at line %d in file %s (ignoring)",
					lineno, file);
	    return TRUE;
	} 

	//
	// This keeps updateAttributes() from sending messages until
	// we're done.
	//
	this->deferVisualNotification();

	this->setMinimumValue(min);
	this->setMaximumValue(max);
	this->setDeltaValue(inc);
	this->setStartValue(beg);
	this->setStopValue(end);

	this->undeferVisualNotification();

	this->next       = cur;
	this->loop       = EqualString(loop, "on");
	this->step       = EqualString(step, "on");
	this->palindrome = EqualString(pal,  "on");
	return TRUE;

    }

	
    if (UIComponent::ParseGeometryComment(comment,file,lineno,
				&this->xpos,&this->ypos,
				&this->width, &this->height)) {
        if (this->seq_window &&
            (this->xpos != UIComponent::UnspecifiedPosition)  &&
            theDXApplication->applyWindowPlacements())
	    this->seq_window->setGeometry(this->xpos,this->ypos,
					this->width,this->height);

	return TRUE;
    }

    return FALSE;
}

// 
// Send the values as per the super class method and also send the
// @startframe, @nextframe, @endframe, @deltaframe values.  If the 
// Sequencer is data-driven then we send the @*frame values as NULL,
// since the Sequencer module will set these internally.  Also, we
// want to be sure we don't pass in any values that might be replicated
// later in the sequence which would confuse the caching mechanism.  
// For example, if we set @nextframe=2, and the sequence goes 0 1 2 3 ...,
// then the output will be 0 1 2 0, because the executive thinks the
// Sequencer has already seen the 4th set of values (remember that the
// Sequencer is called something like the following...
//     Sequencer_1_in_3 = @frame ;
//     Sequencer_1_out_1 = Sequencer(...,Sequencer_1_in_3,...); 
// The idea here is to set @nextframe to something that will never be
// seen during the sequence.
//
char* SequencerNode::valuesString(const char *prefix)
{
    char* inputs = this->ShadowedOutputNode::valuesString(prefix); 
    char* s = new char[STRLEN(inputs) + 128];
 
#if 0
    if (this->isDataDriven()) {
	sprintf(s, 
	    "%s\n" 
	    // FIXME: these -1e6's should be replaced by NULLs and 'play'
  	    // should not init @nextframe=0 if it sees @nextframe==NULL;
	    "@startframe = -1e6;\n"
	    "@nextframe  = -1e6;\n"
	    "@endframe   = -1e6;\n"
	    "@deltaframe = -1e6;\n",
	    inputs);
    } else 
#endif
	{
	sprintf(s,
	    "%s\n" 
	    "@startframe = %d;\n"
	    "@nextframe  = @startframe;\n"
	    "@endframe   = %d;\n"
	    "@deltaframe = %d;\n",
	    inputs,
	    this->getStartValue(),
	    this->getStopValue(),
	    this->getDeltaValue());
    }

    delete inputs;
    return s;
}

Widget  SequencerNode::getVCRWidget()
{
    if(this->seq_window)
       return seq_window->vcr;
    else
       return NUL(Widget);
}

// The messages we parse can contain one or more of the following...
//
// 'min=%g' 'max=%g' 'start=%d' 'end=%d' 'delta=%g' 
//
// If any input or output values are to be changed, don't send them
// because the module backing up the sequencer will have just executed
// and if the UI is in 'execute on change' mode then there will be an
// extra execution.
//
// Returns the number of attributes parsed.
//
int SequencerNode::handleNodeMsgInfo(const char *line)
{
    int index, values = 0;
    char *p, *buf = NULL;
 
    this->wasExecuted = TRUE;

    //
    // Handle the 'min=%g' part of the message.
    //
    if ( (p = strstr((char*)line,"min=")) ) {
	values++;
	while (*p != '=') p++;
	p++;
	buf = DuplicateString(p);
	index = 0;
	if (IsScalar(buf, index)) {
	    buf[index] = '\0'; 
	    this->setInputAttributeFromServer(MIN_PARAM_NUM,
					buf, DXType::UndefinedType);
#ifndef HAS_START_STOP
	    this->setStartValue(atoi(buf));
#endif
	}
	delete buf;
    }
    //
    // Handle the 'max=%g' part of the message.
    //
    if ( (p = strstr((char*)line,"max=")) ) {
	values++;
	while (*p != '=') p++;
	p++;
	buf = DuplicateString(p);
	index = 0;
	if (IsScalar(buf, index)) {
	    buf[index] = '\0'; 
	    this->setInputAttributeFromServer(MAX_PARAM_NUM,
					buf, DXType::UndefinedType);
#ifndef HAS_START_STOP
	     this->setStopValue(atoi(buf));
#endif
	}
	delete buf;
    }
    //
    // Handle the 'delta=%g' part of the message.
    // Since the attribute and parameter value have the same type, but
    // different meanings we only set the attribute value as received from
    // the exec.
    // NOTE: this is done before 'frame=' so that we can compute an accurate
    //	next frame.
    //
    if ( (p = strstr((char*)line,"delta=")) ) {
	values++;
	while (*p != '=') p++;
	p++;
	buf = DuplicateString(p);
	index = 0;
	if (IsScalar(buf, index)) {
	    buf[index] = '\0'; 
	    this->setInputAttributeFromServer(DELTA_PARAM_NUM,
					buf, DXType::UndefinedType);
	}
	delete buf;
    }

    //
    // Handle the 'frame=%g' part of the message.
    //
    if ( (p = strstr((char*)line,"frame=")) ) {
	values++;
	while (*p != '=') p++;
	p++;
	//
	// Change the currently display frame and ignore the next 
	// 'frame %d %d' message from the executive (not Sequencer).
	//
	this->current_defined = TRUE;
	this->current = atoi(p); 
	this->next = this->current + this->getDeltaValue();
	this->ignoreFirstFrameMsg = TRUE;
    }

    //
    // If min/max is defaulting (i.e. set from the frame control) and
    // max/min is set then we need to be sure that the defaulting value 
    // is consistent with the set value.
    //
    int min_val = this->getMinimumValue();
    int max_val = this->getMaximumValue();
    boolean min_dflting = this->isInputDefaulting(MIN_PARAM_NUM);
    boolean max_dflting = this->isInputDefaulting(MAX_PARAM_NUM);
    if (min_dflting || max_dflting) {
	char *setattr = NULL, *set = NULL;
	if (max_dflting && (min_val > max_val)) {
	    this->setMaximumValue(min_val);
	    max_val = min_val;
	    set     = "minimum";
	    setattr = "maximum";
	} else if (min_dflting && (min_val > max_val)) {
	    this->setMinimumValue(max_val);
	    min_val = max_val;
	    set     = "maximum";
	    setattr = "minimum";
 	}
	 if (setattr) {
	    WarningMessage("%s value provided to %s conflicts "
			    "with %s value set with 'Frame Control' dialog"
			    "...adjusting %s.",
			    set, this->getNameString(),
			    setattr,setattr);
	}
    }

    //
    // Make sure that the start/stop values are both within range of the 
    // possibly new min and/or max values.
    //
    int stop_val = this->getStopValue();
    int start_val = this->getStartValue();
    if ((start_val < min_val) || (start_val > max_val)) 
	this->setStartValue(min_val);
    if ((stop_val < min_val) || (stop_val > max_val)) 
	this->setStopValue(max_val);

#ifdef HAS_START_STOP
    //
    // Handle the 'start=%d' part of the message.
    //
    if (p = strstr((char*)line,"start=")) {
	values++;
	while (*p != '=') p++;
	p++;
	buf = DuplicateString(p);
	index = 0;
	if (IsScalar(buf, index)) {
	    buf[index] = '\0'; 
	    this->setInputAttributeFromServer(START_PARAM_NUM,
					buf, DXType::UndefinedType);
	    //
	    // Change the currently display frame and ignore the next 
	    // 'frame %d %d' message.
	    //
	    this->current_defined = TRUE;
	    this->current = this->getStartValue();
	    this->next = this->current + this->getDeltaValue();
	    this->ignoreFirstFrameMsg = TRUE;
	}
	delete buf;
    }
    //
    // Handle the 'end=%d' part of the message.
    //
    if (p = strstr((char*)line,"end=")) {
	values++;
	while (*p != '=') p++;
	p++;
	buf = DuplicateString(p);
	index = 0;
	if (IsScalar(buf, index)) {
	    buf[index] = '\0'; 
	    this->setInputAttributeFromServer(STOP_PARAM_NUM,
					buf, DXType::UndefinedType);
	}
	delete buf;
    }
#endif
   
    return values;
}

void SequencerNode::reflectStateChange(boolean unmanage)
{
    if (this->seq_window)
	this->seq_window->handleStateChange(unmanage);

    //
    // We do this here because we use isVisualNotificationDeferred() in
    // in this->updateAttributes() which means that we must call it again
    // once notification is undeferred (i.e. in reflectStateChange()).
    //
    this->updateAttributes();
}

#if 0 // 8/9/93
boolean SequencerNode::isAttributeVisuallyWriteable(int input_index)
{
     return 
	 this->isInputDefaulting(DATA_PARAM_NUM) && 
     	 this->ShadowedOutputNode::isAttributeVisuallyWriteable(input_index); 
}
#endif
boolean SequencerNode::isMinimumVisuallyWriteable()
{ 
    return this->isInputDefaulting(DATA_PARAM_NUM) &&
   	   this->isAttributeVisuallyWriteable(MIN_PARAM_NUM); 
}
boolean SequencerNode::isMaximumVisuallyWriteable()
{ 
    return this->isInputDefaulting(DATA_PARAM_NUM) &&
   	   this->isAttributeVisuallyWriteable(MAX_PARAM_NUM); 
}
#ifdef HAS_START_STOP
boolean SequencerNode::isStartVisuallyWriteable()
	{ return this->isAttributeVisuallyWriteable(START_PARAM_NUM); }
boolean SequencerNode::isStopVisuallyWriteable()
	{ return this->isAttributeVisuallyWriteable(STOP_PARAM_NUM); }
#endif
boolean SequencerNode::isDeltaVisuallyWriteable()
	{ return this->isAttributeVisuallyWriteable(DELTA_PARAM_NUM); }

boolean SequencerNode::initMinimumValue(int val)
{
    char buf[64];
    sprintf(buf,"%d",val);
    return this->initInputAttributeParameter(MIN_PARAM_NUM,buf);
}
boolean SequencerNode::setMinimumValue(int val)
{
    char buf[64];
    sprintf(buf,"%d",val);
    return this->setInputAttributeParameter(MIN_PARAM_NUM,buf);
}
#if 0
boolean SequencerNode::setMinimumValue(const char *val)
{
    return this->setInputAttributeParameter(MIN_PARAM_NUM,val);
}
#endif
int SequencerNode::getMinimumValue()
{
    const char *val = this->getInputAttributeParameterValue(MIN_PARAM_NUM);
    return atoi(val);
}
boolean SequencerNode::initMaximumValue(int val)
{
    char buf[64];
    sprintf(buf,"%d",val);
    return this->initInputAttributeParameter(MAX_PARAM_NUM,buf);
}
boolean SequencerNode::setMaximumValue(int val)
{
    char buf[64];
    sprintf(buf,"%d",val);
    return this->setInputAttributeParameter(MAX_PARAM_NUM,buf);
}
#if 0
boolean SequencerNode::setMaximumValue(const char *val)
{
    return this->setInputAttributeParameter(MAX_PARAM_NUM,val);
}
#endif
int SequencerNode::getMaximumValue()
{
    const char *val = this->getInputAttributeParameterValue(MAX_PARAM_NUM);
    return atoi(val);
}
boolean SequencerNode::initDeltaValue(int val)
{
    char buf[64];
    sprintf(buf,"%d",val);
    return this->initInputAttributeParameter(DELTA_PARAM_NUM,buf);
}
boolean SequencerNode::setDeltaValue(int val)
{
    char buf[64];
    sprintf(buf,"%d",val);
    return this->setInputAttributeParameter(DELTA_PARAM_NUM,buf);
}
#if 0
boolean SequencerNode::setDeltaValue(const char *val)
{
    return this->setInputAttributeParameter(DELTA_PARAM_NUM,val);
}
#endif
int SequencerNode::getDeltaValue()
{
    const char *val = this->getInputAttributeParameterValue(DELTA_PARAM_NUM);
    return atoi(val);
}
boolean SequencerNode::initStartValue(int val)
{
#ifndef HAS_START_STOP 
    this->startValue = val;
    this->updateAttributes();
    return TRUE;
#else
    char buf[64];
    sprintf(buf,"%d",val);
    return this->initInputAttributeParameter(START_PARAM_NUM,buf);
#endif
}
boolean SequencerNode::setStartValue(int val)
{
#ifndef HAS_START_STOP 
    this->startValue = val;
    this->updateAttributes();
    return TRUE;
#else
    char buf[64];
    sprintf(buf,"%d",val);
    return this->setInputAttributeParameter(START_PARAM_NUM,buf);
#endif
}
#if 0
boolean SequencerNode::setStartValue(const char *val)
{
    return this->setInputAttributeParameter(START_PARAM_NUM,val);
}
#endif
int SequencerNode::getStartValue()
{
#ifndef HAS_START_STOP 
    return this->startValue;
#else
    const char *val = this->getInputAttributeParameterValue(START_PARAM_NUM);
    return atoi(val);
#endif
}
boolean SequencerNode::initStopValue(int val)
{
#ifndef HAS_START_STOP 
    this->stopValue = val;
    this->updateAttributes();
    return TRUE;
#else
    char buf[64];
    sprintf(buf,"%d",val);
    return this->initInputAttributeParameter(STOP_PARAM_NUM,buf);
#endif
}
boolean SequencerNode::setStopValue(int val)
{
#ifndef HAS_START_STOP 
    this->stopValue = val;
    this->updateAttributes();
    return TRUE;
#else
    char buf[64];
    sprintf(buf,"%d",val);
    return this->setInputAttributeParameter(STOP_PARAM_NUM,buf);
#endif
}
#if 0
boolean SequencerNode::setStopValue(const char *val)
{
    return this->setInputAttributeParameter(STOP_PARAM_NUM,val);
}
#endif
int SequencerNode::getStopValue()
{
#ifndef HAS_START_STOP 
    return this->stopValue;
#else
    const char *val = this->getInputAttributeParameterValue(STOP_PARAM_NUM);
    return atoi(val);
#endif
}

//
// Disable the Frame control.
//
void SequencerNode::disableFrameControl()
{
    if (this->seq_window)
	this->seq_window->disableFrameControl();
}
//
// Enable the Frame control if the node is not data driven.
//
void SequencerNode::enableFrameControl()
{
    if (this->seq_window)
	this->seq_window->enableFrameControl();
}
void SequencerNode::setPlayDirection(SequencerDirection dir)
{
    if (this->seq_window) 
	this->seq_window->setPlayDirection(dir);
}
void SequencerNode::setForwardPlay()
{
    if (this->seq_window) 
	this->seq_window->setPlayDirection(SequencerNode::ForwardDirection);
}
void SequencerNode::setBackwardPlay()
{
    if (this->seq_window) 
	this->seq_window->setPlayDirection(SequencerNode::BackwardDirection);
}
void SequencerNode::setStepMode(boolean val)
{
    if (this->step == val)
	return;

    this->step = val;
    if (this->seq_window) 
	this->seq_window->handleStateChange(FALSE);
    
}
void SequencerNode::setLoopMode(boolean val)
{
    if (this->loop == val)
	return;

    this->loop = val;
    if (this->seq_window) 
	this->seq_window->handleStateChange(FALSE);
    
}
void SequencerNode::setPalindromeMode(boolean val)
{
    if (this->palindrome == val)
	return;

    this->palindrome = val;
    if (this->seq_window) 
	this->seq_window->handleStateChange(FALSE);
    
}
//
// Get a string representing the assignment to the global vcr variables, 
// @startframe, @frame, @nextframe, @endframe and @deltaframe.
// The returned string must be deleted by the caller.
// If the Sequencer is data-driven then we send the @*frame values as NULL,
// since the Sequencer module will set these internally.  Also, we
// want to be sure we don't pass in any values that might be replicated
// later in the sequence which would confuse the caching mechanism.  
// For example, if we set @nextframe=2, and the sequence goes 0 1 2 3 ...,
// then the output will be 0 1 2 0, because the executive thinks the
// Sequencer has already seen the 4th set of values (remember that the
// Sequencer is called something like the following...
//     Sequencer_1_in_3 = @frame ;
//     Sequencer_1_out_1 = Sequencer(...,Sequencer_1_in_3,...); 
// The idea here is to set @nextframe to something that will never be
// seen during the sequence.
//
//
char *SequencerNode::getFrameControlString()
{
    char *command = new char[128];

#if 0
    if (this->isDataDriven()) {
	    // FIXME: these -1e6's should be replaced by NULLs and 'play'
  	    // should not init @nextframe=0 if it sees @nextframe==NULL;
	strcpy(command, "@startframe,@frame,@nextframe,@endframe,@deltaframe ="
			     " -1e6, -1e6, -1e6, -1e6, -1e6;\n");
    } 
    else 
#endif
    {
	sprintf(command,
	     "@startframe,@frame,@nextframe,@endframe,@deltaframe "
	     "= %d,%d,%d,%d,%d;\n",
	     this->getStartValue(),
	     this->current_defined ? this->current : this->next,
	     this->next,
	     this->getStopValue(),
	     this->getDeltaValue());
    }

    return command;
}
//
// The Sequencer always expects the 'frame %d %d'  message and when
// data-driven expects 'Sequencer_%d: ...' messages.  We install the
// handler for the 'frame' message here and then call the super class
// to install the 'Sequencer_%d:' handler.
//
void SequencerNode::updateModuleMessageProtocol(DXPacketIF *pif)
{

    pif->setHandler(DXPacketIF::INTERRUPT,
                  SequencerNode::ProcessFrameInterrupt,
                  (void*)this,
                  "frame ");

    this->ShadowedOutputNode::updateModuleMessageProtocol(pif);
}

//
// Handler for 'frame %d %d' messages.
// If the Sequencer is data-driven and has received a 'frame=%d:%d' message
// then we ignore the next 'frame %d %d' message.
//
void SequencerNode::ProcessFrameInterrupt(void *clientData, int id, void *p)
{
    char *line = (char *)p;
    char buffer[10];
    int  frame, next_frame, parsed;
    Widget vcr = NUL(Widget);
    SequencerNode* sequencer = (SequencerNode*)clientData; 

    if (sequencer->ignoreFirstFrameMsg == TRUE) {
    	sequencer->ignoreFirstFrameMsg = FALSE;
	return;
    }

    vcr = sequencer->getVCRWidget();

    parsed = sscanf(line,"%s %d %d", buffer, &frame, &next_frame);
    ASSERT(parsed == 3);

    sequencer->current         = frame;
    sequencer->next            = next_frame;
    sequencer->current_defined = TRUE;

    if (vcr)
          XtVaSetValues(vcr,
                        XmNcurrent,        sequencer->current,
                        XmNcurrentVisible, sequencer->current_defined,
                        XmNnext,           sequencer->next,
                        NULL);
}

//
// Determine if this node is of the given class.
//
boolean SequencerNode::isA(Symbol classname)
{
    Symbol s = theSymbolManager->registerSymbol(ClassSequencerNode);
    if (s == classname)
	return TRUE;
    else
	return this->ShadowedOutputNode::isA(classname);
}

//
// Make sure the value of the parameter that holds a list of ui attributes
// (i.e. min/max/incr) is up to date in the executive.
// Always send it since the input attribute is cache:0.
//
void SequencerNode::updateAttributes()
{
     char val[128];

     //
     // Using isVisualNotificationDeferred() is somewhat inconsistent,
     // but we're using it here to avoid sending multiple messages back to
     // the executive when inside this->handleModuleMessage() during which
     // time we know that visual notification is deferred.
     // This of course means that we must call this again when notification
     // is undeferred (i.e. in reflectStateChange);
     //
     if (!this->isVisualNotificationDeferred()) {
	 sprintf(val,"{ %d %d %d %d %d %d }", 
				  this->getMinimumValue(), 
				  this->getMaximumValue(),
				  this->getDeltaValue(),
				  this->getStartValue(),
				  this->getStopValue(),
				  this->wasExecuted ? 1 : 0);
	 this->setInputValueQuietly(ATTR_PARAM_NUM,val, 
				DXType::IntegerListType);
    }
}
boolean SequencerNode::canSwitchNetwork(Network *from, Network *to)
{
    if (to->sequencer != NULL) {
	WarningMessage("Attempt to add a second Sequencer to network ignored.");
	return FALSE; 
    }

    return TRUE;
}
//
// Return TRUE if this node has state that will be saved in a .cfg file.
//
boolean SequencerNode::hasCfgState()
{
    return TRUE;
}


int SequencerNode::getMessageIdParamNumber() { return ID_PARAM_NUM; }

boolean SequencerNode::isStartup()
{
    if (!this->seq_window) return this->startup;
    return this->seq_window->isStartup();
}

boolean SequencerNode::printInputAsJava(int input)
{
    boolean retval = FALSE;
    switch (input) {
	case MIN_PARAM_NUM:
	case MAX_PARAM_NUM:
	case DELTA_PARAM_NUM:
	    retval = TRUE;
	    break;
	default:
	    break;
    }
    return retval;
}

const char* SequencerNode::getJavaInputValueString(int input)
{
    const char* retval = NUL(char*);
    static char tbuf[32];
    switch (input) {
	case MIN_PARAM_NUM:
	    sprintf (tbuf, "%d", this->getMinimumValue());
	    retval = tbuf;
	    break;
	case MAX_PARAM_NUM:
	    sprintf (tbuf, "%d", this->getMaximumValue());
	    retval = tbuf;
	    break;
	case DELTA_PARAM_NUM:
	    sprintf (tbuf, "%d", this->getDeltaValue());
	    retval = tbuf;
	    break;
	default:
	    retval = this->ShadowedOutputNode::getInputValueString(input);
	    break;
    }
    return retval;
}
