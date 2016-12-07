/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 2002                                        */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>

#include <dx/dx.h>
#include <string.h>
#include <ctype.h>
#include "internals.h"
#include "render.h"
#include "displayutil.h"

/* Both in displayw and displayx */

static Error
traverse(Object o, struct arg *arg)
{
    int             i, inType;
    Object          oo;
    unsigned char  *src;
    Array           colorArray;
    Array           connections;
    Type            type;
    int	            rank, shape[32];
    int		    offsets[2];
    int		    pcounts[2];

    switch (DXGetObjectClass((Object)o)) {

	case CLASS_GROUP:

	    if (DXGetGroupClass((Group)o)!=CLASS_COMPOSITEFIELD)
		DXErrorReturn(ERROR_BAD_PARAMETER, "#11660");

	    for (i=0; (oo=DXGetEnumeratedMember((Group)o, i, NULL)); i++)
		if (!traverse(oo, arg))
		    return ERROR;

	    break;

	case CLASS_FIELD:

	    colorArray = (Array)DXGetComponentValue((Field)o, COLORS);
	    if (! colorArray)
	    {
		DXSetError(ERROR_INTERNAL, "no colors found in image");
		return ERROR;
	    }

	    DXGetArrayInfo(colorArray, NULL, &type, NULL, &rank, shape);

	    if (type == TYPE_UBYTE || type == TYPE_BYTE)
	    {
		if (rank == 0 || (rank == 1 && shape[0] == 1))
		{
		    inType = IN_BYTE_SCALAR;
		}
		else if (rank == 1 && shape[0] == 3)
		{
		    inType = IN_BYTE_VECTOR;
		}
		else
		{
		    DXSetError(ERROR_DATA_INVALID, 
			"byte images must be scalar or 1-vector or 3-vector");
		    return ERROR;
		}
	    }
	    else if (type == TYPE_FLOAT)
	    {
		if (rank != 1 || shape[0] != 3)
		{
		    DXSetError(ERROR_DATA_INVALID, 
			"float images must be 3-vector");
		    return ERROR;
		}

		inType = IN_FLOAT_VECTOR;
	    }
	    else
	    {
		DXSetError(ERROR_DATA_INVALID, 
			    "image fields must be type byte or type float");
		return ERROR;
	    }

	    src = (unsigned char *)DXGetArrayData(colorArray);

	    /*
	     * mesh offsets for position
	     */
	    connections = (Array)DXGetComponentValue((Field)o, CONNECTIONS);
	    DXGetMeshOffsets((MeshArray)connections, offsets);
	    DXQueryGridConnections(connections, NULL, pcounts);

	    return _dxf_translateImage(o,
			    arg->width, arg->height,
			    arg->ox,    arg->oy,
			    offsets[0], offsets[1], pcounts[0], pcounts[1],
			    arg->i, arg->n,
			    arg->translation, 
			    src, inType, arg->out);

	default: /* Shouldn't get here */
	    break;
    }

    return OK;
}

static Error
task(Pointer ptr)
{
    struct arg *arg = (struct arg *)ptr;
    return traverse(arg->image, arg);
}

Error
_dxf_dither(
    Object image,
    int width,
    int height,
    int ox,
    int oy,
    void *translation,
    unsigned char *out
) {
    struct arg arg;

    DXDebug("X", "dither after render");

    arg.image = image;
    arg.width = width;
    arg.height = height;
    arg.ox = ox;
    arg.oy = oy;
    arg.n = DXProcessors(0);
    arg.translation = translation;
    arg.out = out;

    DXCreateTaskGroup();
    for (arg.i=0; arg.i<arg.n; arg.i++)
	if (!DXAddTask(task, (Pointer)&arg, sizeof(arg), 1.0))
	    return ERROR;
    return DXExecuteTaskGroup();
}

/*
 * This function will accept only "X","X8","X12", "X15", "X16","X24","X32" 
 * (with optionally trailing white space)
 * all other strings will generate a warning, and cause a return 
 * value of 0
 */
