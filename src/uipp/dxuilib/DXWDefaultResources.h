/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    (char *)"IBM PUBLIC LICENSE - Open Visualization Data Explorer" */
/***********************************************************************/

#ifndef _DXWDefaultResources_h
#define _DXWDefaultResources_h

#if !defined( DX_NEW_KEYLAYOUT )

String DXWindow::DefaultResources[] = {
    (char *)"*executeMenu.labelString:                    Execute",
    (char *)"*executeMenu.mnemonic:                       x",
    (char *)"*executeOnceOption.labelString:              Execute Once",
    (char *)"*executeOnceOption.mnemonic:                 O",
    (char *)"*executeOnceOption.accelerator:              Ctrl<Key>O",
    (char *)"*executeOnceOption.acceleratorText:          Ctrl+O",
    (char *)"*executeOnChangeOption.labelString:          Execute on Change",
    (char *)"*executeOnChangeOption.mnemonic:             C",
    (char *)"*executeOnChangeOption.accelerator:          Ctrl<Key>C",
    (char *)"*executeOnChangeOption.acceleratorText:      Ctrl+C",
    (char *)"*endExecutionOption.labelString:             End Execution",
    (char *)"*endExecutionOption.mnemonic:                E",
// Switched to using osfEndLine for the accelerator instead of End
// because of problems on osf1.  Motif libs on (ARCH?) can't deal with
// an osf key name as an accelerator.
#if defined( aviion )
    (char *)"*endExecutionOption.accelerator:             Ctrl<Key>End",
#else
    (char *)"*endExecutionOption.accelerator:             Ctrl<Key>osfEndLine",
#endif
    (char *)"*endExecutionOption.acceleratorText:         Ctrl+End",
    (char *)"*sequencerOption.labelString:                Sequencer",
    (char *)"*sequencerOption.mnemonic:                   S",
#if USE_REMAP  // 6/14/93
    (char *)"*remapInteractorOutputsOption.labelString:   Remap Interactor "
            "Outputs",
    (char *)"*remapInteractorOutputsOption.mnemonic:      R",
#endif
    (char *)"*connectionMenu.labelString:                 Connection",
    (char *)"*connectionMenu.mnemonic:                    C",
    (char *)"*startServerOption.labelString:              Start Server...",
    (char *)"*startServerOption.mnemonic:                 S",
    (char *)"*disconnectFromServerOption.labelString:     Disconnect from "
            "Server",
    (char *)"*disconnectFromServerOption.mnemonic:        D",
    (char *)"*resetServerOption.labelString:              Reset Server",
    (char *)"*resetServerOption.mnemonic:                 R",
    (char *)"*processGroupAssignmentOption.labelString:   Execution Group "
            "Assignment...",
    (char *)"*processGroupAssignmentOption.mnemonic:      P",
    (char *)"*helpTutorialOption.labelString:             Tutorial...",
    (char *)"*helpTutorialOption.mnemonic:                u",
    (char *)"*toggleWindowStartupOption.labelString:      Startup Window",
    (char *)"*toggleWindowStartupOption.mnemonic:         S",
    (char *)"*connectionMenuPulldown.tearOffModel:	  XmTEAR_OFF_DISABLED",
    (char *)"*fileHistory.labelString:			  Recent Programs",
    NULL};

#else /* defined(DX_NEW_KEYLAYOUT) */

String DXWindow::DefaultResources[] = {
    (char *)"*executeMenu.labelString:                    	Execute",
    (char *)"*executeMenu.mnemonic:                       	x",
    (char *)"*executeOnceOption.labelString:              	Execute Once",
    (char *)"*executeOnceOption.mnemonic:                 	O",
    (char *)"*executeOnceOption.accelerator:              	Ctrl<Key>E",
    (char *)"*executeOnceOption.acceleratorText:          	Ctrl+E",
    (char *)"*executeOnChangeOption.labelString:          	Execute "
            "on Change",
    (char *)"*executeOnChangeOption.mnemonic:             	C",
    (char *)"*executeOnChangeOption.accelerator:          	"
            "Ctrl<Key>semicolon",
    (char *)"*executeOnChangeOption.acceleratorText:      	Ctrl+;",
    (char *)"*endExecutionOption.labelString:             	End Execution",
    (char *)"*endExecutionOption.mnemonic:                	E",
// Switched to using osfEndLine for the accelerator instead of End
// because of problems on osf1.  Motif libs on (ARCH?) can't deal with
// an osf key name as an accelerator.
#if defined( aviion )
    (char *)"*endExecutionOption.accelerator:             Ctrl<Key>End",
    (char *)"*endExecutionOption.acceleratorText:         Ctrl+End",
#elif defined( macos )
    (char *)"*endExecutionOption.accelerator:			Ctrl "
            "<Key>period",
    (char *)"*endExecutionOption.acceleratorText:		Ctrl+.",
#else
    (char *)"*endExecutionOption.accelerator:             	"
            "Ctrl<Key>osfEndLine",
    (char *)"*endExecutionOption.acceleratorText:         	Ctrl+End",
#endif
    (char *)"*sequencerOption.labelString:                	Sequencer",
    (char *)"*sequencerOption.mnemonic:                   	S",

#if USE_REMAP  // 6/14/93
    (char *)"*remapInteractorOutputsOption.labelString:   Remap "
            "Interactor Outputs",
    (char *)"*remapInteractorOutputsOption.mnemonic:      R",
#endif
    (char *)"*connectionMenu.labelString:                 	Connection",
    (char *)"*connectionMenu.mnemonic:                    	C",
    (char *)"*startServerOption.labelString:              	Start "
            "Server...",
    (char *)"*startServerOption.mnemonic:                 	S",
    (char *)"*disconnectFromServerOption.labelString:     	"
            "Disconnect from Server",
    (char *)"*disconnectFromServerOption.mnemonic:        	D",
    (char *)"*resetServerOption.labelString:              	Reset Server",
    (char *)"*resetServerOption.mnemonic:                 	R",
    (char *)"*resetServerOption.accelerator:			Ctrl "
            "Shift <Key>R",
    (char *)"*resetServerOption.acceleratorText:		Ctrl+Shift+R",
    (char *)"*processGroupAssignmentOption.labelString:   	"
            "Execution Group (char *)"(char *)"Assignment...",
    (char *)"*processGroupAssignmentOption.mnemonic:      	P",
    (char *)"*helpTutorialOption.labelString:             	Tutorial...",
    (char *)"*helpTutorialOption.mnemonic:                	u",
    (char *)"*toggleWindowStartupOption.labelString:      	Startup Window",
    (char *)"*toggleWindowStartupOption.mnemonic:         	S",
    (char *)"*connectionMenuPulldown.tearOffModel:	  XmTEAR_OFF_DISABLED",
    (char *)"*fileHistory.labelString:			  Recent "
            "Programs",
    (char *)NULL};

#endif

#endif  // _DXWDefaultResources_h
