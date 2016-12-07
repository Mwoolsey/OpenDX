/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"



// ScalarInstance.h -							
//                                                                       
// Definition for the ScalarInstance class.				  
//
// Among other things newSetAttrDialog() is provided which 
// builds either a SetScalarAttrDialog or a SetVectorAttrDialog depending
// upon the number of components (dimensions) found in the ScalarNode.
// Derived classes can override this to user their own set attributes
// dialog.
//                                                                         

#ifndef _ScalarInstance_h
#define _ScalarInstance_h


#include "InteractorInstance.h"
#include "LocalAttributes.h"
#include "ScalarNode.h"

//
// Class name definition:
//
#define ClassScalarInstance	"ScalarInstance"

class SetScalarAttrDialog;

//
// Describes an instance of an scalar interactor in a control Panel.
//
class ScalarInstance : public InteractorInstance  {

    friend class ScalarNode;
    friend class SetScalarAttrDialog;
    friend class StepperInteractor;
    friend class DialInteractor;
    friend class SliderInteractor;

  private:

    // 
    // List of local attributes 1 for each component
    // 
    List	localAttributes;	
    boolean	localContinuous;
    boolean	usingLocalContinuous;

  protected:

  //  boolean appendLocalAttributes(LocalAttributes *la)
  //	    { return this->localAttributes.appendElement((void*)la); }

    LocalAttributes *getLocalAttributes(int component)
	    { 
	      ASSERT(component > 0);
	      LocalAttributes *la = 
		(LocalAttributes*) this->localAttributes.getElement(component);
	      ASSERT(la);
	      return la; 
	    }

    void useLocalContinuous() 	{ usingLocalContinuous = TRUE; } 
    void useLocalContinuous(boolean val ) 
				{ 
				  this->usingLocalContinuous = TRUE; 
		  		  this->localContinuous = val; 
				}
    boolean getLocalContinuous()
	    { return this->localContinuous; }
    void    setLocalContinuous(boolean continuous)
	    { this->localContinuous = continuous; }
 	
    void setGlobalContinuous(boolean continuous)	
	    { ASSERT(node);
	      ((ScalarNode*)this->node)->setContinuous(continuous);
	    }
    void useGlobalContinuous() 
				{ this->usingLocalContinuous = FALSE; } 
    void useGlobalContinuous(boolean val) 
				{ 
				  this->usingLocalContinuous = FALSE; 
				  this->setGlobalContinuous(val);
				}
    boolean getGlobalContinuous()
	    { ASSERT(node);
	      return ((ScalarNode*)this->node)->isContinuous();
	    }

    void clrLocalDelta(int component) 	
	    { LocalAttributes *la = this->getLocalAttributes(component); 
	      la->clrLocalDelta();
	    }
    void useLocalDelta(int component, double delta) 
	    { LocalAttributes *la = this->getLocalAttributes(component); 
	      la->useLocalDelta(delta);
	    }
    double getLocalDelta(int component)	
	    { LocalAttributes *la = this->getLocalAttributes(component);
	      return la->getDelta();
	    }
    void setLocalDelta(int component, double delta)	
	    { LocalAttributes *la = this->getLocalAttributes(component);
	      la->setDelta(delta);
	    }
    void setGlobalDelta(int component, double delta)	
	{ ASSERT(node);
	  ((ScalarNode*)this->node)->setComponentDelta(component, delta);
	}
    void useGlobalDelta(int component, double val)	
	{ ASSERT(node);
	  this->clrLocalDelta(component);
	  this->setGlobalDelta(component,val);
	}
    double getGlobalDelta(int component)	
	{ ASSERT(node);
	  return ((ScalarNode*)this->node)->getComponentDelta(component);
	}


#if 0	// 7/13/93
    void setLocalMinimum(int component, double val)	
	    { LocalAttributes *la = this->getLocalAttributes(component);
	      la->setMinimum(val);
	    }
#endif
    double getGlobalMinimum(int component)	
	{ ASSERT(node);
	  return 
	  ((ScalarNode*)this->node)->getComponentMinimum(component);
	}
    void useGlobalMinimum(int component, double val)	
	{ ASSERT(node);
	 ((ScalarNode*)this->node)->setComponentMinimum(component,val);
	}
		   
#if 0	// 7/13/93
    void setLocalMaximum(int component, double val)	
	    { LocalAttributes *la = this->getLocalAttributes(component);
	      la->setMaximum(val);
	    }
#endif
    double getGlobalMaximum(int component)	
	{ ASSERT(node);
	  return 
	   ((ScalarNode*)this->node)->getComponentMaximum(component);
	}
    void useGlobalMaximum(int component, double val)	
	{ ASSERT(node);
	((ScalarNode*)this->node)->setComponentMaximum(component,val);
	}

#if 0	// 7/13/93
    void setLocalDecimals(int component, int val)	
	    { LocalAttributes *la = this->getLocalAttributes(component);
	      la->setDecimals(val);
	    }
#endif
    int getGlobalDecimals(int component)	
	{ ASSERT(node);
	  return 
	   ((ScalarNode*)this->node)->getComponentDecimals(component);
	}
    void useGlobalDecimals(int component, int val)	
	{ ASSERT(node);
	((ScalarNode*)this->node)->setComponentDecimals(component,val);
	}

