/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#ifndef _CMDefaultResources_h
#define _CMDefaultResources_h

#if !defined( DX_NEW_KEYLAYOUT )

String ColormapEditor::DefaultResources[] = {
    (char *)"*editor.accelerators:			#augment\n"
            "<Key>Return:				BulletinBoardReturn()",
    (char *)".title:				    	Colormap Editor",
    (char *)".width:				    	650",
    (char *)".height:				   	450",
    (char *)".minWidth:				 	550",
    (char *)".minHeight:				300",
    (char *)".allowResize:			      	True",
    (char *)".iconName:				 	Color Map",
    (char *)"*fileMenu.labelString:                     File",
    (char *)"*fileMenu.mnemonic:                        F",
    (char *)"*cmeNewOption.labelString:                 New",
    (char *)"*cmeNewOption.mnemonic:                    N",
    (char *)"*cmeOpenOption.labelString:                Open...",
    (char *)"*cmeOpenOption.mnemonic:                   O",
    (char *)"*cmeSaveAsOption.labelString:              Save As...",
    (char *)"*cmeSaveAsOption.mnemonic:                 S",
    (char *)"*cmeCloseOption.labelString:               Close",
    (char *)"*cmeCloseOption.mnemonic:                  C",
    (char *)"*cmeCloseOption.accelerator:               Ctrl<Key>Q",
    (char *)"*cmeCloseOption.acceleratorText:           Ctrl+Q",
    (char *)"*editMenu.labelString:                     Edit",
    (char *)"*editMenu.mnemonic:                     	E",
    (char *)"*cmeUndoOption.labelString: 		Undo",
    (char *)"*cmeUndoOption.mnemonic:       		U",
    (char *)"*cmeUndoOption.accelerator:                Ctrl<Key>U",
    (char *)"*cmeUndoOption.acceleratorText:            Ctrl+U",
    (char *)"*cmeResetCascade.labelString:              Use Application "
            "Default",
    (char *)"*cmeResetCascade.mnemonic:                 R",
    (char *)"*cmeResetAllOption.labelString:		  All",
    (char *)"*cmeResetAllOption.mnemonic:		  A",
    (char *)"*cmeResetHSVOption.labelString:		  Color (HSV) Map",
    (char *)"*cmeResetHSVOption.mnemonic:		  C",
    (char *)"*cmeResetOpacityOption.labelString:	  Opacity Map",
    (char *)"*cmeResetOpacityOption.mnemonic:		  O",
    (char *)"*cmeResetMinMaxOption.labelString:		  Minimum & "
            "Maximum",
    (char *)"*cmeResetMinMaxOption.mnemonic:		  M",
    (char *)"*cmeResetMinOption.labelString:		  Minimum",
    (char *)"*cmeResetMinOption.mnemonic:		  n",
    (char *)"*cmeResetMaxOption.labelString:		  Maximum",
    (char *)"*cmeResetMaxOption.mnemonic:		  x",
    (char *)"*cmeCopyOption.labelString: 		Copy",
    (char *)"*cmeCopyOption.mnemonic:       		C",
    (char *)"*cmePasteOption.labelString:	 	Paste",
    (char *)"*cmePasteOption.mnemonic:       		P",
    (char *)"*cmeAddControlOption.labelString: 		Add Control "
            "Points...",
    (char *)"*cmeAddControlOption.mnemonic:       	A",
    (char *)"*cmeNBinsOption.labelString:	 	Number of histogram "
            "bins...",
    (char *)"*cmeNBinsOption.mnemonic:       		N",
    (char *)"*cmeWaveformOption.labelString: 		Generate Waveforms...",
    (char *)"*cmeWaveformOption.mnemonic:       	W",
    (char *)"*cmeDeleteSelectedOption.labelString:    	Delete Selected "
            "Control Points",
    (char *)"*cmeDeleteSelectedOption.mnemonic:     	D",
#ifdef aviion
    (char *)"*cmeDeleteSelectedOption.accelerator:	Ctrl<Key>Delete",
#else
    (char *)"*cmeDeleteSelectedOption.accelerator:	Ctrl<Key>osfDelete",
#endif
#ifdef sun4
    (char *)"*cmeDeleteSelectedOption.acceleratorText:  Ctrl+Del",
#else
    (char *)"*cmeDeleteSelectedOption.acceleratorText:  Ctrl+Delete",
#endif
    (char *)"*cmeSelectAllOption.labelString:    	Select All Control "
            "Points",
    (char *)"*cmeSelectAllOption.mnemonic:     		S",
    (char *)"*cmeSelectAllOption.accelerator:           Ctrl<Key>A",
    (char *)"*cmeSelectAllOption.acceleratorText:       Ctrl+A",
    (char *)"*optionsMenu.labelString:                  Options",
    (char *)"*optionsMenu.mnemonic:                     O",
    (char *)"*cmeSetBackgroundOption.labelString:       Set Background Style "
            "to Checkboard",
    (char *)"*cmeSetBackgroundOption.mnemonic:          B",
    (char *)"*cmeSetDrawModeOption.labelString:         Axis Display",
    (char *)"*cmeSetDrawModeOption.mnemonic:            A",
    (char *)"*cmeTicksOption.labelString:               	Ticks",
    (char *)"*cmeTicksOption.mnemonic:                  	T",
    (char *)"*cmeHistogramOption.labelString:           	Histogram",
    (char *)"*cmeHistogramOption.mnemonic:              	H",
    (char *)"*cmeLogHistogramOption.labelString:        	Log(Histogram)",
    (char *)"*cmeLogHistogramOption.mnemonic:           	L",
    (char *)"*cmeHorizontalOption.labelString:          Constrain Horizontal",
    (char *)"*cmeHorizontalOption.mnemonic:             H",
    (char *)"*cmeVerticalOption.labelString:            Constrain Vertical",
    (char *)"*cmeVerticalOption.mnemonic:               V",
    (char *)"*displayCPButton.labelString:		Display Control Point "
            "Data Value",
    (char *)"*displayCPButton.mnemonic:			D",
    (char *)"*displayCPOffOption.labelString:			Off",
    (char *)"*displayCPOffOption.mnemonic:			O",
    (char *)"*displayCPAllOption.labelString:			All",
    (char *)"*displayCPAllOption.mnemonic:			A",
    (char *)"*displayCPSelectedOption.labelString:		Selected",
    (char *)"*displayCPSelectedOption.mnemonic:			S",
    (char *)"*setColormapNameOption.labelString:     	Change Colormap "
            "Name...",
    (char *)"*setColormapNameOption.mnemonic:     	C",
    (char *)"*cmapOnVisualProgramOption.labelString:       Application "
            "Comment...",
    (char *)"*cmapOnVisualProgramOption.mnemonic:          A",
    (char *)"*editMenuPulldown.tearOffModel:		XmTEAR_OFF_DISABLED",
    (char *)NULL};

