/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#ifndef _CMDefaultResources_h
#define _CMDefaultResources_h

#if !defined(DX_NEW_KEYLAYOUT)

String ColormapEditor::DefaultResources[] =
{
    "*editor.accelerators:			#augment\n"
    "<Key>Return:				BulletinBoardReturn()",
    ".title:				    	Colormap Editor",
    ".width:				    	650",
    ".height:				   	450",
    ".minWidth:				 	550",
    ".minHeight:				300",
    ".allowResize:			      	True",
    ".iconName:				 	Color Map",

    "*fileMenu.labelString:                     File",
    "*fileMenu.mnemonic:                        F",
    "*cmeNewOption.labelString:                 New",
    "*cmeNewOption.mnemonic:                    N",
    "*cmeOpenOption.labelString:                Open...",
    "*cmeOpenOption.mnemonic:                   O",
    "*cmeSaveAsOption.labelString:              Save As...",
    "*cmeSaveAsOption.mnemonic:                 S",
    "*cmeCloseOption.labelString:               Close",
    "*cmeCloseOption.mnemonic:                  C",
    "*cmeCloseOption.accelerator:               Ctrl<Key>Q",
    "*cmeCloseOption.acceleratorText:           Ctrl+Q",

    "*editMenu.labelString:                     Edit",
    "*editMenu.mnemonic:                     	E",
    "*cmeUndoOption.labelString: 		Undo",
    "*cmeUndoOption.mnemonic:       		U",
    "*cmeUndoOption.accelerator:                Ctrl<Key>U",
    "*cmeUndoOption.acceleratorText:            Ctrl+U",
    "*cmeResetCascade.labelString:              Use Application Default",
    "*cmeResetCascade.mnemonic:                 R",
    "*cmeResetAllOption.labelString:		  All",
    "*cmeResetAllOption.mnemonic:		  A",
    "*cmeResetHSVOption.labelString:		  Color (HSV) Map",
    "*cmeResetHSVOption.mnemonic:		  C",
    "*cmeResetOpacityOption.labelString:	  Opacity Map",
    "*cmeResetOpacityOption.mnemonic:		  O",
    "*cmeResetMinMaxOption.labelString:		  Minimum & Maximum",
    "*cmeResetMinMaxOption.mnemonic:		  M",
    "*cmeResetMinOption.labelString:		  Minimum",
    "*cmeResetMinOption.mnemonic:		  n",
    "*cmeResetMaxOption.labelString:		  Maximum",
    "*cmeResetMaxOption.mnemonic:		  x",
    "*cmeCopyOption.labelString: 		Copy",
    "*cmeCopyOption.mnemonic:       		C",
    "*cmePasteOption.labelString:	 	Paste",
    "*cmePasteOption.mnemonic:       		P",
    "*cmeAddControlOption.labelString: 		Add Control Points...",
    "*cmeAddControlOption.mnemonic:       	A",
    "*cmeNBinsOption.labelString:	 	Number of histogram bins...",
    "*cmeNBinsOption.mnemonic:       		N",
    "*cmeWaveformOption.labelString: 		Generate Waveforms...",
    "*cmeWaveformOption.mnemonic:       	W",


    "*cmeDeleteSelectedOption.labelString:    	Delete Selected Control Points",
    "*cmeDeleteSelectedOption.mnemonic:     	D",
#ifdef aviion
    "*cmeDeleteSelectedOption.accelerator:	Ctrl<Key>Delete",
#else
    "*cmeDeleteSelectedOption.accelerator:	Ctrl<Key>osfDelete",
#endif
#ifdef sun4
    "*cmeDeleteSelectedOption.acceleratorText:  Ctrl+Del",
#else
    "*cmeDeleteSelectedOption.acceleratorText:  Ctrl+Delete",
#endif
    "*cmeSelectAllOption.labelString:    	Select All Control Points",
    "*cmeSelectAllOption.mnemonic:     		S",
    "*cmeSelectAllOption.accelerator:           Ctrl<Key>A",
    "*cmeSelectAllOption.acceleratorText:       Ctrl+A",

    "*optionsMenu.labelString:                  Options",
    "*optionsMenu.mnemonic:                     O",
    "*cmeSetBackgroundOption.labelString:       Set Background Style to Checkboard",
    "*cmeSetBackgroundOption.mnemonic:          B",
    "*cmeSetDrawModeOption.labelString:         Axis Display",
    "*cmeSetDrawModeOption.mnemonic:            A",
    "*cmeTicksOption.labelString:               	Ticks",
    "*cmeTicksOption.mnemonic:                  	T",
    "*cmeHistogramOption.labelString:           	Histogram",
    "*cmeHistogramOption.mnemonic:              	H",
    "*cmeLogHistogramOption.labelString:        	Log(Histogram)",
    "*cmeLogHistogramOption.mnemonic:           	L",
    "*cmeHorizontalOption.labelString:          Constrain Horizontal",
    "*cmeHorizontalOption.mnemonic:             H",
    "*cmeVerticalOption.labelString:            Constrain Vertical",
    "*cmeVerticalOption.mnemonic:               V",
    "*displayCPButton.labelString:		Display Control Point Data Value",
    "*displayCPButton.mnemonic:			D",
    "*displayCPOffOption.labelString:			Off",	
    "*displayCPOffOption.mnemonic:			O",	
    "*displayCPAllOption.labelString:			All",	
    "*displayCPAllOption.mnemonic:			A",	
    "*displayCPSelectedOption.labelString:		Selected",	
    "*displayCPSelectedOption.mnemonic:			S",	
    "*setColormapNameOption.labelString:     	Change Colormap Name...",
    "*setColormapNameOption.mnemonic:     	C",	

    "*cmapOnVisualProgramOption.labelString:       Application Comment...",
    "*cmapOnVisualProgramOption.mnemonic:          A",

    "*editMenuPulldown.tearOffModel:		XmTEAR_OFF_DISABLED",

    NULL
};



