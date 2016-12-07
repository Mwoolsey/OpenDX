/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/*
 * $Header: /src/master/dx/src/exec/dxmods/text.c,v 1.7 2006/01/03 17:02:25 davidt Exp $
 */

#include <dxconfig.h>

#if defined(HAVE_STRING_H)
#include <string.h>
#endif

#include <dx/dx.h>
#include "_post.h"


typedef struct textarg
{
   Field fieldin;
   Group groupout;
   float height;
   char *font;
   Vector direction;
   Vector up;
   int direction_defaulted;
   int up_defaulted;
} Textarg;



static Error TextField(Pointer p)
{
  Textarg *textarg = (Textarg *)p;
  Field fieldin; 
  Group groupout;
  Object font=NULL, o, t;
  float height;
  char *f, *attr, *attr1, *string;
  Vector direction, up, z;
  float dotprod;
  Matrix m;
  Point position;
  int i, j=0, numitems, rank, shape[8], posdim, regcolors, posted=0;
  int trank, tshape[8]; 
  int brank, bshape[8]; 
  int nrank, nshape[8];
  int direction_defaulted, up_defaulted, delayed=0;
  int numtextpos, n, counts[8], groupcount=0;
  float posorigin[30], posdeltas[30];
  Type type, ttype, btype, ntype;
  Category category;
  float *pos_ptr=NULL;
  RGBColor *colors_ptr=NULL;
  RGBColor color;
  Array positions=NULL, data, invalid, colors, textcolors=NULL, textpositions;
  Array tangents=NULL, binormals=NULL, normals=NULL, colormap=NULL;
  InvalidComponentHandle invalidhandle=NULL;
  float *tangents_ptr=NULL, *binormals_ptr=NULL, *normals_ptr=NULL;
  ubyte *delayedcolors_ptr=NULL, delayedcolor;
  Point current_tangent, current_binormal, current_normal;
  RGBColor *colormap_ptr=NULL;
  fieldin = textarg->fieldin;
  groupout = textarg->groupout;
  height = textarg->height;
  f = textarg->font;
  direction = textarg->direction;
  up = textarg->up;
  direction_defaulted = textarg->direction_defaulted;
  up_defaulted = textarg->up_defaulted;

  /* uses cache */
  if (!f)
    f = "variable";
  font = DXGetFont(f, NULL, NULL);
  if (!font)
    goto error;



  data = (Array)DXGetComponentValue(fieldin,"data");
  if (!data) {
    DXSetError(ERROR_DATA_INVALID,"#10250", "field","data");
    goto error;
  }
  
  DXGetArrayInfo(data, &numitems, &type, &category, &rank, shape);
  if (type != TYPE_STRING) {
    DXSetError(ERROR_DATA_INVALID,"#10370", "data component", "type string");
    goto error;
  } 
  
  if (category != CATEGORY_REAL) {
    DXSetError(ERROR_DATA_INVALID,"data component must be category real");
    goto error;
  } 
  
  if (rank != 1) {
    DXSetError(ERROR_DATA_INVALID,"rank of data component must equal 1");
    goto error;
  }
  
  /*datadim = shape[0];*/
  
  attr = DXGetString((String)DXGetComponentAttribute((Field)fieldin, "data",
						     "dep"));
  if (!attr) {
    DXSetError(ERROR_MISSING_DATA,"#10241", "data");
    goto error;
  }
  if (strcmp(attr,"positions")&&(strcmp(attr,"connections"))) {
    DXSetError(ERROR_DATA_INVALID,
               "data component must be dep positions or dep connections");
    goto error;
  }
  
  
  /* see if there's an invalid positions array */
  if (!strcmp(attr,"positions")) {
    invalid = (Array)DXGetComponentValue(fieldin,"invalid positions");
    if (invalid) {
      invalidhandle = DXCreateInvalidComponentHandle((Object)fieldin, 
                                                      NULL, "positions");
      if (!invalidhandle)
        goto error;
    }
  }
  else {
    invalid = (Array)DXGetComponentValue(fieldin,"invalid connections");
    if (invalid) {
      invalidhandle = DXCreateInvalidComponentHandle((Object)fieldin, 
                                                      NULL,"connections");
      if (!invalidhandle)
        goto error;
    }
  } 
  
  /* see if there's a colors component */
  colors = (Array)DXGetComponentValue(fieldin,"colors");
  /* check for delayed */
  if (colors) {
    DXGetArrayInfo(colors, NULL, &type, NULL, &rank, shape);
    if ((type==TYPE_FLOAT)&&(rank==1)&&(shape[0]==3)) { 
       delayed = 0;
    }
    else {
      if (rank==0) {
         rank=1;
         shape[0]=1;
      }
      /* better be delayed */
      if ((type != TYPE_UBYTE)||(rank != 1) || (shape[0] != 1)) {
        DXSetError(ERROR_DATA_INVALID,"#11363", "byte colors", 1);
        goto error;
      }
      if (!(colormap = (Array)DXGetComponentValue(fieldin,"color map"))) {
        DXSetError(ERROR_MISSING_DATA,"#13050");
        goto error;
      }
      colormap_ptr = (RGBColor *)DXGetArrayData(colormap);
      delayed = 1;
    }
  }

  regcolors = 1;
  if (colors) {
    if (!delayed) {
      if (!(DXQueryConstantArray(colors, NULL, (Pointer)&color)))
         regcolors = 0;
      colors_ptr = (RGBColor *)DXGetArrayData(colors);
    }
    else
      if (!(DXQueryConstantArray(colors, NULL, (Pointer)&delayedcolor)))
         regcolors = 0;
      delayedcolors_ptr = (ubyte *)DXGetArrayData(colors);
  }
  
  
  
  posted = 0;
  if (!strcmp(attr,"connections")) {
    /* I need to check whether the positions are regular. If they
     * are, a simple grid shift is in order. Otherwise, use PostArray
     * to shift them */
    Array pos, con;
    pos = (Array)DXGetComponentValue((Field)fieldin,"positions");
    con = (Array)DXGetComponentValue((Field)fieldin,"connections");
    if ((DXQueryGridPositions(pos, &n, counts, posorigin, posdeltas)) &&
        DXQueryGridConnections(con, NULL, NULL)) {
      
      for (i=0; i<n; i++) {
        for (j=0; j<n; j++) {
          if (counts[j] > 1) {
            posorigin[i]+=0.5*posdeltas[i + j*n];
          }
        }
      }
      for (i=0; i<n; i++) {
        if (counts[i] >1) counts[i] = counts[i]-1;
      }
      
      positions = DXMakeGridPositionsV(n, counts, posorigin, posdeltas);
      
    }
    /* else the positions are irregular */
    else {
      positions = _dxfPostArray((Field)fieldin, "positions","connections");
    }
    if (!positions) {
      DXSetError(ERROR_BAD_PARAMETER,
                 "could not compute text positions for cell-centered data");
      goto error;
    }
    posted = 1;
  }
  else {
    /* get positions component */
    positions = (Array)DXGetComponentValue(fieldin,"positions");
    if (!positions) {
      DXSetError(ERROR_DATA_INVALID,"#10250", "field", "positions");
      goto error;
    }
  }
  DXGetArrayInfo(positions, &numitems, &type, &category, &rank, shape);
  if ((type != TYPE_FLOAT)||(category != CATEGORY_REAL)) {
    DXSetError(ERROR_DATA_INVALID,
	       "positions must be type float, category real");
    goto error;
  }
  
  if (rank != 1) {
    DXSetError(ERROR_DATA_INVALID,
	       "positions must be rank 1");
    goto error;
  }
  
  if ((shape[0]<1) || (shape[0] > 3)) {
    DXSetError(ERROR_DATA_INVALID, "#10370", "positions", 
               "1, 2, or 3-dimensional");
    goto error; 
  }
  posdim= shape[0];
  
  /* see if there's a tangent component (correponds to direction) */
  /* if the user specified direction in the config dialog box, we
     don't really care if there's a tangents component */
  tangents = (Array)DXGetComponentValue(fieldin,"tangents");
  if (tangents) {
    if (direction_defaulted) {
      tangents_ptr = (float *)DXGetArrayData(tangents);
      DXGetArrayInfo(tangents, &numitems, &ttype, NULL, &trank, tshape);
      if (ttype != TYPE_FLOAT) {
        DXSetError(ERROR_DATA_INVALID,"tangents component must be type float");
        goto error;
      }
      if (trank != 1) {
        DXSetError(ERROR_DATA_INVALID,"tangents component must be rank 1");
        goto error;
      }
      if (tshape[0] != 2 && tshape[0] != 3) {
        DXSetError(ERROR_DATA_INVALID,"tangents component must be 2D or 3D");
        goto error;
      }
      attr1 = DXGetString((String)DXGetComponentAttribute((Field)fieldin, 
                                                          "tangents", "dep"));
      if (!attr1) {
	DXSetError(ERROR_MISSING_DATA, "#10242", "tangents");
	goto error;
      }
      if (strcmp(attr,attr1)) {
	DXSetError(ERROR_DATA_INVALID, "#10360", "tangents", "data");
	goto error;
      }
    }
    else {
      DXWarning("tangents component is ignored because direction is set in the configuration dialog box");
      tangents=NULL;
    }
  }
  binormals = (Array)DXGetComponentValue(fieldin,"binormals");
  if (binormals) {
    if (up_defaulted) {
      binormals_ptr = (float *)DXGetArrayData(binormals);
      DXGetArrayInfo(binormals, &numitems, &btype, NULL, &brank, bshape);
      if (btype != TYPE_FLOAT) {
        DXSetError(ERROR_DATA_INVALID,
                   "binormals component must be type float");
        goto error;
      }
      if (brank != 1) {
        DXSetError(ERROR_DATA_INVALID,"binormals component must be rank 1");
        goto error;
      }
      if (bshape[0] != 2 && bshape[0] != 3) {
        DXSetError(ERROR_DATA_INVALID,"binormals component must be 2D or 3D");
        goto error;
      }

      attr1 = DXGetString((String)DXGetComponentAttribute((Field)fieldin, 
                                                          "binormals", "dep"));
      if (!attr1) {
	DXSetError(ERROR_MISSING_DATA, "#10241", "binormals");
	goto error;
      }
      if (strcmp(attr,attr1)) {
	DXSetError(ERROR_DATA_INVALID, "#10360", "binormals", "data");
	goto error;
      }
    }
    else {
      DXWarning("binormals component ignored because up is set in configuration dialog box");
      binormals=NULL;
    }
  }
  /* we only care about normals if tangents and binormals aren't both
     already specified */
  normals = (Array)DXGetComponentValue(fieldin,"normals");
  if (normals) {
    if ( (!binormals || up_defaulted ) || (!tangents || direction_defaulted)) {
      normals_ptr = (float *)DXGetArrayData(normals);
      DXGetArrayInfo(normals, &numitems, &ntype, NULL, &nrank, nshape);
      if (ntype != TYPE_FLOAT) {
        DXSetError(ERROR_DATA_INVALID,
                   "normals component must be type float");
        goto error;
      }
      if (nrank != 1) {
        DXSetError(ERROR_DATA_INVALID,"normals component must be rank 1");
        goto error;
      }
      if (nshape[0] != 2 && nshape[0] != 3) {
        DXSetError(ERROR_DATA_INVALID,"normals component must be 2D or 3D");
        goto error;
      }


      attr1 = DXGetString((String)DXGetComponentAttribute((Field)fieldin, 
                                                          "normals", "dep"));
      if (!attr1) {
	DXSetError(ERROR_MISSING_DATA, "#10241", "normals");
	goto error;
      }
      if (strcmp(attr,attr1)) {
	DXSetError(ERROR_DATA_INVALID, "#10360", "normals", "data");
	goto error;
      }
    }
    else {
      DXWarning("normals component is ignored because text direction is already sufficiently specified through direction (or tangents) and up (or binormals)");
      normals=NULL;
    }
  } 
  
  pos_ptr = (float *)DXGetArrayData(positions);
  /* the ones coming in from above */
  /* these should not be colinear, so I don't need to worry here */
  direction = DXNormalize(direction);
  up = DXNormalize(up);
  z = DXNormalize(DXCross(direction, up));
  
  for (i=0; i<numitems; i++) {
    if (!((invalid)&&(DXIsElementInvalid(invalidhandle, i)))) {
      switch (posdim) {
      case (1):
	position= DXPt(pos_ptr[i], 0.0, 0.0);
	break;
      case (2):
	position= DXPt(pos_ptr[2*i], pos_ptr[2*i+1], 0.0);
	break;
      case (3):
	position= DXPt(pos_ptr[3*i], pos_ptr[3*i+1], pos_ptr[3*i+2]);
	break;
      }
      
      /* override if components are present */
      if (tangents) {
        if (tshape[0]==3)
          current_tangent = DXPt(tangents_ptr[3*i], 
                                 tangents_ptr[3*i+1],
                                 tangents_ptr[3*i+2]);
        else
          current_tangent = DXPt(tangents_ptr[2*i], 
                                 tangents_ptr[2*i+1],
                                 0.0);
	direction=DXNormalize(current_tangent);
      }
      if (binormals) {
        if (bshape[0]==3)
          current_binormal = DXPt(binormals_ptr[3*i], 
                                  binormals_ptr[3*i+1],
                                  binormals_ptr[3*i+2]);
        else
          current_binormal = DXPt(binormals_ptr[2*i], 
                                  binormals_ptr[2*i+1],
                                  0.0);
	up = DXNormalize(current_binormal);
      }

      /* check for colinearity */
      dotprod = (direction.x*up.x + direction.y*up.y + direction.z*up.z);
      if (dotprod==1 || dotprod==-1) {
        DXWarning("direction and up are colinear, resetting up");
        if (direction.x==0)
          up = DXVec(0, -direction.z, direction.y);
        else
          up = DXVec(-direction.y, direction.x, 0);
      }

      /* reset z from it's default */
      if (tangents || binormals) {
	z = DXNormalize(DXCross(direction, up));
      }
      /* reset z if normals has been set */
      if (normals) {
        if (nshape[0]==3)
          current_normal = DXPt(normals_ptr[3*i], 
                                normals_ptr[3*i+1],
                                normals_ptr[3*i+2]);
        else
          current_normal = DXPt(normals_ptr[2*i], 
                                normals_ptr[2*i+1],
                                0.0);
	z = DXNormalize(current_normal);
      }
      
      /* compute the transform */
      
      m.A[0][0] = direction.x*height;
      m.A[0][1] = direction.y*height;
      m.A[0][2] = direction.z*height;
      m.A[1][0] = up.x*height;
      m.A[1][1] = up.y*height;
      m.A[1][2] = up.z*height;
      m.A[2][0] = z.x*height; 
      m.A[2][1] = z.y*height;
      m.A[2][2] = z.z*height;
      m.b[0] = position.x;
      m.b[1] = position.y;         
      m.b[2] = position.z;
      
      /* get the string */
      if (!DXExtractNthString((Object)data,i,&string)) {
	DXSetError(ERROR_INTERNAL,"error extracting %dth string from data",
		   i);
	goto error;
      }
      /* create the object */
      o = DXGeometricText(string, font, NULL);
      if (!o)
        goto error;
      /* if there was a colors component, use it */
      if (colors) {
	textpositions = (Array)DXGetComponentValue((Field)o,"positions");
	DXGetArrayInfo(textpositions, &numtextpos, NULL, NULL, NULL, NULL);
        if (!delayed) {
	if (!regcolors)
          textcolors = (Array)DXNewConstantArray(numtextpos, 
                                                 (Pointer)&colors_ptr[i],
                                                 TYPE_FLOAT, CATEGORY_REAL, 
                                                 1, 3);
	else
          textcolors = (Array)DXNewConstantArray(numtextpos, (Pointer)&color,
						 TYPE_FLOAT, 
						 CATEGORY_REAL, 1, 3);
        }
        /* else delayed */
        else {
	if (!regcolors)
          textcolors = (Array)DXNewConstantArray(numtextpos, 
                                (Pointer)&colormap_ptr[delayedcolors_ptr[i]],
                                TYPE_FLOAT, CATEGORY_REAL, 
                                1, 3);
	else {
          color = colormap_ptr[(int)delayedcolor];
          textcolors = (Array)DXNewConstantArray(numtextpos, 
                                                (Pointer)&color,
						 TYPE_FLOAT, 
						 CATEGORY_REAL, 1, 3);
         }
        }
	if (!DXSetComponentValue((Field)o, "colors", (Object)textcolors))
          goto error;
	textcolors = NULL;
      }
      t = (Object)DXNewXform(o, m);
      if (!t) {
        DXDelete(o);
        goto error;
      }
      if (!DXSetFloatAttribute(t, "fuzz", 1.0))
        goto error;
      
      /* Geometric text is a field with positions, connections, and colors */
      if (!DXSetEnumeratedMember(groupout, groupcount++, t))
	goto error; 
    }
  }
  
  if (posted) DXDelete((Object)positions);
  DXDelete((Object)textcolors);
  DXDelete((Object)font);
  DXFreeInvalidComponentHandle(invalidhandle);
  return OK;
 error:
  if (posted) DXDelete((Object)positions);
  DXDelete((Object)textcolors);
  DXDelete((Object)font);
  DXFreeInvalidComponentHandle(invalidhandle);
  return ERROR;
  
}


