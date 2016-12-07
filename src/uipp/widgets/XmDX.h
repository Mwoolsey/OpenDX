/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>



#ifndef _XmDX_H
#define _XmDX_H

#include <Xm/Xm.h>

#define MAX_POINTS 100

/* Define resource strings for the ColorMapEditor Widget */


#define XmNdrawMode           "drawMode"
#define XmCDrawMode           "DrawMode"
#define XmNprintCP            "printCP"
#define XmCPrintCP            "PrintCP"
#define XmNbackgroundStyle "backgroundStyle"
#define XmCBackgroundStyle "BackgroundStyle"
#define XmNdisplayOpacity "displayOpacity"
#define XmCDisplayOpacity "DisplayOpacity"
#define XmNvalueMinimum	 "valueMinimum"
#define XmCValueMinimum	 "ValueMinimum"
#define XmNvalueMaximum	 "valueMaximum"
#define XmCValueMaximum	 "ValueMaximum"
#define XmNminEditable "minEditable"
#define XmNmaxEditable "maxEditable"
#ifndef XmCEditable
    #define XmCEditable "Editable"
#endif
#define XmNdefaultColormap "defaultColormap"
#define XmCDefaultColormap "DefaultColormap"
#define XmNsaveFilename "saveFilename"
#define XmCSaveFilename "SaveFilename"
#define XmNopenFilename "openFilename"
#define XmCOpenFilename "OpenFilename"
#define XmCActivateCallback "ActivateCallback"
#define XmNconstrainVertical "constrainVertical"
#define XmCConstrainVertical "ConstrainVertical"
#define XmNconstrainHorizontal "constrainHorizontal"
#define XmCConstrainHorizontal "ConstrainHorizontal"
#define XmNtriggerCallback "triggerCallback"
#define XmCTriggerCallback "TriggerCallback"

#define XmCR_MODIFIED	1
#define STRIPES 0
#define CHECKERBOARD 1

/*
 * Define resource strings for the Dial widget.
 */
#define XmNincrementsPerMarker	"incrementsPerMarker"

#define XmNnumMarkers		"numMarkers"
#define XmNmajorMarkerWidth	"majorMarkerWidth"
#define XmNminorMarkerWidth     "minorMarkerWidth"
#define XmNmajorPosition	"majorPosition"
#define XmNstartingMarkerPos	"startingMarkerPos"
#define XmNmajorMarkerColor    	"majorMarkerColor"
#define XmNminorMarkerColor     "minorMarkerColor"
#define XmNmajorMarkerThickness	"majorMarkerThickness"
#define XmNminorMarkerThickness "minorMarkerThickness"

#define XmNindicatorWidth	"indicatorWidth"
#define XmNindicatorColor       "indicatorColor"

#define XmNshading		"shading"
#define XmNshadePercentShadow	"shadePercentShadow"
#define XmNshadeIncreasingColor	"shadeIncreasingColor"
#define XmNshadeDecreasingColor "shadeDecreasingColor"
#define XmNincreasingDirection	"increasingDirection"
#define XmNdecimalPlaces	"decimalPlaces"
#define XmCDecimalPlaces	"DecimalPlaces"

#define XmCMarkers		"Markers"
#define XmCMin			"Min"
#define XmCMax			"Max"
#define XmCInc			"Inc"
#define	XmCPos			"Pos"
#define XmCClockDirection	"ClockDirection"

#define CLOCKWISE               0
#define COUNTERCLOCKWISE        1

#define XmNstart	 "start"
#define XmCStart	 "Start"
#define XmNnext	 	 "next"
#define XmCNext	 	 "Next"
#define XmNcurrent	 "current"
#define XmCCurrent	 "Current"
#define XmNstop 	 "stop"
#define XmCStop 	 "Stop"
#define XmNvalueCallback "valueCallback"
#define XmCValueCallback "ValueCallback"
#define XmNlimitColor	 "limitColor"
#define XmCLimitColor	 "LimitColor"
#define XmNcurrentColor  "currentColor"
#define XmCCurrentColor  "CurrentColor"
#define XmNnextColor 	 "nextColor"
#define XmCNextColor     "NextColor"
#define XmNcurrentVisible "currentVisible"
#define XmCCurrentVisible "CurrentVisible"

