/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#ifndef _VCDefaultResources_h
#define _VCDefaultResources_h

#if !defined(DX_NEW_KEYLAYOUT)

String  ViewControlDialog::DefaultResources[] =
{
    ".XmForm.accelerators:		#augment\n"
#if 1
	"<Key>Return:			BulletinBoardReturn()",
#else
	"<Key>Return:			BulletinBoardReturn()\n"
	"<Btn2Down>,<Btn2Up>:		HELP!",
#endif
    "*mainForm.width:				292",
    "*mainForm.undoButton.labelString:		Undo  Ctrl+U",
    "*mainForm.undoButton.width:			100",
    "*mainForm.redoButton.labelString:		Redo  Ctrl+D",
    "*mainForm.redoButton.width:			100",
    "*mainForm.modeLabel.labelString:		Mode:",
    "*mainForm.setViewLabel.labelString:	Set View:",
    "*mainForm.projectionLabel.labelString:	Projection:",
    "*mainForm.viewAngleLabel.labelString:	View Angle:",

    "*mainForm*modePulldownMenu.modeNone.labelString:         None",
#ifdef sgi 
    "*mainForm*modePulldownMenu.modeNone.width:               85",
    "*mainForm*modePulldownMenu.modeNone.recomputeSize:       False",
#endif
    "*mainForm*modePulldownMenu.modeCamera.labelString:       Camera",
    "*mainForm*modePulldownMenu.modeCamera.accelerator:       Ctrl<Key>K",
    "*mainForm*modePulldownMenu.modeCamera.acceleratorText:   Ctrl+K",
    "*mainForm*modePulldownMenu.modeCursors.labelString:      Cursors",
    "*mainForm*modePulldownMenu.modeCursors.accelerator:      Ctrl<Key>X",
    "*mainForm*modePulldownMenu.modeCursors.acceleratorText:  Ctrl+X",
    "*mainForm*modePulldownMenu.modePick.labelString:         Pick",
    "*mainForm*modePulldownMenu.modePick.accelerator:         Ctrl<Key>I",
    "*mainForm*modePulldownMenu.modePick.acceleratorText:     Ctrl+I",
    "*mainForm*modePulldownMenu.modeNavigate.labelString:     Navigate",
    "*mainForm*modePulldownMenu.modeNavigate.accelerator:     Ctrl<Key>N",
    "*mainForm*modePulldownMenu.modeNavigate.acceleratorText: Ctrl+N",
    "*mainForm*modePulldownMenu.modePanZoom.labelString:      Pan/Zoom",
    "*mainForm*modePulldownMenu.modePanZoom.accelerator:      Ctrl<Key>G",
    "*mainForm*modePulldownMenu.modePanZoom.acceleratorText:  Ctrl+G",
    "*mainForm*modePulldownMenu.modeRoam.labelString:         Roam",
    "*mainForm*modePulldownMenu.modeRoam.accelerator:         Ctrl<Key>W",
    "*mainForm*modePulldownMenu.modeRoam.acceleratorText:     Ctrl+W",
    "*mainForm*modePulldownMenu.modeRotate.labelString:       Rotate",
    "*mainForm*modePulldownMenu.modeRotate.accelerator:       Ctrl<Key>R",
    "*mainForm*modePulldownMenu.modeRotate.acceleratorText:   Ctrl+R",
    "*mainForm*modePulldownMenu.modeZoom.labelString:         Zoom",
    "*mainForm*modePulldownMenu.modeZoom.accelerator:         Ctrl<Key>Z",
    "*mainForm*modePulldownMenu.modeZoom.acceleratorText:     Ctrl+Z",

    "*mainForm*setViewPulldownMenu.setViewNone.labelString:       None",
    "*mainForm*setViewPulldownMenu.setViewNone.width:             140",
    "*mainForm*setViewPulldownMenu.setViewNone.recomputeSize:     False",
    "*mainForm*setViewPulldownMenu.setViewTop.labelString:        Top",
    "*mainForm*setViewPulldownMenu.setViewBottom.labelString:     Bottom",
    "*mainForm*setViewPulldownMenu.setViewFront.labelString:      Front",
    "*mainForm*setViewPulldownMenu.setViewBack.labelString:       Back",
    "*mainForm*setViewPulldownMenu.setViewLeft.labelString:       Left",
    "*mainForm*setViewPulldownMenu.setViewRight.labelString:      Right",
    "*mainForm*setViewPulldownMenu.setViewDiagonal.labelString:   Diagonal",
    "*mainForm*setViewPulldownMenu.setViewOffTop.labelString:     Off Top",
    "*mainForm*setViewPulldownMenu.setViewOffBottom.labelString:  Off Bottom",
    "*mainForm*setViewPulldownMenu.setViewOffFront.labelString:   Off Front",
    "*mainForm*setViewPulldownMenu.setViewOffBack.labelString:    Off Back",
    "*mainForm*setViewPulldownMenu.setViewOffLeft.labelString:    Off Left",
    "*mainForm*setViewPulldownMenu.setViewOffRight.labelString:   Off Right",
    "*mainForm*setViewPulldownMenu.setViewOffDiagonal.labelString:Off Diagonal",
    "*mainForm*projectionPulldownMenu.orthographic.labelString:   Orthographic",
    "*mainForm*projectionPulldownMenu.orthographic.width:         140",
    "*mainForm*projectionPulldownMenu.orthographic.recomputeSize: False",
    "*mainForm*projectionPulldownMenu.perspective.labelString:    Perspective",
    "*mainForm*projectionPulldownMenu.perspective.width:         140",
    "*mainForm*projectionPulldownMenu.perspective.recomputeSize: False",

    "*cursorForm.probeLabel.labelString:                    Probe(s):",
    "*cursorForm.probeLabel.width:                          95",
    "*cursorForm*probePulldownMenu.noProbes.labelString:    (None)",
    "*cursorForm*probePulldownMenu*width:                   110",
    "*cursorForm*probePulldownMenu*recomputeSize:           False",
    "*cursorForm.constraintLabel.labelString:               Constraints:",
    "*cursorForm.constraintLabel.width:                     95",
    "*cursorForm*constraintPulldownMenu.None.labelString:   None",
    "*cursorForm*constraintPulldownMenu.None.width:         110",
    "*cursorForm*constraintPulldownMenu.None.recomputeSize: False",
    "*cursorForm*constraintPulldownMenu.X.labelString:      X",
    "*cursorForm*constraintPulldownMenu.Y.labelString:      Y",
    "*cursorForm*constraintPulldownMenu.Z.labelString:      Z",

    "*pickForm.pickLabel.labelString:                       Pick(s):",
    "*pickForm.pickLabel.width:                             95",
    "*pickForm*pickPulldownMenu.noPicks.labelString:        (None)",
    "*pickForm*pickPulldownMenu*width:                      110",
    "*pickForm*pickPulldownMenu*recomputeSize:              False",

    "*cameraForm*cameraWhichPulldownMenu.to.labelString:    To",
    "*cameraForm*cameraWhichPulldownMenu.from.labelString:  From",
    "*cameraForm*cameraWhichPulldownMenu.up.labelString:    Up",
    "*cameraForm.cameraXLabel.labelString:    X",
    "*cameraForm.cameraYLabel.labelString:    Y",
    "*cameraForm.cameraZLabel.labelString:    Z",
    "*cameraForm.cameraWindowWidthLabel.labelString:    Window Width:",
    "*cameraForm.cameraWindowHeightLabel.labelString:   Window Height:",
    "*cameraForm.cameraWidthLabel.labelString:    Camera Width:",

    "*navigateForm*navigateLookPulldownMenu.forward.labelString: Forward",
    "*navigateForm*navigateLookPulldownMenu.forward.width: 110",
    "*navigateForm*navigateLookPulldownMenu.forward.recomputeSize: False",
    "*navigateForm*navigateLookPulldownMenu.left45.labelString:  45\\260\\ Left",
    "*navigateForm*navigateLookPulldownMenu.right45.labelString: 45\\260\\ Right",
    "*navigateForm*navigateLookPulldownMenu.up45.labelString:    45\\260\\ Up",
    "*navigateForm*navigateLookPulldownMenu.down45.labelString:  45\\260\\ Down",
    "*navigateForm*navigateLookPulldownMenu.left90.labelString:  90\\260\\ Left",
    "*navigateForm*navigateLookPulldownMenu.right90.labelString: 90\\260\\ Right",
    "*navigateForm*navigateLookPulldownMenu.up90.labelString:    90\\260\\ Up",
    "*navigateForm*navigateLookPulldownMenu.down90.labelString:  90\\260\\ Down",
    "*navigateForm*navigateLookPulldownMenu.backward.labelString:Backward",
    "*navigateForm*navigateLookPulldownMenu.align.labelString:   Align",
    "*navigateForm.navigationLabel.labelString:   Navigation",
    "*navigateForm.lookLabel.labelString:         Look:",
    "*navigateForm.motionLabel.labelString:       Motion:",
    "*navigateForm.pivotLabel.labelString:        Pivot:",

    "*buttonForm.closeButton.labelString:	Close",
    "*buttonForm.closeButton.width:		100",
    "*buttonForm.resetButton.labelString:	Reset  Ctrl+F",
    "*buttonForm.resetButton.width:		100",

#if defined(aviion)
    "*modeOptionMenu.labelString:",
    "*setViewOptionMenu.labelString:",
    "*projectionMenu.labelString:",
    "*probeMenu.labelString:",
    "*pickMenu.labelString:",
    "*constraintMenu.labelString:",
    "*cameraWhichMenu.labelString:",
    "*navigateLookMenu.labelString:",
#endif
    NULL
};


