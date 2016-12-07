/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#ifndef _MWDefaultResources_h
#define _MWDefaultResources_h

#if !defined(DX_NEW_KEYLAYOUT)

String  MsgWin::DefaultResources[] = {
    ".title:				Message Window",
    ".iconName:				Message Window",
    "*fileMenu.labelString:		File",
    "*allowShellResize:			False",
    "*fileMenu.mnemonic:		F",
    "*msgClearOption.labelString:	Clear",
    "*msgClearOption.mnemonic:		C",
    "*msgLogOption.labelString:		Log...",
    "*msgLogOption.mnemonic:		g",
    "*msgSaveOption.labelString:	Save As...",
    "*msgSaveOption.mnemonic:		S",
    "*msgCloseOption.labelString:	Close",
    "*msgCloseOption.mnemonic:		l",
    "*msgCloseOption.accelerator:	Ctrl<Key>Q",
    "*msgCloseOption.acceleratorText:	Ctrl+Q",
    "*editMenu.labelString:		Edit",
    "*editMenu.mnemonic:		E",
    "*commandsMenu.labelString:		Commands",
    "*commandsMenu.mnemonic:		C",
    "*msgShowMemoryOption.labelString:	Show Memory Use",
    "*msgShowMemoryOption.mnemonic:	M",
    "*msgTraceOption.labelString:	Debug Tracing",
    "*msgTraceOption.mnemonic:		T",
    "*msgExecScriptOption.labelString:	Execute Script Command...",
    "*msgExecScriptOption.mnemonic:	S",
    "*msgNextErrorOption.labelString:	Next Error",
    "*msgNextErrorOption.mnemonic:	N",
    "*msgNextErrorOption.accelerator:	Ctrl<Key>N",
    "*msgNextErrorOption.acceleratorText:	Ctrl+N",
    "*msgPrevErrorOption.labelString:	Previous Error",
    "*msgPrevErrorOption.mnemonic:	P",
    "*msgPrevErrorOption.accelerator:	Ctrl<Key>P",
    "*msgPrevErrorOption.acceleratorText:	Ctrl+P",
    "*optionsMenu.labelString:		Options",
    "*optionsMenu.mnemonic:		O",
    "*msgInfoOption.labelString:	Information Messages",
    "*msgInfoOption.mnemonic:		I",
    "*msgWarningOption.labelString:	Warning Messages",
    "*msgWarningOption.mnemonic:	W",
    "*msgErrorOption.labelString:	Error Messages",
    "*msgErrorOption.mnemonic:		E",
    "*msgFrame.shadowThickness:		2",
    "*msgFrame.marginWidth:		3",
    "*msgFrame.marginHeight:		3",
    "*msgList.visibleItemCount:		12",
    "*workArea.width:			600",
    NULL
};

#else /* defined(DX_NEW_KEYLAYOUT) */

String  MsgWin::DefaultResources[] = {
    ".title:				Message Window",
    ".iconName:				Message Window",
    "*fileMenu.labelString:		File",
    "*allowShellResize:			False",
    "*fileMenu.mnemonic:		F",
    "*msgLogOption.labelString:		Log...",
    "*msgLogOption.mnemonic:		g",
    "*msgSaveOption.labelString:	Save As...",
    "*msgSaveOption.mnemonic:		S",
    "*msgCloseOption.labelString:	Close",
    "*msgCloseOption.mnemonic:		l",
    "*msgCloseOption.accelerator:	Ctrl<Key>W",
    "*msgCloseOption.acceleratorText:	Ctrl+W",
    "*editMenu.labelString:		Edit",
    "*editMenu.mnemonic:		E",
    "*msgClearOption.labelString:	Clear",
    "*msgClearOption.mnemonic:		C",

#if defined(intelnt)
    "*msgClearOption.acceleratorText:   Ctrl+Backspace",
#else
    "*msgClearOption.acceleratorText:   Ctrl+Delete",
#endif
#if defined(macos) || defined(intelnt)
    "*msgClearOption.accelerator:       Ctrl<Key>BackSpace",
#elif defined(aviion)
    "*msgClearOption.accelerator:       Ctrl<Key>Delete",
#else
    "*msgClearOption.accelerator:       Ctrl<Key>osfDelete",
#endif


    "*commandsMenu.labelString:		Commands",
    "*commandsMenu.mnemonic:		C",
    "*msgShowMemoryOption.labelString:	Show Memory Use",
    "*msgShowMemoryOption.mnemonic:	M",
    "*msgTraceOption.labelString:	Debug Tracing",
    "*msgTraceOption.mnemonic:		T",
    "*msgExecScriptOption.labelString:	Execute Script Command...",
    "*msgExecScriptOption.mnemonic:	S",
    "*msgNextErrorOption.labelString:	Next Error",
    "*msgNextErrorOption.mnemonic:	N",
    "*msgNextErrorOption.accelerator:	Ctrl<Key>N",
    "*msgNextErrorOption.acceleratorText:	Ctrl+N",
    "*msgPrevErrorOption.labelString:	Previous Error",
    "*msgPrevErrorOption.mnemonic:	P",
    "*msgPrevErrorOption.accelerator:	Ctrl<Key>P",
    "*msgPrevErrorOption.acceleratorText:	Ctrl+P",
    "*optionsMenu.labelString:		Options",
    "*optionsMenu.mnemonic:		O",
    "*msgInfoOption.labelString:	Information Messages",
    "*msgInfoOption.mnemonic:		I",
    "*msgWarningOption.labelString:	Warning Messages",
    "*msgWarningOption.mnemonic:	W",
    "*msgErrorOption.labelString:	Error Messages",
    "*msgErrorOption.mnemonic:		E",
    "*msgFrame.shadowThickness:		2",
    "*msgFrame.marginWidth:		3",
    "*msgFrame.marginHeight:		3",
    "*msgList.visibleItemCount:		12",
    "*workArea.width:			600",
    NULL
};

#endif

#endif // _MWDefaultResources_h

