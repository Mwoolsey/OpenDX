/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#ifndef _DXWDefaultResources_h
#define _DXWDefaultResources_h

#if !defined(DX_NEW_KEYLAYOUT)

String DXWindow::DefaultResources[] =
{
    "*executeMenu.labelString:                    Execute",
    "*executeMenu.mnemonic:                       x",
    "*executeOnceOption.labelString:              Execute Once",
    "*executeOnceOption.mnemonic:                 O",
    "*executeOnceOption.accelerator:              Ctrl<Key>O",
    "*executeOnceOption.acceleratorText:          Ctrl+O",
    "*executeOnChangeOption.labelString:          Execute on Change",
    "*executeOnChangeOption.mnemonic:             C",
    "*executeOnChangeOption.accelerator:          Ctrl<Key>C",
    "*executeOnChangeOption.acceleratorText:      Ctrl+C",
    "*endExecutionOption.labelString:             End Execution",
    "*endExecutionOption.mnemonic:                E",
// Switched to using osfEndLine for the accelerator instead of End
// because of problems on osf1.  Motif libs on (ARCH?) can't deal with
// an osf key name as an accelerator.
#if defined(aviion)
    "*endExecutionOption.accelerator:             Ctrl<Key>End",
#else
    "*endExecutionOption.accelerator:             Ctrl<Key>osfEndLine",
#endif
    "*endExecutionOption.acceleratorText:         Ctrl+End",
    "*sequencerOption.labelString:                Sequencer",
    "*sequencerOption.mnemonic:                   S",
#if USE_REMAP	// 6/14/93
    "*remapInteractorOutputsOption.labelString:   Remap Interactor Outputs", 
    "*remapInteractorOutputsOption.mnemonic:      R",
#endif

    "*connectionMenu.labelString:                 Connection",
    "*connectionMenu.mnemonic:                    C",
    "*startServerOption.labelString:              Start Server...",
    "*startServerOption.mnemonic:                 S",
    "*disconnectFromServerOption.labelString:     Disconnect from Server",
    "*disconnectFromServerOption.mnemonic:        D",
    "*resetServerOption.labelString:              Reset Server",
    "*resetServerOption.mnemonic:                 R",
    "*processGroupAssignmentOption.labelString:   Execution Group Assignment...",
    "*processGroupAssignmentOption.mnemonic:      P",

    "*helpTutorialOption.labelString:             Tutorial...",
    "*helpTutorialOption.mnemonic:                u",

    "*toggleWindowStartupOption.labelString:      Startup Window",
    "*toggleWindowStartupOption.mnemonic:         S",

    "*connectionMenuPulldown.tearOffModel:	  XmTEAR_OFF_DISABLED",
    "*fileHistory.labelString:			  Recent Programs",

    NULL
};


#else /* defined(DX_NEW_KEYLAYOUT) */


String DXWindow::DefaultResources[] =
{
    "*executeMenu.labelString:                    	Execute",
    "*executeMenu.mnemonic:                       	x",
    "*executeOnceOption.labelString:              	Execute Once",
    "*executeOnceOption.mnemonic:                 	O",
    "*executeOnceOption.accelerator:              	Ctrl<Key>E",
    "*executeOnceOption.acceleratorText:          	Ctrl+E",
    "*executeOnChangeOption.labelString:          	Execute on Change",
    "*executeOnChangeOption.mnemonic:             	C",
    "*executeOnChangeOption.accelerator:          	Ctrl<Key>semicolon",
    "*executeOnChangeOption.acceleratorText:      	Ctrl+;",
    "*endExecutionOption.labelString:             	End Execution",
    "*endExecutionOption.mnemonic:                	E",
// Switched to using osfEndLine for the accelerator instead of End
// because of problems on osf1.  Motif libs on (ARCH?) can't deal with
// an osf key name as an accelerator.
#if defined(aviion)
    "*endExecutionOption.accelerator:             Ctrl<Key>End",
    "*endExecutionOption.acceleratorText:         Ctrl+End",
#elif defined(macos)
    "*endExecutionOption.accelerator:			Ctrl <Key>period",
    "*endExecutionOption.acceleratorText:		Ctrl+.",
#else
    "*endExecutionOption.accelerator:             	Ctrl<Key>osfEndLine",
    "*endExecutionOption.acceleratorText:         	Ctrl+End",
#endif
    "*sequencerOption.labelString:                	Sequencer",
    "*sequencerOption.mnemonic:                   	S",

#if USE_REMAP	// 6/14/93
    "*remapInteractorOutputsOption.labelString:   Remap Interactor Outputs", 
    "*remapInteractorOutputsOption.mnemonic:      R",
#endif

    "*connectionMenu.labelString:                 	Connection",
    "*connectionMenu.mnemonic:                    	C",
    "*startServerOption.labelString:              	Start Server...",
    "*startServerOption.mnemonic:                 	S",
    "*disconnectFromServerOption.labelString:     	Disconnect from Server",
    "*disconnectFromServerOption.mnemonic:        	D",
    "*resetServerOption.labelString:              	Reset Server",
    "*resetServerOption.mnemonic:                 	R",
    "*resetServerOption.accelerator:			Ctrl Shift <Key>R",
    "*resetServerOption.acceleratorText:		Ctrl+Shift+R",
    "*processGroupAssignmentOption.labelString:   	Execution Group Assignment...",
    "*processGroupAssignmentOption.mnemonic:      	P",

    "*helpTutorialOption.labelString:             	Tutorial...",
    "*helpTutorialOption.mnemonic:                	u",

    "*toggleWindowStartupOption.labelString:      	Startup Window",
    "*toggleWindowStartupOption.mnemonic:         	S",

    "*connectionMenuPulldown.tearOffModel:	  XmTEAR_OFF_DISABLED",

    "*fileHistory.labelString:			  Recent Programs",

    NULL
};


#endif

#endif // _DXWDefaultResources_h

