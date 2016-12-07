/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


#ifndef	tdmObject_h
#define	tdmObject_h

#define String dxString
#define Object dxObject
#define Angle dxAngle
#define Matrix dxMatrix
#define Screen dxScreen
#define Boolean dxBoolean

#include <dx/dx.h>

#undef String
#undef Object
#undef Angle
#undef Matrix
#undef Screen
#undef Boolean

typedef enum {
   HW_CLASS_XFIELD,
   HW_CLASS_SCREEN,
   HW_CLASS_CLIPPED,
   HW_CLASS_GATHER,
   HW_CLASS_VIEW,
   HW_CLASS_LIST,
   HW_CLASS_TRANSLATION,
   HW_CLASS_MATERIAL,
   HW_CLASS_ERROR
} hwClass;

typedef struct hwObjectS  {
    hwClass       class;
    Pointer       item;
    Error         (*delete)(Pointer);
} hwObjectT, *hwObjectP;

typedef	dxObject screenO;
typedef	dxObject xfieldO;
typedef	dxObject clippedO;
typedef	dxObject viewO;
typedef	dxObject listO;
typedef	dxObject gatherO;
typedef	dxObject translationO;
typedef	dxObject materialO;

dxObject _dxf_newHwObject(hwClass class, Pointer item, Error (*delete)());
Error    _dxf_deleteHwObject(Pointer p);
Pointer  _dxf_getHwObjectData(dxObject p);
hwClass  _dxf_getHwClass(dxObject p);

#endif 

