/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/


/* this file has #defines for all dx functions,
 * plus the old declarations for routines which are now obsolete.
 */


#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#define delete __delete__
#endif

/* THESE ROUTINES ARE NOW OBSOLETE. */
Array DXMakeGridV(int n, int *counts, float *origins, float *deltas);
Array DXMakeGrid(int n, ...);
Array DXQueryGrid(Array a, int *n, int *counts, float *origins, float *deltas);
Error DXCacheInsertObject(char *id, Object o, double cost);
Error DXCacheInsert(char *id, Pointer data, int (*delete)(), double cost);
Error DXCacheDelete(char *id);
Error DXCacheSearch(char *id, Pointer *data);
typedef struct groupiterator *GroupIterator;
typedef struct itemiterator *ItemIterator;
enum iter_rw { ITER_READONLY, ITER_READWRITE, ITER_WRITEONLY };
enum iter_local { ITER_GLOBAL, ITER_LOCAL };
enum iter_attr { ITER_DEP, ITER_REF };
GroupIterator DXNewGroupIterator(Object root, int isField);
Error DXResetGroupIterator(GroupIterator gi);
Object DXGetNextPart(GroupIterator gi);
ItemIterator DXNewItemIterator(Object model, char *index_component,
			     char **buffer_components, enum iter_rw *rw,
			     enum iter_local *local, enum iter_attr *attr,
		             int n);
Error DXResetItemIterator(ItemIterator ii);
Error DXGetNextItems(ItemIterator ii, Field *field, Pointer *buffers, int *n);
Group _dxfAutoColor(Object o, float opacity, float intensity,
		float phase, float range, float saturation,
		float *inputmin, float *inputmax, Object *map, int delayed,
                RGBColor colormin, RGBColor colormax); 
Object DXSetColor(Object g, float *rd, float *gr, float *bl, float *opacity);
Object DXWrite(Object o, char *name);
Object DXRead(char *name);
/* use DXApplyTransform() instead */
Object DXTransform(Object o, Matrix *tp, Camera c);
#define CLASS_MIXEDFIELD 1000
#define CLASS_PYRAMID    1001



/* all libDX routines now start with DX.  use this file with care --
 *  if you have variables with the same name as old DX routines,
 *  you may have problems using this.
 */