static Error TextObject(Object in, Group out, 
                        float h, char *f, Vector x, Vector y,
                        int direction_defaulted, int up_defaulted)
{
  struct textarg textarg;
  int i;
  Object childin;
  Group group=NULL;
  
  
  switch (DXGetObjectClass(in)) {
  case (CLASS_FIELD):
    group = DXNewGroup();
    DXSetMember((Group)out,NULL,(Object)group);
    textarg.fieldin = (Field)in;
    textarg.groupout = group;
    textarg.height = h;
    textarg.font = f;
    textarg.direction = x;
    textarg.up = y;
    textarg.direction_defaulted = direction_defaulted;
    textarg.up_defaulted = up_defaulted;
    if (!DXAddTask(TextField, (Pointer)&textarg, sizeof(textarg), 1.0))
      goto error;
    break;
  case (CLASS_GROUP):
    switch (DXGetGroupClass((Group)in)) {
      case (CLASS_GROUP):
         group = DXNewGroup();
         DXSetMember((Group)out,NULL,(Object)group);
         break;
      case (CLASS_SERIES):
         group = (Group)DXNewSeries();
         DXSetMember((Group)out,NULL,(Object)group);
         break;
      case (CLASS_COMPOSITEFIELD):
         group = (Group)DXNewCompositeField();
         DXSetMember((Group)out,NULL,(Object)group);
         break;
      case (CLASS_MULTIGRID):
         group = (Group)DXNewMultiGrid();
         DXSetMember((Group)out,NULL,(Object)group);
         break;
      default:
         DXSetError(ERROR_DATA_INVALID,"unknown group class");
         goto error;
    }
    for (i=0; (childin=DXGetEnumeratedMember((Group)in, i, NULL)); i++) {
      if (!TextObject(childin, group, h, f, x, y, direction_defaulted,
                      up_defaulted))
	goto error;
    }
    
    break;
  default:
    DXSetError(ERROR_DATA_INVALID,"#10112", "string");
    goto error;
  }

  return OK;
    
  error:
    DXDelete((Object)group);
    return ERROR;
  
}


