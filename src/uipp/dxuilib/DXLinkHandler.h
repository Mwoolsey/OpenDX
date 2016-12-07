/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"


#ifndef _DXLinkHandler_h
#define _DXLinkHandler_h

#include "Base.h"
#include "PacketIF.h"
#include "LinkHandler.h"

//
// Class name definition:
//
#define ClassDXLinkHandler "DXLinkHandler"

class DXLinkHandler : public LinkHandler
{
    private:

	static boolean SaveNetwork(const char *c, int id, void *va);
	static boolean OpenNetwork(const char *c, int id, void *va);
	static boolean OpenNetworkNoReset(const char *c, int id, void *va);
	static boolean OpenConfig(const char *c, int id, void *va);
	static boolean SaveConfig(const char *c, int id, void *va);
	static boolean ResetServer(const char *c, int id, void *va);
	static boolean Version(const char *c, int id, void *va);
	static boolean SetTabValue(const char *c, int id, void *va);
	static boolean SetGlobalValue(const char *c, int id, void *va);
	static boolean Terminate(const char *, int id, void *);
	static boolean Disconnect(const char *, int id, void *);
	static boolean QueryValue(const char *c, int id, void *va);
	static boolean QueryExecution(const char *c, int id, void *va);
	static boolean ConnectToServer(const char *c, int id, void *va);
	static boolean StartServer(const char *c, int id, void *va);
	static void    SyncCB(void *clientData, int id, void *line);
	static boolean Sync(const char *, int id, void *va);
	static boolean SyncExec(const char *, int id, void *va);
	static boolean OpenControlPanel(const char *c, int id, void *va);
	static boolean CloseControlPanel(const char *c, int id, void *va);
	static boolean ResendParameters(const char *c, int id, void *va);
	static boolean SetProbePoint(const char *c, int id, void *va);
	static boolean SetInteractionMode(const char *c, int id, void *va);
	static boolean LoadMacroFile(const char *c, int id, void *va);
	static boolean LoadMacroDirectory(const char *c, int id, void *va);
	static boolean ExecOnce(const char *c, int id, void *va);
	static boolean ExecOnChange(const char *c, int id, void *va);
	static boolean EndExecution(const char *c, int id, void *va);
	static boolean EndExecOnChange(const char *c, int id, void *va);
	static boolean PopupVCR(const char *c, int id, void *va);
	static boolean OpenVPE(const char *c, int id, void *va);
	static boolean CloseVPE(const char *c, int id, void *va);
	static boolean OpenSequencer(const char *c, int id, void *va);
	static boolean CloseSequencer(const char *c, int id, void *va);
	static boolean OpenColormapEditor(const char *c, int id, void *va);
	static boolean CloseColormapEditor(const char *c, int id, void *va);
	static boolean OpenImage(const char *c, int id, void *va);
	static boolean CloseImage(const char *c, int id, void *va);
	static boolean SelectProbe(const char *c, int id, void *va);
        static boolean SequencerPlay(const char *c, int id, void *va);
        static boolean SequencerPause(const char *c, int id, void *va);
        static boolean SequencerStep(const char *c, int id, void *va);
        static boolean SequencerStop(const char *c, int id, void *va);
        static boolean SequencerPalindrome(const char *c, int id, void *va);
        static boolean SequencerLoop(const char *c, int id, void *va);

	static boolean SetHWRendering(const char *c, int id, void *va);
	static boolean SetSWRendering(const char *c, int id, void *va);
	boolean setRenderMode(const char *msg, int id, boolean swmode);


	static boolean StallNTimes(void *d);
        static boolean StallUntil(const char *c, int id, void *va);

	//
	// These two are used to delay the handling of messages
	// until the executive is done executing.  They also serve, to
	// interleave X events with the handling of messages which
	// keeps messages from being handled consecutively.
	//
	static boolean DestallOnNoExecution(void *d);
	void stallForExecutionIfRequired();
    
	int stallCount;	// For implementing StallNTimes().

    public:

	DXLinkHandler(PacketIF *);

	//
	// Returns a pointer to the class name.
	//
	const char* getClassName()
	{
	    return ClassDXLinkHandler;
	}

};

#endif