    //
    // Be default the component value is kept in the 'current value' input
    // for the interactor module, but can be overridden for modules like 
    // the ScalarList which have a 'component value' but don't send it to
    // the executive.
    //
    virtual void setComponentValue(int component, double val);

    //
    // Change the dimensionality of a Vector interactor.
    //
    boolean handleNewDimensionality();

    //
    // Create the default  set attributes dialog box for this class of
    // Interactor.
    //
    virtual SetAttrDialog *newSetAttrDialog(Widget parent);

    //
    // Make sure the given value (assumed to be valid value that type matches
    // with the given output is) complies with any attributes.
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
    virtual boolean verifyValueAgainstAttributes(int output, const char *val,
							Type t,
                                                        char **reason);
    //
    // Perform the functions of ScalarInstance::verifyValueAgainstAttributes()
    // on a single Vector, Scalar or Integer (VSI) string value.
    //
    boolean verifyVSIAgainstAttributes(const char *val,
                                        Type t, char **reason);

    virtual const char* javaName() { return "step"; }

  public:
    ScalarInstance(ScalarNode *n);

    ~ScalarInstance();

    int getComponentCount()	
	{ ASSERT(node);
	  return ((ScalarNode*)this->node)->getComponentCount();
	}

    boolean isIntegerTypeComponent()	
	{ ASSERT(node); 
	  return ((ScalarNode*)this->node)->isIntegerTypeComponent(); 
	}

    boolean isVectorType()	
	{ ASSERT(node); 
	  return ((ScalarNode*)this->node)->isVectorType(); 
	}

    boolean usingGlobalContinuous()	{ return !usingLocalContinuous ; } 
    boolean isContinuous()	
		{ ASSERT(node); 
		  return (usingLocalContinuous ? localContinuous :
			((ScalarNode*)this->node)->isContinuous());
		 }

    //
    // Get information about the difference between successive values shown 
    // in this interactor.
    //
    boolean isLocalDelta(int component)
		{ LocalAttributes *la = this->getLocalAttributes(component);
		  return la->isLocalDelta();
		}

    double getDelta(int component)	
		{ return (this->isLocalDelta(component) ?
			  this->getLocalDelta(component) :
			  this->getGlobalDelta(component)); 
		} 
		   
    //
    // Get information about the minimum value shown in this interactor.
    //
    boolean isLocalMinimum(int /* component */) { return FALSE; }

    double getMinimum(int component)	 
			{ return this->getGlobalMinimum(component); }

		   
    //
    // Get information about the maximum value shown in this interactor.
    //
    boolean isLocalMaximum(int /* component */) { return FALSE; }

    double getMaximum(int component)	 
			{ return this->getGlobalMaximum(component); }

    //
    // Get information about the number of decimal points shown in the
    // interactor.
    //
    boolean isLocalDecimals(int /* component */) { return FALSE; }

    int getDecimals(int component)	
		{ return this->getGlobalDecimals(component); }
		
    //
    // Be default the component value is kept in the 'current value' input
    // for the interactor module, but can be overridden for modules like 
    // the ScalarList which have a 'component value' but don't send it to
    // the executive.
    //
    virtual double getComponentValue(int component);
		   
    //
    // Create a value string from the component values.
    // The returned string must be deleted by the caller.
    //
    char *buildValueFromComponents();

    virtual boolean printAsJava(FILE*);

    virtual boolean hasSetAttrDialog();

    const char *getClassName() { return ClassScalarInstance; }
	
};

#endif // _ScalarInstance_h

