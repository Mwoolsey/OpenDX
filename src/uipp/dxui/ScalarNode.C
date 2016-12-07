/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"





#include <stdio.h>
#ifdef ABS_IN_MATH_H
# define abs __Dont_define_abs
#endif
#include <math.h>
#ifdef ABS_IN_MATH_H
# undef abs
#endif

#include "lex.h"
#include "ScalarNode.h"
#include "ScalarInstance.h"
#include "ErrorDialogManager.h"
#include "Parameter.h"
#include "AttributeParameter.h"
#include "Network.h"
#include "ListIterator.h"
#include "WarningDialogManager.h"


//
// The following define the mapping of parameter indices to parameter
// functionality in the module that is used to 'data-drive' the
// ScalarNode class and Nodes derived from it.
// These MUST match the entries in the ui.mdf file for the interactors
// that expect to be implemented by the ScalarInteractor class.
//
#define ID_PARAM_NUM            1       // Id used for UI messages
#define DATA_PARAM_NUM          2       // The input object
#define CVAL_PARAM_NUM          3       // Current Value
#define REFRESH_PARAM_NUM       4       // Does the module refresh
#define MIN_PARAM_NUM           5       // The current minimum
#define MAX_PARAM_NUM           6       // The current maximum
#define INCR_PARAM_NUM          7       // The current increment
#define INCRMETHOD_PARAM_NUM	8	// Method used to interpret incr.
#define DECIMALS_PARAM_NUM      9       // The number of decimals to display
#define DFLTVAL_PARAM_NUM       10      // The default output indicator 
#define LABEL_PARAM_NUM         11      // The label

// The total number of inputs as defined in the mdf file.
#define EXPECTED_SCALAR_INPUTS		LABEL_PARAM_NUM


#define DEFAULT_VALUE	0.0
#define DEFAULT_INCR	1.0
#define DEFAULT_MAX	1.0e6
#define DEFAULT_MIN	-1.0e6
#define DEFAULT_SCALAR_DECIMALS 5

#define DEFAULT_VALUE_STR	      " 0 "
#define DEFAULT_INCR_STR	      " 1 "
#define DEFAULT_MAX_STR		"  100000 "
#define DEFAULT_MIN_STR	 	" -100000 "	
#define DEFAULT_SCALAR_DECIMALS_STR   " 5 "

//
// Create the component 
//
ScalarNode::ScalarNode(NodeDefinition *nd, 
			Network *net, int instance, 
			boolean isVector,
			int dimensions) :
                        InteractorNode(nd, net, instance)
{
    ASSERT(dimensions > 0);
    this->isContinuousUpdate = FALSE;
    this->vectorType = isVector;
    this->numComponents = dimensions;
    this->rangeCheckDeferrals = 0;
    this->needsRangeCheck = FALSE;

    if (!this->verifyInputCount())
	return;

    //
    // The attribute is an absolute increment and the parameter value
    // is a relative increment (between 0..1).  They have the same types
    // so turn off value synchronization in the increment parameter (which
    // is where the increment value is stored).
    //
    AttributeParameter *ap = (AttributeParameter*)
			this->getInputParameter(INCR_PARAM_NUM);
    ASSERT(ap);
    ap->setSyncOnTypeMatch(FALSE);
}

boolean ScalarNode::hasDynamicDimensionality(boolean ignoreDataDriven)
{
    return this->isVectorType() && 
	   (ignoreDataDriven || !this->isDataDriven());
}
//
// Change the dimensionality of a Vector interactor. 
//
boolean ScalarNode::doDimensionalityChange(int new_dim)
{
    ScalarInstance *si;
    int old_dim = this->getComponentCount();

    ASSERT(this->isVectorType());

    if (new_dim == old_dim)
	return TRUE;

    this->numComponents = new_dim; 
    //
    // Delete the interactors, so that they don't get updated when setting
    // the output value.  If we don't do this, we can get references to 
    // non-existent vector components. 
    //
    ListIterator iterator(this->instanceList);
    while ( (si = (ScalarInstance*)iterator.getNext()) )  
	si->uncreateInteractor();
    

    //
    // Adjust the and output values first. 
    //
    this->adjustOutputDimensions(old_dim, new_dim);

    //
    // Adjust all the attribute values. 
    //
    this->adjustAttributeDimensions(old_dim,new_dim);
 
    //
    // Now have all the instances adjust themselves
    //
    iterator.setList(this->instanceList);
    while ( (si = (ScalarInstance*)iterator.getNext()) )  {
	if (!si->handleNewDimensionality())
	    return FALSE;	
    }
    return TRUE;
  
}
boolean ScalarNode::adjustOutputDimensions(int old_dim, int new_dim)
{
    const char *oldval = this->getOutputValueString(1);
    char *newval = DXValue::AdjustVectorDimensions(oldval, 
			new_dim, DEFAULT_VALUE, this->isIntegerTypeComponent());
    if (!newval)
	return FALSE;

    this->setOutputValue(1,newval,DXType::VectorType,TRUE);
    delete newval;
    return TRUE;
}