#define XmNframeBuffer		"frameBuffer"
#define XmCFrameBuffer		"FrameBuffer"
#define XmNmotionCallback	"motionCallback"
#define XmNsendMotion		"sendMotion"
#define XmCSendMotion		"SendMotion"
#define XmN8supported		"8supported"
#define XmN12supported		"12supported"
#define XmN15supported		"15supported"
#define XmN16supported		"16supported"
#define XmN24supported		"24supported"
#define XmN32supported		"32supported"
#define XmCSupported		"Supported"

#define XmNdataType	 "dataType"
#define XmCDataType	 "DataType"
#define XmNcharPlaces	 "charPlaces"
#define XmCCharPlaces	 "CharPlaces"
#define XmNcenter	 "center"
#define XmCCenter	 "Center"
#define XmNnoEvents	 "noEvents"
#define XmCNoEvents	 "NoEvents"
#define XmNraised	 "raised"
#define XmCRaised	 "Raised"
#define XmNvisible	 "visible"
#define XmCVisible	 "Visible"
#define XmNfixedNotation "fixedNotation"
#define XmCFixedNotation "FixedNotation"
#define XmNoverflowCallback "overflowCallback"

/*  Values given as double float or int  */
#define XmNdValue	 "dValue"
#define XmNfValue	 "fValue"
#define XmNiValue	 "iValue"
#define XmCDValue	 "DValue"
#define XmCFValue	 "FValue"
#define XmCIValue	 "IValue"
#define XmNdMinimum	 "dMinimum"
#define XmNfMinimum	 "fMinimum"
#define XmNiMinimum	 "iMinimum"
#define XmCDMinimum	 "DMinimum"
#define XmCFMinimum	 "FMinimum"
#define XmCIMinimum	 "IMinimum"
#define XmNdMaximum	 "dMaximum"
#define XmNfMaximum	 "fMaximum"
#define XmNiMaximum	 "iMaximum"
#define XmCDMaximum	 "DMaximum"
#define XmCFMaximum	 "FMaximum"
#define XmCIMaximum	 "IMaximum"

#define INTEGER	0
#define FLOAT	1
#define	DOUBLE	2

#define XmNtuples               "tuples"
#define XmNvectors              "vectors"
#define XmNreadonlyVectors      "readonlyVectors"
#define XmNvectorCount          "vectorCount"
#define XmNvectorSpacing        "vectorSpacing"
#define XmNscrolledWindow       "scrolledWindow"
#define XmNscrolledWindowWidget "scrolledWindowWidget"
#define XmNminColumnWidth	"minColumnWidth"
#define XmCMinColumnWidth	"MinColumnWidth"
#ifndef XmNselectCallback
#  define XmNselectCallback       "selectCallback"
#endif

