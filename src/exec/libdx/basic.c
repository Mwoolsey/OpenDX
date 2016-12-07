/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>



#include <dx/dx.h>

#if INLINE
#    define STATIC static
#    if paxc
/*#         pragma inline (DXPt,DXVec,DXRGB,DXLn,DXTri,DXQuad,DXTetra,DXMat)*/
#    endif
#else
#    define STATIC
#endif

STATIC Point DXPt(double x, double y, double z)
{
    Point p;
    p.x = x;  p.y = y;  p.z = z;
    return p;
}

STATIC Vector DXVec(double x, double y, double z)
{
    Vector v;
    v.x = x;  v.y = y;  v.z = z;
    return v;
}

STATIC RGBColor DXRGB(double r, double g, double b)
{
    RGBColor rgb;
    rgb.r = r;  rgb.g = g;  rgb.b = b;
    return rgb;
}

STATIC Line DXLn(PointId p, PointId q)
{
    Line ln;
    ln.p = p; ln.q = q;
    return ln;
}

STATIC Triangle DXTri(PointId p, PointId q, PointId r)
{
    Triangle tri;
    tri.p = p; tri.q = q; tri.r = r;
    return tri;
}

STATIC Quadrilateral DXQuad(PointId p, PointId q, PointId r, PointId s)
{
    Quadrilateral quad;
    quad.p = p; quad.q = q; quad.r = r; quad.s = s;
    return quad;
}

STATIC Tetrahedron DXTetra(PointId p, PointId q, PointId r, PointId s)
{
    Tetrahedron tetra;
    tetra.p = p; tetra.q = q; tetra.r = r; tetra.s = s;
    return tetra;
}

STATIC Matrix DXMat(
    double a, double b, double c,
    double d, double e, double f,
    double g, double h, double i,
    double j, double k, double l
) {
    Matrix t;
    t.A[0][0] = a;  t.A[0][1] = b;  t.A[0][2] = c;
    t.A[1][0] = d;  t.A[1][1] = e;  t.A[1][2] = f;
    t.A[2][0] = g;  t.A[2][1] = h;  t.A[2][2] = i;
    t.b[0] = j;     t.b[1] = k;     t.b[2] = l;
    return t;
}

#undef STATIC
