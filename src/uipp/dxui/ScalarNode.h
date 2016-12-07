/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"



#ifndef _ScalarNode_h
#define _ScalarNode_h


#include "InteractorNode.h"
#include "StepperInteractor.h"
#include "List.h"

class Network;

//
// Class name definition:
//
#define ClassScalarNode	"ScalarNode"

//
// ScalarNode class definition:
//				
class ScalarNode : public InteractorNode 
{

  private:
    //
    // Private member data:
    //
    boolean	vectorType;
    boolean	needsRangeCheck;
    int		rangeCheckDeferrals;

    //
    // Adjusts the dimensionality of all attribute components for
    // this->doDimensionalityChange().
    //
    boolean adjustAttributeDimensions(int old_dim, int new_dim);

  protected:
    //
    // Protected member data:
    //

    //
    // If we're parsing input #4 (which is now REFRESH and was at one point REMAP)
    // and the net version is older than 3.1.0 (which is compiled in by DXVersion.h),
    // then set the defaulting status = TRUE.  Reason:  the older ui was setting
    // the defaulting status of this unused param to FALSE.  Now that we want to use
    // the param again, old nets believe the param is set.
    //
    virtual boolean parseIOComment(boolean input, const char* comment,
		const char* filename, int lineno, boolean valueOnly = FALSE);

    //
    // Make sure the number of inputs is the number expected.
    //
    boolean verifyInputCount();

    //
    // Adjusts the dimensionality of all outputs for
    // this->doDimensionalityChange().
    //
    virtual boolean adjustOutputDimensions(int old_dim, int new_dim);

    //
    // Global flag for all interactor instances indicating whether or not to 
    // do continuous updates to the exec.
    //
    boolean	isContinuousUpdate;	


    boolean cfgParseComponentComment(const char *comment,
					const char *filename, int lineno);
    boolean cfgParseInstanceComment(const char *comment,
					const char *filename, int lineno);

    //
    // Print the '// interactor...' comment  to the .cfg file.
    // We override the parent's method so that we can print num_components
    // correctly.
    //
    virtual boolean cfgPrintInteractorComment(FILE *f);
    //
    // Print auxiliary info for this interactor, which includes all global
    // information about each component.
    //
    virtual boolean cfgPrintInteractorAuxInfo(FILE *f);
    //
    // Print auxiliary info for the interactor instance,  which includes
    // all local information about each component.
    //
    virtual boolean cfgPrintInstanceAuxInfo(FILE *f, InteractorInstance *ii);

    //
    // Get a ScalarInstance instead of an InteractorInstance.
    //
    virtual InteractorInstance	*newInteractorInstance();

    //
    // Parse comments the 'local increment' comment. 
    //
    boolean cfgParseLocalIncrementComment(const char *comment,
					const char *filename, int lineno);

    //
    // Parse comments the 'local increment' comment. 
    //
    boolean cfgParseLocalContinuousComment(const char *comment,
					const char *filename, int lineno);

    // The messages we deal with can contain one or more of the following...
    //
    // 'min=%g' 'max=%g' 'delta=%g' 'value=%g' 'decimals=%d'
    //
    //  or one or more of the following...
    //
    // 'min=[%g %g %g]' 'max=[%g %g %g]' 'delta=[%g %g %g]' 'value=[%g %g %g]'
    // 'decimals=[%g %g %g]'
    //
    // Returns the number of attributes parsed.
    virtual int  handleInteractorMsgInfo(const char *line);
    //
    // Handles...
    // 'min=%g' 'max=%g' 'delta=%g' 'value=%g' 'decimals=%d'
    //
    int handleScalarMsgInfo(const char *line);
    //
    // Handles...
    // 'dim=%d'
    // 'min=[%g %g %g]' 'max=[%g %g %g]' 'delta=[%g %g %g]' 'value=[%g %g %g]'
    // 'decimals=[%g %g %g]'
    //
    int handleVectorMsgInfo(const char *line);

    //
    // Set the interactor's default attributes (attributes that can
    // be shared by derived classes).
    //
    virtual boolean setDefaultAttributes();

