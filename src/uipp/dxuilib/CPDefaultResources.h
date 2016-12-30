/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#ifndef _CPDefaultResources_h
#define _CPDefaultResources_h

#if !defined( DX_NEW_KEYLAYOUT )

String ControlPanel::DefaultResources[] = {
    (char *)".width:                                      375",
    (char *)".height:                                     450",
    (char *)".iconName:                                   Control Panel",
    (char *)".title:                                      Control Panel",
    (char *)"*fileMenu.labelString:                       File",
    (char *)"*fileMenu.mnemonic:                          F",
    (char *)"*panelOpenOption.labelString:                     Open...",
    (char *)"*panelOpenOption.mnemonic:                        O",
    (char *)"*panelSaveAsOption.labelString:                   Save As...",
    (char *)"*panelSaveAsOption.mnemonic:                      S",
    (char *)"*panelCloseOption.labelString:                    Close",
    (char *)"*panelCloseOption.mnemonic:                       C",
    (char *)"*panelCloseAllOption.labelString:                 Close All "
            "Control Panels",
    (char *)"*stylePulldown.tearOffModel:		  XmTEAR_OFF_DISABLED",
    (char *)"*dimensionalityePulldown.tearOffModel:	  XmTEAR_OFF_DISABLED",
    (char *)"*layoutPulldown.tearOffModel:		  XmTEAR_OFF_DISABLED",
    (char *)"*editMenuPulldown.tearOffModel:		  XmTEAR_OFF_DISABLED",
    (char *)"*editMenu.labelString:                       Edit",
    (char *)"*editMenu.mnemonic:                          E",
    (char *)"*panelSetStyleButton.labelString:                 Set Style",
    (char *)"*panelSetStyleButton.mnemonic:                    y",
    (char *)"*panelSetDimensionalityButton.labelString:        Set "
            "Dimensionality",
    (char *)"*panelSetDimensionalityButton.mnemonic:           i",
    (char *)"*dimensionalityPulldown*set1DOption.labelString:      1 "
            "Dimensional",
    (char *)"*dimensionalityPulldown*set2DOption.labelString:      2 "
            "Dimensional",
    (char *)"*dimensionalityPulldown*set3DOption.labelString:      3 "
            "Dimensional",
    (char *)"*dimensionalityPulldown*set4DOption.labelString:      4 "
            "Dimensional",
    (char *)"*panelAddButton.labelString:    		       Add Element",
    (char *)"*panelAddButton.mnemonic:    		       A",
    (char *)"*panelSetLayoutButton.labelString:                Set Layout",
    (char *)"*panelSetLayoutButton.mnemonic:                   L",
    (char *)"*panelVerticalOption.labelString:                 Vertical",
    (char *)"*panelHorizontalOption.labelString:               Horizontal",
    (char *)"*panelSetAttributesOption.labelString:            Set "
            "Attributes...",
    (char *)"*panelSetAttributesOption.mnemonic:               e",
    (char *)"*panelSetInteractorLabelOption.labelString:       Set Label...",
    (char *)"*panelSetInteractorLabelOption.mnemonic:          t",
    (char *)"*panelDeleteOption.labelString:                   Delete",
    (char *)"*panelDeleteOption.mnemonic:                      D",
#if defined( aviion )
    "*panelDeleteOption.accelerator:                   Ctrl<Key>Delete",
#else
    (char *)"*panelDeleteOption.accelerator:                   "
            "Ctrl<Key>osfDelete",
#endif
#ifdef sun4
    (char *)"*panelDeleteOption.acceleratorText: 		Ctrl+Del",
#else
    (char *)"*panelDeleteOption.acceleratorText: 		Ctrl+Delete",
#endif
    (char *)"*panelAddSelectedInteractorsOption.labelString:   Add Selected "
            "Interactor(s)",
    (char *)"*panelAddSelectedInteractorsOption.mnemonic:      S",
    (char *)"*panelShowSelectedInteractorsOption.labelString:  Show Selected "
            "Interactor(s)",
    (char *)"*panelShowSelectedInteractorsOption.mnemonic:     h",
    (char *)"*panelShowSelectedStandInsOption.labelString:     Show Selected "
            "Tool",
    (char *)"*panelShowSelectedStandInsOption.mnemonic:        S",
    (char *)"*panelCommentOption.labelString:                  Comment...",
    (char *)"*panelCommentOption.mnemonic:                     C",
    (char *)"*panelsMenu.labelString:                     Panels",
    (char *)"*panelsMenu.mnemonic:                        P",
    (char *)"*panelOpenAllControlPanelsOption.labelString:     Open All "
            "Control Panels",
    (char *)"*panelOpenAllControlPanelsOption.mnemonic:        A",
    (char *)"*panelOpenAllControlPanelsOption.accelerator:     Ctrl<Key>P",
    (char *)"*panelOpenAllControlPanelsOption.acceleratorText: Ctrl+P",
    (char *)"*panelOpenControlPanelByNameOption.labelString:   Open Control "
            "Panel by Name",
    (char *)"*panelPanelCascade.labelString:   		  Open Control "
            "Panel by Name",
    (char *)"*panelPanelGroupCascade.labelString:   		  Open Control "
            "Panel by Group",
    (char *)"*panelOpenControlPanelByNameOption.mnemonic:      N",
    (char *)"*optionsMenu.labelString:                    Options",
    (char *)"*optionsMenu.mnemonic:                       O",
    (char *)"*panelChangeControlPanelNameOption.labelString:   Change Control "
            "Panel Name...",
    (char *)"*panelChangeControlPanelNameOption.mnemonic:      C",
    (char *)"*panelSetAccessOption.labelString:                Control Panel "
            "Access...",
    (char *)"*panelSetAccessOption.mnemonic:                   A",
    (char *)"*panelGridOption.labelString:                     Grid...",
    (char *)"*panelGridOption.mnemonic:                        G",
    (char *)"*panelStyleOption.labelString:                    Dialog Style",
    (char *)"*panelStyleOption.mnemonic:                       D",
    (char *)"*toggleWindowStartupOption.labelString:           Startup Control "
            "Panel",
    (char *)"*hitDetectionOption.labelString:                  Prevent Overlap",
    (char *)"*hitDetectionOption.mnemonic:                     v",
    (char *)"*hitDetectionOption.accelerator:                  Ctrl<Key>V",
    (char *)"*hitDetectionOption.acceleratorText:              Ctrl+V",
    (char *)"*panelOnVisualProgramOption.labelString:          Application "
            "Comment...",
    (char *)"*panelOnVisualProgramOption.mnemonic:             A",
    (char *)"*panelOnControlPanelOption.labelString:           On Control "
            "Panel...",
    (char *)"*panelOnControlPanelOption.mnemonic:              P",
    (char *)"*panelOK.labelString:                             OK",
    (char *)"*panelCancel.labelString:                         Close",
    (char *)"*userHelpOption.labelString:                      Help",
    (char *)"*XmPushButtonGadget.shadowThickness:              1",
    (char *)NULL};

