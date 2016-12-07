/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"



#ifndef _SequencerWindow_h
#define _SequencerWindow_h


#include "DXWindow.h"
#include "SequencerNode.h"

//
// Class name definition:
//
#define ClassSequencerWindow	"SequencerWindow"

//
// XtCallbackProc (*CB), XtEventHandler (*EH) and XtActionProc (*AP)
// DialogCallback (*DCB), XtInputCallbackProc (*ICP), XtWorkProc (*WP)
// functions for this and derived classes
//
extern "C" void SequencerWindow_VcrCB(Widget, XtPointer, XtPointer);
extern "C" void SequencerWindow_FrameCB(Widget, XtPointer, XtPointer);
extern "C" void SequencerWindow_PopdownCB(Widget, XtPointer, XtPointer);

//
// SequencerWindow class definition:
//				

class SequencerWindow : public DXWindow 
{
  friend class SequencerNode;

  private:
    //
    // Private member data:
    //
    static Boolean ClassInitialized;
    static String  DefaultResources[];

    Widget vcr;
    SequencerNode* node;
    boolean handlingStateChange;

  protected:
    //
    // Protected member data:
    //
    Widget createWorkArea(Widget);

    friend void SequencerWindow_PopdownCB(Widget, XtPointer , XtPointer);
    friend void SequencerWindow_FrameCB(Widget, XtPointer , XtPointer);
    friend void SequencerWindow_VcrCB(Widget, XtPointer , XtPointer);
    void frameCallback(XtPointer callData);


    //
    // En/Disable the Frame control.
    //
    void disableFrameControl();
    void enableFrameControl();

    virtual void handleStateChange(boolean unmanage = FALSE);

    //
    // Dis/enable frame control and then call the super class method.
    //
    virtual void beginExecution();
    virtual void standBy();
    virtual void endExecution();

    //
    // Install the default resources for this class and then call the
    // same super class method to get the default resources from the
    // super classes.
    //
    virtual void installDefaultResources(Widget baseWidget);

  public:

    //
    // Constructor:
    //
    SequencerWindow(SequencerNode* node);

    //
    // Destructor:
    //
    ~SequencerWindow();

    void    	mapRaise();
    void 	reset();

    //
    // In the local copies of these methods we'll set the startup mode in 
    // the SequencerNode so that the user can control the display of the Sequencer.
    // Other windows provide a menubar button which controls startup mode.  Since
    // we have no menubar we'll cheat.
    //
    virtual void manage();
    virtual void unmanage();
    virtual void setStartup(boolean set = TRUE);


    //
    // Set the buttons that indicate which way the sequencer is playing.
    //
    void setPlayDirection(SequencerDirection dir);

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassSequencerWindow;
    }
};


#endif // _SequencerWindow_h