static
  Object
  Text(char *s, Point p, float h, char *f, Vector x, Vector y)
{
  Matrix m;
  Vector z;
  Object o, t, font;
  
  /* check vectors */
  if (DXLength(x)==0) {
    DXSetError(ERROR_BAD_PARAMETER, "#11822", "direction");
    return ERROR;
  }
  if (DXLength(y)==0) {
    DXSetError(ERROR_BAD_PARAMETER, "#11822", "up");
    return ERROR;
  }
  
  /* uses cache */
  if (!f)
    f = "variable";
  font = DXGetFont(f, NULL, NULL);
  if (!font)
    goto error;
  
  /* compute the transform */
  x = DXNormalize(x);
  y = DXNormalize(y);
  z = DXNormalize(DXCross(x, y));
  
  m.A[0][0] = x.x*h;    m.A[0][1] = x.y*h;    m.A[0][2] = x.z*h;
  m.A[1][0] = y.x*h;    m.A[1][1] = y.y*h;    m.A[1][2] = y.z*h;
  m.A[2][0] = z.x*h;    m.A[2][1] = z.y*h;    m.A[2][2] = z.z*h;
  m.b[0] = p.x;         m.b[1] = p.y;         m.b[2] = p.z;
  
  /* create the object */
  o = DXGeometricText(s, font, NULL);
  if (!o)
    goto error;
  t = (Object)DXNewXform(o, m);
  if (!t) {
    DXDelete(o);
    goto error;
  }
  if (!DXSetFloatAttribute(t, "fuzz", 1.0))
    goto error;
  
  /* all ok */
  DXDelete(font);
  return t;
  
 error:
  DXDelete(font);
  return NULL;
}

