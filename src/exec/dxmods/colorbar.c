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

#if defined(HAVE_STRING_H)
#include <string.h>
#endif

#include "plot.h"
#include "color.h"
#include "autoaxes.h"

#define MINTICPIX 5

typedef struct
{
  float x;
  float y;
} Point2D;


static Error TransposeColors(Array);
static Error SimpleBar(Array *, Array *, RGBColor *);

static RGBColor DEFAULTTICCOLOR = {0.3, 0.3, 0.3};
static RGBColor DEFAULTLABELCOLOR = {1.0, 1.0, 1.0};

Error
m_ColorBar(Object *in, Object *out)
{
  Pointer axeshandle=NULL;
  Object outo, outscreen, o, oo, ooo;
  Object newo, ob;
  Array a_positions, a_newpositions=NULL; 
  Array a_newcolors=NULL, a_colors, a_newconnections=NULL;
  Array corners;
  char *nullstring="", *fontname, extralabel[80];
  int numpos, numcolors, i, flag, horizontal, intzero = 0, frame; 
  int tics, usedobject, adjust, curcount, lastin;
  int colorcount, lastok;
  float *pos_ptr, frac, minticsize, *actualcorners = NULL; 
  int numarrayitems;
  float minvalue, maxvalue;
  Point min, max; 
  float *col_ptr_f=NULL;
  int *col_ptr_i=NULL;
  Point box[8], translation, corner1, corner2, point; 
  float length, scalewidth, scaleheight, captionsize, labelscale=1.0;
  RGBColor color, *color_ptr, ticcolor, labelcolor;
  RGBColor framecolor;
  Class class;
  Type type, colortype;
  int rank, shapearray[32], axeslines=1;
  Category category;
  char *att, *label;
  Field colormap;
  int doaxes = 1;
  Array ticklocations=NULL;
  float *list_ptr;
  int numlist; 
  int dofixedfontsize=0;
  float fixedfontsize=0;
  int fixedfontsizepixels=10;
  

  
  typedef struct {
    float height, width, thickness;
  }  Shape; 
  Shape shape;

  strcpy(extralabel,nullstring);
  adjust = 0; 
  captionsize = 15.0; 
  outo = NULL;
  corners = NULL;
  o = NULL;
  oo = NULL;


  if (!in[0])
    DXErrorGoto(ERROR_BAD_PARAMETER, "missing colormap");


  
  class = DXGetObjectClass(in[0]); 
  if ((class != CLASS_FIELD)&&(class != CLASS_ARRAY)&&(class != CLASS_GROUP)) {
    DXSetError(ERROR_BAD_PARAMETER, "#10111","colormap");
    goto error;
  }
  
  if (class == CLASS_ARRAY) {
    if (!DXGetArrayInfo((Array)in[0], &numarrayitems, &type, &category, 
			&rank, shapearray)){
      goto error;
    }
    if (numarrayitems != 1) {
      DXSetError(ERROR_BAD_PARAMETER,"#10111","colormap");
      goto error;
    }
    if ((type!=TYPE_FLOAT)||(category!=CATEGORY_REAL)||(rank!=1)||
        (shapearray[0]!=3)) {
      DXSetError(ERROR_BAD_PARAMETER,"#10111","colormap");
      goto error;
    }
    DXWarning("colormap is a single color--no axes information available");
    colormap=(Field)in[0];
  }
  else {
    if (class==CLASS_FIELD && DXEmptyField((Field)in[0])) {
      return OK;
    }
    /* see if it's an imported colormap */
    if (!_dxfIsImportedColorMap(in[0], NULL, (Object *)&colormap, NULL)) {
      if (!(_dxfIsColorMap(in[0]))) {
        goto error;
      }
      colormap=(Field)in[0];
    }
  } 
  
  if (!in[3]) {
    horizontal = 0;
  }
  else {
    if (!(DXExtractInteger(in[3], &horizontal))) {
      DXSetError(ERROR_BAD_PARAMETER,"#10070","horizontal");
      goto error;
    }
    if ((horizontal != 0)&&(horizontal != 1)) {
      DXSetError(ERROR_BAD_PARAMETER,"#10070","horizontal");
      goto error;
    }
  }
  
  if (!in[1]) {
    if (horizontal==0) {
      translation.x = 0.95;
      translation.y = 0.95;
      translation.z = 0.0;
    }
    else {
      translation.x = 0.5;
      translation.y = 0.95;
      translation.z = 0.0;
    }
  }
  else {
    if (!DXExtractParameter(in[1], TYPE_FLOAT, 3, 1, (Pointer)&translation)) {
      if (!DXExtractParameter(in[1], TYPE_FLOAT, 2, 1, 
			      (Pointer)&translation)) {
	DXSetError(ERROR_BAD_PARAMETER,"#10231","position", 2, 3);
	goto error;
      }
    }
    translation.z = 0.0;
  }
  
  if ((translation.x < 0)||(translation.x > 1)||(translation.y<0.0)||
      (translation.y>1.0)) {
    DXSetError(ERROR_BAD_PARAMETER, "#10110", "position", 0, 1);      
    goto error;
  }
  
  if (!in[2]) {
    shape.height = 300.0;
    shape.width = 25.0;
    shape.thickness = 1.0;
  }
  else {
    if (!DXExtractParameter(in[2], TYPE_FLOAT, 3, 1, (Pointer)&shape)) {
      if (!DXExtractParameter(in[2], TYPE_FLOAT, 2, 1, (Pointer)&shape)) {
        DXSetError(ERROR_BAD_PARAMETER,"#10231","shape", 2, 3);
        goto error;
      }
      shape.thickness = 1;
    }
    
    if (shape.height <= 0.0) {
      DXSetError(ERROR_BAD_PARAMETER,"#10531","shape");
      goto error;
    }
    if (shape.width <= 0.0) {
      DXSetError(ERROR_BAD_PARAMETER,"#10531","shape");
      goto error;
    }
  }
  
  
  if (!in[4]) {
    /* magic formula to figure out how many ticks */
    tics = shape.height/(captionsize*2.0); 
  }
  else {
    if (!DXExtractInteger(in[4],&tics)) {
      DXSetError(ERROR_BAD_PARAMETER, "#10030","ticks");
      goto error;
    }
  }

  if (!_dxfGetColorBarAnnotationColors(in[8], in[9], DEFAULTTICCOLOR,
                          DEFAULTLABELCOLOR, &ticcolor, &labelcolor, 
                          &framecolor, &frame))
     goto error;

  if (frame==1) axeslines=1;
  else axeslines=0;

  if (in[10]) {
     if (!DXExtractFloat(in[10],&labelscale)) {
        DXSetError(ERROR_BAD_PARAMETER,"#10090","labelscale");
        goto error;
     }
     if (labelscale < 0) {
        DXSetError(ERROR_BAD_PARAMETER,"#10090","labelscale");
        goto error;
     }
  }

  if (in[11]) {
     if (!DXExtractString(in[11],&fontname)) {
        DXSetError(ERROR_BAD_PARAMETER,"#10200","font");
        goto error;
     }
   }
   else {
     fontname = "standard";
   }

  
  if (class == CLASS_ARRAY) {
    /* make a simple, simple, bar, all one color */
    color_ptr = (RGBColor *)DXGetArrayData((Array)colormap);
    if (!SimpleBar(&a_newpositions, &a_newcolors, color_ptr))
       goto error;  
    curcount = 4;
    minvalue = 0;
    maxvalue = 1;
    att = "positions";
    doaxes = 0;
  }
  /* a "normal" field color map */
  else {
    if (class == CLASS_GROUP) {
       DXSetError(ERROR_DATA_INVALID,
                  "colormap must be a single field; not a group"); 
       goto error;
    }
    else if (class != CLASS_FIELD) {
       DXSetError(ERROR_DATA_INVALID,
                  "colormap must be a single field"); 
       goto error;
    }
    usedobject = 0;
    if (in[5]) {
      if (!DXExtractFloat(in[5], &minvalue)) {
	if (!DXStatistics(in[5],"data",&minvalue, &maxvalue, NULL, NULL)) {
          DXSetError(ERROR_BAD_PARAMETER,
		     "min must be a field with data or scalar");
          goto error;
	}
	usedobject = 1;
      }
    }
    else {
      if (!DXStatistics((Object)colormap, "positions",&minvalue, NULL, NULL, NULL)) {
	DXAddMessage("invalid colormap");
	goto error;
      }
    }
    
    
    if (in[6]) {
      if (!DXExtractFloat(in[6], &maxvalue)) {
	if (!DXStatistics(in[6],"data",NULL, &maxvalue, NULL, NULL)) {
          DXSetError(ERROR_BAD_PARAMETER,
		     "max must be a field with data or scalar");
          goto error;
	}
      }
    }
    else {
      if (!usedobject) {
	if (!DXStatistics((Object)colormap, "positions",NULL, &maxvalue, NULL, NULL)) {
	  DXAddMessage("invalid colormap");
	  goto error;
	}
      }
    }
    
    if (minvalue == maxvalue) {
      color_ptr = 
          (RGBColor *)DXGetArrayData((Array)DXGetComponentValue(colormap,
                                                                 "data"));
      if (!SimpleBar(&a_newpositions, &a_newcolors, color_ptr))
         goto error;
      att = "positions";
      curcount = 4;
      sprintf(extralabel,"%f",minvalue); 
      minvalue = 0;
      maxvalue = 1;
      doaxes = 1; 
      tics = 0;
      goto drawbar;
    }


    if (minvalue > maxvalue) {
      DXSetError(ERROR_BAD_PARAMETER,"#11220","min","max");
      goto error;
    }
    
    if (!(a_positions = (Array)DXGetComponentValue((Field)colormap,
						   "positions"))) {
      DXSetError(ERROR_BAD_PARAMETER,"colormap has no positions");
      goto error;
    }
    if (!(a_colors = (Array)DXGetComponentValue((Field)colormap,"data"))) {
      DXSetError(ERROR_BAD_PARAMETER,"colormap has no data");
      goto error;
    }
    
    if (!DXGetArrayInfo(a_positions,&numpos, NULL, NULL, NULL, NULL)) {
      goto error;
    }
    if (!DXGetArrayInfo(a_colors,&numcolors, &colortype, NULL, NULL, NULL)) {
      goto error;
    }
    switch (colortype) {
      case (TYPE_FLOAT):
        if (!(col_ptr_f = (float *)DXGetArrayData(a_colors))) 
           goto error;
        break;
      case (TYPE_INT):
        if (!(col_ptr_i = (int *)DXGetArrayData(a_colors))) 
           goto error;
        break;
      default:
        DXSetError(ERROR_DATA_INVALID,
                   "colormap colors must be integer or float");
        goto error;
    }
    if (numcolors == 0) {
      DXSetError(ERROR_DATA_INVALID,"colormap has no colors (data component)");
      goto error;
    }
    
    if (!(pos_ptr = (float *)DXGetArrayData(a_positions))) {
      goto error;
    }
    
    if (!(a_newpositions = DXNewArray(TYPE_FLOAT,CATEGORY_REAL, 1, 2))) {
      goto error;
    }

    if (! DXSetStringAttribute((Object)a_newpositions, "dep", "positions"))
      goto error;
    
    if (!(a_newcolors = DXNewArray(TYPE_FLOAT,CATEGORY_REAL, 1, 3))) {
      goto error;
    }
    
    if (horizontal == 0) {
      corner1 = DXPt(0.0, minvalue, 0.0);
      corner2 = DXPt(1.0, maxvalue, 0.0);
    }
    else {
      corner1 = DXPt(minvalue, 0.0, 0.0);
      corner2 = DXPt(maxvalue, 1.0, 0.0);
    }
    if (in[5]||in[6]) {
      corners = DXNewArray(TYPE_FLOAT,CATEGORY_REAL,1,3);
      if (!(DXAddArrayData(corners, 0, 1, (Pointer)&corner1))) 
	goto error; 
      if (!(DXAddArrayData(corners, 1, 1, (Pointer)&corner2))) 
	goto error; 
    }
    if (!(att = DXGetString((String)DXGetComponentAttribute((Field)colormap,
							    "data","dep")))) {
      DXSetError(ERROR_DATA_INVALID,"missing data dependency attribute");
      goto error;
    }
    if (strcmp(att,"positions") && strcmp(att,"connections")) {
      DXSetError(ERROR_DATA_INVALID,"unknown data dependent attribute %s",
		 att);
      goto error;
    }
    
    
    curcount = 0;
    colorcount = 0;
    lastok = 0;
    if (!strcmp(att,"positions")) {
      if (corners) {	
        lastin = 0;
	if (horizontal == 0) { 
	  for (i=0; i<numpos; i++) {
	    if ((pos_ptr[i]>=corner1.y) &&  (pos_ptr[i]<=corner2.y)) { 
              if ((i > 0) && (lastin == 0)) {
		point = DXPt(0.0, corner1.y, 0.0);
		if (!(DXAddArrayData(a_newpositions, curcount, 1, 
				     (Pointer)&point))) {
		  goto error;
		}
		curcount++;
		point = DXPt(1.0, corner1.y, 0.0);
		if (!(DXAddArrayData(a_newpositions, curcount, 1, 
				     (Pointer)&point))) {
		  goto error;
		}
		curcount++;
		frac = 
		  (corner1.y - pos_ptr[i-1])/(pos_ptr[i]-pos_ptr[i-1]); 
                switch (colortype) {
                case (TYPE_FLOAT):
		color = DXRGB(frac*(col_ptr_f[3*i]-col_ptr_f[3*(i-1)]) 
			      + col_ptr_f[3*(i-1)], 
			      frac*(col_ptr_f[3*i+1]-col_ptr_f[3*(i-1)+1]) 
			      + col_ptr_f[3*(i-1)+1], 
			      frac*(col_ptr_f[3*i+2]-col_ptr_f[3*(i-1)+2])
			      + col_ptr_f[3*(i-1)+2]);
                break;
                case (TYPE_INT):
		color = DXRGB(frac*(col_ptr_i[3*i]-col_ptr_i[3*(i-1)]) 
			      + col_ptr_i[3*(i-1)], 
			      frac*(col_ptr_i[3*i+1]-col_ptr_i[3*(i-1)+1]) 
			      + col_ptr_i[3*(i-1)+1], 
			      frac*(col_ptr_i[3*i+2]-col_ptr_i[3*(i-1)+2])
			      + col_ptr_i[3*(i-1)+2]);
                break;
		default:
		  break;
                }
		if (!(DXAddArrayData(a_newcolors, colorcount, 
		 		     1, (Pointer)&color))) {
		  goto error;
		}
		colorcount++;
		if (!(DXAddArrayData(a_newcolors, colorcount, 
		 		     1, (Pointer)&color))) {
		  goto error;
		}
		colorcount++;
              }
	      point = DXPt(0.0, pos_ptr[i],0.0);
	      if (!(DXAddArrayData(a_newpositions, curcount, 1, 
				   (Pointer)&point))) {
		goto error;
	      }
	      curcount++;
              switch (colortype) {
              case (TYPE_FLOAT):
	      color = DXRGB(col_ptr_f[3*i], 
			    col_ptr_f[3*i+1], 
			    col_ptr_f[3*i+2]);
              break;
              case (TYPE_INT): 
	      color = DXRGB(col_ptr_i[3*i], 
			    col_ptr_i[3*i+1], 
			    col_ptr_i[3*i+2]);
              break;
	      default:
	        break;
              }

	      if (!(DXAddArrayData(a_newcolors, colorcount, 
				   1, (Pointer)&color))) {
		goto error;
	      }
	      colorcount++;
	      point = DXPt(1.0, pos_ptr[i],0.0);
	      if (!(DXAddArrayData(a_newpositions, curcount, 1, 
				   (Pointer)&point))) {
		goto error;
	      }
	      curcount++;
	      if (!(DXAddArrayData(a_newcolors, colorcount, 
				   1, (Pointer)&color))) {
		goto error;
	      }
	      colorcount++;
              lastin=1;
	    }
	    else {
              if (lastin == 1) {
		point = DXPt(0.0, corner2.y, 0.0);
		if (!(DXAddArrayData(a_newpositions, curcount, 1,
				     (Pointer)&point))) {
		  goto error;
		}
		curcount++;
		point = DXPt(1.0, corner2.y, 0.0);
		if (!(DXAddArrayData(a_newpositions, curcount, 1,
				     (Pointer)&point))) {
		  goto error;
		}
		curcount++;
		frac =
		  (corner2.y - pos_ptr[i-1])/(pos_ptr[i]-pos_ptr[i-1]);
                switch (colortype) {
                case (TYPE_FLOAT):
		color = DXRGB(frac*(col_ptr_f[3*i]-col_ptr_f[3*(i-1)])
			      + col_ptr_f[3*(i-1)],
			      frac*(col_ptr_f[3*i+1]-col_ptr_f[3*(i-1)+1])
			      + col_ptr_f[3*(i-1)+1],
			      frac*(col_ptr_f[3*i+2]-col_ptr_f[3*(i-1)+2])
			      + col_ptr_f[3*(i-1)+2]);
                break;
                case (TYPE_INT):
		color = DXRGB(frac*(col_ptr_i[3*i]-col_ptr_i[3*(i-1)])
			      + col_ptr_i[3*(i-1)],
			      frac*(col_ptr_i[3*i+1]-col_ptr_i[3*(i-1)+1])
			      + col_ptr_i[3*(i-1)+1],
			      frac*(col_ptr_i[3*i+2]-col_ptr_i[3*(i-1)+2])
			      + col_ptr_i[3*(i-1)+2]);
                break;
		default:
		  break;
                }
		if (!(DXAddArrayData(a_newcolors, colorcount,
				     1, (Pointer)&color))) {
		  goto error;
		}
		colorcount++;
		if (!(DXAddArrayData(a_newcolors, colorcount,
				     1, (Pointer)&color))) {
		  goto error;
		}
		colorcount++;
              }
	      lastin = 0;
	    }
	  }
	}
	else {
	  for (i=0; i<numpos; i++) {
	    if ((pos_ptr[i]>=corner1.x) &&  (pos_ptr[i]<=corner2.x)) { 
              if ((i > 0) && (lastin == 0)) {
		point = DXPt(corner1.x, 0.0, 0.0);
		if (!(DXAddArrayData(a_newpositions, curcount, 1, 
				     (Pointer)&point))) {
		  goto error;
		}
		curcount++;
		point = DXPt(corner1.x, 1.0, 0.0);
		if (!(DXAddArrayData(a_newpositions, curcount, 1, 
				     (Pointer)&point))) {
		  goto error;
		}
		curcount++;
		frac = 
		  (corner1.x - pos_ptr[i-1])/(pos_ptr[i]-pos_ptr[i-1]); 
                switch (colortype) {
                case (TYPE_FLOAT):
		color = DXRGB(frac*(col_ptr_f[3*i]-col_ptr_f[3*(i-1)]) 
			      + col_ptr_f[3*(i-1)], 
			      frac*(col_ptr_f[3*i+1]-col_ptr_f[3*(i-1)+1]) 
			      + col_ptr_f[3*(i-1)+1], 
			      frac*(col_ptr_f[3*i+2]-col_ptr_f[3*(i-1)+2])
			      + col_ptr_f[3*(i-1)+2]);
                break;
                case (TYPE_INT):
		color = DXRGB(frac*(col_ptr_i[3*i]-col_ptr_i[3*(i-1)]) 
			      + col_ptr_i[3*(i-1)], 
			      frac*(col_ptr_i[3*i+1]-col_ptr_i[3*(i-1)+1]) 
			      + col_ptr_i[3*(i-1)+1], 
			      frac*(col_ptr_i[3*i+2]-col_ptr_i[3*(i-1)+2])
			      + col_ptr_i[3*(i-1)+2]);
                break;
		default:
		  break;
                }
		if (!(DXAddArrayData(a_newcolors, colorcount, 
				     1, (Pointer)&color))) {
		  goto error;
		}
		colorcount++;
		if (!(DXAddArrayData(a_newcolors, colorcount, 
				     1, (Pointer)&color))) {
		  goto error;
		}
		colorcount++;
              }
	      point = DXPt(0.0, pos_ptr[i],0.0);
	      if (!(DXAddArrayData(a_newpositions, curcount, 1, 
				   (Pointer)&point))) {
		goto error;
	      }
	      point = DXPt(pos_ptr[i],0.0,0.0);
	      if (!(DXAddArrayData(a_newpositions, curcount, 1, 
				   (Pointer)&point))) {
		goto error;
	      }
	      curcount++;
              switch (colortype) {
              case (TYPE_FLOAT):
	      color = DXRGB(col_ptr_f[3*i], col_ptr_f[3*i+1], col_ptr_f[3*i+2]);
              break;
              case (TYPE_INT):
	      color = DXRGB(col_ptr_i[3*i], col_ptr_i[3*i+1], col_ptr_i[3*i+2]);
              break;
	      default:
	        break;
              }
	      if (!(DXAddArrayData(a_newcolors, colorcount, 
				   1, (Pointer)&color))) {
		goto error;
	      }
	      colorcount++;
	      point = DXPt(pos_ptr[i],1.0,0.0);
	      if (!(DXAddArrayData(a_newpositions, curcount, 1, 
				   (Pointer)&point))) {
		goto error;
	      }
	      curcount++;
	      if (!(DXAddArrayData(a_newcolors, colorcount, 
				   1, (Pointer)&color))) {
		goto error;
	      }
	      colorcount++;
              lastin=1;
	    }
            else {
              if (lastin == 1) {
                point = DXPt(corner2.x, 0.0, 0.0);
                if (!(DXAddArrayData(a_newpositions, curcount, 1,
				     (Pointer)&point))) {
                  goto error;
                }
                curcount++;
                point = DXPt(corner2.x, 1.0, 0.0);
                if (!(DXAddArrayData(a_newpositions, curcount, 1,
				     (Pointer)&point))) {
                  goto error;
                }
                curcount++;
                frac =
                  (corner2.x - pos_ptr[i-1])/(pos_ptr[i]-pos_ptr[i-1]);
                switch (colortype) {
                case (TYPE_FLOAT):
                color = DXRGB(frac*(col_ptr_f[3*i]-col_ptr_f[3*(i-1)])
			      + col_ptr_f[3*(i-1)],
			      frac*(col_ptr_f[3*i+1]-col_ptr_f[3*(i-1)+1])
			      + col_ptr_f[3*(i-1)+1],
			      frac*(col_ptr_f[3*i+2]-col_ptr_f[3*(i-1)+2])
			      + col_ptr_f[3*(i-1)+2]);
                break;
                case (TYPE_INT):
                color = DXRGB(frac*(col_ptr_i[3*i]-col_ptr_i[3*(i-1)])
			      + col_ptr_i[3*(i-1)],
			      frac*(col_ptr_i[3*i+1]-col_ptr_i[3*(i-1)+1])
			      + col_ptr_i[3*(i-1)+1],
			      frac*(col_ptr_i[3*i+2]-col_ptr_i[3*(i-1)+2])
			      + col_ptr_i[3*(i-1)+2]);
                  break;
		default:
		  break;
                }
                if (!(DXAddArrayData(a_newcolors, colorcount,
				     1, (Pointer)&color))) {
                  goto error;
                }
                colorcount++;
                if (!(DXAddArrayData(a_newcolors, colorcount,
				     1, (Pointer)&color))) {
                  goto error;
                }
                colorcount++;
              }
              lastin = 0;
            }
	  }
	}
      }
      else {
	if (horizontal == 0) { 
	  for (i=0; i<numpos; i++) {
	    point = DXPt(0.0, pos_ptr[i],0.0);
	    if (!(DXAddArrayData(a_newpositions, curcount, 
				 1, (Pointer)&point))) {
	      goto error;
	    }
	    curcount++;
            switch (colortype) {
            case (TYPE_FLOAT):
	      color = DXRGB(col_ptr_f[3*i], col_ptr_f[3*i+1], col_ptr_f[3*i+2]);
              break;
            case (TYPE_INT):
	      color = DXRGB(col_ptr_i[3*i], col_ptr_i[3*i+1], col_ptr_i[3*i+2]);
              break;
	    default:
	      break;
            }
	    if (!(DXAddArrayData(a_newcolors, colorcount, 1, 
				 (Pointer)&color))) {
	      goto error;
	    }
	    colorcount++;
	    point = DXPt(1.0, pos_ptr[i],0.0);
	    if (!(DXAddArrayData(a_newpositions, curcount, 
				 1, (Pointer)&point))) {
	      goto error;
	    }
	    curcount++;
	    if (!(DXAddArrayData(a_newcolors, colorcount, 1, 
				 (Pointer)&color))) {
	      goto error;
	    }
	    colorcount++;
	  }
	}
	else {
	  for (i=0; i<numpos; i++) {
	    point = DXPt(pos_ptr[i],0.0,0.0);
	    if (!(DXAddArrayData(a_newpositions, curcount, 
				 1, (Pointer)&point))) {
	      goto error;
	    }
	    curcount++;
            switch (colortype) {
            case (TYPE_FLOAT):
	      color = DXRGB(col_ptr_f[3*i], col_ptr_f[3*i+1], col_ptr_f[3*i+2]);
              break;
            case (TYPE_INT):
	      color = DXRGB(col_ptr_i[3*i], col_ptr_i[3*i+1], col_ptr_i[3*i+2]);
              break;
	    default:
	      break;
            }
	    if (!(DXAddArrayData(a_newcolors, colorcount, 1, 
				 (Pointer)&color))) {
	      goto error;
	    }
	    colorcount++;
	    point = DXPt(pos_ptr[i],1.0,0.0);
	    if (!(DXAddArrayData(a_newpositions, curcount, 
				 1, (Pointer)&point))) {
	      goto error;
	    }
	    curcount++;
	    if (!(DXAddArrayData(a_newcolors, colorcount, 1, 
				 (Pointer)&color))) {
	      goto error;
	    }
	    colorcount++;
	  }
	}
      }
    }
    /* else dep on connections */
    else {
      if (corners) {
        lastin=0;
	if (horizontal == 0) { 
	  
          /* add the minimum if given */
          point = DXPt(0.0, corner1.y,0.0);
          if (!(DXAddArrayData(a_newpositions, curcount, 1, 
			       (Pointer)&point))) 
	    goto error;
          curcount++;
	  point = DXPt(1.0, corner1.y,0.0);
	  if (!(DXAddArrayData(a_newpositions, curcount, 1, 
			       (Pointer)&point))) 
            goto error;
          curcount++;
          

	  for (i=0; i<numpos; i++) {
	    if ((pos_ptr[i]>=corner1.y) &&  (pos_ptr[i]<=corner2.y)) { 
	      point = DXPt(0.0, pos_ptr[i],0.0);
	      if (!(DXAddArrayData(a_newpositions, curcount, 1, 
				   (Pointer)&point))) {
		goto error;
	      }
	      curcount++;
              if ((!lastok)&&(i <= numcolors)) { /* deal with the first bin */
                if (i==0) /* make first bin black */
		  {
		    color = DXRGB(0.0, 0.0, 0.0);
		    if (!(DXAddArrayData(a_newcolors, colorcount, 1, 
					 (Pointer)&color))) 
		      goto error;
		    colorcount++;
		  }
                else 
		  {
		    switch (colortype) {
		    case (TYPE_FLOAT):
		      color = DXRGB(col_ptr_f[3*(i-1)], 
			            col_ptr_f[3*(i-1)+1], 
			            col_ptr_f[3*(i-1)+2]);
		      break;
		    case (TYPE_INT):
		      color = DXRGB(col_ptr_i[3*(i-1)], 
		            	    col_ptr_i[3*(i-1)+1], 
			            col_ptr_i[3*(i-1)+2]);
		      break;
		    default:
		      break;
		    }
		    if (!(DXAddArrayData(a_newcolors, colorcount, 1, 
					 (Pointer)&color))) 
		      goto error;
		    colorcount++;
		  }
              } 
	      if (lastok && (i <= numcolors)) { 
                switch (colortype) {
                case (TYPE_FLOAT):
		  color = DXRGB(col_ptr_f[3*(i-1)], 
				col_ptr_f[3*(i-1)+1], 
				col_ptr_f[3*(i-1)+2]);
		  break;
                case (TYPE_INT):
		  color = DXRGB(col_ptr_i[3*(i-1)], 
				col_ptr_i[3*(i-1)+1], 
				col_ptr_i[3*(i-1)+2]);
		  break;
		default:
		  break;
                }
		if (!(DXAddArrayData(a_newcolors, colorcount, 1, 
				     (Pointer)&color))) {
		  goto error;
		}
		colorcount++;
	      }
	      point = DXPt(1.0, pos_ptr[i],0.0);
	      if (!(DXAddArrayData(a_newpositions, curcount, 1, 
				   (Pointer)&point))) {
		goto error;
	      }
	      curcount++;
	      lastok=1;
	    }
	    if (pos_ptr[i]>corner2.y)
               break;
	  }
          /* add the maximum if given */
          point = DXPt(0.0, corner2.y,0.0);
          if (!(DXAddArrayData(a_newpositions, curcount, 1, 
			       (Pointer)&point))) 
	    goto error;
          curcount++;
	  point = DXPt(1.0, corner2.y,0.0);
	  if (!(DXAddArrayData(a_newpositions, curcount, 1, 
			       (Pointer)&point))) 
            goto error;
          curcount++;
          if (i>numcolors) /* make last bin black */
	    { 
	      color = DXRGB(0.0, 0.0, 0.0);
	      if (!(DXAddArrayData(a_newcolors, colorcount, 1, 
				   (Pointer)&color))) 
	        goto error;
	      colorcount++;
	    }
          else 
	    {
	      switch (colortype) {
              case (TYPE_FLOAT):
		color = DXRGB(col_ptr_f[3*(i-1)], 
			      col_ptr_f[3*(i-1)+1], 
			      col_ptr_f[3*(i-1)+2]);
                break;
              case (TYPE_INT):
		color = DXRGB(col_ptr_i[3*(i-1)], 
			      col_ptr_i[3*(i-1)+1], 
			      col_ptr_i[3*(i-1)+2]);
		break;
	      default:
	        break;
	      }
	      if (!(DXAddArrayData(a_newcolors, colorcount, 1, 
				   (Pointer)&color))) 
		goto error;
	      colorcount++;
	    }
	}
	/* else horizontal = 1 */
	else {
          /* add the minimum if given */
          point = DXPt(corner1.x,0.0, 0.0);
          if (!(DXAddArrayData(a_newpositions, curcount, 1, 
			       (Pointer)&point))) 
	    goto error;
          curcount++;
	  point = DXPt(corner1.x,1.0, 0.0);
	  if (!(DXAddArrayData(a_newpositions, curcount, 1, 
			       (Pointer)&point))) 
	    goto error;
          curcount++;
	  for (i=0; i<numpos; i++) {
	    if ((pos_ptr[i]>=corner1.x) &&  (pos_ptr[i]<=corner2.x)) { 
	      point = DXPt(pos_ptr[i],0.0,0.0);
	      if (!(DXAddArrayData(a_newpositions, curcount, 1, 
				   (Pointer)&point))) {
		goto error;
	      }
	      curcount++;
              if ((!lastok)&&(i <= numcolors)) { /* deal with the first bin */
                if (i==0) /* make first bin black */
		  {
		    color = DXRGB(0.0, 0.0, 0.0);
		    if (!(DXAddArrayData(a_newcolors, colorcount, 1, 
					 (Pointer)&color))) 
		      goto error;
		    colorcount++;
		  }
                else 
		  {
		    switch (colortype) {
		    case (TYPE_FLOAT):
		      color = DXRGB(col_ptr_f[3*(i-1)], 
			            col_ptr_f[3*(i-1)+1], 
			            col_ptr_f[3*(i-1)+2]);
		      break;
		    case (TYPE_INT):
		      color = DXRGB(col_ptr_i[3*(i-1)], 
		            	    col_ptr_i[3*(i-1)+1], 
			            col_ptr_i[3*(i-1)+2]);
		      break;
		    default:
		      break;
		    }
		    if (!(DXAddArrayData(a_newcolors, colorcount, 1, 
					 (Pointer)&color))) 
		      goto error;
		    colorcount++;
		  }
              } 
	      if (lastok&&(i<=numcolors)) {
                switch (colortype) {
                case (TYPE_FLOAT):
		  color = DXRGB(col_ptr_f[3*(i-1)], 
				col_ptr_f[3*(i-1)+1], 
				col_ptr_f[3*(i-1)+2]);
		  break;
                case (TYPE_INT):
		  color = DXRGB(col_ptr_i[3*(i-1)], 
				col_ptr_i[3*(i-1)+1], 
				col_ptr_i[3*(i-1)+2]);
		  break;
		default:
		  break;
                }
		if (!(DXAddArrayData(a_newcolors, colorcount, 1, 
				     (Pointer)&color))) {
		  goto error;
		}
		colorcount++;
	      }
	      point = DXPt(pos_ptr[i],1.0,0.0);
	      if (!(DXAddArrayData(a_newpositions, curcount, 1, 
				   (Pointer)&point))) {
		goto error;
	      }
	      curcount++;
	      lastok=1;
	    }
	    if (pos_ptr[i]>corner2.x)
               break;
	  }
          /* add the maximum if given */
          point = DXPt(corner2.x,0.0, 0.0);
          if (!(DXAddArrayData(a_newpositions, curcount, 1, 
			       (Pointer)&point))) 
	    goto error;
          curcount++;
	  point = DXPt(corner2.x, 1.0, 0.0);
	  if (!(DXAddArrayData(a_newpositions, curcount, 1, 
			       (Pointer)&point))) 
            goto error;
          curcount++;
          if (i>numcolors) /* make last bin black */
	    { 
	      color = DXRGB(0.0, 0.0, 0.0);
	      if (!(DXAddArrayData(a_newcolors, colorcount, 1, 
				   (Pointer)&color))) 
	        goto error;
	      colorcount++;
	    }
          else 
	    {
	      switch (colortype) {
              case (TYPE_FLOAT):
		color = DXRGB(col_ptr_f[3*(i-1)], 
			      col_ptr_f[3*(i-1)+1], 
			      col_ptr_f[3*(i-1)+2]);
                break;
              case (TYPE_INT):
		color = DXRGB(col_ptr_i[3*(i-1)], 
			      col_ptr_i[3*(i-1)+1], 
			      col_ptr_i[3*(i-1)+2]);
		break;
	      default:
	        break;
	      }
	      if (!(DXAddArrayData(a_newcolors, colorcount, 1, 
				   (Pointer)&color))) 
		goto error;
	      colorcount++;
	    }
	}
      }
      /* else no corners */
      else {
	if (horizontal == 0) { 
	  for (i=0; i<numpos; i++) {
	    point = DXPt(0.0, pos_ptr[i],0.0);
	    if (!(DXAddArrayData(a_newpositions, curcount, 
				 1, (Pointer)&point))) {
	      goto error;
	    }
	    curcount++;
	    if (lastok&&(i<=numcolors)) {
              switch (colortype) {
              case (TYPE_FLOAT):
	      color = DXRGB(col_ptr_f[3*(i-1)], 
			    col_ptr_f[3*(i-1)+1], 
			    col_ptr_f[3*(i-1)+2]);
              break;
              case (TYPE_INT):
	      color = DXRGB(col_ptr_i[3*(i-1)], 
			    col_ptr_i[3*(i-1)+1], 
			    col_ptr_i[3*(i-1)+2]);
              break;
	      default:
	      break;
              }
	      if (!(DXAddArrayData(a_newcolors, colorcount, 
				   1, (Pointer)&color))) {
		goto error;
	      }
	      colorcount++;
	    }
	    point = DXPt(1.0, pos_ptr[i],0.0);
	    if (!(DXAddArrayData(a_newpositions, curcount, 
				 1, (Pointer)&point))) {
	      goto error;
	    }
	    curcount++;
	    lastok=1;
	  }
	}
	else {
	  for (i=0; i<numpos; i++) {
	    point = DXPt(pos_ptr[i],0.0,0.0);
	    if (!(DXAddArrayData(a_newpositions, curcount, 
				 1, (Pointer)&point))) {
	      goto error;
	    }
	    curcount++;
	    if (lastok&&(i<=numcolors)) {
              switch (colortype) {
              case (TYPE_FLOAT):
	      color = DXRGB(col_ptr_f[3*(i-1)], 
			    col_ptr_f[3*(i-1)+1], 
			    col_ptr_f[3*(i-1)+2]);
              break;
              case (TYPE_INT):
	      color = DXRGB(col_ptr_i[3*(i-1)], 
			    col_ptr_i[3*(i-1)+1], 
			    col_ptr_i[3*(i-1)+2]);
              break;
	      default:
	        break;
              }
	      if (!(DXAddArrayData(a_newcolors, colorcount, 
				   1, (Pointer)&color))) {
		goto error;
	      }
	      colorcount++;
	    }
	    point = DXPt(pos_ptr[i],1.0,0.0);
	    if (!(DXAddArrayData(a_newpositions, curcount, 
				 1, (Pointer)&point))) {
	      goto error;
	    }
	    curcount++;
	    lastok=1;
	  }
	}
      }
    }
    }

drawbar:
    /* now, if it's vertical, need to reorder positions so that the quads 
       face the right way */
    if (!horizontal) {
       if (!DXGetArrayInfo(a_newpositions, &numpos, NULL,NULL,NULL,NULL))
         goto error;
       if (!strcmp(att,"positions")) {
         if (!_dxfTransposePositions(a_newpositions))
            goto error;
         if (!TransposeColors(a_newcolors))
            goto error;
       } 
       else {
         if (!_dxfTransposePositions(a_newpositions))
            goto error;
       }
  }
  outo = (Object)DXNewField();
  
  if (!(DXSetComponentValue((Field)outo,"positions", 
			    (Object)a_newpositions))) {
    goto error;
  }
  a_newpositions=NULL;
  if (!(DXSetComponentValue((Field)outo,"colors", (Object)a_newcolors))) {
    goto error;
  }
  a_newcolors=NULL;
  if (!(DXSetComponentAttribute((Field)outo,"colors","dep",
				(Object)DXNewString(att)))) {
    goto error;
  }
  
  if (curcount == 0) {
    DXSetError(ERROR_BAD_PARAMETER,"#11824");
    goto error;
  }
 
  if (horizontal) { 
    if (!(a_newconnections = DXMakeGridConnections(2, curcount/2, 2))) {
      goto error;
    }
  }
  else {
    if (!(a_newconnections = DXMakeGridConnections(2, 2, curcount/2))) {
      goto error;
    }
  }
  if (!(DXSetComponentValue((Field)outo,"connections",
			    (Object)a_newconnections))) {
    goto error;
  }
  a_newconnections=NULL;
  if (!(DXSetComponentAttribute((Field)outo, "connections", "ref", 
				(Object)DXNewString("positions")))) {
    goto error;
  }
  if (!(DXSetComponentAttribute((Field)outo, "connections", "element type", 
				(Object)DXNewString("quads")))) {
    goto error;
  }
  
  /* first get the map to start at 0.0 */ 
  

  /* now I know that the color map is monotonic */
  length = maxvalue-minvalue; 
  
  scaleheight = shape.height/length; 
  scalewidth = shape.width;
  /* figure out the minimum tic size */
  minticsize = MINTICPIX;

 

  /* check the label param */
  if (!in[7]) {
     label = nullstring;
  }
  else {
     if (!DXExtractString(in[7], &label)) {
        DXSetError(ERROR_DATA_INVALID,"#10200", "label");
        goto error;
     }
  }

  /* now do user-given tick locations and labels */
  if (in[12]) {
     if (!(DXGetObjectClass(in[12])==CLASS_ARRAY)) {
         DXSetError(ERROR_BAD_PARAMETER,"ticklocations must be a scalar list");
         return ERROR;
     }
     ticklocations = (Array)in[12];
   }
   if (in[13]) {
     if (!(DXGetObjectClass(in[13])==CLASS_ARRAY)) {
         DXSetError(ERROR_BAD_PARAMETER,"ticklabels must be a string list");
         return ERROR;
     }
     if (!DXGetArrayInfo((Array)in[13], &numlist, NULL,NULL,NULL,NULL))
        goto error;
     if (!in[12]) {
         /* need to make an array to use. It will go from 0 to n-1 */
         ticklocations = DXNewArray(TYPE_FLOAT, CATEGORY_REAL, 0);
         if (!ticklocations) goto error;
         list_ptr = DXAllocate(numlist*sizeof(float));
         if (!list_ptr) goto error;
         for (i=0; i<numlist; i++)
             list_ptr[i] = (float)i;
         if (!DXAddArrayData(ticklocations, 0, numlist, list_ptr))
             goto error;
         DXFree((Pointer)list_ptr);
      }
   }

   /* determine if the user has set a fixed label size */
   if (in[14]) {
     if (!DXExtractInteger(in[14], &dofixedfontsize)) {
        DXSetError(ERROR_DATA_INVALID,"usefixedfontsize must be 0 or 1");
        goto error;
     } 
     if ((dofixedfontsize != 0)&&(dofixedfontsize != 1)) {
        DXSetError(ERROR_DATA_INVALID,"usefixedfontsize must be 0 or 1");
        goto error;
     }
  }
  if (dofixedfontsize) {
     if (in[15]) {
        if (!DXExtractInteger(in[15], &fixedfontsizepixels)) {
           DXSetError(ERROR_DATA_INVALID,"fixedfontsize must be a positive integer");
           goto error;
        }
        if (fixedfontsize < 0) {
           DXSetError(ERROR_DATA_INVALID,"fixedfontsize must be a positive integer");
           goto error;
        }
     }
     if (horizontal) fixedfontsize = fixedfontsizepixels/shape.height;
     else fixedfontsize = fixedfontsizepixels/shape.width;

  } 

  /* get the thing into pixel units now */
  if (doaxes) {
    if (horizontal == 0) {
      axeshandle = _dxfNew2DAxesObject();
      if (!axeshandle)
	goto error;
      ob = (Object)DXNewXform((Object)outo, 
			      DXScale(scalewidth, scaleheight, 1.0));
  
      /* put a little negative fuzz on the color bar object */
      DXSetFloatAttribute((Object)ob, "fuzz", -2.0);
      _dxfSet2DAxesCharacteristic(axeshandle, "OBJECT", (Pointer)&ob);
      _dxfSet2DAxesCharacteristic(axeshandle, "XLABEL", (Pointer)extralabel);
      _dxfSet2DAxesCharacteristic(axeshandle, "YLABEL",(Pointer)label);
      _dxfSet2DAxesCharacteristic(axeshandle, "FONT",(Pointer)fontname);
      _dxfSet2DAxesCharacteristic(axeshandle, "LABELSCALE",
                                  (Pointer)&labelscale);
      _dxfSet2DAxesCharacteristic(axeshandle,"TICKS", (Pointer)&intzero);
      _dxfSet2DAxesCharacteristic(axeshandle,"TICKSX", (Pointer)&intzero);
      _dxfSet2DAxesCharacteristic(axeshandle,"TICKSY", (Pointer)&tics);
      if (corners)
      _dxfSet2DAxesCharacteristic(axeshandle, "CORNERS", (Pointer)&corners);
      _dxfSet2DAxesCharacteristic(axeshandle,"FRAME", (Pointer)&frame);
      _dxfSet2DAxesCharacteristic(axeshandle,"ADJUST", (Pointer)&adjust);
      _dxfSet2DAxesCharacteristic(axeshandle,"GRID", (Pointer)&intzero);
      _dxfSet2DAxesCharacteristic(axeshandle,"ISLOGX", (Pointer)&intzero);
      _dxfSet2DAxesCharacteristic(axeshandle,"ISLOGY", (Pointer)&intzero);
      _dxfSet2DAxesCharacteristic(axeshandle,"MINTICKSIZE", 
				  (Pointer)&minticsize);
      _dxfSet2DAxesCharacteristic(axeshandle,"AXESLINES",(Pointer)&axeslines);
      _dxfSet2DAxesCharacteristic(axeshandle,"JUSTRIGHT",(Pointer)&intzero);
      _dxfSet2DAxesCharacteristic(axeshandle,"RETURNCORNERS", 
				  (Pointer)actualcorners);
      _dxfSet2DAxesCharacteristic(axeshandle,"LABELCOLOR", 
				  (Pointer)&labelcolor);
      _dxfSet2DAxesCharacteristic(axeshandle,"TICKCOLOR", (Pointer)&ticcolor);
      _dxfSet2DAxesCharacteristic(axeshandle,"AXESCOLOR", (Pointer)&framecolor);
      _dxfSet2DAxesCharacteristic(axeshandle,"XLOCS", (Pointer)NULL);
      _dxfSet2DAxesCharacteristic(axeshandle,"YLOCS", (Pointer)ticklocations);
      _dxfSet2DAxesCharacteristic(axeshandle,"XLABELS", (Pointer)NULL);
      _dxfSet2DAxesCharacteristic(axeshandle,"YLABELS", (Pointer)in[13]);
      _dxfSet2DAxesCharacteristic(axeshandle,"DOFIXEDFONTSIZE", (Pointer)&dofixedfontsize);
      _dxfSet2DAxesCharacteristic(axeshandle,"FIXEDFONTSIZE", (Pointer)&fixedfontsize);
      
      if (DXGetError() != ERROR_NONE) goto error;
      
      o = (Object)_dxfAxes2D(axeshandle);
      if (!o) goto error;
      /* get the new bounding box */
      if (!DXBoundingBox(o,box)) goto error;
      min=box[0];
      max=box[0];
      for (i=0; i<7; i++) {
        min = DXMin(min,box[i]);
      }
      for (i=0; i<7; i++) {
        max = DXMax(max,box[i]);
      }
      shape.width = max.x - min.x;
      shape.height = max.y - min.y;
      newo = (Object)DXNewXform(o, DXTranslate(
			   DXPt(shape.width*(-translation.x
					     +
					     (min.x/(min.x-max.x))), 
				shape.height*(-translation.y + 
					      (min.y/(min.y-max.y))),
				0.0))); 
      if (!newo) goto error;
    }
    /* else horizontal */
    else {
      axeshandle = _dxfNew2DAxesObject();
      if (!axeshandle)
	goto error;
      ob = (Object)DXNewXform((Object)outo, 
			      DXScale(scaleheight, scalewidth, 1.0));
      /* put a little negative fuzz on the color bar object */
      DXSetFloatAttribute((Object)ob, "fuzz", -2);
      _dxfSet2DAxesCharacteristic(axeshandle, "OBJECT", (Pointer)&ob);
      _dxfSet2DAxesCharacteristic(axeshandle, "XLABEL",(Pointer)label);
      _dxfSet2DAxesCharacteristic(axeshandle, "YLABEL", (Pointer)extralabel);
      _dxfSet2DAxesCharacteristic(axeshandle, "FONT",(Pointer)fontname);
      _dxfSet2DAxesCharacteristic(axeshandle, "LABELSCALE", 
                                 (Pointer)&labelscale);
      _dxfSet2DAxesCharacteristic(axeshandle,"TICKS", (Pointer)&intzero);
      _dxfSet2DAxesCharacteristic(axeshandle,"TICKSX", (Pointer)&tics);
      _dxfSet2DAxesCharacteristic(axeshandle,"TICKSY", (Pointer)&intzero);
      if (corners)
      _dxfSet2DAxesCharacteristic(axeshandle, "CORNERS", (Pointer)&corners);
      _dxfSet2DAxesCharacteristic(axeshandle,"FRAME", (Pointer)&frame);
      _dxfSet2DAxesCharacteristic(axeshandle,"ADJUST", (Pointer)&adjust);
      _dxfSet2DAxesCharacteristic(axeshandle,"GRID", (Pointer)&intzero);
      _dxfSet2DAxesCharacteristic(axeshandle,"ISLOGX", (Pointer)&intzero);
      _dxfSet2DAxesCharacteristic(axeshandle,"ISLOGY", (Pointer)&intzero);
      _dxfSet2DAxesCharacteristic(axeshandle,"MINTICKSIZE", 
				  (Pointer)&minticsize);
      _dxfSet2DAxesCharacteristic(axeshandle,"AXESLINES",(Pointer)&axeslines);
      _dxfSet2DAxesCharacteristic(axeshandle,"JUSTRIGHT",(Pointer)&intzero);
      _dxfSet2DAxesCharacteristic(axeshandle,"RETURNCORNERS", 
				  (Pointer)actualcorners);
      _dxfSet2DAxesCharacteristic(axeshandle,"LABELCOLOR", 
				  (Pointer)&labelcolor);
      _dxfSet2DAxesCharacteristic(axeshandle,"TICKCOLOR", (Pointer)&ticcolor);
      _dxfSet2DAxesCharacteristic(axeshandle,"AXESCOLOR", (Pointer)&framecolor);
      _dxfSet2DAxesCharacteristic(axeshandle,"XLOCS", (Pointer)ticklocations);
      _dxfSet2DAxesCharacteristic(axeshandle,"YLOCS", (Pointer)NULL);
      _dxfSet2DAxesCharacteristic(axeshandle,"XLABELS", (Pointer)in[13]);
      _dxfSet2DAxesCharacteristic(axeshandle,"YLABELS", (Pointer)NULL);
      _dxfSet2DAxesCharacteristic(axeshandle,"DOFIXEDFONTSIZE", (Pointer)&dofixedfontsize);
      _dxfSet2DAxesCharacteristic(axeshandle,"FIXEDFONTSIZE", (Pointer)&fixedfontsize);
     
      /* &&& there are prob. corners issues here too */ 
      if (DXGetError() != ERROR_NONE) goto error;
      
      /* get the new bounding box */
      o = (Object)_dxfAxes2D(axeshandle);
      if (!o) goto error;
      if (!DXBoundingBox(o,box)) goto error;
      min=box[0];
      max=box[0];
      for (i=0; i<7; i++) {
        min = DXMin(min,box[i]);
      }
      for (i=0; i<7; i++) {
        max = DXMax(max,box[i]);
      }
      shape.width = max.y - min.y;
      shape.height = max.x - min.x;
      newo = (Object)DXNewXform(o,
				DXTranslate(DXPt(shape.height*(-translation.x +
                                      (min.x/(min.x-max.x))),
				       shape.width*(-translation.y +
						    (min.y/(min.y-max.y))),
						 0.0)));
      if (!newo) goto error;
    }
  }
  /* for non-field color maps, don't call autoaxes */
  else {
    if (horizontal == 0) {
      /* DXTranslate(_dxfAutoAxes(Scaled to pixel object)) */
      newo = (Object)DXNewXform( 
				(Object)DXNewXform((Object)outo, 
						   DXScale(scalewidth, 
							   scaleheight, 1.0)), 
				DXTranslate(DXPt(shape.width*(-translation.x), 
                                   shape.height*(-translation.y + 
					      (minvalue/(minvalue-maxvalue))),
						 0.0))); 
      if (!newo) goto error;
    }
    /* else horizontal */
    else {
      /* DXTranslate(_dxfAutoAxes(Scaled to pixel object)) */
      newo = (Object)DXNewXform( 
				(Object)DXNewXform((Object)outo, 
						   DXScale(scaleheight, 
							   scalewidth,  1.0)),
				DXTranslate(DXPt(shape.height*(-translation.x +
				      (minvalue/(minvalue-maxvalue))), 
						 -translation.y*shape.width, 
						 0.0)));
      if (!newo) goto error;
    }
  }
  
  flag = SCREEN_VIEWPORT;
  if (!(outscreen =(Object)DXNewScreen((Object)newo, flag, 1))) {
    goto error;
  }
  
  oo = (Object)DXNewXform((Object)outscreen, DXTranslate(translation));
  if (!oo) goto error;
  
  /* make it immovable */
  ooo = (Object)DXNewScreen(oo, SCREEN_STATIONARY, 0);
  if (!ooo) goto error;
  
  
  
  out[0]=ooo;
  DXDelete((Object)corners);
  DXDelete((Object)a_newpositions);
  DXDelete((Object)a_newconnections);
  DXDelete((Object)a_newcolors);
  _dxfFreeAxesHandle((Pointer)axeshandle);
  if (!in[12]) DXDelete((Object)ticklocations);
  return OK;
  
 error:
  if (!in[12]) DXDelete((Object)ticklocations);
  DXDelete((Object)oo);
  DXDelete((Object)corners);
  DXDelete((Object)a_newpositions);
  DXDelete((Object)a_newconnections);
  DXDelete((Object)a_newcolors);
  if (axeshandle) 
    _dxfFreeAxesHandle((Pointer)axeshandle);
  return ERROR;
}




