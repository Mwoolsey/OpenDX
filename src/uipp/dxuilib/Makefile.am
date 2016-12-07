## Process this file with automake to produce Makefile.in

INCLUDES =  -I${srcdir}/../base -I${top_srcdir}/include -I${srcdir}/../dxui -D@ARCH@

noinst_LIBRARIES = libDXUI.a

DISTCLEANFILES = netlex.c netyacc.c lex.yy.c 
CLEANFILES = dxmac.h gifmac.h imagemac.h vrmlmac.h

BUILT_SOURCES = \
	sequence.h \
	gifmac.h \
	vrmlmac.h \
	dxmac.h \
	imagemac.h \
	netyacc.c

EXTRA_DIST = \
	sequence.net \
	imagemac.net \
	net.lex \
	net.y \
	netyacc.awk \
	anchor.bm \
	ntractor.bm \
	saw_sor.bm \
	ticks_out.bm \
	anim_mask.bm \
	ntractormask.bm \
	square_sol.bm \
	ticks_out_ins.bm \
	animation.bm \
	pagedrag.bm \
	square_sor.bm \
	tooldrag.bm \
	pagedragmask.bm \
	step.bm \
	tooldragmask.bm \
	moduledrag.bm \
	postit.bm \
	ticks_in.bm \
	moduledragmask.bm \
	saw_sol.bm \
	ticks_in_ins.bm \
	local.mk \
	net2c \
	seq2c


HDRS1 = \
	oem.h \
	AnnotationGroupManager.h \
	ApplicIF.h \
	Ark.h \
	ArkStandIn.h \
	AttributeParameter.h \
	AutoAxesDialog.h \
	BinaryParameter.h \
	CDBAllocatorDictionary.h \
	CDBInput.h \
	CDBOutput.h \
	CDBParameter.h \
	Cacheability.h \
	ColormapAddCtlDialog.h \
	ColormapEditor.h \
	ColormapNBinsDialog.h \
	ColormapWaveDialog.h \
	CommentStyle.h \
	CommentStyleUser.h \
	ComponentAttributes.h \
	ComputeCDB.h \
	ConfigurationDialog.h \
	ControlPanel.h \
	ControlPanelAccessDialog.h \
	ControlPanelGroupDialog.h \
	ControlPanelWorkSpace.h \
	CreateMacroDialog.h \
	DXAnchorWindow.h \
	DXApplication.h \
	DXChild.h \
	DXDragSource.h \
	DXDropSite.h \
	DXExecCtl.h \
	DXLinkHandler.h \
	DXPacketIF.h \
	DXVersion.h \
	DXWindow.h \
	Decorator.h \
	DecoratorInfo.h \
	DecoratorStyle.h \
	DescrDialog.h \
	EditorToolSelector.h \
	EditorWindow.h \
	EditorWorkSpace.h \
	FileSelectorDialog.h \
	FileSelectorInstance.h 

HDRS2 = \
	FindStack.h \
	FindToolDialog.h \
	GetSetConversionDialog.h \
	GraphLayout.h \
	GridDialog.h \
	GroupManager.h \
	GroupStyle.h \
	GroupedObject.h \
	HelpOnNetworkDialog.h \
	HelpOnPanelDialog.h \
	ImageCDB.h \
	ImageFileDialog.h \
	ImageFormat.h \
	ImageFormatDialog.h \
	ImageFormatGIF.h \
	ImageFormatIM.h \
	ImageFormatMIF.h \
	ImageFormatPSColor.h \
	ImageFormatPSColorEnc.h \
	ImageFormatPSGrey.h \
	ImageFormatPSGreyEnc.h \
	ImageFormatREX.h \
	ImageFormatRGB.h \
	ImageFormatTIF.h \
	ImageFormatYUV.h \
	ImageWindow.h \
	InsertNetworkDialog.h \
	InteractorInstance.h \
	InteractorStandIn.h \
	InteractorStyle.h \
	ItalicLabeledStandIn.h \
	JavaNet.h \
	LabelDecorator.h \
	LabeledStandIn.h \
	LinkHandler.h \
	LoadMDFDialog.h \
	LoadMacroDialog.h \
	LocalAttributes.h \
	MWClearCmd.h \
	MWFileDialog.h \
	MoveNodesDialog.h \
	MsgWin.h 