#define XmNdisplayGlobe			"displayGlobe"
#define XmCDisplayGlobe			"DisplayGlobe"
#define XmNlookAtAngle			"lookAtAngle"
#define XmCLookAtAngle			"LookAtAngle"
#define XmNlookAtDirection		"lookAtDirection"
#define XmCLookAtDirection		"LookAtDirection"
#define XmNnavigateDirection		"navigateDirection"
#define XmCNavigateDirection		"NavigateDirection"
#define XmNtranslateSpeed		"translateSpeed"
#define XmCTranslateSpeed		"TranslateSpeed"
#define XmNrotateSpeed			"rotateSpeed"
#define XmCRotateSpeed			"RotateSpeed"
#define XmNnewImage			"newImage"
#define XmCNewImage			"NewImage"
#define XmNconstrainCursor		"XmNconstrainCursor"
#define XmCConstrainCursor		"XmConstrainCursor"
#define XmNoverlayExposure		"XmNoverlayExposure"
#define XmCOverlayExposure		"XmCOverlayExposure"
#define XmNoverlayWid                   "overlayWid"
#define XmCOverlayWid                   "OverlayWid"
#define XmNmode				"mode"
#define XmCMode				"Mode"
#define XmNpicturePixmap		"picturePixmap"
#define XmCPicturePixmap		"PicturePixmap"
#define XmNshowRotatingBBox		"showRotatingBBox"
#define XmCShowRotatingBBox		"ShowRotatingBBox"
#define XmNrotationCenter		"rotationCenter"
#define XmCRotationCenter		"RotationCenter"
#define XmNbasis			"basis"
#define XmCBasis			"Basis"
#define XmNzIncrement			"zIncrement"
#define XmCZIncrement			"XIncrement"
#define XmNboxGrey			"boxGrey"
#define XmCBoxGrey			"BoxGrey"
#define XmNselectedInCursorColor	"selectedInCursorColor"
#define XmNselectedOutCursorColor	"selectedOutCursorColor"
#define XmCSelectedCursorColor		"SelectedCursorColor"
#define XmNunselectedInCursorColor	"unselectedInCursorColor"
#define XmNunselectedOutCursorColor	"unselectedOutCursorColor"
#define XmCUnselectedCursorColor	"UnselectedCursorColor"
#define XmNpictureTransformation        "pictureTransformation"
#define XmCPictureTransformation        "PictureTransformation"
#define XmNpictureCursorColors          "pictureCursorColors"
#define XmCPictureCursorColors          "PictureCursorColors"
#define XmNpictureCursorSize            "pictureCursorSize"
#define XmCPictureCursorSize            "PictureCursorSize"
#define XmNpictureCursorSpeed           "pictureCursorSpeed"
#define XmCPictureCursorSpeed           "PictureCursorSpeed"
#define XmNglobeRadius                  "globeRadius"
#define XmCGlobeRadius 	                "GlobeRadius"
#define XmNcursorShape                  "cursorShape"
#define XmCCursorShape                  "CursorShape"
#define XmNcursorCallback		"cursorCallback"
#define XmNpickCallback			"pickCallback"
#define XmNrotationCallback             "rotationCallback"
#define XmNzoomCallback       		"zoomCallback"
#define XmNroamCallback       		"roamCallback"
#define XmNnavigateCallback 		"navigateCallback"
#define XmNclientMessageCallback       	"clientMessageCallback"
#define XmNpropertyNotifyCallback      	"propertyNotifyCallback"
#define XmNmodeCallback       		"modeCallback"
#define XmNundoCallback       		"undoCallback"
#define XmNkeyCallback       		"keyCallback"

#define XmNULL_MODE 	   	0
#define XmCURSOR_MODE      	1
#define XmROTATION_MODE    	2
#define XmZOOM_MODE	 	3
#define XmPANZOOM_MODE	 	4
#define XmROAM_MODE	   	5
#define XmNAVIGATE_MODE  	6
#define XmPICK_MODE 		7

#define XmPCR_CREATE	   	0
#define XmPCR_DELETE	   	1
#define XmPCR_MOVE	   	2
#define XmPCR_SELECT	   	3
#define XmPCR_DRAG	   	4
#define XmPCR_TRANSLATE_SPEED	5
#define XmPCR_ROTATE_SPEED	6
#define XmPCR_MODE		7
#define XmPCR_UNDO		8
#define XmPCR_KEY		9

#define XmCR_NUM_WARN_DP_INT		0
#define XmCR_NUM_WARN_NO_NEG		1
#define XmCR_NUM_WARN_INVALID_CHAR	2
#define XmCR_NUM_WARN_NO_FLOAT		3
#define XmCR_NUM_WARN_NO_EXPONENT	4
#define XmCR_NUM_WARN_EXCESS_PRECISION	5
#define XmCR_NUM_WARN_ABOVE_MAX		6
#define XmCR_NUM_WARN_BELOW_MIN		7
#define XmCR_NUM_WARN_BAD_INPUT		8

#define XmCONSTRAIN_NONE   0
#define XmCONSTRAIN_X	   1
#define XmCONSTRAIN_Y	   2
#define XmCONSTRAIN_Z	   3

#define XmSLOW_CURSOR	   0
#define XmMEDIUM_CURSOR	   1
#define XmFAST_CURSOR	   2

#define FRONT		1
#define OFF_FRONT	2
#define BACK		3
#define OFF_BACK	4
#define TOP		5
#define OFF_TOP		6
#define BOTTOM		7
#define OFF_BOTTOM	8
#define RIGHT		9
#define OFF_RIGHT	10
#define LEFT		11
#define OFF_LEFT	12
#define DIAGONAL	13
#define OFF_DIAGONAL	14