#define create_lock		DXcreate_lock
#define destroy_lock		DXdestroy_lock
#define lock			DXlock
#define try_lock		DXtry_lock
#define unlock			DXunlock
#define SetGlobalSize		DXSetGlobalSize
#define memsize			DXmemsize
#define initdx			DXinitdx
#define syncmem			DXsyncmem
#define memfork			DXmemfork
#define qmessage		DXqmessage
#define sqmessage		DXsqmessage
#define qwrite			DXqwrite
#define qflush			DXqflush
#define NewArrayV		DXNewArrayV
#define NewArray		DXNewArray
#define GetArrayClass		DXGetArrayClass
#define GetArrayInfo		DXGetArrayInfo
#define TypeCheckV		DXTypeCheckV
#define TypeCheck		DXTypeCheck
#define GetArrayData		DXGetArrayData
#define GetItemSize		DXGetItemSize
#define GetArrayDataLocal	DXGetArrayDataLocal
#define FreeArrayDataLocal	DXFreeArrayDataLocal
#define AddArrayData		DXAddArrayData
#define AllocateArray		DXAllocateArray
#define Trim			DXTrim
#define MakeGridPositionsV	DXMakeGridPositionsV
#define MakeGridPositions	DXMakeGridPositions
#define QueryGridPositions	DXQueryGridPositions
#define MakeGridConnectionsV	DXMakeGridConnectionsV
#define MakeGridConnections	DXMakeGridConnections
#define QueryGridConnections	DXQueryGridConnections
#define NewRegularArray		DXNewRegularArray
#define GetRegularArrayInfo	DXGetRegularArrayInfo
#define NewPathArray		DXNewPathArray
#define GetPathArrayInfo	DXGetPathArrayInfo
#define SetPathOffset		DXSetPathOffset
#define GetPathOffset		DXGetPathOffset
#define NewProductArrayV	DXNewProductArrayV
#define NewProductArray		DXNewProductArray
#define GetProductArrayInfo	DXGetProductArrayInfo
#define NewMeshArrayV		DXNewMeshArrayV
#define NewMeshArray		DXNewMeshArray
#define GetMeshArrayInfo	DXGetMeshArrayInfo
#define SetMeshOffsets		DXSetMeshOffsets
#define GetMeshOffsets		DXGetMeshOffsets
#define NewConstantArray	DXNewConstantArray
#define NewConstantArrayV	DXNewConstantArrayV
#define QueryConstantArray	DXQueryConstantArray
#define GetConstantArrayData	DXGetConstantArrayData
#define Pt			DXPt
#define Vec			DXVec
#define Ln			DXLn
#define Tri			DXTri
#define Quad			DXQuad
#define Tetra			DXTetra
#define RGB			DXRGB
#define RotateX			DXRotateX
#define RotateY			DXRotateY
#define RotateZ			DXRotateZ
#define Scale			DXScale
#define Translate		DXTranslate
#define Mat			DXMat
#define Neg			DXNeg
#define Normalize		DXNormalize
#define Length			DXLength
#define Add			DXAdd
#define Sub			DXSub
#define Min			DXMin
#define Max			DXMax
#define Mul			DXMul
#define Div			DXDiv
#define Dot			DXDot
#define Cross			DXCross
#define Concatenate		DXConcatenate
#define Invert			DXInvert
#define Transpose		DXTranspose
#define AdjointTranspose	DXAdjointTranspose
#define Determinant		DXDeterminant
#define Apply			DXApply
#define SetCacheEntry		DXSetCacheEntry
#define SetCacheEntryV		DXSetCacheEntryV
#define GetCacheEntry		DXGetCacheEntry
#define GetCacheEntryV		DXGetCacheEntryV
#define NewCamera		DXNewCamera
#define SetView			DXSetView
#define SetOrthographic		DXSetOrthographic
#define SetPerspective		DXSetPerspective
#define SetResolution		DXSetResolution
#define GetCameraMatrix		DXGetCameraMatrix
#define GetCameraRotation	DXGetCameraRotation
#define GetCameraMatrixWithFuzz	DXGetCameraMatrixWithFuzz
#define GetView			DXGetView
#define GetCameraResolution	DXGetCameraResolution
#define GetOrthographic		DXGetOrthographic
#define GetPerspective		DXGetPerspective
#define GetBackgroundColor	DXGetBackgroundColor
#define SetBackgroundColor	DXSetBackgroundColor
#define NewClipped		DXNewClipped
#define GetClippedInfo		DXGetClippedInfo
#define SetClippedObjects	DXSetClippedObjects
#define Rename			DXRename
#define Swap			DXSwap
#define Extract			DXExtract
#define Insert			DXInsert
#define Replace			DXReplace
#define Remove			DXRemove
#define Exists			DXExists
#define SetError		DXSetError
#define AddMessage		DXAddMessage
#define GetError		DXGetError
#define GetErrorMessage		DXGetErrorMessage
#define ResetError		DXResetError
#define Warning			DXWarning
#define Message			DXMessage
#define UIMessage		DXUIMessage
#define BeginLongMessage	DXBeginLongMessage
#define EndLongMessage		DXEndLongMessage
#define Debug			DXDebug
#define EnableDebug		DXEnableDebug
#define QueryDebug		DXQueryDebug
#define SetErrorExit		DXSetErrorExit
#define ErrorExit		DXErrorExit
#define PrintError		DXPrintError
#define ExtractInteger		DXExtractInteger
#define ExtractFloat		DXExtractFloat
#define ExtractString		DXExtractString
#define ExtractNthString	DXExtractNthString
#define QueryParameter		DXQueryParameter
#define ExtractParameter	DXExtractParameter
#define NewField		DXNewField
#define SetComponentValue	DXSetComponentValue
#define SetComponentAttribute	DXSetComponentAttribute
#define GetComponentValue	DXGetComponentValue
#define GetComponentAttribute	DXGetComponentAttribute
#define GetEnumeratedComponentValue DXGetEnumeratedComponentValue
#define GetEnumeratedComponentAttribute DXGetEnumeratedComponentAttribute
#define DeleteComponent		DXDeleteComponent
#define ComponentReq		DXComponentReq
#define ComponentOpt		DXComponentOpt
#define ComponentReqLoc		DXComponentReqLoc
#define ComponentOptLoc		DXComponentOptLoc
#define GetFont			DXGetFont
#define GeometricText		DXGeometricText
#define ClipPlane		DXClipPlane
#define ClipBox			DXClipBox
#define Ribbon			DXRibbon
#define Tube			DXTube
#define VectorGlyph		DXVectorGlyph
#define NewGroup		DXNewGroup
#define SetMember		DXSetMember
#define GetMember		DXGetMember
#define GetEnumeratedMember	DXGetEnumeratedMember
#define SetEnumeratedMember	DXSetEnumeratedMember
#define SetGroupType		DXSetGroupType
#define SetGroupTypeV		DXSetGroupTypeV
#define GetGroupClass		DXGetGroupClass
#define NewSeries		DXNewSeries
#define SetSeriesMember		DXSetSeriesMember
#define GetSeriesMember		DXGetSeriesMember
#define NewCompositeField	DXNewCompositeField
#define GetPart			DXGetPart
#define GetPartClass		DXGetPartClass
#define SetPart			DXSetPart
#define ProcessParts		DXProcessParts
#define Grow			DXGrow
#define GrowV			DXGrowV
#define QueryOriginalSizes	DXQueryOriginalSizes
#define QueryOriginalMeshExtents DXQueryOriginalMeshExtents
#define Shrink			DXShrink
#define AddPoint		DXAddPoint
#define AddColor		DXAddColor
#define AddFrontColor		DXAddFrontColor
#define AddBackColor		DXAddBackColor
#define AddOpacity		DXAddOpacity
#define AddNormal		DXAddNormal
#define AddFaceNormal		DXAddFaceNormal
#define AddPoints		DXAddPoints
#define AddColors		DXAddColors
#define AddFrontColors		DXAddFrontColors
#define AddBackColors		DXAddBackColors
#define AddOpacities		DXAddOpacities
#define AddNormals		DXAddNormals
#define AddFaceNormals		DXAddFaceNormals
#define AddLine			DXAddLine
#define AddTriangle		DXAddTriangle
#define AddQuad			DXAddQuad
#define AddTetrahedron		DXAddTetrahedron
#define AddLines		DXAddLines
#define AddTriangles		DXAddTriangles
#define AddQuads		DXAddQuads
#define AddTetrahedra		DXAddTetrahedra
#define SetConnections		DXSetConnections
#define GetConnections		DXGetConnections
#define EndField		DXEndField
#define EmptyField		DXEmptyField
#define ChangedComponentValues	DXChangedComponentValues
#define ChangedComponentStructure DXChangedComponentStructure
#define BoundingBox		DXBoundingBox
#define Neighbors		DXNeighbors
#define Statistics		DXStatistics
#define ApplyTransform		DXApplyTransform
#define MakeImage		DXMakeImage
#define GetPixels		DXGetPixels
#define GetImageSize		DXGetImageSize
#define GetImageBounds		DXGetImageBounds
#define OutputRGB		DXOutputRGB
#define DisplayFB		DXDisplayFB
#define DisplayX		DXDisplayX
#define ImportDX		DXImportDX
#define ImportNetCDF		DXImportNetCDF
#define ImportHDF		DXImportHDF
#define MakeGridV		DXMakeGridV
#define MakeGrid		DXMakeGrid
#define QueryGrid		DXQueryGrid
#define CacheInsertObject	DXCacheInsertObject
#define CacheInsert		DXCacheInsert
#define CacheDelete		DXCacheDelete
#define CacheSearch		DXCacheSearch
#define NewGroupIterator	DXNewGroupIterator
#define ResetGroupIterator	DXResetGroupIterator
#define GetNextPart		DXGetNextPart
#define NewItemIterator		DXNewItemIterator
#define ResetItemIterator	DXResetItemIterator
#define GetNextItems		DXGetNextItems
#define SetPerspective		DXSetPerspective
#define SetCenter		DXSetCenter
#define SetSupersampling	DXSetSupersampling
#define SetStereo		DXSetStereo
#define SetFocus		DXSetFocus
#define GetPerspective		DXGetPerspective
#define InvalidateConnections	DXInvalidateConnections
#define InvalidateUnreferencedPositions DXInvalidateUnreferencedPositions
#define Cull			DXCull
#define CullConditional		DXCullConditional
#define NewDistantLight		DXNewDistantLight
#define NewCameraDistantLight	DXNewCameraDistantLight
#define QueryDistantLight	DXQueryDistantLight
#define QueryCameraDistantLight	DXQueryCameraDistantLight
#define NewAmbientLight		DXNewAmbientLight
#define QueryAmbientLight	DXQueryAmbientLight
#define AutoColor		DXAutoColor
#define SetColor		DXSetColor
#define Allocate		DXAllocate
#define AllocateZero		DXAllocateZero
#define AllocateLocal		DXAllocateLocal
#define AllocateLocalZero	DXAllocateLocalZero
#define ReAllocate		DXReAllocate
#define Free			DXFree
#define RegisterScavenger	DXRegisterScavenger
#define RegisterScavengerLocal	DXRegisterScavengerLocal
#define PrintAlloc		DXPrintAlloc
#define FindAlloc		DXFindAlloc
#define FoundAlloc		DXFoundAlloc
#define GetObjectClass		DXGetObjectClass
#define Reference		DXReference
#define Delete			DXDelete
#define GetObjectTag		DXGetObjectTag
#define SetObjectTag		DXSetObjectTag
#define SetAttribute		DXSetAttribute
#define GetAttribute		DXGetAttribute
#define GetEnumeratedAttribute	DXGetEnumeratedAttribute
#define SetFloatAttribute	DXSetFloatAttribute
#define SetIntegerAttribute	DXSetIntegerAttribute
#define SetStringAttribute	DXSetStringAttribute
#define CopyAttributes		DXCopyAttributes
#define Copy			DXCopy
#define GetType			DXGetType
#define TypeSize		DXTypeSize
#define CategorySize		DXCategorySize
#define Print			DXPrint
#define PrintV			DXPrintV
#define Write			DXWrite
#define Read			DXRead
#define Partition		DXPartition
#define NewPrivate		DXNewPrivate
#define GetPrivateData		DXGetPrivateData
#define Render			DXRender
#define Transform		DXTransform
#define NewInterpolator		DXNewInterpolator
#define Interpolate		DXInterpolate
#define LocalizeInterpolator	DXLocalizeInterpolator
#define Map			DXMap
#define MapArray		DXMapArray
#define MapCheck		DXMapCheck
#define NewScreen		DXNewScreen
#define GetScreenInfo		DXGetScreenInfo
#define SetScreenObject		DXSetScreenObject
#define NewSegList		DXNewSegList
#define NewSegListSegment	DXNewSegListSegment
#define DeleteSegList		DXDeleteSegList
#define NewSegListItem		DXNewSegListItem
#define GetNextSegListItem	DXGetNextSegListItem
#define InitGetNextSegListItem	DXInitGetNextSegListItem
#define GetSegListItemCount	DXGetSegListItemCount
#define InitGetNextSegListSegment  DXInitGetNextSegListSegment
#define GetNextSegListSegment	   DXGetNextSegListSegment
#define GetSegListSegmentPointer   DXGetSegListSegmentPointer
#define GetSegListSegmentItemCount DXGetSegListSegmentItemCount
#define NewString		DXNewString
#define GetString		DXGetString
#define CreateTaskGroup		DXCreateTaskGroup
#define AddTask			DXAddTask
#define AbortTaskGroup		DXAbortTaskGroup
#define ExecuteTaskGroup	DXExecuteTaskGroup
#define Processors		DXProcessors
#define ProcessorId		DXProcessorId
#define MarkTime		DXMarkTime
#define MarkTimeLocal		DXMarkTimeLocal
#define MarkTimeX		DXMarkTimeX
#define MarkTimeLocalX		DXMarkTimeLocalX
#define PrintTimes		DXPrintTimes
#define TraceTime		DXTraceTime
#define GetTime			DXGetTime
#define NewXform		DXNewXform
#define GetXformInfo		DXGetXformInfo
#define SetXformObject		DXSetXformObject
#define hashFunc		DXhashFunc
#define cmpFunc			DXcmpFunc
#define CreateHash		DXCreateHash
#define DestroyHash		DXDestroyHash
#define QueryHashElement	DXQueryHashElement
#define DeleteHashElement	DXDeleteHashElement
#define InsertHashElement	DXInsertHashElement
#define InitGetNextHashElement	DXInitGetNextHashElement
#define GetNextHashElement	DXGetNextHashElement
#define ColorName		DXColorName
#define ScalarConvert		DXScalarConvert
#define ErrorReturn		DXErrorReturn
#define ErrorGoto		DXErrorGoto
#define MessageReturn		DXMessageReturn
#define MessageGoto		DXMessageGoto

#if defined(__cplusplus) || defined(c_plusplus)
#undef delete
}
#endif