int
_dxf_getXDepth(char *type)
{
  char	*tp = type;
  char	depth[4];
  int	i=0;

  /* check for "X" exact match, common case */
  if(!strcmp(type,"X")) return 0;

  /* Must have a leading "X" */
  if(*type++ != 'X') goto noMatch;

  /* make a copy of the digits immediately following X */
  for(i=0;i<3 && *type;) {
    if (!isdigit(*type)) break;
    depth[i++] = *type++;
  }

  /* Make sure the string has a trailing NULL */
  depth[i]='\0';	


  /* Make sure we have nothing but trailing white space, before the comma */
  while(*type) {
    if(*type == ',') break;
    if(!isspace(*type++)) goto noMatch;
  }


  switch (i = atoi(depth)) {
  case 0:
  case 4:
  case 8:
  case 12:
  case 15:
  case 16:
  case 24:
  case 32:
    return i;
    break;
  default:
    goto noMatch;
  }

 noMatch:
  DXSetError(ERROR_BAD_PARAMETER,"#11630",tp);
  return -1;
}

translationP
_dxf_GetXTranslation(Object o)
{
    translationT *d;

    if(o) {
      d = (translationT *) DXGetPrivateData((Private)o);
      return d;
    } else
      return NULL;
}


Error
DXParseWhere(char *where,
	int *depth, char **a0, char **a1, char **a2, char **a3, char **a4)
{
    char *s;
    char *arg0 = NULL,
	 *arg1 = NULL,
         *arg2 = NULL,
         *arg3 = NULL,
         *arg4 = NULL;
    char *copy = NULL;

    if (a0) *a0 = NULL;
    if (a1) *a1 = NULL;
    if (a2) *a2 = NULL;
    if (a3) *a3 = NULL;
    if (a4) *a4 = NULL;

    arg0 = copy = (char *)DXAllocate(strlen(where)+1);
    if (! copy)
	goto error;

    /*
     * parse where parameter
     */
    strcpy(copy, where);
    for (s=copy; *s; s++)
    {
	if (*s==',')
	{
	    if (!arg1) arg1 = s+1;
	    else if (!arg2) arg2 = s+1;
	    else if (!arg3) arg3 = s+1;
	    else if (!arg4) arg4 = s+1;
	    *s = '\0';
	}
    }

    if (a0 && arg0)
    {
	*a0 = (char *)DXAllocate(strlen(copy)+1);
	if (! *a0)
	    goto error;
	strncpy(*a0, arg0, strlen(arg0)+1);
    }

    if (a1)
    {
	if (! arg1)
	{
	    arg1 = getenv("DISPLAY");
	    if (! arg1)
	    {
#if defined(intelnt) || defined(WIN32)
		arg1 = "";
#else
		arg1 = "unix:0";
#endif
	    }
	}

	*a1 = (char *)DXAllocate(strlen(arg1)+1);
	if (! *a1)
	    goto error;
	strncpy(*a1, arg1, strlen(arg1)+1);
    }

    if (a2)
    {
	if (! arg2)
	    arg2 = "Image";

	*a2 = (char *)DXAllocate(strlen(arg2)+1);
	if (! *a2)
	    goto error;
	strncpy(*a2, arg2, strlen(arg2)+1);
    }

    if (a3)
    {
	*a3 = (char *)DXAllocate(strlen(arg3)+1);
	if (! *a3)
	    goto error;
	strncpy(*a3, arg3, strlen(arg3)+1);
    }

    if (a4)
    {
	*a4 = (char *)DXAllocate(strlen(arg4)+1);
	if (! *a4)
	    goto error;
	strncpy(*a4, arg4, strlen(arg4)+1);
    }

    if (depth)
	*depth = _dxf_getXDepth(arg0);

    DXFree(copy);
    return OK;

error:
    DXFree((Pointer)copy);
    if (a0) DXFree((Pointer)*a0);
    if (a1) DXFree((Pointer)*a1);
    if (a2) DXFree((Pointer)*a2);
    if (a3) DXFree((Pointer)*a3);
    if (a4) DXFree((Pointer)*a4);
    return ERROR;
}