HDRS3 = \
	NDAllocatorDictionary.h \
	Network.h \
	NoUndoMWCmd.h \
	OpenCFGDialog.h \
	OpenColormapDialog.h \
	OpenNetCommentDialog.h \
	OpenNetworkDialog.h \
	PacketHandler.h \
	PacketIF.h \
	PageGroupManager.h \
	PageSelector.h \
	PageTab.h \
	PanelAccessManager.h \
	PanelGroupManager.h \
	Parameter.h \
	ParameterCDB.h \
	Parse.h \
	ParseMDF.h \
	PixelImageFormat.h \
	PostScriptImageFormat.h \
	PrintImageDialog.h \
	PrintProgramDialog.h \
	PrintProgramFileDialog.h \
	ProcessGroupAssignDialog.h \
	ProcessGroupCreateDialog.h \
	ProcessGroupManager.h \
	ProcessGroupOptionsDialog.h \
	QueuedPackets.h \
	RenderingOptionsDialog.h \
	ResourceManager.h \
	SIAllocatorDictionary.h \
	SaveAsCCodeDialog.h \
	SaveAsDialog.h \
	SaveCFGDialog.h \
	SaveImageDialog.h \
	ScalarInstance.h \
	ScalarListInstance.h
 
HDRS4 = \
	SelectionAttrDialog.h \
	SelectionInstance.h \
	SelectorInstance.h \
	SelectorListInstance.h \
	SeparatorDecorator.h \
	SequencerWindow.h \
	SetAnnotatorTextDialog.h \
	SetAttrDialog.h \
	SetBGColorDialog.h \
	SetColormapNameDialog.h \
	SetDecoratorTextDialog.h \
	SetImageNameDialog.h \
	SetInteractorNameDialog.h \
	SetMacroNameDialog.h \
	SetNetworkCommentDialog.h \
	SetPageNameDialog.h \
	SetPanelCommentDialog.h \
	SetPanelNameDialog.h \
	SetScalarAttrDialog.h \
	SetSeparatorAttrDlg.h \
	SetVectorAttrDialog.h \
	StandIn.h \
	StartOptionsDialog.h \
	StartServerDialog.h \
	Tab.h \
	ThrottleDialog.h \
	TickLabel.h \
	TickLabelList.h \
	ToggleAttrDialog.h \
	ToggleInstance.h \
	ToolSelector.h \
	UndoableAction.h \
	UndoAddArk.h \
	UndoDeletion.h \
	UndoGrid.h \
	UndoMove.h \
	UndoNode.h \
	UndoRepeatableTab.h \
	VPEAnnotator.h \
	VPEPage.h \
	VPEPostIt.h \
	VPERoot.h \
	ValueInstance.h \
	ValueListInstance.h \
	ViewControlDialog.h \
	WorkSpaceComponent.h \
	enums.h \
	TransferAccelerator.h \
	make.image.sh \
	make.sequence.sh

HDRS5 = \
	CMDefaultResources.h \
	DXWDefaultResources.h \
	IWDefaultResources.h \
	VCDefaultResources.h \
	CPDefaultResources.h \
	EWDefaultResources.h \
	MWDefaultResources.h

noinst_HEADERS = $(HDRS1) $(HDRS2) $(HDRS3) $(HDRS4) $(HDRS5)

SRCS1 = \
	oem.C \
	AnnotationGroupManager.C \
	ApplicIF.C \
	Ark.C \
	ArkStandIn.C \
	AttributeParameter.C \
	AutoAxesDialog.C \
	BuildIFDict.C \
	CDBAllocatorDictionary.C \
	ColormapAddCtlDialog.C \
	ColormapEditor.C \
	ColormapNBinsDialog.C \
	ColormapWaveDialog.C \
	CommentStyle.C \
	CommentStyleUser.C \
	ComputeCDB.C \
	ConfigurationDialog.C \
	ControlPanel.C \
	ControlPanelAccessDialog.C \
	ControlPanelGroupDialog.C \
	ControlPanelWorkSpace.C \
	CreateMacroDialog.C \
	DXAnchorWindow.C \
	DXApplication.C \
	DXChild.C \
	DXDragSource.C \
	DXDropSite.C \
	DXExecCtl.C \
	DXLinkHandler.C \
	DXPacketIF.C \
	DXWindow.C \
	Decorator.C \
	DecoratorStyle.C \
	DescrDialog.C \
	EditorToolSelector.C \
	EditorWindow.C \
	EditorWorkSpace.C \
	FileSelectorDialog.C \
	FileSelectorInstance.C \
	FindStack.C \
	FindToolDialog.C \
	GetSetConversionDialog.C 