boolean ScalarNode::adjustAttributeDimensions(int old_dim, int new_dim)
{

    boolean is_int = this->isIntegerTypeComponent();
    const char *v;
    char *newval;

    //
    // Adjust the min attribute
    //
    v = this->getInputAttributeParameterValue(MIN_PARAM_NUM);		
    newval = DXValue::AdjustVectorDimensions(v, new_dim, DEFAULT_MIN,is_int);
    this->setMinimumAttribute(newval);						
    delete newval;

    //
    // Adjust the max attribute
    //
    v = this->getInputAttributeParameterValue(MAX_PARAM_NUM);		
    newval = DXValue::AdjustVectorDimensions(v, new_dim, DEFAULT_MAX, is_int);
    this->setMaximumAttribute(newval);						
    delete newval;

    //
    // Adjust the delta attribute
    //
    v = this->getInputAttributeParameterValue(INCR_PARAM_NUM);		
    newval = DXValue::AdjustVectorDimensions(v, new_dim, DEFAULT_INCR, is_int);
    this->setDeltaAttribute(newval);						
    delete newval;

    //
    // Adjust the decimals attribute
    //
    double decimals;
    if (is_int)
	decimals = 0;
    else
	decimals = DEFAULT_SCALAR_DECIMALS;
    v = this->getInputAttributeParameterValue(DECIMALS_PARAM_NUM);		
    newval = DXValue::AdjustVectorDimensions(v, new_dim, decimals, TRUE);
    this->setDecimalsAttribute(newval);						
    delete newval;

    return TRUE;
}
//
// Set the interactor's default attributes.
//
boolean ScalarNode::setDefaultAttributes()
{
    const char *id;
    const char *min=NULL, *max=NULL, *incr=NULL, *decimals=NULL;
    boolean r;
 
// FIXME: these strings should be build from DEFAULT_*_STR
    switch (this->numComponents) {
	case 1:
	    if (this->isVectorType()) {
		min     = "[ -1000000 ]";
		max     = "[ 1000000 ]";
		incr    = "[ 1 ]";
		decimals = (this->isIntegerTypeComponent() ? "[0]" : "[5]");
	    } else {
		min     = "-1000000";
		max     = "1000000";
		incr    = "1";
		decimals = (this->isIntegerTypeComponent() ? "0" : "5");
	    }
	    break;
	case 2:
	    min     = "[-1000000 -1000000]";
	    max     = "[ 1000000  1000000]";
	    incr    = "[ 1        1]";
	    decimals= "[ 5        5]";
	    break;
	case 3:
	    min     = "[-1000000 -1000000 -1000000]";
	    max     = "[ 1000000  1000000  1000000]";
	    incr    = "[       1        1        1]";
	    decimals= "[       5        5        5]";
	    break;
	default:
	    ASSERT(0);
    }

    id = this->getModuleMessageIdString();

    if ((this->setInputValue(ID_PARAM_NUM,id, DXType::UndefinedType,FALSE) 
				== DXType::UndefinedType) 
	||
	!this->initMinimumAttribute(min)
	||
    	!this->initMaximumAttribute(max)
	||
    	!this->initDeltaAttribute(incr)
	||
    	!this->initDecimalsAttribute(decimals)
	) {
         r = FALSE;
    } else
	r = TRUE;

    return r;
}

//
// Make sure the number of inputs is the number expected. 
//
boolean ScalarNode::verifyInputCount()
{
    if (this->getInputCount() != EXPECTED_SCALAR_INPUTS) {
        ErrorMessage( 
	   "Expected %d inputs for %s interactor, please check the mdf file.\n",
			EXPECTED_SCALAR_INPUTS,
			this->getNameString());
	return FALSE;
    }
    return TRUE; 
}
//
// Called after allocation is complete.
// The work done here is to assigned default values to the InteractorNode inputs
// so that we can use them later when setting the attributes for the
// Interactor. 
//
boolean ScalarNode::initialize()
{

    if (!this->verifyInputCount())
	return FALSE;

// FIXME: these strings should be build from DEFAULT_*_STR
    const char *value;
    switch (this->numComponents) {
    	case 1: if (this->isVectorType())
		    value = "[ 0 ]"; 
		else
		    value = "0"; 
		break;
	case 2: value = "[ 0 0 ]"; break;
	case 3: value = "[ 0 0 0 ]"; break;
	default: return FALSE;
    }

    if (!this->setDefaultAttributes() 
	||
	(this->setOutputValue(1,value, DXType::UndefinedType,FALSE) == 
				DXType::UndefinedType)
 	){	
        ErrorMessage(
      "Error setting default attributes for %s interactor, check ui.mdf\n",
		this->getNameString());
        return FALSE;
    }
    //
    // Make the shadows defaulting (even though we have a current output)
    // so that the executive module can tell when it is executing a just
    // placed module and one that is read in from a .net or .cfg file.
    // When read in, the output will be set again which should make the
    // corresponding shadowing input be non-defaulting.
    //
    this->setShadowingInputsDefaulting();

    return TRUE;
}