#define XmFORWARD	0
#define XmBACKWARD	1

#define XmTURN_LEFT	0
#define XmTURN_RIGHT	1
#define XmTURN_UP	2
#define XmTURN_DOWN	3
#define XmTURN_BACK	4

#define XmLOOK_LEFT	0
#define XmLOOK_RIGHT	1
#define XmLOOK_UP	2
#define XmLOOK_DOWN	3
#define XmLOOK_BACKWARD	4
#define XmLOOK_FORWARD	5

#define XmNmarkColor	"markColor"
#define XmNminValue	"minValue"
#define XmCMinValue	"MinValue"
#define XmNmaxValue	"maxValue"
#define XmNxMargin	"xMargin"
#define XmCXMargin	"XMargin"
#define XmNyMargin	"yMargin"
#define XmCYMargin	"YMargin"
#define XmNalignOnDrop	"alignOnDrop"
#define XmCAlignOnDrop	"AlignOnDrop"
#define XmNjumpToGrab	"jumpToGrab"
#define XmCJumpToGrab	"JumpToGrab"
#define XmNallowInput	"allowInput"
#define XmCAllowInput	"AllowInput"

/*
 * Define new resource strings for the Number widget.
 */
#define XmNwarningCallback "warningCallback"
#define XmNcrossOver		"crossOver"
#define XmCCrossOver		"CrossOver"
#define XmNcrossUnder		"crossUnder"
#define XmCCrossUnder		"CrossUnder"

/*
 * Define new resource strings for the Stepper widget.
 */
#define XmNdValueStep	 "dValueStep"
#define XmCDValueStep	 "DValueStep"
#define XmNfValueStep	 "fValueStep"
#define XmCFValueStep	 "FValueStep"
#define XmNiValueStep	 "iValueStep"
#define XmCIValueStep	 "IValueStep"
#define XmNdigits	 "digits"
#define XmCDigits	 "Digits"
#define XmNtime		 "time"
#define XmCTime		 "Time"
#define XmNtimeDelta	 "timeDelta"
#define XmCTimeDelta	 "TimeDelta"
#define XmNrollOver	 "rollOver"
#define XmCRollOver	 "RollOver"
/*  Directions use Xm.h Xm_ARROW_UP, etc.  */
#define XmNincreaseDirection "increaseDirection"
#define XmCIncreaseDirection "IncreaseDirection"
#define XmNstepCallback	 "stepCAllback"
#define XmCStepCallback	 "StepCAllback"

#define	VCR_FORWARD	0
#define	VCR_BACKWARD 	1
#define	VCR_LOOP 	2
#define	VCR_PALINDROME 	3
#define	VCR_STEP 	4
#define	VCR_COUNT 	5
#define	VCR_STOP 	6
#define	VCR_PAUSE 	7
#define	VCR_FORWARD_STEP  8
#define	VCR_BACKWARD_STEP 9

#define	VCR_NOTHING	100 

#define	XmCR_NEXT	0
#define XmCR_CURRENT	1
#define	XmCR_START	2
#define	XmCR_STOP	4
#define	XmCR_MIN	5
#define	XmCR_MAX	6

/* define resource strings */

#define	XmNactionCallback	"action_callback"
#define	XmNframeCallback	"frame_callback"
#define	XmNframeSensitive	"frameSensitive"
#define	XmNminSensitive		"minSensitive"
#define	XmNmaxSensitive		"maxSensitive"
#define	XmNincSensitive		"incSensitive"

#define	XmCActionCallback	"Action_callback"
#define	XmCFrameCallback	"Frame_callback"
#define	XmCFrameSensitive	"FrameSensitive"
#define	XmCMinSensitive		"MinSensitive"
#define	XmCMaxSensitive		"MaxSensitive"
#define	XmCIncSensitive		"IncSensitive"

#define XmNforwardButtonState	"forwardButtonState"
#define XmCForwardButtonState	"ForwardButtonState"
#define XmNbackwardButtonState	"backwardButtonState"
#define XmCBackwardButtonState	"BackwardButtonState"
#define XmNloopButtonState	"loopButtonState"
#define XmCLoopButtonState	"LoopButtonState"
#define XmNpalindromeButtonState	"palindromeButtonState"
#define XmCPalindromeButtonState	"PalindromeButtonState"
#define XmNstepButtonState	"stepButtonState"
#define XmCStepButtonState	"StepButtonState"
#define XmNcountButtonState	"countButtonState"
#define XmCCountButtonState	"CountButtonState"

