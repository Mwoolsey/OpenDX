/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>



#include <math.h>

/*
 * NB - not meant to be compiled directly
 * #define Type, c1,..., Neg3,... and then #include this file     
 */

#if INLINE
#    define STATIC static
#    if paxc
/*#        pragma inline (Neg,Add,Sub,Mul,Div,Min,Max,Lerp)*/
/*#        pragma inline (Dot,Cross,Length,Normalize)*/
/*#        pragma inline (Apply,Transpose,Invert,AdjointTranspose)*/
#    endif
#else
#    define STATIC
#endif

#ifdef Neg3
STATIC Point3 Neg3(Point3 v)
{
    v.c1 = -v.c1;
    v.c2 = -v.c2;
    v.c3 = -v.c3;
    return v;
}
#endif

#ifdef Add3
STATIC Point3 Add3(Point3 v, Point3 w)
{
    v.c1 += w.c1;
    v.c2 += w.c2;
    v.c3 += w.c3;
    return v;
}
#endif

#ifdef Sub3
STATIC Point3 Sub3(Point3 v, Point3 w)
{
    v.c1 -= w.c1;
    v.c2 -= w.c2;
    v.c3 -= w.c3;
    return v;
}
#endif

#ifdef Mul3
STATIC Point3 Mul3(Point3 v, double f)
{
    v.c1 *= f;
    v.c2 *= f;
    v.c3 *= f;
    return v;
}
#endif

#ifdef Div3
STATIC Point3 Div3(Point3 v, double f)
{
    v.c1 /= f;
    v.c2 /= f;
    v.c3 /= f;
    return v;
}
#endif

#ifdef Min3
STATIC Point3 Min3(Point3 v, Point3 w)
{
    v.c1 = v.c1<w.c1? v.c1 : w.c1;
    v.c2 = v.c2<w.c2? v.c2 : w.c2;
    v.c3 = v.c3<w.c3? v.c3 : w.c3;
    return v;
}
#endif

#ifdef Max3
STATIC Point3 Max3(Point3 v, Point3 w)
{
    v.c1 = v.c1>w.c1? v.c1 : w.c1;
    v.c2 = v.c2>w.c2? v.c2 : w.c2;
    v.c3 = v.c3>w.c3? v.c3 : w.c3;
    return v;
}
#endif

#ifdef Lerp3
STATIC Point3 Lerp3(double t, Point3 a, Point3 b)
{
    float s = 1-t;
    a.c1 = a.c1*t + b.c1*s;
    a.c2 = a.c2*t + b.c2*s;
    a.c3 = a.c3*t + b.c3*s;
    return a;
}
#endif

#ifdef Dot3
STATIC float Dot3(Point3 v, Point3 w)
{
    return v.x*w.x + v.y*w.y + v.z*w.z;
}
#endif

#ifdef Cross3
STATIC Point3 Cross3(Point3 v, Point3 w)
{
    Point3 r;
    r.c1 = v.c2*w.c3 - v.c3*w.c2;
    r.c2 = v.c3*w.c1 - v.c1*w.c3;
    r.c3 = v.c1*w.c2 - v.c2*w.c1;
    return r;
}
#endif

#ifdef Length3
STATIC double Length3(Point3 v)
{
    double v1 = v.c1;
    double v2 = v.c2;
    double v3 = v.c3;
    return sqrt(v1*v1 + v2*v2 + v3*v3);
}
#endif

#ifdef Normalize3
STATIC Point3 Normalize3(Point3 v)
{
    float d = Length3(v);
    if (d) {
	v.c1 /= d;
	v.c2 /= d;
	v.c3 /= d;
    }
    return v;
}
#endif

#ifdef Apply3
STATIC Point3 Apply3(Point3 p, Matrix3 t)
{
    Point3 q;
    q.c1 = t.A[0][0]*p.c1 + t.A[1][0]*p.c2 + t.A[2][0]*p.c3 + t.b[0];
    q.c2 = t.A[0][1]*p.c1 + t.A[1][1]*p.c2 + t.A[2][1]*p.c3 + t.b[1];
    q.c3 = t.A[0][2]*p.c1 + t.A[1][2]*p.c2 + t.A[2][2]*p.c3 + t.b[2];
    return q;
}
#endif

#ifdef Transpose3
STATIC Matrix3 Transpose3(Matrix3 t)
{
    Matrix3 s;
    s.A[0][0] = t.A[0][0];  s.A[0][1] = t.A[1][0];  s.A[0][2] = t.A[2][0];
    s.A[1][0] = t.A[0][1];  s.A[1][1] = t.A[1][1];  s.A[1][2] = t.A[2][1];
    s.A[2][0] = t.A[0][2];  s.A[2][1] = t.A[1][2];  s.A[2][2] = t.A[2][2];
    s.b[0] = s.b[1] = s.b[2] = 0;  /* XXX - ??? */
    return s;
}
#endif

#define COF(a,b,c,d) (t.A[a][c] * t.A[b][d] - t.A[a][d] * t.A[b][c])

#ifdef Invert3
STATIC Matrix3 Invert3(Matrix3 t)
{
    Point3 p;
    Matrix3 s;
    double inv;

    inv = t.A[0][0]*COF(1,2,1,2)+t.A[0][1]*COF(1,2,2,0)+t.A[0][2]*COF(1,2,0,1);
    inv = 1.0 / inv;

    s.A[0][0] = COF(1,2,1,2) * inv;
    s.A[1][0] = COF(1,2,2,0) * inv;
    s.A[2][0] = COF(1,2,0,1) * inv;
    s.A[0][1] = COF(2,0,1,2) * inv;
    s.A[1][1] = COF(2,0,2,0) * inv;
    s.A[2][1] = COF(2,0,0,1) * inv;
    s.A[0][2] = COF(0,1,1,2) * inv;
    s.A[1][2] = COF(0,1,2,0) * inv;
    s.A[2][2] = COF(0,1,0,1) * inv;

    s.b[0] = s.b[1] = s.b[2] = 0;
    p.c1 = -t.b[0]; p.c2 = -t.b[1]; p.c3 = -t.b[2];
    p  = Apply3(p, s);
    s.b[0] = p.c1; s.b[1] = p.c2; s.b[2] = p.c3;

    return s;
}
#endif

#ifdef Determinant3
STATIC float Determinant3(Matrix3 t)
{
    return
	t.A[0][0]*COF(1,2,1,2)+t.A[0][1]*COF(1,2,2,0)+t.A[0][2]*COF(1,2,0,1);
}
#endif

#ifdef AdjointTranspose3
STATIC Matrix3 AdjointTranspose3(Matrix3 t)
{
    Matrix3 s;

    s.A[0][0] = COF(1,2,1,2);
    s.A[0][1] = COF(1,2,2,0);
    s.A[0][2] = COF(1,2,0,1);
    s.A[1][0] = COF(2,0,1,2);
    s.A[1][1] = COF(2,0,2,0);
    s.A[1][2] = COF(2,0,0,1);
    s.A[2][0] = COF(0,1,1,2);
    s.A[2][1] = COF(0,1,2,0);
    s.A[2][2] = COF(0,1,0,1);

    /* XXX - ??? - NB check users if you change this */
    s.b[0] = s.b[1] = s.b[2] = 0;

    return s;
}
#endif

#undef STATIC
