/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


/*
 * $Header: /src/master/dx/src/exec/dxmods/caption.c,v 1.5 2002/03/21 02:57:33 rhh Exp $
 */


#include <string.h>
#include <dx/dx.h>
#include <math.h>

#define FLAG_VIEWPORT 0
#define FLAG_PIXEL    1
#define ABS(a)      ((a)>0.0 ? (a) : -(a))


static
Object
Caption(char **s, int n, Point p, int flag, Point ref, double align,
	double h, char *f, Vector x, Vector y)
{
    Matrix m;
    Vector z;
    Object a[100], b=NULL, c=NULL, o=NULL, oo=NULL, font;
    float ascent, descent, widths[100], width, height, lead=.133;
    Group g=NULL;
    int i;
    float xr, yr, xMin, xMax, yMin, yMax;
    Vector xlate;

    /* check vectors */
    if (DXLength(x)==0) {
	DXSetError(ERROR_BAD_PARAMETER, "#11822", "direction");
        return ERROR;
    }
    if (DXLength(y)==0) {
	DXSetError(ERROR_BAD_PARAMETER, "#11822","up");
        return ERROR;
    }

    /* for error recovery */
    for (i=0; i<n; i++)
	a[i] = NULL;

    /* interpret flag */
    if (flag==FLAG_VIEWPORT)
	flag = SCREEN_VIEWPORT;
    else if (flag==FLAG_PIXEL)
	flag = SCREEN_PIXEL;
    else {
	DXSetError(ERROR_BAD_PARAMETER, "#10070", "flag");
        return ERROR;
    }

    /* uses cache */
    if (!f)
	f = "variable";
    font = DXGetFont(f, &ascent, &descent);
    if (!font) {
        DXSetError(ERROR_DATA_INVALID,"cannot open %s font", f); 
	goto error;
    }
    /* group to hold caption(s) */
    g = DXNewGroup();
    if (!g) goto error;

    /* generate the caption(s), determine overall width,height */
    /* width is the total length of the caption */
    for (i=0, width=0; i<n; i++) {
	a[i] = DXGeometricText(s[i], font, &widths[i]);
	if (!a[i]) goto error;
	if (widths[i] > width)
	    width = widths[i];
    }

    /* the height is the total letter height */
    height = n*(ascent+descent) + (n-1)*lead;

    x = DXNormalize(x);
    y = DXNormalize(y);
    z = DXNormalize(DXCross(x, y));

    /* if (ABS(x.x) < ABS(x.y)) */
    /* 	 vertical=1;	        */
    /* else		        */
    /* 	 vertical=0;            */

    {
	float r = sqrt(x.x*x.x + x.y*x.y);
	xr = x.x / r;
	yr = x.y / r;
    }

#define MINMAX(a,b)		\
{ float xx = (a)*xr - (b)*yr;	\
  float yy = (a)*yr + (b)*xr;	\
  if (xx > xMax) xMax = xx;	\
  if (yy > yMax) yMax = yy;	\
  if (xx < xMin) xMin = xx;	\
  if (yy < yMin) yMin = yy;	\
}

    xMin = yMin = DXD_MAX_FLOAT;
    xMax = yMax = -DXD_MAX_FLOAT;
    MINMAX(0, -descent);
    MINMAX(width, -descent);
    MINMAX(width, height-descent);
    MINMAX(0, height-descent);

#undef MINMAX

    xlate.x = ((xMin + ref.x*(xMax-xMin))*h);
    xlate.y = ((yMin + ref.y*(yMax-yMin))*h);

    /* place the captions, put them in the group */
    for (i=0; i<n; i++) {
	float xx, yy;
	float X = (align*(width - widths[i]))*h;
	float Y = (((n-1)-i)*(ascent+descent+lead))*h;

	xx = (X*xr + Y*(-yr)) - xlate.x;
	yy = (X*yr + Y*xr) - xlate.y;

        /* compute the transform */
	m.A[0][0] = x.x*h;  m.A[0][1] = x.y*h;  m.A[0][2] = x.z*h;
	m.A[1][0] = y.x*h;  m.A[1][1] = y.y*h;  m.A[1][2] = y.z*h;
	m.A[2][0] = z.x*h;  m.A[2][1] = z.y*h;  m.A[2][2] = z.z*h;

        m.b[0] = xx;
        m.b[1] = yy;
        m.b[2] = 0; 

	/* create the object */
	b = (Object)DXNewXform(a[i], m);
	if (!b) goto error;
	a[i] = NULL;
	c = (Object)DXNewScreen(b, flag, 1);
	if (!c) goto error;
	b = NULL;
	if (!DXSetMember(g, NULL, c)) goto error;
	c = NULL;
    }

    /* overall position */
    o = (Object)DXNewXform((Object)g, DXTranslate(p));
    if (!o) goto error;
    g = NULL;

    /* make it immovable */
    oo = (Object)DXNewScreen(o, SCREEN_STATIONARY, 0);
    if (!oo) goto error;

    /* all ok */
    DXDelete(font);
    return oo;

error:
    for (i=0; i<n; i++)
	DXDelete(a[i]);
    DXDelete(b);
    DXDelete(c);
    DXDelete(o);
    DXDelete((Object)g);
    DXDelete(font);
    return NULL;
}