    //
    // Get and set the USUALLY global component values.  In some cases
    // (i.e. ScalarListNode), the component values are saved with the 
    // InteractorInstance.  So we make these protected and only allow
    // the ScalarInstance class to access them.
    //
    boolean setComponentValue(int component, double value);
    double getComponentValue(int component);
    friend class ScalarInstance;

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
    virtual void doRangeCheckComponentValue(int component, 
				double min, double max); 

    //
    // Call doRangeCheckComponentValue() if range checking is not deferred.
    //
    void rangeCheckComponentValue(int component, double min, double max); 

    //
    // Provide methods to delay doRangeCheckComponentValue().
    //
    boolean isRangeCheckingDeferred()
                        { return this->rangeCheckDeferrals != 0;}
    void deferRangeChecking()   {this->rangeCheckDeferrals++;}
    void undeferRangeChecking()
                        {  ASSERT(this->rangeCheckDeferrals > 0);
                           if ((--this->rangeCheckDeferrals == 0) &&
                               this->needsRangeCheck)
                                this->rangeCheckComponentValue(-1, 0.0, 0.0);
                        }


    //
    // Initialize the attributes with the give string values.
    //
    boolean initMinimumAttribute(const char *val);
    boolean setMinimumAttribute(const char *val);
    boolean initMaximumAttribute(const char *val);
    boolean setMaximumAttribute(const char *val);
    boolean initDeltaAttribute(const char *val);
    boolean setDeltaAttribute(const char *val);
    boolean initDecimalsAttribute(const char *val);
    boolean setDecimalsAttribute(const char *val);

    //
    // Change the dimensionality of a Vector interactor.
    //
    virtual boolean doDimensionalityChange(int new_dim);


  public:
    //
    // Constructor:
    //
    ScalarNode(NodeDefinition *nd, Network *net, int instnc, 
				boolean isVector = FALSE, int dimensions = 1); 

    //
    // Destructor:
    //
    ~ScalarNode(){}

    //
    // Set the attributes for the given component of the given instance
    // of an interactor.
    //
    boolean setAllComponentRanges(double *min, double *max);
    boolean setComponentMinimum(int component, double min);
    boolean setComponentMaximum(int component, double max);
    boolean setComponentDelta(int component, double delta);
    boolean setComponentDecimals(int component, int decimals); 
    void setContinuous(boolean val) { isContinuousUpdate = val; }
    void setContinuous() { isContinuousUpdate = TRUE; }
    void clrContinuous() { isContinuousUpdate = FALSE; }

    //
    // Get the global attributes for the given component of an interactor.
    //
    double getComponentMinimum(int component);
    double getComponentMaximum(int component);
    double getComponentDelta(int component);
    int    getComponentDecimals(int component);
    boolean isIntegerTypeComponent(); 
    boolean isContinuous()  { return isContinuousUpdate; }

    boolean isVectorType() { return this->vectorType; }

    //
    // Called once for each instance 
    //
    virtual boolean initialize();

    //
    // Indicates whether this node has outputs that can be remapped by the
    // server.
    //
    virtual boolean hasRemappableOutput() { return TRUE; }

    //
    // Call the super class method, but then catch the output comment 
    // to determine the number of components.  
    // We need to do this when we have a 2d vector and no .cfg file to
    // tell us so.  We then get numComponent==3, but all the parameter
    // values are 2d, which results in an ASSERTion failure.
    //
    virtual boolean     netParseComment(const char* comment,
					    const char *file, int lineno);

    //
    // Parse comments found in the .cfg that the InteractorNode knows how to
    // parse plus ones that it does not.
    //
    virtual boolean cfgParseComment(const char *comment,
					const char *filename, int lineno);

    //
    // Does this node implement doDimensionalityChange();
    //
    virtual boolean hasDynamicDimensionality(boolean ignoreDataDriven = FALSE);

    //
    // Determine if this node is a node of the given class
    //
    virtual boolean isA(Symbol classname);

    boolean isMinimumVisuallyWriteable();
    boolean isMaximumVisuallyWriteable();
    boolean isDecimalsVisuallyWriteable();
    boolean isDeltaVisuallyWriteable();

    virtual boolean printJavaValue(FILE*);
    virtual const char* getJavaNodeName() { return "ScalarNode"; }


    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassScalarNode;
    }
};


#endif // _ScalarNode_h