SRCS2 = \
	GridDialog.C \
	GraphLayout.C \
	GroupManager.C \
	GroupStyle.C \
	GroupedObject.C \
	HelpOnNetworkDialog.C \
	HelpOnPanelDialog.C \
	ImageCDB.C \
	ImageFileDialog.C \
	ImageFormat.C \
	ImageFormatDialog.C \
	ImageFormatGIF.C \
	ImageFormatIM.C \
	ImageFormatMIF.C \
	ImageFormatPSColor.C \
	ImageFormatPSColorEnc.C \
	ImageFormatPSGrey.C \
	ImageFormatPSGreyEnc.C \
	ImageFormatREX.C \
	ImageFormatRGB.C \
	ImageFormatTIF.C \
	ImageFormatYUV.C \
	ImageNodeUtils.C \
	ImageWindow.C \
	InsertNetworkDialog.C 

SRCS3 = \
	InteractorInstance.C \
	InteractorStandIn.C \
	InteractorStyle.C \
	ItalicLabeledStandIn.C \
	JavaNet.C \
	LabelDecorator.C \
	LabeledStandIn.C \
	LinkHandler.C \
	LoadMDFDialog.C \
	LoadMacroDialog.C \
	MWClearCmd.C \
	MWFileDialog.C \
	MoveNodesDialog.C \
	MsgWin.C \
	NDAllocatorDictionary.C \
	Network.C \
	NoUndoMWCmd.C \
	OpenCFGDialog.C \
	OpenColormapDialog.C \
	OpenNetCommentDialog.C \
	OpenNetworkDialog.C \
	PacketHandler.C \
	PacketIF.C \
	PageGroupManager.C 

SRCS4 = \
	PageSelector.C \
	PageTab.C \
	PanelAccessManager.C \
	PanelGroupManager.C \
	Parameter.C \
	ParameterCDB.C \
	ParseMDF.C \
	PixelImageFormat.C \
	PostScriptImageFormat.C \
	PrintImageDialog.C \
	PrintProgramDialog.C \
	PrintProgramFileDialog.C \
	ProcessGroupAssignDialog.C \
	ProcessGroupCreateDialog.C \
	ProcessGroupManager.C \
	ProcessGroupOptionsDialog.C \
	QueuedPackets.C \
	ResourceManager.C \
	RenderingOptionsDialog.C \
	SIAllocatorDictionary.C \
	SaveAsCCodeDialog.C \
	SaveAsDialog.C \
	SaveCFGDialog.C \
	SaveImageDialog.C \
	ScalarInstance.C \
	ScalarListInstance.C \
	SelectionAttrDialog.C \
	SelectionInstance.C 

SRCS5 = \
	SelectorInstance.C \
	SelectorListInstance.C \
	SeparatorDecorator.C \
	SequencerWindow.C \
	SetAnnotatorTextDialog.C \
	SetAttrDialog.C \
	SetBGColorDialog.C \
	SetColormapNameDialog.C \
	SetDecoratorTextDialog.C \
	SetImageNameDialog.C \
	SetInteractorNameDialog.C \
	SetMacroNameDialog.C \
	SetNetworkCommentDialog.C \
	SetPageNameDialog.C \
	SetPanelCommentDialog.C \
	SetPanelNameDialog.C \
	SetScalarAttrDialog.C \
	SetSeparatorAttrDlg.C \
	SetVectorAttrDialog.C \
	StandIn.C \
	StartOptionsDialog.C \
	StartServerDialog.C \
	Tab.C \
	ThrottleDialog.C \
	TickLabel.C \
	TickLabelList.C \
	ToggleAttrDialog.C \
	ToggleInstance.C \
	ToolSelector.C \
	TransferAccelerator.c \
	UndoAddArk.C \
	UndoDeletion.C \
	UndoGrid.C \
	UndoMove.C \
	UndoNode.C \
	UndoRepeatableTab.C \
	VPEAnnotator.C \
	VPEPage.C \
	VPEPostIt.C \
	VPERoot.C \
	ValueInstance.C \
	ValueListInstance.C \
	ViewControlDialog.C \
	WorkSpaceComponent.C \
	delete.C \
	new.C \
	\
	sequence.h \
	gifmac.h  \
	vrmlmac.h \
	dxmac.h \
	imagemac.h \
	\
	netyacc.c

libDXUI_a_SOURCES = $(SRCS1) $(SRCS2) $(SRCS3) $(SRCS4) $(SRCS5)

@LOCALMAKE@
