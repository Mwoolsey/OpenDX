/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#ifndef _CPDefaultResources_h
#define _CPDefaultResources_h

#if !defined(DX_NEW_KEYLAYOUT)

String ControlPanel::DefaultResources[] =
{
    ".width:                                      375",
    ".height:                                     450",
    ".iconName:                                   Control Panel",
    ".title:                                      Control Panel",

    "*fileMenu.labelString:                       File",
    "*fileMenu.mnemonic:                          F",
    "*panelOpenOption.labelString:                     Open...",
    "*panelOpenOption.mnemonic:                        O",
    "*panelSaveAsOption.labelString:                   Save As...",
    "*panelSaveAsOption.mnemonic:                      S",
    "*panelCloseOption.labelString:                    Close",
    "*panelCloseOption.mnemonic:                       C",
    "*panelCloseAllOption.labelString:                 Close All Control Panels",

    "*stylePulldown.tearOffModel:		  XmTEAR_OFF_DISABLED",
    "*dimensionalityePulldown.tearOffModel:	  XmTEAR_OFF_DISABLED",
    "*layoutPulldown.tearOffModel:		  XmTEAR_OFF_DISABLED",
    "*editMenuPulldown.tearOffModel:		  XmTEAR_OFF_DISABLED",

    "*editMenu.labelString:                       Edit",
    "*editMenu.mnemonic:                          E",
    "*panelSetStyleButton.labelString:                 Set Style",
    "*panelSetStyleButton.mnemonic:                    y",
    "*panelSetDimensionalityButton.labelString:        Set Dimensionality",
    "*panelSetDimensionalityButton.mnemonic:           i",
    "*dimensionalityPulldown*set1DOption.labelString:      1 Dimensional",
    "*dimensionalityPulldown*set2DOption.labelString:      2 Dimensional",
    "*dimensionalityPulldown*set3DOption.labelString:      3 Dimensional",
    "*dimensionalityPulldown*set4DOption.labelString:      4 Dimensional",

    "*panelAddButton.labelString:    		       Add Element",
    "*panelAddButton.mnemonic:    		       A",
    "*panelSetLayoutButton.labelString:                Set Layout",
    "*panelSetLayoutButton.mnemonic:                   L",
    "*panelVerticalOption.labelString:                 Vertical", 
    "*panelHorizontalOption.labelString:               Horizontal",
    "*panelSetAttributesOption.labelString:            Set Attributes...",
    "*panelSetAttributesOption.mnemonic:               e",
    "*panelSetInteractorLabelOption.labelString:       Set Label...",
    "*panelSetInteractorLabelOption.mnemonic:          t",

    "*panelDeleteOption.labelString:                   Delete",
    "*panelDeleteOption.mnemonic:                      D",
#if defined(aviion)
    "*panelDeleteOption.accelerator:                   Ctrl<Key>Delete",
#else
    "*panelDeleteOption.accelerator:                   Ctrl<Key>osfDelete",
#endif
#ifdef sun4
    "*panelDeleteOption.acceleratorText: 		Ctrl+Del",
#else
    "*panelDeleteOption.acceleratorText: 		Ctrl+Delete",
#endif
    "*panelAddSelectedInteractorsOption.labelString:   Add Selected Interactor(s)",
    "*panelAddSelectedInteractorsOption.mnemonic:      S",
    "*panelShowSelectedInteractorsOption.labelString:  Show Selected Interactor(s)",
    "*panelShowSelectedInteractorsOption.mnemonic:     h",
    "*panelShowSelectedStandInsOption.labelString:     Show Selected Tool",
    "*panelShowSelectedStandInsOption.mnemonic:        S",
    "*panelCommentOption.labelString:                  Comment...",
    "*panelCommentOption.mnemonic:                     C",

    "*panelsMenu.labelString:                     Panels",
    "*panelsMenu.mnemonic:                        P",
    "*panelOpenAllControlPanelsOption.labelString:     Open All Control Panels",
    "*panelOpenAllControlPanelsOption.mnemonic:        A",
    "*panelOpenAllControlPanelsOption.accelerator:     Ctrl<Key>P",
    "*panelOpenAllControlPanelsOption.acceleratorText: Ctrl+P",
    "*panelOpenControlPanelByNameOption.labelString:   Open Control Panel by Name",
    "*panelPanelCascade.labelString:   		  Open Control Panel by Name",
    "*panelPanelGroupCascade.labelString:   		  Open Control Panel by Group",
    "*panelOpenControlPanelByNameOption.mnemonic:      N",

    "*optionsMenu.labelString:                    Options",
    "*optionsMenu.mnemonic:                       O",
    "*panelChangeControlPanelNameOption.labelString:   Change Control Panel Name...",
    "*panelChangeControlPanelNameOption.mnemonic:      C",
    "*panelSetAccessOption.labelString:                Control Panel Access...",
    "*panelSetAccessOption.mnemonic:                   A",
    "*panelGridOption.labelString:                     Grid...",
    "*panelGridOption.mnemonic:                        G",
    "*panelStyleOption.labelString:                    Dialog Style",
    "*panelStyleOption.mnemonic:                       D",
    "*toggleWindowStartupOption.labelString:           Startup Control Panel",

    "*hitDetectionOption.labelString:                  Prevent Overlap",
    "*hitDetectionOption.mnemonic:                     v",
    "*hitDetectionOption.accelerator:                  Ctrl<Key>V",
    "*hitDetectionOption.acceleratorText:              Ctrl+V",

    "*panelOnVisualProgramOption.labelString:          Application Comment...",
    "*panelOnVisualProgramOption.mnemonic:             A",
    "*panelOnControlPanelOption.labelString:           On Control Panel...",
    "*panelOnControlPanelOption.mnemonic:              P",
    "*panelOK.labelString:                             OK",
    "*panelCancel.labelString:                         Close",
    "*userHelpOption.labelString:                      Help",
    "*XmPushButtonGadget.shadowThickness:              1",

    NULL
};


