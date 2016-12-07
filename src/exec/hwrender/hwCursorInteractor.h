/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


/*
 *  Private data structures for the Cursor interactor.
 *
 */

#define COORD_WIDTH 350
#define COORD_HEIGHT 25

#define XmCURSOR_MODE 1
#define XmROAM_MODE   4
#define XmPICK_MODE   7

#define XmSQUARE      0
#define Z_LEVELS     16

typedef struct
{
  double Bw[4][4] ;
} XmBasis ;

struct pnt 
{
  double xcoor, ycoor, zcoor ;
  double txcoor, tycoor, tzcoor ;
  double pxcoor, pycoor, pzcoor, pwcoor ;
  double sxcoor, sycoor ;
  short visible ;
} ;

typedef struct
{
  /* state variables */
  int box_state, box_change, view_state, visible ;

  /* storage for bounding box */
  XmBasis *basis ;

  /* roam, cursor, or pick mode */
  int mode ;

  /* text coordinate feedback area, save-under, and font */
  long coord_llx, coord_lly, coord_urx, coord_ury ;
  void *coord_saveunder ;
  int font ;

  /* buffer for cursor save-under */
  void *cursor_saveunder ;
  
  /* active and passive cursor pixmaps, projection mark pixmap */
  void *ActiveSquareCursor ;
  void *PassiveSquareCursor ;
  void *ProjectionMark ;
  
  /* cursor state info */
  int CursorNotBlank, FirstTime, FirstTimeMotion ;
  int n_cursors, cursor_num, cursor_shape, cursor_size ;

  /* coords of the 3D cursor */
  double X, Y, Z ;	
  /* last active cursor location (screen) */
  int pcx, pcy ;        

  /* pointer history buffer */
  int px, ppx, pppx, ppppx, pppppx, ppppppx, pppppppx ;
  int py, ppy, pppy, ppppy, pppppy, ppppppy, pppppppy ;
  /* length of pointer history buffer */
  int K ; 

  /* transformation matrices */
  double Wtrans[4][4] ;
  double WItrans[4][4] ; 
  double PureWtrans[4][4] ;
  double PureWItrans[4][4] ; 

  /* projection mark info */
  int iMark, piMark ;
  double Xmark[3], Ymark[3], Zmark[3], pXmark[3], pYmark[3], pZmark[3] ;

  /* bounding box info */
  double Zmax, Zmin;
  double Ox, Oy, Oz ;
  double FacetXmax, FacetYmax, FacetZmax, FacetXmin, FacetYmin, FacetZmin ;
  struct pnt Face1[4], Face2[4] ;

  /* arrays of screen coords for cursors */
  int *xbuff, *ybuff ;  
  double *zbuff;

  /* arrays of cursor positions in canonical form */
  double *cxbuff, *cybuff, *czbuff ;

  /* array of selection flags, one for each cursor */
  int *selected ;

  /* roam cursor info */
  int roam_xbuff, roam_ybuff ;
  double roam_zbuff ;
  double roam_cxbuff, roam_cybuff, roam_czbuff ;
  int roam_selected, roam_valid ;

  /* angles of projected world coordinate axes on screen */
  double angle_posx, angle_posy, angle_posz ;
  double angle_negx, angle_negy, angle_negz ;

  /* cursor movement constraints */
  int x_movement_allowed, y_movement_allowed, z_movement_allowed ;

  /* current frame buffer and display mode */
  int buffermode, displaymode ;

  /* pick cursor info */
  int pick_xbuff, pick_ybuff ;

  /* apply reset at next Redisplay() */
  int reset ;
} CursorData ;