// The messages we parse can contain one or more of the following...
//
// 'min=%g' 'max=%g' 'delta=%g' 'value=%g' 'decimals=%d'
//
//  or one or more of the following...
//
// 'min=[%g %g %g]' 'max=[%g %g %g]' 'delta=[%g %g %g]' 'value=[%g %g %g]'
// 'decimals=[%g %g %g]'
//
// If any input or output values are to be changed, don't send them
// because the module backing up the interactor will have just executed
// and if the UI is in 'execute on change' mode then there will be an
// extra execution.
//
// Returns the number of attributes parsed.
//
int ScalarNode::handleInteractorMsgInfo(const char *line)
{
    int values;
    if (this->isVectorType())
	values = this->handleVectorMsgInfo(line);
    else 
	values = this->handleScalarMsgInfo(line);

    //
    // Handle the 'method="%s"' part of the message.
    // We only need to look for the message if we don't already have it.
    //
	if (this->isInputConnected(INCRMETHOD_PARAM_NUM)) {
		char *p, buf[128];
		if ((p = strstr((char*)line,"method="))	&& 
			FindDelimitedString(p,'"','"', buf)) {
				values++;
				this->setInputAttributeFromServer(INCRMETHOD_PARAM_NUM,buf, 
					DXType::StringType);
		}
	}


    //
    // Make sure that the min or max sent back from the executive is 
    // consistent.  If the user has not connected the data tab and has
    // only one of min and max set then it is possible that the value sent
    // back by the module conflicts with the other value stored in the ui. 
    //
	if (!this->isInputDefaulting(DATA_PARAM_NUM))  {
		boolean min_dflting = this->isInputDefaulting(MIN_PARAM_NUM);
		boolean max_dflting = this->isInputDefaulting(MAX_PARAM_NUM);
		if ((min_dflting && !max_dflting) || (!min_dflting &&  max_dflting)) {
			int i, comps = this->getComponentCount();
			boolean issue_warning = FALSE;	
			for (i=1 ; i<=comps  ; i++) {
				double minval = this->getComponentMinimum(i);
				double maxval = this->getComponentMaximum(i);
				if (minval > maxval) {
					issue_warning = TRUE;	
					if (min_dflting)
						this->setComponentMinimum(i,maxval);
					else
						this->setComponentMaximum(i,minval);
				}
			}
			if (issue_warning) {
				char *setattr, *set;
				if (min_dflting) {
					set     = "maximum";
					setattr = "minimum";
				} else  {
					set     = "minimum";
					setattr = "maximum";
				}
				WarningMessage("%s value provided to %s conflicts "
					"with %s value set with 'Set Attributes' dialog"
					"...adjusting %s.", 
					set, this->getNameString(),
					setattr,setattr);
			}
		}
	}


    return values;
    
}
// The messages we parse can contain one or more of the following...
//
// 'min=%g' 'max=%g' 'delta=%g' 'value=%g' 'decimals=%d'
//
// If any input or output values are to be changed, don't send them
// because the module backing up the interactor will have just executed
// and if the UI is in 'execute on change' mode then there will be an
// extra execution.
//
// Returns the number of attributes parsed.
//
int ScalarNode::handleScalarMsgInfo(const char *line)
{
	int index, values = 0;
	char *p, *buf = NULL;

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
			this->setInputAttributeFromServer(MIN_PARAM_NUM,buf,
				DXType::UndefinedType);
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
			this->setInputAttributeFromServer(MAX_PARAM_NUM,buf,
				DXType::UndefinedType);
		}
		delete buf;
	}
	//
	// Handle the 'delta=%g' part of the message.
	// Since the attribute and parameter value have the same type, but
	// different meanings we only set the attribute value as received from
	// the exec.
	//
	if ( (p = strstr((char*)line,"delta=")) ) {
		values++;
		while (*p != '=') p++;
		p++;
		buf = DuplicateString(p);
		index = 0;
		if (IsScalar(buf, index)) {
			buf[index] = '\0'; 
			this->setInputAttributeFromServer(INCR_PARAM_NUM,buf,
				DXType::UndefinedType);
		}
		delete buf;
	}
	//
	// Handle the 'decimals=%d' part of the message.
	//
	if ( (p = strstr((char*)line,"decimals=")) ) {
		values++;
		while (*p != '=') p++;
		p++;
		buf = DuplicateString(p);
		index = 0;
		if (IsScalar(buf, index)) {
			buf[index] = '\0'; 
			this->setInputAttributeFromServer(DECIMALS_PARAM_NUM,buf,
				DXType::UndefinedType);
		}
		delete buf;
	}
	//
	// Handle the 'value=%g' part of the message.
	//
	if ( (p = strstr((char*)line,"value=")) ) {
		values++;
		while (*p != '=') p++;
		p++;
		buf = DuplicateString(p);
		index = 0;
		if (IsScalar(buf, index)) {
			buf[index] = '\0'; 
			this->setShadowedOutputSentFromServer(1,buf,
				DXType::UndefinedType);
		}
		delete buf;
	}
	return values;

}

//
// Called when a message is received from the executive to parse class
// specific message information. 
//
// The messages we handle can contain one or more of the following...
//
// 'dim=%d'
// 'min=[%g %g %g]' 'max=[%g %g %g]' 'delta=[%g %g %g]' 'value=[%g %g %g]'
// 'decimals=[%g %g %g]'
//
// If any input or output values are to be changed, don't send them
// because the module backing up the interactor will have just executed
// and if the UI is in 'execute on change' mode then there will be an
// extra execution.
//
// Returns the number of attributes parsed.
//
int ScalarNode::handleVectorMsgInfo(const char *line)
{
    char *p;
    int  values = 0;
    char buf[256];

    //
    // Handle the 'dim=%d' part of the message.
    // This message must be handled first, because changing the dimensionality
    // resets all the interactor values which can be reset by subsequent 
    // assignments in the current message. 
    // We don't need to do a 'values++' since the info is handled locally 
    // with this->changeDimensionality().
    //
    if ( (p = strstr((char*)line,"dim=")) ) {
	while (*p != '=') p++;
	p++;
	int dim = atoi(p);
	if (this->isVectorType() && (this->getComponentCount() != dim))
	    this->changeDimensionality(dim);
    }
    //
    // Handle the 'min=[%g...]' part of the message.
    //
    if ( (p = strstr((char*)line,"min=")) ) {
	values++;
	while (*p != '=') p++;
	p++;
	if (FindDelimitedString(p,'[',']', buf)) {
	    this->setInputAttributeFromServer(MIN_PARAM_NUM,buf,
						DXType::UndefinedType);
	}
	
    }
    //
    // Handle the 'max=[%g...]' part of the message.
    //
    if ( (p = strstr((char*)line,"max=")) ) {
	values++;
	while (*p != '=') p++;
	p++;
	if (FindDelimitedString(p,'[',']', buf)) {
	    this->setInputAttributeFromServer(MAX_PARAM_NUM,buf,
						DXType::UndefinedType);
	}
    }
    //
    // Handle the 'delta=[%g...]' part of the message.
    // Since the attribute and parameter value have the same type, but
    // different meanings we only set the attribute value as received from
    // the exec.
    //
    if ( (p = strstr((char*)line,"delta=")) ) {
	values++;
	while (*p != '=') p++;
	p++;
	if (FindDelimitedString(p,'[',']', buf)) {
	    this->setInputAttributeFromServer(INCR_PARAM_NUM,buf,
						DXType::UndefinedType);
	}
    }
    //
    // Handle the 'decimals=[%g...]' part of the message.
    //
    if ( (p = strstr((char*)line,"decimals=")) ) {
	values++;
	while (*p != '=') p++;
	p++;
	if (FindDelimitedString(p,'[',']', buf)) {
	    this->setInputAttributeFromServer(DECIMALS_PARAM_NUM,buf,
						DXType::UndefinedType);
	}
    }
    //
    // Handle the 'value=[%g...]' part of the message.
    //
    if ( (p = strstr((char*)line,"value=")) ) {
	values++;
	while (*p != '=') p++;
	p++;
	if (FindDelimitedString(p,'[',']', buf)) {
	    this->setShadowedOutputSentFromServer(1,buf,
						DXType::UndefinedType);
	}
    }
    return values;
}