#else /* defined(DX_NEW_KEYLAYOUT) */

String  ViewControlDialog::DefaultResources[] =
{
    ".XmForm.accelerators:					#augment\n"
#if 1
	"<Key>Return:						BulletinBoardReturn()",
#else
	"<Key>Return:						BulletinBoardReturn()\n"
	"<Btn2Down>,<Btn2Up>:					HELP!",
#endif
    "*mainForm.width:						292",
    "*mainForm.undoButton.labelString:				Undo  Ctrl+Z",
    "*mainForm.undoButton.width:				120",
    "*mainForm.redoButton.labelString:				Redo  Ctrl+Y",
    "*mainForm.redoButton.width:				120",
    "*mainForm.modeLabel.labelString:				Mode:",
    "*mainForm.setViewLabel.labelString:			Set View:",
    "*mainForm.projectionLabel.labelString:			Projection:",
    "*mainForm.viewAngleLabel.labelString:			View Angle:",

    "*mainForm*modePulldownMenu.modeNone.labelString:         	None",
#ifdef sgi 
    "*mainForm*modePulldownMenu.modeNone.width:               	85",
    "*mainForm*modePulldownMenu.modeNone.recomputeSize:       	False",
#endif
    "*mainForm*modePulldownMenu.modeCamera.labelString:       	Camera",
    "*mainForm*modePulldownMenu.modeCamera.accelerator:       	Ctrl<Key>K",
    "*mainForm*modePulldownMenu.modeCamera.acceleratorText:   	Ctrl+K",
    "*mainForm*modePulldownMenu.modeCursors.labelString:      	Cursors",
    "*mainForm*modePulldownMenu.modeCursors.accelerator:      	Ctrl<Key>X",
    "*mainForm*modePulldownMenu.modeCursors.acceleratorText:  	Ctrl+X",
    "*mainForm*modePulldownMenu.modePick.labelString:         	Pick",
    "*mainForm*modePulldownMenu.modePick.accelerator:         	Ctrl<Key>I",
    "*mainForm*modePulldownMenu.modePick.acceleratorText:     	Ctrl+I",
    "*mainForm*modePulldownMenu.modeNavigate.labelString:     	Navigate",
    "*mainForm*modePulldownMenu.modeNavigate.accelerator:     	Ctrl<Key>N",
    "*mainForm*modePulldownMenu.modeNavigate.acceleratorText: 	Ctrl+N",
    "*mainForm*modePulldownMenu.modePanZoom.labelString:      	Pan/Zoom",
    "*mainForm*modePulldownMenu.modePanZoom.accelerator:      	Ctrl<Key>space",
    "*mainForm*modePulldownMenu.modePanZoom.acceleratorText:  	Ctrl+Spc",
    "*mainForm*modePulldownMenu.modeRoam.labelString:         	Roam",
    "*mainForm*modePulldownMenu.modeRoam.accelerator:         	Ctrl<Key>Tab",
    "*mainForm*modePulldownMenu.modeRoam.acceleratorText:     	Ctrl+Tab",
    "*mainForm*modePulldownMenu.modeRotate.labelString:       	Rotate",
    "*mainForm*modePulldownMenu.modeRotate.accelerator:       	Ctrl<Key>R",
    "*mainForm*modePulldownMenu.modeRotate.acceleratorText:   	Ctrl+R",
    "*mainForm*modePulldownMenu.modeZoom.labelString:         	Zoom",
    "*mainForm*modePulldownMenu.modeZoom.accelerator:         	Ctrl Shift <Key>space",
    "*mainForm*modePulldownMenu.modeZoom.acceleratorText:     	Ctrl+Shift+Spc",

    "*mainForm*setViewPulldownMenu.setViewNone.labelString:       None",
    "*mainForm*setViewPulldownMenu.setViewNone.width:             140",
    "*mainForm*setViewPulldownMenu.setViewNone.recomputeSize:     False",
    "*mainForm*setViewPulldownMenu.setViewTop.labelString:        Top",
    "*mainForm*setViewPulldownMenu.setViewBottom.labelString:     Bottom",
    "*mainForm*setViewPulldownMenu.setViewFront.labelString:      Front",
    "*mainForm*setViewPulldownMenu.setViewBack.labelString:       Back",
    "*mainForm*setViewPulldownMenu.setViewLeft.labelString:       Left",
    "*mainForm*setViewPulldownMenu.setViewRight.labelString:      Right",
    "*mainForm*setViewPulldownMenu.setViewDiagonal.labelString:   Diagonal",
    "*mainForm*setViewPulldownMenu.setViewOffTop.labelString:     Off Top",
    "*mainForm*setViewPulldownMenu.setViewOffBottom.labelString:  Off Bottom",
    "*mainForm*setViewPulldownMenu.setViewOffFront.labelString:   Off Front",
    "*mainForm*setViewPulldownMenu.setViewOffBack.labelString:    Off Back",
    "*mainForm*setViewPulldownMenu.setViewOffLeft.labelString:    Off Left",
    "*mainForm*setViewPulldownMenu.setViewOffRight.labelString:   Off Right",
    "*mainForm*setViewPulldownMenu.setViewOffDiagonal.labelString:Off Diagonal",
    "*mainForm*projectionPulldownMenu.orthographic.labelString:   Orthographic",
    "*mainForm*projectionPulldownMenu.orthographic.width:         140",
    "*mainForm*projectionPulldownMenu.orthographic.recomputeSize: False",
    "*mainForm*projectionPulldownMenu.perspective.labelString:    Perspective",
    "*mainForm*projectionPulldownMenu.perspective.width:         140",
    "*mainForm*projectionPulldownMenu.perspective.recomputeSize: False",

    "*cursorForm.probeLabel.labelString:                    	Probe(s):",
    "*cursorForm.probeLabel.width:                          	95",
    "*cursorForm*probePulldownMenu.noProbes.labelString:    	(None)",
    "*cursorForm*probePulldownMenu*width:                   	110",
    "*cursorForm*probePulldownMenu*recomputeSize:           	False",
    "*cursorForm.constraintLabel.labelString:               	Constraints:",
    "*cursorForm.constraintLabel.width:                     	95",
    "*cursorForm*constraintPulldownMenu.None.labelString:   	None",
    "*cursorForm*constraintPulldownMenu.None.width:         	110",
    "*cursorForm*constraintPulldownMenu.None.recomputeSize: 	False",
    "*cursorForm*constraintPulldownMenu.X.labelString:      	X",
    "*cursorForm*constraintPulldownMenu.Y.labelString:      	Y",
    "*cursorForm*constraintPulldownMenu.Z.labelString:      	Z",

    "*pickForm.pickLabel.labelString:                       	Pick(s):",
    "*pickForm.pickLabel.width:                             	95",
    "*pickForm*pickPulldownMenu.noPicks.labelString:        	(None)",
    "*pickForm*pickPulldownMenu*width:                      	110",
    "*pickForm*pickPulldownMenu*recomputeSize:              	False",

    "*cameraForm*cameraWhichPulldownMenu.to.labelString:    	To",
    "*cameraForm*cameraWhichPulldownMenu.from.labelString:  	From",
    "*cameraForm*cameraWhichPulldownMenu.up.labelString:    	Up",
    "*cameraForm.cameraXLabel.labelString:    			X",
    "*cameraForm.cameraYLabel.labelString:    			Y",
    "*cameraForm.cameraZLabel.labelString:    			Z",
    "*cameraForm.cameraWindowWidthLabel.labelString:    	Window Width:",
    "*cameraForm.cameraWindowHeightLabel.labelString:   	Window Height:",
    "*cameraForm.cameraWidthLabel.labelString:    		Camera Width:",

    "*navigateForm*navigateLookPulldownMenu.forward.labelString: Forward",
    "*navigateForm*navigateLookPulldownMenu.forward.width: 	110",
    "*navigateForm*navigateLookPulldownMenu.forward.recomputeSize: False",
    "*navigateForm*navigateLookPulldownMenu.left45.labelString:  45\\260\\ Left",
    "*navigateForm*navigateLookPulldownMenu.right45.labelString: 45\\260\\ Right",
    "*navigateForm*navigateLookPulldownMenu.up45.labelString:    45\\260\\ Up",
    "*navigateForm*navigateLookPulldownMenu.down45.labelString:  45\\260\\ Down",
    "*navigateForm*navigateLookPulldownMenu.left90.labelString:  90\\260\\ Left",
    "*navigateForm*navigateLookPulldownMenu.right90.labelString: 90\\260\\ Right",
    "*navigateForm*navigateLookPulldownMenu.up90.labelString:    90\\260\\ Up",
    "*navigateForm*navigateLookPulldownMenu.down90.labelString:  90\\260\\ Down",
    "*navigateForm*navigateLookPulldownMenu.backward.labelString:Backward",
    "*navigateForm*navigateLookPulldownMenu.align.labelString:   Align",
    "*navigateForm.navigationLabel.labelString:   		Navigation",
    "*navigateForm.lookLabel.labelString:         		Look:",
    "*navigateForm.motionLabel.labelString:       		Motion:",
    "*navigateForm.pivotLabel.labelString:        		Pivot:",

    "*buttonForm.closeButton.labelString:			Close",
    "*buttonForm.closeButton.width:				100",
    "*buttonForm.resetButton.labelString:			Reset  Ctrl+F",
    "*buttonForm.resetButton.width:				100",

#if defined(aviion)
    "*modeOptionMenu.labelString:",
    "*setViewOptionMenu.labelString:",
    "*projectionMenu.labelString:",
    "*probeMenu.labelString:",
    "*pickMenu.labelString:",
    "*constraintMenu.labelString:",
    "*cameraWhichMenu.labelString:",
    "*navigateLookMenu.labelString:",
#endif
    NULL
};


#endif

#endif // _VCDefaultResources_h