Error _dxfGetColorBarAnnotationColors(Object colors, Object which,
                                     RGBColor defaultticcolor,
                                     RGBColor defaultlabelcolor,
                                     RGBColor *ticcolor, RGBColor *labelcolor,
                                     RGBColor *framecolor, int *frame)
{
  RGBColor *colorlist =NULL;
  int i, numcolors, numcolorobjects;
  char *colorstring, *newcolorstring;
  
  *frame = 0;
  
  
  /* in[9] is the colors list */
  /* first set up the default colors */
  *ticcolor = defaultticcolor;
  *labelcolor = defaultlabelcolor;
  if (colors) {	
    if (DXExtractNthString(colors,0,&colorstring)) {
      /* it's a list of strings */
      /* first figure out how many there are */
      if (!_dxfHowManyStrings(colors, &numcolors))
	goto error;
      /* allocate space for the 3-vectors */
      colorlist = DXAllocateLocal(numcolors*sizeof(RGBColor));
      if (!colorlist) goto error;
      for (i=0; i<numcolors;i++) {
        DXExtractNthString(colors,i,&colorstring);
        if (!strcmp(colorstring,"clear")) {
          colorlist[i]=DXRGB(-1.0, -1.0, -1.0);
        }
        else {
          if (!DXColorNameToRGB(colorstring, &colorlist[i]))
            goto error;
        }
      }
    }
    else {
      if (!DXQueryParameter(colors, TYPE_FLOAT, 3, &numcolors)) {
	DXSetError(ERROR_BAD_PARAMETER,
		   "colors must be a list of strings or a list of 3-vectors");
	goto error;
      }
      /* it's a list of 3-vectors */
      colorlist = DXAllocateLocal(numcolors*sizeof(RGBColor));
      if (!colorlist) goto error;
      if (!DXExtractParameter(colors, TYPE_FLOAT, 3, numcolors, 
			      (Pointer)colorlist)) {
        DXSetError(ERROR_DATA_INVALID,"bad color list");
	goto error;
      }
    }
  }
  if (!which) {
    if (colors) {
      /* all objects get whatever color was there */
      if (numcolors != 1) {
	DXSetError(ERROR_BAD_PARAMETER,
		   "more than one color specified for colors parameter; this requires a list of objects to color");
	goto error;
      }
      *ticcolor = colorlist[0];
      *labelcolor = colorlist[0];
    }
  }
  else {
    /* a list of strings was specified */
    /* figure out how many */
    if (!_dxfHowManyStrings(which, &numcolorobjects)) {
      DXSetError(ERROR_BAD_PARAMETER,"annotation must be a string list");
      goto error;
    }
    if (numcolors==1) {
      for (i=0; i< numcolorobjects; i++) {
	DXExtractNthString(which, i, &colorstring);
	_dxfLowerCase(colorstring, &newcolorstring);
	/* XXX lower case */
	if (!strcmp(newcolorstring, "ticks")) 
	  *ticcolor = colorlist[0];
	else if (!strcmp(newcolorstring, "labels")) 
	  *labelcolor = colorlist[0];
	else if (!strcmp(newcolorstring, "frame")) {
          if ((colorlist[0].r==-1)&&
              (colorlist[0].g==-1)&&
              (colorlist[0].b==-1)) {
              *frame=0;
          }
          else {
              *frame=1;
              *framecolor=colorlist[0];
          }
        }
	else if (!strcmp(newcolorstring,"all")) {
	  *ticcolor = colorlist[0];
	  *labelcolor = colorlist[0];
	}
	else {
	  DXSetError(ERROR_BAD_PARAMETER,
		     "annotation objects must be one of \"ticks\", \"labels\", or \"frame\" ");
	  goto error;
	}
	DXFree((Pointer)newcolorstring);
      }
    }
    else {
      if (numcolors != numcolorobjects) {
        DXSetError(ERROR_BAD_PARAMETER,"number of colors must match number of annotation objects if number of colors is greater than 1");
        goto error;
      }
      for (i=0; i< numcolorobjects; i++) {
	DXExtractNthString(which, i, &colorstring);
	_dxfLowerCase(colorstring, &newcolorstring);
	/* XXX lower case */
	if (!strcmp(newcolorstring, "ticks"))
	  *ticcolor = colorlist[i];
	else if (!strcmp(newcolorstring, "labels"))
	  *labelcolor = colorlist[i];
	else if (!strcmp(newcolorstring, "frame")) {
	  if ((colorlist[i].r==-1)&&
	      (colorlist[i].g==-1)&&
	      (colorlist[i].b==-1)) {
	    *frame = 0;
	  }
	  else {
	    *frame = 1; 
	    *framecolor = colorlist[i];
	  }
	}
	else {
	  DXSetError(ERROR_BAD_PARAMETER,
		     "annotation objects must be one of \"ticks\", \"labels\", or \"frame\" ");
	  goto error;
	}
	DXFree((Pointer)newcolorstring);
      }
    }
  }
  DXFree((Pointer)colorlist);
  return OK;
 error:
  DXFree((Pointer)colorlist);
  return ERROR;
}