boolean ScalarNode::cfgParseComment(const char *comment,
                                const char *filename, int lineno)
{
    //
    // Don't do range checking until after the value AND the component's
    // min/max have been set (see cfgParseComponentComment()).
    //
    if (strncmp(" interactor",comment,11)) 
	this->deferRangeChecking();

    return this->cfgParseComponentComment(comment, filename,lineno)     ||
           this->cfgParseInstanceComment(comment, filename, lineno) ||
           this->cfgParseLocalIncrementComment(comment, filename, lineno) ||
           this->cfgParseLocalContinuousComment(comment, filename, lineno) ||
           this->InteractorNode::cfgParseComment(comment, filename, lineno);
}

//
// Print the '// interactor...' comment  to the file.
// We override the parent's method so that we can print num_components
// correctly.
//
boolean ScalarNode::cfgPrintInteractorComment(FILE *f)
{
    return fprintf(f,
                "// interactor %s[%d]: num_components = %d, value = %s\n",
                this->getNameString(),
                this->getInstanceNumber(),
                this->getComponentCount(),
                this->getOutputValueString(1)) >= 0;
}

//
// Print auxiliary info for this interactor, which includes all global 
// information about each component.
//
boolean ScalarNode::cfgPrintInteractorAuxInfo(FILE *f)
{
    int i, ncomp = this->getComponentCount();

    for (i=1 ; i<=ncomp ; i++ ) {
    	if (fprintf(f,
            "// component[%d]: minimum = %g, maximum = %g, "
	    "global increment = %g, decimal = %d, global continuous = %d\n", 
		i-1,	// File uses 0 based indexing
		this->getComponentMinimum(i),
		this->getComponentMaximum(i),
		this->getComponentDelta(i),
		this->getComponentDecimals(i),
                this->isContinuous()) < 0)
		return FALSE;
    }
    return TRUE;
}

//
// Print auxiliary info for the interactor instance,  which include all local
// information about each component.
//
boolean ScalarNode::cfgPrintInstanceAuxInfo(FILE *f, 
					InteractorInstance *ii)
{
    int i, ncomp = this->getComponentCount();
    ScalarInstance *si = (ScalarInstance *)ii;

    if (fprintf(f,
            "// local continuous: value = %d, mode = %s\n",
		(si->getLocalContinuous() ? 1 : 0),
		(si->usingGlobalContinuous() ? "global" : "local") ) < 0)
	return FALSE;

    for (i=1 ; i<=ncomp ; i++ ) {
        if (fprintf(f,
            "// local increment[%d]: value = %g, mode = %s\n",
		i-1,	// File uses 0 based indexing
		si->getLocalDelta(i),
		(si->isLocalDelta(i) ? "local" : "global") ) < 0)
	    return FALSE;
    }
    return TRUE;

}
//
// This is the same as for the super class except that when we get a new 
// instance we build the list of local attributes (one LocalAttributes 
// for each component). 
//
boolean ScalarNode::cfgParseInstanceComment(const char *comment,
                                const char *filename, int lineno)
{
    if (!this->InteractorNode::cfgParseInstanceComment(comment,filename,lineno))
	return FALSE;

#if 0
    int i, instance, components;
    //
    // Get the instance added above.
    //
    instance = this->getInstanceCount();	// The just added instance # 
    ScalarInstance *si = (ScalarInstance*)this->getInstance(instance);

    //
    // Copy the global attributes to the local.
    //
    for (i=1 ; i<=this->getComponentCount() ; i++) {
	double   dval;
	int	 ival = this->getComponentDecimals(i);
	dval = this->getComponentMinimum(i);
	si->setLocalMinimum(i, dval);
	dval = this->getComponentMaximum(i);
	si->setLocalMaximum(i, dval);
	dval = this->getComponentDelta(i);
	si->setLocalDelta(i, dval);
	ival = this->getComponentDecimals(i);
	si->setLocalDecimals(i, ival);
    } 
#endif
    
    return TRUE;

}
boolean ScalarNode::cfgParseComponentComment(const char *comment,
                                const char *filename, int lineno)
{
    int      items_parsed;
    double   minimum;
    double   maximum;
    double   delta;
    int      decimal;
    int      component_num;
    int      continuous;

    if (strncmp(comment," component",10))
        return FALSE;

    items_parsed = sscanf
                (comment,
                 " component[%d]: minimum = %lf, maximum = %lf, global increment = %lf, decimal = %d, global continuous = %d",
                 &component_num,
                 &minimum,
                 &maximum,
                 &delta,
                 &decimal,
                 &continuous);

    if (items_parsed != 5 && items_parsed != 6)
    {
        ErrorMessage("Bad 'component' comment (file %s, line %d)",
                                        filename,lineno);
        return FALSE;
    }

    component_num++;	// 1 based indexing.

#if 0
    Parameter *p = this->getOutputParameter(1);
    Type t = p->getValueType();

    //
    // The following is done most for lists, which when the list is empty
    // have "NULL" value.  In this case we want to set the type to the
    // default type.
    //
    if ((t == DXType::ObjectType) || EqualString("NULL",p->getValueString())) {
	t = p->getDefaultType();
	this->clearOutputValue(1);
	p->setValue(NULL,t);
    }
#endif

    this->setComponentMinimum(component_num,minimum);
    this->setComponentMaximum(component_num,maximum);
    this->setComponentDelta(component_num,delta);
    this->setComponentDecimals(component_num,decimal);

    if ((items_parsed == 6) && continuous)
        this->setContinuous();
    else
        this->clrContinuous();

    //
    // Enable range checking of the output value after the last component
    // min/max has been set (see cfgParseComments() for matching 
    // deferComponentCount()).
    //
    if (component_num == this->getComponentCount())
	this->undeferRangeChecking();

    return TRUE;
}

