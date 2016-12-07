#include <dxconfig.h>
#include <string.h>
#include <dx/dx.h>
#include "render.h"
#include "internals.h"
#define CAT(a,b) a##b
 static Error
 tri_vol(
 struct buffer *buf,
 struct xfield *xf,
 int ntri,
 Triangle *tri,
 int *indices,
 int surface,
 int clip_status,
 inv_stat invalid_status
 ) {
 RGBColor *c1, *c2, *c3;
 float x1, y1, x2, y2, x3, y3;
 float r1, g1, b1, o1, z1, r2, g2, b2, o2, z2, r3, g3, b3, o3, z3;
 float Qx, dxo, dxz, dxr, dxg, dxb;
 float Qy, dyr, dyg, dyb, dyo, dyz, dyA, dyB;
 float r, g, b, o, obar, z, nearPlane=xf->nearPlane ;
 float A, B, d, d1, d2, d3;
 int iy1, iy2, iy3;
 int iA, iB, iy, i, n, nn, left, right;
 float minx = buf->min.x, miny = buf->min.y;
 float maxx = buf->max.x, maxy = buf->max.y;
 Pointer fcolors = xf->fcolors;
 Pointer bcolors = xf->bcolors;
 Pointer opacities = xf->opacities;
 RGBColor *cmap = xf->cmap;
 float *omap = xf->omap;
    float cmul = xf->tile.color_multiplier;
    float omul = xf->tile.opacity_multiplier / cmul;
    ASSERT(buf->pix_type==pix_big);
    if (xf->tile.perspective)
 DXErrorReturn(ERROR_BAD_PARAMETER,
      "perspective volume rendering is not supported");
    if (!buf->merged)
 _dxf_MergeBackIntoZ(buf);
    if (opacities) {
 {
 Point *p;
 int v1, v2, v3, i, width, height, valid;
 float ox, oy;
 InvalidComponentHandle iElts;
 char fbyte = xf->fbyte, bbyte = xf->bbyte;
 switch(invalid_status) {
 case INV_VALID: iElts = NULL; valid = 1; break;
 case INV_INVALID: iElts = NULL; valid = 0; break;
 case INV_UNKNOWN: iElts = xf->iElts; break;
 }
 width = buf->width, height = buf->height;
 ox = buf->ox, oy = buf->oy;
 for (i=0; i<ntri; i++, tri++) {
 if (iElts)
 valid = DXIsElementValid(iElts, i);
 v1=tri->p, p= &xf->positions[v1], x1=p->x, y1=p->y, z1=p->z;
 v2=tri->q, p= &xf->positions[v2], x2=p->x, y2=p->y, z2=p->z;
 v3=tri->r, p= &xf->positions[v3], x3=p->x, y3=p->y, z3=p->z;
 if ((x1<minx && x2<minx && x3<minx) ||
 (x1>maxx && x2>maxx && x3>maxx) ||
 (y1<miny && y2<miny && y3<miny) ||
 (y1>maxy && y2>maxy && y3>maxy))
 continue;
 buf->triangles++;
 {
 struct big *pp, *p;
 Pointer colors;
 int cstcolors;
 RGBColor cbuf1, cbuf2, cbuf3;
 char cbyte;
 char obyte = xf->obyte;
 x1+=ox, y1+=oy;
 x2+=ox, y2+=oy;
 x3+=ox, y3+=oy;
 if (x1<-(DXD_MAX_INT/2) || x1>(DXD_MAX_INT/2) || y1<-(DXD_MAX_INT/2) || y1>(DXD_MAX_INT/2) ||
 x2<-(DXD_MAX_INT/2) || x2>(DXD_MAX_INT/2) || y2<-(DXD_MAX_INT/2) || y2>(DXD_MAX_INT/2) ||
 x3<-(DXD_MAX_INT/2) || x3>(DXD_MAX_INT/2) || y3<-(DXD_MAX_INT/2) || y3>(DXD_MAX_INT/2))
 DXErrorReturn(ERROR_BAD_PARAMETER,
 "camera causes numerical overflow");
 d1 = y3-y2, d2 = y1-y3, d3 = y2-y1;
 d = x1*d1 + x2*d2 + x3*d3;
 Qx = d? (float)1.0 / (float)d : 0.0;
 d1 *= Qx, d2 *= Qx, d3 *= Qx;
 if (valid) {
 if (1) {
 if (d < 0)
 cbyte = fbyte, colors = fcolors, cstcolors = xf->fcst;
 else
 cbyte = bbyte, colors = bcolors, cstcolors = xf->bcst;
 if (!colors) goto CAT(tvo,1);
 }
 if (1) {
 if (cmap) c1= &(cmap[((unsigned char *)colors)[cstcolors ? 0 : v1]]), c2= &(cmap[((unsigned char *)colors)[cstcolors ? 0 : v2]]), c3= &(cmap[((unsigned char *)colors)[cstcolors ? 0 : v3]]);
 else {
 
{ 
 if (cbyte) 
 { 
 RGBByteColor *ptr=((RGBByteColor *)colors)+(cstcolors ? 0 : v1); 
 cbuf1.r = _dxd_ubyteToFloat[ptr->r]; 
 cbuf1.g = _dxd_ubyteToFloat[ptr->g]; 
 cbuf1.b = _dxd_ubyteToFloat[ptr->b]; 
 } else 
 cbuf1 = (((RGBColor *)colors)[cstcolors ? 0 : v1]); 
}; 
{ 
 if (cbyte) 
 { 
 RGBByteColor *ptr=((RGBByteColor *)colors)+(cstcolors ? 0 : v2); 
 cbuf2.r = _dxd_ubyteToFloat[ptr->r]; 
 cbuf2.g = _dxd_ubyteToFloat[ptr->g]; 
 cbuf2.b = _dxd_ubyteToFloat[ptr->b]; 
 } else 
 cbuf2 = (((RGBColor *)colors)[cstcolors ? 0 : v2]); 
}; 
{ 
 if (cbyte) 
 { 
 RGBByteColor *ptr=((RGBByteColor *)colors)+(cstcolors ? 0 : v3); 
 cbuf3.r = _dxd_ubyteToFloat[ptr->r]; 
 cbuf3.g = _dxd_ubyteToFloat[ptr->g]; 
 cbuf3.b = _dxd_ubyteToFloat[ptr->b]; 
 } else 
 cbuf3 = (((RGBColor *)colors)[cstcolors ? 0 : v3]); 
};
 c1 = &cbuf1; c2 = &cbuf2; c3 = &cbuf3;
 }
 r1=c1->r, g1=c1->g, b1=c1->b;
 r2=c2->r, g2=c2->g, b2=c2->b;
 r3=c3->r, g3=c3->g, b3=c3->b;
 } else if (1) {
 if (cmap) r=(cmap[((unsigned char *)colors)[cstcolors ? 0 : i]]).r, g=(cmap[((unsigned char *)colors)[cstcolors ? 0 : i]]).g, b=(cmap[((unsigned char *)colors)[cstcolors ? 0 : i]]).b;
 else {
 
{ 
 if (cbyte) 
 { 
 RGBByteColor *ptr=((RGBByteColor *)colors)+(cstcolors ? 0 : i); 
 cbuf1.r = _dxd_ubyteToFloat[ptr->r]; 
 cbuf1.g = _dxd_ubyteToFloat[ptr->g]; 
 cbuf1.b = _dxd_ubyteToFloat[ptr->b]; 
 } else 
 cbuf1 = (((RGBColor *)colors)[cstcolors ? 0 : i]); 
};
 r = cbuf1.r; g = cbuf1.g; b = cbuf1.b;
 }
 }
 if (1) {
 if (omap) o1=(omap[((unsigned char *)opacities)[xf->ocst ? 0 : v1]]), o2=(omap[((unsigned char *)opacities)[xf->ocst ? 0 : v2]]), o3=(omap[((unsigned char *)opacities)[xf->ocst ? 0 : v3]]);
 else {
 
{ 
 if (obyte) 
 o1 = _dxd_ubyteToFloat[((ubyte *)opacities)[xf->ocst ? 0 : v1]]; 
 else 
 o1 = (((float *)opacities)[xf->ocst ? 0 : v1]); 
}; 
{ 
 if (obyte) 
 o2 = _dxd_ubyteToFloat[((ubyte *)opacities)[xf->ocst ? 0 : v2]]; 
 else 
 o2 = (((float *)opacities)[xf->ocst ? 0 : v2]); 
}; 
{ 
 if (obyte) 
 o3 = _dxd_ubyteToFloat[((ubyte *)opacities)[xf->ocst ? 0 : v3]]; 
 else 
 o3 = (((float *)opacities)[xf->ocst ? 0 : v3]); 
};
 }
 } else if (opacities||omap) {
 if (omap) o=(omap[((unsigned char *)opacities)[xf->ocst ? 0 : i]]);
 else 
{ 
 if (obyte) 
 o = _dxd_ubyteToFloat[((ubyte *)opacities)[xf->ocst ? 0 : i]]; 
 else 
 o = (((float *)opacities)[xf->ocst ? 0 : i]); 
};
 obar = 1.0-o;
 } else
 o = 1.0, obar = 0.0;
 if (1) dxr = d1*r1 + d2*r2 + d3*r3;
 if (1) dxg = d1*g1 + d2*g2 + d3*g3;
 if (1) dxb = d1*b1 + d2*b2 + d3*b3;
 if (1) dxo = d1*o1 + d2*o2 + d3*o3;
 } else
 r1 = r2 = r3 = g1 = g2 = g3 = b1 = b2 = b3 = o1 = o2 = o3 = 0.0;
 dxz = d1*z1 + d2*z2 + d3*z3;
 { 
 if (y2 < y1) { 
 (d=y1, y1=y2, y2=d), (d=x1, x1=x2, x2=d); 
 if (1) (d=r1, r1=r2, r2=d), (d=g1, g1=g2, g2=d), (d=b1, b1=b2, b2=d); 
 if (1) (d=o1, o1=o2, o2=d); 
 (d=z1, z1=z2, z2=d); 
 } 
}
 { 
 if (y3 < y1) { 
 (d=y1, y1=y3, y3=d), (d=x1, x1=x3, x3=d); 
 if (1) (d=r1, r1=r3, r3=d), (d=g1, g1=g3, g3=d), (d=b1, b1=b3, b3=d); 
 if (1) (d=o1, o1=o3, o3=d); 
 (d=z1, z1=z3, z3=d); 
 } 
}
 { 
 if (y3 < y2) { 
 (d=y2, y2=y3, y3=d), (d=x2, x2=x3, x3=d); 
 if (1) (d=r2, r2=r3, r3=d), (d=g2, g2=g3, g3=d), (d=b2, b2=b3, b3=d); 
 if (1) (d=o2, o2=o3, o3=d); 
 (d=z2, z2=z3, z3=d); 
 } 
}
 if (0) {
 A = B = x1;
 if (x2<A) A = x2; else B = x2;
 if (x3<A) A = x3; else if (x3>B) B = x3;
 if (A<buf->fmin.x) buf->fmin.x = A;
 if (B>buf->fmax.x) buf->fmax.x = B;
 if (y1<buf->fmin.y) buf->fmin.y = y1;
 if (y3>buf->fmax.y) buf->fmax.y = y3;
 }
 iy1 = ((int)((y1)-30000.0)+30000), iy2 = ((int)((y2)-30000.0)+30000), iy3 = ((int)((y3)-30000.0)+30000);
 if (iy1>iy2 || iy2>iy3) {
 DXSetError(ERROR_DATA_INVALID,
 "position number %d, %d, or %d is invalid", v1, v2, v3);
 return ERROR;
 }
 if (iy1==iy3)
 goto CAT(tvo,1);
 d = y3 - y1;
 Qy = d? (float)1.0 / (float)d : 0.0;
 if (valid) {
 if (1) dyr=(r3-r1)*Qy, dyg=(g3-g1)*Qy, dyb=(b3-b1)*Qy;
 if (1) dyo = (float)(o3-o1) * Qy;
 }
 dyz = (z3-z1) * Qy;
 dyA = (float)(x3-x1) * Qy;
 A = x1;
 pp = buf->u.big + iy1*width;
 d = iy1 - y1;
 if (iy1<0)
 d-=iy1, pp=buf->u.big;
 A += dyA*d;
 if (valid) {
 if (1) r1+=dyr*d, g1+=dyg*d, b1+=dyb*d;
 if (1) o1+=dyo*d;
 }
 z1 += dyz*d;
 nn = 0;
 if (valid) {
 { 
 
 if (iy2>height) 
 iy2 = height; 
 if (iy1<iy2 && iy2>0 && iy1<=height) { 
 B = x1; 
 d = y2 - y1; 
 d = d? (float)1.0 / (float)d : 0.0; 
 dyB = (float)(x2-x1) * d; 
 if (iy1<0) iy1=0; 
 d = iy1 - y1; 
 B += d*dyB; 
 for (iy=iy1; iy<iy2; iy++) { 
 iA = ((int)((A)-30000.0)+30000); 
 iB = ((int)((B)-30000.0)+30000); 
 if (iB>iA) left=iA, right=iB; 
 else left=iB, right=iA; 
 if (left<0) left=0; 
 if (right>width) right=width; 
 n = right - left; 
 if (n>0) { 
 nn += n; 
 p = pp+left; 
 d = left - A; 
 if (1) r=r1+d*dxr, g=g1+d*dxg, b=b1+d*dxb; 
 if (1) o=o1+d*dxo, obar=1.0-o; 
 z=z1+d*dxz; 
 while (--n>=0) { 
 if (1) { 
 { if (p->in<=0) { if (z < p->front) { if (surface) { p->in++; if (z > p->back) p->back = z; if (valid && !xf->tile.flat_z) { p->co.r=r; p->co.g=g; p->co.b=b; } } } } else { if (surface) p->in = 0; if (z >= p->front) { p->in = -DXD_MAX_INT; z = p->front; } if (z > p->back) { float ar; float ag; float ab; float ao; if (valid) { if ( 1) { d = (z - p->back) * cmul; ar = d * r; ag = d * g; ab = d * b; ao = d * o * omul; } else { d = 0.5 * (z - p->back) * cmul; ar = d * (p->co.r+r); ag = d * (p->co.g+g); ab = d * (p->co.b+b); ao = d * (p->co.o+r) * omul; p->co.r = r; p->co.g = g; p->co.b = b; p->co.o = o; } if ( 1) { if ((obar=1-ao) < 0.0) obar = 0.0; } else { if ((obar=1-0.125*ao) < 0.0) obar = 0.0; obar = obar*obar; obar = obar*obar; obar = obar*obar; } p->c.r = p->c.r * obar + ar; p->c.g = p->c.g * obar + ag; p->c.b = p->c.b * obar + ab; } p->back = z; } } }; 
 if (1) r+=dxr, g+=dxg, b+=dxb; 
 if (1) o+=dxo, obar=1.0-o; 
 z+=dxz; 
 p++; 
 } else { 
 if (1) r+=dxr, g+=dxg, b+=dxb; 
 if (1) o+=dxo, obar=1.0-o; 
 z+=dxz; 
 p++; 
 } 
 } 
 } 
 if (1) r1+=dyr, g1+=dyg, b1+=dyb; 
 if (1) o1+=dyo; 
 z1 += dyz; 
 A+=dyA, B+=dyB; 
 pp+=width; 
 } 
 } 
}
 { 
 
 if (iy3>height) 
 iy3 = height; 
 if (iy2<iy3 && iy3>0 && iy2<=height) { 
 B = x2; 
 d = y3 - y2; 
 d = d? (float)1.0 / (float)d : 0.0; 
 dyB = (float)(x3-x2) * d; 
 if (iy2<0) iy2=0; 
 d = iy2 - y2; 
 B += d*dyB; 
 for (iy=iy2; iy<iy3; iy++) { 
 iA = ((int)((A)-30000.0)+30000); 
 iB = ((int)((B)-30000.0)+30000); 
 if (iB>iA) left=iA, right=iB; 
 else left=iB, right=iA; 
 if (left<0) left=0; 
 if (right>width) right=width; 
 n = right - left; 
 if (n>0) { 
 nn += n; 
 p = pp+left; 
 d = left - A; 
 if (1) r=r1+d*dxr, g=g1+d*dxg, b=b1+d*dxb; 
 if (1) o=o1+d*dxo, obar=1.0-o; 
 z=z1+d*dxz; 
 while (--n>=0) { 
 if (1) { 
 { if (p->in<=0) { if (z < p->front) { if (surface) { p->in++; if (z > p->back) p->back = z; if (valid && !xf->tile.flat_z) { p->co.r=r; p->co.g=g; p->co.b=b; } } } } else { if (surface) p->in = 0; if (z >= p->front) { p->in = -DXD_MAX_INT; z = p->front; } if (z > p->back) { float ar; float ag; float ab; float ao; if (valid) { if ( 1) { d = (z - p->back) * cmul; ar = d * r; ag = d * g; ab = d * b; ao = d * o * omul; } else { d = 0.5 * (z - p->back) * cmul; ar = d * (p->co.r+r); ag = d * (p->co.g+g); ab = d * (p->co.b+b); ao = d * (p->co.o+r) * omul; p->co.r = r; p->co.g = g; p->co.b = b; p->co.o = o; } if ( 1) { if ((obar=1-ao) < 0.0) obar = 0.0; } else { if ((obar=1-0.125*ao) < 0.0) obar = 0.0; obar = obar*obar; obar = obar*obar; obar = obar*obar; } p->c.r = p->c.r * obar + ar; p->c.g = p->c.g * obar + ag; p->c.b = p->c.b * obar + ab; } p->back = z; } } }; 
 if (1) r+=dxr, g+=dxg, b+=dxb; 
 if (1) o+=dxo, obar=1.0-o; 
 z+=dxz; 
 p++; 
 } else { 
 if (1) r+=dxr, g+=dxg, b+=dxb; 
 if (1) o+=dxo, obar=1.0-o; 
 z+=dxz; 
 p++; 
 } 
 } 
 } 
 if (1) r1+=dyr, g1+=dyg, b1+=dyb; 
 if (1) o1+=dyo; 
 z1 += dyz; 
 A+=dyA, B+=dyB; 
 pp+=width; 
 } 
 } 
}
 } else {
 { 
 
 if (iy2>height) 
 iy2 = height; 
 if (iy1<iy2 && iy2>0 && iy1<=height) { 
 B = x1; 
 d = y2 - y1; 
 d = d? (float)1.0 / (float)d : 0.0; 
 dyB = (float)(x2-x1) * d; 
 if (iy1<0) iy1=0; 
 d = iy1 - y1; 
 B += d*dyB; 
 for (iy=iy1; iy<iy2; iy++) { 
 iA = ((int)((A)-30000.0)+30000); 
 iB = ((int)((B)-30000.0)+30000); 
 if (iB>iA) left=iA, right=iB; 
 else left=iB, right=iA; 
 if (left<0) left=0; 
 if (right>width) right=width; 
 n = right - left; 
 if (n>0) { 
 nn += n; 
 p = pp+left; 
 d = left - A; 
 if (0) r=r1+d*dxr, g=g1+d*dxg, b=b1+d*dxb; 
 if (0) o=o1+d*dxo, obar=1.0-o; 
 z=z1+d*dxz; 
 while (--n>=0) { 
 if (1) { 
 { if (p->in<=0) { if (z < p->front) { if (surface) { p->in++; if (z > p->back) p->back = z; if (valid && !xf->tile.flat_z) { p->co.r=r; p->co.g=g; p->co.b=b; } } } } else { if (surface) p->in = 0; if (z >= p->front) { p->in = -DXD_MAX_INT; z = p->front; } if (z > p->back) { float ar; float ag; float ab; float ao; if (valid) { if ( 1) { d = (z - p->back) * cmul; ar = d * r; ag = d * g; ab = d * b; ao = d * o * omul; } else { d = 0.5 * (z - p->back) * cmul; ar = d * (p->co.r+r); ag = d * (p->co.g+g); ab = d * (p->co.b+b); ao = d * (p->co.o+r) * omul; p->co.r = r; p->co.g = g; p->co.b = b; p->co.o = o; } if ( 1) { if ((obar=1-ao) < 0.0) obar = 0.0; } else { if ((obar=1-0.125*ao) < 0.0) obar = 0.0; obar = obar*obar; obar = obar*obar; obar = obar*obar; } p->c.r = p->c.r * obar + ar; p->c.g = p->c.g * obar + ag; p->c.b = p->c.b * obar + ab; } p->back = z; } } }; 
 if (0) r+=dxr, g+=dxg, b+=dxb; 
 if (0) o+=dxo, obar=1.0-o; 
 z+=dxz; 
 p++; 
 } else { 
 if (0) r+=dxr, g+=dxg, b+=dxb; 
 if (0) o+=dxo, obar=1.0-o; 
 z+=dxz; 
 p++; 
 } 
 } 
 } 
 if (0) r1+=dyr, g1+=dyg, b1+=dyb; 
 if (0) o1+=dyo; 
 z1 += dyz; 
 A+=dyA, B+=dyB; 
 pp+=width; 
 } 
 } 
}
 { 
 
 if (iy3>height) 
 iy3 = height; 
 if (iy2<iy3 && iy3>0 && iy2<=height) { 
 B = x2; 
 d = y3 - y2; 
 d = d? (float)1.0 / (float)d : 0.0; 
 dyB = (float)(x3-x2) * d; 
 if (iy2<0) iy2=0; 
 d = iy2 - y2; 
 B += d*dyB; 
 for (iy=iy2; iy<iy3; iy++) { 
 iA = ((int)((A)-30000.0)+30000); 
 iB = ((int)((B)-30000.0)+30000); 
 if (iB>iA) left=iA, right=iB; 
 else left=iB, right=iA; 
 if (left<0) left=0; 
 if (right>width) right=width; 
 n = right - left; 
 if (n>0) { 
 nn += n; 
 p = pp+left; 
 d = left - A; 
 if (0) r=r1+d*dxr, g=g1+d*dxg, b=b1+d*dxb; 
 if (0) o=o1+d*dxo, obar=1.0-o; 
 z=z1+d*dxz; 
 while (--n>=0) { 
 if (1) { 
 { if (p->in<=0) { if (z < p->front) { if (surface) { p->in++; if (z > p->back) p->back = z; if (valid && !xf->tile.flat_z) { p->co.r=r; p->co.g=g; p->co.b=b; } } } } else { if (surface) p->in = 0; if (z >= p->front) { p->in = -DXD_MAX_INT; z = p->front; } if (z > p->back) { float ar; float ag; float ab; float ao; if (valid) { if ( 1) { d = (z - p->back) * cmul; ar = d * r; ag = d * g; ab = d * b; ao = d * o * omul; } else { d = 0.5 * (z - p->back) * cmul; ar = d * (p->co.r+r); ag = d * (p->co.g+g); ab = d * (p->co.b+b); ao = d * (p->co.o+r) * omul; p->co.r = r; p->co.g = g; p->co.b = b; p->co.o = o; } if ( 1) { if ((obar=1-ao) < 0.0) obar = 0.0; } else { if ((obar=1-0.125*ao) < 0.0) obar = 0.0; obar = obar*obar; obar = obar*obar; obar = obar*obar; } p->c.r = p->c.r * obar + ar; p->c.g = p->c.g * obar + ag; p->c.b = p->c.b * obar + ab; } p->back = z; } } }; 
 if (0) r+=dxr, g+=dxg, b+=dxb; 
 if (0) o+=dxo, obar=1.0-o; 
 z+=dxz; 
 p++; 
 } else { 
 if (0) r+=dxr, g+=dxg, b+=dxb; 
 if (0) o+=dxo, obar=1.0-o; 
 z+=dxz; 
 p++; 
 } 
 } 
 } 
 if (0) r1+=dyr, g1+=dyg, b1+=dyb; 
 if (0) o1+=dyo; 
 z1 += dyz; 
 A+=dyA, B+=dyB; 
 pp+=width; 
 } 
 } 
}
 }
 buf->pixels += nn;
};
 CAT(tvo,1) :
 continue;
 }
};
    } else {
 {
 Point *p;
 int v1, v2, v3, i, width, height, valid;
 float ox, oy;
 InvalidComponentHandle iElts;
 char fbyte = xf->fbyte, bbyte = xf->bbyte;
 switch(invalid_status) {
 case INV_VALID: iElts = NULL; valid = 1; break;
 case INV_INVALID: iElts = NULL; valid = 0; break;
 case INV_UNKNOWN: iElts = xf->iElts; break;
 }
 width = buf->width, height = buf->height;
 ox = buf->ox, oy = buf->oy;
 for (i=0; i<ntri; i++, tri++) {
 if (iElts)
 valid = DXIsElementValid(iElts, i);
 v1=tri->p, p= &xf->positions[v1], x1=p->x, y1=p->y, z1=p->z;
 v2=tri->q, p= &xf->positions[v2], x2=p->x, y2=p->y, z2=p->z;
 v3=tri->r, p= &xf->positions[v3], x3=p->x, y3=p->y, z3=p->z;
 if ((x1<minx && x2<minx && x3<minx) ||
 (x1>maxx && x2>maxx && x3>maxx) ||
 (y1<miny && y2<miny && y3<miny) ||
 (y1>maxy && y2>maxy && y3>maxy))
 continue;
 buf->triangles++;
 {
 struct big *pp, *p;
 Pointer colors;
 int cstcolors;
 RGBColor cbuf1, cbuf2, cbuf3;
 char cbyte;
 char obyte = xf->obyte;
 x1+=ox, y1+=oy;
 x2+=ox, y2+=oy;
 x3+=ox, y3+=oy;
 if (x1<-(DXD_MAX_INT/2) || x1>(DXD_MAX_INT/2) || y1<-(DXD_MAX_INT/2) || y1>(DXD_MAX_INT/2) ||
 x2<-(DXD_MAX_INT/2) || x2>(DXD_MAX_INT/2) || y2<-(DXD_MAX_INT/2) || y2>(DXD_MAX_INT/2) ||
 x3<-(DXD_MAX_INT/2) || x3>(DXD_MAX_INT/2) || y3<-(DXD_MAX_INT/2) || y3>(DXD_MAX_INT/2))
 DXErrorReturn(ERROR_BAD_PARAMETER,
 "camera causes numerical overflow");
 d1 = y3-y2, d2 = y1-y3, d3 = y2-y1;
 d = x1*d1 + x2*d2 + x3*d3;
 Qx = d? (float)1.0 / (float)d : 0.0;
 d1 *= Qx, d2 *= Qx, d3 *= Qx;
 if (valid) {
 if (1) {
 if (d < 0)
 cbyte = fbyte, colors = fcolors, cstcolors = xf->fcst;
 else
 cbyte = bbyte, colors = bcolors, cstcolors = xf->bcst;
 if (!colors) goto CAT(tvno,1);
 }
 if (1) {
 if (cmap) c1= &(cmap[((unsigned char *)colors)[cstcolors ? 0 : v1]]), c2= &(cmap[((unsigned char *)colors)[cstcolors ? 0 : v2]]), c3= &(cmap[((unsigned char *)colors)[cstcolors ? 0 : v3]]);
 else {
 
{ 
 if (cbyte) 
 { 
 RGBByteColor *ptr=((RGBByteColor *)colors)+(cstcolors ? 0 : v1); 
 cbuf1.r = _dxd_ubyteToFloat[ptr->r]; 
 cbuf1.g = _dxd_ubyteToFloat[ptr->g]; 
 cbuf1.b = _dxd_ubyteToFloat[ptr->b]; 
 } else 
 cbuf1 = (((RGBColor *)colors)[cstcolors ? 0 : v1]); 
}; 
{ 
 if (cbyte) 
 { 
 RGBByteColor *ptr=((RGBByteColor *)colors)+(cstcolors ? 0 : v2); 
 cbuf2.r = _dxd_ubyteToFloat[ptr->r]; 
 cbuf2.g = _dxd_ubyteToFloat[ptr->g]; 
 cbuf2.b = _dxd_ubyteToFloat[ptr->b]; 
 } else 
 cbuf2 = (((RGBColor *)colors)[cstcolors ? 0 : v2]); 
}; 
{ 
 if (cbyte) 
 { 
 RGBByteColor *ptr=((RGBByteColor *)colors)+(cstcolors ? 0 : v3); 
 cbuf3.r = _dxd_ubyteToFloat[ptr->r]; 
 cbuf3.g = _dxd_ubyteToFloat[ptr->g]; 
 cbuf3.b = _dxd_ubyteToFloat[ptr->b]; 
 } else 
 cbuf3 = (((RGBColor *)colors)[cstcolors ? 0 : v3]); 
};
 c1 = &cbuf1; c2 = &cbuf2; c3 = &cbuf3;
 }
 r1=c1->r, g1=c1->g, b1=c1->b;
 r2=c2->r, g2=c2->g, b2=c2->b;
 r3=c3->r, g3=c3->g, b3=c3->b;
 } else if (1) {
 if (cmap) r=(cmap[((unsigned char *)colors)[cstcolors ? 0 : i]]).r, g=(cmap[((unsigned char *)colors)[cstcolors ? 0 : i]]).g, b=(cmap[((unsigned char *)colors)[cstcolors ? 0 : i]]).b;
 else {
 
{ 
 if (cbyte) 
 { 
 RGBByteColor *ptr=((RGBByteColor *)colors)+(cstcolors ? 0 : i); 
 cbuf1.r = _dxd_ubyteToFloat[ptr->r]; 
 cbuf1.g = _dxd_ubyteToFloat[ptr->g]; 
 cbuf1.b = _dxd_ubyteToFloat[ptr->b]; 
 } else 
 cbuf1 = (((RGBColor *)colors)[cstcolors ? 0 : i]); 
};
 r = cbuf1.r; g = cbuf1.g; b = cbuf1.b;
 }
 }
 if (0) {
 if (omap) o1=(omap[((unsigned char *)opacities)[xf->ocst ? 0 : v1]]), o2=(omap[((unsigned char *)opacities)[xf->ocst ? 0 : v2]]), o3=(omap[((unsigned char *)opacities)[xf->ocst ? 0 : v3]]);
 else {
 
{ 
 if (obyte) 
 o1 = _dxd_ubyteToFloat[((ubyte *)opacities)[xf->ocst ? 0 : v1]]; 
 else 
 o1 = (((float *)opacities)[xf->ocst ? 0 : v1]); 
}; 
{ 
 if (obyte) 
 o2 = _dxd_ubyteToFloat[((ubyte *)opacities)[xf->ocst ? 0 : v2]]; 
 else 
 o2 = (((float *)opacities)[xf->ocst ? 0 : v2]); 
}; 
{ 
 if (obyte) 
 o3 = _dxd_ubyteToFloat[((ubyte *)opacities)[xf->ocst ? 0 : v3]]; 
 else 
 o3 = (((float *)opacities)[xf->ocst ? 0 : v3]); 
};
 }
 } else if (opacities||omap) {
 if (omap) o=(omap[((unsigned char *)opacities)[xf->ocst ? 0 : i]]);
 else 
{ 
 if (obyte) 
 o = _dxd_ubyteToFloat[((ubyte *)opacities)[xf->ocst ? 0 : i]]; 
 else 
 o = (((float *)opacities)[xf->ocst ? 0 : i]); 
};
 obar = 1.0-o;
 } else
 o = 1.0, obar = 0.0;
 if (1) dxr = d1*r1 + d2*r2 + d3*r3;
 if (1) dxg = d1*g1 + d2*g2 + d3*g3;
 if (1) dxb = d1*b1 + d2*b2 + d3*b3;
 if (0) dxo = d1*o1 + d2*o2 + d3*o3;
 } else
 r1 = r2 = r3 = g1 = g2 = g3 = b1 = b2 = b3 = o1 = o2 = o3 = 0.0;
 dxz = d1*z1 + d2*z2 + d3*z3;
 { 
 if (y2 < y1) { 
 (d=y1, y1=y2, y2=d), (d=x1, x1=x2, x2=d); 
 if (1) (d=r1, r1=r2, r2=d), (d=g1, g1=g2, g2=d), (d=b1, b1=b2, b2=d); 
 if (0) (d=o1, o1=o2, o2=d); 
 (d=z1, z1=z2, z2=d); 
 } 
}
 { 
 if (y3 < y1) { 
 (d=y1, y1=y3, y3=d), (d=x1, x1=x3, x3=d); 
 if (1) (d=r1, r1=r3, r3=d), (d=g1, g1=g3, g3=d), (d=b1, b1=b3, b3=d); 
 if (0) (d=o1, o1=o3, o3=d); 
 (d=z1, z1=z3, z3=d); 
 } 
}
 { 
 if (y3 < y2) { 
 (d=y2, y2=y3, y3=d), (d=x2, x2=x3, x3=d); 
 if (1) (d=r2, r2=r3, r3=d), (d=g2, g2=g3, g3=d), (d=b2, b2=b3, b3=d); 
 if (0) (d=o2, o2=o3, o3=d); 
 (d=z2, z2=z3, z3=d); 
 } 
}
 if (0) {
 A = B = x1;
 if (x2<A) A = x2; else B = x2;
 if (x3<A) A = x3; else if (x3>B) B = x3;
 if (A<buf->fmin.x) buf->fmin.x = A;
 if (B>buf->fmax.x) buf->fmax.x = B;
 if (y1<buf->fmin.y) buf->fmin.y = y1;
 if (y3>buf->fmax.y) buf->fmax.y = y3;
 }
 iy1 = ((int)((y1)-30000.0)+30000), iy2 = ((int)((y2)-30000.0)+30000), iy3 = ((int)((y3)-30000.0)+30000);
 if (iy1>iy2 || iy2>iy3) {
 DXSetError(ERROR_DATA_INVALID,
 "position number %d, %d, or %d is invalid", v1, v2, v3);
 return ERROR;
 }
 if (iy1==iy3)
 goto CAT(tvno,1);
 d = y3 - y1;
 Qy = d? (float)1.0 / (float)d : 0.0;
 if (valid) {
 if (1) dyr=(r3-r1)*Qy, dyg=(g3-g1)*Qy, dyb=(b3-b1)*Qy;
 if (0) dyo = (float)(o3-o1) * Qy;
 }
 dyz = (z3-z1) * Qy;
 dyA = (float)(x3-x1) * Qy;
 A = x1;
 pp = buf->u.big + iy1*width;
 d = iy1 - y1;
 if (iy1<0)
 d-=iy1, pp=buf->u.big;
 A += dyA*d;
 if (valid) {
 if (1) r1+=dyr*d, g1+=dyg*d, b1+=dyb*d;
 if (0) o1+=dyo*d;
 }
 z1 += dyz*d;
 nn = 0;
 if (valid) {
 { 
 
 if (iy2>height) 
 iy2 = height; 
 if (iy1<iy2 && iy2>0 && iy1<=height) { 
 B = x1; 
 d = y2 - y1; 
 d = d? (float)1.0 / (float)d : 0.0; 
 dyB = (float)(x2-x1) * d; 
 if (iy1<0) iy1=0; 
 d = iy1 - y1; 
 B += d*dyB; 
 for (iy=iy1; iy<iy2; iy++) { 
 iA = ((int)((A)-30000.0)+30000); 
 iB = ((int)((B)-30000.0)+30000); 
 if (iB>iA) left=iA, right=iB; 
 else left=iB, right=iA; 
 if (left<0) left=0; 
 if (right>width) right=width; 
 n = right - left; 
 if (n>0) { 
 nn += n; 
 p = pp+left; 
 d = left - A; 
 if (1) r=r1+d*dxr, g=g1+d*dxg, b=b1+d*dxb; 
 if (0) o=o1+d*dxo, obar=1.0-o; 
 z=z1+d*dxz; 
 while (--n>=0) { 
 if (1) { 
 { if (p->in<=0) { if (z < p->front) { if (surface) { p->in++; if (z > p->back) p->back = z; if (valid && !xf->tile.flat_z) { p->co.r=r; p->co.g=g; p->co.b=b; } } } } else { if (surface) p->in = 0; if (z >= p->front) { p->in = -DXD_MAX_INT; z = p->front; } if (z > p->back) { float ar; float ag; float ab; float ao; if (valid) { if ( 1) { d = (z - p->back) * cmul; ar = d * r; ag = d * g; ab = d * b; ao = d * o * omul; } else { d = 0.5 * (z - p->back) * cmul; ar = d * (p->co.r+r); ag = d * (p->co.g+g); ab = d * (p->co.b+b); ao = d * (p->co.o+r) * omul; p->co.r = r; p->co.g = g; p->co.b = b; p->co.o = o; } if ( 1) { if ((obar=1-ao) < 0.0) obar = 0.0; } else { if ((obar=1-0.125*ao) < 0.0) obar = 0.0; obar = obar*obar; obar = obar*obar; obar = obar*obar; } p->c.r = p->c.r * obar + ar; p->c.g = p->c.g * obar + ag; p->c.b = p->c.b * obar + ab; } p->back = z; } } }; 
 if (1) r+=dxr, g+=dxg, b+=dxb; 
 if (0) o+=dxo, obar=1.0-o; 
 z+=dxz; 
 p++; 
 } else { 
 if (1) r+=dxr, g+=dxg, b+=dxb; 
 if (0) o+=dxo, obar=1.0-o; 
 z+=dxz; 
 p++; 
 } 
 } 
 } 
 if (1) r1+=dyr, g1+=dyg, b1+=dyb; 
 if (0) o1+=dyo; 
 z1 += dyz; 
 A+=dyA, B+=dyB; 
 pp+=width; 
 } 
 } 
}
 { 
 
 if (iy3>height) 
 iy3 = height; 
 if (iy2<iy3 && iy3>0 && iy2<=height) { 
 B = x2; 
 d = y3 - y2; 
 d = d? (float)1.0 / (float)d : 0.0; 
 dyB = (float)(x3-x2) * d; 
 if (iy2<0) iy2=0; 
 d = iy2 - y2; 
 B += d*dyB; 
 for (iy=iy2; iy<iy3; iy++) { 
 iA = ((int)((A)-30000.0)+30000); 
 iB = ((int)((B)-30000.0)+30000); 
 if (iB>iA) left=iA, right=iB; 
 else left=iB, right=iA; 
 if (left<0) left=0; 
 if (right>width) right=width; 
 n = right - left; 
 if (n>0) { 
 nn += n; 
 p = pp+left; 
 d = left - A; 
 if (1) r=r1+d*dxr, g=g1+d*dxg, b=b1+d*dxb; 
 if (0) o=o1+d*dxo, obar=1.0-o; 
 z=z1+d*dxz; 
 while (--n>=0) { 
 if (1) { 
 { if (p->in<=0) { if (z < p->front) { if (surface) { p->in++; if (z > p->back) p->back = z; if (valid && !xf->tile.flat_z) { p->co.r=r; p->co.g=g; p->co.b=b; } } } } else { if (surface) p->in = 0; if (z >= p->front) { p->in = -DXD_MAX_INT; z = p->front; } if (z > p->back) { float ar; float ag; float ab; float ao; if (valid) { if ( 1) { d = (z - p->back) * cmul; ar = d * r; ag = d * g; ab = d * b; ao = d * o * omul; } else { d = 0.5 * (z - p->back) * cmul; ar = d * (p->co.r+r); ag = d * (p->co.g+g); ab = d * (p->co.b+b); ao = d * (p->co.o+r) * omul; p->co.r = r; p->co.g = g; p->co.b = b; p->co.o = o; } if ( 1) { if ((obar=1-ao) < 0.0) obar = 0.0; } else { if ((obar=1-0.125*ao) < 0.0) obar = 0.0; obar = obar*obar; obar = obar*obar; obar = obar*obar; } p->c.r = p->c.r * obar + ar; p->c.g = p->c.g * obar + ag; p->c.b = p->c.b * obar + ab; } p->back = z; } } }; 
 if (1) r+=dxr, g+=dxg, b+=dxb; 
 if (0) o+=dxo, obar=1.0-o; 
 z+=dxz; 
 p++; 
 } else { 
 if (1) r+=dxr, g+=dxg, b+=dxb; 
 if (0) o+=dxo, obar=1.0-o; 
 z+=dxz; 
 p++; 
 } 
 } 
 } 
 if (1) r1+=dyr, g1+=dyg, b1+=dyb; 
 if (0) o1+=dyo; 
 z1 += dyz; 
 A+=dyA, B+=dyB; 
 pp+=width; 
 } 
 } 
}
 } else {
 { 
 
 if (iy2>height) 
 iy2 = height; 
 if (iy1<iy2 && iy2>0 && iy1<=height) { 
 B = x1; 
 d = y2 - y1; 
 d = d? (float)1.0 / (float)d : 0.0; 
 dyB = (float)(x2-x1) * d; 
 if (iy1<0) iy1=0; 
 d = iy1 - y1; 
 B += d*dyB; 
 for (iy=iy1; iy<iy2; iy++) { 
 iA = ((int)((A)-30000.0)+30000); 
 iB = ((int)((B)-30000.0)+30000); 
 if (iB>iA) left=iA, right=iB; 
 else left=iB, right=iA; 
 if (left<0) left=0; 
 if (right>width) right=width; 
 n = right - left; 
 if (n>0) { 
 nn += n; 
 p = pp+left; 
 d = left - A; 
 if (0) r=r1+d*dxr, g=g1+d*dxg, b=b1+d*dxb; 
 if (0) o=o1+d*dxo, obar=1.0-o; 
 z=z1+d*dxz; 
 while (--n>=0) { 
 if (1) { 
 { if (p->in<=0) { if (z < p->front) { if (surface) { p->in++; if (z > p->back) p->back = z; if (valid && !xf->tile.flat_z) { p->co.r=r; p->co.g=g; p->co.b=b; } } } } else { if (surface) p->in = 0; if (z >= p->front) { p->in = -DXD_MAX_INT; z = p->front; } if (z > p->back) { float ar; float ag; float ab; float ao; if (valid) { if ( 1) { d = (z - p->back) * cmul; ar = d * r; ag = d * g; ab = d * b; ao = d * o * omul; } else { d = 0.5 * (z - p->back) * cmul; ar = d * (p->co.r+r); ag = d * (p->co.g+g); ab = d * (p->co.b+b); ao = d * (p->co.o+r) * omul; p->co.r = r; p->co.g = g; p->co.b = b; p->co.o = o; } if ( 1) { if ((obar=1-ao) < 0.0) obar = 0.0; } else { if ((obar=1-0.125*ao) < 0.0) obar = 0.0; obar = obar*obar; obar = obar*obar; obar = obar*obar; } p->c.r = p->c.r * obar + ar; p->c.g = p->c.g * obar + ag; p->c.b = p->c.b * obar + ab; } p->back = z; } } }; 
 if (0) r+=dxr, g+=dxg, b+=dxb; 
 if (0) o+=dxo, obar=1.0-o; 
 z+=dxz; 
 p++; 
 } else { 
 if (0) r+=dxr, g+=dxg, b+=dxb; 
 if (0) o+=dxo, obar=1.0-o; 
 z+=dxz; 
 p++; 
 } 
 } 
 } 
 if (0) r1+=dyr, g1+=dyg, b1+=dyb; 
 if (0) o1+=dyo; 
 z1 += dyz; 
 A+=dyA, B+=dyB; 
 pp+=width; 
 } 
 } 
}
 { 
 
 if (iy3>height) 
 iy3 = height; 
 if (iy2<iy3 && iy3>0 && iy2<=height) { 
 B = x2; 
 d = y3 - y2; 
 d = d? (float)1.0 / (float)d : 0.0; 
 dyB = (float)(x3-x2) * d; 
 if (iy2<0) iy2=0; 
 d = iy2 - y2; 
 B += d*dyB; 
 for (iy=iy2; iy<iy3; iy++) { 
 iA = ((int)((A)-30000.0)+30000); 
 iB = ((int)((B)-30000.0)+30000); 
 if (iB>iA) left=iA, right=iB; 
 else left=iB, right=iA; 
 if (left<0) left=0; 
 if (right>width) right=width; 
 n = right - left; 
 if (n>0) { 
 nn += n; 
 p = pp+left; 
 d = left - A; 
 if (0) r=r1+d*dxr, g=g1+d*dxg, b=b1+d*dxb; 
 if (0) o=o1+d*dxo, obar=1.0-o; 
 z=z1+d*dxz; 
 while (--n>=0) { 
 if (1) { 
 { if (p->in<=0) { if (z < p->front) { if (surface) { p->in++; if (z > p->back) p->back = z; if (valid && !xf->tile.flat_z) { p->co.r=r; p->co.g=g; p->co.b=b; } } } } else { if (surface) p->in = 0; if (z >= p->front) { p->in = -DXD_MAX_INT; z = p->front; } if (z > p->back) { float ar; float ag; float ab; float ao; if (valid) { if ( 1) { d = (z - p->back) * cmul; ar = d * r; ag = d * g; ab = d * b; ao = d * o * omul; } else { d = 0.5 * (z - p->back) * cmul; ar = d * (p->co.r+r); ag = d * (p->co.g+g); ab = d * (p->co.b+b); ao = d * (p->co.o+r) * omul; p->co.r = r; p->co.g = g; p->co.b = b; p->co.o = o; } if ( 1) { if ((obar=1-ao) < 0.0) obar = 0.0; } else { if ((obar=1-0.125*ao) < 0.0) obar = 0.0; obar = obar*obar; obar = obar*obar; obar = obar*obar; } p->c.r = p->c.r * obar + ar; p->c.g = p->c.g * obar + ag; p->c.b = p->c.b * obar + ab; } p->back = z; } } }; 
 if (0) r+=dxr, g+=dxg, b+=dxb; 
 if (0) o+=dxo, obar=1.0-o; 
 z+=dxz; 
 p++; 
 } else { 
 if (0) r+=dxr, g+=dxg, b+=dxb; 
 if (0) o+=dxo, obar=1.0-o; 
 z+=dxz; 
 p++; 
 } 
 } 
 } 
 if (0) r1+=dyr, g1+=dyg, b1+=dyb; 
 if (0) o1+=dyo; 
 z1 += dyz; 
 A+=dyA, B+=dyB; 
 pp+=width; 
 } 
 } 
}
 }
 buf->pixels += nn;
};
 CAT(tvno,1) :
 continue;
 }
};
    }
 return OK;
 }
 static Error
 tri_translucent(
 struct buffer *buf,
 struct xfield *xf,
 int ntri,
 Triangle *tri,
 int *indices,
 int surface,
 int clip_status,
 inv_stat invalid_status
 ) {
 RGBColor *c1, *c2, *c3;
 float x1, y1, x2, y2, x3, y3;
 float r1, g1, b1, o1, z1, r2, g2, b2, o2, z2, r3, g3, b3, o3, z3;
 float Qx, dxo, dxz, dxr, dxg, dxb;
 float Qy, dyr, dyg, dyb, dyo, dyz, dyA, dyB;
 float r, g, b, o, obar, z, nearPlane=xf->nearPlane ;
 float A, B, d, d1, d2, d3;
 int iy1, iy2, iy3;
 int iA, iB, iy, i, n, nn, left, right;
 float minx = buf->min.x, miny = buf->min.y;
 float maxx = buf->max.x, maxy = buf->max.y;
 Pointer fcolors = xf->fcolors;
 Pointer bcolors = xf->bcolors;
 Pointer opacities = xf->opacities;
 RGBColor *cmap = xf->cmap;
 float *omap = xf->omap;
  if (xf->colors_dep == dep_connections)
  {
    if (clip_status) {
      ASSERT(buf->pix_type==pix_big);
      {
 Point *p;
 int v1, v2, v3, i, width, height;
 int i1, i2, i3;
 float ox, oy;
 InvalidComponentHandle iElts;
 RGBColor vcolors[6];
 int shademe;
 char fcst = xf->fcst, bcst = xf->bcst, ncst = xf->ncst;
 char fbyte = xf->fbyte, bbyte = xf->bbyte;
 iElts = (invalid_status == INV_UNKNOWN) ? xf->iElts : NULL;
 width = buf->width, height = buf->height;
 ox = buf->ox, oy = buf->oy;
 if (1 && xf->lights) {
 fcolors = (Pointer)vcolors;
 bcolors = (Pointer)(vcolors+3);
 _dxfInitApplyLights(xf->kaf, xf->kdf, xf->ksf, xf->kspf,
 xf->kab, xf->kdb, xf->ksb, xf->kspb,
 xf->fcolors, xf->bcolors, xf->cmap,
 xf->normals, xf->lights,
 xf->colors_dep, xf->normals_dep,
 fcolors, bcolors, 3, fcst, bcst, ncst,
 fbyte, bbyte);
 cmap = NULL;
 shademe = 1;
 } else {
 shademe = 0;
 if (fbyte || bbyte)
 _dxf_initUbyteToFloat();
 }
 for (i=0; i<ntri; i++, tri++) {
 int index = (indices == NULL) ? i : indices[i];
 if (iElts && DXIsElementInvalid(iElts, index))
 continue;
 v1=tri->p, p= &xf->positions[v1], x1=p->x, y1=p->y, z1=p->z;
 v2=tri->q, p= &xf->positions[v2], x2=p->x, y2=p->y, z2=p->z;
 v3=tri->r, p= &xf->positions[v3], x3=p->x, y3=p->y, z3=p->z;
 if (shademe)
 {
 i1 = 0; i2 = 1; i3 = 2;
 if (! _dxfApplyLights((int *)tri, &index, 1))
 return ERROR;
 }
 if (xf->tile.perspective) {
 if (z1>nearPlane || z2>nearPlane || z3>nearPlane)
 continue;
 z1=(float)-1.0/z1; x1*=z1; y1*=z1;
 z2=(float)-1.0/z2; x2*=z2; y2*=z2;
 z3=(float)-1.0/z3; x3*=z3; y3*=z3;
 }
 if ((x1<minx && x2<minx && x3<minx) ||
 (x1>maxx && x2>maxx && x3>maxx) ||
 (y1<miny && y2<miny && y3<miny) ||
 (y1>maxy && y2>maxy && y3>maxy))
 continue;
 buf->triangles++;
 {
 struct big *pp, *p;
 Pointer colors;
 int cstcolors;
 RGBColor cbuf1, cbuf2, cbuf3;
 char cbyte;
 char obyte = xf->obyte;
 x1+=ox, y1+=oy;
 x2+=ox, y2+=oy;
 x3+=ox, y3+=oy;
 if (x1<-(DXD_MAX_INT/2) || x1>(DXD_MAX_INT/2) || y1<-(DXD_MAX_INT/2) || y1>(DXD_MAX_INT/2) ||
 x2<-(DXD_MAX_INT/2) || x2>(DXD_MAX_INT/2) || y2<-(DXD_MAX_INT/2) || y2>(DXD_MAX_INT/2) ||
 x3<-(DXD_MAX_INT/2) || x3>(DXD_MAX_INT/2) || y3<-(DXD_MAX_INT/2) || y3>(DXD_MAX_INT/2))
 DXErrorReturn(ERROR_BAD_PARAMETER,
 "camera causes numerical overflow");
 d1 = y3-y2, d2 = y1-y3, d3 = y2-y1;
 d = x1*d1 + x2*d2 + x3*d3;
 Qx = d? (float)1.0 / (float)d : 0.0;
 d1 *= Qx, d2 *= Qx, d3 *= Qx;
 if (1) {
 if (d < 0)
 cbyte = fbyte, colors = fcolors, cstcolors = xf->fcst;
 else
 cbyte = bbyte, colors = bcolors, cstcolors = xf->bcst;
 if (!colors) goto CAT(ttcdc,1);
 }
 if (1) {
 if (shademe) {
 c1 = ((RGBColor *)colors) + i1;
 c2 = ((RGBColor *)colors) + i2;
 c3 = ((RGBColor *)colors) + i3;
 } else {
 if (cmap)
 c1= &(cmap[((unsigned char *)colors)[cstcolors ? 0 : v1]]), c2= &(cmap[((unsigned char *)colors)[cstcolors ? 0 : v2]]), c3= &(cmap[((unsigned char *)colors)[cstcolors ? 0 : v3]]);
 else {
 
{ 
 if (cbyte) 
 { 
 RGBByteColor *ptr=((RGBByteColor *)colors)+(cstcolors ? 0 : v1); 
 cbuf1.r = _dxd_ubyteToFloat[ptr->r]; 
 cbuf1.g = _dxd_ubyteToFloat[ptr->g]; 
 cbuf1.b = _dxd_ubyteToFloat[ptr->b]; 
 } else 
 cbuf1 = (((RGBColor *)colors)[cstcolors ? 0 : v1]); 
}; 
{ 
 if (cbyte) 
 { 
 RGBByteColor *ptr=((RGBByteColor *)colors)+(cstcolors ? 0 : v2); 
 cbuf2.r = _dxd_ubyteToFloat[ptr->r]; 
 cbuf2.g = _dxd_ubyteToFloat[ptr->g]; 
 cbuf2.b = _dxd_ubyteToFloat[ptr->b]; 
 } else 
 cbuf2 = (((RGBColor *)colors)[cstcolors ? 0 : v2]); 
}; 
{ 
 if (cbyte) 
 { 
 RGBByteColor *ptr=((RGBByteColor *)colors)+(cstcolors ? 0 : v3); 
 cbuf3.r = _dxd_ubyteToFloat[ptr->r]; 
 cbuf3.g = _dxd_ubyteToFloat[ptr->g]; 
 cbuf3.b = _dxd_ubyteToFloat[ptr->b]; 
 } else 
 cbuf3 = (((RGBColor *)colors)[cstcolors ? 0 : v3]); 
};
 c1 = &cbuf1; c2 = &cbuf2; c3 = &cbuf3;
 }
 }
 r1=c1->r, g1=c1->g, b1=c1->b;
 r2=c2->r, g2=c2->g, b2=c2->b;
 r3=c3->r, g3=c3->g, b3=c3->b;
 } else if (1) {
 if (cmap){
 r=(cmap[((unsigned char *)colors)[cstcolors ? 0 : index]]).r; g=(cmap[((unsigned char *)colors)[cstcolors ? 0 : index]]).g; b=(cmap[((unsigned char *)colors)[cstcolors ? 0 : index]]).b;
 } else {
 
{ 
 if (cbyte) 
 { 
 RGBByteColor *ptr=((RGBByteColor *)colors)+(cstcolors ? 0 : index); 
 cbuf1.r = _dxd_ubyteToFloat[ptr->r]; 
 cbuf1.g = _dxd_ubyteToFloat[ptr->g]; 
 cbuf1.b = _dxd_ubyteToFloat[ptr->b]; 
 } else 
 cbuf1 = (((RGBColor *)colors)[cstcolors ? 0 : index]); 
};
 r = cbuf1.r; g = cbuf1.g; b = cbuf1.b;
 }
 }
 if (0) {
 if (omap) o1=(omap[((unsigned char *)opacities)[xf->ocst ? 0 : v1]]), o2=(omap[((unsigned char *)opacities)[xf->ocst ? 0 : v2]]), o3=(omap[((unsigned char *)opacities)[xf->ocst ? 0 : v3]]);
 else {
 
{ 
 if (obyte) 
 o1 = _dxd_ubyteToFloat[((ubyte *)opacities)[xf->ocst ? 0 : v1]]; 
 else 
 o1 = (((float *)opacities)[xf->ocst ? 0 : v1]); 
}; 
{ 
 if (obyte) 
 o2 = _dxd_ubyteToFloat[((ubyte *)opacities)[xf->ocst ? 0 : v2]]; 
 else 
 o2 = (((float *)opacities)[xf->ocst ? 0 : v2]); 
}; 
{ 
 if (obyte) 
 o3 = _dxd_ubyteToFloat[((ubyte *)opacities)[xf->ocst ? 0 : v3]]; 
 else 
 o3 = (((float *)opacities)[xf->ocst ? 0 : v3]); 
};
 }
 } else if (opacities||omap) {
 if (omap) o=(omap[((unsigned char *)opacities)[xf->ocst ? 0 : index]]);
 else 
{ 
 if (obyte) 
 o = _dxd_ubyteToFloat[((ubyte *)opacities)[xf->ocst ? 0 : index]]; 
 else 
 o = (((float *)opacities)[xf->ocst ? 0 : index]); 
};
 obar = 1.0-o;
 } else
 o = 1.0, obar = 0.0;
 if (1) {
 dxr = (r1 == r2 && r1 == r3) ? 0.0 : d1*r1 + d2*r2 + d3*r3;
 dxg = (g1 == g2 && g1 == g3) ? 0.0 : d1*g1 + d2*g2 + d3*g3;
 dxb = (b1 == b2 && b1 == b3) ? 0.0 : d1*b1 + d2*b2 + d3*b3;
 }
 if (0) dxo = (o1 == o2 && o1 == o3) ? 0.0 : d1*o1 + d2*o2 + d3*o3;
 dxz = (z1 == z2 && z1 == z3) ? 0.0 : d1*z1 + d2*z2 + d3*z3;
 { 
 if (y2 < y1) { 
 (d=y1, y1=y2, y2=d), (d=x1, x1=x2, x2=d); 
 if (1) (d=r1, r1=r2, r2=d), (d=g1, g1=g2, g2=d), (d=b1, b1=b2, b2=d); 
 if (0) (d=o1, o1=o2, o2=d); 
 (d=z1, z1=z2, z2=d); 
 } 
}
 { 
 if (y3 < y1) { 
 (d=y1, y1=y3, y3=d), (d=x1, x1=x3, x3=d); 
 if (1) (d=r1, r1=r3, r3=d), (d=g1, g1=g3, g3=d), (d=b1, b1=b3, b3=d); 
 if (0) (d=o1, o1=o3, o3=d); 
 (d=z1, z1=z3, z3=d); 
 } 
}
 { 
 if (y3 < y2) { 
 (d=y2, y2=y3, y3=d), (d=x2, x2=x3, x3=d); 
 if (1) (d=r2, r2=r3, r3=d), (d=g2, g2=g3, g3=d), (d=b2, b2=b3, b3=d); 
 if (0) (d=o2, o2=o3, o3=d); 
 (d=z2, z2=z3, z3=d); 
 } 
}
 if (0) {
 A = B = x1;
 if (x2<A) A = x2; else B = x2;
 if (x3<A) A = x3; else if (x3>B) B = x3;
 if (A<buf->fmin.x) buf->fmin.x = A;
 if (B>buf->fmax.x) buf->fmax.x = B;
 if (y1<buf->fmin.y) buf->fmin.y = y1;
 if (y3>buf->fmax.y) buf->fmax.y = y3;
 }
 iy1 = ((int)((y1)-30000.0)+30000), iy2 = ((int)((y2)-30000.0)+30000), iy3 = ((int)((y3)-30000.0)+30000);
 if (iy1>iy2 || iy2>iy3) {
 DXSetError(ERROR_DATA_INVALID,
 "position number %d, %d, or %d is invalid", v1, v2, v3);
 return ERROR;
 }
 if (iy1==iy3)
 goto CAT(ttcdc,1);
 d = y3 - y1;
 Qy = d? (float)1.0 / (float)d : 0.0;
 if (1) dyr=(r3-r1)*Qy, dyg=(g3-g1)*Qy, dyb=(b3-b1)*Qy;
 if (0) dyo = (float)(o3-o1) * Qy;
 dyz = (z3-z1) * Qy;
 dyA = (float)(x3-x1) * Qy;
 A = x1;
 pp = buf->u.big + iy1*width;
 d = iy1 - y1;
 if (iy1<0)
 d-=iy1, pp=buf->u.big;
 A += dyA*d;
 if (1) r1+=dyr*d, g1+=dyg*d, b1+=dyb*d;
 if (0) o1+=dyo*d;
 z1 += dyz*d;
 nn = 0;
 { 
 
 if (iy2>height) 
 iy2 = height; 
 if (iy1<iy2 && iy2>0 && iy1<=height) { 
 B = x1; 
 d = y2 - y1; 
 d = d? (float)1.0 / (float)d : 0.0; 
 dyB = (float)(x2-x1) * d; 
 if (iy1<0) iy1=0; 
 d = iy1 - y1; 
 B += d*dyB; 
 for (iy=iy1; iy<iy2; iy++) { 
 iA = ((int)((A)-30000.0)+30000); 
 iB = ((int)((B)-30000.0)+30000); 
 if (iB>iA) left=iA, right=iB; 
 else left=iB, right=iA; 
 if (left<0) left=0; 
 if (right>width) right=width; 
 n = right - left; 
 if (n>0) { 
 nn += n; 
 p = pp+left; 
 d = left - A; 
 if (1) r=r1+d*dxr, g=g1+d*dxg, b=b1+d*dxb; 
 if (0) o=o1+d*dxo, obar=1.0-o; 
 z=z1+d*dxz; 
 while (--n>=0) { 
 if ((z>p->z && z<p->front)) { 
 { p->c.r=o*r+obar*p->c.r; p->c.g=o*g+obar*p->c.g; p->c.b=o*b+obar*p->c.b; if (o > 0.5) p->z=z; }; 
 if (1) r+=dxr, g+=dxg, b+=dxb; 
 if (0) o+=dxo, obar=1.0-o; 
 z+=dxz; 
 p++; 
 } else { 
 if (1) r+=dxr, g+=dxg, b+=dxb; 
 if (0) o+=dxo, obar=1.0-o; 
 z+=dxz; 
 p++; 
 } 
 } 
 } 
 if (1) r1+=dyr, g1+=dyg, b1+=dyb; 
 if (0) o1+=dyo; 
 z1 += dyz; 
 A+=dyA, B+=dyB; 
 pp+=width; 
 } 
 } 
}
 { 
 
 if (iy3>height) 
 iy3 = height; 
 if (iy2<iy3 && iy3>0 && iy2<=height) { 
 B = x2; 
 d = y3 - y2; 
 d = d? (float)1.0 / (float)d : 0.0; 
 dyB = (float)(x3-x2) * d; 
 if (iy2<0) iy2=0; 
 d = iy2 - y2; 
 B += d*dyB; 
 for (iy=iy2; iy<iy3; iy++) { 
 iA = ((int)((A)-30000.0)+30000); 
 iB = ((int)((B)-30000.0)+30000); 
 if (iB>iA) left=iA, right=iB; 
 else left=iB, right=iA; 
 if (left<0) left=0; 
 if (right>width) right=width; 
 n = right - left; 
 if (n>0) { 
 nn += n; 
 p = pp+left; 
 d = left - A; 
 if (1) r=r1+d*dxr, g=g1+d*dxg, b=b1+d*dxb; 
 if (0) o=o1+d*dxo, obar=1.0-o; 
 z=z1+d*dxz; 
 while (--n>=0) { 
 if ((z>p->z && z<p->front)) { 
 { p->c.r=o*r+obar*p->c.r; p->c.g=o*g+obar*p->c.g; p->c.b=o*b+obar*p->c.b; if (o > 0.5) p->z=z; }; 
 if (1) r+=dxr, g+=dxg, b+=dxb; 
 if (0) o+=dxo, obar=1.0-o; 
 z+=dxz; 
 p++; 
 } else { 
 if (1) r+=dxr, g+=dxg, b+=dxb; 
 if (0) o+=dxo, obar=1.0-o; 
 z+=dxz; 
 p++; 
 } 
 } 
 } 
 if (1) r1+=dyr, g1+=dyg, b1+=dyb; 
 if (0) o1+=dyo; 
 z1 += dyz; 
 A+=dyA, B+=dyB; 
 pp+=width; 
 } 
 } 
}
 buf->pixels += nn;
};
 CAT(ttcdc,1) :
 continue;
 }
}
                                    ;
    } else if (buf->pix_type==pix_fast) {
      {
 Point *p;
 int v1, v2, v3, i, width, height;
 int i1, i2, i3;
 float ox, oy;
 InvalidComponentHandle iElts;
 RGBColor vcolors[6];
 int shademe;
 char fcst = xf->fcst, bcst = xf->bcst, ncst = xf->ncst;
 char fbyte = xf->fbyte, bbyte = xf->bbyte;
 iElts = (invalid_status == INV_UNKNOWN) ? xf->iElts : NULL;
 width = buf->width, height = buf->height;
 ox = buf->ox, oy = buf->oy;
 if (1 && xf->lights) {
 fcolors = (Pointer)vcolors;
 bcolors = (Pointer)(vcolors+3);
 _dxfInitApplyLights(xf->kaf, xf->kdf, xf->ksf, xf->kspf,
 xf->kab, xf->kdb, xf->ksb, xf->kspb,
 xf->fcolors, xf->bcolors, xf->cmap,
 xf->normals, xf->lights,
 xf->colors_dep, xf->normals_dep,
 fcolors, bcolors, 3, fcst, bcst, ncst,
 fbyte, bbyte);
 cmap = NULL;
 shademe = 1;
 } else {
 shademe = 0;
 if (fbyte || bbyte)
 _dxf_initUbyteToFloat();
 }
 for (i=0; i<ntri; i++, tri++) {
 int index = (indices == NULL) ? i : indices[i];
 if (iElts && DXIsElementInvalid(iElts, index))
 continue;
 v1=tri->p, p= &xf->positions[v1], x1=p->x, y1=p->y, z1=p->z;
 v2=tri->q, p= &xf->positions[v2], x2=p->x, y2=p->y, z2=p->z;
 v3=tri->r, p= &xf->positions[v3], x3=p->x, y3=p->y, z3=p->z;
 if (shademe)
 {
 i1 = 0; i2 = 1; i3 = 2;
 if (! _dxfApplyLights((int *)tri, &index, 1))
 return ERROR;
 }
 if (xf->tile.perspective) {
 if (z1>nearPlane || z2>nearPlane || z3>nearPlane)
 continue;
 z1=(float)-1.0/z1; x1*=z1; y1*=z1;
 z2=(float)-1.0/z2; x2*=z2; y2*=z2;
 z3=(float)-1.0/z3; x3*=z3; y3*=z3;
 }
 if ((x1<minx && x2<minx && x3<minx) ||
 (x1>maxx && x2>maxx && x3>maxx) ||
 (y1<miny && y2<miny && y3<miny) ||
 (y1>maxy && y2>maxy && y3>maxy))
 continue;
 buf->triangles++;
 {
 struct fast *pp, *p;
 Pointer colors;
 int cstcolors;
 RGBColor cbuf1, cbuf2, cbuf3;
 char cbyte;
 char obyte = xf->obyte;
 x1+=ox, y1+=oy;
 x2+=ox, y2+=oy;
 x3+=ox, y3+=oy;
 if (x1<-(DXD_MAX_INT/2) || x1>(DXD_MAX_INT/2) || y1<-(DXD_MAX_INT/2) || y1>(DXD_MAX_INT/2) ||
 x2<-(DXD_MAX_INT/2) || x2>(DXD_MAX_INT/2) || y2<-(DXD_MAX_INT/2) || y2>(DXD_MAX_INT/2) ||
 x3<-(DXD_MAX_INT/2) || x3>(DXD_MAX_INT/2) || y3<-(DXD_MAX_INT/2) || y3>(DXD_MAX_INT/2))
 DXErrorReturn(ERROR_BAD_PARAMETER,
 "camera causes numerical overflow");
 d1 = y3-y2, d2 = y1-y3, d3 = y2-y1;
 d = x1*d1 + x2*d2 + x3*d3;
 Qx = d? (float)1.0 / (float)d : 0.0;
 d1 *= Qx, d2 *= Qx, d3 *= Qx;
 if (1) {
 if (d < 0)
 cbyte = fbyte, colors = fcolors, cstcolors = xf->fcst;
 else
 cbyte = bbyte, colors = bcolors, cstcolors = xf->bcst;
 if (!colors) goto CAT(ttfdc,1);
 }
 if (1) {
 if (shademe) {
 c1 = ((RGBColor *)colors) + i1;
 c2 = ((RGBColor *)colors) + i2;
 c3 = ((RGBColor *)colors) + i3;
 } else {
 if (cmap)
 c1= &(cmap[((unsigned char *)colors)[cstcolors ? 0 : v1]]), c2= &(cmap[((unsigned char *)colors)[cstcolors ? 0 : v2]]), c3= &(cmap[((unsigned char *)colors)[cstcolors ? 0 : v3]]);
 else {
 
{ 
 if (cbyte) 
 { 
 RGBByteColor *ptr=((RGBByteColor *)colors)+(cstcolors ? 0 : v1); 
 cbuf1.r = _dxd_ubyteToFloat[ptr->r]; 
 cbuf1.g = _dxd_ubyteToFloat[ptr->g]; 
 cbuf1.b = _dxd_ubyteToFloat[ptr->b]; 
 } else 
 cbuf1 = (((RGBColor *)colors)[cstcolors ? 0 : v1]); 
}; 
{ 
 if (cbyte) 
 { 
 RGBByteColor *ptr=((RGBByteColor *)colors)+(cstcolors ? 0 : v2); 
 cbuf2.r = _dxd_ubyteToFloat[ptr->r]; 
 cbuf2.g = _dxd_ubyteToFloat[ptr->g]; 
 cbuf2.b = _dxd_ubyteToFloat[ptr->b]; 
 } else 
 cbuf2 = (((RGBColor *)colors)[cstcolors ? 0 : v2]); 
}; 
{ 
 if (cbyte) 
 { 
 RGBByteColor *ptr=((RGBByteColor *)colors)+(cstcolors ? 0 : v3); 
 cbuf3.r = _dxd_ubyteToFloat[ptr->r]; 
 cbuf3.g = _dxd_ubyteToFloat[ptr->g]; 
 cbuf3.b = _dxd_ubyteToFloat[ptr->b]; 
 } else 
 cbuf3 = (((RGBColor *)colors)[cstcolors ? 0 : v3]); 
};
 c1 = &cbuf1; c2 = &cbuf2; c3 = &cbuf3;
 }
 }
 r1=c1->r, g1=c1->g, b1=c1->b;
 r2=c2->r, g2=c2->g, b2=c2->b;
 r3=c3->r, g3=c3->g, b3=c3->b;
 } else if (1) {
 if (cmap){
 r=(cmap[((unsigned char *)colors)[cstcolors ? 0 : index]]).r; g=(cmap[((unsigned char *)colors)[cstcolors ? 0 : index]]).g; b=(cmap[((unsigned char *)colors)[cstcolors ? 0 : index]]).b;
 } else {
 
{ 
 if (cbyte) 
 { 
 RGBByteColor *ptr=((RGBByteColor *)colors)+(cstcolors ? 0 : index); 
 cbuf1.r = _dxd_ubyteToFloat[ptr->r]; 
 cbuf1.g = _dxd_ubyteToFloat[ptr->g]; 
 cbuf1.b = _dxd_ubyteToFloat[ptr->b]; 
 } else 
 cbuf1 = (((RGBColor *)colors)[cstcolors ? 0 : index]); 
};
 r = cbuf1.r; g = cbuf1.g; b = cbuf1.b;
 }
 }
 if (0) {
 if (omap) o1=(omap[((unsigned char *)opacities)[xf->ocst ? 0 : v1]]), o2=(omap[((unsigned char *)opacities)[xf->ocst ? 0 : v2]]), o3=(omap[((unsigned char *)opacities)[xf->ocst ? 0 : v3]]);
 else {
 
{ 
 if (obyte) 
 o1 = _dxd_ubyteToFloat[((ubyte *)opacities)[xf->ocst ? 0 : v1]]; 
 else 
 o1 = (((float *)opacities)[xf->ocst ? 0 : v1]); 
}; 
{ 
 if (obyte) 
 o2 = _dxd_ubyteToFloat[((ubyte *)opacities)[xf->ocst ? 0 : v2]]; 
 else 
 o2 = (((float *)opacities)[xf->ocst ? 0 : v2]); 
}; 
{ 
 if (obyte) 
 o3 = _dxd_ubyteToFloat[((ubyte *)opacities)[xf->ocst ? 0 : v3]]; 
 else 
 o3 = (((float *)opacities)[xf->ocst ? 0 : v3]); 
};
 }
 } else if (opacities||omap) {
 if (omap) o=(omap[((unsigned char *)opacities)[xf->ocst ? 0 : index]]);
 else 
{ 
 if (obyte) 
 o = _dxd_ubyteToFloat[((ubyte *)opacities)[xf->ocst ? 0 : index]]; 
 else 
 o = (((float *)opacities)[xf->ocst ? 0 : index]); 
};
 obar = 1.0-o;
 } else
 o = 1.0, obar = 0.0;
 if (1) {
 dxr = (r1 == r2 && r1 == r3) ? 0.0 : d1*r1 + d2*r2 + d3*r3;
 dxg = (g1 == g2 && g1 == g3) ? 0.0 : d1*g1 + d2*g2 + d3*g3;
 dxb = (b1 == b2 && b1 == b3) ? 0.0 : d1*b1 + d2*b2 + d3*b3;
 }
 if (0) dxo = (o1 == o2 && o1 == o3) ? 0.0 : d1*o1 + d2*o2 + d3*o3;
 dxz = (z1 == z2 && z1 == z3) ? 0.0 : d1*z1 + d2*z2 + d3*z3;
 { 
 if (y2 < y1) { 
 (d=y1, y1=y2, y2=d), (d=x1, x1=x2, x2=d); 
 if (1) (d=r1, r1=r2, r2=d), (d=g1, g1=g2, g2=d), (d=b1, b1=b2, b2=d); 
 if (0) (d=o1, o1=o2, o2=d); 
 (d=z1, z1=z2, z2=d); 
 } 
}
 { 
 if (y3 < y1) { 
 (d=y1, y1=y3, y3=d), (d=x1, x1=x3, x3=d); 
 if (1) (d=r1, r1=r3, r3=d), (d=g1, g1=g3, g3=d), (d=b1, b1=b3, b3=d); 
 if (0) (d=o1, o1=o3, o3=d); 
 (d=z1, z1=z3, z3=d); 
 } 
}
 { 
 if (y3 < y2) { 
 (d=y2, y2=y3, y3=d), (d=x2, x2=x3, x3=d); 
 if (1) (d=r2, r2=r3, r3=d), (d=g2, g2=g3, g3=d), (d=b2, b2=b3, b3=d); 
 if (0) (d=o2, o2=o3, o3=d); 
 (d=z2, z2=z3, z3=d); 
 } 
}
 if (0) {
 A = B = x1;
 if (x2<A) A = x2; else B = x2;
 if (x3<A) A = x3; else if (x3>B) B = x3;
 if (A<buf->fmin.x) buf->fmin.x = A;
 if (B>buf->fmax.x) buf->fmax.x = B;
 if (y1<buf->fmin.y) buf->fmin.y = y1;
 if (y3>buf->fmax.y) buf->fmax.y = y3;
 }
 iy1 = ((int)((y1)-30000.0)+30000), iy2 = ((int)((y2)-30000.0)+30000), iy3 = ((int)((y3)-30000.0)+30000);
 if (iy1>iy2 || iy2>iy3) {
 DXSetError(ERROR_DATA_INVALID,
 "position number %d, %d, or %d is invalid", v1, v2, v3);
 return ERROR;
 }
 if (iy1==iy3)
 goto CAT(ttfdc,1);
 d = y3 - y1;
 Qy = d? (float)1.0 / (float)d : 0.0;
 if (1) dyr=(r3-r1)*Qy, dyg=(g3-g1)*Qy, dyb=(b3-b1)*Qy;
 if (0) dyo = (float)(o3-o1) * Qy;
 dyz = (z3-z1) * Qy;
 dyA = (float)(x3-x1) * Qy;
 A = x1;
 pp = buf->u.fast + iy1*width;
 d = iy1 - y1;
 if (iy1<0)
 d-=iy1, pp=buf->u.fast;
 A += dyA*d;
 if (1) r1+=dyr*d, g1+=dyg*d, b1+=dyb*d;
 if (0) o1+=dyo*d;
 z1 += dyz*d;
 nn = 0;
 { 
 
 if (iy2>height) 
 iy2 = height; 
 if (iy1<iy2 && iy2>0 && iy1<=height) { 
 B = x1; 
 d = y2 - y1; 
 d = d? (float)1.0 / (float)d : 0.0; 
 dyB = (float)(x2-x1) * d; 
 if (iy1<0) iy1=0; 
 d = iy1 - y1; 
 B += d*dyB; 
 for (iy=iy1; iy<iy2; iy++) { 
 iA = ((int)((A)-30000.0)+30000); 
 iB = ((int)((B)-30000.0)+30000); 
 if (iB>iA) left=iA, right=iB; 
 else left=iB, right=iA; 
 if (left<0) left=0; 
 if (right>width) right=width; 
 n = right - left; 
 if (n>0) { 
 nn += n; 
 p = pp+left; 
 d = left - A; 
 if (1) r=r1+d*dxr, g=g1+d*dxg, b=b1+d*dxb; 
 if (0) o=o1+d*dxo, obar=1.0-o; 
 z=z1+d*dxz; 
 while (--n>=0) { 
 if ((z>p->z)) { 
 { p->c.r=o*r+obar*p->c.r; p->c.g=o*g+obar*p->c.g; p->c.b=o*b+obar*p->c.b; if (o > 0.5) p->z=z; }; 
 if (1) r+=dxr, g+=dxg, b+=dxb; 
 if (0) o+=dxo, obar=1.0-o; 
 z+=dxz; 
 p++; 
 } else { 
 if (1) r+=dxr, g+=dxg, b+=dxb; 
 if (0) o+=dxo, obar=1.0-o; 
 z+=dxz; 
 p++; 
 } 
 } 
 } 
 if (1) r1+=dyr, g1+=dyg, b1+=dyb; 
 if (0) o1+=dyo; 
 z1 += dyz; 
 A+=dyA, B+=dyB; 
 pp+=width; 
 } 
 } 
}
 { 
 
 if (iy3>height) 
 iy3 = height; 
 if (iy2<iy3 && iy3>0 && iy2<=height) { 
 B = x2; 
 d = y3 - y2; 
 d = d? (float)1.0 / (float)d : 0.0; 
 dyB = (float)(x3-x2) * d; 
 if (iy2<0) iy2=0; 
 d = iy2 - y2; 
 B += d*dyB; 
 for (iy=iy2; iy<iy3; iy++) { 
 iA = ((int)((A)-30000.0)+30000); 
 iB = ((int)((B)-30000.0)+30000); 
 if (iB>iA) left=iA, right=iB; 
 else left=iB, right=iA; 
 if (left<0) left=0; 
 if (right>width) right=width; 
 n = right - left; 
 if (n>0) { 
 nn += n; 
 p = pp+left; 
 d = left - A; 
 if (1) r=r1+d*dxr, g=g1+d*dxg, b=b1+d*dxb; 
 if (0) o=o1+d*dxo, obar=1.0-o; 
 z=z1+d*dxz; 
 while (--n>=0) { 
 if ((z>p->z)) { 
 { p->c.r=o*r+obar*p->c.r; p->c.g=o*g+obar*p->c.g; p->c.b=o*b+obar*p->c.b; if (o > 0.5) p->z=z; }; 
 if (1) r+=dxr, g+=dxg, b+=dxb; 
 if (0) o+=dxo, obar=1.0-o; 
 z+=dxz; 
 p++; 
 } else { 
 if (1) r+=dxr, g+=dxg, b+=dxb; 
 if (0) o+=dxo, obar=1.0-o; 
 z+=dxz; 
 p++; 
 } 
 } 
 } 
 if (1) r1+=dyr, g1+=dyg, b1+=dyb; 
 if (0) o1+=dyo; 
 z1 += dyz; 
 A+=dyA, B+=dyB; 
 pp+=width; 
 } 
 } 
}
 buf->pixels += nn;
};
 CAT(ttfdc,1) :
 continue;
 }
}
                                 ;
    } else if (buf->pix_type==pix_big) {
      {
 Point *p;
 int v1, v2, v3, i, width, height;
 int i1, i2, i3;
 float ox, oy;
 InvalidComponentHandle iElts;
 RGBColor vcolors[6];
 int shademe;
 char fcst = xf->fcst, bcst = xf->bcst, ncst = xf->ncst;
 char fbyte = xf->fbyte, bbyte = xf->bbyte;
 iElts = (invalid_status == INV_UNKNOWN) ? xf->iElts : NULL;
 width = buf->width, height = buf->height;
 ox = buf->ox, oy = buf->oy;
 if (1 && xf->lights) {
 fcolors = (Pointer)vcolors;
 bcolors = (Pointer)(vcolors+3);
 _dxfInitApplyLights(xf->kaf, xf->kdf, xf->ksf, xf->kspf,
 xf->kab, xf->kdb, xf->ksb, xf->kspb,
 xf->fcolors, xf->bcolors, xf->cmap,
 xf->normals, xf->lights,
 xf->colors_dep, xf->normals_dep,
 fcolors, bcolors, 3, fcst, bcst, ncst,
 fbyte, bbyte);
 cmap = NULL;
 shademe = 1;
 } else {
 shademe = 0;
 if (fbyte || bbyte)
 _dxf_initUbyteToFloat();
 }
 for (i=0; i<ntri; i++, tri++) {
 int index = (indices == NULL) ? i : indices[i];
 if (iElts && DXIsElementInvalid(iElts, index))
 continue;
 v1=tri->p, p= &xf->positions[v1], x1=p->x, y1=p->y, z1=p->z;
 v2=tri->q, p= &xf->positions[v2], x2=p->x, y2=p->y, z2=p->z;
 v3=tri->r, p= &xf->positions[v3], x3=p->x, y3=p->y, z3=p->z;
 if (shademe)
 {
 i1 = 0; i2 = 1; i3 = 2;
 if (! _dxfApplyLights((int *)tri, &index, 1))
 return ERROR;
 }
 if (xf->tile.perspective) {
 if (z1>nearPlane || z2>nearPlane || z3>nearPlane)
 continue;
 z1=(float)-1.0/z1; x1*=z1; y1*=z1;
 z2=(float)-1.0/z2; x2*=z2; y2*=z2;
 z3=(float)-1.0/z3; x3*=z3; y3*=z3;
 }
 if ((x1<minx && x2<minx && x3<minx) ||
 (x1>maxx && x2>maxx && x3>maxx) ||
 (y1<miny && y2<miny && y3<miny) ||
 (y1>maxy && y2>maxy && y3>maxy))
 continue;
 buf->triangles++;
 {
 struct big *pp, *p;
 Pointer colors;
 int cstcolors;
 RGBColor cbuf1, cbuf2, cbuf3;
 char cbyte;
 char obyte = xf->obyte;
 x1+=ox, y1+=oy;
 x2+=ox, y2+=oy;
 x3+=ox, y3+=oy;
 if (x1<-(DXD_MAX_INT/2) || x1>(DXD_MAX_INT/2) || y1<-(DXD_MAX_INT/2) || y1>(DXD_MAX_INT/2) ||
 x2<-(DXD_MAX_INT/2) || x2>(DXD_MAX_INT/2) || y2<-(DXD_MAX_INT/2) || y2>(DXD_MAX_INT/2) ||
 x3<-(DXD_MAX_INT/2) || x3>(DXD_MAX_INT/2) || y3<-(DXD_MAX_INT/2) || y3>(DXD_MAX_INT/2))
 DXErrorReturn(ERROR_BAD_PARAMETER,
 "camera causes numerical overflow");
 d1 = y3-y2, d2 = y1-y3, d3 = y2-y1;
 d = x1*d1 + x2*d2 + x3*d3;
 Qx = d? (float)1.0 / (float)d : 0.0;
 d1 *= Qx, d2 *= Qx, d3 *= Qx;
 if (1) {
 if (d < 0)
 cbyte = fbyte, colors = fcolors, cstcolors = xf->fcst;
 else
 cbyte = bbyte, colors = bcolors, cstcolors = xf->bcst;
 if (!colors) goto CAT(ttbdc,1);
 }
 if (1) {
 if (shademe) {
 c1 = ((RGBColor *)colors) + i1;
 c2 = ((RGBColor *)colors) + i2;
 c3 = ((RGBColor *)colors) + i3;
 } else {
 if (cmap)
 c1= &(cmap[((unsigned char *)colors)[cstcolors ? 0 : v1]]), c2= &(cmap[((unsigned char *)colors)[cstcolors ? 0 : v2]]), c3= &(cmap[((unsigned char *)colors)[cstcolors ? 0 : v3]]);
 else {
 
{ 
 if (cbyte) 
 { 
 RGBByteColor *ptr=((RGBByteColor *)colors)+(cstcolors ? 0 : v1); 
 cbuf1.r = _dxd_ubyteToFloat[ptr->r]; 
 cbuf1.g = _dxd_ubyteToFloat[ptr->g]; 
 cbuf1.b = _dxd_ubyteToFloat[ptr->b]; 
 } else 
 cbuf1 = (((RGBColor *)colors)[cstcolors ? 0 : v1]); 
}; 
{ 
 if (cbyte) 
 { 
 RGBByteColor *ptr=((RGBByteColor *)colors)+(cstcolors ? 0 : v2); 
 cbuf2.r = _dxd_ubyteToFloat[ptr->r]; 
 cbuf2.g = _dxd_ubyteToFloat[ptr->g]; 
 cbuf2.b = _dxd_ubyteToFloat[ptr->b]; 
 } else 
 cbuf2 = (((RGBColor *)colors)[cstcolors ? 0 : v2]); 
}; 
{ 
 if (cbyte) 
 { 
 RGBByteColor *ptr=((RGBByteColor *)colors)+(cstcolors ? 0 : v3); 
 cbuf3.r = _dxd_ubyteToFloat[ptr->r]; 
 cbuf3.g = _dxd_ubyteToFloat[ptr->g]; 
 cbuf3.b = _dxd_ubyteToFloat[ptr->b]; 
 } else 
 cbuf3 = (((RGBColor *)colors)[cstcolors ? 0 : v3]); 
};
 c1 = &cbuf1; c2 = &cbuf2; c3 = &cbuf3;
 }
 }
 r1=c1->r, g1=c1->g, b1=c1->b;
 r2=c2->r, g2=c2->g, b2=c2->b;
 r3=c3->r, g3=c3->g, b3=c3->b;
 } else if (1) {
 if (cmap){
 r=(cmap[((unsigned char *)colors)[cstcolors ? 0 : index]]).r; g=(cmap[((unsigned char *)colors)[cstcolors ? 0 : index]]).g; b=(cmap[((unsigned char *)colors)[cstcolors ? 0 : index]]).b;
 } else {
 
{ 
 if (cbyte) 
 { 
 RGBByteColor *ptr=((RGBByteColor *)colors)+(cstcolors ? 0 : index); 
 cbuf1.r = _dxd_ubyteToFloat[ptr->r]; 
 cbuf1.g = _dxd_ubyteToFloat[ptr->g]; 
 cbuf1.b = _dxd_ubyteToFloat[ptr->b]; 
 } else 
 cbuf1 = (((RGBColor *)colors)[cstcolors ? 0 : index]); 
};
 r = cbuf1.r; g = cbuf1.g; b = cbuf1.b;
 }
 }
 if (0) {
 if (omap) o1=(omap[((unsigned char *)opacities)[xf->ocst ? 0 : v1]]), o2=(omap[((unsigned char *)opacities)[xf->ocst ? 0 : v2]]), o3=(omap[((unsigned char *)opacities)[xf->ocst ? 0 : v3]]);
 else {
 
{ 
 if (obyte) 
 o1 = _dxd_ubyteToFloat[((ubyte *)opacities)[xf->ocst ? 0 : v1]]; 
 else 
 o1 = (((float *)opacities)[xf->ocst ? 0 : v1]); 
}; 
{ 
 if (obyte) 
 o2 = _dxd_ubyteToFloat[((ubyte *)opacities)[xf->ocst ? 0 : v2]]; 
 else 
 o2 = (((float *)opacities)[xf->ocst ? 0 : v2]); 
}; 
{ 
 if (obyte) 
 o3 = _dxd_ubyteToFloat[((ubyte *)opacities)[xf->ocst ? 0 : v3]]; 
 else 
 o3 = (((float *)opacities)[xf->ocst ? 0 : v3]); 
};
 }
 } else if (opacities||omap) {
 if (omap) o=(omap[((unsigned char *)opacities)[xf->ocst ? 0 : index]]);
 else 
{ 
 if (obyte) 
 o = _dxd_ubyteToFloat[((ubyte *)opacities)[xf->ocst ? 0 : index]]; 
 else 
 o = (((float *)opacities)[xf->ocst ? 0 : index]); 
};
 obar = 1.0-o;
 } else
 o = 1.0, obar = 0.0;
 if (1) {
 dxr = (r1 == r2 && r1 == r3) ? 0.0 : d1*r1 + d2*r2 + d3*r3;
 dxg = (g1 == g2 && g1 == g3) ? 0.0 : d1*g1 + d2*g2 + d3*g3;
 dxb = (b1 == b2 && b1 == b3) ? 0.0 : d1*b1 + d2*b2 + d3*b3;
 }
 if (0) dxo = (o1 == o2 && o1 == o3) ? 0.0 : d1*o1 + d2*o2 + d3*o3;
 dxz = (z1 == z2 && z1 == z3) ? 0.0 : d1*z1 + d2*z2 + d3*z3;
 { 
 if (y2 < y1) { 
 (d=y1, y1=y2, y2=d), (d=x1, x1=x2, x2=d); 
 if (1) (d=r1, r1=r2, r2=d), (d=g1, g1=g2, g2=d), (d=b1, b1=b2, b2=d); 
 if (0) (d=o1, o1=o2, o2=d); 
 (d=z1, z1=z2, z2=d); 
 } 
}
 { 
 if (y3 < y1) { 
 (d=y1, y1=y3, y3=d), (d=x1, x1=x3, x3=d); 
 if (1) (d=r1, r1=r3, r3=d), (d=g1, g1=g3, g3=d), (d=b1, b1=b3, b3=d); 
 if (0) (d=o1, o1=o3, o3=d); 
 (d=z1, z1=z3, z3=d); 
 } 
}
 { 
 if (y3 < y2) { 
 (d=y2, y2=y3, y3=d), (d=x2, x2=x3, x3=d); 
 if (1) (d=r2, r2=r3, r3=d), (d=g2, g2=g3, g3=d), (d=b2, b2=b3, b3=d); 
 if (0) (d=o2, o2=o3, o3=d); 
 (d=z2, z2=z3, z3=d); 
 } 
}
 if (0) {
 A = B = x1;
 if (x2<A) A = x2; else B = x2;
 if (x3<A) A = x3; else if (x3>B) B = x3;
 if (A<buf->fmin.x) buf->fmin.x = A;
 if (B>buf->fmax.x) buf->fmax.x = B;
 if (y1<buf->fmin.y) buf->fmin.y = y1;
 if (y3>buf->fmax.y) buf->fmax.y = y3;
 }
 iy1 = ((int)((y1)-30000.0)+30000), iy2 = ((int)((y2)-30000.0)+30000), iy3 = ((int)((y3)-30000.0)+30000);
 if (iy1>iy2 || iy2>iy3) {
 DXSetError(ERROR_DATA_INVALID,
 "position number %d, %d, or %d is invalid", v1, v2, v3);
 return ERROR;
 }
 if (iy1==iy3)
 goto CAT(ttbdc,1);
 d = y3 - y1;
 Qy = d? (float)1.0 / (float)d : 0.0;
 if (1) dyr=(r3-r1)*Qy, dyg=(g3-g1)*Qy, dyb=(b3-b1)*Qy;
 if (0) dyo = (float)(o3-o1) * Qy;
 dyz = (z3-z1) * Qy;
 dyA = (float)(x3-x1) * Qy;
 A = x1;
 pp = buf->u.big + iy1*width;
 d = iy1 - y1;
 if (iy1<0)
 d-=iy1, pp=buf->u.big;
 A += dyA*d;
 if (1) r1+=dyr*d, g1+=dyg*d, b1+=dyb*d;
 if (0) o1+=dyo*d;
 z1 += dyz*d;
 nn = 0;
 { 
 
 if (iy2>height) 
 iy2 = height; 
 if (iy1<iy2 && iy2>0 && iy1<=height) { 
 B = x1; 
 d = y2 - y1; 
 d = d? (float)1.0 / (float)d : 0.0; 
 dyB = (float)(x2-x1) * d; 
 if (iy1<0) iy1=0; 
 d = iy1 - y1; 
 B += d*dyB; 
 for (iy=iy1; iy<iy2; iy++) { 
 iA = ((int)((A)-30000.0)+30000); 
 iB = ((int)((B)-30000.0)+30000); 
 if (iB>iA) left=iA, right=iB; 
 else left=iB, right=iA; 
 if (left<0) left=0; 
 if (right>width) right=width; 
 n = right - left; 
 if (n>0) { 
 nn += n; 
 p = pp+left; 
 d = left - A; 
 if (1) r=r1+d*dxr, g=g1+d*dxg, b=b1+d*dxb; 
 if (0) o=o1+d*dxo, obar=1.0-o; 
 z=z1+d*dxz; 
 while (--n>=0) { 
 if ((z>p->z)) { 
 { p->c.r=o*r+obar*p->c.r; p->c.g=o*g+obar*p->c.g; p->c.b=o*b+obar*p->c.b; if (o > 0.5) p->z=z; }; 
 if (1) r+=dxr, g+=dxg, b+=dxb; 
 if (0) o+=dxo, obar=1.0-o; 
 z+=dxz; 
 p++; 
 } else { 
 if (1) r+=dxr, g+=dxg, b+=dxb; 
 if (0) o+=dxo, obar=1.0-o; 
 z+=dxz; 
 p++; 
 } 
 } 
 } 
 if (1) r1+=dyr, g1+=dyg, b1+=dyb; 
 if (0) o1+=dyo; 
 z1 += dyz; 
 A+=dyA, B+=dyB; 
 pp+=width; 
 } 
 } 
}
 { 
 
 if (iy3>height) 
 iy3 = height; 
 if (iy2<iy3 && iy3>0 && iy2<=height) { 
 B = x2; 
 d = y3 - y2; 
 d = d? (float)1.0 / (float)d : 0.0; 
 dyB = (float)(x3-x2) * d; 
 if (iy2<0) iy2=0; 
 d = iy2 - y2; 
 B += d*dyB; 
 for (iy=iy2; iy<iy3; iy++) { 
 iA = ((int)((A)-30000.0)+30000); 
 iB = ((int)((B)-30000.0)+30000); 
 if (iB>iA) left=iA, right=iB; 
 else left=iB, right=iA; 
 if (left<0) left=0; 
 if (right>width) right=width; 
 n = right - left; 
 if (n>0) { 
 nn += n; 
 p = pp+left; 
 d = left - A; 
 if (1) r=r1+d*dxr, g=g1+d*dxg, b=b1+d*dxb; 
 if (0) o=o1+d*dxo, obar=1.0-o; 
 z=z1+d*dxz; 
 while (--n>=0) { 
 if ((z>p->z)) { 
 { p->c.r=o*r+obar*p->c.r; p->c.g=o*g+obar*p->c.g; p->c.b=o*b+obar*p->c.b; if (o > 0.5) p->z=z; }; 
 if (1) r+=dxr, g+=dxg, b+=dxb; 
 if (0) o+=dxo, obar=1.0-o; 
 z+=dxz; 
 p++; 
 } else { 
 if (1) r+=dxr, g+=dxg, b+=dxb; 
 if (0) o+=dxo, obar=1.0-o; 
 z+=dxz; 
 p++; 
 } 
 } 
 } 
 if (1) r1+=dyr, g1+=dyg, b1+=dyb; 
 if (0) o1+=dyo; 
 z1 += dyz; 
 A+=dyA, B+=dyB; 
 pp+=width; 
 } 
 } 
}
 buf->pixels += nn;
};
 CAT(ttbdc,1) :
 continue;
 }
}
                                 ;
    } else {
      DXSetError(ERROR_INTERNAL, "unknown pix_type %d", buf->pix_type);
      return ERROR;
    }
  }
  else
  {
    if (clip_status) {
      ASSERT(buf->pix_type==pix_big);
      {
 Point *p;
 int v1, v2, v3, i, width, height;
 int i1, i2, i3;
 float ox, oy;
 InvalidComponentHandle iElts;
 RGBColor vcolors[6];
 int shademe;
 char fcst = xf->fcst, bcst = xf->bcst, ncst = xf->ncst;
 char fbyte = xf->fbyte, bbyte = xf->bbyte;
 iElts = (invalid_status == INV_UNKNOWN) ? xf->iElts : NULL;
 width = buf->width, height = buf->height;
 ox = buf->ox, oy = buf->oy;
 if (1 && xf->lights) {
 fcolors = (Pointer)vcolors;
 bcolors = (Pointer)(vcolors+3);
 _dxfInitApplyLights(xf->kaf, xf->kdf, xf->ksf, xf->kspf,
 xf->kab, xf->kdb, xf->ksb, xf->kspb,
 xf->fcolors, xf->bcolors, xf->cmap,
 xf->normals, xf->lights,
 xf->colors_dep, xf->normals_dep,
 fcolors, bcolors, 3, fcst, bcst, ncst,
 fbyte, bbyte);
 cmap = NULL;
 shademe = 1;
 } else {
 shademe = 0;
 if (fbyte || bbyte)
 _dxf_initUbyteToFloat();
 }
 for (i=0; i<ntri; i++, tri++) {
 int index = (indices == NULL) ? i : indices[i];
 if (iElts && DXIsElementInvalid(iElts, index))
 continue;
 v1=tri->p, p= &xf->positions[v1], x1=p->x, y1=p->y, z1=p->z;
 v2=tri->q, p= &xf->positions[v2], x2=p->x, y2=p->y, z2=p->z;
 v3=tri->r, p= &xf->positions[v3], x3=p->x, y3=p->y, z3=p->z;
 if (shademe)
 {
 i1 = 0; i2 = 1; i3 = 2;
 if (! _dxfApplyLights((int *)tri, &index, 1))
 return ERROR;
 }
 if (xf->tile.perspective) {
 if (z1>nearPlane || z2>nearPlane || z3>nearPlane)
 continue;
 z1=(float)-1.0/z1; x1*=z1; y1*=z1;
 z2=(float)-1.0/z2; x2*=z2; y2*=z2;
 z3=(float)-1.0/z3; x3*=z3; y3*=z3;
 }
 if ((x1<minx && x2<minx && x3<minx) ||
 (x1>maxx && x2>maxx && x3>maxx) ||
 (y1<miny && y2<miny && y3<miny) ||
 (y1>maxy && y2>maxy && y3>maxy))
 continue;
 buf->triangles++;
 {
 struct big *pp, *p;
 Pointer colors;
 int cstcolors;
 RGBColor cbuf1, cbuf2, cbuf3;
 char cbyte;
 char obyte = xf->obyte;
 x1+=ox, y1+=oy;
 x2+=ox, y2+=oy;
 x3+=ox, y3+=oy;
 if (x1<-(DXD_MAX_INT/2) || x1>(DXD_MAX_INT/2) || y1<-(DXD_MAX_INT/2) || y1>(DXD_MAX_INT/2) ||
 x2<-(DXD_MAX_INT/2) || x2>(DXD_MAX_INT/2) || y2<-(DXD_MAX_INT/2) || y2>(DXD_MAX_INT/2) ||
 x3<-(DXD_MAX_INT/2) || x3>(DXD_MAX_INT/2) || y3<-(DXD_MAX_INT/2) || y3>(DXD_MAX_INT/2))
 DXErrorReturn(ERROR_BAD_PARAMETER,
 "camera causes numerical overflow");
 d1 = y3-y2, d2 = y1-y3, d3 = y2-y1;
 d = x1*d1 + x2*d2 + x3*d3;
 Qx = d? (float)1.0 / (float)d : 0.0;
 d1 *= Qx, d2 *= Qx, d3 *= Qx;
 if (1) {
 if (d < 0)
 cbyte = fbyte, colors = fcolors, cstcolors = xf->fcst;
 else
 cbyte = bbyte, colors = bcolors, cstcolors = xf->bcst;
 if (!colors) goto CAT(ttcdp,1);
 }
 if (1) {
 if (shademe) {
 c1 = ((RGBColor *)colors) + i1;
 c2 = ((RGBColor *)colors) + i2;
 c3 = ((RGBColor *)colors) + i3;
 } else {
 if (cmap)
 c1= &(cmap[((unsigned char *)colors)[cstcolors ? 0 : v1]]), c2= &(cmap[((unsigned char *)colors)[cstcolors ? 0 : v2]]), c3= &(cmap[((unsigned char *)colors)[cstcolors ? 0 : v3]]);
 else {
 
{ 
 if (cbyte) 
 { 
 RGBByteColor *ptr=((RGBByteColor *)colors)+(cstcolors ? 0 : v1); 
 cbuf1.r = _dxd_ubyteToFloat[ptr->r]; 
 cbuf1.g = _dxd_ubyteToFloat[ptr->g]; 
 cbuf1.b = _dxd_ubyteToFloat[ptr->b]; 
 } else 
 cbuf1 = (((RGBColor *)colors)[cstcolors ? 0 : v1]); 
}; 
{ 
 if (cbyte) 
 { 
 RGBByteColor *ptr=((RGBByteColor *)colors)+(cstcolors ? 0 : v2); 
 cbuf2.r = _dxd_ubyteToFloat[ptr->r]; 
 cbuf2.g = _dxd_ubyteToFloat[ptr->g]; 
 cbuf2.b = _dxd_ubyteToFloat[ptr->b]; 
 } else 
 cbuf2 = (((RGBColor *)colors)[cstcolors ? 0 : v2]); 
}; 
{ 
 if (cbyte) 
 { 
 RGBByteColor *ptr=((RGBByteColor *)colors)+(cstcolors ? 0 : v3); 
 cbuf3.r = _dxd_ubyteToFloat[ptr->r]; 
 cbuf3.g = _dxd_ubyteToFloat[ptr->g]; 
 cbuf3.b = _dxd_ubyteToFloat[ptr->b]; 
 } else 
 cbuf3 = (((RGBColor *)colors)[cstcolors ? 0 : v3]); 
};
 c1 = &cbuf1; c2 = &cbuf2; c3 = &cbuf3;
 }
 }
 r1=c1->r, g1=c1->g, b1=c1->b;
 r2=c2->r, g2=c2->g, b2=c2->b;
 r3=c3->r, g3=c3->g, b3=c3->b;
 } else if (1) {
 if (cmap){
 r=(cmap[((unsigned char *)colors)[cstcolors ? 0 : index]]).r; g=(cmap[((unsigned char *)colors)[cstcolors ? 0 : index]]).g; b=(cmap[((unsigned char *)colors)[cstcolors ? 0 : index]]).b;
 } else {
 
{ 
 if (cbyte) 
 { 
 RGBByteColor *ptr=((RGBByteColor *)colors)+(cstcolors ? 0 : index); 
 cbuf1.r = _dxd_ubyteToFloat[ptr->r]; 
 cbuf1.g = _dxd_ubyteToFloat[ptr->g]; 
 cbuf1.b = _dxd_ubyteToFloat[ptr->b]; 
 } else 
 cbuf1 = (((RGBColor *)colors)[cstcolors ? 0 : index]); 
};
 r = cbuf1.r; g = cbuf1.g; b = cbuf1.b;
 }
 }
 if (1) {
 if (omap) o1=(omap[((unsigned char *)opacities)[xf->ocst ? 0 : v1]]), o2=(omap[((unsigned char *)opacities)[xf->ocst ? 0 : v2]]), o3=(omap[((unsigned char *)opacities)[xf->ocst ? 0 : v3]]);
 else {
 
{ 
 if (obyte) 
 o1 = _dxd_ubyteToFloat[((ubyte *)opacities)[xf->ocst ? 0 : v1]]; 
 else 
 o1 = (((float *)opacities)[xf->ocst ? 0 : v1]); 
}; 
{ 
 if (obyte) 
 o2 = _dxd_ubyteToFloat[((ubyte *)opacities)[xf->ocst ? 0 : v2]]; 
 else 
 o2 = (((float *)opacities)[xf->ocst ? 0 : v2]); 
}; 
{ 
 if (obyte) 
 o3 = _dxd_ubyteToFloat[((ubyte *)opacities)[xf->ocst ? 0 : v3]]; 
 else 
 o3 = (((float *)opacities)[xf->ocst ? 0 : v3]); 
};
 }
 } else if (opacities||omap) {
 if (omap) o=(omap[((unsigned char *)opacities)[xf->ocst ? 0 : index]]);
 else 
{ 
 if (obyte) 
 o = _dxd_ubyteToFloat[((ubyte *)opacities)[xf->ocst ? 0 : index]]; 
 else 
 o = (((float *)opacities)[xf->ocst ? 0 : index]); 
};
 obar = 1.0-o;
 } else
 o = 1.0, obar = 0.0;
 if (1) {
 dxr = (r1 == r2 && r1 == r3) ? 0.0 : d1*r1 + d2*r2 + d3*r3;
 dxg = (g1 == g2 && g1 == g3) ? 0.0 : d1*g1 + d2*g2 + d3*g3;
 dxb = (b1 == b2 && b1 == b3) ? 0.0 : d1*b1 + d2*b2 + d3*b3;
 }
 if (1) dxo = (o1 == o2 && o1 == o3) ? 0.0 : d1*o1 + d2*o2 + d3*o3;
 dxz = (z1 == z2 && z1 == z3) ? 0.0 : d1*z1 + d2*z2 + d3*z3;
 { 
 if (y2 < y1) { 
 (d=y1, y1=y2, y2=d), (d=x1, x1=x2, x2=d); 
 if (1) (d=r1, r1=r2, r2=d), (d=g1, g1=g2, g2=d), (d=b1, b1=b2, b2=d); 
 if (1) (d=o1, o1=o2, o2=d); 
 (d=z1, z1=z2, z2=d); 
 } 
}
 { 
 if (y3 < y1) { 
 (d=y1, y1=y3, y3=d), (d=x1, x1=x3, x3=d); 
 if (1) (d=r1, r1=r3, r3=d), (d=g1, g1=g3, g3=d), (d=b1, b1=b3, b3=d); 
 if (1) (d=o1, o1=o3, o3=d); 
 (d=z1, z1=z3, z3=d); 
 } 
}
 { 
 if (y3 < y2) { 
 (d=y2, y2=y3, y3=d), (d=x2, x2=x3, x3=d); 
 if (1) (d=r2, r2=r3, r3=d), (d=g2, g2=g3, g3=d), (d=b2, b2=b3, b3=d); 
 if (1) (d=o2, o2=o3, o3=d); 
 (d=z2, z2=z3, z3=d); 
 } 
}
 if (0) {
 A = B = x1;
 if (x2<A) A = x2; else B = x2;
 if (x3<A) A = x3; else if (x3>B) B = x3;
 if (A<buf->fmin.x) buf->fmin.x = A;
 if (B>buf->fmax.x) buf->fmax.x = B;
 if (y1<buf->fmin.y) buf->fmin.y = y1;
 if (y3>buf->fmax.y) buf->fmax.y = y3;
 }
 iy1 = ((int)((y1)-30000.0)+30000), iy2 = ((int)((y2)-30000.0)+30000), iy3 = ((int)((y3)-30000.0)+30000);
 if (iy1>iy2 || iy2>iy3) {
 DXSetError(ERROR_DATA_INVALID,
 "position number %d, %d, or %d is invalid", v1, v2, v3);
 return ERROR;
 }
 if (iy1==iy3)
 goto CAT(ttcdp,1);
 d = y3 - y1;
 Qy = d? (float)1.0 / (float)d : 0.0;
 if (1) dyr=(r3-r1)*Qy, dyg=(g3-g1)*Qy, dyb=(b3-b1)*Qy;
 if (1) dyo = (float)(o3-o1) * Qy;
 dyz = (z3-z1) * Qy;
 dyA = (float)(x3-x1) * Qy;
 A = x1;
 pp = buf->u.big + iy1*width;
 d = iy1 - y1;
 if (iy1<0)
 d-=iy1, pp=buf->u.big;
 A += dyA*d;
 if (1) r1+=dyr*d, g1+=dyg*d, b1+=dyb*d;
 if (1) o1+=dyo*d;
 z1 += dyz*d;
 nn = 0;
 { 
 
 if (iy2>height) 
 iy2 = height; 
 if (iy1<iy2 && iy2>0 && iy1<=height) { 
 B = x1; 
 d = y2 - y1; 
 d = d? (float)1.0 / (float)d : 0.0; 
 dyB = (float)(x2-x1) * d; 
 if (iy1<0) iy1=0; 
 d = iy1 - y1; 
 B += d*dyB; 
 for (iy=iy1; iy<iy2; iy++) { 
 iA = ((int)((A)-30000.0)+30000); 
 iB = ((int)((B)-30000.0)+30000); 
 if (iB>iA) left=iA, right=iB; 
 else left=iB, right=iA; 
 if (left<0) left=0; 
 if (right>width) right=width; 
 n = right - left; 
 if (n>0) { 
 nn += n; 
 p = pp+left; 
 d = left - A; 
 if (1) r=r1+d*dxr, g=g1+d*dxg, b=b1+d*dxb; 
 if (1) o=o1+d*dxo, obar=1.0-o; 
 z=z1+d*dxz; 
 while (--n>=0) { 
 if ((z>p->z && z<p->front)) { 
 { p->c.r=o*r+obar*p->c.r; p->c.g=o*g+obar*p->c.g; p->c.b=o*b+obar*p->c.b; if (o > 0.5) p->z=z; }; 
 if (1) r+=dxr, g+=dxg, b+=dxb; 
 if (1) o+=dxo, obar=1.0-o; 
 z+=dxz; 
 p++; 
 } else { 
 if (1) r+=dxr, g+=dxg, b+=dxb; 
 if (1) o+=dxo, obar=1.0-o; 
 z+=dxz; 
 p++; 
 } 
 } 
 } 
 if (1) r1+=dyr, g1+=dyg, b1+=dyb; 
 if (1) o1+=dyo; 
 z1 += dyz; 
 A+=dyA, B+=dyB; 
 pp+=width; 
 } 
 } 
}
 { 
 
 if (iy3>height) 
 iy3 = height; 
 if (iy2<iy3 && iy3>0 && iy2<=height) { 
 B = x2; 
 d = y3 - y2; 
 d = d? (float)1.0 / (float)d : 0.0; 
 dyB = (float)(x3-x2) * d; 
 if (iy2<0) iy2=0; 
 d = iy2 - y2; 
 B += d*dyB; 
 for (iy=iy2; iy<iy3; iy++) { 
 iA = ((int)((A)-30000.0)+30000); 
 iB = ((int)((B)-30000.0)+30000); 
 if (iB>iA) left=iA, right=iB; 
 else left=iB, right=iA; 
 if (left<0) left=0; 
 if (right>width) right=width; 
 n = right - left; 
 if (n>0) { 
 nn += n; 
 p = pp+left; 
 d = left - A; 
 if (1) r=r1+d*dxr, g=g1+d*dxg, b=b1+d*dxb; 
 if (1) o=o1+d*dxo, obar=1.0-o; 
 z=z1+d*dxz; 
 while (--n>=0) { 
 if ((z>p->z && z<p->front)) { 
 { p->c.r=o*r+obar*p->c.r; p->c.g=o*g+obar*p->c.g; p->c.b=o*b+obar*p->c.b; if (o > 0.5) p->z=z; }; 
 if (1) r+=dxr, g+=dxg, b+=dxb; 
 if (1) o+=dxo, obar=1.0-o; 
 z+=dxz; 
 p++; 
 } else { 
 if (1) r+=dxr, g+=dxg, b+=dxb; 
 if (1) o+=dxo, obar=1.0-o; 
 z+=dxz; 
 p++; 
 } 
 } 
 } 
 if (1) r1+=dyr, g1+=dyg, b1+=dyb; 
 if (1) o1+=dyo; 
 z1 += dyz; 
 A+=dyA, B+=dyB; 
 pp+=width; 
 } 
 } 
}
 buf->pixels += nn;
};
 CAT(ttcdp,1) :
 continue;
 }
}
                                    ;
    } else if (buf->pix_type==pix_fast) {
      {
 Point *p;
 int v1, v2, v3, i, width, height;
 int i1, i2, i3;
 float ox, oy;
 InvalidComponentHandle iElts;
 RGBColor vcolors[6];
 int shademe;
 char fcst = xf->fcst, bcst = xf->bcst, ncst = xf->ncst;
 char fbyte = xf->fbyte, bbyte = xf->bbyte;
 iElts = (invalid_status == INV_UNKNOWN) ? xf->iElts : NULL;
 width = buf->width, height = buf->height;
 ox = buf->ox, oy = buf->oy;
 if (1 && xf->lights) {
 fcolors = (Pointer)vcolors;
 bcolors = (Pointer)(vcolors+3);
 _dxfInitApplyLights(xf->kaf, xf->kdf, xf->ksf, xf->kspf,
 xf->kab, xf->kdb, xf->ksb, xf->kspb,
 xf->fcolors, xf->bcolors, xf->cmap,
 xf->normals, xf->lights,
 xf->colors_dep, xf->normals_dep,
 fcolors, bcolors, 3, fcst, bcst, ncst,
 fbyte, bbyte);
 cmap = NULL;
 shademe = 1;
 } else {
 shademe = 0;
 if (fbyte || bbyte)
 _dxf_initUbyteToFloat();
 }
 for (i=0; i<ntri; i++, tri++) {
 int index = (indices == NULL) ? i : indices[i];
 if (iElts && DXIsElementInvalid(iElts, index))
 continue;
 v1=tri->p, p= &xf->positions[v1], x1=p->x, y1=p->y, z1=p->z;
 v2=tri->q, p= &xf->positions[v2], x2=p->x, y2=p->y, z2=p->z;
 v3=tri->r, p= &xf->positions[v3], x3=p->x, y3=p->y, z3=p->z;
 if (shademe)
 {
 i1 = 0; i2 = 1; i3 = 2;
 if (! _dxfApplyLights((int *)tri, &index, 1))
 return ERROR;
 }
 if (xf->tile.perspective) {
 if (z1>nearPlane || z2>nearPlane || z3>nearPlane)
 continue;
 z1=(float)-1.0/z1; x1*=z1; y1*=z1;
 z2=(float)-1.0/z2; x2*=z2; y2*=z2;
 z3=(float)-1.0/z3; x3*=z3; y3*=z3;
 }
 if ((x1<minx && x2<minx && x3<minx) ||
 (x1>maxx && x2>maxx && x3>maxx) ||
 (y1<miny && y2<miny && y3<miny) ||
 (y1>maxy && y2>maxy && y3>maxy))
 continue;
 buf->triangles++;
 {
 struct fast *pp, *p;
 Pointer colors;
 int cstcolors;
 RGBColor cbuf1, cbuf2, cbuf3;
 char cbyte;
 char obyte = xf->obyte;
 x1+=ox, y1+=oy;
 x2+=ox, y2+=oy;
 x3+=ox, y3+=oy;
 if (x1<-(DXD_MAX_INT/2) || x1>(DXD_MAX_INT/2) || y1<-(DXD_MAX_INT/2) || y1>(DXD_MAX_INT/2) ||
 x2<-(DXD_MAX_INT/2) || x2>(DXD_MAX_INT/2) || y2<-(DXD_MAX_INT/2) || y2>(DXD_MAX_INT/2) ||
 x3<-(DXD_MAX_INT/2) || x3>(DXD_MAX_INT/2) || y3<-(DXD_MAX_INT/2) || y3>(DXD_MAX_INT/2))
 DXErrorReturn(ERROR_BAD_PARAMETER,
 "camera causes numerical overflow");
 d1 = y3-y2, d2 = y1-y3, d3 = y2-y1;
 d = x1*d1 + x2*d2 + x3*d3;
 Qx = d? (float)1.0 / (float)d : 0.0;
 d1 *= Qx, d2 *= Qx, d3 *= Qx;
 if (1) {
 if (d < 0)
 cbyte = fbyte, colors = fcolors, cstcolors = xf->fcst;
 else
 cbyte = bbyte, colors = bcolors, cstcolors = xf->bcst;
 if (!colors) goto CAT(ttfdp,1);
 }
 if (1) {
 if (shademe) {
 c1 = ((RGBColor *)colors) + i1;
 c2 = ((RGBColor *)colors) + i2;
 c3 = ((RGBColor *)colors) + i3;
 } else {
 if (cmap)
 c1= &(cmap[((unsigned char *)colors)[cstcolors ? 0 : v1]]), c2= &(cmap[((unsigned char *)colors)[cstcolors ? 0 : v2]]), c3= &(cmap[((unsigned char *)colors)[cstcolors ? 0 : v3]]);
 else {
 
{ 
 if (cbyte) 
 { 
 RGBByteColor *ptr=((RGBByteColor *)colors)+(cstcolors ? 0 : v1); 
 cbuf1.r = _dxd_ubyteToFloat[ptr->r]; 
 cbuf1.g = _dxd_ubyteToFloat[ptr->g]; 
 cbuf1.b = _dxd_ubyteToFloat[ptr->b]; 
 } else 
 cbuf1 = (((RGBColor *)colors)[cstcolors ? 0 : v1]); 
}; 
{ 
 if (cbyte) 
 { 
 RGBByteColor *ptr=((RGBByteColor *)colors)+(cstcolors ? 0 : v2); 
 cbuf2.r = _dxd_ubyteToFloat[ptr->r]; 
 cbuf2.g = _dxd_ubyteToFloat[ptr->g]; 
 cbuf2.b = _dxd_ubyteToFloat[ptr->b]; 
 } else 
 cbuf2 = (((RGBColor *)colors)[cstcolors ? 0 : v2]); 
}; 
{ 
 if (cbyte) 
 { 
 RGBByteColor *ptr=((RGBByteColor *)colors)+(cstcolors ? 0 : v3); 
 cbuf3.r = _dxd_ubyteToFloat[ptr->r]; 
 cbuf3.g = _dxd_ubyteToFloat[ptr->g]; 
 cbuf3.b = _dxd_ubyteToFloat[ptr->b]; 
 } else 
 cbuf3 = (((RGBColor *)colors)[cstcolors ? 0 : v3]); 
};
 c1 = &cbuf1; c2 = &cbuf2; c3 = &cbuf3;
 }
 }
 r1=c1->r, g1=c1->g, b1=c1->b;
 r2=c2->r, g2=c2->g, b2=c2->b;
 r3=c3->r, g3=c3->g, b3=c3->b;
 } else if (1) {
 if (cmap){
 r=(cmap[((unsigned char *)colors)[cstcolors ? 0 : index]]).r; g=(cmap[((unsigned char *)colors)[cstcolors ? 0 : index]]).g; b=(cmap[((unsigned char *)colors)[cstcolors ? 0 : index]]).b;
 } else {
 
{ 
 if (cbyte) 
 { 
 RGBByteColor *ptr=((RGBByteColor *)colors)+(cstcolors ? 0 : index); 
 cbuf1.r = _dxd_ubyteToFloat[ptr->r]; 
 cbuf1.g = _dxd_ubyteToFloat[ptr->g]; 
 cbuf1.b = _dxd_ubyteToFloat[ptr->b]; 
 } else 
 cbuf1 = (((RGBColor *)colors)[cstcolors ? 0 : index]); 
};
 r = cbuf1.r; g = cbuf1.g; b = cbuf1.b;
 }
 }
 if (1) {
 if (omap) o1=(omap[((unsigned char *)opacities)[xf->ocst ? 0 : v1]]), o2=(omap[((unsigned char *)opacities)[xf->ocst ? 0 : v2]]), o3=(omap[((unsigned char *)opacities)[xf->ocst ? 0 : v3]]);
 else {
 
{ 
 if (obyte) 
 o1 = _dxd_ubyteToFloat[((ubyte *)opacities)[xf->ocst ? 0 : v1]]; 
 else 
 o1 = (((float *)opacities)[xf->ocst ? 0 : v1]); 
}; 
{ 
 if (obyte) 
 o2 = _dxd_ubyteToFloat[((ubyte *)opacities)[xf->ocst ? 0 : v2]]; 
 else 
 o2 = (((float *)opacities)[xf->ocst ? 0 : v2]); 
}; 
{ 
 if (obyte) 
 o3 = _dxd_ubyteToFloat[((ubyte *)opacities)[xf->ocst ? 0 : v3]]; 
 else 
 o3 = (((float *)opacities)[xf->ocst ? 0 : v3]); 
};
 }
 } else if (opacities||omap) {
 if (omap) o=(omap[((unsigned char *)opacities)[xf->ocst ? 0 : index]]);
 else 
{ 
 if (obyte) 
 o = _dxd_ubyteToFloat[((ubyte *)opacities)[xf->ocst ? 0 : index]]; 
 else 
 o = (((float *)opacities)[xf->ocst ? 0 : index]); 
};
 obar = 1.0-o;
 } else
 o = 1.0, obar = 0.0;
 if (1) {
 dxr = (r1 == r2 && r1 == r3) ? 0.0 : d1*r1 + d2*r2 + d3*r3;
 dxg = (g1 == g2 && g1 == g3) ? 0.0 : d1*g1 + d2*g2 + d3*g3;
 dxb = (b1 == b2 && b1 == b3) ? 0.0 : d1*b1 + d2*b2 + d3*b3;
 }
 if (1) dxo = (o1 == o2 && o1 == o3) ? 0.0 : d1*o1 + d2*o2 + d3*o3;
 dxz = (z1 == z2 && z1 == z3) ? 0.0 : d1*z1 + d2*z2 + d3*z3;
 { 
 if (y2 < y1) { 
 (d=y1, y1=y2, y2=d), (d=x1, x1=x2, x2=d); 
 if (1) (d=r1, r1=r2, r2=d), (d=g1, g1=g2, g2=d), (d=b1, b1=b2, b2=d); 
 if (1) (d=o1, o1=o2, o2=d); 
 (d=z1, z1=z2, z2=d); 
 } 
}
 { 
 if (y3 < y1) { 
 (d=y1, y1=y3, y3=d), (d=x1, x1=x3, x3=d); 
 if (1) (d=r1, r1=r3, r3=d), (d=g1, g1=g3, g3=d), (d=b1, b1=b3, b3=d); 
 if (1) (d=o1, o1=o3, o3=d); 
 (d=z1, z1=z3, z3=d); 
 } 
}
 { 
 if (y3 < y2) { 
 (d=y2, y2=y3, y3=d), (d=x2, x2=x3, x3=d); 
 if (1) (d=r2, r2=r3, r3=d), (d=g2, g2=g3, g3=d), (d=b2, b2=b3, b3=d); 
 if (1) (d=o2, o2=o3, o3=d); 
 (d=z2, z2=z3, z3=d); 
 } 
}
 if (0) {
 A = B = x1;
 if (x2<A) A = x2; else B = x2;
 if (x3<A) A = x3; else if (x3>B) B = x3;
 if (A<buf->fmin.x) buf->fmin.x = A;
 if (B>buf->fmax.x) buf->fmax.x = B;
 if (y1<buf->fmin.y) buf->fmin.y = y1;
 if (y3>buf->fmax.y) buf->fmax.y = y3;
 }
 iy1 = ((int)((y1)-30000.0)+30000), iy2 = ((int)((y2)-30000.0)+30000), iy3 = ((int)((y3)-30000.0)+30000);
 if (iy1>iy2 || iy2>iy3) {
 DXSetError(ERROR_DATA_INVALID,
 "position number %d, %d, or %d is invalid", v1, v2, v3);
 return ERROR;
 }
 if (iy1==iy3)
 goto CAT(ttfdp,1);
 d = y3 - y1;
 Qy = d? (float)1.0 / (float)d : 0.0;
 if (1) dyr=(r3-r1)*Qy, dyg=(g3-g1)*Qy, dyb=(b3-b1)*Qy;
 if (1) dyo = (float)(o3-o1) * Qy;
 dyz = (z3-z1) * Qy;
 dyA = (float)(x3-x1) * Qy;
 A = x1;
 pp = buf->u.fast + iy1*width;
 d = iy1 - y1;
 if (iy1<0)
 d-=iy1, pp=buf->u.fast;
 A += dyA*d;
 if (1) r1+=dyr*d, g1+=dyg*d, b1+=dyb*d;
 if (1) o1+=dyo*d;
 z1 += dyz*d;
 nn = 0;
 { 
 
 if (iy2>height) 
 iy2 = height; 
 if (iy1<iy2 && iy2>0 && iy1<=height) { 
 B = x1; 
 d = y2 - y1; 
 d = d? (float)1.0 / (float)d : 0.0; 
 dyB = (float)(x2-x1) * d; 
 if (iy1<0) iy1=0; 
 d = iy1 - y1; 
 B += d*dyB; 
 for (iy=iy1; iy<iy2; iy++) { 
 iA = ((int)((A)-30000.0)+30000); 
 iB = ((int)((B)-30000.0)+30000); 
 if (iB>iA) left=iA, right=iB; 
 else left=iB, right=iA; 
 if (left<0) left=0; 
 if (right>width) right=width; 
 n = right - left; 
 if (n>0) { 
 nn += n; 
 p = pp+left; 
 d = left - A; 
 if (1) r=r1+d*dxr, g=g1+d*dxg, b=b1+d*dxb; 
 if (1) o=o1+d*dxo, obar=1.0-o; 
 z=z1+d*dxz; 
 while (--n>=0) { 
 if ((z>p->z)) { 
 { p->c.r=o*r+obar*p->c.r; p->c.g=o*g+obar*p->c.g; p->c.b=o*b+obar*p->c.b; if (o > 0.5) p->z=z; }; 
 if (1) r+=dxr, g+=dxg, b+=dxb; 
 if (1) o+=dxo, obar=1.0-o; 
 z+=dxz; 
 p++; 
 } else { 
 if (1) r+=dxr, g+=dxg, b+=dxb; 
 if (1) o+=dxo, obar=1.0-o; 
 z+=dxz; 
 p++; 
 } 
 } 
 } 
 if (1) r1+=dyr, g1+=dyg, b1+=dyb; 
 if (1) o1+=dyo; 
 z1 += dyz; 
 A+=dyA, B+=dyB; 
 pp+=width; 
 } 
 } 
}
 { 
 
 if (iy3>height) 
 iy3 = height; 
 if (iy2<iy3 && iy3>0 && iy2<=height) { 
 B = x2; 
 d = y3 - y2; 
 d = d? (float)1.0 / (float)d : 0.0; 
 dyB = (float)(x3-x2) * d; 
 if (iy2<0) iy2=0; 
 d = iy2 - y2; 
 B += d*dyB; 
 for (iy=iy2; iy<iy3; iy++) { 
 iA = ((int)((A)-30000.0)+30000); 
 iB = ((int)((B)-30000.0)+30000); 
 if (iB>iA) left=iA, right=iB; 
 else left=iB, right=iA; 
 if (left<0) left=0; 
 if (right>width) right=width; 
 n = right - left; 
 if (n>0) { 
 nn += n; 
 p = pp+left; 
 d = left - A; 
 if (1) r=r1+d*dxr, g=g1+d*dxg, b=b1+d*dxb; 
 if (1) o=o1+d*dxo, obar=1.0-o; 
 z=z1+d*dxz; 
 while (--n>=0) { 
 if ((z>p->z)) { 
 { p->c.r=o*r+obar*p->c.r; p->c.g=o*g+obar*p->c.g; p->c.b=o*b+obar*p->c.b; if (o > 0.5) p->z=z; }; 
 if (1) r+=dxr, g+=dxg, b+=dxb; 
 if (1) o+=dxo, obar=1.0-o; 
 z+=dxz; 
 p++; 
 } else { 
 if (1) r+=dxr, g+=dxg, b+=dxb; 
 if (1) o+=dxo, obar=1.0-o; 
 z+=dxz; 
 p++; 
 } 
 } 
 } 
 if (1) r1+=dyr, g1+=dyg, b1+=dyb; 
 if (1) o1+=dyo; 
 z1 += dyz; 
 A+=dyA, B+=dyB; 
 pp+=width; 
 } 
 } 
}
 buf->pixels += nn;
};
 CAT(ttfdp,1) :
 continue;
 }
}
                                 ;
    } else if (buf->pix_type==pix_big) {
      {
 Point *p;
 int v1, v2, v3, i, width, height;
 int i1, i2, i3;
 float ox, oy;
 InvalidComponentHandle iElts;
 RGBColor vcolors[6];
 int shademe;
 char fcst = xf->fcst, bcst = xf->bcst, ncst = xf->ncst;
 char fbyte = xf->fbyte, bbyte = xf->bbyte;
 iElts = (invalid_status == INV_UNKNOWN) ? xf->iElts : NULL;
 width = buf->width, height = buf->height;
 ox = buf->ox, oy = buf->oy;
 if (1 && xf->lights) {
 fcolors = (Pointer)vcolors;
 bcolors = (Pointer)(vcolors+3);
 _dxfInitApplyLights(xf->kaf, xf->kdf, xf->ksf, xf->kspf,
 xf->kab, xf->kdb, xf->ksb, xf->kspb,
 xf->fcolors, xf->bcolors, xf->cmap,
 xf->normals, xf->lights,
 xf->colors_dep, xf->normals_dep,
 fcolors, bcolors, 3, fcst, bcst, ncst,
 fbyte, bbyte);
 cmap = NULL;
 shademe = 1;
 } else {
 shademe = 0;
 if (fbyte || bbyte)
 _dxf_initUbyteToFloat();
 }
 for (i=0; i<ntri; i++, tri++) {
 int index = (indices == NULL) ? i : indices[i];
 if (iElts && DXIsElementInvalid(iElts, index))
 continue;
 v1=tri->p, p= &xf->positions[v1], x1=p->x, y1=p->y, z1=p->z;
 v2=tri->q, p= &xf->positions[v2], x2=p->x, y2=p->y, z2=p->z;
 v3=tri->r, p= &xf->positions[v3], x3=p->x, y3=p->y, z3=p->z;
 if (shademe)
 {
 i1 = 0; i2 = 1; i3 = 2;
 if (! _dxfApplyLights((int *)tri, &index, 1))
 return ERROR;
 }
 if (xf->tile.perspective) {
 if (z1>nearPlane || z2>nearPlane || z3>nearPlane)
 continue;
 z1=(float)-1.0/z1; x1*=z1; y1*=z1;
 z2=(float)-1.0/z2; x2*=z2; y2*=z2;
 z3=(float)-1.0/z3; x3*=z3; y3*=z3;
 }
 if ((x1<minx && x2<minx && x3<minx) ||
 (x1>maxx && x2>maxx && x3>maxx) ||
 (y1<miny && y2<miny && y3<miny) ||
 (y1>maxy && y2>maxy && y3>maxy))
 continue;
 buf->triangles++;
 {
 struct big *pp, *p;
 Pointer colors;
 int cstcolors;
 RGBColor cbuf1, cbuf2, cbuf3;
 char cbyte;
 char obyte = xf->obyte;
 x1+=ox, y1+=oy;
 x2+=ox, y2+=oy;
 x3+=ox, y3+=oy;
 if (x1<-(DXD_MAX_INT/2) || x1>(DXD_MAX_INT/2) || y1<-(DXD_MAX_INT/2) || y1>(DXD_MAX_INT/2) ||
 x2<-(DXD_MAX_INT/2) || x2>(DXD_MAX_INT/2) || y2<-(DXD_MAX_INT/2) || y2>(DXD_MAX_INT/2) ||
 x3<-(DXD_MAX_INT/2) || x3>(DXD_MAX_INT/2) || y3<-(DXD_MAX_INT/2) || y3>(DXD_MAX_INT/2))
 DXErrorReturn(ERROR_BAD_PARAMETER,
 "camera causes numerical overflow");
 d1 = y3-y2, d2 = y1-y3, d3 = y2-y1;
 d = x1*d1 + x2*d2 + x3*d3;
 Qx = d? (float)1.0 / (float)d : 0.0;
 d1 *= Qx, d2 *= Qx, d3 *= Qx;
 if (1) {
 if (d < 0)
 cbyte = fbyte, colors = fcolors, cstcolors = xf->fcst;
 else
 cbyte = bbyte, colors = bcolors, cstcolors = xf->bcst;
 if (!colors) goto CAT(ttbdp,1);
 }
 if (1) {
 if (shademe) {
 c1 = ((RGBColor *)colors) + i1;
 c2 = ((RGBColor *)colors) + i2;
 c3 = ((RGBColor *)colors) + i3;
 } else {
 if (cmap)
 c1= &(cmap[((unsigned char *)colors)[cstcolors ? 0 : v1]]), c2= &(cmap[((unsigned char *)colors)[cstcolors ? 0 : v2]]), c3= &(cmap[((unsigned char *)colors)[cstcolors ? 0 : v3]]);
 else {
 
{ 
 if (cbyte) 
 { 
 RGBByteColor *ptr=((RGBByteColor *)colors)+(cstcolors ? 0 : v1); 
 cbuf1.r = _dxd_ubyteToFloat[ptr->r]; 
 cbuf1.g = _dxd_ubyteToFloat[ptr->g]; 
 cbuf1.b = _dxd_ubyteToFloat[ptr->b]; 
 } else 
 cbuf1 = (((RGBColor *)colors)[cstcolors ? 0 : v1]); 
}; 
{ 
 if (cbyte) 
 { 
 RGBByteColor *ptr=((RGBByteColor *)colors)+(cstcolors ? 0 : v2); 
 cbuf2.r = _dxd_ubyteToFloat[ptr->r]; 
 cbuf2.g = _dxd_ubyteToFloat[ptr->g]; 
 cbuf2.b = _dxd_ubyteToFloat[ptr->b]; 
 } else 
 cbuf2 = (((RGBColor *)colors)[cstcolors ? 0 : v2]); 
}; 
{ 
 if (cbyte) 
 { 
 RGBByteColor *ptr=((RGBByteColor *)colors)+(cstcolors ? 0 : v3); 
 cbuf3.r = _dxd_ubyteToFloat[ptr->r]; 
 cbuf3.g = _dxd_ubyteToFloat[ptr->g]; 
 cbuf3.b = _dxd_ubyteToFloat[ptr->b]; 
 } else 
 cbuf3 = (((RGBColor *)colors)[cstcolors ? 0 : v3]); 
};
 c1 = &cbuf1; c2 = &cbuf2; c3 = &cbuf3;
 }
 }
 r1=c1->r, g1=c1->g, b1=c1->b;
 r2=c2->r, g2=c2->g, b2=c2->b;
 r3=c3->r, g3=c3->g, b3=c3->b;
 } else if (1) {
 if (cmap){
 r=(cmap[((unsigned char *)colors)[cstcolors ? 0 : index]]).r; g=(cmap[((unsigned char *)colors)[cstcolors ? 0 : index]]).g; b=(cmap[((unsigned char *)colors)[cstcolors ? 0 : index]]).b;
 } else {
 
{ 
 if (cbyte) 
 { 
 RGBByteColor *ptr=((RGBByteColor *)colors)+(cstcolors ? 0 : index); 
 cbuf1.r = _dxd_ubyteToFloat[ptr->r]; 
 cbuf1.g = _dxd_ubyteToFloat[ptr->g]; 
 cbuf1.b = _dxd_ubyteToFloat[ptr->b]; 
 } else 
 cbuf1 = (((RGBColor *)colors)[cstcolors ? 0 : index]); 
};
 r = cbuf1.r; g = cbuf1.g; b = cbuf1.b;
 }
 }
 if (1) {
 if (omap) o1=(omap[((unsigned char *)opacities)[xf->ocst ? 0 : v1]]), o2=(omap[((unsigned char *)opacities)[xf->ocst ? 0 : v2]]), o3=(omap[((unsigned char *)opacities)[xf->ocst ? 0 : v3]]);
 else {
 
{ 
 if (obyte) 
 o1 = _dxd_ubyteToFloat[((ubyte *)opacities)[xf->ocst ? 0 : v1]]; 
 else 
 o1 = (((float *)opacities)[xf->ocst ? 0 : v1]); 
}; 
{ 
 if (obyte) 
 o2 = _dxd_ubyteToFloat[((ubyte *)opacities)[xf->ocst ? 0 : v2]]; 
 else 
 o2 = (((float *)opacities)[xf->ocst ? 0 : v2]); 
}; 
{ 
 if (obyte) 
 o3 = _dxd_ubyteToFloat[((ubyte *)opacities)[xf->ocst ? 0 : v3]]; 
 else 
 o3 = (((float *)opacities)[xf->ocst ? 0 : v3]); 
};
 }
 } else if (opacities||omap) {
 if (omap) o=(omap[((unsigned char *)opacities)[xf->ocst ? 0 : index]]);
 else 
{ 
 if (obyte) 
 o = _dxd_ubyteToFloat[((ubyte *)opacities)[xf->ocst ? 0 : index]]; 
 else 
 o = (((float *)opacities)[xf->ocst ? 0 : index]); 
};
 obar = 1.0-o;
 } else
 o = 1.0, obar = 0.0;
 if (1) {
 dxr = (r1 == r2 && r1 == r3) ? 0.0 : d1*r1 + d2*r2 + d3*r3;
 dxg = (g1 == g2 && g1 == g3) ? 0.0 : d1*g1 + d2*g2 + d3*g3;
 dxb = (b1 == b2 && b1 == b3) ? 0.0 : d1*b1 + d2*b2 + d3*b3;
 }
 if (1) dxo = (o1 == o2 && o1 == o3) ? 0.0 : d1*o1 + d2*o2 + d3*o3;
 dxz = (z1 == z2 && z1 == z3) ? 0.0 : d1*z1 + d2*z2 + d3*z3;
 { 
 if (y2 < y1) { 
 (d=y1, y1=y2, y2=d), (d=x1, x1=x2, x2=d); 
 if (1) (d=r1, r1=r2, r2=d), (d=g1, g1=g2, g2=d), (d=b1, b1=b2, b2=d); 
 if (1) (d=o1, o1=o2, o2=d); 
 (d=z1, z1=z2, z2=d); 
 } 
}
 { 
 if (y3 < y1) { 
 (d=y1, y1=y3, y3=d), (d=x1, x1=x3, x3=d); 
 if (1) (d=r1, r1=r3, r3=d), (d=g1, g1=g3, g3=d), (d=b1, b1=b3, b3=d); 
 if (1) (d=o1, o1=o3, o3=d); 
 (d=z1, z1=z3, z3=d); 
 } 
}
 { 
 if (y3 < y2) { 
 (d=y2, y2=y3, y3=d), (d=x2, x2=x3, x3=d); 
 if (1) (d=r2, r2=r3, r3=d), (d=g2, g2=g3, g3=d), (d=b2, b2=b3, b3=d); 
 if (1) (d=o2, o2=o3, o3=d); 
 (d=z2, z2=z3, z3=d); 
 } 
}
 if (0) {
 A = B = x1;
 if (x2<A) A = x2; else B = x2;
 if (x3<A) A = x3; else if (x3>B) B = x3;
 if (A<buf->fmin.x) buf->fmin.x = A;
 if (B>buf->fmax.x) buf->fmax.x = B;
 if (y1<buf->fmin.y) buf->fmin.y = y1;
 if (y3>buf->fmax.y) buf->fmax.y = y3;
 }
 iy1 = ((int)((y1)-30000.0)+30000), iy2 = ((int)((y2)-30000.0)+30000), iy3 = ((int)((y3)-30000.0)+30000);
 if (iy1>iy2 || iy2>iy3) {
 DXSetError(ERROR_DATA_INVALID,
 "position number %d, %d, or %d is invalid", v1, v2, v3);
 return ERROR;
 }
 if (iy1==iy3)
 goto CAT(ttbdp,1);
 d = y3 - y1;
 Qy = d? (float)1.0 / (float)d : 0.0;
 if (1) dyr=(r3-r1)*Qy, dyg=(g3-g1)*Qy, dyb=(b3-b1)*Qy;
 if (1) dyo = (float)(o3-o1) * Qy;
 dyz = (z3-z1) * Qy;
 dyA = (float)(x3-x1) * Qy;
 A = x1;
 pp = buf->u.big + iy1*width;
 d = iy1 - y1;
 if (iy1<0)
 d-=iy1, pp=buf->u.big;
 A += dyA*d;
 if (1) r1+=dyr*d, g1+=dyg*d, b1+=dyb*d;
 if (1) o1+=dyo*d;
 z1 += dyz*d;
 nn = 0;
 { 
 
 if (iy2>height) 
 iy2 = height; 
 if (iy1<iy2 && iy2>0 && iy1<=height) { 
 B = x1; 
 d = y2 - y1; 
 d = d? (float)1.0 / (float)d : 0.0; 
 dyB = (float)(x2-x1) * d; 
 if (iy1<0) iy1=0; 
 d = iy1 - y1; 
 B += d*dyB; 
 for (iy=iy1; iy<iy2; iy++) { 
 iA = ((int)((A)-30000.0)+30000); 
 iB = ((int)((B)-30000.0)+30000); 
 if (iB>iA) left=iA, right=iB; 
 else left=iB, right=iA; 
 if (left<0) left=0; 
 if (right>width) right=width; 
 n = right - left; 
 if (n>0) { 
 nn += n; 
 p = pp+left; 
 d = left - A; 
 if (1) r=r1+d*dxr, g=g1+d*dxg, b=b1+d*dxb; 
 if (1) o=o1+d*dxo, obar=1.0-o; 
 z=z1+d*dxz; 
 while (--n>=0) { 
 if ((z>p->z)) { 
 { p->c.r=o*r+obar*p->c.r; p->c.g=o*g+obar*p->c.g; p->c.b=o*b+obar*p->c.b; if (o > 0.5) p->z=z; }; 
 if (1) r+=dxr, g+=dxg, b+=dxb; 
 if (1) o+=dxo, obar=1.0-o; 
 z+=dxz; 
 p++; 
 } else { 
 if (1) r+=dxr, g+=dxg, b+=dxb; 
 if (1) o+=dxo, obar=1.0-o; 
 z+=dxz; 
 p++; 
 } 
 } 
 } 
 if (1) r1+=dyr, g1+=dyg, b1+=dyb; 
 if (1) o1+=dyo; 
 z1 += dyz; 
 A+=dyA, B+=dyB; 
 pp+=width; 
 } 
 } 
}
 { 
 
 if (iy3>height) 
 iy3 = height; 
 if (iy2<iy3 && iy3>0 && iy2<=height) { 
 B = x2; 
 d = y3 - y2; 
 d = d? (float)1.0 / (float)d : 0.0; 
 dyB = (float)(x3-x2) * d; 
 if (iy2<0) iy2=0; 
 d = iy2 - y2; 
 B += d*dyB; 
 for (iy=iy2; iy<iy3; iy++) { 
 iA = ((int)((A)-30000.0)+30000); 
 iB = ((int)((B)-30000.0)+30000); 
 if (iB>iA) left=iA, right=iB; 
 else left=iB, right=iA; 
 if (left<0) left=0; 
 if (right>width) right=width; 
 n = right - left; 
 if (n>0) { 
 nn += n; 
 p = pp+left; 
 d = left - A; 
 if (1) r=r1+d*dxr, g=g1+d*dxg, b=b1+d*dxb; 
 if (1) o=o1+d*dxo, obar=1.0-o; 
 z=z1+d*dxz; 
 while (--n>=0) { 
 if ((z>p->z)) { 
 { p->c.r=o*r+obar*p->c.r; p->c.g=o*g+obar*p->c.g; p->c.b=o*b+obar*p->c.b; if (o > 0.5) p->z=z; }; 
 if (1) r+=dxr, g+=dxg, b+=dxb; 
 if (1) o+=dxo, obar=1.0-o; 
 z+=dxz; 
 p++; 
 } else { 
 if (1) r+=dxr, g+=dxg, b+=dxb; 
 if (1) o+=dxo, obar=1.0-o; 
 z+=dxz; 
 p++; 
 } 
 } 
 } 
 if (1) r1+=dyr, g1+=dyg, b1+=dyb; 
 if (1) o1+=dyo; 
 z1 += dyz; 
 A+=dyA, B+=dyB; 
 pp+=width; 
 } 
 } 
}
 buf->pixels += nn;
};
 CAT(ttbdp,1) :
 continue;
 }
}
                                 ;
    } else {
      DXSetError(ERROR_INTERNAL, "unknown pix_type %d", buf->pix_type);
      return ERROR;
    }
  }
 return OK;
 }
 static Error
 tri_opaque(
 struct buffer *buf,
 struct xfield *xf,
 int ntri,
 Triangle *tri,
 int *indices,
 int surface,
 int clip_status,
 inv_stat invalid_status
 ) {
 RGBColor *c1, *c2, *c3;
 float x1, y1, x2, y2, x3, y3;
 float r1, g1, b1, o1, z1, r2, g2, b2, o2, z2, r3, g3, b3, o3, z3;
 float Qx, dxo, dxz, dxr, dxg, dxb;
 float Qy, dyr, dyg, dyb, dyo, dyz, dyA, dyB;
 float r, g, b, o, obar, z, nearPlane=xf->nearPlane ;
 float A, B, d, d1, d2, d3;
 int iy1, iy2, iy3;
 int iA, iB, iy, i, n, nn, left, right;
 float minx = buf->min.x, miny = buf->min.y;
 float maxx = buf->max.x, maxy = buf->max.y;
 Pointer fcolors = xf->fcolors;
 Pointer bcolors = xf->bcolors;
 Pointer opacities = xf->opacities;
 RGBColor *cmap = xf->cmap;
 float *omap = xf->omap;
    if (clip_status) {
      ASSERT(buf->pix_type==pix_big);
      {
 Point *p;
 int v1, v2, v3, i, width, height;
 int i1, i2, i3;
 float ox, oy;
 InvalidComponentHandle iElts;
 RGBColor vcolors[6];
 int shademe;
 char fcst = xf->fcst, bcst = xf->bcst, ncst = xf->ncst;
 char fbyte = xf->fbyte, bbyte = xf->bbyte;
 iElts = (invalid_status == INV_UNKNOWN) ? xf->iElts : NULL;
 width = buf->width, height = buf->height;
 ox = buf->ox, oy = buf->oy;
 if (1 && xf->lights) {
 fcolors = (Pointer)vcolors;
 bcolors = (Pointer)(vcolors+3);
 _dxfInitApplyLights(xf->kaf, xf->kdf, xf->ksf, xf->kspf,
 xf->kab, xf->kdb, xf->ksb, xf->kspb,
 xf->fcolors, xf->bcolors, xf->cmap,
 xf->normals, xf->lights,
 xf->colors_dep, xf->normals_dep,
 fcolors, bcolors, 3, fcst, bcst, ncst,
 fbyte, bbyte);
 cmap = NULL;
 shademe = 1;
 } else {
 shademe = 0;
 if (fbyte || bbyte)
 _dxf_initUbyteToFloat();
 }
 for (i=0; i<ntri; i++, tri++) {
 int index = (indices == NULL) ? i : indices[i];
 if (iElts && DXIsElementInvalid(iElts, index))
 continue;
 v1=tri->p, p= &xf->positions[v1], x1=p->x, y1=p->y, z1=p->z;
 v2=tri->q, p= &xf->positions[v2], x2=p->x, y2=p->y, z2=p->z;
 v3=tri->r, p= &xf->positions[v3], x3=p->x, y3=p->y, z3=p->z;
 if (shademe)
 {
 i1 = 0; i2 = 1; i3 = 2;
 if (! _dxfApplyLights((int *)tri, &index, 1))
 return ERROR;
 }
 if (xf->tile.perspective) {
 if (z1>nearPlane || z2>nearPlane || z3>nearPlane)
 continue;
 z1=(float)-1.0/z1; x1*=z1; y1*=z1;
 z2=(float)-1.0/z2; x2*=z2; y2*=z2;
 z3=(float)-1.0/z3; x3*=z3; y3*=z3;
 }
 if ((x1<minx && x2<minx && x3<minx) ||
 (x1>maxx && x2>maxx && x3>maxx) ||
 (y1<miny && y2<miny && y3<miny) ||
 (y1>maxy && y2>maxy && y3>maxy))
 continue;
 buf->triangles++;
 {
 struct big *pp, *p;
 Pointer colors;
 int cstcolors;
 RGBColor cbuf1, cbuf2, cbuf3;
 char cbyte;
 char obyte = xf->obyte;
 x1+=ox, y1+=oy;
 x2+=ox, y2+=oy;
 x3+=ox, y3+=oy;
 if (x1<-(DXD_MAX_INT/2) || x1>(DXD_MAX_INT/2) || y1<-(DXD_MAX_INT/2) || y1>(DXD_MAX_INT/2) ||
 x2<-(DXD_MAX_INT/2) || x2>(DXD_MAX_INT/2) || y2<-(DXD_MAX_INT/2) || y2>(DXD_MAX_INT/2) ||
 x3<-(DXD_MAX_INT/2) || x3>(DXD_MAX_INT/2) || y3<-(DXD_MAX_INT/2) || y3>(DXD_MAX_INT/2))
 DXErrorReturn(ERROR_BAD_PARAMETER,
 "camera causes numerical overflow");
 d1 = y3-y2, d2 = y1-y3, d3 = y2-y1;
 d = x1*d1 + x2*d2 + x3*d3;
 Qx = d? (float)1.0 / (float)d : 0.0;
 d1 *= Qx, d2 *= Qx, d3 *= Qx;
 if (1) {
 if (d < 0)
 cbyte = fbyte, colors = fcolors, cstcolors = xf->fcst;
 else
 cbyte = bbyte, colors = bcolors, cstcolors = xf->bcst;
 if (!colors) goto CAT(toc,1);
 }
 if (1) {
 if (shademe) {
 c1 = ((RGBColor *)colors) + i1;
 c2 = ((RGBColor *)colors) + i2;
 c3 = ((RGBColor *)colors) + i3;
 } else {
 if (cmap)
 c1= &(cmap[((unsigned char *)colors)[cstcolors ? 0 : v1]]), c2= &(cmap[((unsigned char *)colors)[cstcolors ? 0 : v2]]), c3= &(cmap[((unsigned char *)colors)[cstcolors ? 0 : v3]]);
 else {
 
{ 
 if (cbyte) 
 { 
 RGBByteColor *ptr=((RGBByteColor *)colors)+(cstcolors ? 0 : v1); 
 cbuf1.r = _dxd_ubyteToFloat[ptr->r]; 
 cbuf1.g = _dxd_ubyteToFloat[ptr->g]; 
 cbuf1.b = _dxd_ubyteToFloat[ptr->b]; 
 } else 
 cbuf1 = (((RGBColor *)colors)[cstcolors ? 0 : v1]); 
}; 
{ 
 if (cbyte) 
 { 
 RGBByteColor *ptr=((RGBByteColor *)colors)+(cstcolors ? 0 : v2); 
 cbuf2.r = _dxd_ubyteToFloat[ptr->r]; 
 cbuf2.g = _dxd_ubyteToFloat[ptr->g]; 
 cbuf2.b = _dxd_ubyteToFloat[ptr->b]; 
 } else 
 cbuf2 = (((RGBColor *)colors)[cstcolors ? 0 : v2]); 
}; 
{ 
 if (cbyte) 
 { 
 RGBByteColor *ptr=((RGBByteColor *)colors)+(cstcolors ? 0 : v3); 
 cbuf3.r = _dxd_ubyteToFloat[ptr->r]; 
 cbuf3.g = _dxd_ubyteToFloat[ptr->g]; 
 cbuf3.b = _dxd_ubyteToFloat[ptr->b]; 
 } else 
 cbuf3 = (((RGBColor *)colors)[cstcolors ? 0 : v3]); 
};
 c1 = &cbuf1; c2 = &cbuf2; c3 = &cbuf3;
 }
 }
 r1=c1->r, g1=c1->g, b1=c1->b;
 r2=c2->r, g2=c2->g, b2=c2->b;
 r3=c3->r, g3=c3->g, b3=c3->b;
 } else if (1) {
 if (cmap){
 r=(cmap[((unsigned char *)colors)[cstcolors ? 0 : index]]).r; g=(cmap[((unsigned char *)colors)[cstcolors ? 0 : index]]).g; b=(cmap[((unsigned char *)colors)[cstcolors ? 0 : index]]).b;
 } else {
 
{ 
 if (cbyte) 
 { 
 RGBByteColor *ptr=((RGBByteColor *)colors)+(cstcolors ? 0 : index); 
 cbuf1.r = _dxd_ubyteToFloat[ptr->r]; 
 cbuf1.g = _dxd_ubyteToFloat[ptr->g]; 
 cbuf1.b = _dxd_ubyteToFloat[ptr->b]; 
 } else 
 cbuf1 = (((RGBColor *)colors)[cstcolors ? 0 : index]); 
};
 r = cbuf1.r; g = cbuf1.g; b = cbuf1.b;
 }
 }
 if (0) {
 if (omap) o1=(omap[((unsigned char *)opacities)[xf->ocst ? 0 : v1]]), o2=(omap[((unsigned char *)opacities)[xf->ocst ? 0 : v2]]), o3=(omap[((unsigned char *)opacities)[xf->ocst ? 0 : v3]]);
 else {
 
{ 
 if (obyte) 
 o1 = _dxd_ubyteToFloat[((ubyte *)opacities)[xf->ocst ? 0 : v1]]; 
 else 
 o1 = (((float *)opacities)[xf->ocst ? 0 : v1]); 
}; 
{ 
 if (obyte) 
 o2 = _dxd_ubyteToFloat[((ubyte *)opacities)[xf->ocst ? 0 : v2]]; 
 else 
 o2 = (((float *)opacities)[xf->ocst ? 0 : v2]); 
}; 
{ 
 if (obyte) 
 o3 = _dxd_ubyteToFloat[((ubyte *)opacities)[xf->ocst ? 0 : v3]]; 
 else 
 o3 = (((float *)opacities)[xf->ocst ? 0 : v3]); 
};
 }
 } else if (opacities||omap) {
 if (omap) o=(omap[((unsigned char *)opacities)[xf->ocst ? 0 : index]]);
 else 
{ 
 if (obyte) 
 o = _dxd_ubyteToFloat[((ubyte *)opacities)[xf->ocst ? 0 : index]]; 
 else 
 o = (((float *)opacities)[xf->ocst ? 0 : index]); 
};
 obar = 1.0-o;
 } else
 o = 1.0, obar = 0.0;
 if (1) {
 dxr = (r1 == r2 && r1 == r3) ? 0.0 : d1*r1 + d2*r2 + d3*r3;
 dxg = (g1 == g2 && g1 == g3) ? 0.0 : d1*g1 + d2*g2 + d3*g3;
 dxb = (b1 == b2 && b1 == b3) ? 0.0 : d1*b1 + d2*b2 + d3*b3;
 }
 if (0) dxo = (o1 == o2 && o1 == o3) ? 0.0 : d1*o1 + d2*o2 + d3*o3;
 dxz = (z1 == z2 && z1 == z3) ? 0.0 : d1*z1 + d2*z2 + d3*z3;
 { 
 if (y2 < y1) { 
 (d=y1, y1=y2, y2=d), (d=x1, x1=x2, x2=d); 
 if (1) (d=r1, r1=r2, r2=d), (d=g1, g1=g2, g2=d), (d=b1, b1=b2, b2=d); 
 if (0) (d=o1, o1=o2, o2=d); 
 (d=z1, z1=z2, z2=d); 
 } 
}
 { 
 if (y3 < y1) { 
 (d=y1, y1=y3, y3=d), (d=x1, x1=x3, x3=d); 
 if (1) (d=r1, r1=r3, r3=d), (d=g1, g1=g3, g3=d), (d=b1, b1=b3, b3=d); 
 if (0) (d=o1, o1=o3, o3=d); 
 (d=z1, z1=z3, z3=d); 
 } 
}
 { 
 if (y3 < y2) { 
 (d=y2, y2=y3, y3=d), (d=x2, x2=x3, x3=d); 
 if (1) (d=r2, r2=r3, r3=d), (d=g2, g2=g3, g3=d), (d=b2, b2=b3, b3=d); 
 if (0) (d=o2, o2=o3, o3=d); 
 (d=z2, z2=z3, z3=d); 
 } 
}
 if (0) {
 A = B = x1;
 if (x2<A) A = x2; else B = x2;
 if (x3<A) A = x3; else if (x3>B) B = x3;
 if (A<buf->fmin.x) buf->fmin.x = A;
 if (B>buf->fmax.x) buf->fmax.x = B;
 if (y1<buf->fmin.y) buf->fmin.y = y1;
 if (y3>buf->fmax.y) buf->fmax.y = y3;
 }
 iy1 = ((int)((y1)-30000.0)+30000), iy2 = ((int)((y2)-30000.0)+30000), iy3 = ((int)((y3)-30000.0)+30000);
 if (iy1>iy2 || iy2>iy3) {
 DXSetError(ERROR_DATA_INVALID,
 "position number %d, %d, or %d is invalid", v1, v2, v3);
 return ERROR;
 }
 if (iy1==iy3)
 goto CAT(toc,1);
 d = y3 - y1;
 Qy = d? (float)1.0 / (float)d : 0.0;
 if (1) dyr=(r3-r1)*Qy, dyg=(g3-g1)*Qy, dyb=(b3-b1)*Qy;
 if (0) dyo = (float)(o3-o1) * Qy;
 dyz = (z3-z1) * Qy;
 dyA = (float)(x3-x1) * Qy;
 A = x1;
 pp = buf->u.big + iy1*width;
 d = iy1 - y1;
 if (iy1<0)
 d-=iy1, pp=buf->u.big;
 A += dyA*d;
 if (1) r1+=dyr*d, g1+=dyg*d, b1+=dyb*d;
 if (0) o1+=dyo*d;
 z1 += dyz*d;
 nn = 0;
 { 
 
 if (iy2>height) 
 iy2 = height; 
 if (iy1<iy2 && iy2>0 && iy1<=height) { 
 B = x1; 
 d = y2 - y1; 
 d = d? (float)1.0 / (float)d : 0.0; 
 dyB = (float)(x2-x1) * d; 
 if (iy1<0) iy1=0; 
 d = iy1 - y1; 
 B += d*dyB; 
 for (iy=iy1; iy<iy2; iy++) { 
 iA = ((int)((A)-30000.0)+30000); 
 iB = ((int)((B)-30000.0)+30000); 
 if (iB>iA) left=iA, right=iB; 
 else left=iB, right=iA; 
 if (left<0) left=0; 
 if (right>width) right=width; 
 n = right - left; 
 if (n>0) { 
 nn += n; 
 p = pp+left; 
 d = left - A; 
 if (1) r=r1+d*dxr, g=g1+d*dxg, b=b1+d*dxb; 
 if (0) o=o1+d*dxo, obar=1.0-o; 
 z=z1+d*dxz; 
 while (--n>=0) { 
 if ((z>p->z && z<p->front && z>p->back)) { 
 { p->c.r=r; p->c.g=g; p->c.b=b; p->z=z; }; 
 if (1) r+=dxr, g+=dxg, b+=dxb; 
 if (0) o+=dxo, obar=1.0-o; 
 z+=dxz; 
 p++; 
 } else { 
 if (1) r+=dxr, g+=dxg, b+=dxb; 
 if (0) o+=dxo, obar=1.0-o; 
 z+=dxz; 
 p++; 
 } 
 } 
 } 
 if (1) r1+=dyr, g1+=dyg, b1+=dyb; 
 if (0) o1+=dyo; 
 z1 += dyz; 
 A+=dyA, B+=dyB; 
 pp+=width; 
 } 
 } 
}
 { 
 
 if (iy3>height) 
 iy3 = height; 
 if (iy2<iy3 && iy3>0 && iy2<=height) { 
 B = x2; 
 d = y3 - y2; 
 d = d? (float)1.0 / (float)d : 0.0; 
 dyB = (float)(x3-x2) * d; 
 if (iy2<0) iy2=0; 
 d = iy2 - y2; 
 B += d*dyB; 
 for (iy=iy2; iy<iy3; iy++) { 
 iA = ((int)((A)-30000.0)+30000); 
 iB = ((int)((B)-30000.0)+30000); 
 if (iB>iA) left=iA, right=iB; 
 else left=iB, right=iA; 
 if (left<0) left=0; 
 if (right>width) right=width; 
 n = right - left; 
 if (n>0) { 
 nn += n; 
 p = pp+left; 
 d = left - A; 
 if (1) r=r1+d*dxr, g=g1+d*dxg, b=b1+d*dxb; 
 if (0) o=o1+d*dxo, obar=1.0-o; 
 z=z1+d*dxz; 
 while (--n>=0) { 
 if ((z>p->z && z<p->front && z>p->back)) { 
 { p->c.r=r; p->c.g=g; p->c.b=b; p->z=z; }; 
 if (1) r+=dxr, g+=dxg, b+=dxb; 
 if (0) o+=dxo, obar=1.0-o; 
 z+=dxz; 
 p++; 
 } else { 
 if (1) r+=dxr, g+=dxg, b+=dxb; 
 if (0) o+=dxo, obar=1.0-o; 
 z+=dxz; 
 p++; 
 } 
 } 
 } 
 if (1) r1+=dyr, g1+=dyg, b1+=dyb; 
 if (0) o1+=dyo; 
 z1 += dyz; 
 A+=dyA, B+=dyB; 
 pp+=width; 
 } 
 } 
}
 buf->pixels += nn;
};
 CAT(toc,1) :
 continue;
 }
};
    } else if (buf->pix_type==pix_fast) {
      {
 Point *p;
 int v1, v2, v3, i, width, height;
 int i1, i2, i3;
 float ox, oy;
 InvalidComponentHandle iElts;
 RGBColor vcolors[6];
 int shademe;
 char fcst = xf->fcst, bcst = xf->bcst, ncst = xf->ncst;
 char fbyte = xf->fbyte, bbyte = xf->bbyte;
 iElts = (invalid_status == INV_UNKNOWN) ? xf->iElts : NULL;
 width = buf->width, height = buf->height;
 ox = buf->ox, oy = buf->oy;
 if (1 && xf->lights) {
 fcolors = (Pointer)vcolors;
 bcolors = (Pointer)(vcolors+3);
 _dxfInitApplyLights(xf->kaf, xf->kdf, xf->ksf, xf->kspf,
 xf->kab, xf->kdb, xf->ksb, xf->kspb,
 xf->fcolors, xf->bcolors, xf->cmap,
 xf->normals, xf->lights,
 xf->colors_dep, xf->normals_dep,
 fcolors, bcolors, 3, fcst, bcst, ncst,
 fbyte, bbyte);
 cmap = NULL;
 shademe = 1;
 } else {
 shademe = 0;
 if (fbyte || bbyte)
 _dxf_initUbyteToFloat();
 }
 for (i=0; i<ntri; i++, tri++) {
 int index = (indices == NULL) ? i : indices[i];
 if (iElts && DXIsElementInvalid(iElts, index))
 continue;
 v1=tri->p, p= &xf->positions[v1], x1=p->x, y1=p->y, z1=p->z;
 v2=tri->q, p= &xf->positions[v2], x2=p->x, y2=p->y, z2=p->z;
 v3=tri->r, p= &xf->positions[v3], x3=p->x, y3=p->y, z3=p->z;
 if (shademe)
 {
 i1 = 0; i2 = 1; i3 = 2;
 if (! _dxfApplyLights((int *)tri, &index, 1))
 return ERROR;
 }
 if (xf->tile.perspective) {
 if (z1>nearPlane || z2>nearPlane || z3>nearPlane)
 continue;
 z1=(float)-1.0/z1; x1*=z1; y1*=z1;
 z2=(float)-1.0/z2; x2*=z2; y2*=z2;
 z3=(float)-1.0/z3; x3*=z3; y3*=z3;
 }
 if ((x1<minx && x2<minx && x3<minx) ||
 (x1>maxx && x2>maxx && x3>maxx) ||
 (y1<miny && y2<miny && y3<miny) ||
 (y1>maxy && y2>maxy && y3>maxy))
 continue;
 buf->triangles++;
 {
 struct fast *pp, *p;
 Pointer colors;
 int cstcolors;
 RGBColor cbuf1, cbuf2, cbuf3;
 char cbyte;
 char obyte = xf->obyte;
 x1+=ox, y1+=oy;
 x2+=ox, y2+=oy;
 x3+=ox, y3+=oy;
 if (x1<-(DXD_MAX_INT/2) || x1>(DXD_MAX_INT/2) || y1<-(DXD_MAX_INT/2) || y1>(DXD_MAX_INT/2) ||
 x2<-(DXD_MAX_INT/2) || x2>(DXD_MAX_INT/2) || y2<-(DXD_MAX_INT/2) || y2>(DXD_MAX_INT/2) ||
 x3<-(DXD_MAX_INT/2) || x3>(DXD_MAX_INT/2) || y3<-(DXD_MAX_INT/2) || y3>(DXD_MAX_INT/2))
 DXErrorReturn(ERROR_BAD_PARAMETER,
 "camera causes numerical overflow");
 d1 = y3-y2, d2 = y1-y3, d3 = y2-y1;
 d = x1*d1 + x2*d2 + x3*d3;
 Qx = d? (float)1.0 / (float)d : 0.0;
 d1 *= Qx, d2 *= Qx, d3 *= Qx;
 if (1) {
 if (d < 0)
 cbyte = fbyte, colors = fcolors, cstcolors = xf->fcst;
 else
 cbyte = bbyte, colors = bcolors, cstcolors = xf->bcst;
 if (!colors) goto CAT(tof,1);
 }
 if (1) {
 if (shademe) {
 c1 = ((RGBColor *)colors) + i1;
 c2 = ((RGBColor *)colors) + i2;
 c3 = ((RGBColor *)colors) + i3;
 } else {
 if (cmap)
 c1= &(cmap[((unsigned char *)colors)[cstcolors ? 0 : v1]]), c2= &(cmap[((unsigned char *)colors)[cstcolors ? 0 : v2]]), c3= &(cmap[((unsigned char *)colors)[cstcolors ? 0 : v3]]);
 else {
 
{ 
 if (cbyte) 
 { 
 RGBByteColor *ptr=((RGBByteColor *)colors)+(cstcolors ? 0 : v1); 
 cbuf1.r = _dxd_ubyteToFloat[ptr->r]; 
 cbuf1.g = _dxd_ubyteToFloat[ptr->g]; 
 cbuf1.b = _dxd_ubyteToFloat[ptr->b]; 
 } else 
 cbuf1 = (((RGBColor *)colors)[cstcolors ? 0 : v1]); 
}; 
{ 
 if (cbyte) 
 { 
 RGBByteColor *ptr=((RGBByteColor *)colors)+(cstcolors ? 0 : v2); 
 cbuf2.r = _dxd_ubyteToFloat[ptr->r]; 
 cbuf2.g = _dxd_ubyteToFloat[ptr->g]; 
 cbuf2.b = _dxd_ubyteToFloat[ptr->b]; 
 } else 
 cbuf2 = (((RGBColor *)colors)[cstcolors ? 0 : v2]); 
}; 
{ 
 if (cbyte) 
 { 
 RGBByteColor *ptr=((RGBByteColor *)colors)+(cstcolors ? 0 : v3); 
 cbuf3.r = _dxd_ubyteToFloat[ptr->r]; 
 cbuf3.g = _dxd_ubyteToFloat[ptr->g]; 
 cbuf3.b = _dxd_ubyteToFloat[ptr->b]; 
 } else 
 cbuf3 = (((RGBColor *)colors)[cstcolors ? 0 : v3]); 
};
 c1 = &cbuf1; c2 = &cbuf2; c3 = &cbuf3;
 }
 }
 r1=c1->r, g1=c1->g, b1=c1->b;
 r2=c2->r, g2=c2->g, b2=c2->b;
 r3=c3->r, g3=c3->g, b3=c3->b;
 } else if (1) {
 if (cmap){
 r=(cmap[((unsigned char *)colors)[cstcolors ? 0 : index]]).r; g=(cmap[((unsigned char *)colors)[cstcolors ? 0 : index]]).g; b=(cmap[((unsigned char *)colors)[cstcolors ? 0 : index]]).b;
 } else {
 
{ 
 if (cbyte) 
 { 
 RGBByteColor *ptr=((RGBByteColor *)colors)+(cstcolors ? 0 : index); 
 cbuf1.r = _dxd_ubyteToFloat[ptr->r]; 
 cbuf1.g = _dxd_ubyteToFloat[ptr->g]; 
 cbuf1.b = _dxd_ubyteToFloat[ptr->b]; 
 } else 
 cbuf1 = (((RGBColor *)colors)[cstcolors ? 0 : index]); 
};
 r = cbuf1.r; g = cbuf1.g; b = cbuf1.b;
 }
 }
 if (0) {
 if (omap) o1=(omap[((unsigned char *)opacities)[xf->ocst ? 0 : v1]]), o2=(omap[((unsigned char *)opacities)[xf->ocst ? 0 : v2]]), o3=(omap[((unsigned char *)opacities)[xf->ocst ? 0 : v3]]);
 else {
 
{ 
 if (obyte) 
 o1 = _dxd_ubyteToFloat[((ubyte *)opacities)[xf->ocst ? 0 : v1]]; 
 else 
 o1 = (((float *)opacities)[xf->ocst ? 0 : v1]); 
}; 
{ 
 if (obyte) 
 o2 = _dxd_ubyteToFloat[((ubyte *)opacities)[xf->ocst ? 0 : v2]]; 
 else 
 o2 = (((float *)opacities)[xf->ocst ? 0 : v2]); 
}; 
{ 
 if (obyte) 
 o3 = _dxd_ubyteToFloat[((ubyte *)opacities)[xf->ocst ? 0 : v3]]; 
 else 
 o3 = (((float *)opacities)[xf->ocst ? 0 : v3]); 
};
 }
 } else if (opacities||omap) {
 if (omap) o=(omap[((unsigned char *)opacities)[xf->ocst ? 0 : index]]);
 else 
{ 
 if (obyte) 
 o = _dxd_ubyteToFloat[((ubyte *)opacities)[xf->ocst ? 0 : index]]; 
 else 
 o = (((float *)opacities)[xf->ocst ? 0 : index]); 
};
 obar = 1.0-o;
 } else
 o = 1.0, obar = 0.0;
 if (1) {
 dxr = (r1 == r2 && r1 == r3) ? 0.0 : d1*r1 + d2*r2 + d3*r3;
 dxg = (g1 == g2 && g1 == g3) ? 0.0 : d1*g1 + d2*g2 + d3*g3;
 dxb = (b1 == b2 && b1 == b3) ? 0.0 : d1*b1 + d2*b2 + d3*b3;
 }
 if (0) dxo = (o1 == o2 && o1 == o3) ? 0.0 : d1*o1 + d2*o2 + d3*o3;
 dxz = (z1 == z2 && z1 == z3) ? 0.0 : d1*z1 + d2*z2 + d3*z3;
 { 
 if (y2 < y1) { 
 (d=y1, y1=y2, y2=d), (d=x1, x1=x2, x2=d); 
 if (1) (d=r1, r1=r2, r2=d), (d=g1, g1=g2, g2=d), (d=b1, b1=b2, b2=d); 
 if (0) (d=o1, o1=o2, o2=d); 
 (d=z1, z1=z2, z2=d); 
 } 
}
 { 
 if (y3 < y1) { 
 (d=y1, y1=y3, y3=d), (d=x1, x1=x3, x3=d); 
 if (1) (d=r1, r1=r3, r3=d), (d=g1, g1=g3, g3=d), (d=b1, b1=b3, b3=d); 
 if (0) (d=o1, o1=o3, o3=d); 
 (d=z1, z1=z3, z3=d); 
 } 
}
 { 
 if (y3 < y2) { 
 (d=y2, y2=y3, y3=d), (d=x2, x2=x3, x3=d); 
 if (1) (d=r2, r2=r3, r3=d), (d=g2, g2=g3, g3=d), (d=b2, b2=b3, b3=d); 
 if (0) (d=o2, o2=o3, o3=d); 
 (d=z2, z2=z3, z3=d); 
 } 
}
 if (0) {
 A = B = x1;
 if (x2<A) A = x2; else B = x2;
 if (x3<A) A = x3; else if (x3>B) B = x3;
 if (A<buf->fmin.x) buf->fmin.x = A;
 if (B>buf->fmax.x) buf->fmax.x = B;
 if (y1<buf->fmin.y) buf->fmin.y = y1;
 if (y3>buf->fmax.y) buf->fmax.y = y3;
 }
 iy1 = ((int)((y1)-30000.0)+30000), iy2 = ((int)((y2)-30000.0)+30000), iy3 = ((int)((y3)-30000.0)+30000);
 if (iy1>iy2 || iy2>iy3) {
 DXSetError(ERROR_DATA_INVALID,
 "position number %d, %d, or %d is invalid", v1, v2, v3);
 return ERROR;
 }
 if (iy1==iy3)
 goto CAT(tof,1);
 d = y3 - y1;
 Qy = d? (float)1.0 / (float)d : 0.0;
 if (1) dyr=(r3-r1)*Qy, dyg=(g3-g1)*Qy, dyb=(b3-b1)*Qy;
 if (0) dyo = (float)(o3-o1) * Qy;
 dyz = (z3-z1) * Qy;
 dyA = (float)(x3-x1) * Qy;
 A = x1;
 pp = buf->u.fast + iy1*width;
 d = iy1 - y1;
 if (iy1<0)
 d-=iy1, pp=buf->u.fast;
 A += dyA*d;
 if (1) r1+=dyr*d, g1+=dyg*d, b1+=dyb*d;
 if (0) o1+=dyo*d;
 z1 += dyz*d;
 nn = 0;
 { 
 
 if (iy2>height) 
 iy2 = height; 
 if (iy1<iy2 && iy2>0 && iy1<=height) { 
 B = x1; 
 d = y2 - y1; 
 d = d? (float)1.0 / (float)d : 0.0; 
 dyB = (float)(x2-x1) * d; 
 if (iy1<0) iy1=0; 
 d = iy1 - y1; 
 B += d*dyB; 
 for (iy=iy1; iy<iy2; iy++) { 
 iA = ((int)((A)-30000.0)+30000); 
 iB = ((int)((B)-30000.0)+30000); 
 if (iB>iA) left=iA, right=iB; 
 else left=iB, right=iA; 
 if (left<0) left=0; 
 if (right>width) right=width; 
 n = right - left; 
 if (n>0) { 
 nn += n; 
 p = pp+left; 
 d = left - A; 
 if (1) r=r1+d*dxr, g=g1+d*dxg, b=b1+d*dxb; 
 if (0) o=o1+d*dxo, obar=1.0-o; 
 z=z1+d*dxz; 
 while (--n>=0) { 
 if ((z>p->z)) { 
 { p->c.r=r; p->c.g=g; p->c.b=b; p->z=z; }; 
 if (1) r+=dxr, g+=dxg, b+=dxb; 
 if (0) o+=dxo, obar=1.0-o; 
 z+=dxz; 
 p++; 
 } else { 
 if (1) r+=dxr, g+=dxg, b+=dxb; 
 if (0) o+=dxo, obar=1.0-o; 
 z+=dxz; 
 p++; 
 } 
 } 
 } 
 if (1) r1+=dyr, g1+=dyg, b1+=dyb; 
 if (0) o1+=dyo; 
 z1 += dyz; 
 A+=dyA, B+=dyB; 
 pp+=width; 
 } 
 } 
}
 { 
 
 if (iy3>height) 
 iy3 = height; 
 if (iy2<iy3 && iy3>0 && iy2<=height) { 
 B = x2; 
 d = y3 - y2; 
 d = d? (float)1.0 / (float)d : 0.0; 
 dyB = (float)(x3-x2) * d; 
 if (iy2<0) iy2=0; 
 d = iy2 - y2; 
 B += d*dyB; 
 for (iy=iy2; iy<iy3; iy++) { 
 iA = ((int)((A)-30000.0)+30000); 
 iB = ((int)((B)-30000.0)+30000); 
 if (iB>iA) left=iA, right=iB; 
 else left=iB, right=iA; 
 if (left<0) left=0; 
 if (right>width) right=width; 
 n = right - left; 
 if (n>0) { 
 nn += n; 
 p = pp+left; 
 d = left - A; 
 if (1) r=r1+d*dxr, g=g1+d*dxg, b=b1+d*dxb; 
 if (0) o=o1+d*dxo, obar=1.0-o; 
 z=z1+d*dxz; 
 while (--n>=0) { 
 if ((z>p->z)) { 
 { p->c.r=r; p->c.g=g; p->c.b=b; p->z=z; }; 
 if (1) r+=dxr, g+=dxg, b+=dxb; 
 if (0) o+=dxo, obar=1.0-o; 
 z+=dxz; 
 p++; 
 } else { 
 if (1) r+=dxr, g+=dxg, b+=dxb; 
 if (0) o+=dxo, obar=1.0-o; 
 z+=dxz; 
 p++; 
 } 
 } 
 } 
 if (1) r1+=dyr, g1+=dyg, b1+=dyb; 
 if (0) o1+=dyo; 
 z1 += dyz; 
 A+=dyA, B+=dyB; 
 pp+=width; 
 } 
 } 
}
 buf->pixels += nn;
};
 CAT(tof,1) :
 continue;
 }
};
    } else if (buf->pix_type==pix_big) {
      {
 Point *p;
 int v1, v2, v3, i, width, height;
 int i1, i2, i3;
 float ox, oy;
 InvalidComponentHandle iElts;
 RGBColor vcolors[6];
 int shademe;
 char fcst = xf->fcst, bcst = xf->bcst, ncst = xf->ncst;
 char fbyte = xf->fbyte, bbyte = xf->bbyte;
 iElts = (invalid_status == INV_UNKNOWN) ? xf->iElts : NULL;
 width = buf->width, height = buf->height;
 ox = buf->ox, oy = buf->oy;
 if (1 && xf->lights) {
 fcolors = (Pointer)vcolors;
 bcolors = (Pointer)(vcolors+3);
 _dxfInitApplyLights(xf->kaf, xf->kdf, xf->ksf, xf->kspf,
 xf->kab, xf->kdb, xf->ksb, xf->kspb,
 xf->fcolors, xf->bcolors, xf->cmap,
 xf->normals, xf->lights,
 xf->colors_dep, xf->normals_dep,
 fcolors, bcolors, 3, fcst, bcst, ncst,
 fbyte, bbyte);
 cmap = NULL;
 shademe = 1;
 } else {
 shademe = 0;
 if (fbyte || bbyte)
 _dxf_initUbyteToFloat();
 }
 for (i=0; i<ntri; i++, tri++) {
 int index = (indices == NULL) ? i : indices[i];
 if (iElts && DXIsElementInvalid(iElts, index))
 continue;
 v1=tri->p, p= &xf->positions[v1], x1=p->x, y1=p->y, z1=p->z;
 v2=tri->q, p= &xf->positions[v2], x2=p->x, y2=p->y, z2=p->z;
 v3=tri->r, p= &xf->positions[v3], x3=p->x, y3=p->y, z3=p->z;
 if (shademe)
 {
 i1 = 0; i2 = 1; i3 = 2;
 if (! _dxfApplyLights((int *)tri, &index, 1))
 return ERROR;
 }
 if (xf->tile.perspective) {
 if (z1>nearPlane || z2>nearPlane || z3>nearPlane)
 continue;
 z1=(float)-1.0/z1; x1*=z1; y1*=z1;
 z2=(float)-1.0/z2; x2*=z2; y2*=z2;
 z3=(float)-1.0/z3; x3*=z3; y3*=z3;
 }
 if ((x1<minx && x2<minx && x3<minx) ||
 (x1>maxx && x2>maxx && x3>maxx) ||
 (y1<miny && y2<miny && y3<miny) ||
 (y1>maxy && y2>maxy && y3>maxy))
 continue;
 buf->triangles++;
 {
 struct big *pp, *p;
 Pointer colors;
 int cstcolors;
 RGBColor cbuf1, cbuf2, cbuf3;
 char cbyte;
 char obyte = xf->obyte;
 x1+=ox, y1+=oy;
 x2+=ox, y2+=oy;
 x3+=ox, y3+=oy;
 if (x1<-(DXD_MAX_INT/2) || x1>(DXD_MAX_INT/2) || y1<-(DXD_MAX_INT/2) || y1>(DXD_MAX_INT/2) ||
 x2<-(DXD_MAX_INT/2) || x2>(DXD_MAX_INT/2) || y2<-(DXD_MAX_INT/2) || y2>(DXD_MAX_INT/2) ||
 x3<-(DXD_MAX_INT/2) || x3>(DXD_MAX_INT/2) || y3<-(DXD_MAX_INT/2) || y3>(DXD_MAX_INT/2))
 DXErrorReturn(ERROR_BAD_PARAMETER,
 "camera causes numerical overflow");
 d1 = y3-y2, d2 = y1-y3, d3 = y2-y1;
 d = x1*d1 + x2*d2 + x3*d3;
 Qx = d? (float)1.0 / (float)d : 0.0;
 d1 *= Qx, d2 *= Qx, d3 *= Qx;
 if (1) {
 if (d < 0)
 cbyte = fbyte, colors = fcolors, cstcolors = xf->fcst;
 else
 cbyte = bbyte, colors = bcolors, cstcolors = xf->bcst;
 if (!colors) goto CAT(tob,1);
 }
 if (1) {
 if (shademe) {
 c1 = ((RGBColor *)colors) + i1;
 c2 = ((RGBColor *)colors) + i2;
 c3 = ((RGBColor *)colors) + i3;
 } else {
 if (cmap)
 c1= &(cmap[((unsigned char *)colors)[cstcolors ? 0 : v1]]), c2= &(cmap[((unsigned char *)colors)[cstcolors ? 0 : v2]]), c3= &(cmap[((unsigned char *)colors)[cstcolors ? 0 : v3]]);
 else {
 
{ 
 if (cbyte) 
 { 
 RGBByteColor *ptr=((RGBByteColor *)colors)+(cstcolors ? 0 : v1); 
 cbuf1.r = _dxd_ubyteToFloat[ptr->r]; 
 cbuf1.g = _dxd_ubyteToFloat[ptr->g]; 
 cbuf1.b = _dxd_ubyteToFloat[ptr->b]; 
 } else 
 cbuf1 = (((RGBColor *)colors)[cstcolors ? 0 : v1]); 
}; 
{ 
 if (cbyte) 
 { 
 RGBByteColor *ptr=((RGBByteColor *)colors)+(cstcolors ? 0 : v2); 
 cbuf2.r = _dxd_ubyteToFloat[ptr->r]; 
 cbuf2.g = _dxd_ubyteToFloat[ptr->g]; 
 cbuf2.b = _dxd_ubyteToFloat[ptr->b]; 
 } else 
 cbuf2 = (((RGBColor *)colors)[cstcolors ? 0 : v2]); 
}; 
{ 
 if (cbyte) 
 { 
 RGBByteColor *ptr=((RGBByteColor *)colors)+(cstcolors ? 0 : v3); 
 cbuf3.r = _dxd_ubyteToFloat[ptr->r]; 
 cbuf3.g = _dxd_ubyteToFloat[ptr->g]; 
 cbuf3.b = _dxd_ubyteToFloat[ptr->b]; 
 } else 
 cbuf3 = (((RGBColor *)colors)[cstcolors ? 0 : v3]); 
};
 c1 = &cbuf1; c2 = &cbuf2; c3 = &cbuf3;
 }
 }
 r1=c1->r, g1=c1->g, b1=c1->b;
 r2=c2->r, g2=c2->g, b2=c2->b;
 r3=c3->r, g3=c3->g, b3=c3->b;
 } else if (1) {
 if (cmap){
 r=(cmap[((unsigned char *)colors)[cstcolors ? 0 : index]]).r; g=(cmap[((unsigned char *)colors)[cstcolors ? 0 : index]]).g; b=(cmap[((unsigned char *)colors)[cstcolors ? 0 : index]]).b;
 } else {
 
{ 
 if (cbyte) 
 { 
 RGBByteColor *ptr=((RGBByteColor *)colors)+(cstcolors ? 0 : index); 
 cbuf1.r = _dxd_ubyteToFloat[ptr->r]; 
 cbuf1.g = _dxd_ubyteToFloat[ptr->g]; 
 cbuf1.b = _dxd_ubyteToFloat[ptr->b]; 
 } else 
 cbuf1 = (((RGBColor *)colors)[cstcolors ? 0 : index]); 
};
 r = cbuf1.r; g = cbuf1.g; b = cbuf1.b;
 }
 }
 if (0) {
 if (omap) o1=(omap[((unsigned char *)opacities)[xf->ocst ? 0 : v1]]), o2=(omap[((unsigned char *)opacities)[xf->ocst ? 0 : v2]]), o3=(omap[((unsigned char *)opacities)[xf->ocst ? 0 : v3]]);
 else {
 
{ 
 if (obyte) 
 o1 = _dxd_ubyteToFloat[((ubyte *)opacities)[xf->ocst ? 0 : v1]]; 
 else 
 o1 = (((float *)opacities)[xf->ocst ? 0 : v1]); 
}; 
{ 
 if (obyte) 
 o2 = _dxd_ubyteToFloat[((ubyte *)opacities)[xf->ocst ? 0 : v2]]; 
 else 
 o2 = (((float *)opacities)[xf->ocst ? 0 : v2]); 
}; 
{ 
 if (obyte) 
 o3 = _dxd_ubyteToFloat[((ubyte *)opacities)[xf->ocst ? 0 : v3]]; 
 else 
 o3 = (((float *)opacities)[xf->ocst ? 0 : v3]); 
};
 }
 } else if (opacities||omap) {
 if (omap) o=(omap[((unsigned char *)opacities)[xf->ocst ? 0 : index]]);
 else 
{ 
 if (obyte) 
 o = _dxd_ubyteToFloat[((ubyte *)opacities)[xf->ocst ? 0 : index]]; 
 else 
 o = (((float *)opacities)[xf->ocst ? 0 : index]); 
};
 obar = 1.0-o;
 } else
 o = 1.0, obar = 0.0;
 if (1) {
 dxr = (r1 == r2 && r1 == r3) ? 0.0 : d1*r1 + d2*r2 + d3*r3;
 dxg = (g1 == g2 && g1 == g3) ? 0.0 : d1*g1 + d2*g2 + d3*g3;
 dxb = (b1 == b2 && b1 == b3) ? 0.0 : d1*b1 + d2*b2 + d3*b3;
 }
 if (0) dxo = (o1 == o2 && o1 == o3) ? 0.0 : d1*o1 + d2*o2 + d3*o3;
 dxz = (z1 == z2 && z1 == z3) ? 0.0 : d1*z1 + d2*z2 + d3*z3;
 { 
 if (y2 < y1) { 
 (d=y1, y1=y2, y2=d), (d=x1, x1=x2, x2=d); 
 if (1) (d=r1, r1=r2, r2=d), (d=g1, g1=g2, g2=d), (d=b1, b1=b2, b2=d); 
 if (0) (d=o1, o1=o2, o2=d); 
 (d=z1, z1=z2, z2=d); 
 } 
}
 { 
 if (y3 < y1) { 
 (d=y1, y1=y3, y3=d), (d=x1, x1=x3, x3=d); 
 if (1) (d=r1, r1=r3, r3=d), (d=g1, g1=g3, g3=d), (d=b1, b1=b3, b3=d); 
 if (0) (d=o1, o1=o3, o3=d); 
 (d=z1, z1=z3, z3=d); 
 } 
}
 { 
 if (y3 < y2) { 
 (d=y2, y2=y3, y3=d), (d=x2, x2=x3, x3=d); 
 if (1) (d=r2, r2=r3, r3=d), (d=g2, g2=g3, g3=d), (d=b2, b2=b3, b3=d); 
 if (0) (d=o2, o2=o3, o3=d); 
 (d=z2, z2=z3, z3=d); 
 } 
}
 if (0) {
 A = B = x1;
 if (x2<A) A = x2; else B = x2;
 if (x3<A) A = x3; else if (x3>B) B = x3;
 if (A<buf->fmin.x) buf->fmin.x = A;
 if (B>buf->fmax.x) buf->fmax.x = B;
 if (y1<buf->fmin.y) buf->fmin.y = y1;
 if (y3>buf->fmax.y) buf->fmax.y = y3;
 }
 iy1 = ((int)((y1)-30000.0)+30000), iy2 = ((int)((y2)-30000.0)+30000), iy3 = ((int)((y3)-30000.0)+30000);
 if (iy1>iy2 || iy2>iy3) {
 DXSetError(ERROR_DATA_INVALID,
 "position number %d, %d, or %d is invalid", v1, v2, v3);
 return ERROR;
 }
 if (iy1==iy3)
 goto CAT(tob,1);
 d = y3 - y1;
 Qy = d? (float)1.0 / (float)d : 0.0;
 if (1) dyr=(r3-r1)*Qy, dyg=(g3-g1)*Qy, dyb=(b3-b1)*Qy;
 if (0) dyo = (float)(o3-o1) * Qy;
 dyz = (z3-z1) * Qy;
 dyA = (float)(x3-x1) * Qy;
 A = x1;
 pp = buf->u.big + iy1*width;
 d = iy1 - y1;
 if (iy1<0)
 d-=iy1, pp=buf->u.big;
 A += dyA*d;
 if (1) r1+=dyr*d, g1+=dyg*d, b1+=dyb*d;
 if (0) o1+=dyo*d;
 z1 += dyz*d;
 nn = 0;
 { 
 
 if (iy2>height) 
 iy2 = height; 
 if (iy1<iy2 && iy2>0 && iy1<=height) { 
 B = x1; 
 d = y2 - y1; 
 d = d? (float)1.0 / (float)d : 0.0; 
 dyB = (float)(x2-x1) * d; 
 if (iy1<0) iy1=0; 
 d = iy1 - y1; 
 B += d*dyB; 
 for (iy=iy1; iy<iy2; iy++) { 
 iA = ((int)((A)-30000.0)+30000); 
 iB = ((int)((B)-30000.0)+30000); 
 if (iB>iA) left=iA, right=iB; 
 else left=iB, right=iA; 
 if (left<0) left=0; 
 if (right>width) right=width; 
 n = right - left; 
 if (n>0) { 
 nn += n; 
 p = pp+left; 
 d = left - A; 
 if (1) r=r1+d*dxr, g=g1+d*dxg, b=b1+d*dxb; 
 if (0) o=o1+d*dxo, obar=1.0-o; 
 z=z1+d*dxz; 
 while (--n>=0) { 
 if ((z>p->z)) { 
 { p->c.r=r; p->c.g=g; p->c.b=b; p->z=z; }; 
 if (1) r+=dxr, g+=dxg, b+=dxb; 
 if (0) o+=dxo, obar=1.0-o; 
 z+=dxz; 
 p++; 
 } else { 
 if (1) r+=dxr, g+=dxg, b+=dxb; 
 if (0) o+=dxo, obar=1.0-o; 
 z+=dxz; 
 p++; 
 } 
 } 
 } 
 if (1) r1+=dyr, g1+=dyg, b1+=dyb; 
 if (0) o1+=dyo; 
 z1 += dyz; 
 A+=dyA, B+=dyB; 
 pp+=width; 
 } 
 } 
}
 { 
 
 if (iy3>height) 
 iy3 = height; 
 if (iy2<iy3 && iy3>0 && iy2<=height) { 
 B = x2; 
 d = y3 - y2; 
 d = d? (float)1.0 / (float)d : 0.0; 
 dyB = (float)(x3-x2) * d; 
 if (iy2<0) iy2=0; 
 d = iy2 - y2; 
 B += d*dyB; 
 for (iy=iy2; iy<iy3; iy++) { 
 iA = ((int)((A)-30000.0)+30000); 
 iB = ((int)((B)-30000.0)+30000); 
 if (iB>iA) left=iA, right=iB; 
 else left=iB, right=iA; 
 if (left<0) left=0; 
 if (right>width) right=width; 
 n = right - left; 
 if (n>0) { 
 nn += n; 
 p = pp+left; 
 d = left - A; 
 if (1) r=r1+d*dxr, g=g1+d*dxg, b=b1+d*dxb; 
 if (0) o=o1+d*dxo, obar=1.0-o; 
 z=z1+d*dxz; 
 while (--n>=0) { 
 if ((z>p->z)) { 
 { p->c.r=r; p->c.g=g; p->c.b=b; p->z=z; }; 
 if (1) r+=dxr, g+=dxg, b+=dxb; 
 if (0) o+=dxo, obar=1.0-o; 
 z+=dxz; 
 p++; 
 } else { 
 if (1) r+=dxr, g+=dxg, b+=dxb; 
 if (0) o+=dxo, obar=1.0-o; 
 z+=dxz; 
 p++; 
 } 
 } 
 } 
 if (1) r1+=dyr, g1+=dyg, b1+=dyb; 
 if (0) o1+=dyo; 
 z1 += dyz; 
 A+=dyA, B+=dyB; 
 pp+=width; 
 } 
 } 
}
 buf->pixels += nn;
};
 CAT(tob,1) :
 continue;
 }
};
    } else {
      DXSetError(ERROR_INTERNAL, "unknown pix_type %d", buf->pix_type);
      return ERROR;
    }
 return OK;
 }
Error
_dxf_Triangle(
    struct buffer *buf,
    struct xfield *xf,
    int ntri,
    Triangle *tri,
    int *indices,
    int surface,
    int clip_status,
    inv_stat invalid_status
) {
    int i;
    if (xf->tile.perspective) {
 struct xfield xx;
 for (i=0; i<ntri; ) {
     i = _dxf_zclip_triangles(xf, (int *)tri, i, indices, ntri,
       &xx, invalid_status);
     if (xx.nconnections)
  _dxf_Triangle(buf, &xx, xx.nconnections, xx.c.triangles,
     xx.indices, surface, clip_status, INV_VALID);
 }
    }
    if (xf->volume)
 return tri_vol(buf, xf, ntri, tri, indices, surface,
     clip_status, invalid_status);
    else if (xf->opacities)
 return tri_translucent(buf, xf, ntri, tri, indices, surface,
     clip_status, invalid_status);
    else
 return tri_opaque(buf, xf, ntri, tri, indices, surface,
     clip_status, invalid_status);
}
 static Error
 tri_flat_face(
 struct buffer *buf,
 struct xfield *xf,
 int ntri,
 Triangle *tri,
 int *indices,
 Pointer fcolors,
 Pointer bcolors,
 Pointer opacities,
 int surface,
 int clip_status,
 inv_stat invalid_status
 ) {
 RGBColor *c1, *c2, *c3;
 float x1, y1, x2, y2, x3, y3;
 float r1, g1, b1, o1, z1, r2, g2, b2, o2, z2, r3, g3, b3, o3, z3;
 float Qx, dxo, dxz, dxr, dxg, dxb;
 float Qy, dyr, dyg, dyb, dyo, dyz, dyA, dyB;
 float r, g, b, o, obar, z, nearPlane=xf->nearPlane ;
 float A, B, d, d1, d2, d3;
 int iy1, iy2, iy3;
 int iA, iB, iy, i, n, nn, left, right;
 float minx = buf->min.x, miny = buf->min.y;
 float maxx = buf->max.x, maxy = buf->max.y;
 RGBColor *cmap = xf->cmap;
 float *omap = xf->omap;
    ASSERT(buf->pix_type==pix_big);
    {
 Point *p;
 int v1, v2, v3, i, width, height;
 int i1, i2, i3;
 float ox, oy;
 InvalidComponentHandle iElts;
 RGBColor vcolors[6];
 int shademe;
 char fcst = xf->fcst, bcst = xf->bcst, ncst = xf->ncst;
 char fbyte = xf->fbyte, bbyte = xf->bbyte;
 iElts = (invalid_status == INV_UNKNOWN) ? xf->iElts : NULL;
 width = buf->width, height = buf->height;
 ox = buf->ox, oy = buf->oy;
 if (0 && xf->lights) {
 fcolors = (Pointer)vcolors;
 bcolors = (Pointer)(vcolors+3);
 _dxfInitApplyLights(xf->kaf, xf->kdf, xf->ksf, xf->kspf,
 xf->kab, xf->kdb, xf->ksb, xf->kspb,
 xf->fcolors, xf->bcolors, xf->cmap,
 xf->normals, xf->lights,
 xf->colors_dep, xf->normals_dep,
 fcolors, bcolors, 3, fcst, bcst, ncst,
 fbyte, bbyte);
 cmap = NULL;
 shademe = 1;
 } else {
 shademe = 0;
 if (fbyte || bbyte)
 _dxf_initUbyteToFloat();
 }
 for (i=0; i<ntri; i++, tri++) {
 int index = (indices == NULL) ? i : indices[i];
 if (iElts && DXIsElementInvalid(iElts, index))
 continue;
 v1=tri->p, p= &xf->positions[v1], x1=p->x, y1=p->y, z1=p->z;
 v2=tri->q, p= &xf->positions[v2], x2=p->x, y2=p->y, z2=p->z;
 v3=tri->r, p= &xf->positions[v3], x3=p->x, y3=p->y, z3=p->z;
 if (shademe)
 {
 i1 = 0; i2 = 1; i3 = 2;
 if (! _dxfApplyLights((int *)tri, &index, 1))
 return ERROR;
 }
 if (xf->tile.perspective) {
 if (z1>nearPlane || z2>nearPlane || z3>nearPlane)
 continue;
 z1=(float)-1.0/z1; x1*=z1; y1*=z1;
 z2=(float)-1.0/z2; x2*=z2; y2*=z2;
 z3=(float)-1.0/z3; x3*=z3; y3*=z3;
 }
 if ((x1<minx && x2<minx && x3<minx) ||
 (x1>maxx && x2>maxx && x3>maxx) ||
 (y1<miny && y2<miny && y3<miny) ||
 (y1>maxy && y2>maxy && y3>maxy))
 continue;
 buf->triangles++;
 {
 struct big *pp, *p;
 Pointer colors;
 int cstcolors;
 RGBColor cbuf1, cbuf2, cbuf3;
 char cbyte;
 char obyte = xf->obyte;
 x1+=ox, y1+=oy;
 x2+=ox, y2+=oy;
 x3+=ox, y3+=oy;
 if (x1<-(DXD_MAX_INT/2) || x1>(DXD_MAX_INT/2) || y1<-(DXD_MAX_INT/2) || y1>(DXD_MAX_INT/2) ||
 x2<-(DXD_MAX_INT/2) || x2>(DXD_MAX_INT/2) || y2<-(DXD_MAX_INT/2) || y2>(DXD_MAX_INT/2) ||
 x3<-(DXD_MAX_INT/2) || x3>(DXD_MAX_INT/2) || y3<-(DXD_MAX_INT/2) || y3>(DXD_MAX_INT/2))
 DXErrorReturn(ERROR_BAD_PARAMETER,
 "camera causes numerical overflow");
 d1 = y3-y2, d2 = y1-y3, d3 = y2-y1;
 d = x1*d1 + x2*d2 + x3*d3;
 Qx = d? (float)1.0 / (float)d : 0.0;
 d1 *= Qx, d2 *= Qx, d3 *= Qx;
 if (1) {
 if (d < 0)
 cbyte = fbyte, colors = fcolors, cstcolors = xf->fcst;
 else
 cbyte = bbyte, colors = bcolors, cstcolors = xf->bcst;
 if (!colors) goto CAT(tff,1);
 }
 if (0) {
 if (shademe) {
 c1 = ((RGBColor *)colors) + i1;
 c2 = ((RGBColor *)colors) + i2;
 c3 = ((RGBColor *)colors) + i3;
 } else {
 if (cmap)
 c1= &(cmap[((unsigned char *)colors)[cstcolors ? 0 : v1]]), c2= &(cmap[((unsigned char *)colors)[cstcolors ? 0 : v2]]), c3= &(cmap[((unsigned char *)colors)[cstcolors ? 0 : v3]]);
 else {
 
{ 
 if (cbyte) 
 { 
 RGBByteColor *ptr=((RGBByteColor *)colors)+(cstcolors ? 0 : v1); 
 cbuf1.r = _dxd_ubyteToFloat[ptr->r]; 
 cbuf1.g = _dxd_ubyteToFloat[ptr->g]; 
 cbuf1.b = _dxd_ubyteToFloat[ptr->b]; 
 } else 
 cbuf1 = (((RGBColor *)colors)[cstcolors ? 0 : v1]); 
}; 
{ 
 if (cbyte) 
 { 
 RGBByteColor *ptr=((RGBByteColor *)colors)+(cstcolors ? 0 : v2); 
 cbuf2.r = _dxd_ubyteToFloat[ptr->r]; 
 cbuf2.g = _dxd_ubyteToFloat[ptr->g]; 
 cbuf2.b = _dxd_ubyteToFloat[ptr->b]; 
 } else 
 cbuf2 = (((RGBColor *)colors)[cstcolors ? 0 : v2]); 
}; 
{ 
 if (cbyte) 
 { 
 RGBByteColor *ptr=((RGBByteColor *)colors)+(cstcolors ? 0 : v3); 
 cbuf3.r = _dxd_ubyteToFloat[ptr->r]; 
 cbuf3.g = _dxd_ubyteToFloat[ptr->g]; 
 cbuf3.b = _dxd_ubyteToFloat[ptr->b]; 
 } else 
 cbuf3 = (((RGBColor *)colors)[cstcolors ? 0 : v3]); 
};
 c1 = &cbuf1; c2 = &cbuf2; c3 = &cbuf3;
 }
 }
 r1=c1->r, g1=c1->g, b1=c1->b;
 r2=c2->r, g2=c2->g, b2=c2->b;
 r3=c3->r, g3=c3->g, b3=c3->b;
 } else if (1) {
 if (cmap){
 r=(cmap[((unsigned char *)colors)[cstcolors ? 0 : index]]).r; g=(cmap[((unsigned char *)colors)[cstcolors ? 0 : index]]).g; b=(cmap[((unsigned char *)colors)[cstcolors ? 0 : index]]).b;
 } else {
 
{ 
 if (cbyte) 
 { 
 RGBByteColor *ptr=((RGBByteColor *)colors)+(cstcolors ? 0 : index); 
 cbuf1.r = _dxd_ubyteToFloat[ptr->r]; 
 cbuf1.g = _dxd_ubyteToFloat[ptr->g]; 
 cbuf1.b = _dxd_ubyteToFloat[ptr->b]; 
 } else 
 cbuf1 = (((RGBColor *)colors)[cstcolors ? 0 : index]); 
};
 r = cbuf1.r; g = cbuf1.g; b = cbuf1.b;
 }
 }
 if (0) {
 if (omap) o1=(omap[((unsigned char *)opacities)[xf->ocst ? 0 : v1]]), o2=(omap[((unsigned char *)opacities)[xf->ocst ? 0 : v2]]), o3=(omap[((unsigned char *)opacities)[xf->ocst ? 0 : v3]]);
 else {
 
{ 
 if (obyte) 
 o1 = _dxd_ubyteToFloat[((ubyte *)opacities)[xf->ocst ? 0 : v1]]; 
 else 
 o1 = (((float *)opacities)[xf->ocst ? 0 : v1]); 
}; 
{ 
 if (obyte) 
 o2 = _dxd_ubyteToFloat[((ubyte *)opacities)[xf->ocst ? 0 : v2]]; 
 else 
 o2 = (((float *)opacities)[xf->ocst ? 0 : v2]); 
}; 
{ 
 if (obyte) 
 o3 = _dxd_ubyteToFloat[((ubyte *)opacities)[xf->ocst ? 0 : v3]]; 
 else 
 o3 = (((float *)opacities)[xf->ocst ? 0 : v3]); 
};
 }
 } else if (opacities||omap) {
 if (omap) o=(omap[((unsigned char *)opacities)[xf->ocst ? 0 : index]]);
 else 
{ 
 if (obyte) 
 o = _dxd_ubyteToFloat[((ubyte *)opacities)[xf->ocst ? 0 : index]]; 
 else 
 o = (((float *)opacities)[xf->ocst ? 0 : index]); 
};
 obar = 1.0-o;
 } else
 o = 1.0, obar = 0.0;
 if (0) {
 dxr = (r1 == r2 && r1 == r3) ? 0.0 : d1*r1 + d2*r2 + d3*r3;
 dxg = (g1 == g2 && g1 == g3) ? 0.0 : d1*g1 + d2*g2 + d3*g3;
 dxb = (b1 == b2 && b1 == b3) ? 0.0 : d1*b1 + d2*b2 + d3*b3;
 }
 if (0) dxo = (o1 == o2 && o1 == o3) ? 0.0 : d1*o1 + d2*o2 + d3*o3;
 dxz = (z1 == z2 && z1 == z3) ? 0.0 : d1*z1 + d2*z2 + d3*z3;
 { 
 if (y2 < y1) { 
 (d=y1, y1=y2, y2=d), (d=x1, x1=x2, x2=d); 
 if (0) (d=r1, r1=r2, r2=d), (d=g1, g1=g2, g2=d), (d=b1, b1=b2, b2=d); 
 if (0) (d=o1, o1=o2, o2=d); 
 (d=z1, z1=z2, z2=d); 
 } 
}
 { 
 if (y3 < y1) { 
 (d=y1, y1=y3, y3=d), (d=x1, x1=x3, x3=d); 
 if (0) (d=r1, r1=r3, r3=d), (d=g1, g1=g3, g3=d), (d=b1, b1=b3, b3=d); 
 if (0) (d=o1, o1=o3, o3=d); 
 (d=z1, z1=z3, z3=d); 
 } 
}
 { 
 if (y3 < y2) { 
 (d=y2, y2=y3, y3=d), (d=x2, x2=x3, x3=d); 
 if (0) (d=r2, r2=r3, r3=d), (d=g2, g2=g3, g3=d), (d=b2, b2=b3, b3=d); 
 if (0) (d=o2, o2=o3, o3=d); 
 (d=z2, z2=z3, z3=d); 
 } 
}
 if (1) {
 A = B = x1;
 if (x2<A) A = x2; else B = x2;
 if (x3<A) A = x3; else if (x3>B) B = x3;
 if (A<buf->fmin.x) buf->fmin.x = A;
 if (B>buf->fmax.x) buf->fmax.x = B;
 if (y1<buf->fmin.y) buf->fmin.y = y1;
 if (y3>buf->fmax.y) buf->fmax.y = y3;
 }
 iy1 = ((int)((y1)-30000.0)+30000), iy2 = ((int)((y2)-30000.0)+30000), iy3 = ((int)((y3)-30000.0)+30000);
 if (iy1>iy2 || iy2>iy3) {
 DXSetError(ERROR_DATA_INVALID,
 "position number %d, %d, or %d is invalid", v1, v2, v3);
 return ERROR;
 }
 if (iy1==iy3)
 goto CAT(tff,1);
 d = y3 - y1;
 Qy = d? (float)1.0 / (float)d : 0.0;
 if (0) dyr=(r3-r1)*Qy, dyg=(g3-g1)*Qy, dyb=(b3-b1)*Qy;
 if (0) dyo = (float)(o3-o1) * Qy;
 dyz = (z3-z1) * Qy;
 dyA = (float)(x3-x1) * Qy;
 A = x1;
 pp = buf->u.big + iy1*width;
 d = iy1 - y1;
 if (iy1<0)
 d-=iy1, pp=buf->u.big;
 A += dyA*d;
 if (0) r1+=dyr*d, g1+=dyg*d, b1+=dyb*d;
 if (0) o1+=dyo*d;
 z1 += dyz*d;
 nn = 0;
 { 
 
 if (iy2>height) 
 iy2 = height; 
 if (iy1<iy2 && iy2>0 && iy1<=height) { 
 B = x1; 
 d = y2 - y1; 
 d = d? (float)1.0 / (float)d : 0.0; 
 dyB = (float)(x2-x1) * d; 
 if (iy1<0) iy1=0; 
 d = iy1 - y1; 
 B += d*dyB; 
 for (iy=iy1; iy<iy2; iy++) { 
 iA = ((int)((A)-30000.0)+30000); 
 iB = ((int)((B)-30000.0)+30000); 
 if (iB>iA) left=iA, right=iB; 
 else left=iB, right=iA; 
 if (left<0) left=0; 
 if (right>width) right=width; 
 n = right - left; 
 if (n>0) { 
 nn += n; 
 p = pp+left; 
 d = left - A; 
 if (0) r=r1+d*dxr, g=g1+d*dxg, b=b1+d*dxb; 
 if (0) o=o1+d*dxo, obar=1.0-o; 
 z=z1+d*dxz; 
 while (--n>=0) { 
 if (1) { 
 { p->co.r=r; p->co.g=g; p->co.b=b; p->co.o=z; p->in ^= IN_FACE; }; 
 if (0) r+=dxr, g+=dxg, b+=dxb; 
 if (0) o+=dxo, obar=1.0-o; 
 z+=dxz; 
 p++; 
 } else { 
 if (0) r+=dxr, g+=dxg, b+=dxb; 
 if (0) o+=dxo, obar=1.0-o; 
 z+=dxz; 
 p++; 
 } 
 } 
 } 
 if (0) r1+=dyr, g1+=dyg, b1+=dyb; 
 if (0) o1+=dyo; 
 z1 += dyz; 
 A+=dyA, B+=dyB; 
 pp+=width; 
 } 
 } 
}
 { 
 
 if (iy3>height) 
 iy3 = height; 
 if (iy2<iy3 && iy3>0 && iy2<=height) { 
 B = x2; 
 d = y3 - y2; 
 d = d? (float)1.0 / (float)d : 0.0; 
 dyB = (float)(x3-x2) * d; 
 if (iy2<0) iy2=0; 
 d = iy2 - y2; 
 B += d*dyB; 
 for (iy=iy2; iy<iy3; iy++) { 
 iA = ((int)((A)-30000.0)+30000); 
 iB = ((int)((B)-30000.0)+30000); 
 if (iB>iA) left=iA, right=iB; 
 else left=iB, right=iA; 
 if (left<0) left=0; 
 if (right>width) right=width; 
 n = right - left; 
 if (n>0) { 
 nn += n; 
 p = pp+left; 
 d = left - A; 
 if (0) r=r1+d*dxr, g=g1+d*dxg, b=b1+d*dxb; 
 if (0) o=o1+d*dxo, obar=1.0-o; 
 z=z1+d*dxz; 
 while (--n>=0) { 
 if (1) { 
 { p->co.r=r; p->co.g=g; p->co.b=b; p->co.o=z; p->in ^= IN_FACE; }; 
 if (0) r+=dxr, g+=dxg, b+=dxb; 
 if (0) o+=dxo, obar=1.0-o; 
 z+=dxz; 
 p++; 
 } else { 
 if (0) r+=dxr, g+=dxg, b+=dxb; 
 if (0) o+=dxo, obar=1.0-o; 
 z+=dxz; 
 p++; 
 } 
 } 
 } 
 if (0) r1+=dyr, g1+=dyg, b1+=dyb; 
 if (0) o1+=dyo; 
 z1 += dyz; 
 A+=dyA, B+=dyB; 
 pp+=width; 
 } 
 } 
}
 buf->pixels += nn;
};
 CAT(tff,1) :
 continue;
 }
};
 return OK;
 }
 static Error
 tri_flat_vol(
 struct buffer *buf,
 struct xfield *xf,
 int ntri,
 Triangle *tri,
 int *indices,
 Pointer fcolors,
 Pointer bcolors,
 Pointer opacities,
 int surface,
 int clip_status,
 inv_stat invalid_status
 ) {
 RGBColor *c1, *c2, *c3;
 float x1, y1, x2, y2, x3, y3;
 float r1, g1, b1, o1, z1, r2, g2, b2, o2, z2, r3, g3, b3, o3, z3;
 float Qx, dxo, dxz, dxr, dxg, dxb;
 float Qy, dyr, dyg, dyb, dyo, dyz, dyA, dyB;
 float r, g, b, o, obar, z, nearPlane=xf->nearPlane ;
 float A, B, d, d1, d2, d3;
 int iy1, iy2, iy3;
 int iA, iB, iy, i, n, nn, left, right;
 float minx = buf->min.x, miny = buf->min.y;
 float maxx = buf->max.x, maxy = buf->max.y;
 RGBColor *cmap = xf->cmap;
 float *omap = xf->omap;
    float cmul = xf->tile.color_multiplier;
    float omul = xf->tile.opacity_multiplier / cmul;
    ASSERT(buf->pix_type==pix_big);
    if (xf->tile.perspective)
 DXErrorReturn(ERROR_BAD_PARAMETER,
      "perspective volume rendering is not supported");
    if (!buf->merged)
        _dxf_MergeBackIntoZ(buf);
    {
 Point *p;
 int v1, v2, v3, i, width, height, valid;
 float ox, oy;
 InvalidComponentHandle iElts;
 char fbyte = xf->fbyte, bbyte = xf->bbyte;
 switch(invalid_status) {
 case INV_VALID: iElts = NULL; valid = 1; break;
 case INV_INVALID: iElts = NULL; valid = 0; break;
 case INV_UNKNOWN: iElts = xf->iElts; break;
 }
 width = buf->width, height = buf->height;
 ox = buf->ox, oy = buf->oy;
 for (i=0; i<ntri; i++, tri++) {
 if (iElts)
 valid = DXIsElementValid(iElts, i);
 v1=tri->p, p= &xf->positions[v1], x1=p->x, y1=p->y, z1=p->z;
 v2=tri->q, p= &xf->positions[v2], x2=p->x, y2=p->y, z2=p->z;
 v3=tri->r, p= &xf->positions[v3], x3=p->x, y3=p->y, z3=p->z;
 if ((x1<minx && x2<minx && x3<minx) ||
 (x1>maxx && x2>maxx && x3>maxx) ||
 (y1<miny && y2<miny && y3<miny) ||
 (y1>maxy && y2>maxy && y3>maxy))
 continue;
 buf->triangles++;
 {
 struct big *pp, *p;
 Pointer colors;
 int cstcolors;
 RGBColor cbuf1, cbuf2, cbuf3;
 char cbyte;
 char obyte = xf->obyte;
 x1+=ox, y1+=oy;
 x2+=ox, y2+=oy;
 x3+=ox, y3+=oy;
 if (x1<-(DXD_MAX_INT/2) || x1>(DXD_MAX_INT/2) || y1<-(DXD_MAX_INT/2) || y1>(DXD_MAX_INT/2) ||
 x2<-(DXD_MAX_INT/2) || x2>(DXD_MAX_INT/2) || y2<-(DXD_MAX_INT/2) || y2>(DXD_MAX_INT/2) ||
 x3<-(DXD_MAX_INT/2) || x3>(DXD_MAX_INT/2) || y3<-(DXD_MAX_INT/2) || y3>(DXD_MAX_INT/2))
 DXErrorReturn(ERROR_BAD_PARAMETER,
 "camera causes numerical overflow");
 d1 = y3-y2, d2 = y1-y3, d3 = y2-y1;
 d = x1*d1 + x2*d2 + x3*d3;
 Qx = d? (float)1.0 / (float)d : 0.0;
 d1 *= Qx, d2 *= Qx, d3 *= Qx;
 if (valid) {
 if (1) {
 if (d < 0)
 cbyte = fbyte, colors = fcolors, cstcolors = xf->fcst;
 else
 cbyte = bbyte, colors = bcolors, cstcolors = xf->bcst;
 if (!colors) goto CAT(tfv,1);
 }
 if (0) {
 if (cmap) c1= &(cmap[((unsigned char *)colors)[cstcolors ? 0 : v1]]), c2= &(cmap[((unsigned char *)colors)[cstcolors ? 0 : v2]]), c3= &(cmap[((unsigned char *)colors)[cstcolors ? 0 : v3]]);
 else {
 
{ 
 if (cbyte) 
 { 
 RGBByteColor *ptr=((RGBByteColor *)colors)+(cstcolors ? 0 : v1); 
 cbuf1.r = _dxd_ubyteToFloat[ptr->r]; 
 cbuf1.g = _dxd_ubyteToFloat[ptr->g]; 
 cbuf1.b = _dxd_ubyteToFloat[ptr->b]; 
 } else 
 cbuf1 = (((RGBColor *)colors)[cstcolors ? 0 : v1]); 
}; 
{ 
 if (cbyte) 
 { 
 RGBByteColor *ptr=((RGBByteColor *)colors)+(cstcolors ? 0 : v2); 
 cbuf2.r = _dxd_ubyteToFloat[ptr->r]; 
 cbuf2.g = _dxd_ubyteToFloat[ptr->g]; 
 cbuf2.b = _dxd_ubyteToFloat[ptr->b]; 
 } else 
 cbuf2 = (((RGBColor *)colors)[cstcolors ? 0 : v2]); 
}; 
{ 
 if (cbyte) 
 { 
 RGBByteColor *ptr=((RGBByteColor *)colors)+(cstcolors ? 0 : v3); 
 cbuf3.r = _dxd_ubyteToFloat[ptr->r]; 
 cbuf3.g = _dxd_ubyteToFloat[ptr->g]; 
 cbuf3.b = _dxd_ubyteToFloat[ptr->b]; 
 } else 
 cbuf3 = (((RGBColor *)colors)[cstcolors ? 0 : v3]); 
};
 c1 = &cbuf1; c2 = &cbuf2; c3 = &cbuf3;
 }
 r1=c1->r, g1=c1->g, b1=c1->b;
 r2=c2->r, g2=c2->g, b2=c2->b;
 r3=c3->r, g3=c3->g, b3=c3->b;
 } else if (1) {
 if (cmap) r=(cmap[((unsigned char *)colors)[cstcolors ? 0 : i]]).r, g=(cmap[((unsigned char *)colors)[cstcolors ? 0 : i]]).g, b=(cmap[((unsigned char *)colors)[cstcolors ? 0 : i]]).b;
 else {
 
{ 
 if (cbyte) 
 { 
 RGBByteColor *ptr=((RGBByteColor *)colors)+(cstcolors ? 0 : i); 
 cbuf1.r = _dxd_ubyteToFloat[ptr->r]; 
 cbuf1.g = _dxd_ubyteToFloat[ptr->g]; 
 cbuf1.b = _dxd_ubyteToFloat[ptr->b]; 
 } else 
 cbuf1 = (((RGBColor *)colors)[cstcolors ? 0 : i]); 
};
 r = cbuf1.r; g = cbuf1.g; b = cbuf1.b;
 }
 }
 if (0) {
 if (omap) o1=(omap[((unsigned char *)opacities)[xf->ocst ? 0 : v1]]), o2=(omap[((unsigned char *)opacities)[xf->ocst ? 0 : v2]]), o3=(omap[((unsigned char *)opacities)[xf->ocst ? 0 : v3]]);
 else {
 
{ 
 if (obyte) 
 o1 = _dxd_ubyteToFloat[((ubyte *)opacities)[xf->ocst ? 0 : v1]]; 
 else 
 o1 = (((float *)opacities)[xf->ocst ? 0 : v1]); 
}; 
{ 
 if (obyte) 
 o2 = _dxd_ubyteToFloat[((ubyte *)opacities)[xf->ocst ? 0 : v2]]; 
 else 
 o2 = (((float *)opacities)[xf->ocst ? 0 : v2]); 
}; 
{ 
 if (obyte) 
 o3 = _dxd_ubyteToFloat[((ubyte *)opacities)[xf->ocst ? 0 : v3]]; 
 else 
 o3 = (((float *)opacities)[xf->ocst ? 0 : v3]); 
};
 }
 } else if (opacities||omap) {
 if (omap) o=(omap[((unsigned char *)opacities)[xf->ocst ? 0 : i]]);
 else 
{ 
 if (obyte) 
 o = _dxd_ubyteToFloat[((ubyte *)opacities)[xf->ocst ? 0 : i]]; 
 else 
 o = (((float *)opacities)[xf->ocst ? 0 : i]); 
};
 obar = 1.0-o;
 } else
 o = 1.0, obar = 0.0;
 if (0) dxr = d1*r1 + d2*r2 + d3*r3;
 if (0) dxg = d1*g1 + d2*g2 + d3*g3;
 if (0) dxb = d1*b1 + d2*b2 + d3*b3;
 if (0) dxo = d1*o1 + d2*o2 + d3*o3;
 } else
 r1 = r2 = r3 = g1 = g2 = g3 = b1 = b2 = b3 = o1 = o2 = o3 = 0.0;
 dxz = d1*z1 + d2*z2 + d3*z3;
 { 
 if (y2 < y1) { 
 (d=y1, y1=y2, y2=d), (d=x1, x1=x2, x2=d); 
 if (0) (d=r1, r1=r2, r2=d), (d=g1, g1=g2, g2=d), (d=b1, b1=b2, b2=d); 
 if (0) (d=o1, o1=o2, o2=d); 
 (d=z1, z1=z2, z2=d); 
 } 
}
 { 
 if (y3 < y1) { 
 (d=y1, y1=y3, y3=d), (d=x1, x1=x3, x3=d); 
 if (0) (d=r1, r1=r3, r3=d), (d=g1, g1=g3, g3=d), (d=b1, b1=b3, b3=d); 
 if (0) (d=o1, o1=o3, o3=d); 
 (d=z1, z1=z3, z3=d); 
 } 
}
 { 
 if (y3 < y2) { 
 (d=y2, y2=y3, y3=d), (d=x2, x2=x3, x3=d); 
 if (0) (d=r2, r2=r3, r3=d), (d=g2, g2=g3, g3=d), (d=b2, b2=b3, b3=d); 
 if (0) (d=o2, o2=o3, o3=d); 
 (d=z2, z2=z3, z3=d); 
 } 
}
 if (0) {
 A = B = x1;
 if (x2<A) A = x2; else B = x2;
 if (x3<A) A = x3; else if (x3>B) B = x3;
 if (A<buf->fmin.x) buf->fmin.x = A;
 if (B>buf->fmax.x) buf->fmax.x = B;
 if (y1<buf->fmin.y) buf->fmin.y = y1;
 if (y3>buf->fmax.y) buf->fmax.y = y3;
 }
 iy1 = ((int)((y1)-30000.0)+30000), iy2 = ((int)((y2)-30000.0)+30000), iy3 = ((int)((y3)-30000.0)+30000);
 if (iy1>iy2 || iy2>iy3) {
 DXSetError(ERROR_DATA_INVALID,
 "position number %d, %d, or %d is invalid", v1, v2, v3);
 return ERROR;
 }
 if (iy1==iy3)
 goto CAT(tfv,1);
 d = y3 - y1;
 Qy = d? (float)1.0 / (float)d : 0.0;
 if (valid) {
 if (0) dyr=(r3-r1)*Qy, dyg=(g3-g1)*Qy, dyb=(b3-b1)*Qy;
 if (0) dyo = (float)(o3-o1) * Qy;
 }
 dyz = (z3-z1) * Qy;
 dyA = (float)(x3-x1) * Qy;
 A = x1;
 pp = buf->u.big + iy1*width;
 d = iy1 - y1;
 if (iy1<0)
 d-=iy1, pp=buf->u.big;
 A += dyA*d;
 if (valid) {
 if (0) r1+=dyr*d, g1+=dyg*d, b1+=dyb*d;
 if (0) o1+=dyo*d;
 }
 z1 += dyz*d;
 nn = 0;
 if (valid) {
 { 
 
 if (iy2>height) 
 iy2 = height; 
 if (iy1<iy2 && iy2>0 && iy1<=height) { 
 B = x1; 
 d = y2 - y1; 
 d = d? (float)1.0 / (float)d : 0.0; 
 dyB = (float)(x2-x1) * d; 
 if (iy1<0) iy1=0; 
 d = iy1 - y1; 
 B += d*dyB; 
 for (iy=iy1; iy<iy2; iy++) { 
 iA = ((int)((A)-30000.0)+30000); 
 iB = ((int)((B)-30000.0)+30000); 
 if (iB>iA) left=iA, right=iB; 
 else left=iB, right=iA; 
 if (left<0) left=0; 
 if (right>width) right=width; 
 n = right - left; 
 if (n>0) { 
 nn += n; 
 p = pp+left; 
 d = left - A; 
 if (0) r=r1+d*dxr, g=g1+d*dxg, b=b1+d*dxb; 
 if (0) o=o1+d*dxo, obar=1.0-o; 
 z=z1+d*dxz; 
 while (--n>=0) { 
 if (1) { 
 { if (p->in<=0) { if (z < p->front) { if (surface) { p->in++; if (z > p->back) p->back = z; if (valid && !xf->tile.flat_z) { p->co.r=r; p->co.g=g; p->co.b=b; } } } } else { if (surface) p->in = 0; if (z >= p->front) { p->in = -DXD_MAX_INT; z = p->front; } if (z > p->back) { float ar; float ag; float ab; float ao; if (valid) { if ( 1) { d = (z - p->back) * cmul; ar = d * r; ag = d * g; ab = d * b; ao = d * o * omul; } else { d = 0.5 * (z - p->back) * cmul; ar = d * (p->co.r+r); ag = d * (p->co.g+g); ab = d * (p->co.b+b); ao = d * (p->co.o+r) * omul; p->co.r = r; p->co.g = g; p->co.b = b; p->co.o = o; } if ( 1) { if ((obar=1-ao) < 0.0) obar = 0.0; } else { if ((obar=1-0.125*ao) < 0.0) obar = 0.0; obar = obar*obar; obar = obar*obar; obar = obar*obar; } p->c.r = p->c.r * obar + ar; p->c.g = p->c.g * obar + ag; p->c.b = p->c.b * obar + ab; } p->back = z; } } }; 
 if (0) r+=dxr, g+=dxg, b+=dxb; 
 if (0) o+=dxo, obar=1.0-o; 
 z+=dxz; 
 p++; 
 } else { 
 if (0) r+=dxr, g+=dxg, b+=dxb; 
 if (0) o+=dxo, obar=1.0-o; 
 z+=dxz; 
 p++; 
 } 
 } 
 } 
 if (0) r1+=dyr, g1+=dyg, b1+=dyb; 
 if (0) o1+=dyo; 
 z1 += dyz; 
 A+=dyA, B+=dyB; 
 pp+=width; 
 } 
 } 
}
 { 
 
 if (iy3>height) 
 iy3 = height; 
 if (iy2<iy3 && iy3>0 && iy2<=height) { 
 B = x2; 
 d = y3 - y2; 
 d = d? (float)1.0 / (float)d : 0.0; 
 dyB = (float)(x3-x2) * d; 
 if (iy2<0) iy2=0; 
 d = iy2 - y2; 
 B += d*dyB; 
 for (iy=iy2; iy<iy3; iy++) { 
 iA = ((int)((A)-30000.0)+30000); 
 iB = ((int)((B)-30000.0)+30000); 
 if (iB>iA) left=iA, right=iB; 
 else left=iB, right=iA; 
 if (left<0) left=0; 
 if (right>width) right=width; 
 n = right - left; 
 if (n>0) { 
 nn += n; 
 p = pp+left; 
 d = left - A; 
 if (0) r=r1+d*dxr, g=g1+d*dxg, b=b1+d*dxb; 
 if (0) o=o1+d*dxo, obar=1.0-o; 
 z=z1+d*dxz; 
 while (--n>=0) { 
 if (1) { 
 { if (p->in<=0) { if (z < p->front) { if (surface) { p->in++; if (z > p->back) p->back = z; if (valid && !xf->tile.flat_z) { p->co.r=r; p->co.g=g; p->co.b=b; } } } } else { if (surface) p->in = 0; if (z >= p->front) { p->in = -DXD_MAX_INT; z = p->front; } if (z > p->back) { float ar; float ag; float ab; float ao; if (valid) { if ( 1) { d = (z - p->back) * cmul; ar = d * r; ag = d * g; ab = d * b; ao = d * o * omul; } else { d = 0.5 * (z - p->back) * cmul; ar = d * (p->co.r+r); ag = d * (p->co.g+g); ab = d * (p->co.b+b); ao = d * (p->co.o+r) * omul; p->co.r = r; p->co.g = g; p->co.b = b; p->co.o = o; } if ( 1) { if ((obar=1-ao) < 0.0) obar = 0.0; } else { if ((obar=1-0.125*ao) < 0.0) obar = 0.0; obar = obar*obar; obar = obar*obar; obar = obar*obar; } p->c.r = p->c.r * obar + ar; p->c.g = p->c.g * obar + ag; p->c.b = p->c.b * obar + ab; } p->back = z; } } }; 
 if (0) r+=dxr, g+=dxg, b+=dxb; 
 if (0) o+=dxo, obar=1.0-o; 
 z+=dxz; 
 p++; 
 } else { 
 if (0) r+=dxr, g+=dxg, b+=dxb; 
 if (0) o+=dxo, obar=1.0-o; 
 z+=dxz; 
 p++; 
 } 
 } 
 } 
 if (0) r1+=dyr, g1+=dyg, b1+=dyb; 
 if (0) o1+=dyo; 
 z1 += dyz; 
 A+=dyA, B+=dyB; 
 pp+=width; 
 } 
 } 
}
 } else {
 { 
 
 if (iy2>height) 
 iy2 = height; 
 if (iy1<iy2 && iy2>0 && iy1<=height) { 
 B = x1; 
 d = y2 - y1; 
 d = d? (float)1.0 / (float)d : 0.0; 
 dyB = (float)(x2-x1) * d; 
 if (iy1<0) iy1=0; 
 d = iy1 - y1; 
 B += d*dyB; 
 for (iy=iy1; iy<iy2; iy++) { 
 iA = ((int)((A)-30000.0)+30000); 
 iB = ((int)((B)-30000.0)+30000); 
 if (iB>iA) left=iA, right=iB; 
 else left=iB, right=iA; 
 if (left<0) left=0; 
 if (right>width) right=width; 
 n = right - left; 
 if (n>0) { 
 nn += n; 
 p = pp+left; 
 d = left - A; 
 if (0) r=r1+d*dxr, g=g1+d*dxg, b=b1+d*dxb; 
 if (0) o=o1+d*dxo, obar=1.0-o; 
 z=z1+d*dxz; 
 while (--n>=0) { 
 if (1) { 
 { if (p->in<=0) { if (z < p->front) { if (surface) { p->in++; if (z > p->back) p->back = z; if (valid && !xf->tile.flat_z) { p->co.r=r; p->co.g=g; p->co.b=b; } } } } else { if (surface) p->in = 0; if (z >= p->front) { p->in = -DXD_MAX_INT; z = p->front; } if (z > p->back) { float ar; float ag; float ab; float ao; if (valid) { if ( 1) { d = (z - p->back) * cmul; ar = d * r; ag = d * g; ab = d * b; ao = d * o * omul; } else { d = 0.5 * (z - p->back) * cmul; ar = d * (p->co.r+r); ag = d * (p->co.g+g); ab = d * (p->co.b+b); ao = d * (p->co.o+r) * omul; p->co.r = r; p->co.g = g; p->co.b = b; p->co.o = o; } if ( 1) { if ((obar=1-ao) < 0.0) obar = 0.0; } else { if ((obar=1-0.125*ao) < 0.0) obar = 0.0; obar = obar*obar; obar = obar*obar; obar = obar*obar; } p->c.r = p->c.r * obar + ar; p->c.g = p->c.g * obar + ag; p->c.b = p->c.b * obar + ab; } p->back = z; } } }; 
 if (0) r+=dxr, g+=dxg, b+=dxb; 
 if (0) o+=dxo, obar=1.0-o; 
 z+=dxz; 
 p++; 
 } else { 
 if (0) r+=dxr, g+=dxg, b+=dxb; 
 if (0) o+=dxo, obar=1.0-o; 
 z+=dxz; 
 p++; 
 } 
 } 
 } 
 if (0) r1+=dyr, g1+=dyg, b1+=dyb; 
 if (0) o1+=dyo; 
 z1 += dyz; 
 A+=dyA, B+=dyB; 
 pp+=width; 
 } 
 } 
}
 { 
 
 if (iy3>height) 
 iy3 = height; 
 if (iy2<iy3 && iy3>0 && iy2<=height) { 
 B = x2; 
 d = y3 - y2; 
 d = d? (float)1.0 / (float)d : 0.0; 
 dyB = (float)(x3-x2) * d; 
 if (iy2<0) iy2=0; 
 d = iy2 - y2; 
 B += d*dyB; 
 for (iy=iy2; iy<iy3; iy++) { 
 iA = ((int)((A)-30000.0)+30000); 
 iB = ((int)((B)-30000.0)+30000); 
 if (iB>iA) left=iA, right=iB; 
 else left=iB, right=iA; 
 if (left<0) left=0; 
 if (right>width) right=width; 
 n = right - left; 
 if (n>0) { 
 nn += n; 
 p = pp+left; 
 d = left - A; 
 if (0) r=r1+d*dxr, g=g1+d*dxg, b=b1+d*dxb; 
 if (0) o=o1+d*dxo, obar=1.0-o; 
 z=z1+d*dxz; 
 while (--n>=0) { 
 if (1) { 
 { if (p->in<=0) { if (z < p->front) { if (surface) { p->in++; if (z > p->back) p->back = z; if (valid && !xf->tile.flat_z) { p->co.r=r; p->co.g=g; p->co.b=b; } } } } else { if (surface) p->in = 0; if (z >= p->front) { p->in = -DXD_MAX_INT; z = p->front; } if (z > p->back) { float ar; float ag; float ab; float ao; if (valid) { if ( 1) { d = (z - p->back) * cmul; ar = d * r; ag = d * g; ab = d * b; ao = d * o * omul; } else { d = 0.5 * (z - p->back) * cmul; ar = d * (p->co.r+r); ag = d * (p->co.g+g); ab = d * (p->co.b+b); ao = d * (p->co.o+r) * omul; p->co.r = r; p->co.g = g; p->co.b = b; p->co.o = o; } if ( 1) { if ((obar=1-ao) < 0.0) obar = 0.0; } else { if ((obar=1-0.125*ao) < 0.0) obar = 0.0; obar = obar*obar; obar = obar*obar; obar = obar*obar; } p->c.r = p->c.r * obar + ar; p->c.g = p->c.g * obar + ag; p->c.b = p->c.b * obar + ab; } p->back = z; } } }; 
 if (0) r+=dxr, g+=dxg, b+=dxb; 
 if (0) o+=dxo, obar=1.0-o; 
 z+=dxz; 
 p++; 
 } else { 
 if (0) r+=dxr, g+=dxg, b+=dxb; 
 if (0) o+=dxo, obar=1.0-o; 
 z+=dxz; 
 p++; 
 } 
 } 
 } 
 if (0) r1+=dyr, g1+=dyg, b1+=dyb; 
 if (0) o1+=dyo; 
 z1 += dyz; 
 A+=dyA, B+=dyB; 
 pp+=width; 
 } 
 } 
}
 }
 buf->pixels += nn;
};
 CAT(tfv,1) :
 continue;
 }
};
 return OK;
 }
 static Error
 tri_flat_translucent(
 struct buffer *buf,
 struct xfield *xf,
 int ntri,
 Triangle *tri,
 int *indices,
 Pointer fcolors,
 Pointer bcolors,
 Pointer opacities,
 int surface,
 int clip_status,
 inv_stat invalid_status
 ) {
 RGBColor *c1, *c2, *c3;
 float x1, y1, x2, y2, x3, y3;
 float r1, g1, b1, o1, z1, r2, g2, b2, o2, z2, r3, g3, b3, o3, z3;
 float Qx, dxo, dxz, dxr, dxg, dxb;
 float Qy, dyr, dyg, dyb, dyo, dyz, dyA, dyB;
 float r, g, b, o, obar, z, nearPlane=xf->nearPlane ;
 float A, B, d, d1, d2, d3;
 int iy1, iy2, iy3;
 int iA, iB, iy, i, n, nn, left, right;
 float minx = buf->min.x, miny = buf->min.y;
 float maxx = buf->max.x, maxy = buf->max.y;
 RGBColor *cmap = xf->cmap;
 float *omap = xf->omap;
    if (clip_status) {
      ASSERT(buf->pix_type==pix_big);
      {
 Point *p;
 int v1, v2, v3, i, width, height;
 int i1, i2, i3;
 float ox, oy;
 InvalidComponentHandle iElts;
 RGBColor vcolors[6];
 int shademe;
 char fcst = xf->fcst, bcst = xf->bcst, ncst = xf->ncst;
 char fbyte = xf->fbyte, bbyte = xf->bbyte;
 iElts = (invalid_status == INV_UNKNOWN) ? xf->iElts : NULL;
 width = buf->width, height = buf->height;
 ox = buf->ox, oy = buf->oy;
 if (0 && xf->lights) {
 fcolors = (Pointer)vcolors;
 bcolors = (Pointer)(vcolors+3);
 _dxfInitApplyLights(xf->kaf, xf->kdf, xf->ksf, xf->kspf,
 xf->kab, xf->kdb, xf->ksb, xf->kspb,
 xf->fcolors, xf->bcolors, xf->cmap,
 xf->normals, xf->lights,
 xf->colors_dep, xf->normals_dep,
 fcolors, bcolors, 3, fcst, bcst, ncst,
 fbyte, bbyte);
 cmap = NULL;
 shademe = 1;
 } else {
 shademe = 0;
 if (fbyte || bbyte)
 _dxf_initUbyteToFloat();
 }
 for (i=0; i<ntri; i++, tri++) {
 int index = (indices == NULL) ? i : indices[i];
 if (iElts && DXIsElementInvalid(iElts, index))
 continue;
 v1=tri->p, p= &xf->positions[v1], x1=p->x, y1=p->y, z1=p->z;
 v2=tri->q, p= &xf->positions[v2], x2=p->x, y2=p->y, z2=p->z;
 v3=tri->r, p= &xf->positions[v3], x3=p->x, y3=p->y, z3=p->z;
 if (shademe)
 {
 i1 = 0; i2 = 1; i3 = 2;
 if (! _dxfApplyLights((int *)tri, &index, 1))
 return ERROR;
 }
 if (xf->tile.perspective) {
 if (z1>nearPlane || z2>nearPlane || z3>nearPlane)
 continue;
 z1=(float)-1.0/z1; x1*=z1; y1*=z1;
 z2=(float)-1.0/z2; x2*=z2; y2*=z2;
 z3=(float)-1.0/z3; x3*=z3; y3*=z3;
 }
 if ((x1<minx && x2<minx && x3<minx) ||
 (x1>maxx && x2>maxx && x3>maxx) ||
 (y1<miny && y2<miny && y3<miny) ||
 (y1>maxy && y2>maxy && y3>maxy))
 continue;
 buf->triangles++;
 {
 struct big *pp, *p;
 Pointer colors;
 int cstcolors;
 RGBColor cbuf1, cbuf2, cbuf3;
 char cbyte;
 char obyte = xf->obyte;
 x1+=ox, y1+=oy;
 x2+=ox, y2+=oy;
 x3+=ox, y3+=oy;
 if (x1<-(DXD_MAX_INT/2) || x1>(DXD_MAX_INT/2) || y1<-(DXD_MAX_INT/2) || y1>(DXD_MAX_INT/2) ||
 x2<-(DXD_MAX_INT/2) || x2>(DXD_MAX_INT/2) || y2<-(DXD_MAX_INT/2) || y2>(DXD_MAX_INT/2) ||
 x3<-(DXD_MAX_INT/2) || x3>(DXD_MAX_INT/2) || y3<-(DXD_MAX_INT/2) || y3>(DXD_MAX_INT/2))
 DXErrorReturn(ERROR_BAD_PARAMETER,
 "camera causes numerical overflow");
 d1 = y3-y2, d2 = y1-y3, d3 = y2-y1;
 d = x1*d1 + x2*d2 + x3*d3;
 Qx = d? (float)1.0 / (float)d : 0.0;
 d1 *= Qx, d2 *= Qx, d3 *= Qx;
 if (1) {
 if (d < 0)
 cbyte = fbyte, colors = fcolors, cstcolors = xf->fcst;
 else
 cbyte = bbyte, colors = bcolors, cstcolors = xf->bcst;
 if (!colors) goto CAT(tftc,1);
 }
 if (0) {
 if (shademe) {
 c1 = ((RGBColor *)colors) + i1;
 c2 = ((RGBColor *)colors) + i2;
 c3 = ((RGBColor *)colors) + i3;
 } else {
 if (cmap)
 c1= &(cmap[((unsigned char *)colors)[cstcolors ? 0 : v1]]), c2= &(cmap[((unsigned char *)colors)[cstcolors ? 0 : v2]]), c3= &(cmap[((unsigned char *)colors)[cstcolors ? 0 : v3]]);
 else {
 
{ 
 if (cbyte) 
 { 
 RGBByteColor *ptr=((RGBByteColor *)colors)+(cstcolors ? 0 : v1); 
 cbuf1.r = _dxd_ubyteToFloat[ptr->r]; 
 cbuf1.g = _dxd_ubyteToFloat[ptr->g]; 
 cbuf1.b = _dxd_ubyteToFloat[ptr->b]; 
 } else 
 cbuf1 = (((RGBColor *)colors)[cstcolors ? 0 : v1]); 
}; 
{ 
 if (cbyte) 
 { 
 RGBByteColor *ptr=((RGBByteColor *)colors)+(cstcolors ? 0 : v2); 
 cbuf2.r = _dxd_ubyteToFloat[ptr->r]; 
 cbuf2.g = _dxd_ubyteToFloat[ptr->g]; 
 cbuf2.b = _dxd_ubyteToFloat[ptr->b]; 
 } else 
 cbuf2 = (((RGBColor *)colors)[cstcolors ? 0 : v2]); 
}; 
{ 
 if (cbyte) 
 { 
 RGBByteColor *ptr=((RGBByteColor *)colors)+(cstcolors ? 0 : v3); 
 cbuf3.r = _dxd_ubyteToFloat[ptr->r]; 
 cbuf3.g = _dxd_ubyteToFloat[ptr->g]; 
 cbuf3.b = _dxd_ubyteToFloat[ptr->b]; 
 } else 
 cbuf3 = (((RGBColor *)colors)[cstcolors ? 0 : v3]); 
};
 c1 = &cbuf1; c2 = &cbuf2; c3 = &cbuf3;
 }
 }
 r1=c1->r, g1=c1->g, b1=c1->b;
 r2=c2->r, g2=c2->g, b2=c2->b;
 r3=c3->r, g3=c3->g, b3=c3->b;
 } else if (1) {
 if (cmap){
 r=(cmap[((unsigned char *)colors)[cstcolors ? 0 : index]]).r; g=(cmap[((unsigned char *)colors)[cstcolors ? 0 : index]]).g; b=(cmap[((unsigned char *)colors)[cstcolors ? 0 : index]]).b;
 } else {
 
{ 
 if (cbyte) 
 { 
 RGBByteColor *ptr=((RGBByteColor *)colors)+(cstcolors ? 0 : index); 
 cbuf1.r = _dxd_ubyteToFloat[ptr->r]; 
 cbuf1.g = _dxd_ubyteToFloat[ptr->g]; 
 cbuf1.b = _dxd_ubyteToFloat[ptr->b]; 
 } else 
 cbuf1 = (((RGBColor *)colors)[cstcolors ? 0 : index]); 
};
 r = cbuf1.r; g = cbuf1.g; b = cbuf1.b;
 }
 }
 if (0) {
 if (omap) o1=(omap[((unsigned char *)opacities)[xf->ocst ? 0 : v1]]), o2=(omap[((unsigned char *)opacities)[xf->ocst ? 0 : v2]]), o3=(omap[((unsigned char *)opacities)[xf->ocst ? 0 : v3]]);
 else {
 
{ 
 if (obyte) 
 o1 = _dxd_ubyteToFloat[((ubyte *)opacities)[xf->ocst ? 0 : v1]]; 
 else 
 o1 = (((float *)opacities)[xf->ocst ? 0 : v1]); 
}; 
{ 
 if (obyte) 
 o2 = _dxd_ubyteToFloat[((ubyte *)opacities)[xf->ocst ? 0 : v2]]; 
 else 
 o2 = (((float *)opacities)[xf->ocst ? 0 : v2]); 
}; 
{ 
 if (obyte) 
 o3 = _dxd_ubyteToFloat[((ubyte *)opacities)[xf->ocst ? 0 : v3]]; 
 else 
 o3 = (((float *)opacities)[xf->ocst ? 0 : v3]); 
};
 }
 } else if (opacities||omap) {
 if (omap) o=(omap[((unsigned char *)opacities)[xf->ocst ? 0 : index]]);
 else 
{ 
 if (obyte) 
 o = _dxd_ubyteToFloat[((ubyte *)opacities)[xf->ocst ? 0 : index]]; 
 else 
 o = (((float *)opacities)[xf->ocst ? 0 : index]); 
};
 obar = 1.0-o;
 } else
 o = 1.0, obar = 0.0;
 if (0) {
 dxr = (r1 == r2 && r1 == r3) ? 0.0 : d1*r1 + d2*r2 + d3*r3;
 dxg = (g1 == g2 && g1 == g3) ? 0.0 : d1*g1 + d2*g2 + d3*g3;
 dxb = (b1 == b2 && b1 == b3) ? 0.0 : d1*b1 + d2*b2 + d3*b3;
 }
 if (0) dxo = (o1 == o2 && o1 == o3) ? 0.0 : d1*o1 + d2*o2 + d3*o3;
 dxz = (z1 == z2 && z1 == z3) ? 0.0 : d1*z1 + d2*z2 + d3*z3;
 { 
 if (y2 < y1) { 
 (d=y1, y1=y2, y2=d), (d=x1, x1=x2, x2=d); 
 if (0) (d=r1, r1=r2, r2=d), (d=g1, g1=g2, g2=d), (d=b1, b1=b2, b2=d); 
 if (0) (d=o1, o1=o2, o2=d); 
 (d=z1, z1=z2, z2=d); 
 } 
}
 { 
 if (y3 < y1) { 
 (d=y1, y1=y3, y3=d), (d=x1, x1=x3, x3=d); 
 if (0) (d=r1, r1=r3, r3=d), (d=g1, g1=g3, g3=d), (d=b1, b1=b3, b3=d); 
 if (0) (d=o1, o1=o3, o3=d); 
 (d=z1, z1=z3, z3=d); 
 } 
}
 { 
 if (y3 < y2) { 
 (d=y2, y2=y3, y3=d), (d=x2, x2=x3, x3=d); 
 if (0) (d=r2, r2=r3, r3=d), (d=g2, g2=g3, g3=d), (d=b2, b2=b3, b3=d); 
 if (0) (d=o2, o2=o3, o3=d); 
 (d=z2, z2=z3, z3=d); 
 } 
}
 if (0) {
 A = B = x1;
 if (x2<A) A = x2; else B = x2;
 if (x3<A) A = x3; else if (x3>B) B = x3;
 if (A<buf->fmin.x) buf->fmin.x = A;
 if (B>buf->fmax.x) buf->fmax.x = B;
 if (y1<buf->fmin.y) buf->fmin.y = y1;
 if (y3>buf->fmax.y) buf->fmax.y = y3;
 }
 iy1 = ((int)((y1)-30000.0)+30000), iy2 = ((int)((y2)-30000.0)+30000), iy3 = ((int)((y3)-30000.0)+30000);
 if (iy1>iy2 || iy2>iy3) {
 DXSetError(ERROR_DATA_INVALID,
 "position number %d, %d, or %d is invalid", v1, v2, v3);
 return ERROR;
 }
 if (iy1==iy3)
 goto CAT(tftc,1);
 d = y3 - y1;
 Qy = d? (float)1.0 / (float)d : 0.0;
 if (0) dyr=(r3-r1)*Qy, dyg=(g3-g1)*Qy, dyb=(b3-b1)*Qy;
 if (0) dyo = (float)(o3-o1) * Qy;
 dyz = (z3-z1) * Qy;
 dyA = (float)(x3-x1) * Qy;
 A = x1;
 pp = buf->u.big + iy1*width;
 d = iy1 - y1;
 if (iy1<0)
 d-=iy1, pp=buf->u.big;
 A += dyA*d;
 if (0) r1+=dyr*d, g1+=dyg*d, b1+=dyb*d;
 if (0) o1+=dyo*d;
 z1 += dyz*d;
 nn = 0;
 { 
 
 if (iy2>height) 
 iy2 = height; 
 if (iy1<iy2 && iy2>0 && iy1<=height) { 
 B = x1; 
 d = y2 - y1; 
 d = d? (float)1.0 / (float)d : 0.0; 
 dyB = (float)(x2-x1) * d; 
 if (iy1<0) iy1=0; 
 d = iy1 - y1; 
 B += d*dyB; 
 for (iy=iy1; iy<iy2; iy++) { 
 iA = ((int)((A)-30000.0)+30000); 
 iB = ((int)((B)-30000.0)+30000); 
 if (iB>iA) left=iA, right=iB; 
 else left=iB, right=iA; 
 if (left<0) left=0; 
 if (right>width) right=width; 
 n = right - left; 
 if (n>0) { 
 nn += n; 
 p = pp+left; 
 d = left - A; 
 if (0) r=r1+d*dxr, g=g1+d*dxg, b=b1+d*dxb; 
 if (0) o=o1+d*dxo, obar=1.0-o; 
 z=z1+d*dxz; 
 while (--n>=0) { 
 if ((z>p->z && z<p->front)) { 
 { p->c.r=o*r+obar*p->c.r; p->c.g=o*g+obar*p->c.g; p->c.b=o*b+obar*p->c.b; if (o > 0.5) p->z=z; }; 
 if (0) r+=dxr, g+=dxg, b+=dxb; 
 if (0) o+=dxo, obar=1.0-o; 
 z+=dxz; 
 p++; 
 } else { 
 if (0) r+=dxr, g+=dxg, b+=dxb; 
 if (0) o+=dxo, obar=1.0-o; 
 z+=dxz; 
 p++; 
 } 
 } 
 } 
 if (0) r1+=dyr, g1+=dyg, b1+=dyb; 
 if (0) o1+=dyo; 
 z1 += dyz; 
 A+=dyA, B+=dyB; 
 pp+=width; 
 } 
 } 
}
 { 
 
 if (iy3>height) 
 iy3 = height; 
 if (iy2<iy3 && iy3>0 && iy2<=height) { 
 B = x2; 
 d = y3 - y2; 
 d = d? (float)1.0 / (float)d : 0.0; 
 dyB = (float)(x3-x2) * d; 
 if (iy2<0) iy2=0; 
 d = iy2 - y2; 
 B += d*dyB; 
 for (iy=iy2; iy<iy3; iy++) { 
 iA = ((int)((A)-30000.0)+30000); 
 iB = ((int)((B)-30000.0)+30000); 
 if (iB>iA) left=iA, right=iB; 
 else left=iB, right=iA; 
 if (left<0) left=0; 
 if (right>width) right=width; 
 n = right - left; 
 if (n>0) { 
 nn += n; 
 p = pp+left; 
 d = left - A; 
 if (0) r=r1+d*dxr, g=g1+d*dxg, b=b1+d*dxb; 
 if (0) o=o1+d*dxo, obar=1.0-o; 
 z=z1+d*dxz; 
 while (--n>=0) { 
 if ((z>p->z && z<p->front)) { 
 { p->c.r=o*r+obar*p->c.r; p->c.g=o*g+obar*p->c.g; p->c.b=o*b+obar*p->c.b; if (o > 0.5) p->z=z; }; 
 if (0) r+=dxr, g+=dxg, b+=dxb; 
 if (0) o+=dxo, obar=1.0-o; 
 z+=dxz; 
 p++; 
 } else { 
 if (0) r+=dxr, g+=dxg, b+=dxb; 
 if (0) o+=dxo, obar=1.0-o; 
 z+=dxz; 
 p++; 
 } 
 } 
 } 
 if (0) r1+=dyr, g1+=dyg, b1+=dyb; 
 if (0) o1+=dyo; 
 z1 += dyz; 
 A+=dyA, B+=dyB; 
 pp+=width; 
 } 
 } 
}
 buf->pixels += nn;
};
 CAT(tftc,1) :
 continue;
 }
}
                                    ;
    } else if (buf->pix_type==pix_fast) {
      {
 Point *p;
 int v1, v2, v3, i, width, height;
 int i1, i2, i3;
 float ox, oy;
 InvalidComponentHandle iElts;
 RGBColor vcolors[6];
 int shademe;
 char fcst = xf->fcst, bcst = xf->bcst, ncst = xf->ncst;
 char fbyte = xf->fbyte, bbyte = xf->bbyte;
 iElts = (invalid_status == INV_UNKNOWN) ? xf->iElts : NULL;
 width = buf->width, height = buf->height;
 ox = buf->ox, oy = buf->oy;
 if (0 && xf->lights) {
 fcolors = (Pointer)vcolors;
 bcolors = (Pointer)(vcolors+3);
 _dxfInitApplyLights(xf->kaf, xf->kdf, xf->ksf, xf->kspf,
 xf->kab, xf->kdb, xf->ksb, xf->kspb,
 xf->fcolors, xf->bcolors, xf->cmap,
 xf->normals, xf->lights,
 xf->colors_dep, xf->normals_dep,
 fcolors, bcolors, 3, fcst, bcst, ncst,
 fbyte, bbyte);
 cmap = NULL;
 shademe = 1;
 } else {
 shademe = 0;
 if (fbyte || bbyte)
 _dxf_initUbyteToFloat();
 }
 for (i=0; i<ntri; i++, tri++) {
 int index = (indices == NULL) ? i : indices[i];
 if (iElts && DXIsElementInvalid(iElts, index))
 continue;
 v1=tri->p, p= &xf->positions[v1], x1=p->x, y1=p->y, z1=p->z;
 v2=tri->q, p= &xf->positions[v2], x2=p->x, y2=p->y, z2=p->z;
 v3=tri->r, p= &xf->positions[v3], x3=p->x, y3=p->y, z3=p->z;
 if (shademe)
 {
 i1 = 0; i2 = 1; i3 = 2;
 if (! _dxfApplyLights((int *)tri, &index, 1))
 return ERROR;
 }
 if (xf->tile.perspective) {
 if (z1>nearPlane || z2>nearPlane || z3>nearPlane)
 continue;
 z1=(float)-1.0/z1; x1*=z1; y1*=z1;
 z2=(float)-1.0/z2; x2*=z2; y2*=z2;
 z3=(float)-1.0/z3; x3*=z3; y3*=z3;
 }
 if ((x1<minx && x2<minx && x3<minx) ||
 (x1>maxx && x2>maxx && x3>maxx) ||
 (y1<miny && y2<miny && y3<miny) ||
 (y1>maxy && y2>maxy && y3>maxy))
 continue;
 buf->triangles++;
 {
 struct fast *pp, *p;
 Pointer colors;
 int cstcolors;
 RGBColor cbuf1, cbuf2, cbuf3;
 char cbyte;
 char obyte = xf->obyte;
 x1+=ox, y1+=oy;
 x2+=ox, y2+=oy;
 x3+=ox, y3+=oy;
 if (x1<-(DXD_MAX_INT/2) || x1>(DXD_MAX_INT/2) || y1<-(DXD_MAX_INT/2) || y1>(DXD_MAX_INT/2) ||
 x2<-(DXD_MAX_INT/2) || x2>(DXD_MAX_INT/2) || y2<-(DXD_MAX_INT/2) || y2>(DXD_MAX_INT/2) ||
 x3<-(DXD_MAX_INT/2) || x3>(DXD_MAX_INT/2) || y3<-(DXD_MAX_INT/2) || y3>(DXD_MAX_INT/2))
 DXErrorReturn(ERROR_BAD_PARAMETER,
 "camera causes numerical overflow");
 d1 = y3-y2, d2 = y1-y3, d3 = y2-y1;
 d = x1*d1 + x2*d2 + x3*d3;
 Qx = d? (float)1.0 / (float)d : 0.0;
 d1 *= Qx, d2 *= Qx, d3 *= Qx;
 if (1) {
 if (d < 0)
 cbyte = fbyte, colors = fcolors, cstcolors = xf->fcst;
 else
 cbyte = bbyte, colors = bcolors, cstcolors = xf->bcst;
 if (!colors) goto CAT(tftf,1);
 }
 if (0) {
 if (shademe) {
 c1 = ((RGBColor *)colors) + i1;
 c2 = ((RGBColor *)colors) + i2;
 c3 = ((RGBColor *)colors) + i3;
 } else {
 if (cmap)
 c1= &(cmap[((unsigned char *)colors)[cstcolors ? 0 : v1]]), c2= &(cmap[((unsigned char *)colors)[cstcolors ? 0 : v2]]), c3= &(cmap[((unsigned char *)colors)[cstcolors ? 0 : v3]]);
 else {
 
{ 
 if (cbyte) 
 { 
 RGBByteColor *ptr=((RGBByteColor *)colors)+(cstcolors ? 0 : v1); 
 cbuf1.r = _dxd_ubyteToFloat[ptr->r]; 
 cbuf1.g = _dxd_ubyteToFloat[ptr->g]; 
 cbuf1.b = _dxd_ubyteToFloat[ptr->b]; 
 } else 
 cbuf1 = (((RGBColor *)colors)[cstcolors ? 0 : v1]); 
}; 
{ 
 if (cbyte) 
 { 
 RGBByteColor *ptr=((RGBByteColor *)colors)+(cstcolors ? 0 : v2); 
 cbuf2.r = _dxd_ubyteToFloat[ptr->r]; 
 cbuf2.g = _dxd_ubyteToFloat[ptr->g]; 
 cbuf2.b = _dxd_ubyteToFloat[ptr->b]; 
 } else 
 cbuf2 = (((RGBColor *)colors)[cstcolors ? 0 : v2]); 
}; 
{ 
 if (cbyte) 
 { 
 RGBByteColor *ptr=((RGBByteColor *)colors)+(cstcolors ? 0 : v3); 
 cbuf3.r = _dxd_ubyteToFloat[ptr->r]; 
 cbuf3.g = _dxd_ubyteToFloat[ptr->g]; 
 cbuf3.b = _dxd_ubyteToFloat[ptr->b]; 
 } else 
 cbuf3 = (((RGBColor *)colors)[cstcolors ? 0 : v3]); 
};
 c1 = &cbuf1; c2 = &cbuf2; c3 = &cbuf3;
 }
 }
 r1=c1->r, g1=c1->g, b1=c1->b;
 r2=c2->r, g2=c2->g, b2=c2->b;
 r3=c3->r, g3=c3->g, b3=c3->b;
 } else if (1) {
 if (cmap){
 r=(cmap[((unsigned char *)colors)[cstcolors ? 0 : index]]).r; g=(cmap[((unsigned char *)colors)[cstcolors ? 0 : index]]).g; b=(cmap[((unsigned char *)colors)[cstcolors ? 0 : index]]).b;
 } else {
 
{ 
 if (cbyte) 
 { 
 RGBByteColor *ptr=((RGBByteColor *)colors)+(cstcolors ? 0 : index); 
 cbuf1.r = _dxd_ubyteToFloat[ptr->r]; 
 cbuf1.g = _dxd_ubyteToFloat[ptr->g]; 
 cbuf1.b = _dxd_ubyteToFloat[ptr->b]; 
 } else 
 cbuf1 = (((RGBColor *)colors)[cstcolors ? 0 : index]); 
};
 r = cbuf1.r; g = cbuf1.g; b = cbuf1.b;
 }
 }
 if (0) {
 if (omap) o1=(omap[((unsigned char *)opacities)[xf->ocst ? 0 : v1]]), o2=(omap[((unsigned char *)opacities)[xf->ocst ? 0 : v2]]), o3=(omap[((unsigned char *)opacities)[xf->ocst ? 0 : v3]]);
 else {
 
{ 
 if (obyte) 
 o1 = _dxd_ubyteToFloat[((ubyte *)opacities)[xf->ocst ? 0 : v1]]; 
 else 
 o1 = (((float *)opacities)[xf->ocst ? 0 : v1]); 
}; 
{ 
 if (obyte) 
 o2 = _dxd_ubyteToFloat[((ubyte *)opacities)[xf->ocst ? 0 : v2]]; 
 else 
 o2 = (((float *)opacities)[xf->ocst ? 0 : v2]); 
}; 
{ 
 if (obyte) 
 o3 = _dxd_ubyteToFloat[((ubyte *)opacities)[xf->ocst ? 0 : v3]]; 
 else 
 o3 = (((float *)opacities)[xf->ocst ? 0 : v3]); 
};
 }
 } else if (opacities||omap) {
 if (omap) o=(omap[((unsigned char *)opacities)[xf->ocst ? 0 : index]]);
 else 
{ 
 if (obyte) 
 o = _dxd_ubyteToFloat[((ubyte *)opacities)[xf->ocst ? 0 : index]]; 
 else 
 o = (((float *)opacities)[xf->ocst ? 0 : index]); 
};
 obar = 1.0-o;
 } else
 o = 1.0, obar = 0.0;
 if (0) {
 dxr = (r1 == r2 && r1 == r3) ? 0.0 : d1*r1 + d2*r2 + d3*r3;
 dxg = (g1 == g2 && g1 == g3) ? 0.0 : d1*g1 + d2*g2 + d3*g3;
 dxb = (b1 == b2 && b1 == b3) ? 0.0 : d1*b1 + d2*b2 + d3*b3;
 }
 if (0) dxo = (o1 == o2 && o1 == o3) ? 0.0 : d1*o1 + d2*o2 + d3*o3;
 dxz = (z1 == z2 && z1 == z3) ? 0.0 : d1*z1 + d2*z2 + d3*z3;
 { 
 if (y2 < y1) { 
 (d=y1, y1=y2, y2=d), (d=x1, x1=x2, x2=d); 
 if (0) (d=r1, r1=r2, r2=d), (d=g1, g1=g2, g2=d), (d=b1, b1=b2, b2=d); 
 if (0) (d=o1, o1=o2, o2=d); 
 (d=z1, z1=z2, z2=d); 
 } 
}
 { 
 if (y3 < y1) { 
 (d=y1, y1=y3, y3=d), (d=x1, x1=x3, x3=d); 
 if (0) (d=r1, r1=r3, r3=d), (d=g1, g1=g3, g3=d), (d=b1, b1=b3, b3=d); 
 if (0) (d=o1, o1=o3, o3=d); 
 (d=z1, z1=z3, z3=d); 
 } 
}
 { 
 if (y3 < y2) { 
 (d=y2, y2=y3, y3=d), (d=x2, x2=x3, x3=d); 
 if (0) (d=r2, r2=r3, r3=d), (d=g2, g2=g3, g3=d), (d=b2, b2=b3, b3=d); 
 if (0) (d=o2, o2=o3, o3=d); 
 (d=z2, z2=z3, z3=d); 
 } 
}
 if (0) {
 A = B = x1;
 if (x2<A) A = x2; else B = x2;
 if (x3<A) A = x3; else if (x3>B) B = x3;
 if (A<buf->fmin.x) buf->fmin.x = A;
 if (B>buf->fmax.x) buf->fmax.x = B;
 if (y1<buf->fmin.y) buf->fmin.y = y1;
 if (y3>buf->fmax.y) buf->fmax.y = y3;
 }
 iy1 = ((int)((y1)-30000.0)+30000), iy2 = ((int)((y2)-30000.0)+30000), iy3 = ((int)((y3)-30000.0)+30000);
 if (iy1>iy2 || iy2>iy3) {
 DXSetError(ERROR_DATA_INVALID,
 "position number %d, %d, or %d is invalid", v1, v2, v3);
 return ERROR;
 }
 if (iy1==iy3)
 goto CAT(tftf,1);
 d = y3 - y1;
 Qy = d? (float)1.0 / (float)d : 0.0;
 if (0) dyr=(r3-r1)*Qy, dyg=(g3-g1)*Qy, dyb=(b3-b1)*Qy;
 if (0) dyo = (float)(o3-o1) * Qy;
 dyz = (z3-z1) * Qy;
 dyA = (float)(x3-x1) * Qy;
 A = x1;
 pp = buf->u.fast + iy1*width;
 d = iy1 - y1;
 if (iy1<0)
 d-=iy1, pp=buf->u.fast;
 A += dyA*d;
 if (0) r1+=dyr*d, g1+=dyg*d, b1+=dyb*d;
 if (0) o1+=dyo*d;
 z1 += dyz*d;
 nn = 0;
 { 
 
 if (iy2>height) 
 iy2 = height; 
 if (iy1<iy2 && iy2>0 && iy1<=height) { 
 B = x1; 
 d = y2 - y1; 
 d = d? (float)1.0 / (float)d : 0.0; 
 dyB = (float)(x2-x1) * d; 
 if (iy1<0) iy1=0; 
 d = iy1 - y1; 
 B += d*dyB; 
 for (iy=iy1; iy<iy2; iy++) { 
 iA = ((int)((A)-30000.0)+30000); 
 iB = ((int)((B)-30000.0)+30000); 
 if (iB>iA) left=iA, right=iB; 
 else left=iB, right=iA; 
 if (left<0) left=0; 
 if (right>width) right=width; 
 n = right - left; 
 if (n>0) { 
 nn += n; 
 p = pp+left; 
 d = left - A; 
 if (0) r=r1+d*dxr, g=g1+d*dxg, b=b1+d*dxb; 
 if (0) o=o1+d*dxo, obar=1.0-o; 
 z=z1+d*dxz; 
 while (--n>=0) { 
 if ((z>p->z)) { 
 { p->c.r=o*r+obar*p->c.r; p->c.g=o*g+obar*p->c.g; p->c.b=o*b+obar*p->c.b; if (o > 0.5) p->z=z; }; 
 if (0) r+=dxr, g+=dxg, b+=dxb; 
 if (0) o+=dxo, obar=1.0-o; 
 z+=dxz; 
 p++; 
 } else { 
 if (0) r+=dxr, g+=dxg, b+=dxb; 
 if (0) o+=dxo, obar=1.0-o; 
 z+=dxz; 
 p++; 
 } 
 } 
 } 
 if (0) r1+=dyr, g1+=dyg, b1+=dyb; 
 if (0) o1+=dyo; 
 z1 += dyz; 
 A+=dyA, B+=dyB; 
 pp+=width; 
 } 
 } 
}
 { 
 
 if (iy3>height) 
 iy3 = height; 
 if (iy2<iy3 && iy3>0 && iy2<=height) { 
 B = x2; 
 d = y3 - y2; 
 d = d? (float)1.0 / (float)d : 0.0; 
 dyB = (float)(x3-x2) * d; 
 if (iy2<0) iy2=0; 
 d = iy2 - y2; 
 B += d*dyB; 
 for (iy=iy2; iy<iy3; iy++) { 
 iA = ((int)((A)-30000.0)+30000); 
 iB = ((int)((B)-30000.0)+30000); 
 if (iB>iA) left=iA, right=iB; 
 else left=iB, right=iA; 
 if (left<0) left=0; 
 if (right>width) right=width; 
 n = right - left; 
 if (n>0) { 
 nn += n; 
 p = pp+left; 
 d = left - A; 
 if (0) r=r1+d*dxr, g=g1+d*dxg, b=b1+d*dxb; 
 if (0) o=o1+d*dxo, obar=1.0-o; 
 z=z1+d*dxz; 
 while (--n>=0) { 
 if ((z>p->z)) { 
 { p->c.r=o*r+obar*p->c.r; p->c.g=o*g+obar*p->c.g; p->c.b=o*b+obar*p->c.b; if (o > 0.5) p->z=z; }; 
 if (0) r+=dxr, g+=dxg, b+=dxb; 
 if (0) o+=dxo, obar=1.0-o; 
 z+=dxz; 
 p++; 
 } else { 
 if (0) r+=dxr, g+=dxg, b+=dxb; 
 if (0) o+=dxo, obar=1.0-o; 
 z+=dxz; 
 p++; 
 } 
 } 
 } 
 if (0) r1+=dyr, g1+=dyg, b1+=dyb; 
 if (0) o1+=dyo; 
 z1 += dyz; 
 A+=dyA, B+=dyB; 
 pp+=width; 
 } 
 } 
}
 buf->pixels += nn;
};
 CAT(tftf,1) :
 continue;
 }
}
                                 ;
    } else if (buf->pix_type==pix_big) {
      {
 Point *p;
 int v1, v2, v3, i, width, height;
 int i1, i2, i3;
 float ox, oy;
 InvalidComponentHandle iElts;
 RGBColor vcolors[6];
 int shademe;
 char fcst = xf->fcst, bcst = xf->bcst, ncst = xf->ncst;
 char fbyte = xf->fbyte, bbyte = xf->bbyte;
 iElts = (invalid_status == INV_UNKNOWN) ? xf->iElts : NULL;
 width = buf->width, height = buf->height;
 ox = buf->ox, oy = buf->oy;
 if (0 && xf->lights) {
 fcolors = (Pointer)vcolors;
 bcolors = (Pointer)(vcolors+3);
 _dxfInitApplyLights(xf->kaf, xf->kdf, xf->ksf, xf->kspf,
 xf->kab, xf->kdb, xf->ksb, xf->kspb,
 xf->fcolors, xf->bcolors, xf->cmap,
 xf->normals, xf->lights,
 xf->colors_dep, xf->normals_dep,
 fcolors, bcolors, 3, fcst, bcst, ncst,
 fbyte, bbyte);
 cmap = NULL;
 shademe = 1;
 } else {
 shademe = 0;
 if (fbyte || bbyte)
 _dxf_initUbyteToFloat();
 }
 for (i=0; i<ntri; i++, tri++) {
 int index = (indices == NULL) ? i : indices[i];
 if (iElts && DXIsElementInvalid(iElts, index))
 continue;
 v1=tri->p, p= &xf->positions[v1], x1=p->x, y1=p->y, z1=p->z;
 v2=tri->q, p= &xf->positions[v2], x2=p->x, y2=p->y, z2=p->z;
 v3=tri->r, p= &xf->positions[v3], x3=p->x, y3=p->y, z3=p->z;
 if (shademe)
 {
 i1 = 0; i2 = 1; i3 = 2;
 if (! _dxfApplyLights((int *)tri, &index, 1))
 return ERROR;
 }
 if (xf->tile.perspective) {
 if (z1>nearPlane || z2>nearPlane || z3>nearPlane)
 continue;
 z1=(float)-1.0/z1; x1*=z1; y1*=z1;
 z2=(float)-1.0/z2; x2*=z2; y2*=z2;
 z3=(float)-1.0/z3; x3*=z3; y3*=z3;
 }
 if ((x1<minx && x2<minx && x3<minx) ||
 (x1>maxx && x2>maxx && x3>maxx) ||
 (y1<miny && y2<miny && y3<miny) ||
 (y1>maxy && y2>maxy && y3>maxy))
 continue;
 buf->triangles++;
 {
 struct big *pp, *p;
 Pointer colors;
 int cstcolors;
 RGBColor cbuf1, cbuf2, cbuf3;
 char cbyte;
 char obyte = xf->obyte;
 x1+=ox, y1+=oy;
 x2+=ox, y2+=oy;
 x3+=ox, y3+=oy;
 if (x1<-(DXD_MAX_INT/2) || x1>(DXD_MAX_INT/2) || y1<-(DXD_MAX_INT/2) || y1>(DXD_MAX_INT/2) ||
 x2<-(DXD_MAX_INT/2) || x2>(DXD_MAX_INT/2) || y2<-(DXD_MAX_INT/2) || y2>(DXD_MAX_INT/2) ||
 x3<-(DXD_MAX_INT/2) || x3>(DXD_MAX_INT/2) || y3<-(DXD_MAX_INT/2) || y3>(DXD_MAX_INT/2))
 DXErrorReturn(ERROR_BAD_PARAMETER,
 "camera causes numerical overflow");
 d1 = y3-y2, d2 = y1-y3, d3 = y2-y1;
 d = x1*d1 + x2*d2 + x3*d3;
 Qx = d? (float)1.0 / (float)d : 0.0;
 d1 *= Qx, d2 *= Qx, d3 *= Qx;
 if (1) {
 if (d < 0)
 cbyte = fbyte, colors = fcolors, cstcolors = xf->fcst;
 else
 cbyte = bbyte, colors = bcolors, cstcolors = xf->bcst;
 if (!colors) goto CAT(tftb,1);
 }
 if (0) {
 if (shademe) {
 c1 = ((RGBColor *)colors) + i1;
 c2 = ((RGBColor *)colors) + i2;
 c3 = ((RGBColor *)colors) + i3;
 } else {
 if (cmap)
 c1= &(cmap[((unsigned char *)colors)[cstcolors ? 0 : v1]]), c2= &(cmap[((unsigned char *)colors)[cstcolors ? 0 : v2]]), c3= &(cmap[((unsigned char *)colors)[cstcolors ? 0 : v3]]);
 else {
 
{ 
 if (cbyte) 
 { 
 RGBByteColor *ptr=((RGBByteColor *)colors)+(cstcolors ? 0 : v1); 
 cbuf1.r = _dxd_ubyteToFloat[ptr->r]; 
 cbuf1.g = _dxd_ubyteToFloat[ptr->g]; 
 cbuf1.b = _dxd_ubyteToFloat[ptr->b]; 
 } else 
 cbuf1 = (((RGBColor *)colors)[cstcolors ? 0 : v1]); 
}; 
{ 
 if (cbyte) 
 { 
 RGBByteColor *ptr=((RGBByteColor *)colors)+(cstcolors ? 0 : v2); 
 cbuf2.r = _dxd_ubyteToFloat[ptr->r]; 
 cbuf2.g = _dxd_ubyteToFloat[ptr->g]; 
 cbuf2.b = _dxd_ubyteToFloat[ptr->b]; 
 } else 
 cbuf2 = (((RGBColor *)colors)[cstcolors ? 0 : v2]); 
}; 
{ 
 if (cbyte) 
 { 
 RGBByteColor *ptr=((RGBByteColor *)colors)+(cstcolors ? 0 : v3); 
 cbuf3.r = _dxd_ubyteToFloat[ptr->r]; 
 cbuf3.g = _dxd_ubyteToFloat[ptr->g]; 
 cbuf3.b = _dxd_ubyteToFloat[ptr->b]; 
 } else 
 cbuf3 = (((RGBColor *)colors)[cstcolors ? 0 : v3]); 
};
 c1 = &cbuf1; c2 = &cbuf2; c3 = &cbuf3;
 }
 }
 r1=c1->r, g1=c1->g, b1=c1->b;
 r2=c2->r, g2=c2->g, b2=c2->b;
 r3=c3->r, g3=c3->g, b3=c3->b;
 } else if (1) {
 if (cmap){
 r=(cmap[((unsigned char *)colors)[cstcolors ? 0 : index]]).r; g=(cmap[((unsigned char *)colors)[cstcolors ? 0 : index]]).g; b=(cmap[((unsigned char *)colors)[cstcolors ? 0 : index]]).b;
 } else {
 
{ 
 if (cbyte) 
 { 
 RGBByteColor *ptr=((RGBByteColor *)colors)+(cstcolors ? 0 : index); 
 cbuf1.r = _dxd_ubyteToFloat[ptr->r]; 
 cbuf1.g = _dxd_ubyteToFloat[ptr->g]; 
 cbuf1.b = _dxd_ubyteToFloat[ptr->b]; 
 } else 
 cbuf1 = (((RGBColor *)colors)[cstcolors ? 0 : index]); 
};
 r = cbuf1.r; g = cbuf1.g; b = cbuf1.b;
 }
 }
 if (0) {
 if (omap) o1=(omap[((unsigned char *)opacities)[xf->ocst ? 0 : v1]]), o2=(omap[((unsigned char *)opacities)[xf->ocst ? 0 : v2]]), o3=(omap[((unsigned char *)opacities)[xf->ocst ? 0 : v3]]);
 else {
 
{ 
 if (obyte) 
 o1 = _dxd_ubyteToFloat[((ubyte *)opacities)[xf->ocst ? 0 : v1]]; 
 else 
 o1 = (((float *)opacities)[xf->ocst ? 0 : v1]); 
}; 
{ 
 if (obyte) 
 o2 = _dxd_ubyteToFloat[((ubyte *)opacities)[xf->ocst ? 0 : v2]]; 
 else 
 o2 = (((float *)opacities)[xf->ocst ? 0 : v2]); 
}; 
{ 
 if (obyte) 
 o3 = _dxd_ubyteToFloat[((ubyte *)opacities)[xf->ocst ? 0 : v3]]; 
 else 
 o3 = (((float *)opacities)[xf->ocst ? 0 : v3]); 
};
 }
 } else if (opacities||omap) {
 if (omap) o=(omap[((unsigned char *)opacities)[xf->ocst ? 0 : index]]);
 else 
{ 
 if (obyte) 
 o = _dxd_ubyteToFloat[((ubyte *)opacities)[xf->ocst ? 0 : index]]; 
 else 
 o = (((float *)opacities)[xf->ocst ? 0 : index]); 
};
 obar = 1.0-o;
 } else
 o = 1.0, obar = 0.0;
 if (0) {
 dxr = (r1 == r2 && r1 == r3) ? 0.0 : d1*r1 + d2*r2 + d3*r3;
 dxg = (g1 == g2 && g1 == g3) ? 0.0 : d1*g1 + d2*g2 + d3*g3;
 dxb = (b1 == b2 && b1 == b3) ? 0.0 : d1*b1 + d2*b2 + d3*b3;
 }
 if (0) dxo = (o1 == o2 && o1 == o3) ? 0.0 : d1*o1 + d2*o2 + d3*o3;
 dxz = (z1 == z2 && z1 == z3) ? 0.0 : d1*z1 + d2*z2 + d3*z3;
 { 
 if (y2 < y1) { 
 (d=y1, y1=y2, y2=d), (d=x1, x1=x2, x2=d); 
 if (0) (d=r1, r1=r2, r2=d), (d=g1, g1=g2, g2=d), (d=b1, b1=b2, b2=d); 
 if (0) (d=o1, o1=o2, o2=d); 
 (d=z1, z1=z2, z2=d); 
 } 
}
 { 
 if (y3 < y1) { 
 (d=y1, y1=y3, y3=d), (d=x1, x1=x3, x3=d); 
 if (0) (d=r1, r1=r3, r3=d), (d=g1, g1=g3, g3=d), (d=b1, b1=b3, b3=d); 
 if (0) (d=o1, o1=o3, o3=d); 
 (d=z1, z1=z3, z3=d); 
 } 
}
 { 
 if (y3 < y2) { 
 (d=y2, y2=y3, y3=d), (d=x2, x2=x3, x3=d); 
 if (0) (d=r2, r2=r3, r3=d), (d=g2, g2=g3, g3=d), (d=b2, b2=b3, b3=d); 
 if (0) (d=o2, o2=o3, o3=d); 
 (d=z2, z2=z3, z3=d); 
 } 
}
 if (0) {
 A = B = x1;
 if (x2<A) A = x2; else B = x2;
 if (x3<A) A = x3; else if (x3>B) B = x3;
 if (A<buf->fmin.x) buf->fmin.x = A;
 if (B>buf->fmax.x) buf->fmax.x = B;
 if (y1<buf->fmin.y) buf->fmin.y = y1;
 if (y3>buf->fmax.y) buf->fmax.y = y3;
 }
 iy1 = ((int)((y1)-30000.0)+30000), iy2 = ((int)((y2)-30000.0)+30000), iy3 = ((int)((y3)-30000.0)+30000);
 if (iy1>iy2 || iy2>iy3) {
 DXSetError(ERROR_DATA_INVALID,
 "position number %d, %d, or %d is invalid", v1, v2, v3);
 return ERROR;
 }
 if (iy1==iy3)
 goto CAT(tftb,1);
 d = y3 - y1;
 Qy = d? (float)1.0 / (float)d : 0.0;
 if (0) dyr=(r3-r1)*Qy, dyg=(g3-g1)*Qy, dyb=(b3-b1)*Qy;
 if (0) dyo = (float)(o3-o1) * Qy;
 dyz = (z3-z1) * Qy;
 dyA = (float)(x3-x1) * Qy;
 A = x1;
 pp = buf->u.big + iy1*width;
 d = iy1 - y1;
 if (iy1<0)
 d-=iy1, pp=buf->u.big;
 A += dyA*d;
 if (0) r1+=dyr*d, g1+=dyg*d, b1+=dyb*d;
 if (0) o1+=dyo*d;
 z1 += dyz*d;
 nn = 0;
 { 
 
 if (iy2>height) 
 iy2 = height; 
 if (iy1<iy2 && iy2>0 && iy1<=height) { 
 B = x1; 
 d = y2 - y1; 
 d = d? (float)1.0 / (float)d : 0.0; 
 dyB = (float)(x2-x1) * d; 
 if (iy1<0) iy1=0; 
 d = iy1 - y1; 
 B += d*dyB; 
 for (iy=iy1; iy<iy2; iy++) { 
 iA = ((int)((A)-30000.0)+30000); 
 iB = ((int)((B)-30000.0)+30000); 
 if (iB>iA) left=iA, right=iB; 
 else left=iB, right=iA; 
 if (left<0) left=0; 
 if (right>width) right=width; 
 n = right - left; 
 if (n>0) { 
 nn += n; 
 p = pp+left; 
 d = left - A; 
 if (0) r=r1+d*dxr, g=g1+d*dxg, b=b1+d*dxb; 
 if (0) o=o1+d*dxo, obar=1.0-o; 
 z=z1+d*dxz; 
 while (--n>=0) { 
 if ((z>p->z)) { 
 { p->c.r=o*r+obar*p->c.r; p->c.g=o*g+obar*p->c.g; p->c.b=o*b+obar*p->c.b; if (o > 0.5) p->z=z; }; 
 if (0) r+=dxr, g+=dxg, b+=dxb; 
 if (0) o+=dxo, obar=1.0-o; 
 z+=dxz; 
 p++; 
 } else { 
 if (0) r+=dxr, g+=dxg, b+=dxb; 
 if (0) o+=dxo, obar=1.0-o; 
 z+=dxz; 
 p++; 
 } 
 } 
 } 
 if (0) r1+=dyr, g1+=dyg, b1+=dyb; 
 if (0) o1+=dyo; 
 z1 += dyz; 
 A+=dyA, B+=dyB; 
 pp+=width; 
 } 
 } 
}
 { 
 
 if (iy3>height) 
 iy3 = height; 
 if (iy2<iy3 && iy3>0 && iy2<=height) { 
 B = x2; 
 d = y3 - y2; 
 d = d? (float)1.0 / (float)d : 0.0; 
 dyB = (float)(x3-x2) * d; 
 if (iy2<0) iy2=0; 
 d = iy2 - y2; 
 B += d*dyB; 
 for (iy=iy2; iy<iy3; iy++) { 
 iA = ((int)((A)-30000.0)+30000); 
 iB = ((int)((B)-30000.0)+30000); 
 if (iB>iA) left=iA, right=iB; 
 else left=iB, right=iA; 
 if (left<0) left=0; 
 if (right>width) right=width; 
 n = right - left; 
 if (n>0) { 
 nn += n; 
 p = pp+left; 
 d = left - A; 
 if (0) r=r1+d*dxr, g=g1+d*dxg, b=b1+d*dxb; 
 if (0) o=o1+d*dxo, obar=1.0-o; 
 z=z1+d*dxz; 
 while (--n>=0) { 
 if ((z>p->z)) { 
 { p->c.r=o*r+obar*p->c.r; p->c.g=o*g+obar*p->c.g; p->c.b=o*b+obar*p->c.b; if (o > 0.5) p->z=z; }; 
 if (0) r+=dxr, g+=dxg, b+=dxb; 
 if (0) o+=dxo, obar=1.0-o; 
 z+=dxz; 
 p++; 
 } else { 
 if (0) r+=dxr, g+=dxg, b+=dxb; 
 if (0) o+=dxo, obar=1.0-o; 
 z+=dxz; 
 p++; 
 } 
 } 
 } 
 if (0) r1+=dyr, g1+=dyg, b1+=dyb; 
 if (0) o1+=dyo; 
 z1 += dyz; 
 A+=dyA, B+=dyB; 
 pp+=width; 
 } 
 } 
}
 buf->pixels += nn;
};
 CAT(tftb,1) :
 continue;
 }
}
                                ;
    } else {
      DXSetError(ERROR_INTERNAL, "unknown pix_type %d", buf->pix_type);
      return ERROR;
    }
 return OK;
 }
 static Error
 tri_flat_opaque(
 struct buffer *buf,
 struct xfield *xf,
 int ntri,
 Triangle *tri,
 int *indices,
 Pointer fcolors,
 Pointer bcolors,
 Pointer opacities,
 int surface,
 int clip_status,
 inv_stat invalid_status
 ) {
 RGBColor *c1, *c2, *c3;
 float x1, y1, x2, y2, x3, y3;
 float r1, g1, b1, o1, z1, r2, g2, b2, o2, z2, r3, g3, b3, o3, z3;
 float Qx, dxo, dxz, dxr, dxg, dxb;
 float Qy, dyr, dyg, dyb, dyo, dyz, dyA, dyB;
 float r, g, b, o, obar, z, nearPlane=xf->nearPlane ;
 float A, B, d, d1, d2, d3;
 int iy1, iy2, iy3;
 int iA, iB, iy, i, n, nn, left, right;
 float minx = buf->min.x, miny = buf->min.y;
 float maxx = buf->max.x, maxy = buf->max.y;
 RGBColor *cmap = xf->cmap;
 float *omap = xf->omap;
    if (clip_status) {
      ASSERT(buf->pix_type==pix_big);
      {
 Point *p;
 int v1, v2, v3, i, width, height;
 int i1, i2, i3;
 float ox, oy;
 InvalidComponentHandle iElts;
 RGBColor vcolors[6];
 int shademe;
 char fcst = xf->fcst, bcst = xf->bcst, ncst = xf->ncst;
 char fbyte = xf->fbyte, bbyte = xf->bbyte;
 iElts = (invalid_status == INV_UNKNOWN) ? xf->iElts : NULL;
 width = buf->width, height = buf->height;
 ox = buf->ox, oy = buf->oy;
 if (0 && xf->lights) {
 fcolors = (Pointer)vcolors;
 bcolors = (Pointer)(vcolors+3);
 _dxfInitApplyLights(xf->kaf, xf->kdf, xf->ksf, xf->kspf,
 xf->kab, xf->kdb, xf->ksb, xf->kspb,
 xf->fcolors, xf->bcolors, xf->cmap,
 xf->normals, xf->lights,
 xf->colors_dep, xf->normals_dep,
 fcolors, bcolors, 3, fcst, bcst, ncst,
 fbyte, bbyte);
 cmap = NULL;
 shademe = 1;
 } else {
 shademe = 0;
 if (fbyte || bbyte)
 _dxf_initUbyteToFloat();
 }
 for (i=0; i<ntri; i++, tri++) {
 int index = (indices == NULL) ? i : indices[i];
 if (iElts && DXIsElementInvalid(iElts, index))
 continue;
 v1=tri->p, p= &xf->positions[v1], x1=p->x, y1=p->y, z1=p->z;
 v2=tri->q, p= &xf->positions[v2], x2=p->x, y2=p->y, z2=p->z;
 v3=tri->r, p= &xf->positions[v3], x3=p->x, y3=p->y, z3=p->z;
 if (shademe)
 {
 i1 = 0; i2 = 1; i3 = 2;
 if (! _dxfApplyLights((int *)tri, &index, 1))
 return ERROR;
 }
 if (xf->tile.perspective) {
 if (z1>nearPlane || z2>nearPlane || z3>nearPlane)
 continue;
 z1=(float)-1.0/z1; x1*=z1; y1*=z1;
 z2=(float)-1.0/z2; x2*=z2; y2*=z2;
 z3=(float)-1.0/z3; x3*=z3; y3*=z3;
 }
 if ((x1<minx && x2<minx && x3<minx) ||
 (x1>maxx && x2>maxx && x3>maxx) ||
 (y1<miny && y2<miny && y3<miny) ||
 (y1>maxy && y2>maxy && y3>maxy))
 continue;
 buf->triangles++;
 {
 struct big *pp, *p;
 Pointer colors;
 int cstcolors;
 RGBColor cbuf1, cbuf2, cbuf3;
 char cbyte;
 char obyte = xf->obyte;
 x1+=ox, y1+=oy;
 x2+=ox, y2+=oy;
 x3+=ox, y3+=oy;
 if (x1<-(DXD_MAX_INT/2) || x1>(DXD_MAX_INT/2) || y1<-(DXD_MAX_INT/2) || y1>(DXD_MAX_INT/2) ||
 x2<-(DXD_MAX_INT/2) || x2>(DXD_MAX_INT/2) || y2<-(DXD_MAX_INT/2) || y2>(DXD_MAX_INT/2) ||
 x3<-(DXD_MAX_INT/2) || x3>(DXD_MAX_INT/2) || y3<-(DXD_MAX_INT/2) || y3>(DXD_MAX_INT/2))
 DXErrorReturn(ERROR_BAD_PARAMETER,
 "camera causes numerical overflow");
 d1 = y3-y2, d2 = y1-y3, d3 = y2-y1;
 d = x1*d1 + x2*d2 + x3*d3;
 Qx = d? (float)1.0 / (float)d : 0.0;
 d1 *= Qx, d2 *= Qx, d3 *= Qx;
 if (1) {
 if (d < 0)
 cbyte = fbyte, colors = fcolors, cstcolors = xf->fcst;
 else
 cbyte = bbyte, colors = bcolors, cstcolors = xf->bcst;
 if (!colors) goto CAT(tfoc,1);
 }
 if (0) {
 if (shademe) {
 c1 = ((RGBColor *)colors) + i1;
 c2 = ((RGBColor *)colors) + i2;
 c3 = ((RGBColor *)colors) + i3;
 } else {
 if (cmap)
 c1= &(cmap[((unsigned char *)colors)[cstcolors ? 0 : v1]]), c2= &(cmap[((unsigned char *)colors)[cstcolors ? 0 : v2]]), c3= &(cmap[((unsigned char *)colors)[cstcolors ? 0 : v3]]);
 else {
 
{ 
 if (cbyte) 
 { 
 RGBByteColor *ptr=((RGBByteColor *)colors)+(cstcolors ? 0 : v1); 
 cbuf1.r = _dxd_ubyteToFloat[ptr->r]; 
 cbuf1.g = _dxd_ubyteToFloat[ptr->g]; 
 cbuf1.b = _dxd_ubyteToFloat[ptr->b]; 
 } else 
 cbuf1 = (((RGBColor *)colors)[cstcolors ? 0 : v1]); 
}; 
{ 
 if (cbyte) 
 { 
 RGBByteColor *ptr=((RGBByteColor *)colors)+(cstcolors ? 0 : v2); 
 cbuf2.r = _dxd_ubyteToFloat[ptr->r]; 
 cbuf2.g = _dxd_ubyteToFloat[ptr->g]; 
 cbuf2.b = _dxd_ubyteToFloat[ptr->b]; 
 } else 
 cbuf2 = (((RGBColor *)colors)[cstcolors ? 0 : v2]); 
}; 
{ 
 if (cbyte) 
 { 
 RGBByteColor *ptr=((RGBByteColor *)colors)+(cstcolors ? 0 : v3); 
 cbuf3.r = _dxd_ubyteToFloat[ptr->r]; 
 cbuf3.g = _dxd_ubyteToFloat[ptr->g]; 
 cbuf3.b = _dxd_ubyteToFloat[ptr->b]; 
 } else 
 cbuf3 = (((RGBColor *)colors)[cstcolors ? 0 : v3]); 
};
 c1 = &cbuf1; c2 = &cbuf2; c3 = &cbuf3;
 }
 }
 r1=c1->r, g1=c1->g, b1=c1->b;
 r2=c2->r, g2=c2->g, b2=c2->b;
 r3=c3->r, g3=c3->g, b3=c3->b;
 } else if (1) {
 if (cmap){
 r=(cmap[((unsigned char *)colors)[cstcolors ? 0 : index]]).r; g=(cmap[((unsigned char *)colors)[cstcolors ? 0 : index]]).g; b=(cmap[((unsigned char *)colors)[cstcolors ? 0 : index]]).b;
 } else {
 
{ 
 if (cbyte) 
 { 
 RGBByteColor *ptr=((RGBByteColor *)colors)+(cstcolors ? 0 : index); 
 cbuf1.r = _dxd_ubyteToFloat[ptr->r]; 
 cbuf1.g = _dxd_ubyteToFloat[ptr->g]; 
 cbuf1.b = _dxd_ubyteToFloat[ptr->b]; 
 } else 
 cbuf1 = (((RGBColor *)colors)[cstcolors ? 0 : index]); 
};
 r = cbuf1.r; g = cbuf1.g; b = cbuf1.b;
 }
 }
 if (0) {
 if (omap) o1=(omap[((unsigned char *)opacities)[xf->ocst ? 0 : v1]]), o2=(omap[((unsigned char *)opacities)[xf->ocst ? 0 : v2]]), o3=(omap[((unsigned char *)opacities)[xf->ocst ? 0 : v3]]);
 else {
 
{ 
 if (obyte) 
 o1 = _dxd_ubyteToFloat[((ubyte *)opacities)[xf->ocst ? 0 : v1]]; 
 else 
 o1 = (((float *)opacities)[xf->ocst ? 0 : v1]); 
}; 
{ 
 if (obyte) 
 o2 = _dxd_ubyteToFloat[((ubyte *)opacities)[xf->ocst ? 0 : v2]]; 
 else 
 o2 = (((float *)opacities)[xf->ocst ? 0 : v2]); 
}; 
{ 
 if (obyte) 
 o3 = _dxd_ubyteToFloat[((ubyte *)opacities)[xf->ocst ? 0 : v3]]; 
 else 
 o3 = (((float *)opacities)[xf->ocst ? 0 : v3]); 
};
 }
 } else if (opacities||omap) {
 if (omap) o=(omap[((unsigned char *)opacities)[xf->ocst ? 0 : index]]);
 else 
{ 
 if (obyte) 
 o = _dxd_ubyteToFloat[((ubyte *)opacities)[xf->ocst ? 0 : index]]; 
 else 
 o = (((float *)opacities)[xf->ocst ? 0 : index]); 
};
 obar = 1.0-o;
 } else
 o = 1.0, obar = 0.0;
 if (0) {
 dxr = (r1 == r2 && r1 == r3) ? 0.0 : d1*r1 + d2*r2 + d3*r3;
 dxg = (g1 == g2 && g1 == g3) ? 0.0 : d1*g1 + d2*g2 + d3*g3;
 dxb = (b1 == b2 && b1 == b3) ? 0.0 : d1*b1 + d2*b2 + d3*b3;
 }
 if (0) dxo = (o1 == o2 && o1 == o3) ? 0.0 : d1*o1 + d2*o2 + d3*o3;
 dxz = (z1 == z2 && z1 == z3) ? 0.0 : d1*z1 + d2*z2 + d3*z3;
 { 
 if (y2 < y1) { 
 (d=y1, y1=y2, y2=d), (d=x1, x1=x2, x2=d); 
 if (0) (d=r1, r1=r2, r2=d), (d=g1, g1=g2, g2=d), (d=b1, b1=b2, b2=d); 
 if (0) (d=o1, o1=o2, o2=d); 
 (d=z1, z1=z2, z2=d); 
 } 
}
 { 
 if (y3 < y1) { 
 (d=y1, y1=y3, y3=d), (d=x1, x1=x3, x3=d); 
 if (0) (d=r1, r1=r3, r3=d), (d=g1, g1=g3, g3=d), (d=b1, b1=b3, b3=d); 
 if (0) (d=o1, o1=o3, o3=d); 
 (d=z1, z1=z3, z3=d); 
 } 
}
 { 
 if (y3 < y2) { 
 (d=y2, y2=y3, y3=d), (d=x2, x2=x3, x3=d); 
 if (0) (d=r2, r2=r3, r3=d), (d=g2, g2=g3, g3=d), (d=b2, b2=b3, b3=d); 
 if (0) (d=o2, o2=o3, o3=d); 
 (d=z2, z2=z3, z3=d); 
 } 
}
 if (0) {
 A = B = x1;
 if (x2<A) A = x2; else B = x2;
 if (x3<A) A = x3; else if (x3>B) B = x3;
 if (A<buf->fmin.x) buf->fmin.x = A;
 if (B>buf->fmax.x) buf->fmax.x = B;
 if (y1<buf->fmin.y) buf->fmin.y = y1;
 if (y3>buf->fmax.y) buf->fmax.y = y3;
 }
 iy1 = ((int)((y1)-30000.0)+30000), iy2 = ((int)((y2)-30000.0)+30000), iy3 = ((int)((y3)-30000.0)+30000);
 if (iy1>iy2 || iy2>iy3) {
 DXSetError(ERROR_DATA_INVALID,
 "position number %d, %d, or %d is invalid", v1, v2, v3);
 return ERROR;
 }
 if (iy1==iy3)
 goto CAT(tfoc,1);
 d = y3 - y1;
 Qy = d? (float)1.0 / (float)d : 0.0;
 if (0) dyr=(r3-r1)*Qy, dyg=(g3-g1)*Qy, dyb=(b3-b1)*Qy;
 if (0) dyo = (float)(o3-o1) * Qy;
 dyz = (z3-z1) * Qy;
 dyA = (float)(x3-x1) * Qy;
 A = x1;
 pp = buf->u.big + iy1*width;
 d = iy1 - y1;
 if (iy1<0)
 d-=iy1, pp=buf->u.big;
 A += dyA*d;
 if (0) r1+=dyr*d, g1+=dyg*d, b1+=dyb*d;
 if (0) o1+=dyo*d;
 z1 += dyz*d;
 nn = 0;
 { 
 
 if (iy2>height) 
 iy2 = height; 
 if (iy1<iy2 && iy2>0 && iy1<=height) { 
 B = x1; 
 d = y2 - y1; 
 d = d? (float)1.0 / (float)d : 0.0; 
 dyB = (float)(x2-x1) * d; 
 if (iy1<0) iy1=0; 
 d = iy1 - y1; 
 B += d*dyB; 
 for (iy=iy1; iy<iy2; iy++) { 
 iA = ((int)((A)-30000.0)+30000); 
 iB = ((int)((B)-30000.0)+30000); 
 if (iB>iA) left=iA, right=iB; 
 else left=iB, right=iA; 
 if (left<0) left=0; 
 if (right>width) right=width; 
 n = right - left; 
 if (n>0) { 
 nn += n; 
 p = pp+left; 
 d = left - A; 
 if (0) r=r1+d*dxr, g=g1+d*dxg, b=b1+d*dxb; 
 if (0) o=o1+d*dxo, obar=1.0-o; 
 z=z1+d*dxz; 
 while (--n>=0) { 
 if ((z>p->z && z<p->front && z>p->back)) { 
 { p->c.r=r; p->c.g=g; p->c.b=b; p->z=z; }; 
 if (0) r+=dxr, g+=dxg, b+=dxb; 
 if (0) o+=dxo, obar=1.0-o; 
 z+=dxz; 
 p++; 
 } else { 
 if (0) r+=dxr, g+=dxg, b+=dxb; 
 if (0) o+=dxo, obar=1.0-o; 
 z+=dxz; 
 p++; 
 } 
 } 
 } 
 if (0) r1+=dyr, g1+=dyg, b1+=dyb; 
 if (0) o1+=dyo; 
 z1 += dyz; 
 A+=dyA, B+=dyB; 
 pp+=width; 
 } 
 } 
}
 { 
 
 if (iy3>height) 
 iy3 = height; 
 if (iy2<iy3 && iy3>0 && iy2<=height) { 
 B = x2; 
 d = y3 - y2; 
 d = d? (float)1.0 / (float)d : 0.0; 
 dyB = (float)(x3-x2) * d; 
 if (iy2<0) iy2=0; 
 d = iy2 - y2; 
 B += d*dyB; 
 for (iy=iy2; iy<iy3; iy++) { 
 iA = ((int)((A)-30000.0)+30000); 
 iB = ((int)((B)-30000.0)+30000); 
 if (iB>iA) left=iA, right=iB; 
 else left=iB, right=iA; 
 if (left<0) left=0; 
 if (right>width) right=width; 
 n = right - left; 
 if (n>0) { 
 nn += n; 
 p = pp+left; 
 d = left - A; 
 if (0) r=r1+d*dxr, g=g1+d*dxg, b=b1+d*dxb; 
 if (0) o=o1+d*dxo, obar=1.0-o; 
 z=z1+d*dxz; 
 while (--n>=0) { 
 if ((z>p->z && z<p->front && z>p->back)) { 
 { p->c.r=r; p->c.g=g; p->c.b=b; p->z=z; }; 
 if (0) r+=dxr, g+=dxg, b+=dxb; 
 if (0) o+=dxo, obar=1.0-o; 
 z+=dxz; 
 p++; 
 } else { 
 if (0) r+=dxr, g+=dxg, b+=dxb; 
 if (0) o+=dxo, obar=1.0-o; 
 z+=dxz; 
 p++; 
 } 
 } 
 } 
 if (0) r1+=dyr, g1+=dyg, b1+=dyb; 
 if (0) o1+=dyo; 
 z1 += dyz; 
 A+=dyA, B+=dyB; 
 pp+=width; 
 } 
 } 
}
 buf->pixels += nn;
};
 CAT(tfoc,1) :
 continue;
 }
}
                         ;
    } else if (buf->pix_type==pix_fast) {
      {
 Point *p;
 int v1, v2, v3, i, width, height;
 int i1, i2, i3;
 float ox, oy;
 InvalidComponentHandle iElts;
 RGBColor vcolors[6];
 int shademe;
 char fcst = xf->fcst, bcst = xf->bcst, ncst = xf->ncst;
 char fbyte = xf->fbyte, bbyte = xf->bbyte;
 iElts = (invalid_status == INV_UNKNOWN) ? xf->iElts : NULL;
 width = buf->width, height = buf->height;
 ox = buf->ox, oy = buf->oy;
 if (0 && xf->lights) {
 fcolors = (Pointer)vcolors;
 bcolors = (Pointer)(vcolors+3);
 _dxfInitApplyLights(xf->kaf, xf->kdf, xf->ksf, xf->kspf,
 xf->kab, xf->kdb, xf->ksb, xf->kspb,
 xf->fcolors, xf->bcolors, xf->cmap,
 xf->normals, xf->lights,
 xf->colors_dep, xf->normals_dep,
 fcolors, bcolors, 3, fcst, bcst, ncst,
 fbyte, bbyte);
 cmap = NULL;
 shademe = 1;
 } else {
 shademe = 0;
 if (fbyte || bbyte)
 _dxf_initUbyteToFloat();
 }
 for (i=0; i<ntri; i++, tri++) {
 int index = (indices == NULL) ? i : indices[i];
 if (iElts && DXIsElementInvalid(iElts, index))
 continue;
 v1=tri->p, p= &xf->positions[v1], x1=p->x, y1=p->y, z1=p->z;
 v2=tri->q, p= &xf->positions[v2], x2=p->x, y2=p->y, z2=p->z;
 v3=tri->r, p= &xf->positions[v3], x3=p->x, y3=p->y, z3=p->z;
 if (shademe)
 {
 i1 = 0; i2 = 1; i3 = 2;
 if (! _dxfApplyLights((int *)tri, &index, 1))
 return ERROR;
 }
 if (xf->tile.perspective) {
 if (z1>nearPlane || z2>nearPlane || z3>nearPlane)
 continue;
 z1=(float)-1.0/z1; x1*=z1; y1*=z1;
 z2=(float)-1.0/z2; x2*=z2; y2*=z2;
 z3=(float)-1.0/z3; x3*=z3; y3*=z3;
 }
 if ((x1<minx && x2<minx && x3<minx) ||
 (x1>maxx && x2>maxx && x3>maxx) ||
 (y1<miny && y2<miny && y3<miny) ||
 (y1>maxy && y2>maxy && y3>maxy))
 continue;
 buf->triangles++;
 {
 struct fast *pp, *p;
 Pointer colors;
 int cstcolors;
 RGBColor cbuf1, cbuf2, cbuf3;
 char cbyte;
 char obyte = xf->obyte;
 x1+=ox, y1+=oy;
 x2+=ox, y2+=oy;
 x3+=ox, y3+=oy;
 if (x1<-(DXD_MAX_INT/2) || x1>(DXD_MAX_INT/2) || y1<-(DXD_MAX_INT/2) || y1>(DXD_MAX_INT/2) ||
 x2<-(DXD_MAX_INT/2) || x2>(DXD_MAX_INT/2) || y2<-(DXD_MAX_INT/2) || y2>(DXD_MAX_INT/2) ||
 x3<-(DXD_MAX_INT/2) || x3>(DXD_MAX_INT/2) || y3<-(DXD_MAX_INT/2) || y3>(DXD_MAX_INT/2))
 DXErrorReturn(ERROR_BAD_PARAMETER,
 "camera causes numerical overflow");
 d1 = y3-y2, d2 = y1-y3, d3 = y2-y1;
 d = x1*d1 + x2*d2 + x3*d3;
 Qx = d? (float)1.0 / (float)d : 0.0;
 d1 *= Qx, d2 *= Qx, d3 *= Qx;
 if (1) {
 if (d < 0)
 cbyte = fbyte, colors = fcolors, cstcolors = xf->fcst;
 else
 cbyte = bbyte, colors = bcolors, cstcolors = xf->bcst;
 if (!colors) goto CAT(tfof,1);
 }
 if (0) {
 if (shademe) {
 c1 = ((RGBColor *)colors) + i1;
 c2 = ((RGBColor *)colors) + i2;
 c3 = ((RGBColor *)colors) + i3;
 } else {
 if (cmap)
 c1= &(cmap[((unsigned char *)colors)[cstcolors ? 0 : v1]]), c2= &(cmap[((unsigned char *)colors)[cstcolors ? 0 : v2]]), c3= &(cmap[((unsigned char *)colors)[cstcolors ? 0 : v3]]);
 else {
 
{ 
 if (cbyte) 
 { 
 RGBByteColor *ptr=((RGBByteColor *)colors)+(cstcolors ? 0 : v1); 
 cbuf1.r = _dxd_ubyteToFloat[ptr->r]; 
 cbuf1.g = _dxd_ubyteToFloat[ptr->g]; 
 cbuf1.b = _dxd_ubyteToFloat[ptr->b]; 
 } else 
 cbuf1 = (((RGBColor *)colors)[cstcolors ? 0 : v1]); 
}; 
{ 
 if (cbyte) 
 { 
 RGBByteColor *ptr=((RGBByteColor *)colors)+(cstcolors ? 0 : v2); 
 cbuf2.r = _dxd_ubyteToFloat[ptr->r]; 
 cbuf2.g = _dxd_ubyteToFloat[ptr->g]; 
 cbuf2.b = _dxd_ubyteToFloat[ptr->b]; 
 } else 
 cbuf2 = (((RGBColor *)colors)[cstcolors ? 0 : v2]); 
}; 
{ 
 if (cbyte) 
 { 
 RGBByteColor *ptr=((RGBByteColor *)colors)+(cstcolors ? 0 : v3); 
 cbuf3.r = _dxd_ubyteToFloat[ptr->r]; 
 cbuf3.g = _dxd_ubyteToFloat[ptr->g]; 
 cbuf3.b = _dxd_ubyteToFloat[ptr->b]; 
 } else 
 cbuf3 = (((RGBColor *)colors)[cstcolors ? 0 : v3]); 
};
 c1 = &cbuf1; c2 = &cbuf2; c3 = &cbuf3;
 }
 }
 r1=c1->r, g1=c1->g, b1=c1->b;
 r2=c2->r, g2=c2->g, b2=c2->b;
 r3=c3->r, g3=c3->g, b3=c3->b;
 } else if (1) {
 if (cmap){
 r=(cmap[((unsigned char *)colors)[cstcolors ? 0 : index]]).r; g=(cmap[((unsigned char *)colors)[cstcolors ? 0 : index]]).g; b=(cmap[((unsigned char *)colors)[cstcolors ? 0 : index]]).b;
 } else {
 
{ 
 if (cbyte) 
 { 
 RGBByteColor *ptr=((RGBByteColor *)colors)+(cstcolors ? 0 : index); 
 cbuf1.r = _dxd_ubyteToFloat[ptr->r]; 
 cbuf1.g = _dxd_ubyteToFloat[ptr->g]; 
 cbuf1.b = _dxd_ubyteToFloat[ptr->b]; 
 } else 
 cbuf1 = (((RGBColor *)colors)[cstcolors ? 0 : index]); 
};
 r = cbuf1.r; g = cbuf1.g; b = cbuf1.b;
 }
 }
 if (0) {
 if (omap) o1=(omap[((unsigned char *)opacities)[xf->ocst ? 0 : v1]]), o2=(omap[((unsigned char *)opacities)[xf->ocst ? 0 : v2]]), o3=(omap[((unsigned char *)opacities)[xf->ocst ? 0 : v3]]);
 else {
 
{ 
 if (obyte) 
 o1 = _dxd_ubyteToFloat[((ubyte *)opacities)[xf->ocst ? 0 : v1]]; 
 else 
 o1 = (((float *)opacities)[xf->ocst ? 0 : v1]); 
}; 
{ 
 if (obyte) 
 o2 = _dxd_ubyteToFloat[((ubyte *)opacities)[xf->ocst ? 0 : v2]]; 
 else 
 o2 = (((float *)opacities)[xf->ocst ? 0 : v2]); 
}; 
{ 
 if (obyte) 
 o3 = _dxd_ubyteToFloat[((ubyte *)opacities)[xf->ocst ? 0 : v3]]; 
 else 
 o3 = (((float *)opacities)[xf->ocst ? 0 : v3]); 
};
 }
 } else if (opacities||omap) {
 if (omap) o=(omap[((unsigned char *)opacities)[xf->ocst ? 0 : index]]);
 else 
{ 
 if (obyte) 
 o = _dxd_ubyteToFloat[((ubyte *)opacities)[xf->ocst ? 0 : index]]; 
 else 
 o = (((float *)opacities)[xf->ocst ? 0 : index]); 
};
 obar = 1.0-o;
 } else
 o = 1.0, obar = 0.0;
 if (0) {
 dxr = (r1 == r2 && r1 == r3) ? 0.0 : d1*r1 + d2*r2 + d3*r3;
 dxg = (g1 == g2 && g1 == g3) ? 0.0 : d1*g1 + d2*g2 + d3*g3;
 dxb = (b1 == b2 && b1 == b3) ? 0.0 : d1*b1 + d2*b2 + d3*b3;
 }
 if (0) dxo = (o1 == o2 && o1 == o3) ? 0.0 : d1*o1 + d2*o2 + d3*o3;
 dxz = (z1 == z2 && z1 == z3) ? 0.0 : d1*z1 + d2*z2 + d3*z3;
 { 
 if (y2 < y1) { 
 (d=y1, y1=y2, y2=d), (d=x1, x1=x2, x2=d); 
 if (0) (d=r1, r1=r2, r2=d), (d=g1, g1=g2, g2=d), (d=b1, b1=b2, b2=d); 
 if (0) (d=o1, o1=o2, o2=d); 
 (d=z1, z1=z2, z2=d); 
 } 
}
 { 
 if (y3 < y1) { 
 (d=y1, y1=y3, y3=d), (d=x1, x1=x3, x3=d); 
 if (0) (d=r1, r1=r3, r3=d), (d=g1, g1=g3, g3=d), (d=b1, b1=b3, b3=d); 
 if (0) (d=o1, o1=o3, o3=d); 
 (d=z1, z1=z3, z3=d); 
 } 
}
 { 
 if (y3 < y2) { 
 (d=y2, y2=y3, y3=d), (d=x2, x2=x3, x3=d); 
 if (0) (d=r2, r2=r3, r3=d), (d=g2, g2=g3, g3=d), (d=b2, b2=b3, b3=d); 
 if (0) (d=o2, o2=o3, o3=d); 
 (d=z2, z2=z3, z3=d); 
 } 
}
 if (0) {
 A = B = x1;
 if (x2<A) A = x2; else B = x2;
 if (x3<A) A = x3; else if (x3>B) B = x3;
 if (A<buf->fmin.x) buf->fmin.x = A;
 if (B>buf->fmax.x) buf->fmax.x = B;
 if (y1<buf->fmin.y) buf->fmin.y = y1;
 if (y3>buf->fmax.y) buf->fmax.y = y3;
 }
 iy1 = ((int)((y1)-30000.0)+30000), iy2 = ((int)((y2)-30000.0)+30000), iy3 = ((int)((y3)-30000.0)+30000);
 if (iy1>iy2 || iy2>iy3) {
 DXSetError(ERROR_DATA_INVALID,
 "position number %d, %d, or %d is invalid", v1, v2, v3);
 return ERROR;
 }
 if (iy1==iy3)
 goto CAT(tfof,1);
 d = y3 - y1;
 Qy = d? (float)1.0 / (float)d : 0.0;
 if (0) dyr=(r3-r1)*Qy, dyg=(g3-g1)*Qy, dyb=(b3-b1)*Qy;
 if (0) dyo = (float)(o3-o1) * Qy;
 dyz = (z3-z1) * Qy;
 dyA = (float)(x3-x1) * Qy;
 A = x1;
 pp = buf->u.fast + iy1*width;
 d = iy1 - y1;
 if (iy1<0)
 d-=iy1, pp=buf->u.fast;
 A += dyA*d;
 if (0) r1+=dyr*d, g1+=dyg*d, b1+=dyb*d;
 if (0) o1+=dyo*d;
 z1 += dyz*d;
 nn = 0;
 { 
 
 if (iy2>height) 
 iy2 = height; 
 if (iy1<iy2 && iy2>0 && iy1<=height) { 
 B = x1; 
 d = y2 - y1; 
 d = d? (float)1.0 / (float)d : 0.0; 
 dyB = (float)(x2-x1) * d; 
 if (iy1<0) iy1=0; 
 d = iy1 - y1; 
 B += d*dyB; 
 for (iy=iy1; iy<iy2; iy++) { 
 iA = ((int)((A)-30000.0)+30000); 
 iB = ((int)((B)-30000.0)+30000); 
 if (iB>iA) left=iA, right=iB; 
 else left=iB, right=iA; 
 if (left<0) left=0; 
 if (right>width) right=width; 
 n = right - left; 
 if (n>0) { 
 nn += n; 
 p = pp+left; 
 d = left - A; 
 if (0) r=r1+d*dxr, g=g1+d*dxg, b=b1+d*dxb; 
 if (0) o=o1+d*dxo, obar=1.0-o; 
 z=z1+d*dxz; 
 while (--n>=0) { 
 if ((z>p->z)) { 
 { p->c.r=r; p->c.g=g; p->c.b=b; p->z=z; }; 
 if (0) r+=dxr, g+=dxg, b+=dxb; 
 if (0) o+=dxo, obar=1.0-o; 
 z+=dxz; 
 p++; 
 } else { 
 if (0) r+=dxr, g+=dxg, b+=dxb; 
 if (0) o+=dxo, obar=1.0-o; 
 z+=dxz; 
 p++; 
 } 
 } 
 } 
 if (0) r1+=dyr, g1+=dyg, b1+=dyb; 
 if (0) o1+=dyo; 
 z1 += dyz; 
 A+=dyA, B+=dyB; 
 pp+=width; 
 } 
 } 
}
 { 
 
 if (iy3>height) 
 iy3 = height; 
 if (iy2<iy3 && iy3>0 && iy2<=height) { 
 B = x2; 
 d = y3 - y2; 
 d = d? (float)1.0 / (float)d : 0.0; 
 dyB = (float)(x3-x2) * d; 
 if (iy2<0) iy2=0; 
 d = iy2 - y2; 
 B += d*dyB; 
 for (iy=iy2; iy<iy3; iy++) { 
 iA = ((int)((A)-30000.0)+30000); 
 iB = ((int)((B)-30000.0)+30000); 
 if (iB>iA) left=iA, right=iB; 
 else left=iB, right=iA; 
 if (left<0) left=0; 
 if (right>width) right=width; 
 n = right - left; 
 if (n>0) { 
 nn += n; 
 p = pp+left; 
 d = left - A; 
 if (0) r=r1+d*dxr, g=g1+d*dxg, b=b1+d*dxb; 
 if (0) o=o1+d*dxo, obar=1.0-o; 
 z=z1+d*dxz; 
 while (--n>=0) { 
 if ((z>p->z)) { 
 { p->c.r=r; p->c.g=g; p->c.b=b; p->z=z; }; 
 if (0) r+=dxr, g+=dxg, b+=dxb; 
 if (0) o+=dxo, obar=1.0-o; 
 z+=dxz; 
 p++; 
 } else { 
 if (0) r+=dxr, g+=dxg, b+=dxb; 
 if (0) o+=dxo, obar=1.0-o; 
 z+=dxz; 
 p++; 
 } 
 } 
 } 
 if (0) r1+=dyr, g1+=dyg, b1+=dyb; 
 if (0) o1+=dyo; 
 z1 += dyz; 
 A+=dyA, B+=dyB; 
 pp+=width; 
 } 
 } 
}
 buf->pixels += nn;
};
 CAT(tfof,1) :
 continue;
 }
}
                           ;
    } else if (buf->pix_type==pix_big) {
      {
 Point *p;
 int v1, v2, v3, i, width, height;
 int i1, i2, i3;
 float ox, oy;
 InvalidComponentHandle iElts;
 RGBColor vcolors[6];
 int shademe;
 char fcst = xf->fcst, bcst = xf->bcst, ncst = xf->ncst;
 char fbyte = xf->fbyte, bbyte = xf->bbyte;
 iElts = (invalid_status == INV_UNKNOWN) ? xf->iElts : NULL;
 width = buf->width, height = buf->height;
 ox = buf->ox, oy = buf->oy;
 if (0 && xf->lights) {
 fcolors = (Pointer)vcolors;
 bcolors = (Pointer)(vcolors+3);
 _dxfInitApplyLights(xf->kaf, xf->kdf, xf->ksf, xf->kspf,
 xf->kab, xf->kdb, xf->ksb, xf->kspb,
 xf->fcolors, xf->bcolors, xf->cmap,
 xf->normals, xf->lights,
 xf->colors_dep, xf->normals_dep,
 fcolors, bcolors, 3, fcst, bcst, ncst,
 fbyte, bbyte);
 cmap = NULL;
 shademe = 1;
 } else {
 shademe = 0;
 if (fbyte || bbyte)
 _dxf_initUbyteToFloat();
 }
 for (i=0; i<ntri; i++, tri++) {
 int index = (indices == NULL) ? i : indices[i];
 if (iElts && DXIsElementInvalid(iElts, index))
 continue;
 v1=tri->p, p= &xf->positions[v1], x1=p->x, y1=p->y, z1=p->z;
 v2=tri->q, p= &xf->positions[v2], x2=p->x, y2=p->y, z2=p->z;
 v3=tri->r, p= &xf->positions[v3], x3=p->x, y3=p->y, z3=p->z;
 if (shademe)
 {
 i1 = 0; i2 = 1; i3 = 2;
 if (! _dxfApplyLights((int *)tri, &index, 1))
 return ERROR;
 }
 if (xf->tile.perspective) {
 if (z1>nearPlane || z2>nearPlane || z3>nearPlane)
 continue;
 z1=(float)-1.0/z1; x1*=z1; y1*=z1;
 z2=(float)-1.0/z2; x2*=z2; y2*=z2;
 z3=(float)-1.0/z3; x3*=z3; y3*=z3;
 }
 if ((x1<minx && x2<minx && x3<minx) ||
 (x1>maxx && x2>maxx && x3>maxx) ||
 (y1<miny && y2<miny && y3<miny) ||
 (y1>maxy && y2>maxy && y3>maxy))
 continue;
 buf->triangles++;
 {
 struct big *pp, *p;
 Pointer colors;
 int cstcolors;
 RGBColor cbuf1, cbuf2, cbuf3;
 char cbyte;
 char obyte = xf->obyte;
 x1+=ox, y1+=oy;
 x2+=ox, y2+=oy;
 x3+=ox, y3+=oy;
 if (x1<-(DXD_MAX_INT/2) || x1>(DXD_MAX_INT/2) || y1<-(DXD_MAX_INT/2) || y1>(DXD_MAX_INT/2) ||
 x2<-(DXD_MAX_INT/2) || x2>(DXD_MAX_INT/2) || y2<-(DXD_MAX_INT/2) || y2>(DXD_MAX_INT/2) ||
 x3<-(DXD_MAX_INT/2) || x3>(DXD_MAX_INT/2) || y3<-(DXD_MAX_INT/2) || y3>(DXD_MAX_INT/2))
 DXErrorReturn(ERROR_BAD_PARAMETER,
 "camera causes numerical overflow");
 d1 = y3-y2, d2 = y1-y3, d3 = y2-y1;
 d = x1*d1 + x2*d2 + x3*d3;
 Qx = d? (float)1.0 / (float)d : 0.0;
 d1 *= Qx, d2 *= Qx, d3 *= Qx;
 if (1) {
 if (d < 0)
 cbyte = fbyte, colors = fcolors, cstcolors = xf->fcst;
 else
 cbyte = bbyte, colors = bcolors, cstcolors = xf->bcst;
 if (!colors) goto CAT(tfob,1);
 }
 if (0) {
 if (shademe) {
 c1 = ((RGBColor *)colors) + i1;
 c2 = ((RGBColor *)colors) + i2;
 c3 = ((RGBColor *)colors) + i3;
 } else {
 if (cmap)
 c1= &(cmap[((unsigned char *)colors)[cstcolors ? 0 : v1]]), c2= &(cmap[((unsigned char *)colors)[cstcolors ? 0 : v2]]), c3= &(cmap[((unsigned char *)colors)[cstcolors ? 0 : v3]]);
 else {
 
{ 
 if (cbyte) 
 { 
 RGBByteColor *ptr=((RGBByteColor *)colors)+(cstcolors ? 0 : v1); 
 cbuf1.r = _dxd_ubyteToFloat[ptr->r]; 
 cbuf1.g = _dxd_ubyteToFloat[ptr->g]; 
 cbuf1.b = _dxd_ubyteToFloat[ptr->b]; 
 } else 
 cbuf1 = (((RGBColor *)colors)[cstcolors ? 0 : v1]); 
}; 
{ 
 if (cbyte) 
 { 
 RGBByteColor *ptr=((RGBByteColor *)colors)+(cstcolors ? 0 : v2); 
 cbuf2.r = _dxd_ubyteToFloat[ptr->r]; 
 cbuf2.g = _dxd_ubyteToFloat[ptr->g]; 
 cbuf2.b = _dxd_ubyteToFloat[ptr->b]; 
 } else 
 cbuf2 = (((RGBColor *)colors)[cstcolors ? 0 : v2]); 
}; 
{ 
 if (cbyte) 
 { 
 RGBByteColor *ptr=((RGBByteColor *)colors)+(cstcolors ? 0 : v3); 
 cbuf3.r = _dxd_ubyteToFloat[ptr->r]; 
 cbuf3.g = _dxd_ubyteToFloat[ptr->g]; 
 cbuf3.b = _dxd_ubyteToFloat[ptr->b]; 
 } else 
 cbuf3 = (((RGBColor *)colors)[cstcolors ? 0 : v3]); 
};
 c1 = &cbuf1; c2 = &cbuf2; c3 = &cbuf3;
 }
 }
 r1=c1->r, g1=c1->g, b1=c1->b;
 r2=c2->r, g2=c2->g, b2=c2->b;
 r3=c3->r, g3=c3->g, b3=c3->b;
 } else if (1) {
 if (cmap){
 r=(cmap[((unsigned char *)colors)[cstcolors ? 0 : index]]).r; g=(cmap[((unsigned char *)colors)[cstcolors ? 0 : index]]).g; b=(cmap[((unsigned char *)colors)[cstcolors ? 0 : index]]).b;
 } else {
 
{ 
 if (cbyte) 
 { 
 RGBByteColor *ptr=((RGBByteColor *)colors)+(cstcolors ? 0 : index); 
 cbuf1.r = _dxd_ubyteToFloat[ptr->r]; 
 cbuf1.g = _dxd_ubyteToFloat[ptr->g]; 
 cbuf1.b = _dxd_ubyteToFloat[ptr->b]; 
 } else 
 cbuf1 = (((RGBColor *)colors)[cstcolors ? 0 : index]); 
};
 r = cbuf1.r; g = cbuf1.g; b = cbuf1.b;
 }
 }
 if (0) {
 if (omap) o1=(omap[((unsigned char *)opacities)[xf->ocst ? 0 : v1]]), o2=(omap[((unsigned char *)opacities)[xf->ocst ? 0 : v2]]), o3=(omap[((unsigned char *)opacities)[xf->ocst ? 0 : v3]]);
 else {
 
{ 
 if (obyte) 
 o1 = _dxd_ubyteToFloat[((ubyte *)opacities)[xf->ocst ? 0 : v1]]; 
 else 
 o1 = (((float *)opacities)[xf->ocst ? 0 : v1]); 
}; 
{ 
 if (obyte) 
 o2 = _dxd_ubyteToFloat[((ubyte *)opacities)[xf->ocst ? 0 : v2]]; 
 else 
 o2 = (((float *)opacities)[xf->ocst ? 0 : v2]); 
}; 
{ 
 if (obyte) 
 o3 = _dxd_ubyteToFloat[((ubyte *)opacities)[xf->ocst ? 0 : v3]]; 
 else 
 o3 = (((float *)opacities)[xf->ocst ? 0 : v3]); 
};
 }
 } else if (opacities||omap) {
 if (omap) o=(omap[((unsigned char *)opacities)[xf->ocst ? 0 : index]]);
 else 
{ 
 if (obyte) 
 o = _dxd_ubyteToFloat[((ubyte *)opacities)[xf->ocst ? 0 : index]]; 
 else 
 o = (((float *)opacities)[xf->ocst ? 0 : index]); 
};
 obar = 1.0-o;
 } else
 o = 1.0, obar = 0.0;
 if (0) {
 dxr = (r1 == r2 && r1 == r3) ? 0.0 : d1*r1 + d2*r2 + d3*r3;
 dxg = (g1 == g2 && g1 == g3) ? 0.0 : d1*g1 + d2*g2 + d3*g3;
 dxb = (b1 == b2 && b1 == b3) ? 0.0 : d1*b1 + d2*b2 + d3*b3;
 }
 if (0) dxo = (o1 == o2 && o1 == o3) ? 0.0 : d1*o1 + d2*o2 + d3*o3;
 dxz = (z1 == z2 && z1 == z3) ? 0.0 : d1*z1 + d2*z2 + d3*z3;
 { 
 if (y2 < y1) { 
 (d=y1, y1=y2, y2=d), (d=x1, x1=x2, x2=d); 
 if (0) (d=r1, r1=r2, r2=d), (d=g1, g1=g2, g2=d), (d=b1, b1=b2, b2=d); 
 if (0) (d=o1, o1=o2, o2=d); 
 (d=z1, z1=z2, z2=d); 
 } 
}
 { 
 if (y3 < y1) { 
 (d=y1, y1=y3, y3=d), (d=x1, x1=x3, x3=d); 
 if (0) (d=r1, r1=r3, r3=d), (d=g1, g1=g3, g3=d), (d=b1, b1=b3, b3=d); 
 if (0) (d=o1, o1=o3, o3=d); 
 (d=z1, z1=z3, z3=d); 
 } 
}
 { 
 if (y3 < y2) { 
 (d=y2, y2=y3, y3=d), (d=x2, x2=x3, x3=d); 
 if (0) (d=r2, r2=r3, r3=d), (d=g2, g2=g3, g3=d), (d=b2, b2=b3, b3=d); 
 if (0) (d=o2, o2=o3, o3=d); 
 (d=z2, z2=z3, z3=d); 
 } 
}
 if (0) {
 A = B = x1;
 if (x2<A) A = x2; else B = x2;
 if (x3<A) A = x3; else if (x3>B) B = x3;
 if (A<buf->fmin.x) buf->fmin.x = A;
 if (B>buf->fmax.x) buf->fmax.x = B;
 if (y1<buf->fmin.y) buf->fmin.y = y1;
 if (y3>buf->fmax.y) buf->fmax.y = y3;
 }
 iy1 = ((int)((y1)-30000.0)+30000), iy2 = ((int)((y2)-30000.0)+30000), iy3 = ((int)((y3)-30000.0)+30000);
 if (iy1>iy2 || iy2>iy3) {
 DXSetError(ERROR_DATA_INVALID,
 "position number %d, %d, or %d is invalid", v1, v2, v3);
 return ERROR;
 }
 if (iy1==iy3)
 goto CAT(tfob,1);
 d = y3 - y1;
 Qy = d? (float)1.0 / (float)d : 0.0;
 if (0) dyr=(r3-r1)*Qy, dyg=(g3-g1)*Qy, dyb=(b3-b1)*Qy;
 if (0) dyo = (float)(o3-o1) * Qy;
 dyz = (z3-z1) * Qy;
 dyA = (float)(x3-x1) * Qy;
 A = x1;
 pp = buf->u.big + iy1*width;
 d = iy1 - y1;
 if (iy1<0)
 d-=iy1, pp=buf->u.big;
 A += dyA*d;
 if (0) r1+=dyr*d, g1+=dyg*d, b1+=dyb*d;
 if (0) o1+=dyo*d;
 z1 += dyz*d;
 nn = 0;
 { 
 
 if (iy2>height) 
 iy2 = height; 
 if (iy1<iy2 && iy2>0 && iy1<=height) { 
 B = x1; 
 d = y2 - y1; 
 d = d? (float)1.0 / (float)d : 0.0; 
 dyB = (float)(x2-x1) * d; 
 if (iy1<0) iy1=0; 
 d = iy1 - y1; 
 B += d*dyB; 
 for (iy=iy1; iy<iy2; iy++) { 
 iA = ((int)((A)-30000.0)+30000); 
 iB = ((int)((B)-30000.0)+30000); 
 if (iB>iA) left=iA, right=iB; 
 else left=iB, right=iA; 
 if (left<0) left=0; 
 if (right>width) right=width; 
 n = right - left; 
 if (n>0) { 
 nn += n; 
 p = pp+left; 
 d = left - A; 
 if (0) r=r1+d*dxr, g=g1+d*dxg, b=b1+d*dxb; 
 if (0) o=o1+d*dxo, obar=1.0-o; 
 z=z1+d*dxz; 
 while (--n>=0) { 
 if ((z>p->z)) { 
 { p->c.r=r; p->c.g=g; p->c.b=b; p->z=z; }; 
 if (0) r+=dxr, g+=dxg, b+=dxb; 
 if (0) o+=dxo, obar=1.0-o; 
 z+=dxz; 
 p++; 
 } else { 
 if (0) r+=dxr, g+=dxg, b+=dxb; 
 if (0) o+=dxo, obar=1.0-o; 
 z+=dxz; 
 p++; 
 } 
 } 
 } 
 if (0) r1+=dyr, g1+=dyg, b1+=dyb; 
 if (0) o1+=dyo; 
 z1 += dyz; 
 A+=dyA, B+=dyB; 
 pp+=width; 
 } 
 } 
}
 { 
 
 if (iy3>height) 
 iy3 = height; 
 if (iy2<iy3 && iy3>0 && iy2<=height) { 
 B = x2; 
 d = y3 - y2; 
 d = d? (float)1.0 / (float)d : 0.0; 
 dyB = (float)(x3-x2) * d; 
 if (iy2<0) iy2=0; 
 d = iy2 - y2; 
 B += d*dyB; 
 for (iy=iy2; iy<iy3; iy++) { 
 iA = ((int)((A)-30000.0)+30000); 
 iB = ((int)((B)-30000.0)+30000); 
 if (iB>iA) left=iA, right=iB; 
 else left=iB, right=iA; 
 if (left<0) left=0; 
 if (right>width) right=width; 
 n = right - left; 
 if (n>0) { 
 nn += n; 
 p = pp+left; 
 d = left - A; 
 if (0) r=r1+d*dxr, g=g1+d*dxg, b=b1+d*dxb; 
 if (0) o=o1+d*dxo, obar=1.0-o; 
 z=z1+d*dxz; 
 while (--n>=0) { 
 if ((z>p->z)) { 
 { p->c.r=r; p->c.g=g; p->c.b=b; p->z=z; }; 
 if (0) r+=dxr, g+=dxg, b+=dxb; 
 if (0) o+=dxo, obar=1.0-o; 
 z+=dxz; 
 p++; 
 } else { 
 if (0) r+=dxr, g+=dxg, b+=dxb; 
 if (0) o+=dxo, obar=1.0-o; 
 z+=dxz; 
 p++; 
 } 
 } 
 } 
 if (0) r1+=dyr, g1+=dyg, b1+=dyb; 
 if (0) o1+=dyo; 
 z1 += dyz; 
 A+=dyA, B+=dyB; 
 pp+=width; 
 } 
 } 
}
 buf->pixels += nn;
};
 CAT(tfob,1) :
 continue;
 }
}
                           ;
    } else {
      DXSetError(ERROR_INTERNAL, "unknown pix_type %d", buf->pix_type);
      return ERROR;
    }
 return OK;
 }
Error
_dxf_TriangleFlat(
    struct buffer *buf,
    struct xfield *xf,
    int ntri,
    Triangle *tri,
    int *indices,
    Pointer fcolors,
    Pointer bcolors,
    Pointer opacities,
    int surface,
    int clip_status,
    inv_stat invalid_status
) {
 RGBColor *c1, *c2, *c3;
 float x1, y1, x2, y2, x3, y3;
 float r1, g1, b1, o1, z1, r2, g2, b2, o2, z2, r3, g3, b3, o3, z3;
 float Qx, dxo, dxz, dxr, dxg, dxb;
 float Qy, dyr, dyg, dyb, dyo, dyz, dyA, dyB;
 float r, g, b, o, obar, z, nearPlane=xf->nearPlane ;
 float A, B, d, d1, d2, d3;
 int iy1, iy2, iy3;
 int iA, iB, iy, i, n, nn, left, right;
 float minx = buf->min.x, miny = buf->min.y;
 float maxx = buf->max.x, maxy = buf->max.y;
    RGBColor *cmap = xf->cmap;
    float *omap = xf->omap;
    if (xf->tile.perspective) {
 struct xfield xx;
 for (i=0; i<ntri; ) {
     i = _dxf_zclip_triangles(xf, (int *)tri, i, indices,
     ntri, &xx, invalid_status);
     if (xx.nconnections)
  _dxf_TriangleFlat(buf, &xx, xx.nconnections, xx.c.triangles,
         xx.indices, xx.fcolors, xx.bcolors, xx.opacities,
         surface, clip_status, invalid_status);
 }
    }
    if (xf->ct!=ct_triangles && xf->ct!=ct_quads)
 return tri_flat_vol(buf, xf, ntri, tri, indices,
       fcolors, bcolors, opacities,
       surface, clip_status, invalid_status);
    else if (xf->opacities)
 return tri_flat_translucent(buf, xf, ntri, tri, indices,
        fcolors, bcolors, opacities,
        surface, clip_status, invalid_status);
    else
 return tri_flat_opaque(buf, xf, ntri, tri, indices,
          fcolors, bcolors, opacities,
          surface, clip_status, invalid_status);
}
Error
_dxf_TriangleClipping(
    struct buffer *buf,
    struct xfield *xf,
    int ntri,
    Triangle *tri,
    int *indices)
{
 RGBColor *c1, *c2, *c3;
 float x1, y1, x2, y2, x3, y3;
 float r1, g1, b1, o1, z1, r2, g2, b2, o2, z2, r3, g3, b3, o3, z3;
 float Qx, dxo, dxz, dxr, dxg, dxb;
 float Qy, dyr, dyg, dyb, dyo, dyz, dyA, dyB;
 float r, g, b, o, obar, z, nearPlane=xf->nearPlane ;
 float A, B, d, d1, d2, d3;
 int iy1, iy2, iy3;
 int iA, iB, iy, i, n, nn, left, right;
 float minx = buf->min.x, miny = buf->min.y;
 float maxx = buf->max.x, maxy = buf->max.y;
    Pointer fcolors = NULL;
    Pointer bcolors = NULL;
    Pointer opacities = NULL;
    RGBColor *cmap = NULL;
    float *omap = NULL;
    inv_stat invalid_status = INV_VALID;
    if (xf->tile.perspective) {
 struct xfield xx;
 for (i=0; i<ntri; ) {
     i = _dxf_zclip_triangles(xf, (int *)tri, i,
    indices, ntri, &xx, INV_VALID);
     if (xx.nconnections)
  _dxf_TriangleClipping(buf, &xx, xx.nconnections,
  xx.c.triangles, xx.indices);
 }
    }
    ASSERT(buf->pix_type==pix_big);
    {
 Point *p;
 int v1, v2, v3, i, width, height;
 int i1, i2, i3;
 float ox, oy;
 InvalidComponentHandle iElts;
 RGBColor vcolors[6];
 int shademe;
 char fcst = xf->fcst, bcst = xf->bcst, ncst = xf->ncst;
 char fbyte = xf->fbyte, bbyte = xf->bbyte;
 iElts = (invalid_status == INV_UNKNOWN) ? xf->iElts : NULL;
 width = buf->width, height = buf->height;
 ox = buf->ox, oy = buf->oy;
 if (0 && xf->lights) {
 fcolors = (Pointer)vcolors;
 bcolors = (Pointer)(vcolors+3);
 _dxfInitApplyLights(xf->kaf, xf->kdf, xf->ksf, xf->kspf,
 xf->kab, xf->kdb, xf->ksb, xf->kspb,
 xf->fcolors, xf->bcolors, xf->cmap,
 xf->normals, xf->lights,
 xf->colors_dep, xf->normals_dep,
 fcolors, bcolors, 3, fcst, bcst, ncst,
 fbyte, bbyte);
 cmap = NULL;
 shademe = 1;
 } else {
 shademe = 0;
 if (fbyte || bbyte)
 _dxf_initUbyteToFloat();
 }
 for (i=0; i<ntri; i++, tri++) {
 int index = (indices == NULL) ? i : indices[i];
 if (iElts && DXIsElementInvalid(iElts, index))
 continue;
 v1=tri->p, p= &xf->positions[v1], x1=p->x, y1=p->y, z1=p->z;
 v2=tri->q, p= &xf->positions[v2], x2=p->x, y2=p->y, z2=p->z;
 v3=tri->r, p= &xf->positions[v3], x3=p->x, y3=p->y, z3=p->z;
 if (shademe)
 {
 i1 = 0; i2 = 1; i3 = 2;
 if (! _dxfApplyLights((int *)tri, &index, 1))
 return ERROR;
 }
 if (xf->tile.perspective) {
 if (z1>nearPlane || z2>nearPlane || z3>nearPlane)
 continue;
 z1=(float)-1.0/z1; x1*=z1; y1*=z1;
 z2=(float)-1.0/z2; x2*=z2; y2*=z2;
 z3=(float)-1.0/z3; x3*=z3; y3*=z3;
 }
 if ((x1<minx && x2<minx && x3<minx) ||
 (x1>maxx && x2>maxx && x3>maxx) ||
 (y1<miny && y2<miny && y3<miny) ||
 (y1>maxy && y2>maxy && y3>maxy))
 continue;
 buf->triangles++;
 {
 struct big *pp, *p;
 Pointer colors;
 int cstcolors;
 RGBColor cbuf1, cbuf2, cbuf3;
 char cbyte;
 char obyte = xf->obyte;
 x1+=ox, y1+=oy;
 x2+=ox, y2+=oy;
 x3+=ox, y3+=oy;
 if (x1<-(DXD_MAX_INT/2) || x1>(DXD_MAX_INT/2) || y1<-(DXD_MAX_INT/2) || y1>(DXD_MAX_INT/2) ||
 x2<-(DXD_MAX_INT/2) || x2>(DXD_MAX_INT/2) || y2<-(DXD_MAX_INT/2) || y2>(DXD_MAX_INT/2) ||
 x3<-(DXD_MAX_INT/2) || x3>(DXD_MAX_INT/2) || y3<-(DXD_MAX_INT/2) || y3>(DXD_MAX_INT/2))
 DXErrorReturn(ERROR_BAD_PARAMETER,
 "camera causes numerical overflow");
 d1 = y3-y2, d2 = y1-y3, d3 = y2-y1;
 d = x1*d1 + x2*d2 + x3*d3;
 Qx = d? (float)1.0 / (float)d : 0.0;
 d1 *= Qx, d2 *= Qx, d3 *= Qx;
 if (0) {
 if (d < 0)
 cbyte = fbyte, colors = fcolors, cstcolors = xf->fcst;
 else
 cbyte = bbyte, colors = bcolors, cstcolors = xf->bcst;
 if (!colors) goto CAT(tc,1);
 }
 if (0) {
 if (shademe) {
 c1 = ((RGBColor *)colors) + i1;
 c2 = ((RGBColor *)colors) + i2;
 c3 = ((RGBColor *)colors) + i3;
 } else {
 if (cmap)
 c1= &(cmap[((unsigned char *)colors)[cstcolors ? 0 : v1]]), c2= &(cmap[((unsigned char *)colors)[cstcolors ? 0 : v2]]), c3= &(cmap[((unsigned char *)colors)[cstcolors ? 0 : v3]]);
 else {
 
{ 
 if (cbyte) 
 { 
 RGBByteColor *ptr=((RGBByteColor *)colors)+(cstcolors ? 0 : v1); 
 cbuf1.r = _dxd_ubyteToFloat[ptr->r]; 
 cbuf1.g = _dxd_ubyteToFloat[ptr->g]; 
 cbuf1.b = _dxd_ubyteToFloat[ptr->b]; 
 } else 
 cbuf1 = (((RGBColor *)colors)[cstcolors ? 0 : v1]); 
}; 
{ 
 if (cbyte) 
 { 
 RGBByteColor *ptr=((RGBByteColor *)colors)+(cstcolors ? 0 : v2); 
 cbuf2.r = _dxd_ubyteToFloat[ptr->r]; 
 cbuf2.g = _dxd_ubyteToFloat[ptr->g]; 
 cbuf2.b = _dxd_ubyteToFloat[ptr->b]; 
 } else 
 cbuf2 = (((RGBColor *)colors)[cstcolors ? 0 : v2]); 
}; 
{ 
 if (cbyte) 
 { 
 RGBByteColor *ptr=((RGBByteColor *)colors)+(cstcolors ? 0 : v3); 
 cbuf3.r = _dxd_ubyteToFloat[ptr->r]; 
 cbuf3.g = _dxd_ubyteToFloat[ptr->g]; 
 cbuf3.b = _dxd_ubyteToFloat[ptr->b]; 
 } else 
 cbuf3 = (((RGBColor *)colors)[cstcolors ? 0 : v3]); 
};
 c1 = &cbuf1; c2 = &cbuf2; c3 = &cbuf3;
 }
 }
 r1=c1->r, g1=c1->g, b1=c1->b;
 r2=c2->r, g2=c2->g, b2=c2->b;
 r3=c3->r, g3=c3->g, b3=c3->b;
 } else if (0) {
 if (cmap){
 r=(cmap[((unsigned char *)colors)[cstcolors ? 0 : index]]).r; g=(cmap[((unsigned char *)colors)[cstcolors ? 0 : index]]).g; b=(cmap[((unsigned char *)colors)[cstcolors ? 0 : index]]).b;
 } else {
 
{ 
 if (cbyte) 
 { 
 RGBByteColor *ptr=((RGBByteColor *)colors)+(cstcolors ? 0 : index); 
 cbuf1.r = _dxd_ubyteToFloat[ptr->r]; 
 cbuf1.g = _dxd_ubyteToFloat[ptr->g]; 
 cbuf1.b = _dxd_ubyteToFloat[ptr->b]; 
 } else 
 cbuf1 = (((RGBColor *)colors)[cstcolors ? 0 : index]); 
};
 r = cbuf1.r; g = cbuf1.g; b = cbuf1.b;
 }
 }
 if (0) {
 if (omap) o1=(omap[((unsigned char *)opacities)[xf->ocst ? 0 : v1]]), o2=(omap[((unsigned char *)opacities)[xf->ocst ? 0 : v2]]), o3=(omap[((unsigned char *)opacities)[xf->ocst ? 0 : v3]]);
 else {
 
{ 
 if (obyte) 
 o1 = _dxd_ubyteToFloat[((ubyte *)opacities)[xf->ocst ? 0 : v1]]; 
 else 
 o1 = (((float *)opacities)[xf->ocst ? 0 : v1]); 
}; 
{ 
 if (obyte) 
 o2 = _dxd_ubyteToFloat[((ubyte *)opacities)[xf->ocst ? 0 : v2]]; 
 else 
 o2 = (((float *)opacities)[xf->ocst ? 0 : v2]); 
}; 
{ 
 if (obyte) 
 o3 = _dxd_ubyteToFloat[((ubyte *)opacities)[xf->ocst ? 0 : v3]]; 
 else 
 o3 = (((float *)opacities)[xf->ocst ? 0 : v3]); 
};
 }
 } else if (opacities||omap) {
 if (omap) o=(omap[((unsigned char *)opacities)[xf->ocst ? 0 : index]]);
 else 
{ 
 if (obyte) 
 o = _dxd_ubyteToFloat[((ubyte *)opacities)[xf->ocst ? 0 : index]]; 
 else 
 o = (((float *)opacities)[xf->ocst ? 0 : index]); 
};
 obar = 1.0-o;
 } else
 o = 1.0, obar = 0.0;
 if (0) {
 dxr = (r1 == r2 && r1 == r3) ? 0.0 : d1*r1 + d2*r2 + d3*r3;
 dxg = (g1 == g2 && g1 == g3) ? 0.0 : d1*g1 + d2*g2 + d3*g3;
 dxb = (b1 == b2 && b1 == b3) ? 0.0 : d1*b1 + d2*b2 + d3*b3;
 }
 if (0) dxo = (o1 == o2 && o1 == o3) ? 0.0 : d1*o1 + d2*o2 + d3*o3;
 dxz = (z1 == z2 && z1 == z3) ? 0.0 : d1*z1 + d2*z2 + d3*z3;
 { 
 if (y2 < y1) { 
 (d=y1, y1=y2, y2=d), (d=x1, x1=x2, x2=d); 
 if (0) (d=r1, r1=r2, r2=d), (d=g1, g1=g2, g2=d), (d=b1, b1=b2, b2=d); 
 if (0) (d=o1, o1=o2, o2=d); 
 (d=z1, z1=z2, z2=d); 
 } 
}
 { 
 if (y3 < y1) { 
 (d=y1, y1=y3, y3=d), (d=x1, x1=x3, x3=d); 
 if (0) (d=r1, r1=r3, r3=d), (d=g1, g1=g3, g3=d), (d=b1, b1=b3, b3=d); 
 if (0) (d=o1, o1=o3, o3=d); 
 (d=z1, z1=z3, z3=d); 
 } 
}
 { 
 if (y3 < y2) { 
 (d=y2, y2=y3, y3=d), (d=x2, x2=x3, x3=d); 
 if (0) (d=r2, r2=r3, r3=d), (d=g2, g2=g3, g3=d), (d=b2, b2=b3, b3=d); 
 if (0) (d=o2, o2=o3, o3=d); 
 (d=z2, z2=z3, z3=d); 
 } 
}
 if (0) {
 A = B = x1;
 if (x2<A) A = x2; else B = x2;
 if (x3<A) A = x3; else if (x3>B) B = x3;
 if (A<buf->fmin.x) buf->fmin.x = A;
 if (B>buf->fmax.x) buf->fmax.x = B;
 if (y1<buf->fmin.y) buf->fmin.y = y1;
 if (y3>buf->fmax.y) buf->fmax.y = y3;
 }
 iy1 = ((int)((y1)-30000.0)+30000), iy2 = ((int)((y2)-30000.0)+30000), iy3 = ((int)((y3)-30000.0)+30000);
 if (iy1>iy2 || iy2>iy3) {
 DXSetError(ERROR_DATA_INVALID,
 "position number %d, %d, or %d is invalid", v1, v2, v3);
 return ERROR;
 }
 if (iy1==iy3)
 goto CAT(tc,1);
 d = y3 - y1;
 Qy = d? (float)1.0 / (float)d : 0.0;
 if (0) dyr=(r3-r1)*Qy, dyg=(g3-g1)*Qy, dyb=(b3-b1)*Qy;
 if (0) dyo = (float)(o3-o1) * Qy;
 dyz = (z3-z1) * Qy;
 dyA = (float)(x3-x1) * Qy;
 A = x1;
 pp = buf->u.big + iy1*width;
 d = iy1 - y1;
 if (iy1<0)
 d-=iy1, pp=buf->u.big;
 A += dyA*d;
 if (0) r1+=dyr*d, g1+=dyg*d, b1+=dyb*d;
 if (0) o1+=dyo*d;
 z1 += dyz*d;
 nn = 0;
 { 
 
 if (iy2>height) 
 iy2 = height; 
 if (iy1<iy2 && iy2>0 && iy1<=height) { 
 B = x1; 
 d = y2 - y1; 
 d = d? (float)1.0 / (float)d : 0.0; 
 dyB = (float)(x2-x1) * d; 
 if (iy1<0) iy1=0; 
 d = iy1 - y1; 
 B += d*dyB; 
 for (iy=iy1; iy<iy2; iy++) { 
 iA = ((int)((A)-30000.0)+30000); 
 iB = ((int)((B)-30000.0)+30000); 
 if (iB>iA) left=iA, right=iB; 
 else left=iB, right=iA; 
 if (left<0) left=0; 
 if (right>width) right=width; 
 n = right - left; 
 if (n>0) { 
 nn += n; 
 p = pp+left; 
 d = left - A; 
 if (0) r=r1+d*dxr, g=g1+d*dxg, b=b1+d*dxb; 
 if (0) o=o1+d*dxo, obar=1.0-o; 
 z=z1+d*dxz; 
 while (--n>=0) { 
 if (1) { 
 { if (z > p->front) p->front = z; if (z < p->back) p->back = z; }; 
 if (0) r+=dxr, g+=dxg, b+=dxb; 
 if (0) o+=dxo, obar=1.0-o; 
 z+=dxz; 
 p++; 
 } else { 
 if (0) r+=dxr, g+=dxg, b+=dxb; 
 if (0) o+=dxo, obar=1.0-o; 
 z+=dxz; 
 p++; 
 } 
 } 
 } 
 if (0) r1+=dyr, g1+=dyg, b1+=dyb; 
 if (0) o1+=dyo; 
 z1 += dyz; 
 A+=dyA, B+=dyB; 
 pp+=width; 
 } 
 } 
}
 { 
 
 if (iy3>height) 
 iy3 = height; 
 if (iy2<iy3 && iy3>0 && iy2<=height) { 
 B = x2; 
 d = y3 - y2; 
 d = d? (float)1.0 / (float)d : 0.0; 
 dyB = (float)(x3-x2) * d; 
 if (iy2<0) iy2=0; 
 d = iy2 - y2; 
 B += d*dyB; 
 for (iy=iy2; iy<iy3; iy++) { 
 iA = ((int)((A)-30000.0)+30000); 
 iB = ((int)((B)-30000.0)+30000); 
 if (iB>iA) left=iA, right=iB; 
 else left=iB, right=iA; 
 if (left<0) left=0; 
 if (right>width) right=width; 
 n = right - left; 
 if (n>0) { 
 nn += n; 
 p = pp+left; 
 d = left - A; 
 if (0) r=r1+d*dxr, g=g1+d*dxg, b=b1+d*dxb; 
 if (0) o=o1+d*dxo, obar=1.0-o; 
 z=z1+d*dxz; 
 while (--n>=0) { 
 if (1) { 
 { if (z > p->front) p->front = z; if (z < p->back) p->back = z; }; 
 if (0) r+=dxr, g+=dxg, b+=dxb; 
 if (0) o+=dxo, obar=1.0-o; 
 z+=dxz; 
 p++; 
 } else { 
 if (0) r+=dxr, g+=dxg, b+=dxb; 
 if (0) o+=dxo, obar=1.0-o; 
 z+=dxz; 
 p++; 
 } 
 } 
 } 
 if (0) r1+=dyr, g1+=dyg, b1+=dyb; 
 if (0) o1+=dyo; 
 z1 += dyz; 
 A+=dyA, B+=dyB; 
 pp+=width; 
 } 
 } 
}
 buf->pixels += nn;
};
 CAT(tc,1) :
 continue;
 }
};
    return OK;
}
static Error
_dxf_TriangleComposite(
    struct buffer *buf,
    struct xfield *xf,
    int ntri,
    Triangle *tri,
    int *indices,
    int clip_status,
    inv_stat invalid_status
) {
 RGBColor *c1, *c2, *c3;
 float x1, y1, x2, y2, x3, y3;
 float r1, g1, b1, o1, z1, r2, g2, b2, o2, z2, r3, g3, b3, o3, z3;
 float Qx, dxo, dxz, dxr, dxg, dxb;
 float Qy, dyr, dyg, dyb, dyo, dyz, dyA, dyB;
 float r, g, b, o, obar, z, nearPlane=xf->nearPlane ;
 float A, B, d, d1, d2, d3;
 int iy1, iy2, iy3;
 int iA, iB, iy, i, n, nn, left, right;
 float minx = buf->min.x, miny = buf->min.y;
 float maxx = buf->max.x, maxy = buf->max.y;
    Pointer fcolors = xf->fcolors;
    Pointer bcolors = xf->bcolors;
    Pointer opacities = xf->opacities;
    RGBColor *cmap = xf->cmap;
    float *omap = xf->omap;
    if (xf->tile.perspective)
 DXErrorReturn(ERROR_BAD_PARAMETER,
      "perspective volume rendering is not supported");
    if (clip_status) {
      ASSERT(buf->pix_type==pix_big);
      {
 Point *p;
 int v1, v2, v3, i, width, height;
 int i1, i2, i3;
 float ox, oy;
 InvalidComponentHandle iElts;
 RGBColor vcolors[6];
 int shademe;
 char fcst = xf->fcst, bcst = xf->bcst, ncst = xf->ncst;
 char fbyte = xf->fbyte, bbyte = xf->bbyte;
 iElts = (invalid_status == INV_UNKNOWN) ? xf->iElts : NULL;
 width = buf->width, height = buf->height;
 ox = buf->ox, oy = buf->oy;
 if (1 && xf->lights) {
 fcolors = (Pointer)vcolors;
 bcolors = (Pointer)(vcolors+3);
 _dxfInitApplyLights(xf->kaf, xf->kdf, xf->ksf, xf->kspf,
 xf->kab, xf->kdb, xf->ksb, xf->kspb,
 xf->fcolors, xf->bcolors, xf->cmap,
 xf->normals, xf->lights,
 xf->colors_dep, xf->normals_dep,
 fcolors, bcolors, 3, fcst, bcst, ncst,
 fbyte, bbyte);
 cmap = NULL;
 shademe = 1;
 } else {
 shademe = 0;
 if (fbyte || bbyte)
 _dxf_initUbyteToFloat();
 }
 for (i=0; i<ntri; i++, tri++) {
 int index = (indices == NULL) ? i : indices[i];
 if (iElts && DXIsElementInvalid(iElts, index))
 continue;
 v1=tri->p, p= &xf->positions[v1], x1=p->x, y1=p->y, z1=p->z;
 v2=tri->q, p= &xf->positions[v2], x2=p->x, y2=p->y, z2=p->z;
 v3=tri->r, p= &xf->positions[v3], x3=p->x, y3=p->y, z3=p->z;
 if (shademe)
 {
 i1 = 0; i2 = 1; i3 = 2;
 if (! _dxfApplyLights((int *)tri, &index, 1))
 return ERROR;
 }
 if (xf->tile.perspective) {
 if (z1>nearPlane || z2>nearPlane || z3>nearPlane)
 continue;
 z1=(float)-1.0/z1; x1*=z1; y1*=z1;
 z2=(float)-1.0/z2; x2*=z2; y2*=z2;
 z3=(float)-1.0/z3; x3*=z3; y3*=z3;
 }
 if ((x1<minx && x2<minx && x3<minx) ||
 (x1>maxx && x2>maxx && x3>maxx) ||
 (y1<miny && y2<miny && y3<miny) ||
 (y1>maxy && y2>maxy && y3>maxy))
 continue;
 buf->triangles++;
 {
 struct big *pp, *p;
 Pointer colors;
 int cstcolors;
 RGBColor cbuf1, cbuf2, cbuf3;
 char cbyte;
 char obyte = xf->obyte;
 x1+=ox, y1+=oy;
 x2+=ox, y2+=oy;
 x3+=ox, y3+=oy;
 if (x1<-(DXD_MAX_INT/2) || x1>(DXD_MAX_INT/2) || y1<-(DXD_MAX_INT/2) || y1>(DXD_MAX_INT/2) ||
 x2<-(DXD_MAX_INT/2) || x2>(DXD_MAX_INT/2) || y2<-(DXD_MAX_INT/2) || y2>(DXD_MAX_INT/2) ||
 x3<-(DXD_MAX_INT/2) || x3>(DXD_MAX_INT/2) || y3<-(DXD_MAX_INT/2) || y3>(DXD_MAX_INT/2))
 DXErrorReturn(ERROR_BAD_PARAMETER,
 "camera causes numerical overflow");
 d1 = y3-y2, d2 = y1-y3, d3 = y2-y1;
 d = x1*d1 + x2*d2 + x3*d3;
 Qx = d? (float)1.0 / (float)d : 0.0;
 d1 *= Qx, d2 *= Qx, d3 *= Qx;
 if (1) {
 if (d < 0)
 cbyte = fbyte, colors = fcolors, cstcolors = xf->fcst;
 else
 cbyte = bbyte, colors = bcolors, cstcolors = xf->bcst;
 if (!colors) goto CAT(tcc,1);
 }
 if (1) {
 if (shademe) {
 c1 = ((RGBColor *)colors) + i1;
 c2 = ((RGBColor *)colors) + i2;
 c3 = ((RGBColor *)colors) + i3;
 } else {
 if (cmap)
 c1= &(cmap[((unsigned char *)colors)[cstcolors ? 0 : v1]]), c2= &(cmap[((unsigned char *)colors)[cstcolors ? 0 : v2]]), c3= &(cmap[((unsigned char *)colors)[cstcolors ? 0 : v3]]);
 else {
 
{ 
 if (cbyte) 
 { 
 RGBByteColor *ptr=((RGBByteColor *)colors)+(cstcolors ? 0 : v1); 
 cbuf1.r = _dxd_ubyteToFloat[ptr->r]; 
 cbuf1.g = _dxd_ubyteToFloat[ptr->g]; 
 cbuf1.b = _dxd_ubyteToFloat[ptr->b]; 
 } else 
 cbuf1 = (((RGBColor *)colors)[cstcolors ? 0 : v1]); 
}; 
{ 
 if (cbyte) 
 { 
 RGBByteColor *ptr=((RGBByteColor *)colors)+(cstcolors ? 0 : v2); 
 cbuf2.r = _dxd_ubyteToFloat[ptr->r]; 
 cbuf2.g = _dxd_ubyteToFloat[ptr->g]; 
 cbuf2.b = _dxd_ubyteToFloat[ptr->b]; 
 } else 
 cbuf2 = (((RGBColor *)colors)[cstcolors ? 0 : v2]); 
}; 
{ 
 if (cbyte) 
 { 
 RGBByteColor *ptr=((RGBByteColor *)colors)+(cstcolors ? 0 : v3); 
 cbuf3.r = _dxd_ubyteToFloat[ptr->r]; 
 cbuf3.g = _dxd_ubyteToFloat[ptr->g]; 
 cbuf3.b = _dxd_ubyteToFloat[ptr->b]; 
 } else 
 cbuf3 = (((RGBColor *)colors)[cstcolors ? 0 : v3]); 
};
 c1 = &cbuf1; c2 = &cbuf2; c3 = &cbuf3;
 }
 }
 r1=c1->r, g1=c1->g, b1=c1->b;
 r2=c2->r, g2=c2->g, b2=c2->b;
 r3=c3->r, g3=c3->g, b3=c3->b;
 } else if (1) {
 if (cmap){
 r=(cmap[((unsigned char *)colors)[cstcolors ? 0 : index]]).r; g=(cmap[((unsigned char *)colors)[cstcolors ? 0 : index]]).g; b=(cmap[((unsigned char *)colors)[cstcolors ? 0 : index]]).b;
 } else {
 
{ 
 if (cbyte) 
 { 
 RGBByteColor *ptr=((RGBByteColor *)colors)+(cstcolors ? 0 : index); 
 cbuf1.r = _dxd_ubyteToFloat[ptr->r]; 
 cbuf1.g = _dxd_ubyteToFloat[ptr->g]; 
 cbuf1.b = _dxd_ubyteToFloat[ptr->b]; 
 } else 
 cbuf1 = (((RGBColor *)colors)[cstcolors ? 0 : index]); 
};
 r = cbuf1.r; g = cbuf1.g; b = cbuf1.b;
 }
 }
 if (1) {
 if (omap) o1=(omap[((unsigned char *)opacities)[xf->ocst ? 0 : v1]]), o2=(omap[((unsigned char *)opacities)[xf->ocst ? 0 : v2]]), o3=(omap[((unsigned char *)opacities)[xf->ocst ? 0 : v3]]);
 else {
 
{ 
 if (obyte) 
 o1 = _dxd_ubyteToFloat[((ubyte *)opacities)[xf->ocst ? 0 : v1]]; 
 else 
 o1 = (((float *)opacities)[xf->ocst ? 0 : v1]); 
}; 
{ 
 if (obyte) 
 o2 = _dxd_ubyteToFloat[((ubyte *)opacities)[xf->ocst ? 0 : v2]]; 
 else 
 o2 = (((float *)opacities)[xf->ocst ? 0 : v2]); 
}; 
{ 
 if (obyte) 
 o3 = _dxd_ubyteToFloat[((ubyte *)opacities)[xf->ocst ? 0 : v3]]; 
 else 
 o3 = (((float *)opacities)[xf->ocst ? 0 : v3]); 
};
 }
 } else if (opacities||omap) {
 if (omap) o=(omap[((unsigned char *)opacities)[xf->ocst ? 0 : index]]);
 else 
{ 
 if (obyte) 
 o = _dxd_ubyteToFloat[((ubyte *)opacities)[xf->ocst ? 0 : index]]; 
 else 
 o = (((float *)opacities)[xf->ocst ? 0 : index]); 
};
 obar = 1.0-o;
 } else
 o = 1.0, obar = 0.0;
 if (1) {
 dxr = (r1 == r2 && r1 == r3) ? 0.0 : d1*r1 + d2*r2 + d3*r3;
 dxg = (g1 == g2 && g1 == g3) ? 0.0 : d1*g1 + d2*g2 + d3*g3;
 dxb = (b1 == b2 && b1 == b3) ? 0.0 : d1*b1 + d2*b2 + d3*b3;
 }
 if (1) dxo = (o1 == o2 && o1 == o3) ? 0.0 : d1*o1 + d2*o2 + d3*o3;
 dxz = (z1 == z2 && z1 == z3) ? 0.0 : d1*z1 + d2*z2 + d3*z3;
 { 
 if (y2 < y1) { 
 (d=y1, y1=y2, y2=d), (d=x1, x1=x2, x2=d); 
 if (1) (d=r1, r1=r2, r2=d), (d=g1, g1=g2, g2=d), (d=b1, b1=b2, b2=d); 
 if (1) (d=o1, o1=o2, o2=d); 
 (d=z1, z1=z2, z2=d); 
 } 
}
 { 
 if (y3 < y1) { 
 (d=y1, y1=y3, y3=d), (d=x1, x1=x3, x3=d); 
 if (1) (d=r1, r1=r3, r3=d), (d=g1, g1=g3, g3=d), (d=b1, b1=b3, b3=d); 
 if (1) (d=o1, o1=o3, o3=d); 
 (d=z1, z1=z3, z3=d); 
 } 
}
 { 
 if (y3 < y2) { 
 (d=y2, y2=y3, y3=d), (d=x2, x2=x3, x3=d); 
 if (1) (d=r2, r2=r3, r3=d), (d=g2, g2=g3, g3=d), (d=b2, b2=b3, b3=d); 
 if (1) (d=o2, o2=o3, o3=d); 
 (d=z2, z2=z3, z3=d); 
 } 
}
 if (0) {
 A = B = x1;
 if (x2<A) A = x2; else B = x2;
 if (x3<A) A = x3; else if (x3>B) B = x3;
 if (A<buf->fmin.x) buf->fmin.x = A;
 if (B>buf->fmax.x) buf->fmax.x = B;
 if (y1<buf->fmin.y) buf->fmin.y = y1;
 if (y3>buf->fmax.y) buf->fmax.y = y3;
 }
 iy1 = ((int)((y1)-30000.0)+30000), iy2 = ((int)((y2)-30000.0)+30000), iy3 = ((int)((y3)-30000.0)+30000);
 if (iy1>iy2 || iy2>iy3) {
 DXSetError(ERROR_DATA_INVALID,
 "position number %d, %d, or %d is invalid", v1, v2, v3);
 return ERROR;
 }
 if (iy1==iy3)
 goto CAT(tcc,1);
 d = y3 - y1;
 Qy = d? (float)1.0 / (float)d : 0.0;
 if (1) dyr=(r3-r1)*Qy, dyg=(g3-g1)*Qy, dyb=(b3-b1)*Qy;
 if (1) dyo = (float)(o3-o1) * Qy;
 dyz = (z3-z1) * Qy;
 dyA = (float)(x3-x1) * Qy;
 A = x1;
 pp = buf->u.big + iy1*width;
 d = iy1 - y1;
 if (iy1<0)
 d-=iy1, pp=buf->u.big;
 A += dyA*d;
 if (1) r1+=dyr*d, g1+=dyg*d, b1+=dyb*d;
 if (1) o1+=dyo*d;
 z1 += dyz*d;
 nn = 0;
 { 
 
 if (iy2>height) 
 iy2 = height; 
 if (iy1<iy2 && iy2>0 && iy1<=height) { 
 B = x1; 
 d = y2 - y1; 
 d = d? (float)1.0 / (float)d : 0.0; 
 dyB = (float)(x2-x1) * d; 
 if (iy1<0) iy1=0; 
 d = iy1 - y1; 
 B += d*dyB; 
 for (iy=iy1; iy<iy2; iy++) { 
 iA = ((int)((A)-30000.0)+30000); 
 iB = ((int)((B)-30000.0)+30000); 
 if (iB>iA) left=iA, right=iB; 
 else left=iB, right=iA; 
 if (left<0) left=0; 
 if (right>width) right=width; 
 n = right - left; 
 if (n>0) { 
 nn += n; 
 p = pp+left; 
 d = left - A; 
 if (1) r=r1+d*dxr, g=g1+d*dxg, b=b1+d*dxb; 
 if (1) o=o1+d*dxo, obar=1.0-o; 
 z=z1+d*dxz; 
 while (--n>=0) { 
 if ((z>p->z && z<p->front)) { 
 { p->c.r=r+obar*p->c.r; p->c.g=g+obar*p->c.g; p->c.b=b+obar*p->c.b; }; 
 if (1) r+=dxr, g+=dxg, b+=dxb; 
 if (1) o+=dxo, obar=1.0-o; 
 z+=dxz; 
 p++; 
 } else { 
 if (1) r+=dxr, g+=dxg, b+=dxb; 
 if (1) o+=dxo, obar=1.0-o; 
 z+=dxz; 
 p++; 
 } 
 } 
 } 
 if (1) r1+=dyr, g1+=dyg, b1+=dyb; 
 if (1) o1+=dyo; 
 z1 += dyz; 
 A+=dyA, B+=dyB; 
 pp+=width; 
 } 
 } 
}
 { 
 
 if (iy3>height) 
 iy3 = height; 
 if (iy2<iy3 && iy3>0 && iy2<=height) { 
 B = x2; 
 d = y3 - y2; 
 d = d? (float)1.0 / (float)d : 0.0; 
 dyB = (float)(x3-x2) * d; 
 if (iy2<0) iy2=0; 
 d = iy2 - y2; 
 B += d*dyB; 
 for (iy=iy2; iy<iy3; iy++) { 
 iA = ((int)((A)-30000.0)+30000); 
 iB = ((int)((B)-30000.0)+30000); 
 if (iB>iA) left=iA, right=iB; 
 else left=iB, right=iA; 
 if (left<0) left=0; 
 if (right>width) right=width; 
 n = right - left; 
 if (n>0) { 
 nn += n; 
 p = pp+left; 
 d = left - A; 
 if (1) r=r1+d*dxr, g=g1+d*dxg, b=b1+d*dxb; 
 if (1) o=o1+d*dxo, obar=1.0-o; 
 z=z1+d*dxz; 
 while (--n>=0) { 
 if ((z>p->z && z<p->front)) { 
 { p->c.r=r+obar*p->c.r; p->c.g=g+obar*p->c.g; p->c.b=b+obar*p->c.b; }; 
 if (1) r+=dxr, g+=dxg, b+=dxb; 
 if (1) o+=dxo, obar=1.0-o; 
 z+=dxz; 
 p++; 
 } else { 
 if (1) r+=dxr, g+=dxg, b+=dxb; 
 if (1) o+=dxo, obar=1.0-o; 
 z+=dxz; 
 p++; 
 } 
 } 
 } 
 if (1) r1+=dyr, g1+=dyg, b1+=dyb; 
 if (1) o1+=dyo; 
 z1 += dyz; 
 A+=dyA, B+=dyB; 
 pp+=width; 
 } 
 } 
}
 buf->pixels += nn;
};
 CAT(tcc,1) :
 continue;
 }
};
    } else if (buf->pix_type==pix_fast) {
      {
 Point *p;
 int v1, v2, v3, i, width, height;
 int i1, i2, i3;
 float ox, oy;
 InvalidComponentHandle iElts;
 RGBColor vcolors[6];
 int shademe;
 char fcst = xf->fcst, bcst = xf->bcst, ncst = xf->ncst;
 char fbyte = xf->fbyte, bbyte = xf->bbyte;
 iElts = (invalid_status == INV_UNKNOWN) ? xf->iElts : NULL;
 width = buf->width, height = buf->height;
 ox = buf->ox, oy = buf->oy;
 if (1 && xf->lights) {
 fcolors = (Pointer)vcolors;
 bcolors = (Pointer)(vcolors+3);
 _dxfInitApplyLights(xf->kaf, xf->kdf, xf->ksf, xf->kspf,
 xf->kab, xf->kdb, xf->ksb, xf->kspb,
 xf->fcolors, xf->bcolors, xf->cmap,
 xf->normals, xf->lights,
 xf->colors_dep, xf->normals_dep,
 fcolors, bcolors, 3, fcst, bcst, ncst,
 fbyte, bbyte);
 cmap = NULL;
 shademe = 1;
 } else {
 shademe = 0;
 if (fbyte || bbyte)
 _dxf_initUbyteToFloat();
 }
 for (i=0; i<ntri; i++, tri++) {
 int index = (indices == NULL) ? i : indices[i];
 if (iElts && DXIsElementInvalid(iElts, index))
 continue;
 v1=tri->p, p= &xf->positions[v1], x1=p->x, y1=p->y, z1=p->z;
 v2=tri->q, p= &xf->positions[v2], x2=p->x, y2=p->y, z2=p->z;
 v3=tri->r, p= &xf->positions[v3], x3=p->x, y3=p->y, z3=p->z;
 if (shademe)
 {
 i1 = 0; i2 = 1; i3 = 2;
 if (! _dxfApplyLights((int *)tri, &index, 1))
 return ERROR;
 }
 if (xf->tile.perspective) {
 if (z1>nearPlane || z2>nearPlane || z3>nearPlane)
 continue;
 z1=(float)-1.0/z1; x1*=z1; y1*=z1;
 z2=(float)-1.0/z2; x2*=z2; y2*=z2;
 z3=(float)-1.0/z3; x3*=z3; y3*=z3;
 }
 if ((x1<minx && x2<minx && x3<minx) ||
 (x1>maxx && x2>maxx && x3>maxx) ||
 (y1<miny && y2<miny && y3<miny) ||
 (y1>maxy && y2>maxy && y3>maxy))
 continue;
 buf->triangles++;
 {
 struct fast *pp, *p;
 Pointer colors;
 int cstcolors;
 RGBColor cbuf1, cbuf2, cbuf3;
 char cbyte;
 char obyte = xf->obyte;
 x1+=ox, y1+=oy;
 x2+=ox, y2+=oy;
 x3+=ox, y3+=oy;
 if (x1<-(DXD_MAX_INT/2) || x1>(DXD_MAX_INT/2) || y1<-(DXD_MAX_INT/2) || y1>(DXD_MAX_INT/2) ||
 x2<-(DXD_MAX_INT/2) || x2>(DXD_MAX_INT/2) || y2<-(DXD_MAX_INT/2) || y2>(DXD_MAX_INT/2) ||
 x3<-(DXD_MAX_INT/2) || x3>(DXD_MAX_INT/2) || y3<-(DXD_MAX_INT/2) || y3>(DXD_MAX_INT/2))
 DXErrorReturn(ERROR_BAD_PARAMETER,
 "camera causes numerical overflow");
 d1 = y3-y2, d2 = y1-y3, d3 = y2-y1;
 d = x1*d1 + x2*d2 + x3*d3;
 Qx = d? (float)1.0 / (float)d : 0.0;
 d1 *= Qx, d2 *= Qx, d3 *= Qx;
 if (1) {
 if (d < 0)
 cbyte = fbyte, colors = fcolors, cstcolors = xf->fcst;
 else
 cbyte = bbyte, colors = bcolors, cstcolors = xf->bcst;
 if (!colors) goto CAT(tcf,1);
 }
 if (1) {
 if (shademe) {
 c1 = ((RGBColor *)colors) + i1;
 c2 = ((RGBColor *)colors) + i2;
 c3 = ((RGBColor *)colors) + i3;
 } else {
 if (cmap)
 c1= &(cmap[((unsigned char *)colors)[cstcolors ? 0 : v1]]), c2= &(cmap[((unsigned char *)colors)[cstcolors ? 0 : v2]]), c3= &(cmap[((unsigned char *)colors)[cstcolors ? 0 : v3]]);
 else {
 
{ 
 if (cbyte) 
 { 
 RGBByteColor *ptr=((RGBByteColor *)colors)+(cstcolors ? 0 : v1); 
 cbuf1.r = _dxd_ubyteToFloat[ptr->r]; 
 cbuf1.g = _dxd_ubyteToFloat[ptr->g]; 
 cbuf1.b = _dxd_ubyteToFloat[ptr->b]; 
 } else 
 cbuf1 = (((RGBColor *)colors)[cstcolors ? 0 : v1]); 
}; 
{ 
 if (cbyte) 
 { 
 RGBByteColor *ptr=((RGBByteColor *)colors)+(cstcolors ? 0 : v2); 
 cbuf2.r = _dxd_ubyteToFloat[ptr->r]; 
 cbuf2.g = _dxd_ubyteToFloat[ptr->g]; 
 cbuf2.b = _dxd_ubyteToFloat[ptr->b]; 
 } else 
 cbuf2 = (((RGBColor *)colors)[cstcolors ? 0 : v2]); 
}; 
{ 
 if (cbyte) 
 { 
 RGBByteColor *ptr=((RGBByteColor *)colors)+(cstcolors ? 0 : v3); 
 cbuf3.r = _dxd_ubyteToFloat[ptr->r]; 
 cbuf3.g = _dxd_ubyteToFloat[ptr->g]; 
 cbuf3.b = _dxd_ubyteToFloat[ptr->b]; 
 } else 
 cbuf3 = (((RGBColor *)colors)[cstcolors ? 0 : v3]); 
};
 c1 = &cbuf1; c2 = &cbuf2; c3 = &cbuf3;
 }
 }
 r1=c1->r, g1=c1->g, b1=c1->b;
 r2=c2->r, g2=c2->g, b2=c2->b;
 r3=c3->r, g3=c3->g, b3=c3->b;
 } else if (1) {
 if (cmap){
 r=(cmap[((unsigned char *)colors)[cstcolors ? 0 : index]]).r; g=(cmap[((unsigned char *)colors)[cstcolors ? 0 : index]]).g; b=(cmap[((unsigned char *)colors)[cstcolors ? 0 : index]]).b;
 } else {
 
{ 
 if (cbyte) 
 { 
 RGBByteColor *ptr=((RGBByteColor *)colors)+(cstcolors ? 0 : index); 
 cbuf1.r = _dxd_ubyteToFloat[ptr->r]; 
 cbuf1.g = _dxd_ubyteToFloat[ptr->g]; 
 cbuf1.b = _dxd_ubyteToFloat[ptr->b]; 
 } else 
 cbuf1 = (((RGBColor *)colors)[cstcolors ? 0 : index]); 
};
 r = cbuf1.r; g = cbuf1.g; b = cbuf1.b;
 }
 }
 if (1) {
 if (omap) o1=(omap[((unsigned char *)opacities)[xf->ocst ? 0 : v1]]), o2=(omap[((unsigned char *)opacities)[xf->ocst ? 0 : v2]]), o3=(omap[((unsigned char *)opacities)[xf->ocst ? 0 : v3]]);
 else {
 
{ 
 if (obyte) 
 o1 = _dxd_ubyteToFloat[((ubyte *)opacities)[xf->ocst ? 0 : v1]]; 
 else 
 o1 = (((float *)opacities)[xf->ocst ? 0 : v1]); 
}; 
{ 
 if (obyte) 
 o2 = _dxd_ubyteToFloat[((ubyte *)opacities)[xf->ocst ? 0 : v2]]; 
 else 
 o2 = (((float *)opacities)[xf->ocst ? 0 : v2]); 
}; 
{ 
 if (obyte) 
 o3 = _dxd_ubyteToFloat[((ubyte *)opacities)[xf->ocst ? 0 : v3]]; 
 else 
 o3 = (((float *)opacities)[xf->ocst ? 0 : v3]); 
};
 }
 } else if (opacities||omap) {
 if (omap) o=(omap[((unsigned char *)opacities)[xf->ocst ? 0 : index]]);
 else 
{ 
 if (obyte) 
 o = _dxd_ubyteToFloat[((ubyte *)opacities)[xf->ocst ? 0 : index]]; 
 else 
 o = (((float *)opacities)[xf->ocst ? 0 : index]); 
};
 obar = 1.0-o;
 } else
 o = 1.0, obar = 0.0;
 if (1) {
 dxr = (r1 == r2 && r1 == r3) ? 0.0 : d1*r1 + d2*r2 + d3*r3;
 dxg = (g1 == g2 && g1 == g3) ? 0.0 : d1*g1 + d2*g2 + d3*g3;
 dxb = (b1 == b2 && b1 == b3) ? 0.0 : d1*b1 + d2*b2 + d3*b3;
 }
 if (1) dxo = (o1 == o2 && o1 == o3) ? 0.0 : d1*o1 + d2*o2 + d3*o3;
 dxz = (z1 == z2 && z1 == z3) ? 0.0 : d1*z1 + d2*z2 + d3*z3;
 { 
 if (y2 < y1) { 
 (d=y1, y1=y2, y2=d), (d=x1, x1=x2, x2=d); 
 if (1) (d=r1, r1=r2, r2=d), (d=g1, g1=g2, g2=d), (d=b1, b1=b2, b2=d); 
 if (1) (d=o1, o1=o2, o2=d); 
 (d=z1, z1=z2, z2=d); 
 } 
}
 { 
 if (y3 < y1) { 
 (d=y1, y1=y3, y3=d), (d=x1, x1=x3, x3=d); 
 if (1) (d=r1, r1=r3, r3=d), (d=g1, g1=g3, g3=d), (d=b1, b1=b3, b3=d); 
 if (1) (d=o1, o1=o3, o3=d); 
 (d=z1, z1=z3, z3=d); 
 } 
}
 { 
 if (y3 < y2) { 
 (d=y2, y2=y3, y3=d), (d=x2, x2=x3, x3=d); 
 if (1) (d=r2, r2=r3, r3=d), (d=g2, g2=g3, g3=d), (d=b2, b2=b3, b3=d); 
 if (1) (d=o2, o2=o3, o3=d); 
 (d=z2, z2=z3, z3=d); 
 } 
}
 if (0) {
 A = B = x1;
 if (x2<A) A = x2; else B = x2;
 if (x3<A) A = x3; else if (x3>B) B = x3;
 if (A<buf->fmin.x) buf->fmin.x = A;
 if (B>buf->fmax.x) buf->fmax.x = B;
 if (y1<buf->fmin.y) buf->fmin.y = y1;
 if (y3>buf->fmax.y) buf->fmax.y = y3;
 }
 iy1 = ((int)((y1)-30000.0)+30000), iy2 = ((int)((y2)-30000.0)+30000), iy3 = ((int)((y3)-30000.0)+30000);
 if (iy1>iy2 || iy2>iy3) {
 DXSetError(ERROR_DATA_INVALID,
 "position number %d, %d, or %d is invalid", v1, v2, v3);
 return ERROR;
 }
 if (iy1==iy3)
 goto CAT(tcf,1);
 d = y3 - y1;
 Qy = d? (float)1.0 / (float)d : 0.0;
 if (1) dyr=(r3-r1)*Qy, dyg=(g3-g1)*Qy, dyb=(b3-b1)*Qy;
 if (1) dyo = (float)(o3-o1) * Qy;
 dyz = (z3-z1) * Qy;
 dyA = (float)(x3-x1) * Qy;
 A = x1;
 pp = buf->u.fast + iy1*width;
 d = iy1 - y1;
 if (iy1<0)
 d-=iy1, pp=buf->u.fast;
 A += dyA*d;
 if (1) r1+=dyr*d, g1+=dyg*d, b1+=dyb*d;
 if (1) o1+=dyo*d;
 z1 += dyz*d;
 nn = 0;
 { 
 
 if (iy2>height) 
 iy2 = height; 
 if (iy1<iy2 && iy2>0 && iy1<=height) { 
 B = x1; 
 d = y2 - y1; 
 d = d? (float)1.0 / (float)d : 0.0; 
 dyB = (float)(x2-x1) * d; 
 if (iy1<0) iy1=0; 
 d = iy1 - y1; 
 B += d*dyB; 
 for (iy=iy1; iy<iy2; iy++) { 
 iA = ((int)((A)-30000.0)+30000); 
 iB = ((int)((B)-30000.0)+30000); 
 if (iB>iA) left=iA, right=iB; 
 else left=iB, right=iA; 
 if (left<0) left=0; 
 if (right>width) right=width; 
 n = right - left; 
 if (n>0) { 
 nn += n; 
 p = pp+left; 
 d = left - A; 
 if (1) r=r1+d*dxr, g=g1+d*dxg, b=b1+d*dxb; 
 if (1) o=o1+d*dxo, obar=1.0-o; 
 z=z1+d*dxz; 
 while (--n>=0) { 
 if ((z>p->z)) { 
 { p->c.r=r+obar*p->c.r; p->c.g=g+obar*p->c.g; p->c.b=b+obar*p->c.b; }; 
 if (1) r+=dxr, g+=dxg, b+=dxb; 
 if (1) o+=dxo, obar=1.0-o; 
 z+=dxz; 
 p++; 
 } else { 
 if (1) r+=dxr, g+=dxg, b+=dxb; 
 if (1) o+=dxo, obar=1.0-o; 
 z+=dxz; 
 p++; 
 } 
 } 
 } 
 if (1) r1+=dyr, g1+=dyg, b1+=dyb; 
 if (1) o1+=dyo; 
 z1 += dyz; 
 A+=dyA, B+=dyB; 
 pp+=width; 
 } 
 } 
}
 { 
 
 if (iy3>height) 
 iy3 = height; 
 if (iy2<iy3 && iy3>0 && iy2<=height) { 
 B = x2; 
 d = y3 - y2; 
 d = d? (float)1.0 / (float)d : 0.0; 
 dyB = (float)(x3-x2) * d; 
 if (iy2<0) iy2=0; 
 d = iy2 - y2; 
 B += d*dyB; 
 for (iy=iy2; iy<iy3; iy++) { 
 iA = ((int)((A)-30000.0)+30000); 
 iB = ((int)((B)-30000.0)+30000); 
 if (iB>iA) left=iA, right=iB; 
 else left=iB, right=iA; 
 if (left<0) left=0; 
 if (right>width) right=width; 
 n = right - left; 
 if (n>0) { 
 nn += n; 
 p = pp+left; 
 d = left - A; 
 if (1) r=r1+d*dxr, g=g1+d*dxg, b=b1+d*dxb; 
 if (1) o=o1+d*dxo, obar=1.0-o; 
 z=z1+d*dxz; 
 while (--n>=0) { 
 if ((z>p->z)) { 
 { p->c.r=r+obar*p->c.r; p->c.g=g+obar*p->c.g; p->c.b=b+obar*p->c.b; }; 
 if (1) r+=dxr, g+=dxg, b+=dxb; 
 if (1) o+=dxo, obar=1.0-o; 
 z+=dxz; 
 p++; 
 } else { 
 if (1) r+=dxr, g+=dxg, b+=dxb; 
 if (1) o+=dxo, obar=1.0-o; 
 z+=dxz; 
 p++; 
 } 
 } 
 } 
 if (1) r1+=dyr, g1+=dyg, b1+=dyb; 
 if (1) o1+=dyo; 
 z1 += dyz; 
 A+=dyA, B+=dyB; 
 pp+=width; 
 } 
 } 
}
 buf->pixels += nn;
};
 CAT(tcf,1) :
 continue;
 }
};
    } else if (buf->pix_type==pix_big) {
      {
 Point *p;
 int v1, v2, v3, i, width, height;
 int i1, i2, i3;
 float ox, oy;
 InvalidComponentHandle iElts;
 RGBColor vcolors[6];
 int shademe;
 char fcst = xf->fcst, bcst = xf->bcst, ncst = xf->ncst;
 char fbyte = xf->fbyte, bbyte = xf->bbyte;
 iElts = (invalid_status == INV_UNKNOWN) ? xf->iElts : NULL;
 width = buf->width, height = buf->height;
 ox = buf->ox, oy = buf->oy;
 if (1 && xf->lights) {
 fcolors = (Pointer)vcolors;
 bcolors = (Pointer)(vcolors+3);
 _dxfInitApplyLights(xf->kaf, xf->kdf, xf->ksf, xf->kspf,
 xf->kab, xf->kdb, xf->ksb, xf->kspb,
 xf->fcolors, xf->bcolors, xf->cmap,
 xf->normals, xf->lights,
 xf->colors_dep, xf->normals_dep,
 fcolors, bcolors, 3, fcst, bcst, ncst,
 fbyte, bbyte);
 cmap = NULL;
 shademe = 1;
 } else {
 shademe = 0;
 if (fbyte || bbyte)
 _dxf_initUbyteToFloat();
 }
 for (i=0; i<ntri; i++, tri++) {
 int index = (indices == NULL) ? i : indices[i];
 if (iElts && DXIsElementInvalid(iElts, index))
 continue;
 v1=tri->p, p= &xf->positions[v1], x1=p->x, y1=p->y, z1=p->z;
 v2=tri->q, p= &xf->positions[v2], x2=p->x, y2=p->y, z2=p->z;
 v3=tri->r, p= &xf->positions[v3], x3=p->x, y3=p->y, z3=p->z;
 if (shademe)
 {
 i1 = 0; i2 = 1; i3 = 2;
 if (! _dxfApplyLights((int *)tri, &index, 1))
 return ERROR;
 }
 if (xf->tile.perspective) {
 if (z1>nearPlane || z2>nearPlane || z3>nearPlane)
 continue;
 z1=(float)-1.0/z1; x1*=z1; y1*=z1;
 z2=(float)-1.0/z2; x2*=z2; y2*=z2;
 z3=(float)-1.0/z3; x3*=z3; y3*=z3;
 }
 if ((x1<minx && x2<minx && x3<minx) ||
 (x1>maxx && x2>maxx && x3>maxx) ||
 (y1<miny && y2<miny && y3<miny) ||
 (y1>maxy && y2>maxy && y3>maxy))
 continue;
 buf->triangles++;
 {
 struct big *pp, *p;
 Pointer colors;
 int cstcolors;
 RGBColor cbuf1, cbuf2, cbuf3;
 char cbyte;
 char obyte = xf->obyte;
 x1+=ox, y1+=oy;
 x2+=ox, y2+=oy;
 x3+=ox, y3+=oy;
 if (x1<-(DXD_MAX_INT/2) || x1>(DXD_MAX_INT/2) || y1<-(DXD_MAX_INT/2) || y1>(DXD_MAX_INT/2) ||
 x2<-(DXD_MAX_INT/2) || x2>(DXD_MAX_INT/2) || y2<-(DXD_MAX_INT/2) || y2>(DXD_MAX_INT/2) ||
 x3<-(DXD_MAX_INT/2) || x3>(DXD_MAX_INT/2) || y3<-(DXD_MAX_INT/2) || y3>(DXD_MAX_INT/2))
 DXErrorReturn(ERROR_BAD_PARAMETER,
 "camera causes numerical overflow");
 d1 = y3-y2, d2 = y1-y3, d3 = y2-y1;
 d = x1*d1 + x2*d2 + x3*d3;
 Qx = d? (float)1.0 / (float)d : 0.0;
 d1 *= Qx, d2 *= Qx, d3 *= Qx;
 if (1) {
 if (d < 0)
 cbyte = fbyte, colors = fcolors, cstcolors = xf->fcst;
 else
 cbyte = bbyte, colors = bcolors, cstcolors = xf->bcst;
 if (!colors) goto CAT(tcb,1);
 }
 if (1) {
 if (shademe) {
 c1 = ((RGBColor *)colors) + i1;
 c2 = ((RGBColor *)colors) + i2;
 c3 = ((RGBColor *)colors) + i3;
 } else {
 if (cmap)
 c1= &(cmap[((unsigned char *)colors)[cstcolors ? 0 : v1]]), c2= &(cmap[((unsigned char *)colors)[cstcolors ? 0 : v2]]), c3= &(cmap[((unsigned char *)colors)[cstcolors ? 0 : v3]]);
 else {
 
{ 
 if (cbyte) 
 { 
 RGBByteColor *ptr=((RGBByteColor *)colors)+(cstcolors ? 0 : v1); 
 cbuf1.r = _dxd_ubyteToFloat[ptr->r]; 
 cbuf1.g = _dxd_ubyteToFloat[ptr->g]; 
 cbuf1.b = _dxd_ubyteToFloat[ptr->b]; 
 } else 
 cbuf1 = (((RGBColor *)colors)[cstcolors ? 0 : v1]); 
}; 
{ 
 if (cbyte) 
 { 
 RGBByteColor *ptr=((RGBByteColor *)colors)+(cstcolors ? 0 : v2); 
 cbuf2.r = _dxd_ubyteToFloat[ptr->r]; 
 cbuf2.g = _dxd_ubyteToFloat[ptr->g]; 
 cbuf2.b = _dxd_ubyteToFloat[ptr->b]; 
 } else 
 cbuf2 = (((RGBColor *)colors)[cstcolors ? 0 : v2]); 
}; 
{ 
 if (cbyte) 
 { 
 RGBByteColor *ptr=((RGBByteColor *)colors)+(cstcolors ? 0 : v3); 
 cbuf3.r = _dxd_ubyteToFloat[ptr->r]; 
 cbuf3.g = _dxd_ubyteToFloat[ptr->g]; 
 cbuf3.b = _dxd_ubyteToFloat[ptr->b]; 
 } else 
 cbuf3 = (((RGBColor *)colors)[cstcolors ? 0 : v3]); 
};
 c1 = &cbuf1; c2 = &cbuf2; c3 = &cbuf3;
 }
 }
 r1=c1->r, g1=c1->g, b1=c1->b;
 r2=c2->r, g2=c2->g, b2=c2->b;
 r3=c3->r, g3=c3->g, b3=c3->b;
 } else if (1) {
 if (cmap){
 r=(cmap[((unsigned char *)colors)[cstcolors ? 0 : index]]).r; g=(cmap[((unsigned char *)colors)[cstcolors ? 0 : index]]).g; b=(cmap[((unsigned char *)colors)[cstcolors ? 0 : index]]).b;
 } else {
 
{ 
 if (cbyte) 
 { 
 RGBByteColor *ptr=((RGBByteColor *)colors)+(cstcolors ? 0 : index); 
 cbuf1.r = _dxd_ubyteToFloat[ptr->r]; 
 cbuf1.g = _dxd_ubyteToFloat[ptr->g]; 
 cbuf1.b = _dxd_ubyteToFloat[ptr->b]; 
 } else 
 cbuf1 = (((RGBColor *)colors)[cstcolors ? 0 : index]); 
};
 r = cbuf1.r; g = cbuf1.g; b = cbuf1.b;
 }
 }
 if (1) {
 if (omap) o1=(omap[((unsigned char *)opacities)[xf->ocst ? 0 : v1]]), o2=(omap[((unsigned char *)opacities)[xf->ocst ? 0 : v2]]), o3=(omap[((unsigned char *)opacities)[xf->ocst ? 0 : v3]]);
 else {
 
{ 
 if (obyte) 
 o1 = _dxd_ubyteToFloat[((ubyte *)opacities)[xf->ocst ? 0 : v1]]; 
 else 
 o1 = (((float *)opacities)[xf->ocst ? 0 : v1]); 
}; 
{ 
 if (obyte) 
 o2 = _dxd_ubyteToFloat[((ubyte *)opacities)[xf->ocst ? 0 : v2]]; 
 else 
 o2 = (((float *)opacities)[xf->ocst ? 0 : v2]); 
}; 
{ 
 if (obyte) 
 o3 = _dxd_ubyteToFloat[((ubyte *)opacities)[xf->ocst ? 0 : v3]]; 
 else 
 o3 = (((float *)opacities)[xf->ocst ? 0 : v3]); 
};
 }
 } else if (opacities||omap) {
 if (omap) o=(omap[((unsigned char *)opacities)[xf->ocst ? 0 : index]]);
 else 
{ 
 if (obyte) 
 o = _dxd_ubyteToFloat[((ubyte *)opacities)[xf->ocst ? 0 : index]]; 
 else 
 o = (((float *)opacities)[xf->ocst ? 0 : index]); 
};
 obar = 1.0-o;
 } else
 o = 1.0, obar = 0.0;
 if (1) {
 dxr = (r1 == r2 && r1 == r3) ? 0.0 : d1*r1 + d2*r2 + d3*r3;
 dxg = (g1 == g2 && g1 == g3) ? 0.0 : d1*g1 + d2*g2 + d3*g3;
 dxb = (b1 == b2 && b1 == b3) ? 0.0 : d1*b1 + d2*b2 + d3*b3;
 }
 if (1) dxo = (o1 == o2 && o1 == o3) ? 0.0 : d1*o1 + d2*o2 + d3*o3;
 dxz = (z1 == z2 && z1 == z3) ? 0.0 : d1*z1 + d2*z2 + d3*z3;
 { 
 if (y2 < y1) { 
 (d=y1, y1=y2, y2=d), (d=x1, x1=x2, x2=d); 
 if (1) (d=r1, r1=r2, r2=d), (d=g1, g1=g2, g2=d), (d=b1, b1=b2, b2=d); 
 if (1) (d=o1, o1=o2, o2=d); 
 (d=z1, z1=z2, z2=d); 
 } 
}
 { 
 if (y3 < y1) { 
 (d=y1, y1=y3, y3=d), (d=x1, x1=x3, x3=d); 
 if (1) (d=r1, r1=r3, r3=d), (d=g1, g1=g3, g3=d), (d=b1, b1=b3, b3=d); 
 if (1) (d=o1, o1=o3, o3=d); 
 (d=z1, z1=z3, z3=d); 
 } 
}
 { 
 if (y3 < y2) { 
 (d=y2, y2=y3, y3=d), (d=x2, x2=x3, x3=d); 
 if (1) (d=r2, r2=r3, r3=d), (d=g2, g2=g3, g3=d), (d=b2, b2=b3, b3=d); 
 if (1) (d=o2, o2=o3, o3=d); 
 (d=z2, z2=z3, z3=d); 
 } 
}
 if (0) {
 A = B = x1;
 if (x2<A) A = x2; else B = x2;
 if (x3<A) A = x3; else if (x3>B) B = x3;
 if (A<buf->fmin.x) buf->fmin.x = A;
 if (B>buf->fmax.x) buf->fmax.x = B;
 if (y1<buf->fmin.y) buf->fmin.y = y1;
 if (y3>buf->fmax.y) buf->fmax.y = y3;
 }
 iy1 = ((int)((y1)-30000.0)+30000), iy2 = ((int)((y2)-30000.0)+30000), iy3 = ((int)((y3)-30000.0)+30000);
 if (iy1>iy2 || iy2>iy3) {
 DXSetError(ERROR_DATA_INVALID,
 "position number %d, %d, or %d is invalid", v1, v2, v3);
 return ERROR;
 }
 if (iy1==iy3)
 goto CAT(tcb,1);
 d = y3 - y1;
 Qy = d? (float)1.0 / (float)d : 0.0;
 if (1) dyr=(r3-r1)*Qy, dyg=(g3-g1)*Qy, dyb=(b3-b1)*Qy;
 if (1) dyo = (float)(o3-o1) * Qy;
 dyz = (z3-z1) * Qy;
 dyA = (float)(x3-x1) * Qy;
 A = x1;
 pp = buf->u.big + iy1*width;
 d = iy1 - y1;
 if (iy1<0)
 d-=iy1, pp=buf->u.big;
 A += dyA*d;
 if (1) r1+=dyr*d, g1+=dyg*d, b1+=dyb*d;
 if (1) o1+=dyo*d;
 z1 += dyz*d;
 nn = 0;
 { 
 
 if (iy2>height) 
 iy2 = height; 
 if (iy1<iy2 && iy2>0 && iy1<=height) { 
 B = x1; 
 d = y2 - y1; 
 d = d? (float)1.0 / (float)d : 0.0; 
 dyB = (float)(x2-x1) * d; 
 if (iy1<0) iy1=0; 
 d = iy1 - y1; 
 B += d*dyB; 
 for (iy=iy1; iy<iy2; iy++) { 
 iA = ((int)((A)-30000.0)+30000); 
 iB = ((int)((B)-30000.0)+30000); 
 if (iB>iA) left=iA, right=iB; 
 else left=iB, right=iA; 
 if (left<0) left=0; 
 if (right>width) right=width; 
 n = right - left; 
 if (n>0) { 
 nn += n; 
 p = pp+left; 
 d = left - A; 
 if (1) r=r1+d*dxr, g=g1+d*dxg, b=b1+d*dxb; 
 if (1) o=o1+d*dxo, obar=1.0-o; 
 z=z1+d*dxz; 
 while (--n>=0) { 
 if ((z>p->z)) { 
 { p->c.r=r+obar*p->c.r; p->c.g=g+obar*p->c.g; p->c.b=b+obar*p->c.b; }; 
 if (1) r+=dxr, g+=dxg, b+=dxb; 
 if (1) o+=dxo, obar=1.0-o; 
 z+=dxz; 
 p++; 
 } else { 
 if (1) r+=dxr, g+=dxg, b+=dxb; 
 if (1) o+=dxo, obar=1.0-o; 
 z+=dxz; 
 p++; 
 } 
 } 
 } 
 if (1) r1+=dyr, g1+=dyg, b1+=dyb; 
 if (1) o1+=dyo; 
 z1 += dyz; 
 A+=dyA, B+=dyB; 
 pp+=width; 
 } 
 } 
}
 { 
 
 if (iy3>height) 
 iy3 = height; 
 if (iy2<iy3 && iy3>0 && iy2<=height) { 
 B = x2; 
 d = y3 - y2; 
 d = d? (float)1.0 / (float)d : 0.0; 
 dyB = (float)(x3-x2) * d; 
 if (iy2<0) iy2=0; 
 d = iy2 - y2; 
 B += d*dyB; 
 for (iy=iy2; iy<iy3; iy++) { 
 iA = ((int)((A)-30000.0)+30000); 
 iB = ((int)((B)-30000.0)+30000); 
 if (iB>iA) left=iA, right=iB; 
 else left=iB, right=iA; 
 if (left<0) left=0; 
 if (right>width) right=width; 
 n = right - left; 
 if (n>0) { 
 nn += n; 
 p = pp+left; 
 d = left - A; 
 if (1) r=r1+d*dxr, g=g1+d*dxg, b=b1+d*dxb; 
 if (1) o=o1+d*dxo, obar=1.0-o; 
 z=z1+d*dxz; 
 while (--n>=0) { 
 if ((z>p->z)) { 
 { p->c.r=r+obar*p->c.r; p->c.g=g+obar*p->c.g; p->c.b=b+obar*p->c.b; }; 
 if (1) r+=dxr, g+=dxg, b+=dxb; 
 if (1) o+=dxo, obar=1.0-o; 
 z+=dxz; 
 p++; 
 } else { 
 if (1) r+=dxr, g+=dxg, b+=dxb; 
 if (1) o+=dxo, obar=1.0-o; 
 z+=dxz; 
 p++; 
 } 
 } 
 } 
 if (1) r1+=dyr, g1+=dyg, b1+=dyb; 
 if (1) o1+=dyo; 
 z1 += dyz; 
 A+=dyA, B+=dyB; 
 pp+=width; 
 } 
 } 
}
 buf->pixels += nn;
};
 CAT(tcb,1) :
 continue;
 }
};
    } else {
      DXSetError(ERROR_INTERNAL, "unknown pix_type %d", buf->pix_type);
      return ERROR;
    }
    return OK;
}
