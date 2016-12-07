/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


#include <dx/dx.h>
#include <stdlib.h>

static Object ScreenObject(Object, Vector, Vector, int, int, int);
  
Error
m_Screen(Object *in, Object *out)
{
  Point translation, size;
  Object ino;
  int depthflag, units, how;
  float scalarsize;
  
  /* in[0] is the object to be turned into a screen object */
  if (!in[0]) {
    DXSetError(ERROR_BAD_PARAMETER, "#10000", "object");
    return ERROR;
  }
  ino = in[0];
  
  /* in[1] is the position */
  if (in[1]) {
    if (!DXExtractParameter(in[1], TYPE_FLOAT, 3, 1, (Pointer)&translation)) {
      if (!DXExtractParameter(in[1], TYPE_FLOAT, 2, 1, (Pointer)&translation)) {
	DXSetError(ERROR_BAD_PARAMETER,"#10231", "position", 2, 3);
	goto error;
      }
    }
    translation.z = 0.0;
  }
    
  /* in[2] is units of screen object location */
  if (in[2]) {
    if (!DXExtractInteger(in[2], &how)) {
      DXSetError(ERROR_BAD_PARAMETER, "#10040", "how", 0, 2);
      goto error;
    }
    if ((how < 0)||(how > 2)) {
      DXSetError(ERROR_BAD_PARAMETER, "#10040", "how", 0, 2);
      goto error;
    }
    if (how == 0) 
      how = SCREEN_VIEWPORT;
    else if (how == 1) 
      how = SCREEN_PIXEL;
    else
      how = SCREEN_WORLD;
  }
  else {
    how = SCREEN_VIEWPORT;
  }


  if (!in[1]) {
    if (how == SCREEN_VIEWPORT) {
      translation.x = 0.5;
      translation.y = 0.5;
      translation.z = 0.0;
    }
    else {
      translation.x = 0.0;
      translation.y = 0.0;
      translation.z = 0.0;
    }
  } 

  /* now check the translation */
  if (how==SCREEN_VIEWPORT) {
     if ((translation.x < 0)||(translation.y < 0)||
         (translation.x > 1)||(translation.y > 1)) {
         DXSetError(ERROR_BAD_PARAMETER, "#10110", 
                    "for how = viewport, position", 0, 1);
         goto error;
     }
   }
   else if (how==SCREEN_PIXEL) {
     if ((translation.x < 0)||(translation.y < 0)) {
         DXSetError(ERROR_BAD_PARAMETER, "#10532", 
                    "for how = pixel, position");
         goto error;
     }
  }
    
  /* in[3] is the size */
  if (in[3]) {
    /* here's where you want to handle a single number also */
    if (DXExtractFloat(in[3], &scalarsize)) {
      size.x = scalarsize;
      /* if the user specifies only an x size, then the aspect ratio
       * of the object is preserved. size.y = -1 is a flag that this
       * is what's going on */
      size.y = -1;
      if (size.x < 0) {
         DXSetError(ERROR_BAD_PARAMETER,"#10531", "size");
         goto error;
      }
    }
    else { 
      if (!DXExtractParameter(in[3], TYPE_FLOAT, 3, 1, (Pointer)&size)) {
        if (!DXExtractParameter(in[3], TYPE_FLOAT, 2, 1, (Pointer)&size)) {
  	  DXSetError(ERROR_BAD_PARAMETER,"#10231", "size", 2, 3);
	  goto error;
        }
      }
      if ((size.x < 0)||(size.y<0.0)) {
         DXSetError(ERROR_BAD_PARAMETER,"#10531", "size");
         goto error;
      }
    }
    size.z = 0.0;
  }
  else {
    /* default */
    size.x = 100;
    size.y = -1;
    size.z = 1;
  }



  /* now check the translation */
  if (how == SCREEN_VIEWPORT) {
    if ((translation.x < 0)||(translation.x > 1)||(translation.y<0.0)||
        (translation.y>1.0)) {
      DXSetError(ERROR_BAD_PARAMETER, "#10110",
                 "for viewport relative, each position element", 0, 1);
      goto error;
    }
  }

  /* now the coordinates of the size parameter */
  if (in[4]) {
    if (!DXExtractInteger(in[4], &units)) {
       DXSetError(ERROR_NOT_IMPLEMENTED,"#10370","units", "1");
       goto error;
    }
    if (units != 1) {
       DXSetError(ERROR_NOT_IMPLEMENTED,"#10370","units", "1");
       goto error;
    }
  }
  else {
    units = 1;
  }
  
  
  /* in[5] is within, infrontof, behind */
  if (in[5]) {
    if (!DXExtractInteger(in[5], &depthflag)) {
      DXSetError(ERROR_BAD_PARAMETER, "#10040", "depth", -1, 1);
      goto error;
    }
    if ((depthflag != -1)&&(depthflag != 0)&&(depthflag != 1)) {
      DXSetError(ERROR_BAD_PARAMETER, "#10040", "depth", -1, 1);
      goto error;
    }
  }
  else {
    depthflag = 1;
  }

  if (!(out[0] = ScreenObject(ino, translation, size, how, units, depthflag)))
     goto error;
  return OK;

error:
  return ERROR;
}
  