//
// Get a an ScalarInstance instead of an InteractorInstance.
//
InteractorInstance *ScalarNode::newInteractorInstance()
{
    ScalarInstance *si = new ScalarInstance(this);

    return (InteractorInstance*)si;
}
//
// G/Set the 'minimum' attribute for the given component 
//
boolean ScalarNode::setAllComponentRanges(double *min, double *max)
{
    this->deferRangeChecking();

    int i, ncomps = this->getComponentCount();
    for (i=1 ; i<=ncomps ; i++) {
	if (min)
	    this->setComponentMinimum(i,min[i-1]);
	if (max)
	    this->setComponentMaximum(i,max[i-1]);
    }

    this->undeferRangeChecking();
    return TRUE;
}
boolean ScalarNode::initMinimumAttribute(const char *val)
{
    AttributeParameter *p = (AttributeParameter*) this->getInputParameter(
						MIN_PARAM_NUM);
    return p->initAttributeValue(val);
}
boolean ScalarNode::setMinimumAttribute(const char *val)
{
    AttributeParameter *p = (AttributeParameter*) this->getInputParameter(
						MIN_PARAM_NUM);
    return p->setAttributeValue(val);
}
boolean ScalarNode::setComponentMinimum(int component, double min)
{
    AttributeParameter *p = (AttributeParameter*) this->getInputParameter(
						MIN_PARAM_NUM);

    double oldmin = p->getAttributeComponentValue(component);

    if (!p->setAttributeComponentValue(component, min)) 
	return FALSE;

    if (min > oldmin)
	this->rangeCheckComponentValue(component, 
			    min, this->getComponentMaximum(component));
    return TRUE;

}
double ScalarNode::getComponentMinimum(int component)
{
    AttributeParameter *p = (AttributeParameter*) this->getInputParameter(
						MIN_PARAM_NUM);

    if (p->getAttributeComponentCount() < component)
	return DEFAULT_MIN;
    else
        return p->getAttributeComponentValue(component);
}
//
// G/Set the 'maximum' attribute for the given component 
//
boolean ScalarNode::initMaximumAttribute(const char *val)
{
    AttributeParameter *p = (AttributeParameter*) this->getInputParameter(
						MAX_PARAM_NUM);
    return p->initAttributeValue(val);
}
boolean ScalarNode::setMaximumAttribute(const char *val)
{
    AttributeParameter *p = (AttributeParameter*) this->getInputParameter(
						MAX_PARAM_NUM);
    return p->setAttributeValue(val);
}
boolean ScalarNode::setComponentMaximum(int component, double max)
{
    AttributeParameter *p = (AttributeParameter*) this->getInputParameter(
							MAX_PARAM_NUM);

    double oldmax = p->getAttributeComponentValue(component);

    if (!p->setAttributeComponentValue(component, max))
	return FALSE;

    if (max < oldmax)
	this->rangeCheckComponentValue(component, 
			this->getComponentMinimum(component), max);
    return TRUE;
}
double ScalarNode::getComponentMaximum(int component)
{
    AttributeParameter *p = (AttributeParameter*) this->getInputParameter(
						MAX_PARAM_NUM);
    if (p->getAttributeComponentCount() < component)
	return DEFAULT_MAX;
    else
        return p->getAttributeComponentValue(component);
}