#else /* defined(DX_NEW_KEYLAYOUT) */


String ColormapEditor::DefaultResources[] =
{
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
    "*cmeResetCascade.labelString:              	Use Application Default",
    "*cmeResetCascade.mnemonic:                 	R",
    "*cmeResetAllOption.labelString:		  	All",
    "*cmeResetAllOption.mnemonic:		  	A",
    "*cmeResetHSVOption.labelString:		  	Color (HSV) Map",
    "*cmeResetHSVOption.mnemonic:		  	C",
    "*cmeResetOpacityOption.labelString:	  	Opacity Map",
    "*cmeResetOpacityOption.mnemonic:		  	O",
    "*cmeResetMinMaxOption.labelString:		  	Minimum & Maximum",
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
    "*cmeAddControlOption.labelString: 			Add Control Points...",
    "*cmeAddControlOption.mnemonic:       		A",
    "*cmeNBinsOption.labelString:	 		Number of histogram bins...",
    "*cmeNBinsOption.mnemonic:       			N",
    "*cmeWaveformOption.labelString: 			Generate Waveforms...",
    "*cmeWaveformOption.mnemonic:       		W",


    "*cmeDeleteSelectedOption.labelString:    		Delete Selected Control Points",
    "*cmeDeleteSelectedOption.mnemonic:     		D",

#if defined(intelnt)
    "*cmeDeleteSelectedOption.acceleratorText:          Ctrl+Backspace",
#else
    "*cmeDeleteSelectedOption.acceleratorText:          Ctrl+Delete",
#endif
#if defined(macos) || defined(intelnt)
    "*cmeDeleteSelectedOption.accelerator:              Ctrl<Key>BackSpace",
#elif defined(aviion)
    "*cmeDeleteSelectedOption.accelerator:              Ctrl<Key>Delete",
#else
    "*cmeDeleteSelectedOption.accelerator:              Ctrl<Key>osfDelete",
#endif

    "*cmeSelectAllOption.labelString:    		Select All Control Points",
    "*cmeSelectAllOption.mnemonic:     			S",
    "*cmeSelectAllOption.accelerator:           	Ctrl<Key>A",
    "*cmeSelectAllOption.acceleratorText:       	Ctrl+A",

    "*optionsMenu.labelString:                  	Options",
    "*optionsMenu.mnemonic:                     	O",
    "*cmeSetBackgroundOption.labelString:       	Set Background Style to Checkboard",
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
    "*displayCPButton.labelString:			Display Control Point Data Value",
    "*displayCPButton.mnemonic:				D",
    "*displayCPOffOption.labelString:			Off",	
    "*displayCPOffOption.mnemonic:			O",	
    "*displayCPAllOption.labelString:			All",	
    "*displayCPAllOption.mnemonic:			A",	
    "*displayCPSelectedOption.labelString:		Selected",	
    "*displayCPSelectedOption.mnemonic:			S",	
    "*setColormapNameOption.labelString:     		Change Colormap Name...",
    "*setColormapNameOption.mnemonic:     		C",	

    "*cmapOnVisualProgramOption.labelString:       	Application Comment...",
    "*cmapOnVisualProgramOption.mnemonic:          	A",

    "*editMenuPulldown.tearOffModel:			XmTEAR_OFF_DISABLED",

    NULL
};




#endif

#endif // _CMDefaultResources_h

