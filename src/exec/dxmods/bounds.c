/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/*
 * $Header: /src/master/dx/src/exec/dxmods/bounds.c,v 1.3 1999/05/10 15:45:22 gda Exp $
 */

#include <dxconfig.h>


#include <math.h>
#include <dx/dx.h>
#include <string.h>
#include "bounds.h"


/* general bounding box functions */

Object _dxf_BBoxPoint(Object o, Point *p, int flag)
{
    Point b[8];
    
    if(!p || !DXBoundingBox(o, b))
        return NULL;

    
    switch(flag) {

      case BB_CENTER:
        p->x = (b[7].x + b[0].x) / 2;
        p->y = (b[7].y + b[0].y) / 2;
        p->z = (b[7].z + b[0].z) / 2;
        break;

      case BB_FRONT:
        p->x = (b[7].x + b[0].x) / 2;
        p->y = (b[7].y + b[0].y) / 2;
        p->z = b[7].z;
        break;

      case BB_BACK:
        p->x = (b[7].x + b[0].x) / 2;
        p->y = (b[7].y + b[0].y) / 2;
        p->z = b[0].z;
        break;

      case BB_RIGHT:
        p->x = b[7].x;
        p->y = (b[7].y + b[0].y) / 2;
        p->z = (b[7].z + b[0].z) / 2;
        break;

      case BB_LEFT:
        p->x = b[0].x;
        p->y = (b[7].y + b[0].y) / 2;
        p->z = (b[7].z + b[0].z) / 2;
        break;

      case BB_TOP:
        p->x = (b[7].x + b[0].x) / 2;
        p->y = b[7].y;
        p->z = (b[7].z + b[0].z) / 2;
        break;

      case BB_BOTTOM:
        p->x = (b[7].x + b[0].x) / 2;
        p->y = b[0].y;
        p->z = (b[7].z + b[0].z) / 2;
        break;

        
      case BB_FRONTTOPRIGHT:
        p->x = b[7].x;
        p->y = b[7].y;
        p->z = b[7].z;
        break;

      case BB_FRONTTOPLEFT:
        p->x = b[0].x;
        p->y = b[7].y;
        p->z = b[7].z;
        break;

      case BB_FRONTBOTRIGHT:
        p->x = b[7].x;
        p->y = b[0].y;
        p->z = b[7].z;
        break;

      case BB_FRONTBOTLEFT:
        p->x = b[0].x;
        p->y = b[0].y;
        p->z = b[7].z;
        break;

      case BB_BACKTOPRIGHT:
        p->x = b[7].x;
        p->y = b[7].y;
        p->z = b[0].z;
        break;

      case BB_BACKTOPLEFT:
        p->x = b[0].x;
        p->y = b[7].y;
        p->z = b[0].z;
        break;

      case BB_BACKBOTRIGHT:
        p->x = b[7].x;
        p->y = b[0].y;
        p->z = b[0].z;
        break;

      case BB_BACKBOTLEFT:
        p->x = b[0].x;
        p->y = b[0].y;
        p->z = b[0].z;
        break;


      default:
        DXSetError(ERROR_BAD_PARAMETER, "bad flag value");
        return NULL;
    }

    return o;
}


Object _dxf_BBoxDistance(Object o, float *distance, int flag)
{
    Point b[8];
    Vector v;

    if(!distance || !DXBoundingBox(o, b))
        return NULL;

    v = DXSub(b[7], b[0]);

    switch(flag) {
      case BB_WIDTH:
        *distance = v.x;
        break;

      case BB_HEIGHT:
        *distance = v.y;
        break;

      case BB_DEPTH:
        *distance = v.z;
        break;

      case BB_DIAGONAL:
        *distance = DXLength(v);
        break;

      default:
        DXSetError(ERROR_BAD_PARAMETER, "bad flag value");
        return NULL;
    }


    return o;
}