Error
  m_Text(Object *in, Object *out)
{
  struct textarg textarg;
  Vector x, y;
  Point p;
  char *s, *f;
  float h;
  int textfield=0, direction_defaulted, up_defaulted;
  Group outobject=NULL;
  
  /* string */
  if (!in[0]) {
    DXSetError(ERROR_BAD_PARAMETER, "#10000", "string");
    return ERROR;
  } else
    if (!DXExtractString(in[0], &s)) textfield=1;
  
  
  /* origin */
  p = DXPt(0,0,0);
  if ((textfield)&&(in[1])) 
    DXWarning("position is ignored if string parameter is a field");
  
  if (in[1] && !DXExtractParameter(in[1], TYPE_FLOAT, 3, 1, (Pointer)&p)) {
    DXSetError(ERROR_BAD_PARAMETER, "#10230", "position", 3);
    return ERROR;
  }
 
  /* height */
  h = 1;
  if (in[2] && !DXExtractFloat(in[2], &h)) {
    DXSetError(ERROR_BAD_PARAMETER, "#10080", "height");
    return ERROR;
  }
  
  /* font */
  f = NULL;
  if (in[3] && !DXExtractString(in[3], &f)) {
    DXSetError(ERROR_BAD_PARAMETER, "#10200", "font");
    return ERROR;
  }
 
  direction_defaulted=0; 
  /* baseline */
  x = DXVec(1,0,0);
  if (!in[4]) direction_defaulted=1;
  if (in[4] && !DXExtractParameter(in[4], TYPE_FLOAT, 3, 1, (Pointer)&x)) {
    DXSetError(ERROR_BAD_PARAMETER, "#10230", "direction", 3);
    return ERROR;
  }
  
  /* up */
  if (x.x==0)
    y = DXVec(0, -x.z, x.y);
  else
    y = DXVec(-x.y, x.x, 0);

  up_defaulted=0;
  if (!in[5]) up_defaulted=1;
  if (in[5] && !DXExtractParameter(in[5], TYPE_FLOAT, 3, 1, (Pointer)&y)) {
    DXSetError(ERROR_BAD_PARAMETER, "#10230", "up", 3);
    return ERROR;
  }
  
  /* do it */
  if (!textfield)
    out[0] = Text(s, p, h, f, x, y);
  else {
    DXCreateTaskGroup();
    /* make a new holder of the same type as the old */
    switch (DXGetObjectClass(in[0])) {
      case (CLASS_GROUP):
        switch (DXGetGroupClass((Group)in[0])) {
          case (CLASS_GROUP):
             outobject = DXNewGroup();
             break;
          case (CLASS_SERIES):
             outobject = (Group)DXNewSeries();
             break;
          case (CLASS_COMPOSITEFIELD):
             outobject = (Group)DXNewCompositeField();
             break;
          case (CLASS_MULTIGRID):
             outobject = (Group)DXNewMultiGrid();
             break;
          default:
             DXSetError(ERROR_DATA_INVALID,"unknown group class");
             goto error;
        }
        if (!TextObject(in[0], outobject, h, f, x, y, direction_defaulted,
                        up_defaulted))
          goto error;
        break;
      case (CLASS_FIELD):
        outobject = DXNewGroup();
        textarg.fieldin = (Field)in[0];
        textarg.groupout = outobject;
        textarg.height = h;
        textarg.font = f;
        textarg.direction = x;
        textarg.up = y;
        textarg.direction_defaulted=direction_defaulted;
        textarg.up_defaulted=up_defaulted;
        if (!TextField((Pointer)&textarg))
           goto error;
        break;
     default:
        DXSetError(ERROR_BAD_PARAMETER,"#10112", "string");
        goto error; 
    }
    if (!DXExecuteTaskGroup())
      goto error;
    out[0] = (Object)outobject;
  }
  
  if (!out[0])
    return ERROR;
  
  return OK;
 error:
  out[0]=NULL;
  DXDelete((Object)outobject);
  return ERROR;
}
