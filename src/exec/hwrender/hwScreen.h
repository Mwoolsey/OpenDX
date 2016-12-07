/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>



screenO _dxf_newHwScreen(dxScreen s, dxObject subObject,
			    float vm[4][4],float pm[4][4],
			    int width, int height);

screenO _dxf_setHwScreenView(screenO sp,
			    dxScreen s, dxObject subObject,
			    float vm[4][4],float pm[4][4],
			    int width, int height);

screenO _dxf_getHwScreenView(screenO sp,
			    dxScreen* s, dxObject* subObject,
			    float vm[4][4],float pm[4][4],
			    int* width, int* height);