Error _dxfTransposePositions(Array array)
{

  int i, num;
  Point2D *scratch=NULL, *oldarrayptr;


  if (!DXGetArrayInfo(array, &num, NULL, NULL, NULL, NULL))
     goto error;
  oldarrayptr = (Point2D *)DXGetArrayData(array);
  scratch = (Point2D *)DXAllocate(num*sizeof(Point2D));
  if (!scratch) goto error;
  for (i=0; i<num; i+=2) {
     scratch[i/2]=oldarrayptr[i]; 
     scratch[num/2 + i/2 ]= oldarrayptr[i+1];
  }
  if (!DXAddArrayData(array, 0, num, scratch))
     goto error;

  DXFree(scratch);
  return OK;
  

error:
  DXFree(scratch);
  return ERROR;

}


static Error TransposeColors(Array array)
{

  int i, num;
  Point *scratch=NULL, *oldarrayptr;


  if (!DXGetArrayInfo(array, &num, NULL, NULL, NULL, NULL))
     goto error;
  oldarrayptr = (Point *)DXGetArrayData(array);
  scratch = (Point *)DXAllocate(num*sizeof(Point));
  if (!scratch) goto error;
  for (i=0; i<num; i+=2) {
     scratch[i/2]=oldarrayptr[i]; 
     scratch[num/2 + i/2 ]= oldarrayptr[i+1];
  }
  if (!DXAddArrayData(array, 0, num, scratch))
     goto error;

  DXFree(scratch);
  return OK;
  

error:
  DXFree(scratch);
  return ERROR;

}