#define XmRWorkspaceType        "WorkspaceType"

#define XmALIGNMENT_NONE	100

#define XmNautoArrange		"autoArrange"
#define XmCAutoArrange		"AutoArrange"
#define XmNforceRoute    	"forceRoute"
#define XmCForceRoute    	"ForceRoute"
#define XmNcollisionSpacing    "collisionSpacing"
#define XmCCollisionSpacing    "CollisionSpacing"
#define XmNhaloThickness       "haloThickness"
#define XmCHaloThickness       "HaloThickness"
#define XmNpositionChangeCallback       "positionChangeCallback"
#define XmCPositionChangeCallback       "PositionChangeCallback"
#define XmNerrorCallback       "errorCallback"
#define XmCErrorCallback       "ErrorCallback"
#define XmNaccentCallback       "accentCallback"
#define XmCAccentCallback       "AccentCallback"
#define XmNaccentColor          "accentColor"
#define XmCAccentColor          "AccentColor"
#define XmNaccentPolicy         "accentPolicy"
#define XmCAccentPolicy         "AccentPolicy"
#define XmNallowMovement        "allowMovement"
#define XmCAllowMovement        "AllowMovement"
#define XmNallowVerticalResizing        "allowVerticalResizing"
#define XmNallowHorizontalResizing      "allowHorizontalResizing"
#define XmNstretchOnResize      "stretchOnResize"
#define XmCStretchOnResize      "StretchOnResize"
#define XmNpinLeftRight   	"pinLeftRight"
#define XmNpinTopBottom   	"pinTopBottom"
#define XmCPinSides	        "PinSides"
#define XmCAllowResizing        "AllowResizing"
#define XmNbackgroundCallback   "backgroundCallback"
#define XmCDrawGrid             "DrawGrid"
#define XmNgridHeight           "gridHeight"
#define XmCGridHeight           "GridHeight"
#define XmNgridWidth            "gridWidth"
#define XmCGridWidth            "GridWidth"
#define XmNhorizontalAlignment  "horizontalAlignment"
#define XmNhorizontalDrawGrid   "horizontalDrawGrid"
#define XmNid                   "id"
#define XmCId                   "Id"
#define XmNinclusionPolicy      "inclusionPolicy"
#define XmCInclusionPolicy      "InclusionPolicy"
#define XmNlineThickness        "lineThickness"
#define XmCLineThickness        "LineThickness"
#define XmNmanhattanRoute       "manhattanRoute"
#define XmCManhattanRoute       "ManhattanRoute"
#define XmNoutlineType          "outlineType"
#define XmCOutlineType          "OutlineType"
#define XmNplacementPolicy      "placementPolicy"
#define XmCPlacementPolicy      "PlacementPolicy"
#define XmNresizingCallback     "resizingCallback"
#define XmCResizingCallback     "ResizingCallback"
#define XmCSelectionCallback    "SelectionCallback"
#define XmNselectable           "selectable"
#define XmCSelectable           "Selectable"
#define XmNselected             "selected"
#define XmCSelected             "Selected"
#define XmNsnapToGrid           "snapToGrid"
#define XmCSnapToGrid           "SnapToGrid"
#define XmNsortPolicy           "sortPolicy"
#define XmNuserCallback         "userCallback"
#define XmCUserCallback         "UserCallback"
#define XmNverticalAlignment    "verticalAlignment"
#define XmNverticalDrawGrid     "verticalDrawGrid"
#define XmNbutton1PressMode	"button1PressMode"
#define XmCButton1PressMode	"Button1PressMode"
#define XmNwidgetTolerance	"widgetTolerance"
#define XmCWidgetTolerance	"WidgetTolerance"
#define XmNlineTolerance	"lineTolerance"
#define XmCLineTolerance	"LineTolerance"
#define XmNlineDrawingEnabled	"lineDrawingEnabled"
#define XmCLineDrawingEnabled	"LineDrawingEnabled"
#define XmNpreventOverlap	"preventOverlap"