//
// G/Set the 'delta' attribute for the given component
//
boolean ScalarNode::initDeltaAttribute(const char *val)
{
    AttributeParameter *p = (AttributeParameter*) this->getInputParameter(
						INCR_PARAM_NUM);
    return p->initAttributeValue(val);
}
boolean ScalarNode::setDeltaAttribute(const char *val)
{
    AttributeParameter *p = (AttributeParameter*) this->getInputParameter(
						INCR_PARAM_NUM);
    return p->setAttributeValue(val);
}
boolean ScalarNode::setComponentDelta(int component, double delta)
{
    AttributeParameter *p = (AttributeParameter*) this->getInputParameter(
						INCR_PARAM_NUM);
    return p->setAttributeComponentValue(component, delta);
}
double ScalarNode::getComponentDelta(int component)
{
    AttributeParameter *p = (AttributeParameter*) this->getInputParameter(
						INCR_PARAM_NUM);
    if (p->getAttributeComponentCount() < component)
	return DEFAULT_INCR;
    else
        return p->getAttributeComponentValue(component);
}
//
// G/Set the 'decimal places' attribute for the given component 
//
boolean ScalarNode::initDecimalsAttribute(const char *val)
{
    AttributeParameter *p = (AttributeParameter*) this->getInputParameter(
						DECIMALS_PARAM_NUM);
    return p->initAttributeValue(val);
}
boolean ScalarNode::setDecimalsAttribute(const char *val)
{
    AttributeParameter *p = (AttributeParameter*) this->getInputParameter(
						DECIMALS_PARAM_NUM);
    return p->setAttributeValue(val);
}
boolean ScalarNode::setComponentDecimals(int component, int decimals)
{
    AttributeParameter *p = (AttributeParameter*) this->getInputParameter(
						DECIMALS_PARAM_NUM);
    return p->setAttributeComponentValue(component, decimals);
}
int ScalarNode::getComponentDecimals(int component)
{
    AttributeParameter *p = (AttributeParameter*) this->getInputParameter(
						DECIMALS_PARAM_NUM);
    if (p->getAttributeComponentCount() < component) {
	if (this->isIntegerTypeComponent())
	    return 0;
	else 
	    return DEFAULT_SCALAR_DECIMALS;
    } else
        return (int) p->getAttributeComponentValue(component);
}
boolean ScalarNode::setComponentValue(int component, double value)
{
    Parameter *out = this->getOutputParameter(1);
    if (!out->setComponentValue(component, value))
	return FALSE;
    const char *v = this->getOutputValueString(1);
    // This forces update of the shadowing input.
    return this->setOutputValue(1,v,DXType::UndefinedType,FALSE);
}
double ScalarNode::getComponentValue(int component)
{
    Parameter *p = this->getOutputParameter(1);
    ASSERT(p->getComponentCount() >= component);
    return p->getComponentValue(component);
}

//
// Call doRangeCheckComponentValue() if range checking is not
// deferred. 
//
void ScalarNode::rangeCheckComponentValue(int component, double min, double max)
{
    if (!this->isRangeCheckingDeferred()) {
        this->needsRangeCheck = FALSE;
        this->doRangeCheckComponentValue(component, min,max);
    } else {
        this->needsRangeCheck = TRUE;
    }
}

//
// Do what ever is necessary when the given component of the output
// value is out of range as indicated by the limits.  Typically, this just
// means resetting the current output value that is associated with the
// node.  ScalarListNodes however, have the current (non-output) value
// associated with the interactor instance. Therefore, ScalarListNodes,
// re-implement this to reset the component values of all their instances.
// If 'component' is less than 0, then min/max are ignored and all
// components are checked with the current min/max values.
//
void ScalarNode::doRangeCheckComponentValue(int component, 
				double min, double max)
{
    double val;

    if (component < 0) {
	//
	// Check all components of current output value against current
	// component min/max values (ignore input min/max).
	//
	int i, ncomp = this->getComponentCount();
	double *mins = new double[ncomp];
	double *maxs = new double[ncomp];
	Type output_type = this->getOutputSetValueType(1);
	char *newval;

	for (i=1 ; i<=ncomp ; i++) {
	    mins[i-1] = this->getComponentMinimum(i);
	    maxs[i-1] = this->getComponentMaximum(i);
	}
	if (DXValue::ClampVSIValue(this->getOutputValueString(1),
			  output_type, 
		 	  mins,maxs,&newval)) {
	    ASSERT(newval);
	    this->setOutputValue(1,newval,output_type,TRUE);
	    delete newval;
   	}
	delete[] mins;
	delete[] maxs;
    } else {
	//
	// Just do a single component.
	// Instead of doing this we could just always do the above, but
	// this is much faster.
	// Eventually, this causes a setOutputValue() just as above.
	//
        val = this->getComponentValue(component);
	if (val < min) {
	    this->setComponentValue(component,min);
	} else if (val > max) {
	    this->setComponentValue(component,max);
	}
    } 

}
//
// Determine if the given component is an integer type.
// We ignore component and assume all components have the same type.
//
boolean ScalarNode::isIntegerTypeComponent()
{
    // FIXME: is there a better way to do this.
    if (!strncmp("Integer",this->getNameString(),7))
	return TRUE;
    else
	return FALSE;
}

boolean ScalarNode::cfgParseLocalIncrementComment(
		const char *comment, const char *filename, int lineno)

