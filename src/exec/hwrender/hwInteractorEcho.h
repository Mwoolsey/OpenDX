/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


#ifndef tdmInteractorEcho_h
#define tdmInteractorEcho_h
/*---------------------------------------------------------------------------*\
 $Source: /src/master/dx/src/exec/hwrender/hwInteractorEcho.h,v $
  Author: Mark Hood

  Prototypes and stuff for the routines used to implement TDM direct
  interactor echos.

\*---------------------------------------------------------------------------*/

#include "hwInteractor.h"


struct tdmInteractorEchoS 
{
  void (*BufferConfig)(void *ctx, void *image,int llx,int lly,int urx, int ury,
		       int *CurrentDisplayMode, int *CurrentBufferMode,
 		       tdmInteractorRedrawMode redrawMode) ;
  void (*BufferRestoreConfig)(void *ctx,int OrigDisplayMode,int OrigBufferMode,
			      tdmInteractorRedrawMode redrawMode) ;
  void (*CreateCursorPixmaps)(tdmInteractor I) ;
  void (*DrawArrowhead)(void* ctx, float  ax, float ay) ;
  void (*DrawCursorCoords)(tdmInteractor I,char *text) ;
  void (*DrawGlobe)(tdmInteractor I, void *udata, float[4][4],int draw) ;
  void (*DrawGnomon )(tdmInteractor I, void *udata, float[4][4], int draw) ;
  void (*DrawLine)(void *ctx, int x1, int y1, int x2, int y2) ;
  void (*DrawMarker)(tdmInteractor I) ;
  void (*DrawZoombox)(tdmInteractor I, void *udata, float[4][4],int draw) ;
  void (*EraseCursor)(tdmInteractor I, int x, int y) ;
  void (*ErasePreviousMarks)(tdmInteractor I) ;
  void (*GetMaxscreen)(void *ctx, int *w, int *h) ;
  void (*GetWindowOrigin)(void *ctx, int *x, int *y) ;
  int  (*GetZbufferStatus)(void *ctx) ;
  void (*PointerInvisible)(void *ctx) ;
  void (*PointerVisible)(void *ctx) ;
  void (*ReadBuffer)(void *ctx, int llx, int lly, int urx, int ury,void *buff) ;
  void (*SetLineColorGray)(void *ctx) ;
  void (*SetLineColorWhite)(void *ctx) ;
  void (*SetSolidFillPattern)(void *ctx) ;
  void (*SetWorldScreen)(void *ctx, int w, int h) ;
  void (*SetZbufferStatus)(void *ctx, int status) ;
  void (*WarpPointer)(void *ctx, int x, int y) ;
  void (*WriteBuffer)(void *ctx, int llx, int lly, int urx, int ury,void *buff);
  void (*SetLineAttributes)(void *ctx, int32 color, int style, float width) ;
} ;

#ifndef STRUCTURES_ONLY
#define _dxf_GET_ZBUFFER_STATUS(ctx) \
	 (_eFuncs->GetZbufferStatus)(ctx) 

#define _dxf_SET_ZBUFFER_STATUS(ctx, status) \
	 (_eFuncs->SetZbufferStatus)(ctx, status) 

#define _dxf_SET_SOLID_FILL_PATTERN(ctx) \
	 (_eFuncs->SetSolidFillPattern)(ctx) 

#define _dxf_READ_BUFFER(ctx, llx, lly, urx, ury, buff) \
	 (_eFuncs->ReadBuffer)(ctx, llx, lly, urx, ury, buff) 

#define _dxf_WRITE_BUFFER(ctx, llx, lly, urx, ury, buff) \
	 (_eFuncs->WriteBuffer)(ctx, llx, lly, urx, ury, buff) 

#define _dxf_BUFFER_CONFIG(ctx, image, llx, lly, urx, ury,CurrentDisplayMode, \
		   CurrentBufferMode,redrawMode) 			\
	 (_eFuncs->BufferConfig)(ctx, image, llx, lly, urx, ury, 	\
				   CurrentDisplayMode, CurrentBufferMode, \
				   redrawMode) 

#define _dxf_BUFFER_RESTORE_CONFIG(ctx, OrigDisplayMode, OrigBufferMode, \
				   redrawMode) \
	 (_eFuncs->BufferRestoreConfig)(ctx, OrigDisplayMode, 	\
					  OrigBufferMode, redrawMode) 

#define _dxf_GET_WINDOW_ORIGIN(ctx, x, y) \
	 (_eFuncs->GetWindowOrigin)(ctx, x, y) 

#define _dxf_GET_MAXSCREEN(ctx, w, h) \
	 (_eFuncs->GetMaxscreen)(ctx, w, h) 

#define _dxf_SET_WORLD_SCREEN(ctx, w, h) \
	 (_eFuncs->SetWorldScreen)(ctx, w, h) 

#define _dxf_DRAW_GLOBE(I, udata, matrix, draw) \
	 (_eFuncs->DrawGlobe)(I, udata, matrix, draw) 

#define _dxf_DRAW_GNOMON(I, udata, matrix, draw) \
	 (_eFuncs->DrawGnomon )(I, udata, matrix, draw)

#define _dxf_DRAW_ARROWHEAD(ctx, ax, ay) \
	 (_eFuncs->DrawArrowhead)(ctx, ax, ay)

#define _dxf_DRAW_ZOOMBOX(I, udata, matrix, draw) \
	 (_eFuncs->DrawZoombox)(I, udata, matrix, draw) 

#define _dxf_CREATE_CURSOR_PIXMAPS(I) \
	 (_eFuncs->CreateCursorPixmaps)(I) 

#define _dxf_ERASE_PREVIOUS_MARKS(I) \
	 (_eFuncs->ErasePreviousMarks)(I) 

#define _dxf_ERASE_CURSOR(I, x, y) \
	 (_eFuncs->EraseCursor)(I, x, y) 

#define _dxf_POINTER_INVISIBLE(ctx) \
	 (_eFuncs->PointerInvisible)(ctx) 

#define _dxf_POINTER_VISIBLE(ctx) \
	 (_eFuncs->PointerVisible)(ctx) 

#define _dxf_WARP_POINTER(ctx, x, y) \
	 (_eFuncs->WarpPointer)(ctx, x, y) 

#define _dxf_SET_LINE_ATTRIBUTES(ctx, color, style, width) \
         (_eFuncs->SetLineAttributes)(ctx, color, style, width)

/* obsolete: do not use in new code */
#define _dxf_SET_LINE_COLOR_WHITE(ctx) \
	 (_eFuncs->SetLineColorWhite)(ctx) 

/* obsolete: do not use in new code */
#define _dxf_SET_LINE_COLOR_GRAY(ctx) \
	 (_eFuncs->SetLineColorGray)(ctx) 

#define _dxf_DRAW_LINE(ctx, x1, y1, x2, y2) \
	 (_eFuncs->DrawLine)(ctx, x1, y1, x2, y2) 

#define _dxf_DRAW_MARKER(I) \
	 (_eFuncs->DrawMarker)(I) 

#define _dxf_DRAW_CURSOR_COORDS(I,text) \
	 (_eFuncs->DrawCursorCoords)(I,text)

#endif  /* STRUCTURES_ONLY */


#endif /* tdmInteractorEcho_h */