#if !defined(XmNselectionCallback)
#define XmNselectionCallback    "selectionCallback"
#endif
#if !defined(XmNposition)
#define XmNposition		"position"
#endif
#if !defined(XmNarrowSize)
#define XmNarrowSize	"arrowSize"
#endif
#if !defined(XmCArrowSize)
#define XmCArrowSize	"ArrowSize"
#endif
/*
 * Workspace placement policy constants.
 * ...added 3/95 to replace integers used.  The values
 * 0,1,2 were hardcoded throughout WorkspaceW.c, making the code difficult to read.
 * XmSPACE_WARS_SELECTED_STAYS is the default and is used in the vpe.
 * The use of these constants is for deciding who should move if there is overlap
 * among child widgets in the Workspace.
 * - Martin
 */
#define XmSPACE_WARS_SELECTED_STAYS             2
#define XmSPACE_WARS_LEFT_MOST_STAYS            1
#define XmSPACE_WARS_SELECTED_MIGRATE           0

/*
 * constraint resources provided by the workspace widget which hold on to the
 * form constraints.  The values are stuck into the form class when turning
 * autoArrange on.
 */
#define XmNwwLeftAttachment   "wwLeftAttachment"
#define XmNwwRightAttachment  "wwRightAttachment"
#define XmNwwTopAttachment    "wwTopAttachment"
#define XmNwwBottomAttachment "wwBottomAttachment"
#define XmNwwLeftPosition     "wwLeftPosition"
#define XmNwwRightPosition    "wwRightPosition"
#define XmNwwTopPosition      "wwTopPosition"
#define XmNwwBottomPosition   "wwBottomPosition"
#define XmNwwLeftOffset       "wwLeftOffset"
#define XmNwwRightOffset      "wwRightOffset"
#define XmNwwTopOffset        "wwTopOffset"
#define XmNwwBottomOffset     "wwBottomOffset"
#define XmNwwLeftWidget       "wwLeftWidget"
#define XmNwwRightWidget      "wwRightWidget"
#define XmNwwTopWidget        "wwTopWidget"
#define XmNwwBottomWidget     "wwBottomWidget"

#define OFF     0
#define ON      1

#define XmINCLUDE_ALL           0
#define XmINCLUDE_ANY           1
#define XmINCLUDE_CENTER        2
#define XmACCENT_BACKGROUND     2
#define XmACCENT_BORDER         1
#define XmACCENT_NONE           0
#define XmCR_SELECT             90
#define XmCR_ACCENT             91
#define XmCR_BACKGROUND         92
#define XmCR_ERROR		93
#define XmCR_USER_NOTIFY	94
#define XmOUTLINE_EACH          70
#define XmOUTLINE_ALL           71
#define XmOUTLINE_PLUS          72
#define XmDRAW_NONE             0
#define XmDRAW_HASH             1
#define XmDRAW_LINE             2
#define XmDRAW_OUTLINE          3


/*
// For alpha (and any other architecture where sizeof(double) <= sizeof(long)),
// pass double resources by value.  
// For all others, pass by reference. Xt checks to see if XtArgType
// (usually defined as a long) is shorter than the type being passed in.  If
// it is, it does a "pass by reference", else "pass by value"
//
// XtDoubleSetArg is to be used in place of XtSetArg when a double resource
//                is being set.
// DoubleVal is to be used in XtVaSetValues, for example:
//	XtVaSetValues(w, XmNdValue, DoubleVal(dval, l), NULL);
//
// Note DoubleVal needs an XtArgVal as it's second parameter in case we
// are using it twice in the same XtVaSetValues.  In this case, two
// separate and distinct XtArgVal params should be supplied
//
*/
#if defined(alphax) || defined(__ia64__) || defined(__LP64__)
#define PASSDOUBLEVALUE 1
#endif

#if PASSDOUBLEVALUE
#define DoubleSetArg(a, b, c)				\
	(a).name = (b);					\
	memcpy(&((a).value), &c, sizeof(double));

#define DoubleVal(a, dx_l)				\
	(memcpy(&dx_l, &a, sizeof(double)), dx_l)
#else
#define DoubleSetArg(a, b, c)				\
	(a).name = (b);					\
	(a).value =  (XtArgVal)&(c);

#define DoubleVal(a, l) &a                         

#endif
#endif
