/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"


// ScalarInteractor.h -							    //
//                                                                          //
// Definition for the ScalarInteractor class.				    //
//
// This class (a virtual class) and its derived classes together with 
// ScalarInstance are designed to support Integer, Scalar and n-dimensional
// Vector interactors.  Implementations include a stepper interactor, and
// will include sliders and dials.  It is up to the classes derived from this 
// to implement the support for those particular types.  
// 
// The ScalarNode maintains much of the state associated with an
// interactor of this type.  The ScalarInstance maintains some state local
// to particular instance of an interactor (an InteractorNode can have many
// Interactors) and provides access to the state in the ScalarNode.
// 
// 
// For further discussion on the implementation of interactors, see 
// Interactor.h, InteractorInstance.h and InteractorNode.h. 
// 
//                                                                          //

#ifndef _ScalarInteractor_h
#define _ScalarInteractor_h


#include "Interactor.h"


//
// Class name definition:
//
#define ClassScalarInteractor	"ScalarInteractor"

//
// XtCallbackProc (*CB), XtEventHandler (*EH) and XtActionProc (*AP)
// DialogCallback (*DCB), XtInputCallbackProc (*ICP), XtWorkProc (*WP)
// functions for this and derived classes
//
extern "C" void ScalarInteractor_NumberWarningCB(Widget, XtPointer, XtPointer);

class InteractorInstance;

//
// ScalarInteractor class definition:
//				
class ScalarInteractor : public Interactor
{
  private:
    //
    // Private member data:
    //

  protected:
    //
    // Protected member data:
    //


  public:
    //
    // Constructor:
    //
    // ScalarInteractor(const char *name) : Interactor(name) {}
    ScalarInteractor(const char *name,
		     InteractorInstance *ii) : Interactor(name,ii) {}

    //
    // Destructor:
    //
    ~ScalarInteractor(){}

    //
    // Warning callback that can be used by interactors that use
    // Number widgets.
    //
    friend void ScalarInteractor_NumberWarningCB(Widget  widget,
                        XtPointer clientData,
                        XtPointer callData);

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassScalarInteractor;
    }
};


#endif // _ScalarInteractor_h