Field
_dxfSaveSoftwareWindow(char *where)
{
    Field image = NULL;
    char *host, *title;
    int depth;

    if (! DXParseWhere(where, &depth, NULL, &host, &title, NULL, NULL))
	goto error;

    image = _dxf_CaptureSoftwareImage(depth, host, title);
    if (! image)
	goto error;
    
    return image;

error:

    DXDelete((Object)image);
    return NULL;
}


/************** INTELNT WINDOWS *******************************/

#if defined(DX_NATIVE_WINDOWS)

#include "displayw.h"

Object
DXDisplay(Object image, int depth, char *host, char *window)
{
    return DXDisplayWindows(image, depth, host, window);
}

unsigned char *
_dxf_GetPixels(Field i)
{
    return _dxf_GetWindowsPixels(i, NULL);
}

Field
_dxf_MakeImage(int w, int h, int d, char *where)
{
    return _dxf_MakeWindowsImage(w, h, d, where);
}

Field
_dxf_CaptureSoftwareImage(int d, char *h, char *w)
{
    return _dxf_CaptureWindowsSoftwareImage(d, h, w);
}

Error
_dxf_translateImage(Object image, /* object defining overall pixel grid          */
	int iwidth, int iheight,  /* composite width and height                  */
	int offx, int offy,	  /* origin of composite image in pixel grid     */
	int px, int py,		  /* explicit partition origin                   */
	int cx, int cy,		  /* explicit partition width and height         */
	int slab, int nslabs,	  /* implicit partitioning slab number and count */
	translationP translation, 
	unsigned char *src, int inType, unsigned char *dst)
{
    return _dxf_translateWindowsImage(image, iwidth, iheight, offx, offy, 
    			px, py, cx, cy, slab, nslabs, translation, src, inType, dst);
}

Error
DXSetSoftwareWindowActive(char *w, int a)
{
    return _dxf_SetSoftwareWindowsWindowActive(w, a);
}

Error
_dxf_CopyBufferToPImage(struct buffer *b, Field i, int xoff, int yoff) {
	return _dxf_CopyBufferToWindowsImage(b, i, xoff, yoff);
}

/************** X WINDOWS *******************************/
#else

#include "displayx.h"

Object
DXDisplay(Object image, int depth, char *host, char *window)
{
    return DXDisplayXAny(image, depth, host, window);
}

unsigned char *
_dxf_GetPixels(Field i)
{
    return _dxf_GetXPixels(i);
}

Field
_dxf_MakeImage(int w, int h, int d, char *where)
{
    return _dxf_MakeXImage(w, h, d, where);
}

Field
_dxf_CaptureSoftwareImage(int d, char *h, char *w)
{
    return _dxf_CaptureXSoftwareImage(d, h, w);
}

Error
_dxf_translateImage(Object image, /* object defining overall pixel grid          */
	int iwidth, int iheight,  /* composite width and height                  */
	int offx, int offy,	  /* origin of composite image in pixel grid     */
	int px, int py,		  /* explicit partition origin                   */
	int cx, int cy,		  /* explicit partition width and height         */
	int slab, int nslabs,	  /* implicit partitioning slab number and count */
	translationP translation, 
	unsigned char *src, int inType, unsigned char *dst)
{
    return _dxf_translateXImage(image, iwidth, iheight, offx, offy, 
    			px, py, cx, cy, slab, nslabs, translation, src, inType, dst);
}

Error
DXSetSoftwareWindowActive(char *w, int a)
{
	return _dxf_SetSoftwareXWindowActive(w, a);
}

Error
_dxf_CopyBufferToPImage(struct buffer *b, Field i, int xoff, int yoff) {
	return _dxf_CopyBufferToXImage(b, i, xoff, yoff);
}

#endif
