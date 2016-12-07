#include <dxconfig.h>
#include <math.h>
#include <string.h>
#include "lightClass.h"
#include "render.h"
#include "internals.h"
#include "../dxmods/_newtri.h"
static Error work(struct xfield *, struct shade *, Array, Array *, Array *);
struct shade {
    Object o;
    Group g;
    Field f;
    Camera c;
    Matrix m;
    char xflag;
    char xonly;
    char notight;
    char nocolors;
    struct docount {
 int docount;
 struct docount *next;
    } *docount, **docountlist;
    struct count *count;
    short i;
    struct l {
 int nl;
 RGBColor ambient;
 int na;
 Light lights[100];
    } *l;
    struct parms {
 float ambient;
 float diffuse;
 float specular;
 int shininess;
    } front, back;
    float fuzz;
    float ff;
    enum approx approx;
    Private lightList;
};
static struct parms default_parms = {
    1,
    .7,
    .5,
    10,
};
Error
_dxf_approx(Object o, enum approx *approx)
{
    Object oo;
    char *s;
    if ((oo=DXGetAttribute(o, "rendering approximation"))!=NULL) {
 if (!DXExtractString(oo, &s))
     DXErrorReturn(ERROR_BAD_PARAMETER,
   "bad rendering approximation attribute");
 if (strcmp(s,"box")==0)
     *approx = approx_box;
 else if (strcmp(s,"dots")==0)
     *approx = approx_dots;
    }
    return OK;
}
static Error
parameters(Object o, struct shade *new, struct shade *old)
{
    Object a;
    *new = *old;
    if (old->xonly)
 return ERROR;
    { 
 float p; 
 Object a; 
 a = DXGetAttribute(o, "ambient"); 
 if (a) { 
 if (!DXExtractFloat(DXGetAttribute(o, "ambient"), &p)) 
 DXErrorReturn(ERROR_BAD_PARAMETER, "ambient" " must be " "a float"); 
 new->front.ambient = new->back.ambient = p; 
 } 
 a = DXGetAttribute(o, "front " "ambient"); 
 if (a && !DXExtractFloat(a, &(new->front.ambient))) 
 DXErrorReturn(ERROR_BAD_PARAMETER, 
 "front " "ambient" " must be " "a float"); 
 a = DXGetAttribute(o, "back " "ambient"); 
 if (a && !DXExtractFloat(a, &(new->back.ambient))) 
 DXErrorReturn(ERROR_BAD_PARAMETER, 
 "back " "ambient" " must be " "a float"); 
};
    { 
 float p; 
 Object a; 
 a = DXGetAttribute(o, "specular"); 
 if (a) { 
 if (!DXExtractFloat(DXGetAttribute(o, "specular"), &p)) 
 DXErrorReturn(ERROR_BAD_PARAMETER, "specular" " must be " "a float"); 
 new->front.specular = new->back.specular = p; 
 } 
 a = DXGetAttribute(o, "front " "specular"); 
 if (a && !DXExtractFloat(a, &(new->front.specular))) 
 DXErrorReturn(ERROR_BAD_PARAMETER, 
 "front " "specular" " must be " "a float"); 
 a = DXGetAttribute(o, "back " "specular"); 
 if (a && !DXExtractFloat(a, &(new->back.specular))) 
 DXErrorReturn(ERROR_BAD_PARAMETER, 
 "back " "specular" " must be " "a float"); 
};
    { 
 float p; 
 Object a; 
 a = DXGetAttribute(o, "diffuse"); 
 if (a) { 
 if (!DXExtractFloat(DXGetAttribute(o, "diffuse"), &p)) 
 DXErrorReturn(ERROR_BAD_PARAMETER, "diffuse" " must be " "a float"); 
 new->front.diffuse = new->back.diffuse = p; 
 } 
 a = DXGetAttribute(o, "front " "diffuse"); 
 if (a && !DXExtractFloat(a, &(new->front.diffuse))) 
 DXErrorReturn(ERROR_BAD_PARAMETER, 
 "front " "diffuse" " must be " "a float"); 
 a = DXGetAttribute(o, "back " "diffuse"); 
 if (a && !DXExtractFloat(a, &(new->back.diffuse))) 
 DXErrorReturn(ERROR_BAD_PARAMETER, 
 "back " "diffuse" " must be " "a float"); 
};
    { 
 int p; 
 Object a; 
 a = DXGetAttribute(o, "shininess"); 
 if (a) { 
 if (!DXExtractInteger(DXGetAttribute(o, "shininess"), &p)) 
 DXErrorReturn(ERROR_BAD_PARAMETER, "shininess" " must be " "an integer"); 
 new->front.shininess = new->back.shininess = p; 
 } 
 a = DXGetAttribute(o, "front " "shininess"); 
 if (a && !DXExtractInteger(a, &(new->front.shininess))) 
 DXErrorReturn(ERROR_BAD_PARAMETER, 
 "front " "shininess" " must be " "an integer"); 
 a = DXGetAttribute(o, "back " "shininess"); 
 if (a && !DXExtractInteger(a, &(new->back.shininess))) 
 DXErrorReturn(ERROR_BAD_PARAMETER, 
 "back " "shininess" " must be " "an integer"); 
};
    if (NULL != (a = DXGetAttribute(o, "fuzz")))
    {
 float f = 0;
 if (! DXExtractFloat(a, &f))
 {
     DXSetError(ERROR_BAD_PARAMETER, "\"fuzz\" must be a float");
     return ERROR;
 }
 new->fuzz += f * new->ff;
    }
    if (DXGetAttribute(o, "interference object")) old->count->fast = 0;
    if (DXGetAttribute(o, "interference group")) old->count->fast = 0;
    if (!_dxf_approx(o, &new->approx))
 return ERROR;
    return OK;
}
Object
_dxfXShade(Object o, Camera c, struct count *count, Point *box)
{
    struct shade shade;
    Light distant_light = NULL;
    struct docount *docountlist = NULL, *dcl, *dclnext;
    int i;
    shade.o = o;
    shade.c = c;
    shade.front = default_parms;
    shade.back = default_parms;
    shade.fuzz = 0;
    shade.ff = 1;
    shade.g = DXNewGroup();
    if (!shade.g) goto error;
    shade.xflag = 0;
    shade.xonly = 0;
    shade.nocolors = 0;
    shade.count = count;
    shade.l = (struct l *) DXAllocate(sizeof(struct l));
    if (!shade.l) goto error;
    shade.l->nl = 0;
    shade.l->ambient = DXRGB(0,0,0);
    shade.l->na = 0;
    shade.notight = DXGetAttribute(o, "notight")? 1 : 0;
    shade.docount = NULL;
    shade.docountlist = &docountlist;
    shade.approx = approx_none;
    shade.lightList = NULL;
    shade.lightList = _dxfGetLights(shade.o, shade.c);
    if (! shade.lightList)
 goto error;
    DXReference((Object)shade.lightList);
    DXCreateTaskGroup();
    if (!_dxfShade(o, &shade)) goto error;
    ASSERT(shade.l->nl<100);
    if (shade.l->nl + shade.l->na == 0) {
 Point from, to;
 Vector eye, up, left, dir;
 DXGetView(c, &from, &to, &up);
 eye = DXNormalize(DXSub(from,to));
 left = DXCross(eye,up);
 dir = DXAdd(eye,left);
 distant_light = DXNewDistantLight(dir, DXRGB(1,1,1));
 if (!distant_light)
     return NULL;
 shade.l->lights[shade.l->nl++] = distant_light;
 shade.l->ambient = DXRGB(.2, .2, .2);
    }
    if (!DXExecuteTaskGroup()) goto error;
    for (dcl=docountlist; dcl; dcl=dclnext) {
 dclnext = dcl->next;
 DXFree((Pointer)dcl);
    }
    for (i=0; i<shade.l->nl; i++)
 DXDelete((Object)shade.l->lights[i]);
    DXFree((Pointer) shade.l);
    if (shade.lightList)
 DXDelete((Object)shade.lightList);
    return (Object) shade.g;
error:
    if (shade.l) {
 for (i=0; i<shade.l->nl; i++)
     DXDelete((Object)shade.l->lights[i]);
 DXFree((Pointer)shade.l);
    }
    if (shade.g)
 DXDelete((Object)shade.g);
    if (shade.lightList)
 DXDelete((Object)shade.lightList);
    return NULL;
}
Object
DXTransform(Object o, Matrix *mp, Camera c)
{
    struct shade shade;
    memset(&shade, 0, sizeof(shade));
    shade.c = c;
    if (mp) {
 shade.m = *mp;
 shade.xflag = 1;
    }
    shade.xonly = 1;
    shade.nocolors = 1;
    shade.g = DXNewGroup();
    DXCreateTaskGroup();
    if (!_dxfShade(o, &shade)) return NULL;
    if (!DXExecuteTaskGroup()) return NULL;
    if (!DXCopyAttributes((Object)shade.g, o)) return NULL;
    return (Object) shade.g;
}
static float
ipow(float x, int n)
{
    float xn;
    for (xn=1.0; n; n>>=1, x*=x)
 if (n&1)
     xn *= x;
    return xn;
}
static Error
count(struct xfield *xf, struct shade *shade, Point *box);
static Error
task(struct shade *shade)
{
    Array a, normals_array = NULL, box = NULL;
    struct xfield xf;
    Field f = NULL;
    Matrix *mp = shade->xflag? &shade->m : NULL;
    Object o;
    int behind = 0;
 InitializeXfield(xf);
    if (DXEmptyField(shade->f))
 return OK;
    f = (Field) DXCopy((Object)shade->f, COPY_HEADER);
    if (!f)
 goto error;
    if (DXGetComponentValue(f, "faces"))
 if (! _dxfTriangulateField(f))
     goto error;
    _dxf_XZero(&xf);
    if (!shade->nocolors) {
 if (!_dxf_XColors(f, &xf, XR_REQUIRED, XD_GLOBAL)) goto error;
 if (shade->approx!=approx_dots)
     if (!_dxf_XOpacities(f, &xf, XR_OPTIONAL, XD_NONE)) goto error;
    }
    if (! _dxf_XPositions(f, &xf, XR_REQUIRED, XD_NONE)) goto error;
    if (shade->approx!=approx_dots) {
 if (!_dxf_XConnections(f, &xf, XR_OPTIONAL)) goto error;
 if (!_dxf_XPolylines(f, &xf, XR_OPTIONAL, XD_NONE)) goto error;
 if (!_dxf_XNormals(f, &xf, XR_OPTIONAL, XD_NONE)) goto error;
 if (xf.colors_dep==dep_connections || xf.normals_dep==dep_connections)
     if (! _dxf_XInvalidConnections(f, &xf)) goto error;
 if (xf.colors_dep==dep_positions || xf.normals_dep==dep_positions)
     if (! _dxf_XInvalidPositions(f, &xf)) goto error;
 if (xf.colors_dep==dep_polylines || xf.normals_dep==dep_polylines)
     if (! _dxf_XInvalidPolylines(f, &xf)) goto error;
    }
    if (xf.colors_dep==dep_positions) {
 if (xf.ncolors && xf.ncolors!=xf.npositions) {
     DXSetError(ERROR_BAD_PARAMETER,
     "colors are dependent on positions but have different number of entries");
     goto error;
 }
    } else {
 if (xf.ncolors && xf.nconnections && xf.ncolors!=xf.nconnections) {
     DXSetError(ERROR_BAD_PARAMETER,
     "colors are dependent on connections but have different number of entries");
     goto error;
 }
    }
    DXMarkTimeLocal("extract");
    if (! DXInvalidateConnections((Object)f))
 goto error;
    if (shade->approx!=approx_dots) {
 if (xf.colors_dep==dep_connections || xf.normals_dep==dep_connections)
     if (! _dxf_XInvalidConnections(f, &xf)) goto error;
 if (xf.colors_dep==dep_positions || xf.normals_dep==dep_positions)
     if (! _dxf_XInvalidPositions(f, &xf)) goto error;
 if (xf.colors_dep==dep_polylines || xf.normals_dep==dep_polylines)
     if (! _dxf_XInvalidPolylines(f, &xf)) goto error;
    }
    a = _dxf_TransformArray(mp, shade->c, xf.positions_array,
   shade->notight? NULL : &box,
   f, &behind, shade->fuzz);
    if (!a)
 goto error;
    DXSetComponentValue(f, POSITIONS, (Object) a);
    if (!box) {
 box = (Array) DXGetComponentValue(f, BOX);
 box = _dxf_TransformBox(mp, shade->c, box, &behind, shade->fuzz);
 if (!box) goto error;
    }
    DXSetComponentValue(f, BOX, (Object) box);
    if (behind) {
 DXDelete((Object)f);
 DXDebug("R", "1 field (%d connections) rejected", xf.nconnections);
 DXMarkTimeLocal("rejected");
 return OK;
    }
    if (mp) {
 float l, mul;
 l = DXDeterminant(*mp);
 if (l<0) l = -l;
 if (l) {
     l = pow(l, -1.0/3.0);
     o = DXGetAttribute((Object)f, "color multiplier");
     if (!DXExtractFloat(o, &mul)) mul = 1;
     DXSetFloatAttribute((Object)f, "color multiplier", l*mul);
     o = DXGetAttribute((Object)f, "opacity multiplier");
     if (!DXExtractFloat(o, &mul)) mul = 1;
     DXSetFloatAttribute((Object)f, "opacity multiplier", l*mul);
 }
 DXMarkTimeLocal("multiplier");
    }
    if (shade->xonly) {
 DXDelete((Object)f);
 return OK;
    }
    count(&xf, shade, (Point *)DXGetArrayData(box));
    DXMarkTimeLocal("count");
    if (!xf.fcolors_array && !xf.bcolors_array)
    {
 if (! DXSetMember(shade->g, NULL, (Object)f))
     goto error;
 _dxf_XFreeLocal(&xf);
 return OK;
    }
    if (shade->approx==approx_dots && xf.colors_dep!=dep_positions) {
 static RGBColor color = {1, 1, 0};
 static RGBColor zero = {0, 0, 0};
 Object a = (Object) DXNewRegularArray(TYPE_FLOAT, 3, xf.npositions,
         (Pointer)&color, (Pointer)&zero);
 if (!DXGetArrayData((Array)a))
     goto error;
 if (!DXSetComponentValue(f, FRONT_COLORS, (Object) a))
     goto error;
 if (!DXSetAttribute(a, DEP, O_POSITIONS))
     goto error;
 DXDeleteComponent(f, BACK_COLORS);
 DXDeleteComponent(f, COLORS);
    }
    if (xf.volume || !xf.normals_array || !shade->c)
    {
 if (! DXSetMember(shade->g, NULL, (Object)f))
     goto error;
 _dxf_XFreeLocal(&xf);
 return OK;
    }
    normals_array = _dxf_TransformNormals(mp, xf.normals_array);
    if (!normals_array) goto error;
    if (xf.colors_dep != xf.normals_dep)
    {
 if (! DXSetComponentValue(f, NORMALS, (Object)normals_array))
     goto error;
 normals_array = NULL;
 if (! shade->lightList)
 {
     DXSetError(ERROR_INTERNAL, "missing light list");
     return ERROR;
 }
 if (! DXSetAttribute((Object)f, "lights", (Object)shade->lightList))
     goto error;
    }
    else
    {
 Array fcolors_array = NULL, bcolors_array = NULL;
 if (! work(&xf, shade, normals_array, &fcolors_array, &bcolors_array))
     goto error;
 DXDeleteComponent(f, NORMALS);
 DXDeleteComponent(f, COLORS);
 DXDeleteComponent(f, FRONT_COLORS);
 DXDeleteComponent(f, BACK_COLORS);
 if (fcolors_array)
 {
     DXSetComponentValue(f, "front colors", (Object) fcolors_array);
     if (xf.normals_dep==dep_connections)
  DXSetAttribute((Object)fcolors_array, DEP, O_CONNECTIONS);
     else if (xf.normals_dep==dep_positions)
  DXSetAttribute((Object)fcolors_array, DEP, O_POSITIONS);
     else if (xf.normals_dep==dep_polylines)
  DXSetAttribute((Object)fcolors_array, DEP, O_POLYLINES);
 }
 if (bcolors_array)
 {
     DXSetComponentValue(f, "back colors", (Object) bcolors_array);
     if (xf.normals_dep==dep_connections)
  DXSetAttribute((Object)bcolors_array, DEP, O_CONNECTIONS);
     else if (xf.normals_dep==dep_positions)
  DXSetAttribute((Object)bcolors_array, DEP, O_POSITIONS);
     else if (xf.normals_dep==dep_polylines)
  DXSetAttribute((Object)bcolors_array, DEP, O_POLYLINES);
 }
 if (normals_array != xf.normals_array)
     DXDelete((Object)normals_array);
    }
    if (! DXSetMember(shade->g, NULL, (Object)f))
 goto error;
    f = NULL;
    _dxf_XFreeLocal(&xf);
    return OK;
error:
    DXDelete((Object)f);
    if (normals_array != xf.normals_array)
 DXDelete((Object)normals_array);
    _dxf_XFreeLocal(&xf);
    return ERROR;
}
static Error box_approx(Object o, struct shade *shade);
static int
IsInvisible(Object o)
{
    Object attr;
    int i;
    attr = DXGetAttribute(o, "visible");
    if (! attr)
 return 0;
    if (! DXExtractInteger(attr, &i))
 return 0;
    return i ? 0 : 1;
}
Error
_dxfField_Shade(Field f, struct shade *old)
{
    struct shade new;
    Array a;
    int n = 0;
    if (IsInvisible((Object)f)) return OK;
    if (!parameters((Object)f, &new, old)) return ERROR;
    if (new.approx==approx_box)
 return box_approx((Object)f, &new);
    a = (Array) DXGetComponentValue(f, POSITIONS);
    DXGetArrayInfo(a, &n, NULL, NULL, NULL, NULL);
    new.f = f;
    if (n)
 DXAddTask((Error(*)(Pointer))task, (Pointer)&new, sizeof(new), (float)n);
    return OK;
}
static Error
box_approx(Object o, struct shade *shade)
{
    int i;
    Field f;
    Point box[8];
    RGBColor colors[8];
    static Line lines[12] = {
 {0,1}, {0,2}, {0,4}, {1,3}, {1,5}, {2,3}, {2,6}, {3,7}, {4,5}, {4,6}, {5,7}, {6,7}
    };
    if (!DXBoundingBox(o, box))
 return OK;
    for (i=0; i<8; i++)
 colors[i] = DXRGB(1,1,0);
    f = DXNewField();
    if (!DXAddPoints(f, 0, 8, box)) return ERROR;
    if (!DXAddLines(f, 0, 12, lines)) return ERROR;
    if (!DXAddColors(f, 0, 8, colors)) return ERROR;
    if (!DXEndField(f)) return ERROR;
    shade->approx = approx_none;
    return _dxfField_Shade(f, shade);
}
Error
_dxfXform_Shade(Xform x, struct shade *old)
{
    struct shade new;
    Object o;
    if (IsInvisible((Object)x)) return OK;
    if (!parameters((Object)x, &new, old)) return ERROR;
    if (new.approx==approx_box)
 return box_approx((Object)x, &new);
    DXGetXformInfo(x, &o, &new.m);
    if (old->xflag)
 new.m = DXConcatenate(new.m, old->m);
    new.xflag = 1;
    new.g = DXNewGroup();
    if (!new.g) goto error;
    if (!DXCopyAttributes((Object)new.g, (Object)x)) goto error;
    if (!_dxfShade(o, &new)) goto error;
    if (!DXSetMember(old->g, NULL, (Object)new.g)) goto error;
    return OK;
error:
    DXDelete((Object)new.g);
    return ERROR;
}
Error
_dxfScreen_Shade(Screen s, struct shade *old)
{
    Object ns = NULL, o, oo;
    struct shade new;
    int fixed, z, width, height;
    if (IsInvisible((Object)s)) return OK;
    if (!parameters((Object)s, &new, old)) return ERROR;
    if (new.approx==approx_box)
 return box_approx((Object)s, &new);
    if (!DXGetScreenInfo(s, &o, &fixed, &z)) return ERROR;
    DXGetCameraResolution(old->c, &width, &height);
    if (fixed==SCREEN_VIEWPORT) {
 if (old->c) {
     new.m.b[0] = new.m.b[0]*width - width/2;
     new.m.b[1] = new.m.b[1]*height - height/2;
 }
    } else if (fixed==SCREEN_PIXEL) {
 if (old->c) {
     new.m.b[0] -= width/2;
     new.m.b[1] -= height/2;
 }
    } else if (fixed==SCREEN_WORLD) {
 if (old->xflag && old->c) {
     Matrix cm;
     cm = DXGetCameraMatrix(old->c);
     new.m = DXConcatenate(new.m, cm);
     new.xflag = 1;
 } else if (old->xflag) {
     new.m = old->m;
     new.xflag = 1;
 } else if (old->c) {
     new.m = DXGetCameraMatrix(old->c);
     new.xflag = 1;
 }
 if (DXGetPerspective(old->c, NULL, NULL)) {
     new.m.b[0] = - new.m.b[0] / new.m.b[2];
     new.m.b[1] = - new.m.b[1] / new.m.b[2];
 }
    } else if (fixed==SCREEN_STATIONARY) {
 new.m.b[0] = new.m.b[1] = new.m.b[2] = 0;
    }
    if (fixed!=SCREEN_STATIONARY) {
 if (z>0) {
     new.m.b[2] = DXD_MAX_FLOAT / 2;
     new.ff = DXD_MAX_FLOAT / 1000000;
 } else if (z==0) {
     new.m.b[2] = 0;
 } else {
     new.m.b[2] = -DXD_MAX_FLOAT / 2;
     new.ff = DXD_MAX_FLOAT / 1000000;
 }
 new.fuzz *= new.ff;
 new.c = NULL;
    }
    new.m.A[0][0] = 1, new.m.A[0][1] = 0, new.m.A[0][2] = 0;
    new.m.A[1][0] = 0, new.m.A[1][1] = 1, new.m.A[1][2] = 0;
    new.m.A[2][0] = 0, new.m.A[2][1] = 0, new.m.A[2][2] = 1;
    new.g = DXNewGroup();
    if (!new.g) goto error;
    if (!DXCopyAttributes((Object)new.g, (Object)s)) goto error;
    if (!_dxfShade(o, &new)) goto error;
    oo = (Object) DXNewScreen((Object)new.g, 0, 0);
    if (!DXSetMember(old->g, NULL, oo)) goto error;
    return OK;
error:
    DXDelete((Object)new.g);
    DXDelete((Object)ns);
    return ERROR;
}
Error
_dxfGroup_Shade(Group g, struct shade *old)
{
    int i;
    Object m;
    struct shade new;
    if (IsInvisible((Object)g)) return OK;
    if (!parameters((Object)g, &new, old)) return ERROR;
    if (new.approx==approx_box)
 return box_approx((Object)g, &new);
    new.g = (Group) DXCopy((Object)g, COPY_ATTRIBUTES);
    if (DXGetGroupClass(g)==CLASS_COMPOSITEFIELD && !new.docount) {
 new.docount = (struct docount *) DXAllocate(sizeof(struct docount));
 if (!new.docount) goto error;
 new.docount->docount = 1;
 new.docount->next = *new.docountlist;
 *new.docountlist = new.docount;
    }
    for (i=0; (m=DXGetEnumeratedMember(g, i, NULL)); i++) {
 if (!_dxfShade(m, &new))
     return ERROR;
    }
    if (!DXSetMember(old->g, NULL, (Object)new.g)) goto error;
    return OK;
error:
    DXDelete((Object) new.g);
    return ERROR;
}
Error
_dxfLight_Shade(Light l, struct shade *shade)
{
    if (shade->xonly)
 return OK;
    if (l->kind==ambient) {
 shade->l->ambient.r += l->color.r;
 shade->l->ambient.g += l->color.g;
 shade->l->ambient.b += l->color.b;
 shade->l->na++;
    } else if (l->kind==distant) {
 if (l->relative==world) {
     if (shade->xflag) {
  Matrix m;
  m = shade->m;
  m.b[0] = m.b[1] = m.b[2] = 0;
  shade->l->lights[shade->l->nl++]
      = DXNewDistantLight(DXApply(l->direction,m), l->color);
     } else {
  int i = shade->l->nl;
  shade->l->lights[i]=(Light)DXReference((Object)l);
  shade->l->nl = i+1;
     }
 } else if (l->relative==camera) {
     Vector from, to, x, y, z, where;
     DXGetView(shade->c, &from, &to, &y);
     z = DXNormalize(DXSub(from,to));
     y = DXNormalize(y);
     x = DXCross(y,z);
     where = DXMul(x, l->direction.x);
     where = DXAdd(where, DXMul(y, l->direction.y));
     where = DXAdd(where, DXMul(z, l->direction.z));
     shade->l->lights[shade->l->nl++]
  = DXNewDistantLight(where, l->color);
 } else {
     DXErrorReturn(ERROR_INTERNAL, "invalid light object");
 }
    } else {
 DXErrorReturn(ERROR_DATA_INVALID,
      "only ambient and distant lights allowed");
    }
    return OK;
}
Error
_dxfClipped_Shade(Clipped c, struct shade *old)
{
    Object nc=NULL, render, clipping;
    Group nrender=NULL, nclipping=NULL;
    struct shade new;
    DXGetClippedInfo(c, &render, &clipping);
    if (IsInvisible((Object)c) || IsInvisible(render)) return OK;
    old->count->fast = 0;
    if (!parameters((Object)c, &new, old)) return ERROR;
    if (new.approx==approx_box)
 return box_approx((Object)c, &new);
    DXGetClippedInfo(c, &render, &clipping);
    nrender = new.g = DXNewGroup();
    if (!nrender) goto error;
    if (!_dxfShade(render, &new)) goto error;
    nclipping = new.g = DXNewGroup();
    new.nocolors = 1;
    if (!nclipping) goto error;
    if (!_dxfShade(clipping, &new)) goto error;
    nc = (Object) DXNewClipped((Object)nrender, (Object)nclipping);
    if (!nc) goto error;
    if (!DXCopyAttributes((Object)nc, (Object)c)) goto error;
    if (old->g && !DXSetMember(old->g, NULL, (Object)nc)) goto error;
    return OK;
error:
    DXDelete((Object)nrender);
    DXDelete((Object)nclipping);
    DXDelete((Object)nc);
    return ERROR;
}
static Error
count(struct xfield *xf, struct shade *shade, Point *box)
{
    Point min, max;
    int lt, rt, tp, bt, i, j, n;
    int nconnections, tfields, nbytes;
    int counts[100];
    int w, h, nx, ny;
    struct pcount *p, *pp;
    struct count *count = shade->count;
    nconnections = xf->ct==ct_none? xf->npositions : xf->nconnections;
    if (nconnections==0)
 return ERROR;
    min.x = min.y = DXD_MAX_FLOAT;
    max.x = max.y = -DXD_MAX_FLOAT;
    for (i=0; i<8; i++) {
 if (box[i].x<min.x) min.x = box[i].x;
 if (box[i].x>max.x) max.x = box[i].x;
 if (box[i].y<min.y) min.y = box[i].y;
 if (box[i].y>max.y) max.y = box[i].y;
    }
    min.x -= .5 ;
    min.y -= .5 ;
    max.x += .5 ;
    max.y += .5 ;
    w = count->width, h = count->height;
    nx = count->nx, ny = count->ny;
    if (min.x<-w) min.x = -w; else if (min.x>w) min.x = w;
    if (max.x<-w) max.x = -w; else if (max.x>w) max.x = w;
    if (min.y<-h) min.y = -h; else if (min.y>h) min.y = h;
    if (max.y<-h) max.y = -h; else if (max.y>h) max.y = h;
    lt = ((((min.x)>=0 || (int)(min.x)==min.x? (int)(min.x) : (int)(min.x)-1)+1 + w/2) * nx - 1 + 1024*w) / w - 1024;
    rt = ((((max.x)>=0 || (int)(max.x)==max.x? (int)(max.x) : (int)(max.x)-1)+1 + w/2) * nx - 1 + 1024*w) / w - 1024;
    bt = ((((min.y)>=0 || (int)(min.y)==min.y? (int)(min.y) : (int)(min.y)-1)+1 + h/2) * ny - 1 + 1024*h) / h - 1024;
    tp = ((((max.y)>=0 || (int)(max.y)==max.y? (int)(max.y) : (int)(max.y)-1)+1 + h/2) * ny - 1 + 1024*h) / h - 1024;
    if (lt<0) lt=0;
    if (rt>=count->nx) rt = count->nx-1;
    if (bt<0) bt=0;
    if (tp>=count->ny) tp = count->ny-1;
    DXDebug("c", "lt %g rt %g bt %g tp %g -> lt %d rt %d  bt %d tp %d",
   min.x, max.x, min.y, max.y, lt, rt, bt, tp);
    DXlock(&count->DXlock, 0);
    tfields = 0;
    nbytes = 0;
    if (xf->opacities_array || xf->volume) {
 int regular=0, irregular=0, volume=0;
 if (xf->ct==ct_none) {
     nbytes = nconnections * (sizeof(struct sort) + (1) * sizeof(int));
     irregular = 1;
 } else if (xf->ct==ct_lines) {
     nbytes = nconnections * (sizeof(struct sort) + (1) * sizeof(int));
     irregular = 1;
 } else if (xf->ct==ct_polylines) {
     nbytes = xf->nedges * (sizeof(struct sort) + (2) * sizeof(int));
     irregular = 1;
 } else if (xf->ct==ct_triangles) {
     nbytes = nconnections * (sizeof(struct sort) + (1) * sizeof(int));
     irregular = 1;
 } else if (xf->ct==ct_quads) {
     nbytes = nconnections * (sizeof(struct sort) + (1) * sizeof(int));
     irregular = 1;
 } else if (xf->ct==ct_tetrahedra) {
     int size = (sizeof(struct sort) + (xf->colors_dep==dep_positions? 3 : 4) * sizeof(int));
     nbytes = 4*nconnections * size;
     irregular = 1;
     volume = 1;
     count->fast = 0;
 } else if (xf->ct==ct_cubes) {
     int size = (sizeof(struct sort) + (xf->colors_dep==dep_positions? 4 : 5) * sizeof(int));
     if (DXQueryGridConnections(xf->connections_array,&n,counts)&&n==3) {
  int x = counts[0], y = counts[1], z = counts[2];
  nbytes = ((x-1)*(y-1)*z+(x-1)*y*(z-1)+x*(y-1)*(z-1)) * size;
  if (DXQueryGridPositions(xf->positions_array,
           NULL, NULL, NULL, NULL))
      regular = 1;
  else
      irregular = 1;
     } else {
  nbytes = 6*nconnections * size;
  irregular = 1;
     }
     volume = 1;
     count->fast = 0;
 }
 tfields = 1;
 if (!shade->docount || shade->docount->docount) {
     count->irregular += irregular;
     count->regular += regular;
     count->volume += volume;
     if (shade->docount)
  shade->docount->docount = 0;
 }
    }
    for (nx=count->nx, p=count->pcount+bt*nx, i=bt; i<=tp; i++, p+=nx) {
 for (pp=p+lt, j=lt; j<=rt; j++, pp++) {
     pp->nconnections += nconnections;
     pp->tfields += tfields;
     pp->nbytes += nbytes;
 }
    }
    DXunlock(&count->DXlock, 0);
    return OK;
}
#define CAT(x,y) x##y
static int byteTableFlag = 0;
static float byteTable[256];
static Error
work(struct xfield *xf, struct shade *shade,
 Array normals_array, Array *front_colors, Array *back_colors)
{
    struct l *l = shade->l;
    int nl = l->nl;
    float fambr = shade->front.ambient * l->ambient.r;
    float fambg = shade->front.ambient * l->ambient.g;
    float fambb = shade->front.ambient * l->ambient.b;
    float bambr = shade->back.ambient * l->ambient.r;
    float bambg = shade->back.ambient * l->ambient.g;
    float bambb = shade->back.ambient * l->ambient.b;
    float fspec = shade->front.specular;
    float bspec = shade->back.specular;
    float fdiff = shade->front.diffuse;
    float bdiff = shade->back.diffuse;
    int fshine = shade->front.shininess;
    int bshine = shade->back.shininess;
    int n = xf->ncolors, i, j;
    RGBColor *ofc, *obc, *col, *cmap = xf->cmap;
    Pointer ifc, ibc;
    Point from, to, eye;
    Vector *norm, *normals = NULL;
    float ex, ey, ez;
    float fspecr, fspecg, fspecb, bspecr, bspecg, bspecb;
    float fdiffr, fdiffg, fdiffb, bdiffr, bdiffg, bdiffb;
    float nx, ny, nz;
    float lr, lg, lb, lx, ly, lz, _hx, _hy, _hz;
    float d ;
    InvalidComponentHandle ich;
    int fcst = xf->fcst, bcst = xf->bcst, ncst;
    Point cstnormal;
    int m;
    RGBColor *fcolors, *bcolors;
    int fbyte = xf->fbyte, bbyte = xf->bbyte;
    if ((fbyte || bbyte) && !byteTableFlag)
    {
 int i;
 for (i = 0; i < 256; i++)
    byteTable[i] = i / 256.0;
 byteTableFlag = 1;
    }
    *front_colors = NULL;
    *back_colors = NULL;
    DXGetView(shade->c, &from, &to, NULL);
    eye = DXNormalize(DXSub(from, to));
    ex=eye.x, ey=eye.y, ez=eye.z;
    if (fshine<0 || bshine<0)
 DXErrorReturn(ERROR_BAD_PARAMETER, "shininess may not be negative");
    if (xf->normals_dep == dep_positions)
 ich = xf->iPts;
    else
 ich = xf->iElts;
    if (DXQueryConstantArray(normals_array, &m, (Pointer)&cstnormal))
    {
 norm = (Vector *)&cstnormal;
 if (fcst || bcst)
 {
     RGBColor fout, bout;
     if (fcst)
     {
  if (fbyte)
  {
      unsigned char *fin = (unsigned char *)xf->fcolors;
      fout.r = byteTable[*fin++] * fambr;
      fout.g = byteTable[*fin++] * fambg;
      fout.b = byteTable[*fin++] * fambb;
  }
  else
  {
      RGBColor *fin;
      if (cmap) fin = cmap + *(unsigned char *)&(xf->fcolors);
      else fin = (RGBColor *)xf->fcolors;
      fout.r = fin->r * fambr;
      fout.g = fin->g * fambg;
      fout.b = fin->b * fambb;
  }
     }
     if (bcst)
     {
  if (bbyte)
  {
      unsigned char *bin = (unsigned char *)xf->bcolors;
      bout.r = byteTable[*bin++] * bambr;
      bout.g = byteTable[*bin++] * bambg;
      bout.b = byteTable[*bin++] * bambb;
  }
  else
  {
      RGBColor *bin;
      if (cmap) bin = cmap + *(unsigned char *)&(xf->bcolors);
      else bin = (RGBColor *)xf->bcolors;
      bout.r = bin->r * bambr;
      bout.g = bin->g * bambg;
      bout.b = bin->b * bambb;
  }
     }
     for (j = 0; j < nl; j++)
     {
  Light light = l->lights[j];
  lr = light->color.r;
  lg = light->color.g;
  lb = light->color.b;
  lx = light->direction.x,
  ly = light->direction.y;
  lz = light->direction.z;
  _hx=ex+lx, _hy=ey+ly, _hz=ez+lz;
  d = _hx*_hx + _hy*_hy + _hz*_hz;
  if (d != 0.0)
      d = (float)1.0 / sqrt(d);
  _hx*=d, _hy*=d, _hz*=d;
  if (light->kind == distant)
  {
      { 
 
 
 nx=norm->x, ny=norm->y, nz=norm->z; 
 
 
 d = _hx*nx + _hy*ny + _hz*nz; 
 if (d>0) { 
 d = fspec * ipow(d, fshine); 
 fspecr=d*lr, fspecg=d*lg, fspecb=d*lb; 
 bspecr = bspecg = bspecb = 0; 
 } else { 
 d = bspec * ipow(-d, bshine); 
 bspecr=d*lr, bspecg=d*lg, bspecb=d*lb; 
 fspecr = fspecg = fspecb = 0; 
 } 
 
 
 d = lx*nx + ly*ny + lz*nz; 
 if (d>0) { 
 d = d * fdiff; 
 fdiffr=d*lr, fdiffg=d*lg, fdiffb=d*lb; 
 bdiffr = bdiffg = bdiffb = 0; 
 } else { 
 d = -d * bdiff; 
 bdiffr=d*lr, bdiffg=d*lg, bdiffb=d*lb; 
 fdiffr = fdiffg = fdiffb = 0; 
 } 
};
      if (fcst)
      {
   if (fbyte)
   {
       unsigned char *fin = (unsigned char *)xf->fcolors;
       fout.r += (byteTable[*fin++] * fdiffr) + fspecr;
       fout.g += (byteTable[*fin++] * fdiffg) + fspecg;
       fout.b += (byteTable[*fin++] * fdiffb) + fspecb;
   }
   else
   {
       RGBColor *fin = (RGBColor *)xf->fcolors;
       fout.r += (fin->r * fdiffr) + fspecr;
       fout.g += (fin->g * fdiffg) + fspecg;
       fout.b += (fin->b * fdiffb) + fspecb;
   }
      }
      if (bcst)
      {
   if (fbyte)
   {
       unsigned char *bin = (unsigned char *)xf->bcolors;
       bout.r += (byteTable[*bin++] * bdiffr) + bspecr;
       bout.g += (byteTable[*bin++] * bdiffg) + bspecg;
       bout.b += (byteTable[*bin++] * bdiffb) + bspecb;
   }
   else
   {
       RGBColor *bin = (RGBColor *)xf->bcolors;
       bout.r += (bin->r * bdiffr) + bspecr;
       bout.g += (bin->g * bdiffg) + bspecg;
       bout.b += (bin->b * bdiffb) + bspecb;
   }
      }
  }
     }
     if (fcst)
     {
  ((fout.r) = ((fout.r) > 1.0) ? 1.0 : ((fout.r) < 0.0) ? 0.0 : (fout.r));
  ((fout.g) = ((fout.g) > 1.0) ? 1.0 : ((fout.g) < 0.0) ? 0.0 : (fout.g));
  ((fout.b) = ((fout.b) > 1.0) ? 1.0 : ((fout.b) < 0.0) ? 0.0 : (fout.b));
  *front_colors = (Array)DXNewConstantArray(m, (Pointer)&fout,
         TYPE_FLOAT, CATEGORY_REAL, 1, 3);
  if (! *front_colors)
      goto error;
     }
     if (bcst)
     {
  ((bout.r) = ((bout.r) > 1.0) ? 1.0 : ((bout.r) < 0.0) ? 0.0 : (bout.r));
  ((bout.g) = ((bout.g) > 1.0) ? 1.0 : ((bout.g) < 0.0) ? 0.0 : (bout.g));
  ((bout.b) = ((bout.b) > 1.0) ? 1.0 : ((bout.b) < 0.0) ? 0.0 : (bout.b));
  *back_colors = (Array)DXNewConstantArray(m, (Pointer)&bout,
         TYPE_FLOAT, CATEGORY_REAL, 1, 3);
  if (! *back_colors)
      goto error;
     }
 }
 ncst = 1;
    }
    else
    {
 normals = (Vector *)DXGetArrayData(normals_array);
 ncst = 0;
    }
    if ((!fcst || !ncst) && xf->fcolors_array)
    {
 *front_colors = DXNewArray(TYPE_FLOAT, CATEGORY_REAL, 1, 3);
 if (! *front_colors)
     goto error;
 if (! DXAddArrayData(*front_colors, 0, xf->ncolors, NULL))
     goto error;
 fcolors = (RGBColor *)DXGetArrayData(*front_colors);
 if (!fcolors)
     goto error;
    }
    else
 fcolors = NULL;
    if ((!bcst || !ncst) && xf->bcolors_array)
    {
 *back_colors = DXNewArray(TYPE_FLOAT, CATEGORY_REAL, 1, 3);
 if (! *back_colors)
     goto error;
 if (! DXAddArrayData(*back_colors, 0, xf->ncolors, NULL))
     goto error;
 bcolors = (RGBColor *)DXGetArrayData(*back_colors);
 if (!bcolors)
     goto error;
    }
    else
 bcolors = NULL;
    if (nl==0) {
 ifc = xf->fcolors, ofc = fcolors;
 ibc = xf->bcolors, obc = bcolors;
 if (cmap) {
     if (ofc) {
  if (fcst)
  {
      col = &cmap[*(unsigned char *)ifc];
      fcolors->r = col->r * fambr;
      fcolors->g = col->g * fambg;
      fcolors->b = col->b * fambb;
      for (ofc++, i=1; i<n; i++) {
   *ofc++ = *fcolors;
      }
  }
  else
  {
      for (i=0; i<n; i++) {
   col = &cmap[*(unsigned char *)ifc];
   ofc->r = col->r * fambr;
   ofc->g = col->g * fambg;
   ofc->b = col->b * fambb;
   ofc++, ifc=(Pointer)(((unsigned char *)ifc)+1);
      }
  }
     }
     if (obc) {
  if (bcst)
  {
      col = &cmap[*(unsigned char *)ibc];
      bcolors->r = col->r * bambr;
      bcolors->g = col->g * bambg;
      bcolors->b = col->b * bambb;
      for (obc++, i=1; i<n; i++) {
   *obc++ = *bcolors;
      }
  }
  else
  {
      for (i=0; i<n; i++) {
   col = &cmap[*(unsigned char *)ibc];
   obc->r = col->r * bambr;
   obc->g = col->g * bambg;
   obc->b = col->b * bambb;
   obc++, ibc=(Pointer)(((unsigned char *)ibc)+1);
      }
  }
     }
 } else {
     if (ofc) {
  if (fcst)
  {
      if (fbyte)
      {
   fcolors->r = byteTable[((unsigned char *)ifc)[0]] * fambr;
   fcolors->g = byteTable[((unsigned char *)ifc)[1]] * fambg;
   fcolors->b = byteTable[((unsigned char *)ifc)[2]] * fambb;
      }
      else
      {
   fcolors->r = ((RGBColor *)ifc)->r * fambr;
   fcolors->g = ((RGBColor *)ifc)->g * fambg;
   fcolors->b = ((RGBColor *)ifc)->b * fambb;
      }
      for (ofc++, i=1; i<n; i++) {
   *ofc++ = *fcolors;
      }
  }
  else
  {
      if (fbyte)
      {
   unsigned char *ifc_uchar = (unsigned char *)ifc;
   for (i=0; i<n; i++) {
       ofc->r = byteTable[*ifc_uchar++] * fambr;
       ofc->g = byteTable[*ifc_uchar++] * fambg;
       ofc->b = byteTable[*ifc_uchar++] * fambb;
       ofc++;
   }
      }
      else
      {
   for (i=0; i<n; i++) {
       ofc->r = ((RGBColor *)ifc)->r * fambr;
       ofc->g = ((RGBColor *)ifc)->g * fambg;
       ofc->b = ((RGBColor *)ifc)->b * fambb;
       ofc++, ifc=(Pointer)(((RGBColor *)ifc)+1);
   }
      }
  }
     }
     if (obc) {
  if (bcst)
  {
      if (bbyte)
      {
   bcolors->r = byteTable[((unsigned char *)ibc)[0]] * bambr;
   bcolors->g = byteTable[((unsigned char *)ibc)[1]] * bambg;
   bcolors->b = byteTable[((unsigned char *)ibc)[2]] * bambb;
      }
      else
      {
   bcolors->r = ((RGBColor *)ibc)->r * bambr;
   bcolors->g = ((RGBColor *)ibc)->g * bambg;
   bcolors->b = ((RGBColor *)ibc)->b * bambb;
      }
      for (obc++, i=1; i<n; i++) {
   *obc++ = *bcolors;
      }
  }
  else
  {
      if (bbyte)
      {
   unsigned char *ibc_uchar = (unsigned char *)ibc;
   for (i=0; i<n; i++) {
       obc->r = byteTable[*ibc_uchar++] * bambr;
       obc->g = byteTable[*ibc_uchar++] * bambg;
       obc->b = byteTable[*ibc_uchar++] * bambb;
       obc++;
   }
      }
      else
      {
   for (i=0; i<n; i++) {
       obc->r = ((RGBColor *)ibc)->r * bambr;
       obc->g = ((RGBColor *)ibc)->g * bambg;
       obc->b = ((RGBColor *)ibc)->b * bambb;
       obc++, ibc=(Pointer)(((RGBColor *)ibc)+1);
   }
      }
  }
     }
 }
    }
    for (j=0; j<nl; j++) {
 Light light = l->lights[j];
 lr = light->color.r;
 lg = light->color.g;
 lb = light->color.b;
 lx = light->direction.x,
 ly = light->direction.y;
 lz = light->direction.z;
 if (light->kind==distant) {
     ifc = xf->fcolors, ofc = fcolors;
     ibc = xf->bcolors, obc = bcolors;
     _hx=ex+lx, _hy=ey+ly, _hz=ez+lz;
     d = _hx*_hx + _hy*_hy + _hz*_hz;
     if (d != 0.0)
  d = (float)1.0 / sqrt(d);
     _hx*=d, _hy*=d, _hz*=d;
     if (cmap) {
  if (j==0) {
      { 
 if (ich) { 
 if (ofc && obc) { 
 if (ncst) norm = &cstnormal; else norm = normals; 
 for (i=0; i < n; i++) { 
 if (DXIsElementValid(ich, i)) { 
 { 
 
 
 nx=norm->x, ny=norm->y, nz=norm->z; 
 
 
 d = _hx*nx + _hy*ny + _hz*nz; 
 if (d>0) { 
 d = fspec * ipow(d, fshine); 
 fspecr=d*lr, fspecg=d*lg, fspecb=d*lb; 
 bspecr = bspecg = bspecb = 0; 
 } else { 
 d = bspec * ipow(-d, bshine); 
 bspecr=d*lr, bspecg=d*lg, bspecb=d*lb; 
 fspecr = fspecg = fspecb = 0; 
 } 
 
 
 d = lx*nx + ly*ny + lz*nz; 
 if (d>0) { 
 d = d * fdiff; 
 fdiffr=d*lr, fdiffg=d*lg, fdiffb=d*lb; 
 bdiffr = bdiffg = bdiffb = 0; 
 } else { 
 d = -d * bdiff; 
 bdiffr=d*lr, bdiffg=d*lg, bdiffb=d*lb; 
 fdiffr = fdiffg = fdiffb = 0; 
 } 
}; 
 
 col = &cmap[*(unsigned char *)ifc]; 
 ofc->r = col->r * (CAT(f,diffr) + CAT(f,ambr)) + CAT(f,specr); 
 ofc->g = col->g * (CAT(f,diffg) + CAT(f,ambg)) + CAT(f,specg); 
 ofc->b = col->b * (CAT(f,diffb) + CAT(f,ambb)) + CAT(f,specb);; 
 
 col = &cmap[*(unsigned char *)ibc]; 
 obc->r = col->r * (CAT(b,diffr) + CAT(b,ambr)) + CAT(b,specr); 
 obc->g = col->g * (CAT(b,diffg) + CAT(b,ambg)) + CAT(b,specg); 
 obc->b = col->b * (CAT(b,diffb) + CAT(b,ambb)) + CAT(b,specb);; 
 } 
 
 ofc++; if (!fcst) ifc=(Pointer)(((unsigned char *)ifc)+1); 
 
 obc++; if (!bcst) ibc=(Pointer)(((unsigned char *)ibc)+1); 
 if (!ncst) norm ++; 
 } 
 } else if (ofc) { 
 if (ncst) norm = &cstnormal; else norm = normals; 
 for (i=0; i < n; i++) { 
 if (DXIsElementValid(ich, i)) { 
 { 
 
 
 nx=norm->x, ny=norm->y, nz=norm->z; 
 
 
 d = _hx*nx + _hy*ny + _hz*nz; 
 if (d>0) { 
 d = fspec * ipow(d, fshine); 
 fspecr=d*lr, fspecg=d*lg, fspecb=d*lb; 
 bspecr = bspecg = bspecb = 0; 
 } else { 
 d = bspec * ipow(-d, bshine); 
 bspecr=d*lr, bspecg=d*lg, bspecb=d*lb; 
 fspecr = fspecg = fspecb = 0; 
 } 
 
 
 d = lx*nx + ly*ny + lz*nz; 
 if (d>0) { 
 d = d * fdiff; 
 fdiffr=d*lr, fdiffg=d*lg, fdiffb=d*lb; 
 bdiffr = bdiffg = bdiffb = 0; 
 } else { 
 d = -d * bdiff; 
 bdiffr=d*lr, bdiffg=d*lg, bdiffb=d*lb; 
 fdiffr = fdiffg = fdiffb = 0; 
 } 
}; 
 
 col = &cmap[*(unsigned char *)ifc]; 
 ofc->r = col->r * (CAT(f,diffr) + CAT(f,ambr)) + CAT(f,specr); 
 ofc->g = col->g * (CAT(f,diffg) + CAT(f,ambg)) + CAT(f,specg); 
 ofc->b = col->b * (CAT(f,diffb) + CAT(f,ambb)) + CAT(f,specb);; 
 } 
 
 ofc++; if (!fcst) ifc=(Pointer)(((unsigned char *)ifc)+1); 
 if (!ncst) norm ++; 
 } 
 } else if (obc) { 
 if (ncst) norm = &cstnormal; else norm = normals; 
 for (i=0; i < n; i++) { 
 if (DXIsElementValid(ich, i)) { 
 { 
 
 
 nx=norm->x, ny=norm->y, nz=norm->z; 
 
 
 d = _hx*nx + _hy*ny + _hz*nz; 
 if (d>0) { 
 d = fspec * ipow(d, fshine); 
 fspecr=d*lr, fspecg=d*lg, fspecb=d*lb; 
 bspecr = bspecg = bspecb = 0; 
 } else { 
 d = bspec * ipow(-d, bshine); 
 bspecr=d*lr, bspecg=d*lg, bspecb=d*lb; 
 fspecr = fspecg = fspecb = 0; 
 } 
 
 
 d = lx*nx + ly*ny + lz*nz; 
 if (d>0) { 
 d = d * fdiff; 
 fdiffr=d*lr, fdiffg=d*lg, fdiffb=d*lb; 
 bdiffr = bdiffg = bdiffb = 0; 
 } else { 
 d = -d * bdiff; 
 bdiffr=d*lr, bdiffg=d*lg, bdiffb=d*lb; 
 fdiffr = fdiffg = fdiffb = 0; 
 } 
}; 
 
 col = &cmap[*(unsigned char *)ibc]; 
 obc->r = col->r * (CAT(b,diffr) + CAT(b,ambr)) + CAT(b,specr); 
 obc->g = col->g * (CAT(b,diffg) + CAT(b,ambg)) + CAT(b,specg); 
 obc->b = col->b * (CAT(b,diffb) + CAT(b,ambb)) + CAT(b,specb);; 
 } 
 
 obc++; if (!bcst) ibc=(Pointer)(((unsigned char *)ibc)+1); 
 if (!ncst) norm ++; 
 } 
 } 
 } else { 
 if (ofc && obc) { 
 if (ncst) norm = &cstnormal; else norm = normals; 
 for (i=0; i < n; i++) { 
 { 
 
 
 nx=norm->x, ny=norm->y, nz=norm->z; 
 
 
 d = _hx*nx + _hy*ny + _hz*nz; 
 if (d>0) { 
 d = fspec * ipow(d, fshine); 
 fspecr=d*lr, fspecg=d*lg, fspecb=d*lb; 
 bspecr = bspecg = bspecb = 0; 
 } else { 
 d = bspec * ipow(-d, bshine); 
 bspecr=d*lr, bspecg=d*lg, bspecb=d*lb; 
 fspecr = fspecg = fspecb = 0; 
 } 
 
 
 d = lx*nx + ly*ny + lz*nz; 
 if (d>0) { 
 d = d * fdiff; 
 fdiffr=d*lr, fdiffg=d*lg, fdiffb=d*lb; 
 bdiffr = bdiffg = bdiffb = 0; 
 } else { 
 d = -d * bdiff; 
 bdiffr=d*lr, bdiffg=d*lg, bdiffb=d*lb; 
 fdiffr = fdiffg = fdiffb = 0; 
 } 
}; 
 
 col = &cmap[*(unsigned char *)ifc]; 
 ofc->r = col->r * (CAT(f,diffr) + CAT(f,ambr)) + CAT(f,specr); 
 ofc->g = col->g * (CAT(f,diffg) + CAT(f,ambg)) + CAT(f,specg); 
 ofc->b = col->b * (CAT(f,diffb) + CAT(f,ambb)) + CAT(f,specb);; 
 
 col = &cmap[*(unsigned char *)ibc]; 
 obc->r = col->r * (CAT(b,diffr) + CAT(b,ambr)) + CAT(b,specr); 
 obc->g = col->g * (CAT(b,diffg) + CAT(b,ambg)) + CAT(b,specg); 
 obc->b = col->b * (CAT(b,diffb) + CAT(b,ambb)) + CAT(b,specb);; 
 
 ofc++; if (!fcst) ifc=(Pointer)(((unsigned char *)ifc)+1); 
 
 obc++; if (!bcst) ibc=(Pointer)(((unsigned char *)ibc)+1); 
 if (!ncst) norm ++; 
 } 
 } else if (ofc) { 
 if (ncst) norm = &cstnormal; else norm = normals; 
 for (i=0; i < n; i++) { 
 { 
 
 
 nx=norm->x, ny=norm->y, nz=norm->z; 
 
 
 d = _hx*nx + _hy*ny + _hz*nz; 
 if (d>0) { 
 d = fspec * ipow(d, fshine); 
 fspecr=d*lr, fspecg=d*lg, fspecb=d*lb; 
 bspecr = bspecg = bspecb = 0; 
 } else { 
 d = bspec * ipow(-d, bshine); 
 bspecr=d*lr, bspecg=d*lg, bspecb=d*lb; 
 fspecr = fspecg = fspecb = 0; 
 } 
 
 
 d = lx*nx + ly*ny + lz*nz; 
 if (d>0) { 
 d = d * fdiff; 
 fdiffr=d*lr, fdiffg=d*lg, fdiffb=d*lb; 
 bdiffr = bdiffg = bdiffb = 0; 
 } else { 
 d = -d * bdiff; 
 bdiffr=d*lr, bdiffg=d*lg, bdiffb=d*lb; 
 fdiffr = fdiffg = fdiffb = 0; 
 } 
}; 
 
 col = &cmap[*(unsigned char *)ifc]; 
 ofc->r = col->r * (CAT(f,diffr) + CAT(f,ambr)) + CAT(f,specr); 
 ofc->g = col->g * (CAT(f,diffg) + CAT(f,ambg)) + CAT(f,specg); 
 ofc->b = col->b * (CAT(f,diffb) + CAT(f,ambb)) + CAT(f,specb);; 
 
 ofc++; if (!fcst) ifc=(Pointer)(((unsigned char *)ifc)+1); 
 if (!ncst) norm ++; 
 } 
 } else if (obc) { 
 if (ncst) norm = &cstnormal; else norm = normals; 
 for (i=0; i < n; i++) { 
 { 
 
 
 nx=norm->x, ny=norm->y, nz=norm->z; 
 
 
 d = _hx*nx + _hy*ny + _hz*nz; 
 if (d>0) { 
 d = fspec * ipow(d, fshine); 
 fspecr=d*lr, fspecg=d*lg, fspecb=d*lb; 
 bspecr = bspecg = bspecb = 0; 
 } else { 
 d = bspec * ipow(-d, bshine); 
 bspecr=d*lr, bspecg=d*lg, bspecb=d*lb; 
 fspecr = fspecg = fspecb = 0; 
 } 
 
 
 d = lx*nx + ly*ny + lz*nz; 
 if (d>0) { 
 d = d * fdiff; 
 fdiffr=d*lr, fdiffg=d*lg, fdiffb=d*lb; 
 bdiffr = bdiffg = bdiffb = 0; 
 } else { 
 d = -d * bdiff; 
 bdiffr=d*lr, bdiffg=d*lg, bdiffb=d*lb; 
 fdiffr = fdiffg = fdiffb = 0; 
 } 
}; 
 
 col = &cmap[*(unsigned char *)ibc]; 
 obc->r = col->r * (CAT(b,diffr) + CAT(b,ambr)) + CAT(b,specr); 
 obc->g = col->g * (CAT(b,diffg) + CAT(b,ambg)) + CAT(b,specg); 
 obc->b = col->b * (CAT(b,diffb) + CAT(b,ambb)) + CAT(b,specb);; 
 
 obc++; if (!bcst) ibc=(Pointer)(((unsigned char *)ibc)+1); 
 if (!ncst) norm ++; 
 } 
 } 
 } 
};
  } else {
      { 
 if (ich) { 
 if (ofc && obc) { 
 if (ncst) norm = &cstnormal; else norm = normals; 
 for (i=0; i < n; i++) { 
 if (DXIsElementValid(ich, i)) { 
 { 
 
 
 nx=norm->x, ny=norm->y, nz=norm->z; 
 
 
 d = _hx*nx + _hy*ny + _hz*nz; 
 if (d>0) { 
 d = fspec * ipow(d, fshine); 
 fspecr=d*lr, fspecg=d*lg, fspecb=d*lb; 
 bspecr = bspecg = bspecb = 0; 
 } else { 
 d = bspec * ipow(-d, bshine); 
 bspecr=d*lr, bspecg=d*lg, bspecb=d*lb; 
 fspecr = fspecg = fspecb = 0; 
 } 
 
 
 d = lx*nx + ly*ny + lz*nz; 
 if (d>0) { 
 d = d * fdiff; 
 fdiffr=d*lr, fdiffg=d*lg, fdiffb=d*lb; 
 bdiffr = bdiffg = bdiffb = 0; 
 } else { 
 d = -d * bdiff; 
 bdiffr=d*lr, bdiffg=d*lg, bdiffb=d*lb; 
 fdiffr = fdiffg = fdiffb = 0; 
 } 
}; 
 
 col = &cmap[*(unsigned char *)ifc]; 
 ofc->r += col->r * CAT(f,diffr) + CAT(f,specr); 
 ofc->g += col->g * CAT(f,diffg) + CAT(f,specg); 
 ofc->b += col->b * CAT(f,diffb) + CAT(f,specb);; 
 
 col = &cmap[*(unsigned char *)ibc]; 
 obc->r += col->r * CAT(b,diffr) + CAT(b,specr); 
 obc->g += col->g * CAT(b,diffg) + CAT(b,specg); 
 obc->b += col->b * CAT(b,diffb) + CAT(b,specb);; 
 } 
 
 ofc++; if (!fcst) ifc=(Pointer)(((unsigned char *)ifc)+1); 
 
 obc++; if (!bcst) ibc=(Pointer)(((unsigned char *)ibc)+1); 
 if (!ncst) norm ++; 
 } 
 } else if (ofc) { 
 if (ncst) norm = &cstnormal; else norm = normals; 
 for (i=0; i < n; i++) { 
 if (DXIsElementValid(ich, i)) { 
 { 
 
 
 nx=norm->x, ny=norm->y, nz=norm->z; 
 
 
 d = _hx*nx + _hy*ny + _hz*nz; 
 if (d>0) { 
 d = fspec * ipow(d, fshine); 
 fspecr=d*lr, fspecg=d*lg, fspecb=d*lb; 
 bspecr = bspecg = bspecb = 0; 
 } else { 
 d = bspec * ipow(-d, bshine); 
 bspecr=d*lr, bspecg=d*lg, bspecb=d*lb; 
 fspecr = fspecg = fspecb = 0; 
 } 
 
 
 d = lx*nx + ly*ny + lz*nz; 
 if (d>0) { 
 d = d * fdiff; 
 fdiffr=d*lr, fdiffg=d*lg, fdiffb=d*lb; 
 bdiffr = bdiffg = bdiffb = 0; 
 } else { 
 d = -d * bdiff; 
 bdiffr=d*lr, bdiffg=d*lg, bdiffb=d*lb; 
 fdiffr = fdiffg = fdiffb = 0; 
 } 
}; 
 
 col = &cmap[*(unsigned char *)ifc]; 
 ofc->r += col->r * CAT(f,diffr) + CAT(f,specr); 
 ofc->g += col->g * CAT(f,diffg) + CAT(f,specg); 
 ofc->b += col->b * CAT(f,diffb) + CAT(f,specb);; 
 } 
 
 ofc++; if (!fcst) ifc=(Pointer)(((unsigned char *)ifc)+1); 
 if (!ncst) norm ++; 
 } 
 } else if (obc) { 
 if (ncst) norm = &cstnormal; else norm = normals; 
 for (i=0; i < n; i++) { 
 if (DXIsElementValid(ich, i)) { 
 { 
 
 
 nx=norm->x, ny=norm->y, nz=norm->z; 
 
 
 d = _hx*nx + _hy*ny + _hz*nz; 
 if (d>0) { 
 d = fspec * ipow(d, fshine); 
 fspecr=d*lr, fspecg=d*lg, fspecb=d*lb; 
 bspecr = bspecg = bspecb = 0; 
 } else { 
 d = bspec * ipow(-d, bshine); 
 bspecr=d*lr, bspecg=d*lg, bspecb=d*lb; 
 fspecr = fspecg = fspecb = 0; 
 } 
 
 
 d = lx*nx + ly*ny + lz*nz; 
 if (d>0) { 
 d = d * fdiff; 
 fdiffr=d*lr, fdiffg=d*lg, fdiffb=d*lb; 
 bdiffr = bdiffg = bdiffb = 0; 
 } else { 
 d = -d * bdiff; 
 bdiffr=d*lr, bdiffg=d*lg, bdiffb=d*lb; 
 fdiffr = fdiffg = fdiffb = 0; 
 } 
}; 
 
 col = &cmap[*(unsigned char *)ibc]; 
 obc->r += col->r * CAT(b,diffr) + CAT(b,specr); 
 obc->g += col->g * CAT(b,diffg) + CAT(b,specg); 
 obc->b += col->b * CAT(b,diffb) + CAT(b,specb);; 
 } 
 
 obc++; if (!bcst) ibc=(Pointer)(((unsigned char *)ibc)+1); 
 if (!ncst) norm ++; 
 } 
 } 
 } else { 
 if (ofc && obc) { 
 if (ncst) norm = &cstnormal; else norm = normals; 
 for (i=0; i < n; i++) { 
 { 
 
 
 nx=norm->x, ny=norm->y, nz=norm->z; 
 
 
 d = _hx*nx + _hy*ny + _hz*nz; 
 if (d>0) { 
 d = fspec * ipow(d, fshine); 
 fspecr=d*lr, fspecg=d*lg, fspecb=d*lb; 
 bspecr = bspecg = bspecb = 0; 
 } else { 
 d = bspec * ipow(-d, bshine); 
 bspecr=d*lr, bspecg=d*lg, bspecb=d*lb; 
 fspecr = fspecg = fspecb = 0; 
 } 
 
 
 d = lx*nx + ly*ny + lz*nz; 
 if (d>0) { 
 d = d * fdiff; 
 fdiffr=d*lr, fdiffg=d*lg, fdiffb=d*lb; 
 bdiffr = bdiffg = bdiffb = 0; 
 } else { 
 d = -d * bdiff; 
 bdiffr=d*lr, bdiffg=d*lg, bdiffb=d*lb; 
 fdiffr = fdiffg = fdiffb = 0; 
 } 
}; 
 
 col = &cmap[*(unsigned char *)ifc]; 
 ofc->r += col->r * CAT(f,diffr) + CAT(f,specr); 
 ofc->g += col->g * CAT(f,diffg) + CAT(f,specg); 
 ofc->b += col->b * CAT(f,diffb) + CAT(f,specb);; 
 
 col = &cmap[*(unsigned char *)ibc]; 
 obc->r += col->r * CAT(b,diffr) + CAT(b,specr); 
 obc->g += col->g * CAT(b,diffg) + CAT(b,specg); 
 obc->b += col->b * CAT(b,diffb) + CAT(b,specb);; 
 
 ofc++; if (!fcst) ifc=(Pointer)(((unsigned char *)ifc)+1); 
 
 obc++; if (!bcst) ibc=(Pointer)(((unsigned char *)ibc)+1); 
 if (!ncst) norm ++; 
 } 
 } else if (ofc) { 
 if (ncst) norm = &cstnormal; else norm = normals; 
 for (i=0; i < n; i++) { 
 { 
 
 
 nx=norm->x, ny=norm->y, nz=norm->z; 
 
 
 d = _hx*nx + _hy*ny + _hz*nz; 
 if (d>0) { 
 d = fspec * ipow(d, fshine); 
 fspecr=d*lr, fspecg=d*lg, fspecb=d*lb; 
 bspecr = bspecg = bspecb = 0; 
 } else { 
 d = bspec * ipow(-d, bshine); 
 bspecr=d*lr, bspecg=d*lg, bspecb=d*lb; 
 fspecr = fspecg = fspecb = 0; 
 } 
 
 
 d = lx*nx + ly*ny + lz*nz; 
 if (d>0) { 
 d = d * fdiff; 
 fdiffr=d*lr, fdiffg=d*lg, fdiffb=d*lb; 
 bdiffr = bdiffg = bdiffb = 0; 
 } else { 
 d = -d * bdiff; 
 bdiffr=d*lr, bdiffg=d*lg, bdiffb=d*lb; 
 fdiffr = fdiffg = fdiffb = 0; 
 } 
}; 
 
 col = &cmap[*(unsigned char *)ifc]; 
 ofc->r += col->r * CAT(f,diffr) + CAT(f,specr); 
 ofc->g += col->g * CAT(f,diffg) + CAT(f,specg); 
 ofc->b += col->b * CAT(f,diffb) + CAT(f,specb);; 
 
 ofc++; if (!fcst) ifc=(Pointer)(((unsigned char *)ifc)+1); 
 if (!ncst) norm ++; 
 } 
 } else if (obc) { 
 if (ncst) norm = &cstnormal; else norm = normals; 
 for (i=0; i < n; i++) { 
 { 
 
 
 nx=norm->x, ny=norm->y, nz=norm->z; 
 
 
 d = _hx*nx + _hy*ny + _hz*nz; 
 if (d>0) { 
 d = fspec * ipow(d, fshine); 
 fspecr=d*lr, fspecg=d*lg, fspecb=d*lb; 
 bspecr = bspecg = bspecb = 0; 
 } else { 
 d = bspec * ipow(-d, bshine); 
 bspecr=d*lr, bspecg=d*lg, bspecb=d*lb; 
 fspecr = fspecg = fspecb = 0; 
 } 
 
 
 d = lx*nx + ly*ny + lz*nz; 
 if (d>0) { 
 d = d * fdiff; 
 fdiffr=d*lr, fdiffg=d*lg, fdiffb=d*lb; 
 bdiffr = bdiffg = bdiffb = 0; 
 } else { 
 d = -d * bdiff; 
 bdiffr=d*lr, bdiffg=d*lg, bdiffb=d*lb; 
 fdiffr = fdiffg = fdiffb = 0; 
 } 
}; 
 
 col = &cmap[*(unsigned char *)ibc]; 
 obc->r += col->r * CAT(b,diffr) + CAT(b,specr); 
 obc->g += col->g * CAT(b,diffg) + CAT(b,specg); 
 obc->b += col->b * CAT(b,diffb) + CAT(b,specb);; 
 
 obc++; if (!bcst) ibc=(Pointer)(((unsigned char *)ibc)+1); 
 if (!ncst) norm ++; 
 } 
 } 
 } 
};
  }
     } else {
  if (j==0) {
      { 
 if (ich) { 
 if (ofc && obc) { 
 if (ncst) norm = &cstnormal; else norm = normals; 
 for (i=0; i < n; i++) { 
 if (DXIsElementValid(ich, i)) { 
 { 
 
 
 nx=norm->x, ny=norm->y, nz=norm->z; 
 
 
 d = _hx*nx + _hy*ny + _hz*nz; 
 if (d>0) { 
 d = fspec * ipow(d, fshine); 
 fspecr=d*lr, fspecg=d*lg, fspecb=d*lb; 
 bspecr = bspecg = bspecb = 0; 
 } else { 
 d = bspec * ipow(-d, bshine); 
 bspecr=d*lr, bspecg=d*lg, bspecb=d*lb; 
 fspecr = fspecg = fspecb = 0; 
 } 
 
 
 d = lx*nx + ly*ny + lz*nz; 
 if (d>0) { 
 d = d * fdiff; 
 fdiffr=d*lr, fdiffg=d*lg, fdiffb=d*lb; 
 bdiffr = bdiffg = bdiffb = 0; 
 } else { 
 d = -d * bdiff; 
 bdiffr=d*lr, bdiffg=d*lg, bdiffb=d*lb; 
 fdiffr = fdiffg = fdiffb = 0; 
 } 
}; 
 
{ 
 if (fbyte) 
 { 
 ofc->r = byteTable[((unsigned char *)ifc)[0]] 
 * (CAT(f,diffr)+CAT(f,ambr)) + CAT(f,specr); 
 ofc->g = byteTable[((unsigned char *)ifc)[1]] 
 * (CAT(f,diffg)+CAT(f,ambg)) + CAT(f,specg); 
 ofc->b = byteTable[((unsigned char *)ifc)[2]] 
 * (CAT(f,diffb)+CAT(f,ambb)) + CAT(f,specb); 
 } else { 
 ofc->r = ((RGBColor *)ifc)->r * (CAT(f,diffr)+CAT(f,ambr)) + CAT(f,specr); 
 ofc->g = ((RGBColor *)ifc)->g * (CAT(f,diffg)+CAT(f,ambg)) + CAT(f,specg); 
 ofc->b = ((RGBColor *)ifc)->b * (CAT(f,diffb)+CAT(f,ambb)) + CAT(f,specb); 
 } 
}; 
 
{ 
 if (bbyte) 
 { 
 obc->r = byteTable[((unsigned char *)ibc)[0]] 
 * (CAT(b,diffr)+CAT(b,ambr)) + CAT(b,specr); 
 obc->g = byteTable[((unsigned char *)ibc)[1]] 
 * (CAT(b,diffg)+CAT(b,ambg)) + CAT(b,specg); 
 obc->b = byteTable[((unsigned char *)ibc)[2]] 
 * (CAT(b,diffb)+CAT(b,ambb)) + CAT(b,specb); 
 } else { 
 obc->r = ((RGBColor *)ibc)->r * (CAT(b,diffr)+CAT(b,ambr)) + CAT(b,specr); 
 obc->g = ((RGBColor *)ibc)->g * (CAT(b,diffg)+CAT(b,ambg)) + CAT(b,specg); 
 obc->b = ((RGBColor *)ibc)->b * (CAT(b,diffb)+CAT(b,ambb)) + CAT(b,specb); 
 } 
}; 
 } 
 
{ 
 if (fbyte) { 
 ofc++; if (!fcst) ifc=(Pointer)(((unsigned char *)ifc)+3); 
 } else { 
 ofc++; if (!fcst) ifc=(Pointer)(((RGBColor *)ifc)+1); 
 } 
}; 
 
{ 
 if (bbyte) { 
 obc++; if (!bcst) ibc=(Pointer)(((unsigned char *)ibc)+3); 
 } else { 
 obc++; if (!bcst) ibc=(Pointer)(((RGBColor *)ibc)+1); 
 } 
}; 
 if (!ncst) norm ++; 
 } 
 } else if (ofc) { 
 if (ncst) norm = &cstnormal; else norm = normals; 
 for (i=0; i < n; i++) { 
 if (DXIsElementValid(ich, i)) { 
 { 
 
 
 nx=norm->x, ny=norm->y, nz=norm->z; 
 
 
 d = _hx*nx + _hy*ny + _hz*nz; 
 if (d>0) { 
 d = fspec * ipow(d, fshine); 
 fspecr=d*lr, fspecg=d*lg, fspecb=d*lb; 
 bspecr = bspecg = bspecb = 0; 
 } else { 
 d = bspec * ipow(-d, bshine); 
 bspecr=d*lr, bspecg=d*lg, bspecb=d*lb; 
 fspecr = fspecg = fspecb = 0; 
 } 
 
 
 d = lx*nx + ly*ny + lz*nz; 
 if (d>0) { 
 d = d * fdiff; 
 fdiffr=d*lr, fdiffg=d*lg, fdiffb=d*lb; 
 bdiffr = bdiffg = bdiffb = 0; 
 } else { 
 d = -d * bdiff; 
 bdiffr=d*lr, bdiffg=d*lg, bdiffb=d*lb; 
 fdiffr = fdiffg = fdiffb = 0; 
 } 
}; 
 
{ 
 if (fbyte) 
 { 
 ofc->r = byteTable[((unsigned char *)ifc)[0]] 
 * (CAT(f,diffr)+CAT(f,ambr)) + CAT(f,specr); 
 ofc->g = byteTable[((unsigned char *)ifc)[1]] 
 * (CAT(f,diffg)+CAT(f,ambg)) + CAT(f,specg); 
 ofc->b = byteTable[((unsigned char *)ifc)[2]] 
 * (CAT(f,diffb)+CAT(f,ambb)) + CAT(f,specb); 
 } else { 
 ofc->r = ((RGBColor *)ifc)->r * (CAT(f,diffr)+CAT(f,ambr)) + CAT(f,specr); 
 ofc->g = ((RGBColor *)ifc)->g * (CAT(f,diffg)+CAT(f,ambg)) + CAT(f,specg); 
 ofc->b = ((RGBColor *)ifc)->b * (CAT(f,diffb)+CAT(f,ambb)) + CAT(f,specb); 
 } 
}; 
 } 
 
{ 
 if (fbyte) { 
 ofc++; if (!fcst) ifc=(Pointer)(((unsigned char *)ifc)+3); 
 } else { 
 ofc++; if (!fcst) ifc=(Pointer)(((RGBColor *)ifc)+1); 
 } 
}; 
 if (!ncst) norm ++; 
 } 
 } else if (obc) { 
 if (ncst) norm = &cstnormal; else norm = normals; 
 for (i=0; i < n; i++) { 
 if (DXIsElementValid(ich, i)) { 
 { 
 
 
 nx=norm->x, ny=norm->y, nz=norm->z; 
 
 
 d = _hx*nx + _hy*ny + _hz*nz; 
 if (d>0) { 
 d = fspec * ipow(d, fshine); 
 fspecr=d*lr, fspecg=d*lg, fspecb=d*lb; 
 bspecr = bspecg = bspecb = 0; 
 } else { 
 d = bspec * ipow(-d, bshine); 
 bspecr=d*lr, bspecg=d*lg, bspecb=d*lb; 
 fspecr = fspecg = fspecb = 0; 
 } 
 
 
 d = lx*nx + ly*ny + lz*nz; 
 if (d>0) { 
 d = d * fdiff; 
 fdiffr=d*lr, fdiffg=d*lg, fdiffb=d*lb; 
 bdiffr = bdiffg = bdiffb = 0; 
 } else { 
 d = -d * bdiff; 
 bdiffr=d*lr, bdiffg=d*lg, bdiffb=d*lb; 
 fdiffr = fdiffg = fdiffb = 0; 
 } 
}; 
 
{ 
 if (bbyte) 
 { 
 obc->r = byteTable[((unsigned char *)ibc)[0]] 
 * (CAT(b,diffr)+CAT(b,ambr)) + CAT(b,specr); 
 obc->g = byteTable[((unsigned char *)ibc)[1]] 
 * (CAT(b,diffg)+CAT(b,ambg)) + CAT(b,specg); 
 obc->b = byteTable[((unsigned char *)ibc)[2]] 
 * (CAT(b,diffb)+CAT(b,ambb)) + CAT(b,specb); 
 } else { 
 obc->r = ((RGBColor *)ibc)->r * (CAT(b,diffr)+CAT(b,ambr)) + CAT(b,specr); 
 obc->g = ((RGBColor *)ibc)->g * (CAT(b,diffg)+CAT(b,ambg)) + CAT(b,specg); 
 obc->b = ((RGBColor *)ibc)->b * (CAT(b,diffb)+CAT(b,ambb)) + CAT(b,specb); 
 } 
}; 
 } 
 
{ 
 if (fbyte) { 
 obc++; if (!bcst) ibc=(Pointer)(((unsigned char *)ibc)+3); 
 } else { 
 obc++; if (!bcst) ibc=(Pointer)(((RGBColor *)ibc)+1); 
 } 
}; 
 if (!ncst) norm ++; 
 } 
 } 
 } else { 
 if (ofc && obc) { 
 if (ncst) norm = &cstnormal; else norm = normals; 
 for (i=0; i < n; i++) { 
 { 
 
 
 nx=norm->x, ny=norm->y, nz=norm->z; 
 
 
 d = _hx*nx + _hy*ny + _hz*nz; 
 if (d>0) { 
 d = fspec * ipow(d, fshine); 
 fspecr=d*lr, fspecg=d*lg, fspecb=d*lb; 
 bspecr = bspecg = bspecb = 0; 
 } else { 
 d = bspec * ipow(-d, bshine); 
 bspecr=d*lr, bspecg=d*lg, bspecb=d*lb; 
 fspecr = fspecg = fspecb = 0; 
 } 
 
 
 d = lx*nx + ly*ny + lz*nz; 
 if (d>0) { 
 d = d * fdiff; 
 fdiffr=d*lr, fdiffg=d*lg, fdiffb=d*lb; 
 bdiffr = bdiffg = bdiffb = 0; 
 } else { 
 d = -d * bdiff; 
 bdiffr=d*lr, bdiffg=d*lg, bdiffb=d*lb; 
 fdiffr = fdiffg = fdiffb = 0; 
 } 
}; 
 
{ 
 if (fbyte) 
 { 
 ofc->r = byteTable[((unsigned char *)ifc)[0]] 
 * (CAT(f,diffr)+CAT(f,ambr)) + CAT(f,specr); 
 ofc->g = byteTable[((unsigned char *)ifc)[1]] 
 * (CAT(f,diffg)+CAT(f,ambg)) + CAT(f,specg); 
 ofc->b = byteTable[((unsigned char *)ifc)[2]] 
 * (CAT(f,diffb)+CAT(f,ambb)) + CAT(f,specb); 
 } else { 
 ofc->r = ((RGBColor *)ifc)->r * (CAT(f,diffr)+CAT(f,ambr)) + CAT(f,specr); 
 ofc->g = ((RGBColor *)ifc)->g * (CAT(f,diffg)+CAT(f,ambg)) + CAT(f,specg); 
 ofc->b = ((RGBColor *)ifc)->b * (CAT(f,diffb)+CAT(f,ambb)) + CAT(f,specb); 
 } 
}; 
 
{ 
 if (bbyte) 
 { 
 obc->r = byteTable[((unsigned char *)ibc)[0]] 
 * (CAT(b,diffr)+CAT(b,ambr)) + CAT(b,specr); 
 obc->g = byteTable[((unsigned char *)ibc)[1]] 
 * (CAT(b,diffg)+CAT(b,ambg)) + CAT(b,specg); 
 obc->b = byteTable[((unsigned char *)ibc)[2]] 
 * (CAT(b,diffb)+CAT(b,ambb)) + CAT(b,specb); 
 } else { 
 obc->r = ((RGBColor *)ibc)->r * (CAT(b,diffr)+CAT(b,ambr)) + CAT(b,specr); 
 obc->g = ((RGBColor *)ibc)->g * (CAT(b,diffg)+CAT(b,ambg)) + CAT(b,specg); 
 obc->b = ((RGBColor *)ibc)->b * (CAT(b,diffb)+CAT(b,ambb)) + CAT(b,specb); 
 } 
}; 
 
{ 
 if (fbyte) { 
 ofc++; if (!fcst) ifc=(Pointer)(((unsigned char *)ifc)+3); 
 } else { 
 ofc++; if (!fcst) ifc=(Pointer)(((RGBColor *)ifc)+1); 
 } 
}; 
 
{ 
 if (bbyte) { 
 obc++; if (!bcst) ibc=(Pointer)(((unsigned char *)ibc)+3); 
 } else { 
 obc++; if (!bcst) ibc=(Pointer)(((RGBColor *)ibc)+1); 
 } 
}; 
 if (!ncst) norm ++; 
 } 
 } else if (ofc) { 
 if (ncst) norm = &cstnormal; else norm = normals; 
 for (i=0; i < n; i++) { 
 { 
 
 
 nx=norm->x, ny=norm->y, nz=norm->z; 
 
 
 d = _hx*nx + _hy*ny + _hz*nz; 
 if (d>0) { 
 d = fspec * ipow(d, fshine); 
 fspecr=d*lr, fspecg=d*lg, fspecb=d*lb; 
 bspecr = bspecg = bspecb = 0; 
 } else { 
 d = bspec * ipow(-d, bshine); 
 bspecr=d*lr, bspecg=d*lg, bspecb=d*lb; 
 fspecr = fspecg = fspecb = 0; 
 } 
 
 
 d = lx*nx + ly*ny + lz*nz; 
 if (d>0) { 
 d = d * fdiff; 
 fdiffr=d*lr, fdiffg=d*lg, fdiffb=d*lb; 
 bdiffr = bdiffg = bdiffb = 0; 
 } else { 
 d = -d * bdiff; 
 bdiffr=d*lr, bdiffg=d*lg, bdiffb=d*lb; 
 fdiffr = fdiffg = fdiffb = 0; 
 } 
}; 
 
{ 
 if (fbyte) 
 { 
 ofc->r = byteTable[((unsigned char *)ifc)[0]] 
 * (CAT(f,diffr)+CAT(f,ambr)) + CAT(f,specr); 
 ofc->g = byteTable[((unsigned char *)ifc)[1]] 
 * (CAT(f,diffg)+CAT(f,ambg)) + CAT(f,specg); 
 ofc->b = byteTable[((unsigned char *)ifc)[2]] 
 * (CAT(f,diffb)+CAT(f,ambb)) + CAT(f,specb); 
 } else { 
 ofc->r = ((RGBColor *)ifc)->r * (CAT(f,diffr)+CAT(f,ambr)) + CAT(f,specr); 
 ofc->g = ((RGBColor *)ifc)->g * (CAT(f,diffg)+CAT(f,ambg)) + CAT(f,specg); 
 ofc->b = ((RGBColor *)ifc)->b * (CAT(f,diffb)+CAT(f,ambb)) + CAT(f,specb); 
 } 
}; 
 
{ 
 if (fbyte) { 
 ofc++; if (!fcst) ifc=(Pointer)(((unsigned char *)ifc)+3); 
 } else { 
 ofc++; if (!fcst) ifc=(Pointer)(((RGBColor *)ifc)+1); 
 } 
}; 
 if (!ncst) norm ++; 
 } 
 } else if (obc) { 
 if (ncst) norm = &cstnormal; else norm = normals; 
 for (i=0; i < n; i++) { 
 { 
 
 
 nx=norm->x, ny=norm->y, nz=norm->z; 
 
 
 d = _hx*nx + _hy*ny + _hz*nz; 
 if (d>0) { 
 d = fspec * ipow(d, fshine); 
 fspecr=d*lr, fspecg=d*lg, fspecb=d*lb; 
 bspecr = bspecg = bspecb = 0; 
 } else { 
 d = bspec * ipow(-d, bshine); 
 bspecr=d*lr, bspecg=d*lg, bspecb=d*lb; 
 fspecr = fspecg = fspecb = 0; 
 } 
 
 
 d = lx*nx + ly*ny + lz*nz; 
 if (d>0) { 
 d = d * fdiff; 
 fdiffr=d*lr, fdiffg=d*lg, fdiffb=d*lb; 
 bdiffr = bdiffg = bdiffb = 0; 
 } else { 
 d = -d * bdiff; 
 bdiffr=d*lr, bdiffg=d*lg, bdiffb=d*lb; 
 fdiffr = fdiffg = fdiffb = 0; 
 } 
}; 
 
{ 
 if (bbyte) 
 { 
 obc->r = byteTable[((unsigned char *)ibc)[0]] 
 * (CAT(b,diffr)+CAT(b,ambr)) + CAT(b,specr); 
 obc->g = byteTable[((unsigned char *)ibc)[1]] 
 * (CAT(b,diffg)+CAT(b,ambg)) + CAT(b,specg); 
 obc->b = byteTable[((unsigned char *)ibc)[2]] 
 * (CAT(b,diffb)+CAT(b,ambb)) + CAT(b,specb); 
 } else { 
 obc->r = ((RGBColor *)ibc)->r * (CAT(b,diffr)+CAT(b,ambr)) + CAT(b,specr); 
 obc->g = ((RGBColor *)ibc)->g * (CAT(b,diffg)+CAT(b,ambg)) + CAT(b,specg); 
 obc->b = ((RGBColor *)ibc)->b * (CAT(b,diffb)+CAT(b,ambb)) + CAT(b,specb); 
 } 
}; 
 
{ 
 if (bbyte) { 
 obc++; if (!bcst) ibc=(Pointer)(((unsigned char *)ibc)+3); 
 } else { 
 obc++; if (!bcst) ibc=(Pointer)(((RGBColor *)ibc)+1); 
 } 
}; 
 if (!ncst) norm ++; 
 } 
 } 
 } 
};
  } else {
      { 
 if (ich) { 
 if (ofc && obc) { 
 if (ncst) norm = &cstnormal; else norm = normals; 
 for (i=0; i < n; i++) { 
 if (DXIsElementValid(ich, i)) { 
 { 
 
 
 nx=norm->x, ny=norm->y, nz=norm->z; 
 
 
 d = _hx*nx + _hy*ny + _hz*nz; 
 if (d>0) { 
 d = fspec * ipow(d, fshine); 
 fspecr=d*lr, fspecg=d*lg, fspecb=d*lb; 
 bspecr = bspecg = bspecb = 0; 
 } else { 
 d = bspec * ipow(-d, bshine); 
 bspecr=d*lr, bspecg=d*lg, bspecb=d*lb; 
 fspecr = fspecg = fspecb = 0; 
 } 
 
 
 d = lx*nx + ly*ny + lz*nz; 
 if (d>0) { 
 d = d * fdiff; 
 fdiffr=d*lr, fdiffg=d*lg, fdiffb=d*lb; 
 bdiffr = bdiffg = bdiffb = 0; 
 } else { 
 d = -d * bdiff; 
 bdiffr=d*lr, bdiffg=d*lg, bdiffb=d*lb; 
 fdiffr = fdiffg = fdiffb = 0; 
 } 
}; 
 
{ 
 if (fbyte) 
 { 
 ofc->r += byteTable[((unsigned char *)ifc)[0]] * CAT(f,diffr) + CAT(f,specr); 
 ofc->g += byteTable[((unsigned char *)ifc)[1]] * CAT(f,diffg) + CAT(f,specg); 
 ofc->b += byteTable[((unsigned char *)ifc)[2]] * CAT(f,diffb) + CAT(f,specb); 
 } else { 
 ofc->r += ((RGBColor *)ifc)->r * CAT(f,diffr) + CAT(f,specr); 
 ofc->g += ((RGBColor *)ifc)->g * CAT(f,diffg) + CAT(f,specg); 
 ofc->b += ((RGBColor *)ifc)->b * CAT(f,diffb) + CAT(f,specb); 
 } 
}; 
 
{ 
 if (bbyte) 
 { 
 obc->r += byteTable[((unsigned char *)ibc)[0]] * CAT(b,diffr) + CAT(b,specr); 
 obc->g += byteTable[((unsigned char *)ibc)[1]] * CAT(b,diffg) + CAT(b,specg); 
 obc->b += byteTable[((unsigned char *)ibc)[2]] * CAT(b,diffb) + CAT(b,specb); 
 } else { 
 obc->r += ((RGBColor *)ibc)->r * CAT(b,diffr) + CAT(b,specr); 
 obc->g += ((RGBColor *)ibc)->g * CAT(b,diffg) + CAT(b,specg); 
 obc->b += ((RGBColor *)ibc)->b * CAT(b,diffb) + CAT(b,specb); 
 } 
}; 
 } 
 
{ 
 if (fbyte) { 
 ofc++; if (!fcst) ifc=(Pointer)(((unsigned char *)ifc)+3); 
 } else { 
 ofc++; if (!fcst) ifc=(Pointer)(((RGBColor *)ifc)+1); 
 } 
}; 
 
{ 
 if (bbyte) { 
 obc++; if (!bcst) ibc=(Pointer)(((unsigned char *)ibc)+3); 
 } else { 
 obc++; if (!bcst) ibc=(Pointer)(((RGBColor *)ibc)+1); 
 } 
}; 
 if (!ncst) norm ++; 
 } 
 } else if (ofc) { 
 if (ncst) norm = &cstnormal; else norm = normals; 
 for (i=0; i < n; i++) { 
 if (DXIsElementValid(ich, i)) { 
 { 
 
 
 nx=norm->x, ny=norm->y, nz=norm->z; 
 
 
 d = _hx*nx + _hy*ny + _hz*nz; 
 if (d>0) { 
 d = fspec * ipow(d, fshine); 
 fspecr=d*lr, fspecg=d*lg, fspecb=d*lb; 
 bspecr = bspecg = bspecb = 0; 
 } else { 
 d = bspec * ipow(-d, bshine); 
 bspecr=d*lr, bspecg=d*lg, bspecb=d*lb; 
 fspecr = fspecg = fspecb = 0; 
 } 
 
 
 d = lx*nx + ly*ny + lz*nz; 
 if (d>0) { 
 d = d * fdiff; 
 fdiffr=d*lr, fdiffg=d*lg, fdiffb=d*lb; 
 bdiffr = bdiffg = bdiffb = 0; 
 } else { 
 d = -d * bdiff; 
 bdiffr=d*lr, bdiffg=d*lg, bdiffb=d*lb; 
 fdiffr = fdiffg = fdiffb = 0; 
 } 
}; 
 
{ 
 if (fbyte) 
 { 
 ofc->r += byteTable[((unsigned char *)ifc)[0]] * CAT(f,diffr) + CAT(f,specr); 
 ofc->g += byteTable[((unsigned char *)ifc)[1]] * CAT(f,diffg) + CAT(f,specg); 
 ofc->b += byteTable[((unsigned char *)ifc)[2]] * CAT(f,diffb) + CAT(f,specb); 
 } else { 
 ofc->r += ((RGBColor *)ifc)->r * CAT(f,diffr) + CAT(f,specr); 
 ofc->g += ((RGBColor *)ifc)->g * CAT(f,diffg) + CAT(f,specg); 
 ofc->b += ((RGBColor *)ifc)->b * CAT(f,diffb) + CAT(f,specb); 
 } 
}; 
 } 
 
{ 
 if (fbyte) { 
 ofc++; if (!fcst) ifc=(Pointer)(((unsigned char *)ifc)+3); 
 } else { 
 ofc++; if (!fcst) ifc=(Pointer)(((RGBColor *)ifc)+1); 
 } 
}; 
 if (!ncst) norm ++; 
 } 
 } else if (obc) { 
 if (ncst) norm = &cstnormal; else norm = normals; 
 for (i=0; i < n; i++) { 
 if (DXIsElementValid(ich, i)) { 
 { 
 
 
 nx=norm->x, ny=norm->y, nz=norm->z; 
 
 
 d = _hx*nx + _hy*ny + _hz*nz; 
 if (d>0) { 
 d = fspec * ipow(d, fshine); 
 fspecr=d*lr, fspecg=d*lg, fspecb=d*lb; 
 bspecr = bspecg = bspecb = 0; 
 } else { 
 d = bspec * ipow(-d, bshine); 
 bspecr=d*lr, bspecg=d*lg, bspecb=d*lb; 
 fspecr = fspecg = fspecb = 0; 
 } 
 
 
 d = lx*nx + ly*ny + lz*nz; 
 if (d>0) { 
 d = d * fdiff; 
 fdiffr=d*lr, fdiffg=d*lg, fdiffb=d*lb; 
 bdiffr = bdiffg = bdiffb = 0; 
 } else { 
 d = -d * bdiff; 
 bdiffr=d*lr, bdiffg=d*lg, bdiffb=d*lb; 
 fdiffr = fdiffg = fdiffb = 0; 
 } 
}; 
 
{ 
 if (bbyte) 
 { 
 obc->r += byteTable[((unsigned char *)ibc)[0]] * CAT(b,diffr) + CAT(b,specr); 
 obc->g += byteTable[((unsigned char *)ibc)[1]] * CAT(b,diffg) + CAT(b,specg); 
 obc->b += byteTable[((unsigned char *)ibc)[2]] * CAT(b,diffb) + CAT(b,specb); 
 } else { 
 obc->r += ((RGBColor *)ibc)->r * CAT(b,diffr) + CAT(b,specr); 
 obc->g += ((RGBColor *)ibc)->g * CAT(b,diffg) + CAT(b,specg); 
 obc->b += ((RGBColor *)ibc)->b * CAT(b,diffb) + CAT(b,specb); 
 } 
}; 
 } 
 
{ 
 if (fbyte) { 
 obc++; if (!bcst) ibc=(Pointer)(((unsigned char *)ibc)+3); 
 } else { 
 obc++; if (!bcst) ibc=(Pointer)(((RGBColor *)ibc)+1); 
 } 
}; 
 if (!ncst) norm ++; 
 } 
 } 
 } else { 
 if (ofc && obc) { 
 if (ncst) norm = &cstnormal; else norm = normals; 
 for (i=0; i < n; i++) { 
 { 
 
 
 nx=norm->x, ny=norm->y, nz=norm->z; 
 
 
 d = _hx*nx + _hy*ny + _hz*nz; 
 if (d>0) { 
 d = fspec * ipow(d, fshine); 
 fspecr=d*lr, fspecg=d*lg, fspecb=d*lb; 
 bspecr = bspecg = bspecb = 0; 
 } else { 
 d = bspec * ipow(-d, bshine); 
 bspecr=d*lr, bspecg=d*lg, bspecb=d*lb; 
 fspecr = fspecg = fspecb = 0; 
 } 
 
 
 d = lx*nx + ly*ny + lz*nz; 
 if (d>0) { 
 d = d * fdiff; 
 fdiffr=d*lr, fdiffg=d*lg, fdiffb=d*lb; 
 bdiffr = bdiffg = bdiffb = 0; 
 } else { 
 d = -d * bdiff; 
 bdiffr=d*lr, bdiffg=d*lg, bdiffb=d*lb; 
 fdiffr = fdiffg = fdiffb = 0; 
 } 
}; 
 
{ 
 if (fbyte) 
 { 
 ofc->r += byteTable[((unsigned char *)ifc)[0]] * CAT(f,diffr) + CAT(f,specr); 
 ofc->g += byteTable[((unsigned char *)ifc)[1]] * CAT(f,diffg) + CAT(f,specg); 
 ofc->b += byteTable[((unsigned char *)ifc)[2]] * CAT(f,diffb) + CAT(f,specb); 
 } else { 
 ofc->r += ((RGBColor *)ifc)->r * CAT(f,diffr) + CAT(f,specr); 
 ofc->g += ((RGBColor *)ifc)->g * CAT(f,diffg) + CAT(f,specg); 
 ofc->b += ((RGBColor *)ifc)->b * CAT(f,diffb) + CAT(f,specb); 
 } 
}; 
 
{ 
 if (bbyte) 
 { 
 obc->r += byteTable[((unsigned char *)ibc)[0]] * CAT(b,diffr) + CAT(b,specr); 
 obc->g += byteTable[((unsigned char *)ibc)[1]] * CAT(b,diffg) + CAT(b,specg); 
 obc->b += byteTable[((unsigned char *)ibc)[2]] * CAT(b,diffb) + CAT(b,specb); 
 } else { 
 obc->r += ((RGBColor *)ibc)->r * CAT(b,diffr) + CAT(b,specr); 
 obc->g += ((RGBColor *)ibc)->g * CAT(b,diffg) + CAT(b,specg); 
 obc->b += ((RGBColor *)ibc)->b * CAT(b,diffb) + CAT(b,specb); 
 } 
}; 
 
{ 
 if (fbyte) { 
 ofc++; if (!fcst) ifc=(Pointer)(((unsigned char *)ifc)+3); 
 } else { 
 ofc++; if (!fcst) ifc=(Pointer)(((RGBColor *)ifc)+1); 
 } 
}; 
 
{ 
 if (bbyte) { 
 obc++; if (!bcst) ibc=(Pointer)(((unsigned char *)ibc)+3); 
 } else { 
 obc++; if (!bcst) ibc=(Pointer)(((RGBColor *)ibc)+1); 
 } 
}; 
 if (!ncst) norm ++; 
 } 
 } else if (ofc) { 
 if (ncst) norm = &cstnormal; else norm = normals; 
 for (i=0; i < n; i++) { 
 { 
 
 
 nx=norm->x, ny=norm->y, nz=norm->z; 
 
 
 d = _hx*nx + _hy*ny + _hz*nz; 
 if (d>0) { 
 d = fspec * ipow(d, fshine); 
 fspecr=d*lr, fspecg=d*lg, fspecb=d*lb; 
 bspecr = bspecg = bspecb = 0; 
 } else { 
 d = bspec * ipow(-d, bshine); 
 bspecr=d*lr, bspecg=d*lg, bspecb=d*lb; 
 fspecr = fspecg = fspecb = 0; 
 } 
 
 
 d = lx*nx + ly*ny + lz*nz; 
 if (d>0) { 
 d = d * fdiff; 
 fdiffr=d*lr, fdiffg=d*lg, fdiffb=d*lb; 
 bdiffr = bdiffg = bdiffb = 0; 
 } else { 
 d = -d * bdiff; 
 bdiffr=d*lr, bdiffg=d*lg, bdiffb=d*lb; 
 fdiffr = fdiffg = fdiffb = 0; 
 } 
}; 
 
{ 
 if (fbyte) 
 { 
 ofc->r += byteTable[((unsigned char *)ifc)[0]] * CAT(f,diffr) + CAT(f,specr); 
 ofc->g += byteTable[((unsigned char *)ifc)[1]] * CAT(f,diffg) + CAT(f,specg); 
 ofc->b += byteTable[((unsigned char *)ifc)[2]] * CAT(f,diffb) + CAT(f,specb); 
 } else { 
 ofc->r += ((RGBColor *)ifc)->r * CAT(f,diffr) + CAT(f,specr); 
 ofc->g += ((RGBColor *)ifc)->g * CAT(f,diffg) + CAT(f,specg); 
 ofc->b += ((RGBColor *)ifc)->b * CAT(f,diffb) + CAT(f,specb); 
 } 
}; 
 
{ 
 if (fbyte) { 
 ofc++; if (!fcst) ifc=(Pointer)(((unsigned char *)ifc)+3); 
 } else { 
 ofc++; if (!fcst) ifc=(Pointer)(((RGBColor *)ifc)+1); 
 } 
}; 
 if (!ncst) norm ++; 
 } 
 } else if (obc) { 
 if (ncst) norm = &cstnormal; else norm = normals; 
 for (i=0; i < n; i++) { 
 { 
 
 
 nx=norm->x, ny=norm->y, nz=norm->z; 
 
 
 d = _hx*nx + _hy*ny + _hz*nz; 
 if (d>0) { 
 d = fspec * ipow(d, fshine); 
 fspecr=d*lr, fspecg=d*lg, fspecb=d*lb; 
 bspecr = bspecg = bspecb = 0; 
 } else { 
 d = bspec * ipow(-d, bshine); 
 bspecr=d*lr, bspecg=d*lg, bspecb=d*lb; 
 fspecr = fspecg = fspecb = 0; 
 } 
 
 
 d = lx*nx + ly*ny + lz*nz; 
 if (d>0) { 
 d = d * fdiff; 
 fdiffr=d*lr, fdiffg=d*lg, fdiffb=d*lb; 
 bdiffr = bdiffg = bdiffb = 0; 
 } else { 
 d = -d * bdiff; 
 bdiffr=d*lr, bdiffg=d*lg, bdiffb=d*lb; 
 fdiffr = fdiffg = fdiffb = 0; 
 } 
}; 
 
{ 
 if (bbyte) 
 { 
 obc->r += byteTable[((unsigned char *)ibc)[0]] * CAT(b,diffr) + CAT(b,specr); 
 obc->g += byteTable[((unsigned char *)ibc)[1]] * CAT(b,diffg) + CAT(b,specg); 
 obc->b += byteTable[((unsigned char *)ibc)[2]] * CAT(b,diffb) + CAT(b,specb); 
 } else { 
 obc->r += ((RGBColor *)ibc)->r * CAT(b,diffr) + CAT(b,specr); 
 obc->g += ((RGBColor *)ibc)->g * CAT(b,diffg) + CAT(b,specg); 
 obc->b += ((RGBColor *)ibc)->b * CAT(b,diffb) + CAT(b,specb); 
 } 
}; 
 
{ 
 if (bbyte) { 
 obc++; if (!bcst) ibc=(Pointer)(((unsigned char *)ibc)+3); 
 } else { 
 obc++; if (!bcst) ibc=(Pointer)(((RGBColor *)ibc)+1); 
 } 
}; 
 if (!ncst) norm ++; 
 } 
 } 
 } 
};
  }
     }
 }
 else
     DXErrorReturn(ERROR_BAD_PARAMETER, "bad light type");
    }
    if (ich)
    {
 if (fcolors) {
     for (i=0, ofc=fcolors; i<n; i++) {
  if (DXIsElementValid(ich, i)) {
      ((ofc->r) = ((ofc->r) > 1.0) ? 1.0 : ((ofc->r) < 0.0) ? 0.0 : (ofc->r));
      ((ofc->g) = ((ofc->g) > 1.0) ? 1.0 : ((ofc->g) < 0.0) ? 0.0 : (ofc->g));
      ((ofc->b) = ((ofc->b) > 1.0) ? 1.0 : ((ofc->b) < 0.0) ? 0.0 : (ofc->b));
  }
  else
      ofc->r = ofc->g = ofc->b = 0.0;
  ofc++;
     }
 }
 if (bcolors) {
     for (i=0, obc=bcolors; i<n; i++) {
  if (DXIsElementValid(ich, i)) {
      ((obc->r) = ((obc->r) > 1.0) ? 1.0 : ((obc->r) < 0.0) ? 0.0 : (obc->r));
      ((obc->g) = ((obc->g) > 1.0) ? 1.0 : ((obc->g) < 0.0) ? 0.0 : (obc->g));
      ((obc->b) = ((obc->b) > 1.0) ? 1.0 : ((obc->b) < 0.0) ? 0.0 : (obc->b));
  }
  else
      obc->r = obc->g = obc->b = 0.0;
  obc++;
     }
 }
    } else {
 if (fcolors) {
     for (i=0, ofc=fcolors; i<n; i++) {
  ((ofc->r) = ((ofc->r) > 1.0) ? 1.0 : ((ofc->r) < 0.0) ? 0.0 : (ofc->r));
  ((ofc->g) = ((ofc->g) > 1.0) ? 1.0 : ((ofc->g) < 0.0) ? 0.0 : (ofc->g));
  ((ofc->b) = ((ofc->b) > 1.0) ? 1.0 : ((ofc->b) < 0.0) ? 0.0 : (ofc->b));
  ofc++;
     }
 }
 if (bcolors) {
     for (i=0, obc=bcolors; i<n; i++) {
  ((obc->r) = ((obc->r) > 1.0) ? 1.0 : ((obc->r) < 0.0) ? 0.0 : (obc->r));
  ((obc->g) = ((obc->g) > 1.0) ? 1.0 : ((obc->g) < 0.0) ? 0.0 : (obc->g));
  ((obc->b) = ((obc->b) > 1.0) ? 1.0 : ((obc->b) < 0.0) ? 0.0 : (obc->b));
  obc++;
     }
 }
    }
    return OK;
error:
    DXDelete((Object)*front_colors);
    DXDelete((Object)*back_colors);
    return ERROR;
}
static Error
_GatherLights(LightList lightlist, Object obj, Matrix *stack, Camera cam)
{
    Class class = DXGetObjectClass(obj);
    if (class == CLASS_GROUP)
 class = DXGetGroupClass((Group)obj);
    switch (class)
    {
 case CLASS_LIGHT:
 {
     Light l = (Light)obj;
     if (l->kind == ambient)
     {
  lightlist->ambient.r += l->color.r;
  lightlist->ambient.g += l->color.g;
  lightlist->ambient.b += l->color.b;
  lightlist->na++;
     }
     else if (l->kind == distant)
     {
  if (l->relative == world)
  {
      if (stack)
      {
   Matrix m = *stack;
   m.b[0] = m.b[1] = m.b[2] = 0;
   lightlist->lights[lightlist->nl]
       = DXNewDistantLight(DXApply(l->direction,m),
        l->color);
      }
      else
   lightlist->lights[lightlist->nl] = l;
      DXReference((Object)lightlist->lights[lightlist->nl]);
  }
  else if (l->relative == camera)
  {
      Vector from, to, x, y, z, where;
      DXGetView(cam, &from, &to, &y);
      z = DXNormalize(DXSub(from,to));
      y = DXNormalize(y);
      x = DXCross(y,z);
      where = DXMul(x, l->direction.x);
      where = DXAdd(where, DXMul(y, l->direction.y));
      where = DXAdd(where, DXMul(z, l->direction.z));
      lightlist->lights[lightlist->nl]
       = DXNewDistantLight(where, l->color);
      DXReference((Object)lightlist->lights[lightlist->nl]);
  }
  else
  {
      DXErrorReturn(ERROR_INTERNAL, "invalid light object");
  }
  lightlist->nl++;
     }
     else
     {
  DXErrorReturn(ERROR_DATA_INVALID,
   "only ambient and distant lights allowed");
     }
     return OK;
 }
 case CLASS_XFORM:
 {
    Xform x = (Xform)obj;
    Object child;
    Matrix m;
    if (! DXGetXformInfo(x, &child, &m))
        return ERROR;
     if (stack)
     {
  Matrix product;
  product = DXConcatenate(m, *stack);
  return _GatherLights(lightlist, child, &product, cam);
     }
     else
  return _GatherLights(lightlist, child, &m, cam);
 }
 case CLASS_CLIPPED:
 {
    Clipped c = (Clipped)obj;
    Object child;
    if (! DXGetClippedInfo(c, &child, NULL))
        return ERROR;
     return _GatherLights(lightlist, child, stack, cam);
 }
 case CLASS_SERIES:
 case CLASS_GROUP:
 {
     Group g = (Group)obj;
     Object child;
     int i = 0;
     while (NULL != (child = DXGetEnumeratedMember(g, i++, NULL)))
  if (! _GatherLights(lightlist, child, stack, cam))
      return ERROR;
     return OK;
 }
 default:
     return OK;
    }
}
static Error
_dxfFreeLightListPrivate(Pointer p)
{
    _dxfFreeLightList((LightList)p);
    return OK;
}
Private
_dxfGetLights(Object obj, Camera cam)
{
    int i;
    Private p;
    Point from, to, up, eye;
    LightList lightlist = (LightList)DXAllocateZero(sizeof(struct lightList));
    if (! lightlist)
 return NULL;
    lightlist->na = lightlist->nl = 0;
    if (! _GatherLights(lightlist, obj, NULL, cam))
 goto error;
    DXGetView(cam, &from, &to, &up);
    eye = DXNormalize(DXSub(from, to));
    if (lightlist->na == 0 && lightlist->nl == 0)
    {
 Vector left;
 Vector dir;
 left = DXCross(eye,up);
 dir = DXAdd(eye,left);
 lightlist->lights[0] = DXNewDistantLight(dir, DXRGB(1,1,1));
 if (!lightlist->lights[0])
     return NULL;
 lightlist->nl ++;
    }
    if (lightlist->na == 0)
 lightlist->ambient = DXRGB(.2, .2, .2);
    for (i = 0; i < lightlist->nl; i++)
    {
 Light l = lightlist->lights[i];
 float _hx = eye.x+l->direction.x;
 float _hy = eye.y+l->direction.y;
 float _hz = eye.z+l->direction.z;
 float d = _hx*_hx + _hy*_hy + _hz*_hz;
 if (d != 0.0)
     d = 1.0 / sqrt(d);
 lightlist->h[i] = DXVec(_hx*d, _hy*d, _hz*d);
    }
    p = DXNewPrivate((Pointer)lightlist, _dxfFreeLightListPrivate);
    if (! p)
 goto error;
    return p;
error:
    _dxfFreeLightList(lightlist);
    return NULL;
}
Error
_dxfFreeLightList(LightList lightlist)
{
    if (lightlist)
    {
 int i;
 for (i = 0; i < lightlist->nl; i++)
     DXDelete((Object)(lightlist->lights[i]));
 DXFree((Pointer)lightlist);
    }
    return OK;
}
static float _kdf, _ksf;
static int _kspf;
static float _kdb, _ksb;
static int _kspb;
static Pointer _fc, _bc;
static Vector *_normals;
static RGBColor *_map;
static LightList _lights;
static dependency _cdep, _ndep;
static RGBColor *_fbuf;
static RGBColor *_bbuf;
static int _vPerE;
static char _fcst, _bcst, _ncst;
static char _fbyte, _bbyte;
Error
_dxfInitApplyLights(float kaf, float kdf, float ksf, int kspf,
      float kab, float kdb, float ksb, int kspb,
      Pointer fc, Pointer bc, RGBColor *map,
      Vector *normals, LightList lights,
      dependency cdep, dependency ndep,
      RGBColor *fbuf, RGBColor *bbuf, int vPerE,
      char fcst, char bcst, char ncst,
      char fbyte, char bbyte)
{
    _kdf = kdf; _ksf = ksf; _kspf = kspf;
    _kdb = kdb; _ksb = ksb; _kspb = kspb;
    _map = map; _lights = lights;
    _normals = normals; _cdep = cdep; _ndep = ndep;
    _fbuf = fbuf; _bbuf = bbuf; _vPerE = vPerE;
    _fcst = fcst; _bcst = bcst, _ncst = ncst;
    _fbyte = fbyte; _bbyte = bbyte;
    _fc = fc;
    _bc = bc;
    if (_fbyte || _bbyte)
 _dxf_initUbyteToFloat();
    return OK;
}
struct lighting
{
    float frspec; float fgspec; float fbspec;
    float frdiff; float fgdiff; float fbdiff;
    float brspec; float bgspec; float bbspec;
    float brdiff; float bgdiff; float bbdiff;
};
Error
_dxfApplyLights(int *vertices, int *indices, int knt)
{
    int i, j, k, nknt, rknt, *iPtr;
    struct lighting *lighting, _lighting[4], *l;
    RGBColor *f=NULL, *b=NULL;
    Vector *normal = NULL;
    RGBColor fbuf, bbuf;
    rknt = _vPerE * knt;
    if (_ndep == dep_positions)
        nknt = rknt;
    else
 nknt = knt;
    if (nknt > 4)
    {
 lighting = (struct lighting *)DXAllocate(nknt*sizeof(struct lighting));
 if (! lighting)
     goto error;
    }
    else lighting = _lighting;
    if (_ndep == dep_positions)
 iPtr = vertices;
    else
 iPtr = indices;
    memset((char *)lighting, 0, nknt*sizeof(struct lighting));
    if (_ncst)
 normal = _normals;
    for (j = 0; j < nknt; j++)
    {
 if (!_ncst)
     normal = _normals + *iPtr++;
 for (i = 0; i < _lights->nl; i++)
 {
     Light l = _lights->lights[i];
     Vector *h = _lights->h + i;
     float d = normal->x*h->x + normal->y*h->y + normal->z*h->z;
     if (d > 0)
     {
  d = _ksf * ipow(d, _kspf);
  lighting[j].frspec += d * l->color.r;
  lighting[j].fgspec += d * l->color.g;
  lighting[j].fbspec += d * l->color.b;
     }
     else
     {
  d = -d;
  d = _ksb * ipow(d, _kspb);
  lighting[j].brspec += d * l->color.r;
  lighting[j].bgspec += d * l->color.g;
  lighting[j].bbspec += d * l->color.b;
     }
     d = l->direction.x*normal->x +
  l->direction.y*normal->y +
  l->direction.z*normal->z;
     if (d > 0)
     {
  d *= _kdf;
  lighting[j].frdiff += d * l->color.r;
  lighting[j].fgdiff += d * l->color.g;
  lighting[j].fbdiff += d * l->color.b;
     }
     else
     {
  d = -d;
  d *= _kdb;
  lighting[j].brdiff += d * l->color.r;
  lighting[j].bgdiff += d * l->color.g;
  lighting[j].bbdiff += d * l->color.b;
     }
     ((lighting[j].frspec) = ((lighting[j].frspec) > 1.0) ? 1.0 : ((lighting[j].frspec) < 0.0) ? 0.0 : (lighting[j].frspec));
     ((lighting[j].fgspec) = ((lighting[j].fgspec) > 1.0) ? 1.0 : ((lighting[j].fgspec) < 0.0) ? 0.0 : (lighting[j].fgspec));
     ((lighting[j].fbspec) = ((lighting[j].fbspec) > 1.0) ? 1.0 : ((lighting[j].fbspec) < 0.0) ? 0.0 : (lighting[j].fbspec));
     ((lighting[j].frdiff) = ((lighting[j].frdiff) > 1.0) ? 1.0 : ((lighting[j].frdiff) < 0.0) ? 0.0 : (lighting[j].frdiff));
     ((lighting[j].fgdiff) = ((lighting[j].fgdiff) > 1.0) ? 1.0 : ((lighting[j].fgdiff) < 0.0) ? 0.0 : (lighting[j].fgdiff));
     ((lighting[j].fbdiff) = ((lighting[j].fbdiff) > 1.0) ? 1.0 : ((lighting[j].fbdiff) < 0.0) ? 0.0 : (lighting[j].fbdiff));
     ((lighting[j].brspec) = ((lighting[j].brspec) > 1.0) ? 1.0 : ((lighting[j].brspec) < 0.0) ? 0.0 : (lighting[j].brspec));
     ((lighting[j].bgspec) = ((lighting[j].bgspec) > 1.0) ? 1.0 : ((lighting[j].bgspec) < 0.0) ? 0.0 : (lighting[j].bgspec));
     ((lighting[j].bbspec) = ((lighting[j].bbspec) > 1.0) ? 1.0 : ((lighting[j].bbspec) < 0.0) ? 0.0 : (lighting[j].bbspec));
     ((lighting[j].brdiff) = ((lighting[j].brdiff) > 1.0) ? 1.0 : ((lighting[j].brdiff) < 0.0) ? 0.0 : (lighting[j].brdiff));
     ((lighting[j].bgdiff) = ((lighting[j].bgdiff) > 1.0) ? 1.0 : ((lighting[j].bgdiff) < 0.0) ? 0.0 : (lighting[j].bgdiff));
     ((lighting[j].bbdiff) = ((lighting[j].bbdiff) > 1.0) ? 1.0 : ((lighting[j].bbdiff) < 0.0) ? 0.0 : (lighting[j].bbdiff));
 }
    }
    if (_cdep == dep_positions)
 iPtr = vertices;
    else
 iPtr = indices;
    if (_fcst) {
        if (_map)
            f = _map;
        else if (_fbyte)
        {
            unsigned char *ptr = (unsigned char *)_fc;
            fbuf.r = _dxd_ubyteToFloat[*ptr++];
            fbuf.g = _dxd_ubyteToFloat[*ptr++];
            fbuf.b = _dxd_ubyteToFloat[*ptr++];
            f = &fbuf;
        }
        else
            f = _fc;
    }
    if (_bcst) {
        if (_map)
            b = _map;
        else if (_bbyte)
        {
            unsigned char *ptr = (unsigned char *)_bc;
            bbuf.r = _dxd_ubyteToFloat[*ptr++];
            bbuf.g = _dxd_ubyteToFloat[*ptr++];
            bbuf.b = _dxd_ubyteToFloat[*ptr++];
            b = &bbuf;
        }
        else
            b = _bc;
    }
    l = lighting;
    for (i = k = 0; i < knt; i++)
    {
 for (j = 0; j < _vPerE; j++, k++)
 {
     int ci = *iPtr;
     if (_map)
     {
  if (! _fcst)
      f = _map + ((unsigned char *)_fc)[ci];
  if (! _bcst)
      b = _map + ((unsigned char *)_bc)[ci];
     }
     else
     {
  if (! _fcst)
  {
      if (_fbyte)
      {
   unsigned char *ptr = ((unsigned char *)_fc) + ci*3;
   fbuf.r = _dxd_ubyteToFloat[*ptr++];
   fbuf.g = _dxd_ubyteToFloat[*ptr++];
   fbuf.b = _dxd_ubyteToFloat[*ptr++];
   f = &fbuf;
      }
      else
   f = ((RGBColor *)_fc) + ci;
  }
  if (! _bcst)
  {
      if (_bbyte)
      {
   unsigned char *ptr = ((unsigned char *)_bc) + ci*3;
   bbuf.r = _dxd_ubyteToFloat[*ptr++];
   bbuf.g = _dxd_ubyteToFloat[*ptr++];
   bbuf.b = _dxd_ubyteToFloat[*ptr++];
   b = &bbuf;
      }
      else
   b = ((RGBColor *)_bc) + ci;
  }
     }
     if (_fbuf)
     {
  _fbuf[k].r = f->r*(_lights->ambient.r + l->frdiff) + l->frspec;
  _fbuf[k].g = f->g*(_lights->ambient.g + l->fgdiff) + l->fgspec;
  _fbuf[k].b = f->b*(_lights->ambient.b + l->fbdiff) + l->fbspec;
     }
     if (_bbuf)
     {
  _bbuf[k].r = b->r*(_lights->ambient.r + l->brdiff) + l->brspec;
  _bbuf[k].g = b->g*(_lights->ambient.g + l->bgdiff) + l->bgspec;
  _bbuf[k].b = b->b*(_lights->ambient.b + l->bbdiff) + l->bbspec;
     }
     if (_cdep == dep_positions) iPtr++;
     if (_ndep == dep_positions) l++;
 }
 if (_cdep == dep_connections) iPtr++;
 if (_ndep == dep_connections) l++;
    }
    if (lighting != _lighting)
 DXFree(lighting);
    return OK;
error:
    if (lighting != _lighting)
 DXFree(lighting);
    return ERROR;
}
static int _dxd_firstUbyteToFloat = 1;
float _dxd_ubyteToFloat[256] = {0};
void
_dxf_initUbyteToFloat()
{
    int i;
    if (_dxd_firstUbyteToFloat)
    {
 _dxd_firstUbyteToFloat = 0;
 for (i = 0; i < 256; i++)
     _dxd_ubyteToFloat[i] = i / 256.0;
    }
}