#else /* defined(DX_NEW_KEYLAYOUT) */

String ControlPanel::DefaultResources[] =
{
    ".width:                                      	375",
    ".height:                                     	450",
    ".iconName:                                   	Control Panel",
    ".title:                                      	Control Panel",

    "*fileMenu.labelString:                       	File",
    "*fileMenu.mnemonic:                          	F",
    "*panelCloseOption.labelString:                    	Close",
    "*panelCloseOption.mnemonic:                       	C",
    "*panelCloseOption.accelerator:			Ctrl <Key>W",
    "*panelCloseOption.acceleratorText:			Ctrl+W",
    "*panelCloseAllOption.labelString:                 	Close All Control Panels",
    "*panelCloseAllOption.mnemonic:			P",
    "*panelCloseAllOption.accelerator:			Ctrl Shift <Key> W",
    "*panelCloseAllOption.acceleratorText:		Ctrl+Shift+W",

    "*stylePulldown.tearOffModel:		  	XmTEAR_OFF_DISABLED",
    "*dimensionalityePulldown.tearOffModel:	  	XmTEAR_OFF_DISABLED",
    "*layoutPulldown.tearOffModel:		  	XmTEAR_OFF_DISABLED",
    "*editMenuPulldown.tearOffModel:		  	XmTEAR_OFF_DISABLED",

    "*editMenu.labelString:                       	Edit",
    "*editMenu.mnemonic:                          	E",
    "*panelSetStyleButton.labelString:                 	Set Style",
    "*panelSetStyleButton.mnemonic:                    	y",
    "*panelSetDimensionalityButton.labelString:        	Set Dimensionality",
    "*panelSetDimensionalityButton.mnemonic:           	i",
    "*dimensionalityPulldown*set1DOption.labelString:   1 Dimensional",
    "*dimensionalityPulldown*set1DOption.mnemonic:	1",
    "*dimensionalityPulldown*set2DOption.labelString:   2 Dimensional",
    "*dimensionalityPulldown*set2DOption.mnemonic:	2",
    "*dimensionalityPulldown*set3DOption.labelString:   3 Dimensional",
    "*dimensionalityPulldown*set3DOption.mnemonic:	3",
    "*dimensionalityPulldown*set4DOption.labelString:   4 Dimensional",
    "*dimensionalityPulldown*set4DOption.mnemonic:	4",

    "*panelAddButton.labelString:    			Add Element",
    "*panelAddButton.mnemonic:    			A",
    "*panelSetLayoutButton.labelString:			Set Layout",
    "*panelSetLayoutButton.mnemonic:			L",
    "*panelVerticalOption.labelString:			Vertical",
    "*panelVerticalOption.mnemonic:			V",
    "*panelHorizontalOption.labelString:		Horizontal",
    "*panelHorizontalOption.mnemonic:			H",
    "*panelSetAttributesOption.labelString:		Set Attributes...",
    "*panelSetAttributesOption.mnemonic:		e",
    "*panelSetAttributesOption.accelerator:		Ctrl <Key>F",
    "*panelSetAttributesOption.acceleratorText:		Ctrl+F",
    "*panelSetInteractorLabelOption.labelString:	Set Label...",
    "*panelSetInteractorLabelOption.mnemonic:		t",

    "*panelDeleteOption.labelString:                   	Delete",
    "*panelDeleteOption.mnemonic:                      	D",

#if defined(intelnt)
    "*panelDeleteOption.acceleratorText:                  Ctrl+Backspace",
#else
    "*panelDeleteOption.acceleratorText:                  Ctrl+Delete",
#endif
#if defined(macos) || defined(intelnt)
    "*panelDeleteOption.accelerator:                      Ctrl<Key>BackSpace",
#elif defined(aviion)
    "*panelDeleteOption.accelerator:                      Ctrl<Key>Delete",
#else
    "*panelDeleteOption.accelerator:                      Ctrl<Key>osfDelete",
#endif

    "*panelAddSelectedInteractorsOption.labelString:	Add Selected Interactor(s)",
    "*panelAddSelectedInteractorsOption.mnemonic:      	R",
    "*panelShowSelectedInteractorsOption.labelString:  	Show Selected Interactor(s)",
    "*panelShowSelectedInteractorsOption.mnemonic:     	h",
    "*panelShowSelectedStandInsOption.labelString:     	Show Selected Tool",
    "*panelShowSelectedStandInsOption.mnemonic:        	S",
    "*panelCommentOption.labelString:                  	Comment...",
    "*panelCommentOption.mnemonic:                     	C",

    "*panelsMenu.labelString:                     	Panels",
    "*panelsMenu.mnemonic:                        	P",
    "*panelOpenAllControlPanelsOption.labelString:     	Open All Control Panels",
    "*panelOpenAllControlPanelsOption.mnemonic:        	A",
    "*panelOpenAllControlPanelsOption.accelerator:     	Ctrl Shift <Key>P",
    "*panelOpenAllControlPanelsOption.acceleratorText: 	Ctrl+Shift+P",
    "*panelOpenControlPanelByNameOption.labelString:   	Open Control Panel by Name",
    "*panelOpenControlPanelByNameOption.mnemonic:	N",
    "*panelPanelCascade.labelString:   		  	Open Control Panel by Name",
    "*panelPanelGroupCascade.labelString:   		Open Control Panel by Group",

    "*optionsMenu.labelString:                    	Options",
    "*optionsMenu.mnemonic:                       	O",
    "*panelChangeControlPanelNameOption.labelString:   Change Control Panel Name...",
    "*panelChangeControlPanelNameOption.mnemonic:      C",
    "*panelSetAccessOption.labelString:                Control Panel Access...",
    "*panelSetAccessOption.mnemonic:                   A",
    "*panelGridOption.labelString:                     Grid...",
    "*panelGridOption.mnemonic:                        G",
    "*panelStyleOption.labelString:                    Dialog Style",
    "*panelStyleOption.mnemonic:                       D",
    "*toggleWindowStartupOption.labelString:           Startup Control Panel",

    "*hitDetectionOption.labelString:                  Prevent Overlap",
    "*hitDetectionOption.mnemonic:                     v",

    "*panelOnVisualProgramOption.labelString:          Application Comment...",
    "*panelOnVisualProgramOption.mnemonic:             A",
    "*panelOnControlPanelOption.labelString:           On Control Panel...",
    "*panelOnControlPanelOption.mnemonic:              P",
    "*panelOK.labelString:                             OK",
    "*panelCancel.labelString:                         Close",
    "*userHelpOption.labelString:                      Help",
    "*XmPushButtonGadget.shadowThickness:              1",

    NULL
};

#endif

#endif // _CPDefaultResources_h

