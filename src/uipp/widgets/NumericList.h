/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>



#ifndef NumericList_H
#define NumericList_H

#include "XmDX.h"

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

extern WidgetClass xmNumericListWidgetClass;

typedef struct _XmNumericListClassRec * XmNumericListWidgetClass;
typedef struct _XmNumericListRec      * XmNumericListWidget;

typedef	double	*Vector;
typedef	Vector	*VectorList;

typedef struct {
    unsigned char reason;
    XEvent	  *event;
    int		  position;
    VectorList    vector_list;
} XmNumericListCallbackStruct;

/*  Declare types for convenience routine to create the widget  */
extern Widget XmCreateNumericList
  (Widget parent, String name, ArgList args, Cardinal num_args);

extern Widget XmCreateScrolledNumericList(Widget parent, char *name, 
				ArgList args, int argCount);

Boolean XmNumericListDeleteVector(XmNumericListWidget nlw, int vector_num);

Boolean XmNumericListAddVector(XmNumericListWidget nlw,Vector vector, 
						int vector_num);

Boolean XmNumericListReplaceSelectedVector(XmNumericListWidget nlw, 
							Vector newvec);

#if defined(__cplusplus) || defined(c_plusplus)
 }
#endif

#endif