static Object ScreenObject(Object ino, Vector translation, Vector size,
                          int how, int units, int depthflag)
{  
  Point box[8], min, max;
  Object o=NULL, oo=NULL, outscreen=NULL;
  Object oscreen=NULL, ooscreen=NULL, oooscreen=NULL;
  int i;
  float width, height, scalewidth, scaleheight, xtranslation, ytranslation;


  /* need the bounding box of the input object */
  if (!DXBoundingBox(ino,box)) {
    DXSetError(ERROR_DATA_INVALID,"#11020", "object");
    goto error;
  }
  min=box[0];
  max=box[0];
  for (i=0; i<7; i++) {
    min = DXMin(min,box[i]);
  }
  for (i=0; i<7; i++) {
    max = DXMax(max,box[i]);
  }
  width = max.x - min.x;
  height = max.y - min.y;
  
  /* get the thing into pixel units now */
  /* now nudge it a bit so that the reference point is at "position"
     relative to the object (just like caption, colorbar, etc) */
  if (how != SCREEN_VIEWPORT)
    o = (Object)DXNewXform(ino,
			 DXTranslate(DXPt(-min.x, -min.y,
				           0.0)));
  else  {
    if (width != 0) 
          xtranslation = width*(-translation.x - min.x/width);
    else
          xtranslation = 0.0;
    if (height != 0)
          ytranslation = height*(-translation.y - min.y/height);
    else
          ytranslation = 0.0;

    o = (Object)DXNewXform(ino,
			 DXTranslate(DXPt(xtranslation, ytranslation, 0.0)));
  }
  if (!o) goto error;
  
  
  
  /* scale the object to be unit size; therefore need to use the 
     width and height. Different behavior if the user specified only
     an x size. */ 

  if (size.y >= 0) {

    if (width > 0)
       scalewidth = size.x/width;
    else
       scalewidth = size.x;

    if (height > 0)
       scaleheight = size.y/height; 
    else
       scaleheight = size.y; 

    oo = (Object)DXNewXform((Object)o, DXScale(scalewidth, scaleheight, 1.0));
  }
  else {
    if (width > 0)
      scalewidth = size.x/width;
    else
      scalewidth = size.x;

    oo = (Object)DXNewXform((Object)o, DXScale(scalewidth, scalewidth, 1.0));
  }
  if (!o) goto error;
  
  
  /* now make it a screen object */ 
  if (!(outscreen =(Object)DXNewScreen((Object)oo, how, depthflag))) {
    goto error;
  }
  
  /* now we're ready to really translate it where it's supposed to go */ 
  ooscreen = (Object)DXNewXform((Object)outscreen, DXTranslate(translation));
  if (!ooscreen) goto error;
  
  /* make it immovable (only if not in world coordinates) */
  if (how != SCREEN_WORLD) {
     oooscreen = (Object)DXNewScreen(ooscreen, SCREEN_STATIONARY, 0);
     if (!oooscreen) goto error;
     return oooscreen;
  }
  else {
     return ooscreen;
  }
  
 error:
  DXDelete((Object)o);
  DXDelete((Object)oo);
  DXDelete((Object)oscreen);
  DXDelete((Object)ooscreen);
  DXDelete((Object)oooscreen);
  return NULL;
}

