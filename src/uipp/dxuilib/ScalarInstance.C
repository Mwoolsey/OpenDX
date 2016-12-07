/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#include "ScalarInstance.h"
#include "ScalarNode.h"
#include "LocalAttributes.h"
#include "ControlPanel.h"
#include "SetScalarAttrDialog.h"
#include "SetVectorAttrDialog.h"
#include "Parameter.h"


ScalarInstance::ScalarInstance(ScalarNode *n) :
                InteractorInstance((InteractorNode*)n)
{
    LocalAttributes *la;
    this->localContinuous = FALSE;
    this->usingLocalContinuous = FALSE;
    int ncomp = n->getComponentCount();
    ASSERT(ncomp > 0);
    while (ncomp-- > 0) {
	la =  new LocalAttributes(n->isIntegerTypeComponent());
	this->localAttributes.appendElement((void*)la);
    }
}
	
ScalarInstance::~ScalarInstance() 
{
    LocalAttributes *la;
    int pos=1;
    while ( (la = (LocalAttributes*) this->localAttributes.getElement(pos++)) )
	delete la;
}

//
// Change the dimensionality of a Vector interactor.
//
boolean ScalarInstance::handleNewDimensionality()
{
    int i;
    LocalAttributes *la;
    int new_dim = this->node->getComponentCount();

    if (!this->isVectorType())
	return FALSE;

    int n_lattr = this->localAttributes.getSize();

    //
    // Adjust the number of LocalAttributes
    //
    if (n_lattr < new_dim) {
	boolean isInt = 
		((ScalarNode*)this->getNode())->isIntegerTypeComponent();
	for (i=n_lattr ; i<new_dim ; i++) {
	    la =  new LocalAttributes(isInt);
	    this->localAttributes.appendElement((void*)la);
	}
    } else if (n_lattr > new_dim) {
	for (i=new_dim+1 ; i<=n_lattr ; i++) {
	    la =  (LocalAttributes*)this->localAttributes.getElement(i);
	    if (la) {
		this->localAttributes.deleteElement(i);
		delete la;
	    }
	}
    } 


    //
    // Rebuild the interactor.
    //
    if (this->interactor) {
	delete this->interactor;
	this->interactor = NULL;  
    }
    if (this->controlPanel->getRootWidget())
        this->createInteractor();
    return TRUE;
}
//
// S/Get value associated with this interactor instance.
// Be default the component value is kept with the Node in the 'current value' 
// input for the interactor module, but can be overridden for modules like
// the ScalarList which have an instance-local 'component value' and don't 
// send it to the executive.
//
double ScalarInstance::getComponentValue(int component)
{ 
  ASSERT(this->node);
  return
   ((ScalarNode*)this->node)->getComponentValue(component);
}

void ScalarInstance::setComponentValue(int component, double val)
{ 
   ASSERT(this->node);
  ((ScalarNode*)this->node)->setComponentValue(component,val);
}

//
// Based on the values contained in the components, build a value
// string that is acceptable to a DXValue.
// The return string needs to be deleted by the caller.
// FIXME: should use DXValue methods.
//
char *ScalarInstance::buildValueFromComponents()
{
    int i, components = this->getComponentCount();
    boolean ints = this->isIntegerTypeComponent();
    char comp_val[128];
    char *s = new char[components * sizeof(comp_val)];
    boolean isVector = this->isVectorType();

    if (isVector)
        strcpy(s,"[ ");
    else
        strcpy(s,"");

    for (i=1 ; i<=components ; i++) {
	if (ints) {
	    int tmp = (int)this->getComponentValue(i);
	    DXValue::FormatDouble((double)tmp,comp_val, 0);
	} else if (isVector) {
	    DXValue::FormatDouble((double)this->getComponentValue(i), 
						comp_val, 0, TRUE);
	} else {
	    DXValue::FormatDouble((double)this->getComponentValue(i),comp_val);
	}
        strcat(s, comp_val);
        strcat(s, " ");
    }

    if (this->isVectorType())
        strcat(s,"]");

    return s;
}
//
// Return TRUE/FALSE indicating if this class of interactor instance has
// a set attributes dialog (i.e. this->newSetAttrDialog returns non-NULL).
//
boolean ScalarInstance::hasSetAttrDialog()
{
    return TRUE;
}

//
// Create the default set attributes dialog box for this type of interactor.
//
SetAttrDialog *ScalarInstance::newSetAttrDialog(Widget parent)
{
    ScalarNode *sinode = (ScalarNode*)this->getNode();

    SetScalarAttrDialog *d = NULL;

    if (sinode->getComponentCount() > 1) {
        d = new SetVectorAttrDialog(parent, "Set Attributes...", this);
    } else {
        d = new SetScalarAttrDialog(parent, "Set Attributes...", this);
    }
    return (SetAttrDialog*)d;
}
//
// Make sure the given output's current value complies with any attributes.
// This is called by InteractorInstance::setOutputValue() which is
// intern intended to be called by the Interactors.
// If verification fails (returns FALSE), then a reason is expected to
// placed in *reason.  This string must be freed by the caller.
// At this level we always return TRUE (assuming that there are no
// attributes) and set *reason to NULL.
//
// This class verifies the dimensionality of vectors and the range
// of the component values.
//
boolean ScalarInstance::verifyValueAgainstAttributes(int output, 
						const char *val, 
						Type t,
						char **reason)
{
    return this->verifyVSIAgainstAttributes(val,t,reason);
}