static Error SimpleBar(Array *pos, Array *col, RGBColor *color)
{
    Array a_newpositions, a_newcolors;
    float *pos_ptr;
    int i;

    if (!(a_newpositions = DXNewArray(TYPE_FLOAT,CATEGORY_REAL,1,2))) {
      goto error;
    }
    if (!(a_newcolors = DXNewArray(TYPE_FLOAT,CATEGORY_REAL,1,3))) {
      goto error;
    }
    if (!(DXAddArrayData(a_newpositions,0,4,NULL))) {
      goto error;
    }
    for (i=0; i<4; i++) {
      if (!(DXAddArrayData(a_newcolors,i,1,(Pointer)color))) {
        goto error;
      }
    }
    pos_ptr = (float *)DXGetArrayData(a_newpositions);
    /* bottom left */
    pos_ptr[0] = pos_ptr[1] = 0.0;  
    /* bottom right */
    pos_ptr[2] = 1.0;  
    pos_ptr[3] = 0.0;  
    /* top left */
    pos_ptr[4] = 0.0;  
    pos_ptr[5] = 1.0;  
    /* top right */ 
    pos_ptr[6] = 1.0;  
    pos_ptr[7] = 1.0;  

    *pos = a_newpositions;
    *col = a_newcolors;
    return OK;
error:
    return ERROR;
}
