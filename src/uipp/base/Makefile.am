## Process this file with automake to produce Makefile.in

INCLUDES = -I${srcdir}/../widgets -I${top_srcdir}/include -D@ARCH@

noinst_LIBRARIES = libBASE.a

EXTRA_DIST = \
	minus.bm \
	plus.bm 

SRCS1 =  \
	Application.C			\
	ApplyFileDialog.C		\
	ButtonInterface.C		\
	CascadeMenu.C			\
	CloseWindowCommand.C		\
	CmdEntry.C			\
	Command.C			\
	CommandInterface.C		\
	ConfirmedCommand.C		\
	DeferrableAction.C		\
	Dialog.C			\
	DialogManager.C			\
	Dictionary.C			\
	DragSource.C			\
	DropSite.C			\
	DXTensor.C			\
	DXType.C			\
	DXValue.C			\
	DynamicResource.C		\
	ErrorDialogManager.C		\
	FileDialog.C			\
	HelpMenuCommand.C		\
	HelpOnContextCommand.C		\
	HelpWin.C			\
	IBMApplication.C		\
	IBMMainWindow.C			\
	InfoDialogManager.C		

SRCS2 = \
	License.C			\
	Link.C				\
	List.C				\
	ListEditor.C			\
	ListIterator.C			\
	MainWindow.C			\
	NoOpCommand.C			\
	Notebook.C			\
	NotebookTab.C			\
	NoUndoCommand.C			\
	NoUndoHelpCmd.C			\
	OptionalPreActionCommand.C	\
	PreActionCommand.C		\
	QuestionDialogManager.C		\
	QuitCommand.C			\
	RepeatingToggle.C		\
	SaveFileDialog.C		\
	Server.C			\
	SetNameDialog.C			\
	Stack.C				\
	StartWebBrowser.C		\
	Strings.C			\
	StringTable.C			\
	SymbolManager.C			\
	TemporaryLicense.C		\
	TextEditDialog.C		\
	TextFile.C			\
	TextFileFileDialog.C		\
	TextPopup.C			\
	TextSelector.C			\
	TimedDialog.C			\
	TimedMessage.C			\
	ToggleButtonInterface.C		\
	TransferStyle.C			\
	TreeView.C 			\
	UIComponent.C			\
	UIComponentHelpCommand.C	\
	UndoCommand.C			\
	WarningDialogManager.C		\
	WizardDialog.C			\
	WorkSpace.C			\
	WorkSpaceGrid.C			\
	WorkSpaceInfo.C			\
	WorkSpaceRoot.C			\
	XHandler.C			\
	XmUtility.C			\
	lex.C				\
	util.C				\
	helpcallbacks.c			\
	help.c				\
	helplist.c			\
	helpstack.c			\
	history.c			\
	parse.c

libBASE_a_SOURCES =  $(SRCS1) $(SRCS2)

HDRS1 = \
	ActiveItemDictionary.h \
	AllocatorDictionary.h \
	Application.h \
	ApplyFileDialog.h \
	Base.h \
	ButtonInterface.h \
	CascadeMenu.h \
	Client.h \
	CloseWindowCommand.h \
	CmdEntry.h \
	Command.h \
	CommandInterface.h \
	CommandScope.h \
	ConfirmedCommand.h \
	DXTensor.h \
	DXType.h \
	DXValue.h \
	DefaultingDictionary.h \
	DeferrableAction.h \
	Definition.h \
	Dialog.h \
	DialogData.h \
	DialogManager.h \
	Dictionary.h \
	DictionaryIterator.h \
	DragSource.h \
	DropSite.h \
	DynamicResource.h \
	ErrorDialogManager.h \
	FileDialog.h \
	HelpMenuCommand.h \
	HelpOnContextCommand.h \
	HelpWin.h \
	IBMApplication.h \
	IBMMainWindow.h \
	IBMVersion.h \
	InfoDialogManager.h \
	License.h \
	Link.h \
	List.h \
	ListEditor.h 

HDRS2 = \
	ListIterator.h \
	MainWindow.h \
	NoOpCommand.h \
	Notebook.h \
	NotebookTab.h \
	NoUndoCommand.h \
	NoUndoHelpCmd.h \
	OptionalPreActionCommand.h \
	PreActionCommand.h \
	QuestionDialogManager.h \
	QuitCommand.h \
	RepeatingToggle.h \
	SaveFileDialog.h \
	Server.h \
	SetNameDialog.h \
	Stack.h \
	StartWebBrowser.h \
	StringTable.h \
	DXStrings.h \
	SymbolManager.h \
	TemporaryLicense.h \
	TextEditDialog.h \
	TextFile.h \
	TextFileFileDialog.h \
	TextPopup.h \
	TextSelector.h \
	TimedDialog.h \
	TimedMessage.h \
	ToggleButtonInterface.h \
	TransferStyle.h \
	TreeNode.h \
	TreeView.h \
	UIComponent.h \
	UIComponentHelpCommand.h \
	UIConfig.h \
	UndoCommand.h \
	WarningDialogManager.h \
	WizardDialog.h \
	WorkSpace.h \
	WorkSpaceGrid.h \
	WorkSpaceInfo.h \
	WorkSpacePage.h \
	WorkSpaceRoot.h \
	XHandler.h \
	XmUtility.h \
	defines.h \
	help.h \
	helplist.h \
	helpstack.h \
	history.h \
	icon50.h \
	lex.h \
	logo.h 

noinst_HEADERS = $(HDRS1) $(HDRS2)
