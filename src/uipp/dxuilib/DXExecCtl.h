/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"


#ifndef _DXExecCtl_h
#define _DXExecCtl_h


#include "Base.h"


//
// Class name definition:
//
#define ClassDXExecCtl	"DXExecCtl"

//
// Referenced classes.
class Network;

//
// DXExecCtl class definition:
//				
class DXExecCtl : public Base
{
  private:
    //
    // Private member data:
    //
    static void BGBeginMessage(void *clientData, int id, void *p);
    static void BGEndMessage(void *clientData, int id, void *p);
    static void HWBeginMessage(void *clientData, int id, void *p);
    static void HWEndMessage(void *clientData, int id, void *p);
    static void ExecComplete(void *clientData, int id, void *p);
    static void ResumeExecOnChange(void *clientData, int id, void *p);
    static void ResetVcrNextFrame(void *clientData, int id, void *p);

    void 	beginSingleExecution(boolean update_macros);
    void	endLastExecution(boolean resume = FALSE);

    boolean 	endExecOnChangePending;

  protected:
    //
    // Protected member data:
    //

    //
    // Execution flag.
    //
    boolean isCurrentlyExecuting;
    boolean vcrIsExecuting;
    boolean execOnChange;
    int	    execOnChangeSuspensions;
    int     hwExecuting;  	// counter for HW rendering messages
    boolean hwBusy;  		// setting of busy cursor for HW rendering 
    boolean forceNetworkResend;	
    boolean forceParameterResend;	
    boolean resumeExecOnChangeAfterExecution;	
    boolean isExecOnChangeSuspended()  
			     {	return this->execOnChangeSuspensions > 0; }
  public:
    //
    // Constructor:
    //
    DXExecCtl();

    //
    // Destructor:
    //
    ~DXExecCtl(){}


    //
    // Enter/leave execute on change mode.
    //
    void newConnection();
    void suspendExecOnChange();
    void resumeExecOnChange();

    //
    // Go out of execution on change mode without terminating the current
    // graph execution.  If not current executing, then we go ahead and
    // go out of eoc mode, otherwise schedule the exit from eoc mode for
    // the end of the current graph execution (see endLastExecution()).
    // We return TRUE if we were able to go out of eoc mode now, FALSE if
    // we won't be going out until the end of the current execution.
    //
    boolean endExecOnChange();

    void enableExecOnChange();
    void updateMacros(boolean force = FALSE);
    void executeOnce();

    boolean inExecOnChange() { return this->execOnChange;}
    boolean isExecuting() { return this->isCurrentlyExecuting; }
    boolean isVcrExecuting() { return this->vcrIsExecuting; }
    boolean assignmentRequiresExecution()
		{ return !this->isVcrExecuting() && !this->inExecOnChange(); }

    //
    // Functions called by the Sequencer.
    //
    void vcrFrameSet(char* command);
    void vcrTransmit();
    void vcrExecute(int action);
    void vcrCommand(int action, boolean state);
    
    //
    // Will take you out of execute on change if in it, and
    // terminate the current execution.
    void terminateExecution();
    void updateMacro(Network *n);

    //
    // This is used to force networks and parameters to be resent
    // after a reset.
    //
    void forceFullResend();

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassDXExecCtl;
    }
};


#endif // _DXExecCtl_h