#else /* defined(DX_NEW_KEYLAYOUT) */

String ControlPanel::DefaultResources[] = {
    (char *)".width:                                      	375",
    (char *)".height:                                     	450",
    (char *)".iconName:                                   	Control Panel",
    (char *)".title:                                      	Control Panel",
    (char *)"*fileMenu.labelString:                       	File",
    (char *)"*fileMenu.mnemonic:                          	F",
    (char *)"*panelCloseOption.labelString:                    	Close",
    (char *)"*panelCloseOption.mnemonic:                       	C",
    (char *)"*panelCloseOption.accelerator:			Ctrl <Key>W",
    (char *)"*panelCloseOption.acceleratorText:			Ctrl+W",
    (char *)"*panelCloseAllOption.labelString:                 	Close All "(
        char *)"Control Panels",
    (char *)"*panelCloseAllOption.mnemonic:			P",
    (char *)"*panelCloseAllOption.accelerator:			Ctrl Shift "
            "<Key> W",
    (char *)"*panelCloseAllOption.acceleratorText:		Ctrl+Shift+W",
    (char *)"*stylePulldown.tearOffModel:		  	"
            "XmTEAR_OFF_DISABLED",
    (char *)"*dimensionalityePulldown.tearOffModel:	  	"
            "XmTEAR_OFF_DISABLED",
    (char *)"*layoutPulldown.tearOffModel:		  	"
            "XmTEAR_OFF_DISABLED",
    (char *)"*editMenuPulldown.tearOffModel:		  	"
            "XmTEAR_OFF_DISABLED",
    (char *)"*editMenu.labelString:                       	Edit",
    (char *)"*editMenu.mnemonic:                          	E",
    (char *)"*panelSetStyleButton.labelString:                 	Set Style",
    (char *)"*panelSetStyleButton.mnemonic:                    	y",
    (char *)"*panelSetDimensionalityButton.labelString:        	Set "
            "Dimensionality",
    (char *)"*panelSetDimensionalityButton.mnemonic:           	i",
    (char *)"*dimensionalityPulldown*set1DOption.labelString:   1 Dimensional",
    (char *)"*dimensionalityPulldown*set1DOption.mnemonic:	1",
    (char *)"*dimensionalityPulldown*set2DOption.labelString:   2 Dimensional",
    (char *)"*dimensionalityPulldown*set2DOption.mnemonic:	2",
    (char *)"*dimensionalityPulldown*set3DOption.labelString:   3 Dimensional",
    (char *)"*dimensionalityPulldown*set3DOption.mnemonic:	3",
    (char *)"*dimensionalityPulldown*set4DOption.labelString:   4 Dimensional",
    (char *)"*dimensionalityPulldown*set4DOption.mnemonic:	4",
    (char *)"*panelAddButton.labelString:    			Add Element",
    (char *)"*panelAddButton.mnemonic:    			A",
    (char *)"*panelSetLayoutButton.labelString:			Set Layout",
    (char *)"*panelSetLayoutButton.mnemonic:			L",
    (char *)"*panelVerticalOption.labelString:			Vertical",
    (char *)"*panelVerticalOption.mnemonic:			V",
    (char *)"*panelHorizontalOption.labelString:		Horizontal",
    (char *)"*panelHorizontalOption.mnemonic:			H",
    (char *)"*panelSetAttributesOption.labelString:		Set "
            "Attributes...",
    (char *)"*panelSetAttributesOption.mnemonic:		e",
    (char *)"*panelSetAttributesOption.accelerator:		Ctrl <Key>F",
    (char *)"*panelSetAttributesOption.acceleratorText:		Ctrl+F",
    (char *)"*panelSetInteractorLabelOption.labelString:	Set Label...",
    (char *)"*panelSetInteractorLabelOption.mnemonic:		t",
    (char *)"*panelDeleteOption.labelString:                   	Delete",
    (char *)"*panelDeleteOption.mnemonic:                      	D",

#if defined( intelnt )
    (char *)"*panelDeleteOption.acceleratorText:                  "
            "Ctrl+Backspace",
#else
    (char *)"*panelDeleteOption.acceleratorText:                  Ctrl+Delete",
#endif
#if defined( macos ) || defined( intelnt )
    (char *)"*panelDeleteOption.accelerator:                      "
            "Ctrl<Key>BackSpace",
#elif defined( aviion )
    (char *)"*panelDeleteOption.accelerator:                      "
            "Ctrl<Key>Delete",
#else
    (char *)"*panelDeleteOption.accelerator:                      "
            "Ctrl<Key>osfDelete",
#endif
    (char *)"*panelAddSelectedInteractorsOption.labelString:	Add Selected "
            "Interactor(s)",
    (char *)"*panelAddSelectedInteractorsOption.mnemonic:      	R",
    (char *)"*panelShowSelectedInteractorsOption.labelString:  	Show Selected "
            "Interactor(s)",
    (char *)"*panelShowSelectedInteractorsOption.mnemonic:     	h",
    (char *)"*panelShowSelectedStandInsOption.labelString:     	Show "
            "Selected Tool",
    (char *)"*panelShowSelectedStandInsOption.mnemonic:        	S",
    (char *)"*panelCommentOption.labelString:                  	Comment...",
    (char *)"*panelCommentOption.mnemonic:                     	C",
    (char *)"*panelsMenu.labelString:                     	Panels",
    (char *)"*panelsMenu.mnemonic:                        	P",
    (char *)"*panelOpenAllControlPanelsOption.labelString:     	Open All "
            "Control Panels",
    (char *)"*panelOpenAllControlPanelsOption.mnemonic:        	A",
    (char *)"*panelOpenAllControlPanelsOption.accelerator:     	Ctrl "
            "Shift <Key>P",
    (char *)"*panelOpenAllControlPanelsOption.acceleratorText: 	Ctrl+Shift+P",
    (char *)"*panelOpenControlPanelByNameOption.labelString:   	Open Control "
            "Panel by Name",
    (char *)"*panelOpenControlPanelByNameOption.mnemonic:	N",
    (char *)"*panelPanelCascade.labelString:   		  	Open Control "
            "Panel by Name",
    (char *)"*panelPanelGroupCascade.labelString:   		Open Control "
            "Panel by Group",
    (char *)"*optionsMenu.labelString:                    	Options",
    (char *)"*optionsMenu.mnemonic:                       	O",
    (char *)"*panelChangeControlPanelNameOption.labelString:   Change Control "
            "Panel "
            "Name...",
    (char *)"*panelChangeControlPanelNameOption.mnemonic:      C",
    (char *)"*panelSetAccessOption.labelString:                Control Panel "
            "Access...",
    (char *)"*panelSetAccessOption.mnemonic:                   A",
    (char *)"*panelGridOption.labelString:                     Grid...",
    (char *)"*panelGridOption.mnemonic:                        G",
    (char *)"*panelStyleOption.labelString:                    Dialog Style",
    (char *)"*panelStyleOption.mnemonic:                       D",
    (char *)"*toggleWindowStartupOption.labelString:           Startup Control "
            "Panel",
    (char *)"*hitDetectionOption.labelString:                  Prevent Overlap",
    (char *)"*hitDetectionOption.mnemonic:                     v",
    (char *)"*panelOnVisualProgramOption.labelString:          Application "
            "Comment...",
    (char *)"*panelOnVisualProgramOption.mnemonic:             A",
    (char *)"*panelOnControlPanelOption.labelString:           On Control "
            "Panel...",
    (char *)"*panelOnControlPanelOption.mnemonic:              P",
    (char *)"*panelOK.labelString:                             OK",
    (char *)"*panelCancel.labelString:                         Close",
    (char *)"*userHelpOption.labelString:                      Help",
    (char *)"*XmPushButtonGadget.shadowThickness:              1",
    (char *)NULL};

#endif

#endif  // _CPDefaultResources_h