{
    int            items_parsed;
    int            component_num;
    double         double_step;
    char           mode[32];

    ASSERT(comment);

    if (strncmp(comment," local increment",16))
        return FALSE;

#if 0
    is_int = uiuEqualString(_interactor_name, "Integer");
#endif
	
    items_parsed =
	    sscanf(comment,
		   " local increment[%d]: value = %lf, mode = %s",
		   &component_num, // Ignoring component number (always 0?)
		   &double_step,
		   mode);

    component_num++;	// 1 based indexing
    if (component_num < 1 || component_num > this->getComponentCount()) {
	ErrorMessage(
  "'local increment' comment references undefined component (file %s, line %d)",
				filename, lineno);
	return FALSE;
    }
	

    if (items_parsed != 3)
    {
#if 1
	ErrorMessage("Bad 'local increment' comment (file %s, line %d)",
				filename, lineno);
	return FALSE;
    }
#else
	uiuModelessErrorMessageDialog
	    (_parse_widget,
	     "#10001",
	     "local increment",
	     _parse_file,
	     yylineno);
	_error_occurred = TRUE;
	return;
    }
	
    if (_interactor_index < 0)
    {
	uiuModelessErrorMessageDialog
	    (_parse_widget,
	     "#10011", "local increment", "interactor", _parse_file, yylineno);
	_error_occurred = TRUE;
	return;
    }

    interactor = _network->interactor + _interactor_index;

    if (NOT interactor->in_use)
    {
	uiuModelessErrorMessageDialog
	    (_parse_widget,
	     "#10011",
	     "local increment",
	     "interactor",
	     _parse_file,
	     yylineno);
	_error_occurred = TRUE;
	return;
    }
#endif

#if 1
    int instance = this->getInstanceCount();
    ScalarInstance *si = (ScalarInstance*)this->getInstance(instance);
    if (!si) {
 	ErrorMessage("'local increment' comment out of order (file %s, line%d)",
			filename, lineno);
	return FALSE;
    }
    si->useLocalDelta(component_num, double_step);
    if (!EqualString(mode,"local")) 
       si->clrLocalDelta(component_num);

#else
    if (is_int)
    {
	interactor->increment[component_num].integer =
	    interactor->restore_increment[component_num].integer =
		int_step;
    }
    else
    {
	interactor->increment[component_num].real =
	    interactor->restore_increment[component_num].real =
		double_step;
    }

    interactor->local_increment[component_num] = 
	interactor->restore_local_increment[component_num] = 
	    uiuEqualString(mode, "local");
#endif
   return TRUE;

}

boolean ScalarNode::cfgParseLocalContinuousComment(
		const char *comment, const char *filename, int lineno)
{
    int            items_parsed;
    int	           continuous;
    char           mode[32];

    ASSERT(comment);

    if (strncmp(comment," local continuous",17))
        return FALSE;

    items_parsed =
	sscanf(comment,
	       " local continuous: value = %d, mode = %s",
	       &continuous,
	       mode);

    /*
     * Backwards compatibility...
     */
    if (items_parsed != 2)
    {
	items_parsed =
	    sscanf(comment,
		   " local continuous[0]: value = %d, mode = %s",
		   &continuous,
		   mode);
    }

    if (items_parsed != 2)
    {
#if 1
	ErrorMessage("Bad 'local continuous' comment (file %s, line %d)",
			filename, lineno);
	return FALSE;
#else
	uiuModelessErrorMessageDialog
	    (_parse_widget,
	     "#10001",
	     "local continuous",
	     _parse_file,
	     yylineno);
	_error_occurred = TRUE;
	return;
#endif
    }

#if 0
    if (_interactor_index < 0)
    {
	uiuModelessErrorMessageDialog
	    (_parse_widget,
	     "#10011",
	     "local continuous",
	     "interactor",
	     _parse_file,
	     yylineno);
	_error_occurred = TRUE;
	return;
    }

    interactor = _network->interactor + _interactor_index;

    if (NOT interactor->in_use)
    {
	uiuModelessErrorMessageDialog
	    (_parse_widget,
	     "#10011",
	     "local continuous",
	     "interactor",
	     _parse_file,
	     yylineno);
	_error_occurred = TRUE;
	return;
    }
#endif

#if 1
    int instance = this->getInstanceCount();
    ScalarInstance *si = (ScalarInstance*)this->getInstance(instance);
    if (!si) {
 	ErrorMessage("'local continuous' comment out of order (file %s, line%d)",
			filename, lineno);
	return FALSE;
    }
    si->useLocalContinuous((continuous == 0 ? FALSE : TRUE));
    if (!EqualString(mode,"local"))
       si->useGlobalContinuous();
    
#else
    interactor->continuous =
	interactor->restore_continuous = 
	    continuous;

    interactor->restore_local_continuous =
	interactor->local_continuous =
	    uiuEqualString(mode, "local");
#endif

    return TRUE;

}

//
// Determine if this node is of the given class.
//
boolean ScalarNode::isA(Symbol classname)
{
    Symbol s = theSymbolManager->registerSymbol(ClassScalarNode);
    if (s == classname)
	return TRUE;
    else
	return this->InteractorNode::isA(classname);
}