static Error
getvec(Object o, Vector *v)
{
    int rc = DXExtractParameter(o, TYPE_FLOAT, 3, 1, (Pointer)v)
	  || DXExtractParameter(o, TYPE_FLOAT, 2, 1, (Pointer)v);
    v->z = 0;
    return rc;
}

Error
m_Caption(Object *in, Object *out)
{
  Vector x, y;
  Point p, ref;
  Array positionobject=NULL;
  char *s[100], *f, tmpstring[1000], buf[1000], *s1[100], *newstring;
  float h, align, *pptr;
  int flag, n, vertical, horizontal, i, j, k, count=0;
  
  /* string(s) */
  if (!in[0]) {
    DXSetError(ERROR_BAD_PARAMETER, "#10000", "string");
    goto error;
  } else {
    for (n=0; n<100; n++)
      if (!DXExtractNthString(in[0], n, &s[n]))
	break;
    if (!n) {
      DXSetError(ERROR_BAD_PARAMETER, "#10200","string");
      goto error;
    }
  }	
  
  /* look for newlines in the string */
  count = 0;
  for (i=0; i<n; i++) {
    strcpy(tmpstring, s[i]);
    j=0;
    k=0;
    while (tmpstring[j] != '\0') {
      if (tmpstring[j]=='\n') {
	buf[k]='\0';
        newstring = (char *)DXAllocate(strlen(buf)+1);
        strcpy(newstring, buf);
	s1[count]=newstring; 
	k=0;
        count++;
      }
      else { 
	buf[k] = tmpstring[j];
	k++; 
      }
      j++;
    }
    buf[k]='\0';
    newstring = (char *)DXAllocate(strlen(buf)+1);
    strcpy(newstring, buf);
    s1[count]=newstring;
    count++;
  }

  /* position */
  p = DXPt(.5,.05,0);
  if (in[1] && !getvec(in[1], &p)) {
    DXSetError(ERROR_BAD_PARAMETER, "#10231","position", 2, 3);
    goto error;
  }
  
  /* flag */
  flag = FLAG_VIEWPORT;
  if (in[2] && !DXExtractInteger(in[2], &flag)) {
    /* flag must be either 0 or 1 */
    DXSetError(ERROR_BAD_PARAMETER, "#10070", "flag");
    goto error;
  }
  
  /* reference point */
  ref = (flag==FLAG_VIEWPORT? p : DXPt(0,0,0));
  if (in[3] && !getvec(in[3], &ref)) {
    DXSetError(ERROR_BAD_PARAMETER, "#10231","reference", 2, 3);
    goto error;
  }
  
  /* baseline */
  x = DXVec(1,0,0);
  if (in[7] && !getvec(in[7], &x)) {
    DXSetError(ERROR_BAD_PARAMETER, "#10231","direction", 2, 3);
    goto error;
  }
  
  
  /* should depend on whether the captions are vertical or 
     horizontal */
  x = DXNormalize(x);
  horizontal = 0;
  vertical = 0;
  if ((x.x == 0) && (x.y == 1))
    vertical=1;
  if ((x.y == 0) && (x.x == 1))
    horizontal=1;
  /* alignment for multi-line captions */
  /* if truly vertical, use ref.y, if truly horizontal use ref.x,
     else use 0 */
  align = (flag==FLAG_VIEWPORT? (vertical ? ref.y : 
				 (horizontal ? ref.x : 0)) : 0);
  if (in[4] && !DXExtractFloat(in[4], &align)) {
    /* alignment must be a scalar value */
    DXSetError(ERROR_BAD_PARAMETER, "#10080","alignment");
    goto error;
  }
  
  /* height */
  h = 15;
  if (in[5] && !DXExtractFloat(in[5], &h)) {
    DXSetError(ERROR_BAD_PARAMETER, "#10080", "height");
    goto error;
  }
  
  /* font */
  f = NULL;
  if (in[6] && !DXExtractString(in[6], &f)) {
    /* font must be a string */
    DXSetError(ERROR_BAD_PARAMETER, "#10200", "font");
    goto error;
  }
  
  
  /* up */
  y = DXVec(-x.y,x.x,0);
  if (in[8] && !getvec(in[8], &y)) {
    DXSetError(ERROR_BAD_PARAMETER, "#10231", "up", 2, 3);
    goto error;
  }
  
  /* do it */
  out[0] = Caption(s1, count, p, flag, ref, align, h, f, x, y);
  if (!out[0])
    goto error;
  /* apply the original string as an attribute */
  if (!DXSetAttribute(out[0], "Caption string", in[0]))
    goto error;
  /* apply the caption position as an attribute */
  positionobject = DXNewArray(TYPE_FLOAT, CATEGORY_REAL, 1, 2);
  if (!DXAddArrayData(positionobject, 0, 1, NULL))
    goto error;
  pptr = DXGetArrayData(positionobject);
  if (!pptr) goto error;
  pptr[0] = p.x;
  pptr[1] = p.y;
  if (!DXSetAttribute(out[0], "Caption position", (Object)positionobject))
    goto error;
  /* apply the caption flag as an attribute */
  if (!DXSetIntegerAttribute(out[0], "Caption flag", flag))
    goto error;
 
  for (i=0; i<count; i++) {
     DXFree((Pointer)s1[i]);
  }
  return OK;
error:
  for (i=0; i<count; i++) {
     DXFree((Pointer)s1[i]);
  }
  return ERROR;
}
