/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
#define Screen dxScreen
#define Object dxObject
#include <dx/dx.h>
#undef Screen
#undef Object

#include <dxconfig.h>

#if !defined(DX_NATIVE_WINDOWS)
#include <GL/glx.h>
#endif
#include <GL/gl.h>

typedef struct {
	GLuint list;
} DXDisplayListOGL;

static int _dxd_glDisplayListOpen = 0;

int
_dxf_isDisplayListOpenOGL() { return _dxd_glDisplayListOpen; }

static Error
_delete_displayListObjectOGL(Pointer p)
{
	DXDisplayListOGL *dl = (DXDisplayListOGL *)p;
	glDeleteLists(dl->list, 1);
	DXFree(p);
	return OK;
}

void
_dxf_callDisplayListOGL(dxObject dlo)
{
	if (dlo)
	{
		DXDisplayListOGL *dl = (DXDisplayListOGL *)DXGetPrivateData((Private)dlo);
	    if (dl)
			glCallList(dl->list);
	}
}

dxObject
_dxf_openDisplayListOGL()
{
	dxObject dlo = NULL;
	DXDisplayListOGL *dl = (DXDisplayListOGL *)DXAllocate(sizeof(DXDisplayListOGL));
	if (! dl)
		goto error;

	dl->list = glGenLists(1);
	if (dl->list == 0)
		goto error;

	glNewList(dl->list, GL_COMPILE_AND_EXECUTE);
	_dxd_glDisplayListOpen = 1;

	dlo = (dxObject)DXNewPrivate((Pointer)dl, _delete_displayListObjectOGL);

	return dlo;

error:
	if (dlo)
		DXDelete(dlo);
	else if (dl)
		DXFree((Pointer)dl);
	return NULL;
}

void
_dxf_endDisplayListOGL()
{
	glEndList();
	_dxd_glDisplayListOpen = 0;
}