//
// Perform the functions of ScalarInstance::verifyValueAgainstAttributes()
// on a single Vector, Scalar or Integer (VSI) string value.
//
boolean ScalarInstance::verifyVSIAgainstAttributes(const char *val,
					Type t, char **reason)
{
    DXValue dxval;
    boolean r = TRUE;
    double d, dmin, dmax;
    int ncomp , i, imin, imax;

    if (*reason)
	*reason = NULL;

    if (!dxval.setValue(val,t))
	return FALSE;

    switch (t) {
	default:
	    ASSERT(0);
	break;

	case DXType::IntegerType:
	    i = dxval.getInteger();
	    imin = (int)this->getMinimum(1);
	    imax = (int)this->getMaximum(1);
	    if ((i < imin) || (i > imax)) {
		r = FALSE;
		if (reason) {
		    *reason = new char[128];
		    sprintf(*reason,"Integer values must be greater than or "
				    "equal to %d and less than or equal to %d.",
				    imin, imax);
		}
	    } 
	break;

	case DXType::ScalarType:
	    d = dxval.getScalar();
	    dmin = this->getMinimum(1);
	    dmax = this->getMaximum(1);

	    if ((d < dmin) || (d > dmax)) {
		r = FALSE;
		if (reason) {
		    *reason = new char[128];
		    sprintf(*reason,"Scalar values must be greater than or "
				    "equal to %g and less than or equal to %g.",
				    dmin, dmax);
		}
	    } 
	break;

	case DXType::VectorType:
	    ncomp = this->getComponentCount();
	    if (dxval.getVectorComponentCount() != ncomp) {
		r = FALSE;
		if (reason) {
		    *reason = new char[128];
		    sprintf(*reason,"Vector values must be %d dimensional.",
				   	ncomp); 
		}
	    } else {
	 	for (i=1; i<= ncomp && r ; i++) {
		    d = dxval.getVectorComponentValue(i);
		    dmin = this->getMinimum(i);
		    dmax = this->getMaximum(i);
		    if ((d < dmin) || (d > dmax)) {
			r = FALSE;
			if (reason) {
			    *reason = new char[128];
			    sprintf(*reason,
				"Vector component #%d must be greater than or "
				"equal to %g and less than or equal to %g.",
					    i, dmin, dmax);
			}
		    } 
		}
	    } 
	break;
    }
    return r;
}


boolean ScalarInstance::printAsJava (FILE* jf)
{
    const char* indent = "        ";
    if (this->style->hasJavaStyle() == FALSE)
	return InteractorInstance::printAsJava(jf);

    ScalarNode* sn = (ScalarNode*)this->node;
    const char* node_var_name = sn->getJavaVariable();
    int comp_count = sn->getComponentCount();

    int x,y,w,h;
    this->getXYPosition (&x, &y);
    this->getXYSize (&w,&h);
    ControlPanel* cpan = this->getControlPanel();
    boolean devstyle = cpan->isDeveloperStyle();

    //
    // Count up the lines in the label and split it up since java doesn't
    // know how to do this for us.
    //
    const char* ilab = this->getInteractorLabel();
    int line_count = CountLines(ilab);

    fprintf (jf, "\n");
    const char* java_style = this->style->getJavaStyle();
    ASSERT(java_style);
    const char* var_name = this->getJavaVariable();
    fprintf (jf, "%s%s %s = new %s(%d);\n", 
	indent, java_style, var_name, java_style, comp_count);
    fprintf (jf, "        %s.addInteractor(%s);\n", node_var_name, var_name);

    if (this->isVectorType())
	fprintf (jf, "%s%s.setVector();\n", indent, var_name);
    fprintf (jf, "%s%s.setStyle(%d);\n", indent, var_name, devstyle?1:0);

    fprintf (jf, "%s%s.setLabelLines(%d);\n", indent, var_name, line_count);
    int i;
    for (i=1; i<=line_count; i++) {
	const char* cp = InteractorInstance::FetchLine (ilab, i);
	fprintf (jf, "%s%s.setLabel(\"%s\");\n", indent, var_name, cp);
    }

    if (this->isVerticalLayout())
	fprintf (jf, "%s%s.setVertical();\n", indent, var_name);
    else
	fprintf (jf, "%s%s.setHorizontal();\n", indent, var_name);
    if (this->style->hasJavaStyle())
	fprintf (jf, "%s%s.setBounds (%s, %d,%d,%d,%d);\n", 
	    indent, node_var_name, var_name,x,y,w,h);
    else
	fprintf (jf, "%s%s.setBounds (%s, %d,%d,%d,%d);\n", 
	    indent, node_var_name, var_name, x,y,w,65);
	
    fprintf(jf, "        %s.addInteractor(%s);\n", cpan->getJavaVariableName(), var_name);

    return TRUE;
}