#else /* defined(DX_NEW_KEYLAYOUT) */

String ColormapEditor::DefaultResources[] = {
    "*editor.accelerators:			#augment\n"
    "<Key>Return:				BulletinBoardReturn()",
    ".title:				    		Colormap Editor",
    ".width:				    		650",
    ".height:				   		450",
    ".minWidth:				 		550",
    ".minHeight:					300",
    ".allowResize:			      		True",
    ".iconName:				 		Color Map",
    "*fileMenu.labelString:                     	File",
    "*fileMenu.mnemonic:                        	F",
    "*cmeNewOption.labelString:                 	New",
    "*cmeNewOption.mnemonic:                    	N",
    "*cmeNewOption.accelerator:				Ctrl <Key>N",
    "*cmeNewOption.acceleratorText:			Ctrl+N",
    "*cmeOpenOption.labelString:                	Open...",
    "*cmeOpenOption.mnemonic:                   	O",
    "*cmeOpenOption.accelerator:			Ctrl <Key>O",
    "*cmeOpenOption.acceleratorText:			Ctrl+O",
    "*cmeSaveAsOption.labelString:              	Save As...",
    "*cmeSaveAsOption.mnemonic:                 	S",
    "*cmeSaveAsOption.accelerator:			Ctrl Shift <Key>S",
    "*cmeSaveAsOption.acceleratorText:			Ctrl+Shift+S",
    "*cmeCloseOption.labelString:               	Close",
    "*cmeCloseOption.mnemonic:                  	C",
    "*cmeCloseOption.accelerator:               	Ctrl<Key>W",
    "*cmeCloseOption.acceleratorText:           	Ctrl+W",
    "*editMenu.labelString:                     	Edit",
    "*editMenu.mnemonic:                     		E",
    "*cmeUndoOption.labelString: 			Undo",
    "*cmeUndoOption.mnemonic:       			U",
    "*cmeUndoOption.accelerator:                	Ctrl<Key>Z",
    "*cmeUndoOption.acceleratorText:            	Ctrl+Z",
    "*cmeResetCascade.labelString:              	Use Application "
    "Default",
    "*cmeResetCascade.mnemonic:                 	R",
    "*cmeResetAllOption.labelString:		  	All",
    "*cmeResetAllOption.mnemonic:		  	A",
    "*cmeResetHSVOption.labelString:		  	Color (HSV) Map",
    "*cmeResetHSVOption.mnemonic:		  	C",
    "*cmeResetOpacityOption.labelString:	  	Opacity Map",
    "*cmeResetOpacityOption.mnemonic:		  	O",
    "*cmeResetMinMaxOption.labelString:		  	Minimum & "
    "Maximum",
    "*cmeResetMinMaxOption.mnemonic:		  	M",
    "*cmeResetMinOption.labelString:		  	Minimum",
    "*cmeResetMinOption.mnemonic:		  	n",
    "*cmeResetMaxOption.labelString:		 	Maximum",
    "*cmeResetMaxOption.mnemonic:		 	x",
    "*cmeCopyOption.labelString: 			Copy",
    "*cmeCopyOption.mnemonic:       			C",
    "*cmeCopyOption.accelerator:			Ctrl <Key>C",
    "*cmeCopyOption.acceleratorText:			Ctrl+C",
    "*cmePasteOption.labelString:	 		Paste",
    "*cmePasteOption.mnemonic:       			P",
    "*cmePasteOption.accelerator:			Ctrl <Key>V",
    "*cmePasteOption.acceleratorText:			Ctrl+V",
    "*cmeAddControlOption.labelString: 			Add Control "
    "Points...",
    "*cmeAddControlOption.mnemonic:       		A",
    "*cmeNBinsOption.labelString:	 		Number of histogram "
    "bins...",
    "*cmeNBinsOption.mnemonic:       			N",
    "*cmeWaveformOption.labelString: 			Generate Waveforms...",
    "*cmeWaveformOption.mnemonic:       		W",
    "*cmeDeleteSelectedOption.labelString:    		Delete Selected "
    "Control Points",
    "*cmeDeleteSelectedOption.mnemonic:     		D",

#if defined( intelnt )
    "*cmeDeleteSelectedOption.acceleratorText:          Ctrl+Backspace",
#else
    "*cmeDeleteSelectedOption.acceleratorText:          Ctrl+Delete",
#endif
#if defined( macos ) || defined( intelnt )
    "*cmeDeleteSelectedOption.accelerator:              Ctrl<Key>BackSpace",
#elif defined( aviion )
    "*cmeDeleteSelectedOption.accelerator:              Ctrl<Key>Delete",
#else
    "*cmeDeleteSelectedOption.accelerator:              Ctrl<Key>osfDelete",
#endif
    "*cmeSelectAllOption.labelString:    		Select All Control "
    "Points",
    "*cmeSelectAllOption.mnemonic:     			S",
    "*cmeSelectAllOption.accelerator:           	Ctrl<Key>A",
    "*cmeSelectAllOption.acceleratorText:       	Ctrl+A",
    "*optionsMenu.labelString:                  	Options",
    "*optionsMenu.mnemonic:                     	O", "*cmeSetBackgroundO"
                                                            "ption.labelString:"
                                                            "       	Set "
                                                            "Background Style "
                                                            "to Checkboard",
    "*cmeSetBackgroundOption.mnemonic:          	B",
    "*cmeSetDrawModeOption.labelString:         	Axis Display",
    "*cmeSetDrawModeOption.mnemonic:            	A",
    "*cmeTicksOption.labelString:               	Ticks",
    "*cmeTicksOption.mnemonic:                  	T",
    "*cmeHistogramOption.labelString:           	Histogram",
    "*cmeHistogramOption.mnemonic:              	H",
    "*cmeLogHistogramOption.labelString:        	Log(Histogram)",
    "*cmeLogHistogramOption.mnemonic:           	L",
    "*cmeHorizontalOption.labelString:          	Constrain Horizontal",
    "*cmeHorizontalOption.mnemonic:             	H",
    "*cmeVerticalOption.labelString:            	Constrain Vertical",
    "*cmeVerticalOption.mnemonic:               	V",
    "*displayCPButton.labelString:			Display Control Point "
    "Data Value",
    "*displayCPButton.mnemonic:				D",
    "*displayCPOffOption.labelString:			Off",
    "*displayCPOffOption.mnemonic:			O",
    "*displayCPAllOption.labelString:			All",
    "*displayCPAllOption.mnemonic:			A",
    "*displayCPSelectedOption.labelString:		Selected",
    "*displayCPSelectedOption.mnemonic:			S",
    "*setColormapNameOption.labelString:     		Change Colormap "
    "Name...",
    "*setColormapNameOption.mnemonic:     		C",
    "*cmapOnVisualProgramOption.labelString:       	Application Comment...",
    "*cmapOnVisualProgramOption.mnemonic:          	A",
    "*editMenuPulldown.tearOffModel:			XmTEAR_OFF_DISABLED",
    NULL};

#endif

#endif  // _CMDefaultResources_h