boolean ScalarNode::isMinimumVisuallyWriteable()
{ 
    return 
	this->isInputDefaulting(DATA_PARAM_NUM) 	&& 
	this->isAttributeVisuallyWriteable(MIN_PARAM_NUM); 
}
boolean ScalarNode::isMaximumVisuallyWriteable()
{ 
    return 
	this->isInputDefaulting(DATA_PARAM_NUM) 	&& 
	this->isAttributeVisuallyWriteable(MAX_PARAM_NUM); 
}
boolean ScalarNode::isDecimalsVisuallyWriteable()
{ 
    return 
	this->isInputDefaulting(DATA_PARAM_NUM) 	&& 
	(this->isInputDefaulting(MIN_PARAM_NUM) || 
 	 this->isInputDefaulting(MAX_PARAM_NUM))  	&&
        this->isAttributeVisuallyWriteable(DECIMALS_PARAM_NUM);
}
boolean ScalarNode::isDeltaVisuallyWriteable()
{ 
    const char *delta_method = this->getInputValueString(INCRMETHOD_PARAM_NUM);

    //
    // Check to see of the tab-down value of the method input is set to 
    // 'absolute'.  We can only write this value when using absolute 
    // increments.  Actually, this test should be moved to the set attributes
    // code since it is the one that decides what kind of increments it
    // can support.
    //
    if (!this->isInputDefaulting(INCRMETHOD_PARAM_NUM) && 
	!this->isInputConnected(INCRMETHOD_PARAM_NUM)  && 
	!EqualString(delta_method,"absolute")) 		
	return FALSE;

    return 
	this->isInputDefaulting(DATA_PARAM_NUM) 	&& 
	(this->isInputDefaulting(MIN_PARAM_NUM) || 
 	 this->isInputDefaulting(MAX_PARAM_NUM))  	&&
	this->isAttributeVisuallyWriteable(INCR_PARAM_NUM);
}
//
// Catch the output comment to determine the number of components.
// We need to do this when we have a 2d vector and no .cfg file to
// tell us so.  We then get numComponent==3, but all the parameter
// values are 2d, which results in an ASSERTion failure.
//
boolean     ScalarNode::netParseComment(const char* comment,
                                        const char *file, int lineno)
{
    boolean r = this->InteractorNode::netParseComment(comment,file,lineno);
    if (r && EqualSubstring(comment," output[",8)) {
  	Parameter *p = this->getOutputParameter(1);
	this->numComponents = p->getComponentCount();
    }
    return r;
}



//
// If we're parsing input #4 (which is now REFRESH and was at one point REMAP)
// and the net version is older than 3.1.0 (which is compiled in by DXVersion.h),
// then set the defaulting status = TRUE.  Reason:  the older ui was setting
// the defaulting status of this unused param to FALSE.  Now that we want to use
// the param again, old nets believe the param is set.
//
// So, this chunk of code should go away if REFRESH goes away.  It should also
// go away sometime in the future (after no more old nets exist).
//
// In case it's not obvious, I'll tell you: this code was copy/pasted from 
// Node.C.  Maybe it should be kept up to date with Node.C, but really its only
// purpose is to deal with old nets, so if comments change this code won't really
// affected.
//
boolean ScalarNode::parseIOComment(boolean input, const char* comment,
								   const char* filename, int lineno, boolean valueOnly)
{
	int      defaulting = 0, items_parsed, ionum, tmp_type;
	int	     visible = TRUE;
	boolean  parse_error = FALSE; 

	if(this->Node::parseIOComment(input, comment, filename, lineno, valueOnly) == FALSE)
		return FALSE;

	//
	//
	// If the net version is less than 3.1.0 then set param 4 defaulting status
	// to true.  The ui had mistakenly been setting it to false although the param
	// was not in use by the module.  Now we want to use that param but existing
	// nets don't work unless param 4 (REFRESH) starts out defaulting.
	//
	// This code turns into NoOp starting with net version == 3.1.0
	//
	Network *net = this->getNetwork();
	if (net->getNetMajorVersion() >  3) return TRUE;
	if (net->getNetMinorVersion() >= 1) return TRUE;

	ASSERT(comment);

	if ((input) && (!parse_error)) {

		if (valueOnly) {
			if (sscanf(comment, " input[%d]: defaulting = %d", 
				&ionum, &defaulting) != 2)
				parse_error = TRUE;
			/*type = DXType::UndefinedType;*/
		} else {
			items_parsed = sscanf(comment,
				" input[%d]: defaulting = %d, visible = %d, type = %d", 
				&ionum, &defaulting, &visible, &tmp_type);
			/*type = (Type) tmp_type;*/
			if (items_parsed != 4) {
				// Invisibility added 3/30/93
				items_parsed = sscanf(comment, " input[%d]: visible = %d", 
					&ionum, &visible);
				if (items_parsed != 2) {
					// Backwards compatibility added 3/25/93
					items_parsed = sscanf(comment, " input[%d]: type = %d", 
						&ionum, &tmp_type);
					/*type = (Type) tmp_type;*/
					if (items_parsed != 2) {
						items_parsed = sscanf(comment,
							" input[%d]: defaulting = %d, type = %d", 
							&ionum, &defaulting, &tmp_type);
						/*type = (Type) tmp_type;*/
						if (items_parsed != 3) 
							parse_error = TRUE;
					} 
				}
				else 
				{
					defaulting = 1;
				}
			}
		}
	} 

	if ((!parse_error) && (input) && 
		(ionum == REFRESH_PARAM_NUM) && (defaulting == FALSE)) {
			Parameter *par = this->getInputParameter(REFRESH_PARAM_NUM);
			par->setUnconnectedDefaultingStatus(TRUE);
	}


	return TRUE;
}


boolean ScalarNode::printJavaValue (FILE* jf)
{
    const char* indent = "        ";
    int comp_count = this->getComponentCount();
    const char* var_name = this->getJavaVariable();
    int i;
    for (i=1; i<=comp_count; i++) {
	if (this->isIntegerTypeComponent()) {
	    int min = (int)this->getComponentMinimum(i);
	    int max = (int)this->getComponentMaximum(i);
	    int step = (int)this->getComponentDelta(i);
	    int value = (int)this->getComponentValue(i);
	    fprintf (jf, "%s%s.setValues(%d,%d,%d,%d,%d);\n",
		indent, var_name, i,min,max,step,value);
	} else {
	    double min = this->getComponentMinimum(i);
	    double max = this->getComponentMaximum(i);
	    double step = this->getComponentDelta(i);
	    double value = this->getComponentValue(i);
	    int decimals = this->getComponentDecimals(i);
	    fprintf (jf, "%s%s.setValues(%d,%g,%g,%g,%d,%g);\n",
		indent, var_name, i,min,max,step,decimals,value);
	}
    }
    return TRUE;
}


