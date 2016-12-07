#include <dxconfig.h>
#include <dx/dx.h>
#include "render.h"
enum type {
    type_colors,
    type_fast,
    type_big
};
static Error
shear(Pointer icstart, char cstc, RGBColor *cmap,
      Pointer iostart, char csto, float *omap,
      float cmul, float omul,
      int idx, int idy, int isx, int isy, int ikx, int iky,
      float osz, float dzdx, float dzdy, int clip,
      Pointer ocstart, enum type otype, float *oostart,
      int osx, float osy, int onx, int ony,
      int it, float scale, float skew, int ot
) {
    int odx, ody, oky;
    int x, iy, oy, dip, dop, ky;
    float f, f1, yy, df, iscale, y, z, dz;
    float r, g, b, o, r0, g0, b0, o0, r1, g1, b1, o1, s0, s1, obar;
    float *iop, *oop;
    if (it) {
 {int t=ikx; ikx=iky; iky=t;};
 {int t=idx; idx=idy; idy=t;};
 {int t=isx; isx=isy; isy=t;};
    }
    if (ot) {
 {int t=onx; onx=ony; ony=t;};
 ody = 1, odx = ony;
 {float t=dzdx; dzdx=dzdy; dzdy=t;};
    } else
 odx = 1, ody = onx;
    if (osx<0) {
 osy-=skew*osx;
 isx-=osx;
 ikx+=osx;
 osx=0;
    }
    if (onx-osx < ikx)
 ikx = onx-osx;
    oky = ony;
    if (cmap && omap) {
 unsigned char *icp;
 RGBColor *ocp;
 {if (!(otype==type_colors && oostart && iostart)) DXErrorReturn(ERROR_ASSERTION, "assertion failure")};
 
 if (((scale)<0? -(scale) : (scale)) > 1.0) { 
 { 
 
 iscale = 1.0 / scale; 
 dip = idy; 
 dop = scale>0? ody : -ody; 
 dz = scale>0? dzdy : -dzdy; 
 df = scale>0? iscale : -iscale; 
 
 for (x=0, yy=osy; x<ikx; x++, yy+=skew) { 
 oy = scale>0? ((yy)<=0 || (int)(yy)==yy? (int)(yy) : (int)(yy)+1) : ((yy)>=0 || (int)(yy)==yy? (int)(yy) : (int)(yy)-1); 
 if (oy<0) oy = 0; 
 if (oy>=oky) oy = oky-1; 
 f = (oy - yy) * iscale; 
 iy = ((f)>=0 || (int)(f)==f? (int)(f) : (int)(f)-1); 
 if (iy<0) continue; 
 if (iy>=iky) continue; 
 f = f - iy + 1.0; 
 ocp = (RGBColor *)ocstart + (x+osx)*odx + oy*ody; 
 if (1) oop = oostart + (x+osx)*odx + oy*ody; 
 ky = scale>0? oky-oy : oy+1; 
 icp = cstc ? (unsigned char *)icstart : 
 (unsigned char *)icstart + (x+isx)*idx + (iy+isy)*idy; 
 if (0) iop = csto ? (float *)iostart : 
 (float *)iostart + (x+isx)*idx + (iy+isy)*idy; 
 r1 = cmap[*icp].r * cmul, g1 = cmap[*icp].g * cmul, b1 = cmap[*icp].b * cmul; 
 o1 = omap[*icp] * omul; 
 r0 = g0 = b0 = o0 = 0; 
 z = osz + (osx+x)*dzdx + oy*dzdy; 
 while (ky>0) { 
 if (f > 1.0) { 
 f -= 1.0; 
 iy += 1; 
 if (iy>=iky) break; 
 r0 = r1, g0 = g1, b0 = b1; 
 if (1) o0 = o1; 
 if (!cstc) icp += dip; 
 if (!csto && 0) iop += dip; 
 r1 = cmap[*icp].r*cmul, g1 = cmap[*icp].g*cmul, b1 = cmap[*icp].b*cmul; 
 o1 = omap[*icp] * omul; 
 } 
 f1 = 1.0 - f; 
 if (!(1)); 
 else if (1 && !1) { 
 obar = 1.0 - (o0*f1 + o1*f); 
 ocp->r = ocp->r*obar + r0*f1 + r1*f; 
 ocp->g = ocp->g*obar + g0*f1 + g1*f; 
 ocp->b = ocp->b*obar + b0*f1 + b1*f; 
 } else { 
 ocp->r = r0*f1 + r1*f; 
 ocp->g = g0*f1 + g1*f; 
 ocp->b = b0*f1 + b1*f; 
 if (1) *oop = o0*f1 + o1*f; 
 } 
 f += df; 
 ocp += dop; 
 if (1) oop += dop; 
 z += dz; 
 ky -= 1; 
 } 
 } 
}; 
 } else { 
 { 
 
 iscale = 1.0 / scale; 
 dip = idy; 
 dop = scale>0? ody : -ody; 
 dz = scale>0? dzdy : -dzdy; 
 df = scale>0? scale : -scale; 
 
 for (x=0, yy=osy; x<ikx; x++, yy+=skew) { 
 if (scale>0) { 
 oy = ((yy)<=0 || (int)(yy)==yy? (int)(yy) : (int)(yy)+1); 
 if (oy<0) oy = 0; 
 if (oy>=oky) continue; 
 ky = oky-oy; 
 y = (oy - 1 - yy) * iscale; 
 iy = ((y)<=0 || (int)(y)==y? (int)(y) : (int)(y)+1); 
 } else { 
 oy = ((yy)>=0 || (int)(yy)==yy? (int)(yy) : (int)(yy)-1); 
 if (oy<0) continue; 
 if (oy>=oky) oy = oky-1; 
 ky = oy+1; 
 y = (oy + 1 - yy) * iscale; 
 iy = ((y)<=0 || (int)(y)==y? (int)(y) : (int)(y)+1); 
 } 
 if (iy<0) iy = 0; 
 if (iy>=iky) continue; 
 f = scale>0? oy - (yy + (iy * scale)) 
 : (yy + (iy * scale)) - oy; 
 f = 1 - f; 
 icp = cstc ? (unsigned char *)icstart : 
 (unsigned char *)icstart + (x+isx)*idx + (iy+isy)*idy; 
 if (0) iop = csto ? (float *)iostart : 
 (float *)iostart + (x+isx)*idx + (iy+isy)*idy; 
 ocp = (RGBColor *)ocstart + (x+osx)*odx + oy*ody; 
 if (1) oop = oostart + (x+osx)*odx + oy*ody; 
 r0 = r1 = g0 = g1 = b0 = b1 = o0 = o1 = s0 = s1 = 0; 
 while (f<1 && iy<iky) { 
 r = cmap[*icp].r, g = cmap[*icp].g, b = cmap[*icp].b; 
 o = omap[*icp]; 
 r0 += r * f, g0 += g * f, b0 += b * f; 
 if (1) o0 += o * f; 
 s0 += f; 
 f += df; 
 iy += 1; 
 if (! cstc) icp += dip; 
 if (!csto && 0) iop += dip; 
 } 
 f -= 1.0; 
 z = osz + (osx+x)*dzdx + oy*dzdy; 
 while (iy<iky && ky>0) { 
 r = cmap[*icp].r, g = cmap[*icp].g, b = cmap[*icp].b; 
 o = omap[*icp]; 
 f1 = (1.0 - f); 
 r0 += r*f1, g0 += g*f1, b0 += b*f1; 
 if (1) o0 += o*f1; 
 s0 += f1; 
 r1 += r*f, g1 += g*f, b1 += b*f; 
 if (1) o1 += o*f; 
 s1 += f; 
 f += df; 
 if (f >= 1.0) { 
 f -= 1.0; 
 s0 = s0? (float)1.0/s0 : 0.0; 
 if (!(1)); 
 else if (1 && !1) { 
 obar = 1.0 - o0 * s0 * omul; 
 s0 *= cmul; 
 ocp->r = ocp->r*obar + r0*s0; 
 ocp->g = ocp->g*obar + g0*s0; 
 ocp->b = ocp->b*obar + b0*s0; 
 } else { 
 if (1) *oop = o0 * s0 * omul; 
 s0 *= cmul; 
 ocp->r = r0*s0; 
 ocp->g = g0*s0; 
 ocp->b = b0*s0; 
 } 
 r0 = r1, g0 = g1, b0 = b1; 
 if (1) o0 = o1; 
 s0 = s1; 
 r1 = g1 = b1 = s1 = 0; 
 if (1) o1 = 0; 
 ocp += dop; 
 if (1) oop += dop; 
 z += dz; 
 ky -= 1; 
 } 
 iy += 1; 
 if (! cstc) icp += dip; 
 if (!csto && 0) iop += dip; 
 } 
 if (ky>0 && f>df) { 
 s0 = s0? (float)1.0/s0 : 0.0; 
 if (!(1)); 
 else if (1 && !1) { 
 obar = 1.0 - o0 * s0 * omul; 
 s0 *= cmul; 
 ocp->r = ocp->r*obar + r0*s0; 
 ocp->g = ocp->g*obar + g0*s0; 
 ocp->b = ocp->b*obar + b0*s0; 
 } else { 
 if (1) *oop = o0 * s0 * omul; 
 s0 *= cmul; 
 ocp->r = r0*s0; 
 ocp->g = g0*s0; 
 ocp->b = b0*s0; 
 } 
 } 
 } 
}; 
 }
                                ;
    } else if (cmap && !omap) {
 unsigned char *icp;
 RGBColor *ocp;
 {if (!(otype==type_colors && oostart)) DXErrorReturn(ERROR_ASSERTION, "assertion failure")};
 if (iostart) {
     
 if (((scale)<0? -(scale) : (scale)) > 1.0) { 
 { 
 
 iscale = 1.0 / scale; 
 dip = idy; 
 dop = scale>0? ody : -ody; 
 dz = scale>0? dzdy : -dzdy; 
 df = scale>0? iscale : -iscale; 
 
 for (x=0, yy=osy; x<ikx; x++, yy+=skew) { 
 oy = scale>0? ((yy)<=0 || (int)(yy)==yy? (int)(yy) : (int)(yy)+1) : ((yy)>=0 || (int)(yy)==yy? (int)(yy) : (int)(yy)-1); 
 if (oy<0) oy = 0; 
 if (oy>=oky) oy = oky-1; 
 f = (oy - yy) * iscale; 
 iy = ((f)>=0 || (int)(f)==f? (int)(f) : (int)(f)-1); 
 if (iy<0) continue; 
 if (iy>=iky) continue; 
 f = f - iy + 1.0; 
 ocp = (RGBColor *)ocstart + (x+osx)*odx + oy*ody; 
 if (1) oop = oostart + (x+osx)*odx + oy*ody; 
 ky = scale>0? oky-oy : oy+1; 
 icp = cstc ? (unsigned char *)icstart : 
 (unsigned char *)icstart + (x+isx)*idx + (iy+isy)*idy; 
 if (1) iop = csto ? (float *)iostart : 
 (float *)iostart + (x+isx)*idx + (iy+isy)*idy; 
 r1 = cmap[*icp].r * cmul, g1 = cmap[*icp].g * cmul, b1 = cmap[*icp].b * cmul; 
 o1 = *iop * omul; 
 r0 = g0 = b0 = o0 = 0; 
 z = osz + (osx+x)*dzdx + oy*dzdy; 
 while (ky>0) { 
 if (f > 1.0) { 
 f -= 1.0; 
 iy += 1; 
 if (iy>=iky) break; 
 r0 = r1, g0 = g1, b0 = b1; 
 if (1) o0 = o1; 
 if (!cstc) icp += dip; 
 if (!csto && 1) iop += dip; 
 r1 = cmap[*icp].r*cmul, g1 = cmap[*icp].g*cmul, b1 = cmap[*icp].b*cmul; 
 o1 = *iop * omul; 
 } 
 f1 = 1.0 - f; 
 if (!(1)); 
 else if (1 && !1) { 
 obar = 1.0 - (o0*f1 + o1*f); 
 ocp->r = ocp->r*obar + r0*f1 + r1*f; 
 ocp->g = ocp->g*obar + g0*f1 + g1*f; 
 ocp->b = ocp->b*obar + b0*f1 + b1*f; 
 } else { 
 ocp->r = r0*f1 + r1*f; 
 ocp->g = g0*f1 + g1*f; 
 ocp->b = b0*f1 + b1*f; 
 if (1) *oop = o0*f1 + o1*f; 
 } 
 f += df; 
 ocp += dop; 
 if (1) oop += dop; 
 z += dz; 
 ky -= 1; 
 } 
 } 
}; 
 } else { 
 { 
 
 iscale = 1.0 / scale; 
 dip = idy; 
 dop = scale>0? ody : -ody; 
 dz = scale>0? dzdy : -dzdy; 
 df = scale>0? scale : -scale; 
 
 for (x=0, yy=osy; x<ikx; x++, yy+=skew) { 
 if (scale>0) { 
 oy = ((yy)<=0 || (int)(yy)==yy? (int)(yy) : (int)(yy)+1); 
 if (oy<0) oy = 0; 
 if (oy>=oky) continue; 
 ky = oky-oy; 
 y = (oy - 1 - yy) * iscale; 
 iy = ((y)<=0 || (int)(y)==y? (int)(y) : (int)(y)+1); 
 } else { 
 oy = ((yy)>=0 || (int)(yy)==yy? (int)(yy) : (int)(yy)-1); 
 if (oy<0) continue; 
 if (oy>=oky) oy = oky-1; 
 ky = oy+1; 
 y = (oy + 1 - yy) * iscale; 
 iy = ((y)<=0 || (int)(y)==y? (int)(y) : (int)(y)+1); 
 } 
 if (iy<0) iy = 0; 
 if (iy>=iky) continue; 
 f = scale>0? oy - (yy + (iy * scale)) 
 : (yy + (iy * scale)) - oy; 
 f = 1 - f; 
 icp = cstc ? (unsigned char *)icstart : 
 (unsigned char *)icstart + (x+isx)*idx + (iy+isy)*idy; 
 if (1) iop = csto ? (float *)iostart : 
 (float *)iostart + (x+isx)*idx + (iy+isy)*idy; 
 ocp = (RGBColor *)ocstart + (x+osx)*odx + oy*ody; 
 if (1) oop = oostart + (x+osx)*odx + oy*ody; 
 r0 = r1 = g0 = g1 = b0 = b1 = o0 = o1 = s0 = s1 = 0; 
 while (f<1 && iy<iky) { 
 r = cmap[*icp].r, g = cmap[*icp].g, b = cmap[*icp].b; 
 o = *iop; 
 r0 += r * f, g0 += g * f, b0 += b * f; 
 if (1) o0 += o * f; 
 s0 += f; 
 f += df; 
 iy += 1; 
 if (! cstc) icp += dip; 
 if (!csto && 1) iop += dip; 
 } 
 f -= 1.0; 
 z = osz + (osx+x)*dzdx + oy*dzdy; 
 while (iy<iky && ky>0) { 
 r = cmap[*icp].r, g = cmap[*icp].g, b = cmap[*icp].b; 
 o = *iop; 
 f1 = (1.0 - f); 
 r0 += r*f1, g0 += g*f1, b0 += b*f1; 
 if (1) o0 += o*f1; 
 s0 += f1; 
 r1 += r*f, g1 += g*f, b1 += b*f; 
 if (1) o1 += o*f; 
 s1 += f; 
 f += df; 
 if (f >= 1.0) { 
 f -= 1.0; 
 s0 = s0? (float)1.0/s0 : 0.0; 
 if (!(1)); 
 else if (1 && !1) { 
 obar = 1.0 - o0 * s0 * omul; 
 s0 *= cmul; 
 ocp->r = ocp->r*obar + r0*s0; 
 ocp->g = ocp->g*obar + g0*s0; 
 ocp->b = ocp->b*obar + b0*s0; 
 } else { 
 if (1) *oop = o0 * s0 * omul; 
 s0 *= cmul; 
 ocp->r = r0*s0; 
 ocp->g = g0*s0; 
 ocp->b = b0*s0; 
 } 
 r0 = r1, g0 = g1, b0 = b1; 
 if (1) o0 = o1; 
 s0 = s1; 
 r1 = g1 = b1 = s1 = 0; 
 if (1) o1 = 0; 
 ocp += dop; 
 if (1) oop += dop; 
 z += dz; 
 ky -= 1; 
 } 
 iy += 1; 
 if (! cstc) icp += dip; 
 if (!csto && 1) iop += dip; 
 } 
 if (ky>0 && f>df) { 
 s0 = s0? (float)1.0/s0 : 0.0; 
 if (!(1)); 
 else if (1 && !1) { 
 obar = 1.0 - o0 * s0 * omul; 
 s0 *= cmul; 
 ocp->r = ocp->r*obar + r0*s0; 
 ocp->g = ocp->g*obar + g0*s0; 
 ocp->b = ocp->b*obar + b0*s0; 
 } else { 
 if (1) *oop = o0 * s0 * omul; 
 s0 *= cmul; 
 ocp->r = r0*s0; 
 ocp->g = g0*s0; 
 ocp->b = b0*s0; 
 } 
 } 
 } 
}; 
 }
                             ;
 } else {
     
 if (((scale)<0? -(scale) : (scale)) > 1.0) { 
 { 
 
 iscale = 1.0 / scale; 
 dip = idy; 
 dop = scale>0? ody : -ody; 
 dz = scale>0? dzdy : -dzdy; 
 df = scale>0? iscale : -iscale; 
 
 for (x=0, yy=osy; x<ikx; x++, yy+=skew) { 
 oy = scale>0? ((yy)<=0 || (int)(yy)==yy? (int)(yy) : (int)(yy)+1) : ((yy)>=0 || (int)(yy)==yy? (int)(yy) : (int)(yy)-1); 
 if (oy<0) oy = 0; 
 if (oy>=oky) oy = oky-1; 
 f = (oy - yy) * iscale; 
 iy = ((f)>=0 || (int)(f)==f? (int)(f) : (int)(f)-1); 
 if (iy<0) continue; 
 if (iy>=iky) continue; 
 f = f - iy + 1.0; 
 ocp = (RGBColor *)ocstart + (x+osx)*odx + oy*ody; 
 if (1) oop = oostart + (x+osx)*odx + oy*ody; 
 ky = scale>0? oky-oy : oy+1; 
 icp = cstc ? (unsigned char *)icstart : 
 (unsigned char *)icstart + (x+isx)*idx + (iy+isy)*idy; 
 if (0) iop = csto ? (float *)iostart : 
 (float *)iostart + (x+isx)*idx + (iy+isy)*idy; 
 r1 = cmap[*icp].r * cmul, g1 = cmap[*icp].g * cmul, b1 = cmap[*icp].b * cmul; 
 o1 = 1 * omul; 
 r0 = g0 = b0 = o0 = 0; 
 z = osz + (osx+x)*dzdx + oy*dzdy; 
 while (ky>0) { 
 if (f > 1.0) { 
 f -= 1.0; 
 iy += 1; 
 if (iy>=iky) break; 
 r0 = r1, g0 = g1, b0 = b1; 
 if (1) o0 = o1; 
 if (!cstc) icp += dip; 
 if (!csto && 0) iop += dip; 
 r1 = cmap[*icp].r*cmul, g1 = cmap[*icp].g*cmul, b1 = cmap[*icp].b*cmul; 
 o1 = 1 * omul; 
 } 
 f1 = 1.0 - f; 
 if (!(1)); 
 else if (1 && !1) { 
 obar = 1.0 - (o0*f1 + o1*f); 
 ocp->r = ocp->r*obar + r0*f1 + r1*f; 
 ocp->g = ocp->g*obar + g0*f1 + g1*f; 
 ocp->b = ocp->b*obar + b0*f1 + b1*f; 
 } else { 
 ocp->r = r0*f1 + r1*f; 
 ocp->g = g0*f1 + g1*f; 
 ocp->b = b0*f1 + b1*f; 
 if (1) *oop = o0*f1 + o1*f; 
 } 
 f += df; 
 ocp += dop; 
 if (1) oop += dop; 
 z += dz; 
 ky -= 1; 
 } 
 } 
}; 
 } else { 
 { 
 
 iscale = 1.0 / scale; 
 dip = idy; 
 dop = scale>0? ody : -ody; 
 dz = scale>0? dzdy : -dzdy; 
 df = scale>0? scale : -scale; 
 
 for (x=0, yy=osy; x<ikx; x++, yy+=skew) { 
 if (scale>0) { 
 oy = ((yy)<=0 || (int)(yy)==yy? (int)(yy) : (int)(yy)+1); 
 if (oy<0) oy = 0; 
 if (oy>=oky) continue; 
 ky = oky-oy; 
 y = (oy - 1 - yy) * iscale; 
 iy = ((y)<=0 || (int)(y)==y? (int)(y) : (int)(y)+1); 
 } else { 
 oy = ((yy)>=0 || (int)(yy)==yy? (int)(yy) : (int)(yy)-1); 
 if (oy<0) continue; 
 if (oy>=oky) oy = oky-1; 
 ky = oy+1; 
 y = (oy + 1 - yy) * iscale; 
 iy = ((y)<=0 || (int)(y)==y? (int)(y) : (int)(y)+1); 
 } 
 if (iy<0) iy = 0; 
 if (iy>=iky) continue; 
 f = scale>0? oy - (yy + (iy * scale)) 
 : (yy + (iy * scale)) - oy; 
 f = 1 - f; 
 icp = cstc ? (unsigned char *)icstart : 
 (unsigned char *)icstart + (x+isx)*idx + (iy+isy)*idy; 
 if (0) iop = csto ? (float *)iostart : 
 (float *)iostart + (x+isx)*idx + (iy+isy)*idy; 
 ocp = (RGBColor *)ocstart + (x+osx)*odx + oy*ody; 
 if (1) oop = oostart + (x+osx)*odx + oy*ody; 
 r0 = r1 = g0 = g1 = b0 = b1 = o0 = o1 = s0 = s1 = 0; 
 while (f<1 && iy<iky) { 
 r = cmap[*icp].r, g = cmap[*icp].g, b = cmap[*icp].b; 
 o = 1; 
 r0 += r * f, g0 += g * f, b0 += b * f; 
 if (1) o0 += o * f; 
 s0 += f; 
 f += df; 
 iy += 1; 
 if (! cstc) icp += dip; 
 if (!csto && 0) iop += dip; 
 } 
 f -= 1.0; 
 z = osz + (osx+x)*dzdx + oy*dzdy; 
 while (iy<iky && ky>0) { 
 r = cmap[*icp].r, g = cmap[*icp].g, b = cmap[*icp].b; 
 o = 1; 
 f1 = (1.0 - f); 
 r0 += r*f1, g0 += g*f1, b0 += b*f1; 
 if (1) o0 += o*f1; 
 s0 += f1; 
 r1 += r*f, g1 += g*f, b1 += b*f; 
 if (1) o1 += o*f; 
 s1 += f; 
 f += df; 
 if (f >= 1.0) { 
 f -= 1.0; 
 s0 = s0? (float)1.0/s0 : 0.0; 
 if (!(1)); 
 else if (1 && !1) { 
 obar = 1.0 - o0 * s0 * omul; 
 s0 *= cmul; 
 ocp->r = ocp->r*obar + r0*s0; 
 ocp->g = ocp->g*obar + g0*s0; 
 ocp->b = ocp->b*obar + b0*s0; 
 } else { 
 if (1) *oop = o0 * s0 * omul; 
 s0 *= cmul; 
 ocp->r = r0*s0; 
 ocp->g = g0*s0; 
 ocp->b = b0*s0; 
 } 
 r0 = r1, g0 = g1, b0 = b1; 
 if (1) o0 = o1; 
 s0 = s1; 
 r1 = g1 = b1 = s1 = 0; 
 if (1) o1 = 0; 
 ocp += dop; 
 if (1) oop += dop; 
 z += dz; 
 ky -= 1; 
 } 
 iy += 1; 
 if (! cstc) icp += dip; 
 if (!csto && 0) iop += dip; 
 } 
 if (ky>0 && f>df) { 
 s0 = s0? (float)1.0/s0 : 0.0; 
 if (!(1)); 
 else if (1 && !1) { 
 obar = 1.0 - o0 * s0 * omul; 
 s0 *= cmul; 
 ocp->r = ocp->r*obar + r0*s0; 
 ocp->g = ocp->g*obar + g0*s0; 
 ocp->b = ocp->b*obar + b0*s0; 
 } else { 
 if (1) *oop = o0 * s0 * omul; 
 s0 *= cmul; 
 ocp->r = r0*s0; 
 ocp->g = g0*s0; 
 ocp->b = b0*s0; 
 } 
 } 
 } 
}; 
 }
                             ;
 }
    } else if (otype==type_colors) {
 { 
 RGBColor *icp; 
 RGBColor *ocp; 
 if (iostart && oostart) { 
 
 if (((scale)<0? -(scale) : (scale)) > 1.0) { 
 { 
 
 iscale = 1.0 / scale; 
 dip = idy; 
 dop = scale>0? ody : -ody; 
 dz = scale>0? dzdy : -dzdy; 
 df = scale>0? iscale : -iscale; 
 
 for (x=0, yy=osy; x<ikx; x++, yy+=skew) { 
 oy = scale>0? ((yy)<=0 || (int)(yy)==yy? (int)(yy) : (int)(yy)+1) : ((yy)>=0 || (int)(yy)==yy? (int)(yy) : (int)(yy)-1); 
 if (oy<0) oy = 0; 
 if (oy>=oky) oy = oky-1; 
 f = (oy - yy) * iscale; 
 iy = ((f)>=0 || (int)(f)==f? (int)(f) : (int)(f)-1); 
 if (iy<0) continue; 
 if (iy>=iky) continue; 
 f = f - iy + 1.0; 
 ocp = (RGBColor *)ocstart + (x+osx)*odx + oy*ody; 
 if (1) oop = oostart + (x+osx)*odx + oy*ody; 
 ky = scale>0? oky-oy : oy+1; 
 icp = cstc ? (RGBColor *)icstart : 
 (RGBColor *)icstart + (x+isx)*idx + (iy+isy)*idy; 
 if (1) iop = csto ? (float *)iostart : 
 (float *)iostart + (x+isx)*idx + (iy+isy)*idy; 
 r1 = icp->r * cmul, g1 = icp->g * cmul, b1 = icp->b * cmul; 
 o1 = *iop * omul; 
 r0 = g0 = b0 = o0 = 0; 
 z = osz + (osx+x)*dzdx + oy*dzdy; 
 while (ky>0) { 
 if (f > 1.0) { 
 f -= 1.0; 
 iy += 1; 
 if (iy>=iky) break; 
 r0 = r1, g0 = g1, b0 = b1; 
 if (1) o0 = o1; 
 if (!cstc) icp += dip; 
 if (!csto && 1) iop += dip; 
 r1 = icp->r*cmul, g1 = icp->g*cmul, b1 = icp->b*cmul; 
 o1 = *iop * omul; 
 } 
 f1 = 1.0 - f; 
 if (!(1)); 
 else if (1 && !1) { 
 obar = 1.0 - (o0*f1 + o1*f); 
 ocp->r = ocp->r*obar + r0*f1 + r1*f; 
 ocp->g = ocp->g*obar + g0*f1 + g1*f; 
 ocp->b = ocp->b*obar + b0*f1 + b1*f; 
 } else { 
 ocp->r = r0*f1 + r1*f; 
 ocp->g = g0*f1 + g1*f; 
 ocp->b = b0*f1 + b1*f; 
 if (1) *oop = o0*f1 + o1*f; 
 } 
 f += df; 
 ocp += dop; 
 if (1) oop += dop; 
 z += dz; 
 ky -= 1; 
 } 
 } 
}; 
 } else { 
 { 
 
 iscale = 1.0 / scale; 
 dip = idy; 
 dop = scale>0? ody : -ody; 
 dz = scale>0? dzdy : -dzdy; 
 df = scale>0? scale : -scale; 
 
 for (x=0, yy=osy; x<ikx; x++, yy+=skew) { 
 if (scale>0) { 
 oy = ((yy)<=0 || (int)(yy)==yy? (int)(yy) : (int)(yy)+1); 
 if (oy<0) oy = 0; 
 if (oy>=oky) continue; 
 ky = oky-oy; 
 y = (oy - 1 - yy) * iscale; 
 iy = ((y)<=0 || (int)(y)==y? (int)(y) : (int)(y)+1); 
 } else { 
 oy = ((yy)>=0 || (int)(yy)==yy? (int)(yy) : (int)(yy)-1); 
 if (oy<0) continue; 
 if (oy>=oky) oy = oky-1; 
 ky = oy+1; 
 y = (oy + 1 - yy) * iscale; 
 iy = ((y)<=0 || (int)(y)==y? (int)(y) : (int)(y)+1); 
 } 
 if (iy<0) iy = 0; 
 if (iy>=iky) continue; 
 f = scale>0? oy - (yy + (iy * scale)) 
 : (yy + (iy * scale)) - oy; 
 f = 1 - f; 
 icp = cstc ? (RGBColor *)icstart : 
 (RGBColor *)icstart + (x+isx)*idx + (iy+isy)*idy; 
 if (1) iop = csto ? (float *)iostart : 
 (float *)iostart + (x+isx)*idx + (iy+isy)*idy; 
 ocp = (RGBColor *)ocstart + (x+osx)*odx + oy*ody; 
 if (1) oop = oostart + (x+osx)*odx + oy*ody; 
 r0 = r1 = g0 = g1 = b0 = b1 = o0 = o1 = s0 = s1 = 0; 
 while (f<1 && iy<iky) { 
 r = icp->r, g = icp->g, b = icp->b; 
 o = *iop; 
 r0 += r * f, g0 += g * f, b0 += b * f; 
 if (1) o0 += o * f; 
 s0 += f; 
 f += df; 
 iy += 1; 
 if (! cstc) icp += dip; 
 if (!csto && 1) iop += dip; 
 } 
 f -= 1.0; 
 z = osz + (osx+x)*dzdx + oy*dzdy; 
 while (iy<iky && ky>0) { 
 r = icp->r, g = icp->g, b = icp->b; 
 o = *iop; 
 f1 = (1.0 - f); 
 r0 += r*f1, g0 += g*f1, b0 += b*f1; 
 if (1) o0 += o*f1; 
 s0 += f1; 
 r1 += r*f, g1 += g*f, b1 += b*f; 
 if (1) o1 += o*f; 
 s1 += f; 
 f += df; 
 if (f >= 1.0) { 
 f -= 1.0; 
 s0 = s0? (float)1.0/s0 : 0.0; 
 if (!(1)); 
 else if (1 && !1) { 
 obar = 1.0 - o0 * s0 * omul; 
 s0 *= cmul; 
 ocp->r = ocp->r*obar + r0*s0; 
 ocp->g = ocp->g*obar + g0*s0; 
 ocp->b = ocp->b*obar + b0*s0; 
 } else { 
 if (1) *oop = o0 * s0 * omul; 
 s0 *= cmul; 
 ocp->r = r0*s0; 
 ocp->g = g0*s0; 
 ocp->b = b0*s0; 
 } 
 r0 = r1, g0 = g1, b0 = b1; 
 if (1) o0 = o1; 
 s0 = s1; 
 r1 = g1 = b1 = s1 = 0; 
 if (1) o1 = 0; 
 ocp += dop; 
 if (1) oop += dop; 
 z += dz; 
 ky -= 1; 
 } 
 iy += 1; 
 if (! cstc) icp += dip; 
 if (!csto && 1) iop += dip; 
 } 
 if (ky>0 && f>df) { 
 s0 = s0? (float)1.0/s0 : 0.0; 
 if (!(1)); 
 else if (1 && !1) { 
 obar = 1.0 - o0 * s0 * omul; 
 s0 *= cmul; 
 ocp->r = ocp->r*obar + r0*s0; 
 ocp->g = ocp->g*obar + g0*s0; 
 ocp->b = ocp->b*obar + b0*s0; 
 } else { 
 if (1) *oop = o0 * s0 * omul; 
 s0 *= cmul; 
 ocp->r = r0*s0; 
 ocp->g = g0*s0; 
 ocp->b = b0*s0; 
 } 
 } 
 } 
}; 
 }; 
 } else if (iostart) { 
 
 if (((scale)<0? -(scale) : (scale)) > 1.0) { 
 { 
 
 iscale = 1.0 / scale; 
 dip = idy; 
 dop = scale>0? ody : -ody; 
 dz = scale>0? dzdy : -dzdy; 
 df = scale>0? iscale : -iscale; 
 
 for (x=0, yy=osy; x<ikx; x++, yy+=skew) { 
 oy = scale>0? ((yy)<=0 || (int)(yy)==yy? (int)(yy) : (int)(yy)+1) : ((yy)>=0 || (int)(yy)==yy? (int)(yy) : (int)(yy)-1); 
 if (oy<0) oy = 0; 
 if (oy>=oky) oy = oky-1; 
 f = (oy - yy) * iscale; 
 iy = ((f)>=0 || (int)(f)==f? (int)(f) : (int)(f)-1); 
 if (iy<0) continue; 
 if (iy>=iky) continue; 
 f = f - iy + 1.0; 
 ocp = (RGBColor *)ocstart + (x+osx)*odx + oy*ody; 
 if (0) oop = oostart + (x+osx)*odx + oy*ody; 
 ky = scale>0? oky-oy : oy+1; 
 icp = cstc ? (RGBColor *)icstart : 
 (RGBColor *)icstart + (x+isx)*idx + (iy+isy)*idy; 
 if (1) iop = csto ? (float *)iostart : 
 (float *)iostart + (x+isx)*idx + (iy+isy)*idy; 
 r1 = icp->r * cmul, g1 = icp->g * cmul, b1 = icp->b * cmul; 
 o1 = *iop * omul; 
 r0 = g0 = b0 = o0 = 0; 
 z = osz + (osx+x)*dzdx + oy*dzdy; 
 while (ky>0) { 
 if (f > 1.0) { 
 f -= 1.0; 
 iy += 1; 
 if (iy>=iky) break; 
 r0 = r1, g0 = g1, b0 = b1; 
 if (1) o0 = o1; 
 if (!cstc) icp += dip; 
 if (!csto && 1) iop += dip; 
 r1 = icp->r*cmul, g1 = icp->g*cmul, b1 = icp->b*cmul; 
 o1 = *iop * omul; 
 } 
 f1 = 1.0 - f; 
 if (!(1)); 
 else if (1 && !0) { 
 obar = 1.0 - (o0*f1 + o1*f); 
 ocp->r = ocp->r*obar + r0*f1 + r1*f; 
 ocp->g = ocp->g*obar + g0*f1 + g1*f; 
 ocp->b = ocp->b*obar + b0*f1 + b1*f; 
 } else { 
 ocp->r = r0*f1 + r1*f; 
 ocp->g = g0*f1 + g1*f; 
 ocp->b = b0*f1 + b1*f; 
 if (0) *oop = o0*f1 + o1*f; 
 } 
 f += df; 
 ocp += dop; 
 if (0) oop += dop; 
 z += dz; 
 ky -= 1; 
 } 
 } 
}; 
 } else { 
 { 
 
 iscale = 1.0 / scale; 
 dip = idy; 
 dop = scale>0? ody : -ody; 
 dz = scale>0? dzdy : -dzdy; 
 df = scale>0? scale : -scale; 
 
 for (x=0, yy=osy; x<ikx; x++, yy+=skew) { 
 if (scale>0) { 
 oy = ((yy)<=0 || (int)(yy)==yy? (int)(yy) : (int)(yy)+1); 
 if (oy<0) oy = 0; 
 if (oy>=oky) continue; 
 ky = oky-oy; 
 y = (oy - 1 - yy) * iscale; 
 iy = ((y)<=0 || (int)(y)==y? (int)(y) : (int)(y)+1); 
 } else { 
 oy = ((yy)>=0 || (int)(yy)==yy? (int)(yy) : (int)(yy)-1); 
 if (oy<0) continue; 
 if (oy>=oky) oy = oky-1; 
 ky = oy+1; 
 y = (oy + 1 - yy) * iscale; 
 iy = ((y)<=0 || (int)(y)==y? (int)(y) : (int)(y)+1); 
 } 
 if (iy<0) iy = 0; 
 if (iy>=iky) continue; 
 f = scale>0? oy - (yy + (iy * scale)) 
 : (yy + (iy * scale)) - oy; 
 f = 1 - f; 
 icp = cstc ? (RGBColor *)icstart : 
 (RGBColor *)icstart + (x+isx)*idx + (iy+isy)*idy; 
 if (1) iop = csto ? (float *)iostart : 
 (float *)iostart + (x+isx)*idx + (iy+isy)*idy; 
 ocp = (RGBColor *)ocstart + (x+osx)*odx + oy*ody; 
 if (0) oop = oostart + (x+osx)*odx + oy*ody; 
 r0 = r1 = g0 = g1 = b0 = b1 = o0 = o1 = s0 = s1 = 0; 
 while (f<1 && iy<iky) { 
 r = icp->r, g = icp->g, b = icp->b; 
 o = *iop; 
 r0 += r * f, g0 += g * f, b0 += b * f; 
 if (1) o0 += o * f; 
 s0 += f; 
 f += df; 
 iy += 1; 
 if (! cstc) icp += dip; 
 if (!csto && 1) iop += dip; 
 } 
 f -= 1.0; 
 z = osz + (osx+x)*dzdx + oy*dzdy; 
 while (iy<iky && ky>0) { 
 r = icp->r, g = icp->g, b = icp->b; 
 o = *iop; 
 f1 = (1.0 - f); 
 r0 += r*f1, g0 += g*f1, b0 += b*f1; 
 if (1) o0 += o*f1; 
 s0 += f1; 
 r1 += r*f, g1 += g*f, b1 += b*f; 
 if (1) o1 += o*f; 
 s1 += f; 
 f += df; 
 if (f >= 1.0) { 
 f -= 1.0; 
 s0 = s0? (float)1.0/s0 : 0.0; 
 if (!(1)); 
 else if (1 && !0) { 
 obar = 1.0 - o0 * s0 * omul; 
 s0 *= cmul; 
 ocp->r = ocp->r*obar + r0*s0; 
 ocp->g = ocp->g*obar + g0*s0; 
 ocp->b = ocp->b*obar + b0*s0; 
 } else { 
 if (0) *oop = o0 * s0 * omul; 
 s0 *= cmul; 
 ocp->r = r0*s0; 
 ocp->g = g0*s0; 
 ocp->b = b0*s0; 
 } 
 r0 = r1, g0 = g1, b0 = b1; 
 if (1) o0 = o1; 
 s0 = s1; 
 r1 = g1 = b1 = s1 = 0; 
 if (1) o1 = 0; 
 ocp += dop; 
 if (0) oop += dop; 
 z += dz; 
 ky -= 1; 
 } 
 iy += 1; 
 if (! cstc) icp += dip; 
 if (!csto && 1) iop += dip; 
 } 
 if (ky>0 && f>df) { 
 s0 = s0? (float)1.0/s0 : 0.0; 
 if (!(1)); 
 else if (1 && !0) { 
 obar = 1.0 - o0 * s0 * omul; 
 s0 *= cmul; 
 ocp->r = ocp->r*obar + r0*s0; 
 ocp->g = ocp->g*obar + g0*s0; 
 ocp->b = ocp->b*obar + b0*s0; 
 } else { 
 if (0) *oop = o0 * s0 * omul; 
 s0 *= cmul; 
 ocp->r = r0*s0; 
 ocp->g = g0*s0; 
 ocp->b = b0*s0; 
 } 
 } 
 } 
}; 
 }; 
 } else if (oostart) { 
 
 if (((scale)<0? -(scale) : (scale)) > 1.0) { 
 { 
 
 iscale = 1.0 / scale; 
 dip = idy; 
 dop = scale>0? ody : -ody; 
 dz = scale>0? dzdy : -dzdy; 
 df = scale>0? iscale : -iscale; 
 
 for (x=0, yy=osy; x<ikx; x++, yy+=skew) { 
 oy = scale>0? ((yy)<=0 || (int)(yy)==yy? (int)(yy) : (int)(yy)+1) : ((yy)>=0 || (int)(yy)==yy? (int)(yy) : (int)(yy)-1); 
 if (oy<0) oy = 0; 
 if (oy>=oky) oy = oky-1; 
 f = (oy - yy) * iscale; 
 iy = ((f)>=0 || (int)(f)==f? (int)(f) : (int)(f)-1); 
 if (iy<0) continue; 
 if (iy>=iky) continue; 
 f = f - iy + 1.0; 
 ocp = (RGBColor *)ocstart + (x+osx)*odx + oy*ody; 
 if (1) oop = oostart + (x+osx)*odx + oy*ody; 
 ky = scale>0? oky-oy : oy+1; 
 icp = cstc ? (RGBColor *)icstart : 
 (RGBColor *)icstart + (x+isx)*idx + (iy+isy)*idy; 
 if (0) iop = csto ? (float *)iostart : 
 (float *)iostart + (x+isx)*idx + (iy+isy)*idy; 
 r1 = icp->r * cmul, g1 = icp->g * cmul, b1 = icp->b * cmul; 
 o1 = 1 * omul; 
 r0 = g0 = b0 = o0 = 0; 
 z = osz + (osx+x)*dzdx + oy*dzdy; 
 while (ky>0) { 
 if (f > 1.0) { 
 f -= 1.0; 
 iy += 1; 
 if (iy>=iky) break; 
 r0 = r1, g0 = g1, b0 = b1; 
 if (1) o0 = o1; 
 if (!cstc) icp += dip; 
 if (!csto && 0) iop += dip; 
 r1 = icp->r*cmul, g1 = icp->g*cmul, b1 = icp->b*cmul; 
 o1 = 1 * omul; 
 } 
 f1 = 1.0 - f; 
 if (!(1)); 
 else if (1 && !1) { 
 obar = 1.0 - (o0*f1 + o1*f); 
 ocp->r = ocp->r*obar + r0*f1 + r1*f; 
 ocp->g = ocp->g*obar + g0*f1 + g1*f; 
 ocp->b = ocp->b*obar + b0*f1 + b1*f; 
 } else { 
 ocp->r = r0*f1 + r1*f; 
 ocp->g = g0*f1 + g1*f; 
 ocp->b = b0*f1 + b1*f; 
 if (1) *oop = o0*f1 + o1*f; 
 } 
 f += df; 
 ocp += dop; 
 if (1) oop += dop; 
 z += dz; 
 ky -= 1; 
 } 
 } 
}; 
 } else { 
 { 
 
 iscale = 1.0 / scale; 
 dip = idy; 
 dop = scale>0? ody : -ody; 
 dz = scale>0? dzdy : -dzdy; 
 df = scale>0? scale : -scale; 
 
 for (x=0, yy=osy; x<ikx; x++, yy+=skew) { 
 if (scale>0) { 
 oy = ((yy)<=0 || (int)(yy)==yy? (int)(yy) : (int)(yy)+1); 
 if (oy<0) oy = 0; 
 if (oy>=oky) continue; 
 ky = oky-oy; 
 y = (oy - 1 - yy) * iscale; 
 iy = ((y)<=0 || (int)(y)==y? (int)(y) : (int)(y)+1); 
 } else { 
 oy = ((yy)>=0 || (int)(yy)==yy? (int)(yy) : (int)(yy)-1); 
 if (oy<0) continue; 
 if (oy>=oky) oy = oky-1; 
 ky = oy+1; 
 y = (oy + 1 - yy) * iscale; 
 iy = ((y)<=0 || (int)(y)==y? (int)(y) : (int)(y)+1); 
 } 
 if (iy<0) iy = 0; 
 if (iy>=iky) continue; 
 f = scale>0? oy - (yy + (iy * scale)) 
 : (yy + (iy * scale)) - oy; 
 f = 1 - f; 
 icp = cstc ? (RGBColor *)icstart : 
 (RGBColor *)icstart + (x+isx)*idx + (iy+isy)*idy; 
 if (0) iop = csto ? (float *)iostart : 
 (float *)iostart + (x+isx)*idx + (iy+isy)*idy; 
 ocp = (RGBColor *)ocstart + (x+osx)*odx + oy*ody; 
 if (1) oop = oostart + (x+osx)*odx + oy*ody; 
 r0 = r1 = g0 = g1 = b0 = b1 = o0 = o1 = s0 = s1 = 0; 
 while (f<1 && iy<iky) { 
 r = icp->r, g = icp->g, b = icp->b; 
 o = 1; 
 r0 += r * f, g0 += g * f, b0 += b * f; 
 if (1) o0 += o * f; 
 s0 += f; 
 f += df; 
 iy += 1; 
 if (! cstc) icp += dip; 
 if (!csto && 0) iop += dip; 
 } 
 f -= 1.0; 
 z = osz + (osx+x)*dzdx + oy*dzdy; 
 while (iy<iky && ky>0) { 
 r = icp->r, g = icp->g, b = icp->b; 
 o = 1; 
 f1 = (1.0 - f); 
 r0 += r*f1, g0 += g*f1, b0 += b*f1; 
 if (1) o0 += o*f1; 
 s0 += f1; 
 r1 += r*f, g1 += g*f, b1 += b*f; 
 if (1) o1 += o*f; 
 s1 += f; 
 f += df; 
 if (f >= 1.0) { 
 f -= 1.0; 
 s0 = s0? (float)1.0/s0 : 0.0; 
 if (!(1)); 
 else if (1 && !1) { 
 obar = 1.0 - o0 * s0 * omul; 
 s0 *= cmul; 
 ocp->r = ocp->r*obar + r0*s0; 
 ocp->g = ocp->g*obar + g0*s0; 
 ocp->b = ocp->b*obar + b0*s0; 
 } else { 
 if (1) *oop = o0 * s0 * omul; 
 s0 *= cmul; 
 ocp->r = r0*s0; 
 ocp->g = g0*s0; 
 ocp->b = b0*s0; 
 } 
 r0 = r1, g0 = g1, b0 = b1; 
 if (1) o0 = o1; 
 s0 = s1; 
 r1 = g1 = b1 = s1 = 0; 
 if (1) o1 = 0; 
 ocp += dop; 
 if (1) oop += dop; 
 z += dz; 
 ky -= 1; 
 } 
 iy += 1; 
 if (! cstc) icp += dip; 
 if (!csto && 0) iop += dip; 
 } 
 if (ky>0 && f>df) { 
 s0 = s0? (float)1.0/s0 : 0.0; 
 if (!(1)); 
 else if (1 && !1) { 
 obar = 1.0 - o0 * s0 * omul; 
 s0 *= cmul; 
 ocp->r = ocp->r*obar + r0*s0; 
 ocp->g = ocp->g*obar + g0*s0; 
 ocp->b = ocp->b*obar + b0*s0; 
 } else { 
 if (1) *oop = o0 * s0 * omul; 
 s0 *= cmul; 
 ocp->r = r0*s0; 
 ocp->g = g0*s0; 
 ocp->b = b0*s0; 
 } 
 } 
 } 
}; 
 }; 
 } else { 
 {if (!(0)) DXErrorReturn(ERROR_ASSERTION, "assertion failure")}; 
 
 
 
 } 
};
    } else if (otype==type_fast) {
 {if (!(!clip)) DXErrorReturn(ERROR_ASSERTION, "assertion failure")};
 { 
 RGBColor *icp; 
 struct fast *ocp; 
 if (iostart && oostart) { 
 
 if (((scale)<0? -(scale) : (scale)) > 1.0) { 
 { 
 
 iscale = 1.0 / scale; 
 dip = idy; 
 dop = scale>0? ody : -ody; 
 dz = scale>0? dzdy : -dzdy; 
 df = scale>0? iscale : -iscale; 
 
 for (x=0, yy=osy; x<ikx; x++, yy+=skew) { 
 oy = scale>0? ((yy)<=0 || (int)(yy)==yy? (int)(yy) : (int)(yy)+1) : ((yy)>=0 || (int)(yy)==yy? (int)(yy) : (int)(yy)-1); 
 if (oy<0) oy = 0; 
 if (oy>=oky) oy = oky-1; 
 f = (oy - yy) * iscale; 
 iy = ((f)>=0 || (int)(f)==f? (int)(f) : (int)(f)-1); 
 if (iy<0) continue; 
 if (iy>=iky) continue; 
 f = f - iy + 1.0; 
 ocp = (struct fast *)ocstart + (x+osx)*odx + oy*ody; 
 if (1) oop = oostart + (x+osx)*odx + oy*ody; 
 ky = scale>0? oky-oy : oy+1; 
 icp = cstc ? (RGBColor *)icstart : 
 (RGBColor *)icstart + (x+isx)*idx + (iy+isy)*idy; 
 if (1) iop = csto ? (float *)iostart : 
 (float *)iostart + (x+isx)*idx + (iy+isy)*idy; 
 r1 = icp->r * cmul, g1 = icp->g * cmul, b1 = icp->b * cmul; 
 o1 = *iop * omul; 
 r0 = g0 = b0 = o0 = 0; 
 z = osz + (osx+x)*dzdx + oy*dzdy; 
 while (ky>0) { 
 if (f > 1.0) { 
 f -= 1.0; 
 iy += 1; 
 if (iy>=iky) break; 
 r0 = r1, g0 = g1, b0 = b1; 
 if (1) o0 = o1; 
 if (!cstc) icp += dip; 
 if (!csto && 1) iop += dip; 
 r1 = icp->r*cmul, g1 = icp->g*cmul, b1 = icp->b*cmul; 
 o1 = *iop * omul; 
 } 
 f1 = 1.0 - f; 
 if (!((z>ocp->z))); 
 else if (1 && !1) { 
 obar = 1.0 - (o0*f1 + o1*f); 
 ocp->c.r = ocp->c.r*obar + r0*f1 + r1*f; 
 ocp->c.g = ocp->c.g*obar + g0*f1 + g1*f; 
 ocp->c.b = ocp->c.b*obar + b0*f1 + b1*f; 
 } else { 
 ocp->c.r = r0*f1 + r1*f; 
 ocp->c.g = g0*f1 + g1*f; 
 ocp->c.b = b0*f1 + b1*f; 
 if (1) *oop = o0*f1 + o1*f; 
 } 
 f += df; 
 ocp += dop; 
 if (1) oop += dop; 
 z += dz; 
 ky -= 1; 
 } 
 } 
}; 
 } else { 
 { 
 
 iscale = 1.0 / scale; 
 dip = idy; 
 dop = scale>0? ody : -ody; 
 dz = scale>0? dzdy : -dzdy; 
 df = scale>0? scale : -scale; 
 
 for (x=0, yy=osy; x<ikx; x++, yy+=skew) { 
 if (scale>0) { 
 oy = ((yy)<=0 || (int)(yy)==yy? (int)(yy) : (int)(yy)+1); 
 if (oy<0) oy = 0; 
 if (oy>=oky) continue; 
 ky = oky-oy; 
 y = (oy - 1 - yy) * iscale; 
 iy = ((y)<=0 || (int)(y)==y? (int)(y) : (int)(y)+1); 
 } else { 
 oy = ((yy)>=0 || (int)(yy)==yy? (int)(yy) : (int)(yy)-1); 
 if (oy<0) continue; 
 if (oy>=oky) oy = oky-1; 
 ky = oy+1; 
 y = (oy + 1 - yy) * iscale; 
 iy = ((y)<=0 || (int)(y)==y? (int)(y) : (int)(y)+1); 
 } 
 if (iy<0) iy = 0; 
 if (iy>=iky) continue; 
 f = scale>0? oy - (yy + (iy * scale)) 
 : (yy + (iy * scale)) - oy; 
 f = 1 - f; 
 icp = cstc ? (RGBColor *)icstart : 
 (RGBColor *)icstart + (x+isx)*idx + (iy+isy)*idy; 
 if (1) iop = csto ? (float *)iostart : 
 (float *)iostart + (x+isx)*idx + (iy+isy)*idy; 
 ocp = (struct fast *)ocstart + (x+osx)*odx + oy*ody; 
 if (1) oop = oostart + (x+osx)*odx + oy*ody; 
 r0 = r1 = g0 = g1 = b0 = b1 = o0 = o1 = s0 = s1 = 0; 
 while (f<1 && iy<iky) { 
 r = icp->r, g = icp->g, b = icp->b; 
 o = *iop; 
 r0 += r * f, g0 += g * f, b0 += b * f; 
 if (1) o0 += o * f; 
 s0 += f; 
 f += df; 
 iy += 1; 
 if (! cstc) icp += dip; 
 if (!csto && 1) iop += dip; 
 } 
 f -= 1.0; 
 z = osz + (osx+x)*dzdx + oy*dzdy; 
 while (iy<iky && ky>0) { 
 r = icp->r, g = icp->g, b = icp->b; 
 o = *iop; 
 f1 = (1.0 - f); 
 r0 += r*f1, g0 += g*f1, b0 += b*f1; 
 if (1) o0 += o*f1; 
 s0 += f1; 
 r1 += r*f, g1 += g*f, b1 += b*f; 
 if (1) o1 += o*f; 
 s1 += f; 
 f += df; 
 if (f >= 1.0) { 
 f -= 1.0; 
 s0 = s0? (float)1.0/s0 : 0.0; 
 if (!((z>ocp->z))); 
 else if (1 && !1) { 
 obar = 1.0 - o0 * s0 * omul; 
 s0 *= cmul; 
 ocp->c.r = ocp->c.r*obar + r0*s0; 
 ocp->c.g = ocp->c.g*obar + g0*s0; 
 ocp->c.b = ocp->c.b*obar + b0*s0; 
 } else { 
 if (1) *oop = o0 * s0 * omul; 
 s0 *= cmul; 
 ocp->c.r = r0*s0; 
 ocp->c.g = g0*s0; 
 ocp->c.b = b0*s0; 
 } 
 r0 = r1, g0 = g1, b0 = b1; 
 if (1) o0 = o1; 
 s0 = s1; 
 r1 = g1 = b1 = s1 = 0; 
 if (1) o1 = 0; 
 ocp += dop; 
 if (1) oop += dop; 
 z += dz; 
 ky -= 1; 
 } 
 iy += 1; 
 if (! cstc) icp += dip; 
 if (!csto && 1) iop += dip; 
 } 
 if (ky>0 && f>df) { 
 s0 = s0? (float)1.0/s0 : 0.0; 
 if (!((z>ocp->z))); 
 else if (1 && !1) { 
 obar = 1.0 - o0 * s0 * omul; 
 s0 *= cmul; 
 ocp->c.r = ocp->c.r*obar + r0*s0; 
 ocp->c.g = ocp->c.g*obar + g0*s0; 
 ocp->c.b = ocp->c.b*obar + b0*s0; 
 } else { 
 if (1) *oop = o0 * s0 * omul; 
 s0 *= cmul; 
 ocp->c.r = r0*s0; 
 ocp->c.g = g0*s0; 
 ocp->c.b = b0*s0; 
 } 
 } 
 } 
}; 
 }; 
 } else if (iostart) { 
 
 if (((scale)<0? -(scale) : (scale)) > 1.0) { 
 { 
 
 iscale = 1.0 / scale; 
 dip = idy; 
 dop = scale>0? ody : -ody; 
 dz = scale>0? dzdy : -dzdy; 
 df = scale>0? iscale : -iscale; 
 
 for (x=0, yy=osy; x<ikx; x++, yy+=skew) { 
 oy = scale>0? ((yy)<=0 || (int)(yy)==yy? (int)(yy) : (int)(yy)+1) : ((yy)>=0 || (int)(yy)==yy? (int)(yy) : (int)(yy)-1); 
 if (oy<0) oy = 0; 
 if (oy>=oky) oy = oky-1; 
 f = (oy - yy) * iscale; 
 iy = ((f)>=0 || (int)(f)==f? (int)(f) : (int)(f)-1); 
 if (iy<0) continue; 
 if (iy>=iky) continue; 
 f = f - iy + 1.0; 
 ocp = (struct fast *)ocstart + (x+osx)*odx + oy*ody; 
 if (0) oop = oostart + (x+osx)*odx + oy*ody; 
 ky = scale>0? oky-oy : oy+1; 
 icp = cstc ? (RGBColor *)icstart : 
 (RGBColor *)icstart + (x+isx)*idx + (iy+isy)*idy; 
 if (1) iop = csto ? (float *)iostart : 
 (float *)iostart + (x+isx)*idx + (iy+isy)*idy; 
 r1 = icp->r * cmul, g1 = icp->g * cmul, b1 = icp->b * cmul; 
 o1 = *iop * omul; 
 r0 = g0 = b0 = o0 = 0; 
 z = osz + (osx+x)*dzdx + oy*dzdy; 
 while (ky>0) { 
 if (f > 1.0) { 
 f -= 1.0; 
 iy += 1; 
 if (iy>=iky) break; 
 r0 = r1, g0 = g1, b0 = b1; 
 if (1) o0 = o1; 
 if (!cstc) icp += dip; 
 if (!csto && 1) iop += dip; 
 r1 = icp->r*cmul, g1 = icp->g*cmul, b1 = icp->b*cmul; 
 o1 = *iop * omul; 
 } 
 f1 = 1.0 - f; 
 if (!((z>ocp->z))); 
 else if (1 && !0) { 
 obar = 1.0 - (o0*f1 + o1*f); 
 ocp->c.r = ocp->c.r*obar + r0*f1 + r1*f; 
 ocp->c.g = ocp->c.g*obar + g0*f1 + g1*f; 
 ocp->c.b = ocp->c.b*obar + b0*f1 + b1*f; 
 } else { 
 ocp->c.r = r0*f1 + r1*f; 
 ocp->c.g = g0*f1 + g1*f; 
 ocp->c.b = b0*f1 + b1*f; 
 if (0) *oop = o0*f1 + o1*f; 
 } 
 f += df; 
 ocp += dop; 
 if (0) oop += dop; 
 z += dz; 
 ky -= 1; 
 } 
 } 
}; 
 } else { 
 { 
 
 iscale = 1.0 / scale; 
 dip = idy; 
 dop = scale>0? ody : -ody; 
 dz = scale>0? dzdy : -dzdy; 
 df = scale>0? scale : -scale; 
 
 for (x=0, yy=osy; x<ikx; x++, yy+=skew) { 
 if (scale>0) { 
 oy = ((yy)<=0 || (int)(yy)==yy? (int)(yy) : (int)(yy)+1); 
 if (oy<0) oy = 0; 
 if (oy>=oky) continue; 
 ky = oky-oy; 
 y = (oy - 1 - yy) * iscale; 
 iy = ((y)<=0 || (int)(y)==y? (int)(y) : (int)(y)+1); 
 } else { 
 oy = ((yy)>=0 || (int)(yy)==yy? (int)(yy) : (int)(yy)-1); 
 if (oy<0) continue; 
 if (oy>=oky) oy = oky-1; 
 ky = oy+1; 
 y = (oy + 1 - yy) * iscale; 
 iy = ((y)<=0 || (int)(y)==y? (int)(y) : (int)(y)+1); 
 } 
 if (iy<0) iy = 0; 
 if (iy>=iky) continue; 
 f = scale>0? oy - (yy + (iy * scale)) 
 : (yy + (iy * scale)) - oy; 
 f = 1 - f; 
 icp = cstc ? (RGBColor *)icstart : 
 (RGBColor *)icstart + (x+isx)*idx + (iy+isy)*idy; 
 if (1) iop = csto ? (float *)iostart : 
 (float *)iostart + (x+isx)*idx + (iy+isy)*idy; 
 ocp = (struct fast *)ocstart + (x+osx)*odx + oy*ody; 
 if (0) oop = oostart + (x+osx)*odx + oy*ody; 
 r0 = r1 = g0 = g1 = b0 = b1 = o0 = o1 = s0 = s1 = 0; 
 while (f<1 && iy<iky) { 
 r = icp->r, g = icp->g, b = icp->b; 
 o = *iop; 
 r0 += r * f, g0 += g * f, b0 += b * f; 
 if (1) o0 += o * f; 
 s0 += f; 
 f += df; 
 iy += 1; 
 if (! cstc) icp += dip; 
 if (!csto && 1) iop += dip; 
 } 
 f -= 1.0; 
 z = osz + (osx+x)*dzdx + oy*dzdy; 
 while (iy<iky && ky>0) { 
 r = icp->r, g = icp->g, b = icp->b; 
 o = *iop; 
 f1 = (1.0 - f); 
 r0 += r*f1, g0 += g*f1, b0 += b*f1; 
 if (1) o0 += o*f1; 
 s0 += f1; 
 r1 += r*f, g1 += g*f, b1 += b*f; 
 if (1) o1 += o*f; 
 s1 += f; 
 f += df; 
 if (f >= 1.0) { 
 f -= 1.0; 
 s0 = s0? (float)1.0/s0 : 0.0; 
 if (!((z>ocp->z))); 
 else if (1 && !0) { 
 obar = 1.0 - o0 * s0 * omul; 
 s0 *= cmul; 
 ocp->c.r = ocp->c.r*obar + r0*s0; 
 ocp->c.g = ocp->c.g*obar + g0*s0; 
 ocp->c.b = ocp->c.b*obar + b0*s0; 
 } else { 
 if (0) *oop = o0 * s0 * omul; 
 s0 *= cmul; 
 ocp->c.r = r0*s0; 
 ocp->c.g = g0*s0; 
 ocp->c.b = b0*s0; 
 } 
 r0 = r1, g0 = g1, b0 = b1; 
 if (1) o0 = o1; 
 s0 = s1; 
 r1 = g1 = b1 = s1 = 0; 
 if (1) o1 = 0; 
 ocp += dop; 
 if (0) oop += dop; 
 z += dz; 
 ky -= 1; 
 } 
 iy += 1; 
 if (! cstc) icp += dip; 
 if (!csto && 1) iop += dip; 
 } 
 if (ky>0 && f>df) { 
 s0 = s0? (float)1.0/s0 : 0.0; 
 if (!((z>ocp->z))); 
 else if (1 && !0) { 
 obar = 1.0 - o0 * s0 * omul; 
 s0 *= cmul; 
 ocp->c.r = ocp->c.r*obar + r0*s0; 
 ocp->c.g = ocp->c.g*obar + g0*s0; 
 ocp->c.b = ocp->c.b*obar + b0*s0; 
 } else { 
 if (0) *oop = o0 * s0 * omul; 
 s0 *= cmul; 
 ocp->c.r = r0*s0; 
 ocp->c.g = g0*s0; 
 ocp->c.b = b0*s0; 
 } 
 } 
 } 
}; 
 }; 
 } else if (oostart) { 
 
 if (((scale)<0? -(scale) : (scale)) > 1.0) { 
 { 
 
 iscale = 1.0 / scale; 
 dip = idy; 
 dop = scale>0? ody : -ody; 
 dz = scale>0? dzdy : -dzdy; 
 df = scale>0? iscale : -iscale; 
 
 for (x=0, yy=osy; x<ikx; x++, yy+=skew) { 
 oy = scale>0? ((yy)<=0 || (int)(yy)==yy? (int)(yy) : (int)(yy)+1) : ((yy)>=0 || (int)(yy)==yy? (int)(yy) : (int)(yy)-1); 
 if (oy<0) oy = 0; 
 if (oy>=oky) oy = oky-1; 
 f = (oy - yy) * iscale; 
 iy = ((f)>=0 || (int)(f)==f? (int)(f) : (int)(f)-1); 
 if (iy<0) continue; 
 if (iy>=iky) continue; 
 f = f - iy + 1.0; 
 ocp = (struct fast *)ocstart + (x+osx)*odx + oy*ody; 
 if (1) oop = oostart + (x+osx)*odx + oy*ody; 
 ky = scale>0? oky-oy : oy+1; 
 icp = cstc ? (RGBColor *)icstart : 
 (RGBColor *)icstart + (x+isx)*idx + (iy+isy)*idy; 
 if (0) iop = csto ? (float *)iostart : 
 (float *)iostart + (x+isx)*idx + (iy+isy)*idy; 
 r1 = icp->r * cmul, g1 = icp->g * cmul, b1 = icp->b * cmul; 
 o1 = 1 * omul; 
 r0 = g0 = b0 = o0 = 0; 
 z = osz + (osx+x)*dzdx + oy*dzdy; 
 while (ky>0) { 
 if (f > 1.0) { 
 f -= 1.0; 
 iy += 1; 
 if (iy>=iky) break; 
 r0 = r1, g0 = g1, b0 = b1; 
 if (1) o0 = o1; 
 if (!cstc) icp += dip; 
 if (!csto && 0) iop += dip; 
 r1 = icp->r*cmul, g1 = icp->g*cmul, b1 = icp->b*cmul; 
 o1 = 1 * omul; 
 } 
 f1 = 1.0 - f; 
 if (!((z>ocp->z))); 
 else if (1 && !1) { 
 obar = 1.0 - (o0*f1 + o1*f); 
 ocp->c.r = ocp->c.r*obar + r0*f1 + r1*f; 
 ocp->c.g = ocp->c.g*obar + g0*f1 + g1*f; 
 ocp->c.b = ocp->c.b*obar + b0*f1 + b1*f; 
 } else { 
 ocp->c.r = r0*f1 + r1*f; 
 ocp->c.g = g0*f1 + g1*f; 
 ocp->c.b = b0*f1 + b1*f; 
 if (1) *oop = o0*f1 + o1*f; 
 } 
 f += df; 
 ocp += dop; 
 if (1) oop += dop; 
 z += dz; 
 ky -= 1; 
 } 
 } 
}; 
 } else { 
 { 
 
 iscale = 1.0 / scale; 
 dip = idy; 
 dop = scale>0? ody : -ody; 
 dz = scale>0? dzdy : -dzdy; 
 df = scale>0? scale : -scale; 
 
 for (x=0, yy=osy; x<ikx; x++, yy+=skew) { 
 if (scale>0) { 
 oy = ((yy)<=0 || (int)(yy)==yy? (int)(yy) : (int)(yy)+1); 
 if (oy<0) oy = 0; 
 if (oy>=oky) continue; 
 ky = oky-oy; 
 y = (oy - 1 - yy) * iscale; 
 iy = ((y)<=0 || (int)(y)==y? (int)(y) : (int)(y)+1); 
 } else { 
 oy = ((yy)>=0 || (int)(yy)==yy? (int)(yy) : (int)(yy)-1); 
 if (oy<0) continue; 
 if (oy>=oky) oy = oky-1; 
 ky = oy+1; 
 y = (oy + 1 - yy) * iscale; 
 iy = ((y)<=0 || (int)(y)==y? (int)(y) : (int)(y)+1); 
 } 
 if (iy<0) iy = 0; 
 if (iy>=iky) continue; 
 f = scale>0? oy - (yy + (iy * scale)) 
 : (yy + (iy * scale)) - oy; 
 f = 1 - f; 
 icp = cstc ? (RGBColor *)icstart : 
 (RGBColor *)icstart + (x+isx)*idx + (iy+isy)*idy; 
 if (0) iop = csto ? (float *)iostart : 
 (float *)iostart + (x+isx)*idx + (iy+isy)*idy; 
 ocp = (struct fast *)ocstart + (x+osx)*odx + oy*ody; 
 if (1) oop = oostart + (x+osx)*odx + oy*ody; 
 r0 = r1 = g0 = g1 = b0 = b1 = o0 = o1 = s0 = s1 = 0; 
 while (f<1 && iy<iky) { 
 r = icp->r, g = icp->g, b = icp->b; 
 o = 1; 
 r0 += r * f, g0 += g * f, b0 += b * f; 
 if (1) o0 += o * f; 
 s0 += f; 
 f += df; 
 iy += 1; 
 if (! cstc) icp += dip; 
 if (!csto && 0) iop += dip; 
 } 
 f -= 1.0; 
 z = osz + (osx+x)*dzdx + oy*dzdy; 
 while (iy<iky && ky>0) { 
 r = icp->r, g = icp->g, b = icp->b; 
 o = 1; 
 f1 = (1.0 - f); 
 r0 += r*f1, g0 += g*f1, b0 += b*f1; 
 if (1) o0 += o*f1; 
 s0 += f1; 
 r1 += r*f, g1 += g*f, b1 += b*f; 
 if (1) o1 += o*f; 
 s1 += f; 
 f += df; 
 if (f >= 1.0) { 
 f -= 1.0; 
 s0 = s0? (float)1.0/s0 : 0.0; 
 if (!((z>ocp->z))); 
 else if (1 && !1) { 
 obar = 1.0 - o0 * s0 * omul; 
 s0 *= cmul; 
 ocp->c.r = ocp->c.r*obar + r0*s0; 
 ocp->c.g = ocp->c.g*obar + g0*s0; 
 ocp->c.b = ocp->c.b*obar + b0*s0; 
 } else { 
 if (1) *oop = o0 * s0 * omul; 
 s0 *= cmul; 
 ocp->c.r = r0*s0; 
 ocp->c.g = g0*s0; 
 ocp->c.b = b0*s0; 
 } 
 r0 = r1, g0 = g1, b0 = b1; 
 if (1) o0 = o1; 
 s0 = s1; 
 r1 = g1 = b1 = s1 = 0; 
 if (1) o1 = 0; 
 ocp += dop; 
 if (1) oop += dop; 
 z += dz; 
 ky -= 1; 
 } 
 iy += 1; 
 if (! cstc) icp += dip; 
 if (!csto && 0) iop += dip; 
 } 
 if (ky>0 && f>df) { 
 s0 = s0? (float)1.0/s0 : 0.0; 
 if (!((z>ocp->z))); 
 else if (1 && !1) { 
 obar = 1.0 - o0 * s0 * omul; 
 s0 *= cmul; 
 ocp->c.r = ocp->c.r*obar + r0*s0; 
 ocp->c.g = ocp->c.g*obar + g0*s0; 
 ocp->c.b = ocp->c.b*obar + b0*s0; 
 } else { 
 if (1) *oop = o0 * s0 * omul; 
 s0 *= cmul; 
 ocp->c.r = r0*s0; 
 ocp->c.g = g0*s0; 
 ocp->c.b = b0*s0; 
 } 
 } 
 } 
}; 
 }; 
 } else { 
 {if (!(0)) DXErrorReturn(ERROR_ASSERTION, "assertion failure")}; 
 
 
 
 } 
}
                                      ;
    } else if (otype==type_big) {
 if (clip) {
     { 
 RGBColor *icp; 
 struct big *ocp; 
 if (iostart && oostart) { 
 
 if (((scale)<0? -(scale) : (scale)) > 1.0) { 
 { 
 
 iscale = 1.0 / scale; 
 dip = idy; 
 dop = scale>0? ody : -ody; 
 dz = scale>0? dzdy : -dzdy; 
 df = scale>0? iscale : -iscale; 
 
 for (x=0, yy=osy; x<ikx; x++, yy+=skew) { 
 oy = scale>0? ((yy)<=0 || (int)(yy)==yy? (int)(yy) : (int)(yy)+1) : ((yy)>=0 || (int)(yy)==yy? (int)(yy) : (int)(yy)-1); 
 if (oy<0) oy = 0; 
 if (oy>=oky) oy = oky-1; 
 f = (oy - yy) * iscale; 
 iy = ((f)>=0 || (int)(f)==f? (int)(f) : (int)(f)-1); 
 if (iy<0) continue; 
 if (iy>=iky) continue; 
 f = f - iy + 1.0; 
 ocp = (struct big *)ocstart + (x+osx)*odx + oy*ody; 
 if (1) oop = oostart + (x+osx)*odx + oy*ody; 
 ky = scale>0? oky-oy : oy+1; 
 icp = cstc ? (RGBColor *)icstart : 
 (RGBColor *)icstart + (x+isx)*idx + (iy+isy)*idy; 
 if (1) iop = csto ? (float *)iostart : 
 (float *)iostart + (x+isx)*idx + (iy+isy)*idy; 
 r1 = icp->r * cmul, g1 = icp->g * cmul, b1 = icp->b * cmul; 
 o1 = *iop * omul; 
 r0 = g0 = b0 = o0 = 0; 
 z = osz + (osx+x)*dzdx + oy*dzdy; 
 while (ky>0) { 
 if (f > 1.0) { 
 f -= 1.0; 
 iy += 1; 
 if (iy>=iky) break; 
 r0 = r1, g0 = g1, b0 = b1; 
 if (1) o0 = o1; 
 if (!cstc) icp += dip; 
 if (!csto && 1) iop += dip; 
 r1 = icp->r*cmul, g1 = icp->g*cmul, b1 = icp->b*cmul; 
 o1 = *iop * omul; 
 } 
 f1 = 1.0 - f; 
 if (!((z>ocp->z && z<ocp->front))); 
 else if (1 && !1) { 
 obar = 1.0 - (o0*f1 + o1*f); 
 ocp->c.r = ocp->c.r*obar + r0*f1 + r1*f; 
 ocp->c.g = ocp->c.g*obar + g0*f1 + g1*f; 
 ocp->c.b = ocp->c.b*obar + b0*f1 + b1*f; 
 } else { 
 ocp->c.r = r0*f1 + r1*f; 
 ocp->c.g = g0*f1 + g1*f; 
 ocp->c.b = b0*f1 + b1*f; 
 if (1) *oop = o0*f1 + o1*f; 
 } 
 f += df; 
 ocp += dop; 
 if (1) oop += dop; 
 z += dz; 
 ky -= 1; 
 } 
 } 
}; 
 } else { 
 { 
 
 iscale = 1.0 / scale; 
 dip = idy; 
 dop = scale>0? ody : -ody; 
 dz = scale>0? dzdy : -dzdy; 
 df = scale>0? scale : -scale; 
 
 for (x=0, yy=osy; x<ikx; x++, yy+=skew) { 
 if (scale>0) { 
 oy = ((yy)<=0 || (int)(yy)==yy? (int)(yy) : (int)(yy)+1); 
 if (oy<0) oy = 0; 
 if (oy>=oky) continue; 
 ky = oky-oy; 
 y = (oy - 1 - yy) * iscale; 
 iy = ((y)<=0 || (int)(y)==y? (int)(y) : (int)(y)+1); 
 } else { 
 oy = ((yy)>=0 || (int)(yy)==yy? (int)(yy) : (int)(yy)-1); 
 if (oy<0) continue; 
 if (oy>=oky) oy = oky-1; 
 ky = oy+1; 
 y = (oy + 1 - yy) * iscale; 
 iy = ((y)<=0 || (int)(y)==y? (int)(y) : (int)(y)+1); 
 } 
 if (iy<0) iy = 0; 
 if (iy>=iky) continue; 
 f = scale>0? oy - (yy + (iy * scale)) 
 : (yy + (iy * scale)) - oy; 
 f = 1 - f; 
 icp = cstc ? (RGBColor *)icstart : 
 (RGBColor *)icstart + (x+isx)*idx + (iy+isy)*idy; 
 if (1) iop = csto ? (float *)iostart : 
 (float *)iostart + (x+isx)*idx + (iy+isy)*idy; 
 ocp = (struct big *)ocstart + (x+osx)*odx + oy*ody; 
 if (1) oop = oostart + (x+osx)*odx + oy*ody; 
 r0 = r1 = g0 = g1 = b0 = b1 = o0 = o1 = s0 = s1 = 0; 
 while (f<1 && iy<iky) { 
 r = icp->r, g = icp->g, b = icp->b; 
 o = *iop; 
 r0 += r * f, g0 += g * f, b0 += b * f; 
 if (1) o0 += o * f; 
 s0 += f; 
 f += df; 
 iy += 1; 
 if (! cstc) icp += dip; 
 if (!csto && 1) iop += dip; 
 } 
 f -= 1.0; 
 z = osz + (osx+x)*dzdx + oy*dzdy; 
 while (iy<iky && ky>0) { 
 r = icp->r, g = icp->g, b = icp->b; 
 o = *iop; 
 f1 = (1.0 - f); 
 r0 += r*f1, g0 += g*f1, b0 += b*f1; 
 if (1) o0 += o*f1; 
 s0 += f1; 
 r1 += r*f, g1 += g*f, b1 += b*f; 
 if (1) o1 += o*f; 
 s1 += f; 
 f += df; 
 if (f >= 1.0) { 
 f -= 1.0; 
 s0 = s0? (float)1.0/s0 : 0.0; 
 if (!((z>ocp->z && z<ocp->front))); 
 else if (1 && !1) { 
 obar = 1.0 - o0 * s0 * omul; 
 s0 *= cmul; 
 ocp->c.r = ocp->c.r*obar + r0*s0; 
 ocp->c.g = ocp->c.g*obar + g0*s0; 
 ocp->c.b = ocp->c.b*obar + b0*s0; 
 } else { 
 if (1) *oop = o0 * s0 * omul; 
 s0 *= cmul; 
 ocp->c.r = r0*s0; 
 ocp->c.g = g0*s0; 
 ocp->c.b = b0*s0; 
 } 
 r0 = r1, g0 = g1, b0 = b1; 
 if (1) o0 = o1; 
 s0 = s1; 
 r1 = g1 = b1 = s1 = 0; 
 if (1) o1 = 0; 
 ocp += dop; 
 if (1) oop += dop; 
 z += dz; 
 ky -= 1; 
 } 
 iy += 1; 
 if (! cstc) icp += dip; 
 if (!csto && 1) iop += dip; 
 } 
 if (ky>0 && f>df) { 
 s0 = s0? (float)1.0/s0 : 0.0; 
 if (!((z>ocp->z && z<ocp->front))); 
 else if (1 && !1) { 
 obar = 1.0 - o0 * s0 * omul; 
 s0 *= cmul; 
 ocp->c.r = ocp->c.r*obar + r0*s0; 
 ocp->c.g = ocp->c.g*obar + g0*s0; 
 ocp->c.b = ocp->c.b*obar + b0*s0; 
 } else { 
 if (1) *oop = o0 * s0 * omul; 
 s0 *= cmul; 
 ocp->c.r = r0*s0; 
 ocp->c.g = g0*s0; 
 ocp->c.b = b0*s0; 
 } 
 } 
 } 
}; 
 }; 
 } else if (iostart) { 
 
 if (((scale)<0? -(scale) : (scale)) > 1.0) { 
 { 
 
 iscale = 1.0 / scale; 
 dip = idy; 
 dop = scale>0? ody : -ody; 
 dz = scale>0? dzdy : -dzdy; 
 df = scale>0? iscale : -iscale; 
 
 for (x=0, yy=osy; x<ikx; x++, yy+=skew) { 
 oy = scale>0? ((yy)<=0 || (int)(yy)==yy? (int)(yy) : (int)(yy)+1) : ((yy)>=0 || (int)(yy)==yy? (int)(yy) : (int)(yy)-1); 
 if (oy<0) oy = 0; 
 if (oy>=oky) oy = oky-1; 
 f = (oy - yy) * iscale; 
 iy = ((f)>=0 || (int)(f)==f? (int)(f) : (int)(f)-1); 
 if (iy<0) continue; 
 if (iy>=iky) continue; 
 f = f - iy + 1.0; 
 ocp = (struct big *)ocstart + (x+osx)*odx + oy*ody; 
 if (0) oop = oostart + (x+osx)*odx + oy*ody; 
 ky = scale>0? oky-oy : oy+1; 
 icp = cstc ? (RGBColor *)icstart : 
 (RGBColor *)icstart + (x+isx)*idx + (iy+isy)*idy; 
 if (1) iop = csto ? (float *)iostart : 
 (float *)iostart + (x+isx)*idx + (iy+isy)*idy; 
 r1 = icp->r * cmul, g1 = icp->g * cmul, b1 = icp->b * cmul; 
 o1 = *iop * omul; 
 r0 = g0 = b0 = o0 = 0; 
 z = osz + (osx+x)*dzdx + oy*dzdy; 
 while (ky>0) { 
 if (f > 1.0) { 
 f -= 1.0; 
 iy += 1; 
 if (iy>=iky) break; 
 r0 = r1, g0 = g1, b0 = b1; 
 if (1) o0 = o1; 
 if (!cstc) icp += dip; 
 if (!csto && 1) iop += dip; 
 r1 = icp->r*cmul, g1 = icp->g*cmul, b1 = icp->b*cmul; 
 o1 = *iop * omul; 
 } 
 f1 = 1.0 - f; 
 if (!((z>ocp->z && z<ocp->front))); 
 else if (1 && !0) { 
 obar = 1.0 - (o0*f1 + o1*f); 
 ocp->c.r = ocp->c.r*obar + r0*f1 + r1*f; 
 ocp->c.g = ocp->c.g*obar + g0*f1 + g1*f; 
 ocp->c.b = ocp->c.b*obar + b0*f1 + b1*f; 
 } else { 
 ocp->c.r = r0*f1 + r1*f; 
 ocp->c.g = g0*f1 + g1*f; 
 ocp->c.b = b0*f1 + b1*f; 
 if (0) *oop = o0*f1 + o1*f; 
 } 
 f += df; 
 ocp += dop; 
 if (0) oop += dop; 
 z += dz; 
 ky -= 1; 
 } 
 } 
}; 
 } else { 
 { 
 
 iscale = 1.0 / scale; 
 dip = idy; 
 dop = scale>0? ody : -ody; 
 dz = scale>0? dzdy : -dzdy; 
 df = scale>0? scale : -scale; 
 
 for (x=0, yy=osy; x<ikx; x++, yy+=skew) { 
 if (scale>0) { 
 oy = ((yy)<=0 || (int)(yy)==yy? (int)(yy) : (int)(yy)+1); 
 if (oy<0) oy = 0; 
 if (oy>=oky) continue; 
 ky = oky-oy; 
 y = (oy - 1 - yy) * iscale; 
 iy = ((y)<=0 || (int)(y)==y? (int)(y) : (int)(y)+1); 
 } else { 
 oy = ((yy)>=0 || (int)(yy)==yy? (int)(yy) : (int)(yy)-1); 
 if (oy<0) continue; 
 if (oy>=oky) oy = oky-1; 
 ky = oy+1; 
 y = (oy + 1 - yy) * iscale; 
 iy = ((y)<=0 || (int)(y)==y? (int)(y) : (int)(y)+1); 
 } 
 if (iy<0) iy = 0; 
 if (iy>=iky) continue; 
 f = scale>0? oy - (yy + (iy * scale)) 
 : (yy + (iy * scale)) - oy; 
 f = 1 - f; 
 icp = cstc ? (RGBColor *)icstart : 
 (RGBColor *)icstart + (x+isx)*idx + (iy+isy)*idy; 
 if (1) iop = csto ? (float *)iostart : 
 (float *)iostart + (x+isx)*idx + (iy+isy)*idy; 
 ocp = (struct big *)ocstart + (x+osx)*odx + oy*ody; 
 if (0) oop = oostart + (x+osx)*odx + oy*ody; 
 r0 = r1 = g0 = g1 = b0 = b1 = o0 = o1 = s0 = s1 = 0; 
 while (f<1 && iy<iky) { 
 r = icp->r, g = icp->g, b = icp->b; 
 o = *iop; 
 r0 += r * f, g0 += g * f, b0 += b * f; 
 if (1) o0 += o * f; 
 s0 += f; 
 f += df; 
 iy += 1; 
 if (! cstc) icp += dip; 
 if (!csto && 1) iop += dip; 
 } 
 f -= 1.0; 
 z = osz + (osx+x)*dzdx + oy*dzdy; 
 while (iy<iky && ky>0) { 
 r = icp->r, g = icp->g, b = icp->b; 
 o = *iop; 
 f1 = (1.0 - f); 
 r0 += r*f1, g0 += g*f1, b0 += b*f1; 
 if (1) o0 += o*f1; 
 s0 += f1; 
 r1 += r*f, g1 += g*f, b1 += b*f; 
 if (1) o1 += o*f; 
 s1 += f; 
 f += df; 
 if (f >= 1.0) { 
 f -= 1.0; 
 s0 = s0? (float)1.0/s0 : 0.0; 
 if (!((z>ocp->z && z<ocp->front))); 
 else if (1 && !0) { 
 obar = 1.0 - o0 * s0 * omul; 
 s0 *= cmul; 
 ocp->c.r = ocp->c.r*obar + r0*s0; 
 ocp->c.g = ocp->c.g*obar + g0*s0; 
 ocp->c.b = ocp->c.b*obar + b0*s0; 
 } else { 
 if (0) *oop = o0 * s0 * omul; 
 s0 *= cmul; 
 ocp->c.r = r0*s0; 
 ocp->c.g = g0*s0; 
 ocp->c.b = b0*s0; 
 } 
 r0 = r1, g0 = g1, b0 = b1; 
 if (1) o0 = o1; 
 s0 = s1; 
 r1 = g1 = b1 = s1 = 0; 
 if (1) o1 = 0; 
 ocp += dop; 
 if (0) oop += dop; 
 z += dz; 
 ky -= 1; 
 } 
 iy += 1; 
 if (! cstc) icp += dip; 
 if (!csto && 1) iop += dip; 
 } 
 if (ky>0 && f>df) { 
 s0 = s0? (float)1.0/s0 : 0.0; 
 if (!((z>ocp->z && z<ocp->front))); 
 else if (1 && !0) { 
 obar = 1.0 - o0 * s0 * omul; 
 s0 *= cmul; 
 ocp->c.r = ocp->c.r*obar + r0*s0; 
 ocp->c.g = ocp->c.g*obar + g0*s0; 
 ocp->c.b = ocp->c.b*obar + b0*s0; 
 } else { 
 if (0) *oop = o0 * s0 * omul; 
 s0 *= cmul; 
 ocp->c.r = r0*s0; 
 ocp->c.g = g0*s0; 
 ocp->c.b = b0*s0; 
 } 
 } 
 } 
}; 
 }; 
 } else if (oostart) { 
 
 if (((scale)<0? -(scale) : (scale)) > 1.0) { 
 { 
 
 iscale = 1.0 / scale; 
 dip = idy; 
 dop = scale>0? ody : -ody; 
 dz = scale>0? dzdy : -dzdy; 
 df = scale>0? iscale : -iscale; 
 
 for (x=0, yy=osy; x<ikx; x++, yy+=skew) { 
 oy = scale>0? ((yy)<=0 || (int)(yy)==yy? (int)(yy) : (int)(yy)+1) : ((yy)>=0 || (int)(yy)==yy? (int)(yy) : (int)(yy)-1); 
 if (oy<0) oy = 0; 
 if (oy>=oky) oy = oky-1; 
 f = (oy - yy) * iscale; 
 iy = ((f)>=0 || (int)(f)==f? (int)(f) : (int)(f)-1); 
 if (iy<0) continue; 
 if (iy>=iky) continue; 
 f = f - iy + 1.0; 
 ocp = (struct big *)ocstart + (x+osx)*odx + oy*ody; 
 if (1) oop = oostart + (x+osx)*odx + oy*ody; 
 ky = scale>0? oky-oy : oy+1; 
 icp = cstc ? (RGBColor *)icstart : 
 (RGBColor *)icstart + (x+isx)*idx + (iy+isy)*idy; 
 if (0) iop = csto ? (float *)iostart : 
 (float *)iostart + (x+isx)*idx + (iy+isy)*idy; 
 r1 = icp->r * cmul, g1 = icp->g * cmul, b1 = icp->b * cmul; 
 o1 = 1 * omul; 
 r0 = g0 = b0 = o0 = 0; 
 z = osz + (osx+x)*dzdx + oy*dzdy; 
 while (ky>0) { 
 if (f > 1.0) { 
 f -= 1.0; 
 iy += 1; 
 if (iy>=iky) break; 
 r0 = r1, g0 = g1, b0 = b1; 
 if (1) o0 = o1; 
 if (!cstc) icp += dip; 
 if (!csto && 0) iop += dip; 
 r1 = icp->r*cmul, g1 = icp->g*cmul, b1 = icp->b*cmul; 
 o1 = 1 * omul; 
 } 
 f1 = 1.0 - f; 
 if (!((z>ocp->z && z<ocp->front))); 
 else if (1 && !1) { 
 obar = 1.0 - (o0*f1 + o1*f); 
 ocp->c.r = ocp->c.r*obar + r0*f1 + r1*f; 
 ocp->c.g = ocp->c.g*obar + g0*f1 + g1*f; 
 ocp->c.b = ocp->c.b*obar + b0*f1 + b1*f; 
 } else { 
 ocp->c.r = r0*f1 + r1*f; 
 ocp->c.g = g0*f1 + g1*f; 
 ocp->c.b = b0*f1 + b1*f; 
 if (1) *oop = o0*f1 + o1*f; 
 } 
 f += df; 
 ocp += dop; 
 if (1) oop += dop; 
 z += dz; 
 ky -= 1; 
 } 
 } 
}; 
 } else { 
 { 
 
 iscale = 1.0 / scale; 
 dip = idy; 
 dop = scale>0? ody : -ody; 
 dz = scale>0? dzdy : -dzdy; 
 df = scale>0? scale : -scale; 
 
 for (x=0, yy=osy; x<ikx; x++, yy+=skew) { 
 if (scale>0) { 
 oy = ((yy)<=0 || (int)(yy)==yy? (int)(yy) : (int)(yy)+1); 
 if (oy<0) oy = 0; 
 if (oy>=oky) continue; 
 ky = oky-oy; 
 y = (oy - 1 - yy) * iscale; 
 iy = ((y)<=0 || (int)(y)==y? (int)(y) : (int)(y)+1); 
 } else { 
 oy = ((yy)>=0 || (int)(yy)==yy? (int)(yy) : (int)(yy)-1); 
 if (oy<0) continue; 
 if (oy>=oky) oy = oky-1; 
 ky = oy+1; 
 y = (oy + 1 - yy) * iscale; 
 iy = ((y)<=0 || (int)(y)==y? (int)(y) : (int)(y)+1); 
 } 
 if (iy<0) iy = 0; 
 if (iy>=iky) continue; 
 f = scale>0? oy - (yy + (iy * scale)) 
 : (yy + (iy * scale)) - oy; 
 f = 1 - f; 
 icp = cstc ? (RGBColor *)icstart : 
 (RGBColor *)icstart + (x+isx)*idx + (iy+isy)*idy; 
 if (0) iop = csto ? (float *)iostart : 
 (float *)iostart + (x+isx)*idx + (iy+isy)*idy; 
 ocp = (struct big *)ocstart + (x+osx)*odx + oy*ody; 
 if (1) oop = oostart + (x+osx)*odx + oy*ody; 
 r0 = r1 = g0 = g1 = b0 = b1 = o0 = o1 = s0 = s1 = 0; 
 while (f<1 && iy<iky) { 
 r = icp->r, g = icp->g, b = icp->b; 
 o = 1; 
 r0 += r * f, g0 += g * f, b0 += b * f; 
 if (1) o0 += o * f; 
 s0 += f; 
 f += df; 
 iy += 1; 
 if (! cstc) icp += dip; 
 if (!csto && 0) iop += dip; 
 } 
 f -= 1.0; 
 z = osz + (osx+x)*dzdx + oy*dzdy; 
 while (iy<iky && ky>0) { 
 r = icp->r, g = icp->g, b = icp->b; 
 o = 1; 
 f1 = (1.0 - f); 
 r0 += r*f1, g0 += g*f1, b0 += b*f1; 
 if (1) o0 += o*f1; 
 s0 += f1; 
 r1 += r*f, g1 += g*f, b1 += b*f; 
 if (1) o1 += o*f; 
 s1 += f; 
 f += df; 
 if (f >= 1.0) { 
 f -= 1.0; 
 s0 = s0? (float)1.0/s0 : 0.0; 
 if (!((z>ocp->z && z<ocp->front))); 
 else if (1 && !1) { 
 obar = 1.0 - o0 * s0 * omul; 
 s0 *= cmul; 
 ocp->c.r = ocp->c.r*obar + r0*s0; 
 ocp->c.g = ocp->c.g*obar + g0*s0; 
 ocp->c.b = ocp->c.b*obar + b0*s0; 
 } else { 
 if (1) *oop = o0 * s0 * omul; 
 s0 *= cmul; 
 ocp->c.r = r0*s0; 
 ocp->c.g = g0*s0; 
 ocp->c.b = b0*s0; 
 } 
 r0 = r1, g0 = g1, b0 = b1; 
 if (1) o0 = o1; 
 s0 = s1; 
 r1 = g1 = b1 = s1 = 0; 
 if (1) o1 = 0; 
 ocp += dop; 
 if (1) oop += dop; 
 z += dz; 
 ky -= 1; 
 } 
 iy += 1; 
 if (! cstc) icp += dip; 
 if (!csto && 0) iop += dip; 
 } 
 if (ky>0 && f>df) { 
 s0 = s0? (float)1.0/s0 : 0.0; 
 if (!((z>ocp->z && z<ocp->front))); 
 else if (1 && !1) { 
 obar = 1.0 - o0 * s0 * omul; 
 s0 *= cmul; 
 ocp->c.r = ocp->c.r*obar + r0*s0; 
 ocp->c.g = ocp->c.g*obar + g0*s0; 
 ocp->c.b = ocp->c.b*obar + b0*s0; 
 } else { 
 if (1) *oop = o0 * s0 * omul; 
 s0 *= cmul; 
 ocp->c.r = r0*s0; 
 ocp->c.g = g0*s0; 
 ocp->c.b = b0*s0; 
 } 
 } 
 } 
}; 
 }; 
 } else { 
 {if (!(0)) DXErrorReturn(ERROR_ASSERTION, "assertion failure")}; 
 
 
 
 } 
}
                                       ;
 } else {
     { 
 RGBColor *icp; 
 struct big *ocp; 
 if (iostart && oostart) { 
 
 if (((scale)<0? -(scale) : (scale)) > 1.0) { 
 { 
 
 iscale = 1.0 / scale; 
 dip = idy; 
 dop = scale>0? ody : -ody; 
 dz = scale>0? dzdy : -dzdy; 
 df = scale>0? iscale : -iscale; 
 
 for (x=0, yy=osy; x<ikx; x++, yy+=skew) { 
 oy = scale>0? ((yy)<=0 || (int)(yy)==yy? (int)(yy) : (int)(yy)+1) : ((yy)>=0 || (int)(yy)==yy? (int)(yy) : (int)(yy)-1); 
 if (oy<0) oy = 0; 
 if (oy>=oky) oy = oky-1; 
 f = (oy - yy) * iscale; 
 iy = ((f)>=0 || (int)(f)==f? (int)(f) : (int)(f)-1); 
 if (iy<0) continue; 
 if (iy>=iky) continue; 
 f = f - iy + 1.0; 
 ocp = (struct big *)ocstart + (x+osx)*odx + oy*ody; 
 if (1) oop = oostart + (x+osx)*odx + oy*ody; 
 ky = scale>0? oky-oy : oy+1; 
 icp = cstc ? (RGBColor *)icstart : 
 (RGBColor *)icstart + (x+isx)*idx + (iy+isy)*idy; 
 if (1) iop = csto ? (float *)iostart : 
 (float *)iostart + (x+isx)*idx + (iy+isy)*idy; 
 r1 = icp->r * cmul, g1 = icp->g * cmul, b1 = icp->b * cmul; 
 o1 = *iop * omul; 
 r0 = g0 = b0 = o0 = 0; 
 z = osz + (osx+x)*dzdx + oy*dzdy; 
 while (ky>0) { 
 if (f > 1.0) { 
 f -= 1.0; 
 iy += 1; 
 if (iy>=iky) break; 
 r0 = r1, g0 = g1, b0 = b1; 
 if (1) o0 = o1; 
 if (!cstc) icp += dip; 
 if (!csto && 1) iop += dip; 
 r1 = icp->r*cmul, g1 = icp->g*cmul, b1 = icp->b*cmul; 
 o1 = *iop * omul; 
 } 
 f1 = 1.0 - f; 
 if (!((z>ocp->z))); 
 else if (1 && !1) { 
 obar = 1.0 - (o0*f1 + o1*f); 
 ocp->c.r = ocp->c.r*obar + r0*f1 + r1*f; 
 ocp->c.g = ocp->c.g*obar + g0*f1 + g1*f; 
 ocp->c.b = ocp->c.b*obar + b0*f1 + b1*f; 
 } else { 
 ocp->c.r = r0*f1 + r1*f; 
 ocp->c.g = g0*f1 + g1*f; 
 ocp->c.b = b0*f1 + b1*f; 
 if (1) *oop = o0*f1 + o1*f; 
 } 
 f += df; 
 ocp += dop; 
 if (1) oop += dop; 
 z += dz; 
 ky -= 1; 
 } 
 } 
}; 
 } else { 
 { 
 
 iscale = 1.0 / scale; 
 dip = idy; 
 dop = scale>0? ody : -ody; 
 dz = scale>0? dzdy : -dzdy; 
 df = scale>0? scale : -scale; 
 
 for (x=0, yy=osy; x<ikx; x++, yy+=skew) { 
 if (scale>0) { 
 oy = ((yy)<=0 || (int)(yy)==yy? (int)(yy) : (int)(yy)+1); 
 if (oy<0) oy = 0; 
 if (oy>=oky) continue; 
 ky = oky-oy; 
 y = (oy - 1 - yy) * iscale; 
 iy = ((y)<=0 || (int)(y)==y? (int)(y) : (int)(y)+1); 
 } else { 
 oy = ((yy)>=0 || (int)(yy)==yy? (int)(yy) : (int)(yy)-1); 
 if (oy<0) continue; 
 if (oy>=oky) oy = oky-1; 
 ky = oy+1; 
 y = (oy + 1 - yy) * iscale; 
 iy = ((y)<=0 || (int)(y)==y? (int)(y) : (int)(y)+1); 
 } 
 if (iy<0) iy = 0; 
 if (iy>=iky) continue; 
 f = scale>0? oy - (yy + (iy * scale)) 
 : (yy + (iy * scale)) - oy; 
 f = 1 - f; 
 icp = cstc ? (RGBColor *)icstart : 
 (RGBColor *)icstart + (x+isx)*idx + (iy+isy)*idy; 
 if (1) iop = csto ? (float *)iostart : 
 (float *)iostart + (x+isx)*idx + (iy+isy)*idy; 
 ocp = (struct big *)ocstart + (x+osx)*odx + oy*ody; 
 if (1) oop = oostart + (x+osx)*odx + oy*ody; 
 r0 = r1 = g0 = g1 = b0 = b1 = o0 = o1 = s0 = s1 = 0; 
 while (f<1 && iy<iky) { 
 r = icp->r, g = icp->g, b = icp->b; 
 o = *iop; 
 r0 += r * f, g0 += g * f, b0 += b * f; 
 if (1) o0 += o * f; 
 s0 += f; 
 f += df; 
 iy += 1; 
 if (! cstc) icp += dip; 
 if (!csto && 1) iop += dip; 
 } 
 f -= 1.0; 
 z = osz + (osx+x)*dzdx + oy*dzdy; 
 while (iy<iky && ky>0) { 
 r = icp->r, g = icp->g, b = icp->b; 
 o = *iop; 
 f1 = (1.0 - f); 
 r0 += r*f1, g0 += g*f1, b0 += b*f1; 
 if (1) o0 += o*f1; 
 s0 += f1; 
 r1 += r*f, g1 += g*f, b1 += b*f; 
 if (1) o1 += o*f; 
 s1 += f; 
 f += df; 
 if (f >= 1.0) { 
 f -= 1.0; 
 s0 = s0? (float)1.0/s0 : 0.0; 
 if (!((z>ocp->z))); 
 else if (1 && !1) { 
 obar = 1.0 - o0 * s0 * omul; 
 s0 *= cmul; 
 ocp->c.r = ocp->c.r*obar + r0*s0; 
 ocp->c.g = ocp->c.g*obar + g0*s0; 
 ocp->c.b = ocp->c.b*obar + b0*s0; 
 } else { 
 if (1) *oop = o0 * s0 * omul; 
 s0 *= cmul; 
 ocp->c.r = r0*s0; 
 ocp->c.g = g0*s0; 
 ocp->c.b = b0*s0; 
 } 
 r0 = r1, g0 = g1, b0 = b1; 
 if (1) o0 = o1; 
 s0 = s1; 
 r1 = g1 = b1 = s1 = 0; 
 if (1) o1 = 0; 
 ocp += dop; 
 if (1) oop += dop; 
 z += dz; 
 ky -= 1; 
 } 
 iy += 1; 
 if (! cstc) icp += dip; 
 if (!csto && 1) iop += dip; 
 } 
 if (ky>0 && f>df) { 
 s0 = s0? (float)1.0/s0 : 0.0; 
 if (!((z>ocp->z))); 
 else if (1 && !1) { 
 obar = 1.0 - o0 * s0 * omul; 
 s0 *= cmul; 
 ocp->c.r = ocp->c.r*obar + r0*s0; 
 ocp->c.g = ocp->c.g*obar + g0*s0; 
 ocp->c.b = ocp->c.b*obar + b0*s0; 
 } else { 
 if (1) *oop = o0 * s0 * omul; 
 s0 *= cmul; 
 ocp->c.r = r0*s0; 
 ocp->c.g = g0*s0; 
 ocp->c.b = b0*s0; 
 } 
 } 
 } 
}; 
 }; 
 } else if (iostart) { 
 
 if (((scale)<0? -(scale) : (scale)) > 1.0) { 
 { 
 
 iscale = 1.0 / scale; 
 dip = idy; 
 dop = scale>0? ody : -ody; 
 dz = scale>0? dzdy : -dzdy; 
 df = scale>0? iscale : -iscale; 
 
 for (x=0, yy=osy; x<ikx; x++, yy+=skew) { 
 oy = scale>0? ((yy)<=0 || (int)(yy)==yy? (int)(yy) : (int)(yy)+1) : ((yy)>=0 || (int)(yy)==yy? (int)(yy) : (int)(yy)-1); 
 if (oy<0) oy = 0; 
 if (oy>=oky) oy = oky-1; 
 f = (oy - yy) * iscale; 
 iy = ((f)>=0 || (int)(f)==f? (int)(f) : (int)(f)-1); 
 if (iy<0) continue; 
 if (iy>=iky) continue; 
 f = f - iy + 1.0; 
 ocp = (struct big *)ocstart + (x+osx)*odx + oy*ody; 
 if (0) oop = oostart + (x+osx)*odx + oy*ody; 
 ky = scale>0? oky-oy : oy+1; 
 icp = cstc ? (RGBColor *)icstart : 
 (RGBColor *)icstart + (x+isx)*idx + (iy+isy)*idy; 
 if (1) iop = csto ? (float *)iostart : 
 (float *)iostart + (x+isx)*idx + (iy+isy)*idy; 
 r1 = icp->r * cmul, g1 = icp->g * cmul, b1 = icp->b * cmul; 
 o1 = *iop * omul; 
 r0 = g0 = b0 = o0 = 0; 
 z = osz + (osx+x)*dzdx + oy*dzdy; 
 while (ky>0) { 
 if (f > 1.0) { 
 f -= 1.0; 
 iy += 1; 
 if (iy>=iky) break; 
 r0 = r1, g0 = g1, b0 = b1; 
 if (1) o0 = o1; 
 if (!cstc) icp += dip; 
 if (!csto && 1) iop += dip; 
 r1 = icp->r*cmul, g1 = icp->g*cmul, b1 = icp->b*cmul; 
 o1 = *iop * omul; 
 } 
 f1 = 1.0 - f; 
 if (!((z>ocp->z))); 
 else if (1 && !0) { 
 obar = 1.0 - (o0*f1 + o1*f); 
 ocp->c.r = ocp->c.r*obar + r0*f1 + r1*f; 
 ocp->c.g = ocp->c.g*obar + g0*f1 + g1*f; 
 ocp->c.b = ocp->c.b*obar + b0*f1 + b1*f; 
 } else { 
 ocp->c.r = r0*f1 + r1*f; 
 ocp->c.g = g0*f1 + g1*f; 
 ocp->c.b = b0*f1 + b1*f; 
 if (0) *oop = o0*f1 + o1*f; 
 } 
 f += df; 
 ocp += dop; 
 if (0) oop += dop; 
 z += dz; 
 ky -= 1; 
 } 
 } 
}; 
 } else { 
 { 
 
 iscale = 1.0 / scale; 
 dip = idy; 
 dop = scale>0? ody : -ody; 
 dz = scale>0? dzdy : -dzdy; 
 df = scale>0? scale : -scale; 
 
 for (x=0, yy=osy; x<ikx; x++, yy+=skew) { 
 if (scale>0) { 
 oy = ((yy)<=0 || (int)(yy)==yy? (int)(yy) : (int)(yy)+1); 
 if (oy<0) oy = 0; 
 if (oy>=oky) continue; 
 ky = oky-oy; 
 y = (oy - 1 - yy) * iscale; 
 iy = ((y)<=0 || (int)(y)==y? (int)(y) : (int)(y)+1); 
 } else { 
 oy = ((yy)>=0 || (int)(yy)==yy? (int)(yy) : (int)(yy)-1); 
 if (oy<0) continue; 
 if (oy>=oky) oy = oky-1; 
 ky = oy+1; 
 y = (oy + 1 - yy) * iscale; 
 iy = ((y)<=0 || (int)(y)==y? (int)(y) : (int)(y)+1); 
 } 
 if (iy<0) iy = 0; 
 if (iy>=iky) continue; 
 f = scale>0? oy - (yy + (iy * scale)) 
 : (yy + (iy * scale)) - oy; 
 f = 1 - f; 
 icp = cstc ? (RGBColor *)icstart : 
 (RGBColor *)icstart + (x+isx)*idx + (iy+isy)*idy; 
 if (1) iop = csto ? (float *)iostart : 
 (float *)iostart + (x+isx)*idx + (iy+isy)*idy; 
 ocp = (struct big *)ocstart + (x+osx)*odx + oy*ody; 
 if (0) oop = oostart + (x+osx)*odx + oy*ody; 
 r0 = r1 = g0 = g1 = b0 = b1 = o0 = o1 = s0 = s1 = 0; 
 while (f<1 && iy<iky) { 
 r = icp->r, g = icp->g, b = icp->b; 
 o = *iop; 
 r0 += r * f, g0 += g * f, b0 += b * f; 
 if (1) o0 += o * f; 
 s0 += f; 
 f += df; 
 iy += 1; 
 if (! cstc) icp += dip; 
 if (!csto && 1) iop += dip; 
 } 
 f -= 1.0; 
 z = osz + (osx+x)*dzdx + oy*dzdy; 
 while (iy<iky && ky>0) { 
 r = icp->r, g = icp->g, b = icp->b; 
 o = *iop; 
 f1 = (1.0 - f); 
 r0 += r*f1, g0 += g*f1, b0 += b*f1; 
 if (1) o0 += o*f1; 
 s0 += f1; 
 r1 += r*f, g1 += g*f, b1 += b*f; 
 if (1) o1 += o*f; 
 s1 += f; 
 f += df; 
 if (f >= 1.0) { 
 f -= 1.0; 
 s0 = s0? (float)1.0/s0 : 0.0; 
 if (!((z>ocp->z))); 
 else if (1 && !0) { 
 obar = 1.0 - o0 * s0 * omul; 
 s0 *= cmul; 
 ocp->c.r = ocp->c.r*obar + r0*s0; 
 ocp->c.g = ocp->c.g*obar + g0*s0; 
 ocp->c.b = ocp->c.b*obar + b0*s0; 
 } else { 
 if (0) *oop = o0 * s0 * omul; 
 s0 *= cmul; 
 ocp->c.r = r0*s0; 
 ocp->c.g = g0*s0; 
 ocp->c.b = b0*s0; 
 } 
 r0 = r1, g0 = g1, b0 = b1; 
 if (1) o0 = o1; 
 s0 = s1; 
 r1 = g1 = b1 = s1 = 0; 
 if (1) o1 = 0; 
 ocp += dop; 
 if (0) oop += dop; 
 z += dz; 
 ky -= 1; 
 } 
 iy += 1; 
 if (! cstc) icp += dip; 
 if (!csto && 1) iop += dip; 
 } 
 if (ky>0 && f>df) { 
 s0 = s0? (float)1.0/s0 : 0.0; 
 if (!((z>ocp->z))); 
 else if (1 && !0) { 
 obar = 1.0 - o0 * s0 * omul; 
 s0 *= cmul; 
 ocp->c.r = ocp->c.r*obar + r0*s0; 
 ocp->c.g = ocp->c.g*obar + g0*s0; 
 ocp->c.b = ocp->c.b*obar + b0*s0; 
 } else { 
 if (0) *oop = o0 * s0 * omul; 
 s0 *= cmul; 
 ocp->c.r = r0*s0; 
 ocp->c.g = g0*s0; 
 ocp->c.b = b0*s0; 
 } 
 } 
 } 
}; 
 }; 
 } else if (oostart) { 
 
 if (((scale)<0? -(scale) : (scale)) > 1.0) { 
 { 
 
 iscale = 1.0 / scale; 
 dip = idy; 
 dop = scale>0? ody : -ody; 
 dz = scale>0? dzdy : -dzdy; 
 df = scale>0? iscale : -iscale; 
 
 for (x=0, yy=osy; x<ikx; x++, yy+=skew) { 
 oy = scale>0? ((yy)<=0 || (int)(yy)==yy? (int)(yy) : (int)(yy)+1) : ((yy)>=0 || (int)(yy)==yy? (int)(yy) : (int)(yy)-1); 
 if (oy<0) oy = 0; 
 if (oy>=oky) oy = oky-1; 
 f = (oy - yy) * iscale; 
 iy = ((f)>=0 || (int)(f)==f? (int)(f) : (int)(f)-1); 
 if (iy<0) continue; 
 if (iy>=iky) continue; 
 f = f - iy + 1.0; 
 ocp = (struct big *)ocstart + (x+osx)*odx + oy*ody; 
 if (1) oop = oostart + (x+osx)*odx + oy*ody; 
 ky = scale>0? oky-oy : oy+1; 
 icp = cstc ? (RGBColor *)icstart : 
 (RGBColor *)icstart + (x+isx)*idx + (iy+isy)*idy; 
 if (0) iop = csto ? (float *)iostart : 
 (float *)iostart + (x+isx)*idx + (iy+isy)*idy; 
 r1 = icp->r * cmul, g1 = icp->g * cmul, b1 = icp->b * cmul; 
 o1 = 1 * omul; 
 r0 = g0 = b0 = o0 = 0; 
 z = osz + (osx+x)*dzdx + oy*dzdy; 
 while (ky>0) { 
 if (f > 1.0) { 
 f -= 1.0; 
 iy += 1; 
 if (iy>=iky) break; 
 r0 = r1, g0 = g1, b0 = b1; 
 if (1) o0 = o1; 
 if (!cstc) icp += dip; 
 if (!csto && 0) iop += dip; 
 r1 = icp->r*cmul, g1 = icp->g*cmul, b1 = icp->b*cmul; 
 o1 = 1 * omul; 
 } 
 f1 = 1.0 - f; 
 if (!((z>ocp->z))); 
 else if (1 && !1) { 
 obar = 1.0 - (o0*f1 + o1*f); 
 ocp->c.r = ocp->c.r*obar + r0*f1 + r1*f; 
 ocp->c.g = ocp->c.g*obar + g0*f1 + g1*f; 
 ocp->c.b = ocp->c.b*obar + b0*f1 + b1*f; 
 } else { 
 ocp->c.r = r0*f1 + r1*f; 
 ocp->c.g = g0*f1 + g1*f; 
 ocp->c.b = b0*f1 + b1*f; 
 if (1) *oop = o0*f1 + o1*f; 
 } 
 f += df; 
 ocp += dop; 
 if (1) oop += dop; 
 z += dz; 
 ky -= 1; 
 } 
 } 
}; 
 } else { 
 { 
 
 iscale = 1.0 / scale; 
 dip = idy; 
 dop = scale>0? ody : -ody; 
 dz = scale>0? dzdy : -dzdy; 
 df = scale>0? scale : -scale; 
 
 for (x=0, yy=osy; x<ikx; x++, yy+=skew) { 
 if (scale>0) { 
 oy = ((yy)<=0 || (int)(yy)==yy? (int)(yy) : (int)(yy)+1); 
 if (oy<0) oy = 0; 
 if (oy>=oky) continue; 
 ky = oky-oy; 
 y = (oy - 1 - yy) * iscale; 
 iy = ((y)<=0 || (int)(y)==y? (int)(y) : (int)(y)+1); 
 } else { 
 oy = ((yy)>=0 || (int)(yy)==yy? (int)(yy) : (int)(yy)-1); 
 if (oy<0) continue; 
 if (oy>=oky) oy = oky-1; 
 ky = oy+1; 
 y = (oy + 1 - yy) * iscale; 
 iy = ((y)<=0 || (int)(y)==y? (int)(y) : (int)(y)+1); 
 } 
 if (iy<0) iy = 0; 
 if (iy>=iky) continue; 
 f = scale>0? oy - (yy + (iy * scale)) 
 : (yy + (iy * scale)) - oy; 
 f = 1 - f; 
 icp = cstc ? (RGBColor *)icstart : 
 (RGBColor *)icstart + (x+isx)*idx + (iy+isy)*idy; 
 if (0) iop = csto ? (float *)iostart : 
 (float *)iostart + (x+isx)*idx + (iy+isy)*idy; 
 ocp = (struct big *)ocstart + (x+osx)*odx + oy*ody; 
 if (1) oop = oostart + (x+osx)*odx + oy*ody; 
 r0 = r1 = g0 = g1 = b0 = b1 = o0 = o1 = s0 = s1 = 0; 
 while (f<1 && iy<iky) { 
 r = icp->r, g = icp->g, b = icp->b; 
 o = 1; 
 r0 += r * f, g0 += g * f, b0 += b * f; 
 if (1) o0 += o * f; 
 s0 += f; 
 f += df; 
 iy += 1; 
 if (! cstc) icp += dip; 
 if (!csto && 0) iop += dip; 
 } 
 f -= 1.0; 
 z = osz + (osx+x)*dzdx + oy*dzdy; 
 while (iy<iky && ky>0) { 
 r = icp->r, g = icp->g, b = icp->b; 
 o = 1; 
 f1 = (1.0 - f); 
 r0 += r*f1, g0 += g*f1, b0 += b*f1; 
 if (1) o0 += o*f1; 
 s0 += f1; 
 r1 += r*f, g1 += g*f, b1 += b*f; 
 if (1) o1 += o*f; 
 s1 += f; 
 f += df; 
 if (f >= 1.0) { 
 f -= 1.0; 
 s0 = s0? (float)1.0/s0 : 0.0; 
 if (!((z>ocp->z))); 
 else if (1 && !1) { 
 obar = 1.0 - o0 * s0 * omul; 
 s0 *= cmul; 
 ocp->c.r = ocp->c.r*obar + r0*s0; 
 ocp->c.g = ocp->c.g*obar + g0*s0; 
 ocp->c.b = ocp->c.b*obar + b0*s0; 
 } else { 
 if (1) *oop = o0 * s0 * omul; 
 s0 *= cmul; 
 ocp->c.r = r0*s0; 
 ocp->c.g = g0*s0; 
 ocp->c.b = b0*s0; 
 } 
 r0 = r1, g0 = g1, b0 = b1; 
 if (1) o0 = o1; 
 s0 = s1; 
 r1 = g1 = b1 = s1 = 0; 
 if (1) o1 = 0; 
 ocp += dop; 
 if (1) oop += dop; 
 z += dz; 
 ky -= 1; 
 } 
 iy += 1; 
 if (! cstc) icp += dip; 
 if (!csto && 0) iop += dip; 
 } 
 if (ky>0 && f>df) { 
 s0 = s0? (float)1.0/s0 : 0.0; 
 if (!((z>ocp->z))); 
 else if (1 && !1) { 
 obar = 1.0 - o0 * s0 * omul; 
 s0 *= cmul; 
 ocp->c.r = ocp->c.r*obar + r0*s0; 
 ocp->c.g = ocp->c.g*obar + g0*s0; 
 ocp->c.b = ocp->c.b*obar + b0*s0; 
 } else { 
 if (1) *oop = o0 * s0 * omul; 
 s0 *= cmul; 
 ocp->c.r = r0*s0; 
 ocp->c.g = g0*s0; 
 ocp->c.b = b0*s0; 
 } 
 } 
 } 
}; 
 }; 
 } else { 
 {if (!(0)) DXErrorReturn(ERROR_ASSERTION, "assertion failure")}; 
 
 
 
 } 
}
                                         ;
 }
    }
    return OK;
}
static Error
plane(Pointer ic,
      char cstc,
      RGBColor *cmap,
      Pointer io,
      char csto,
      float *omap,
      float cmul, float omul,
      int idx, int idy, int inx, int iny,
      float ux, float uy, float uz, float vx, float vy, float vz,
      Pointer oc, enum type otype,
      int onx, int ony, float osx, float osy, float osz, int clip
) {
    float x, y, minx, miny, maxx, maxy, A, dzdx, dzdy;
    int isx, isy, ikx, iky;
    float scale1=0, skew1=0, scale2=0, skew2=0, osy1, osy2, best, extra;
    int ti1=0, to1, ti2, to2=0, contrary=0, osx2;
    int nx1, ny1, nk, nc;
    RGBColor *ic1=NULL;
    float *io1=NULL;
    Error rc;
    A = ux * vy - uy * vx;
    best = 0;
    if (((ux)<0? -(ux) : (ux))>best) ti1=1, scale1=ux, skew1=vx, scale2=A/ux, skew2=uy/ux, to2=0, best=((ux)<0? -(ux) : (ux));
    if (((uy)<0? -(uy) : (uy))>best) ti1=1, scale1=uy, skew1=vy, scale2=-A/uy, skew2=ux/uy, to2=1, best=((uy)<0? -(uy) : (uy));
    if (((vx)<0? -(vx) : (vx))>best) ti1=0, scale1=vx, skew1=ux, scale2=-A/vx, skew2=vy/vx, to2=0, best=((vx)<0? -(vx) : (vx));
    if (((vy)<0? -(vy) : (vy))>best) ti1=0, scale1=vy, skew1=uy, scale2=A/vy, skew2=vx/vy, to2=1, best=((vy)<0? -(vy) : (vy));
    if (scale1==0 || scale2==0)
 return OK;
    dzdx = (vy*uz - uy*vz) / A;
    dzdy = (ux*vz - vx*uz) / A;
    osz -= osx*dzdx + osy*dzdy;
    minx = miny = DXD_MAX_FLOAT;
    maxx = maxy = -DXD_MAX_FLOAT;
    x = ( (-osx)*vy - (-osy)*vx) / A; y = (- (-osx)*uy + (-osy)*ux) / A; if (x<minx) minx = x; if (x>maxx) maxx = x; if (y<miny) miny = y; if (y>maxy) maxy = y;;
    x = ( (-osx+onx)*vy - (-osy)*vx) / A; y = (- (-osx+onx)*uy + (-osy)*ux) / A; if (x<minx) minx = x; if (x>maxx) maxx = x; if (y<miny) miny = y; if (y>maxy) maxy = y;;
    x = ( (-osx)*vy - (-osy+ony-1)*vx) / A; y = (- (-osx)*uy + (-osy+ony-1)*ux) / A; if (x<minx) minx = x; if (x>maxx) maxx = x; if (y<miny) miny = y; if (y>maxy) maxy = y;;
    x = ( (-osx+onx)*vy - (-osy+ony-1)*vx) / A; y = (- (-osx+onx)*uy + (-osy+ony-1)*ux) / A; if (x<minx) minx = x; if (x>maxx) maxx = x; if (y<miny) miny = y; if (y>maxy) maxy = y;;
    extra = 1.0 / ((((scale1)<0? -(scale1) : (scale1)))<(((scale2)<0? -(scale2) : (scale2)))? (((scale1)<0? -(scale1) : (scale1))) : (((scale2)<0? -(scale2) : (scale2))));
    minx-=extra; maxx+=extra;
    miny-=extra; maxy+=extra;
    if (minx<0) minx = 0; else if (minx>inx-1) minx = inx-1;
    if (maxx<0) maxx = 0; else if (maxx>inx-1) maxx = inx-1;
    if (miny<0) miny = 0; else if (miny>iny-1) miny = iny-1;
    if (maxy<0) maxy = 0; else if (maxy>iny-1) maxy = iny-1;
    isx = ((minx)>=0 || (int)(minx)==minx? (int)(minx) : (int)(minx)-1);
    ikx = ((maxx)<=0 || (int)(maxx)==maxx? (int)(maxx) : (int)(maxx)+1) - isx + 1;
    isy = ((miny)>=0 || (int)(miny)==miny? (int)(miny) : (int)(miny)-1);
    iky = ((maxy)<=0 || (int)(maxy)==maxy? (int)(maxy) : (int)(maxy)+1) - isy + 1;
    if (ikx==0 || iky==0) return OK;
    osx += isx * ux + isy * vx;
    osy += isx * uy + isy * vy;
    if (contrary) to1=1, ti2=0;
    else to1=0, ti2=1;
    if (to2) {float t=osx; osx=osy; osy=t;};
    if (ti1) {int t=ikx; ikx=iky; iky=t;};
    nx1 = ikx;
    nk = ((((skew1)<0? -(skew1) : (skew1))*ikx)<=0 || (int)(((skew1)<0? -(skew1) : (skew1))*ikx)==((skew1)<0? -(skew1) : (skew1))*ikx? (int)(((skew1)<0? -(skew1) : (skew1))*ikx) : (int)(((skew1)<0? -(skew1) : (skew1))*ikx)+1);
    nc = ((((scale1)<0? -(scale1) : (scale1))*iky)<=0 || (int)(((scale1)<0? -(scale1) : (scale1))*iky)==((scale1)<0? -(scale1) : (scale1))*iky? (int)(((scale1)<0? -(scale1) : (scale1))*iky) : (int)(((scale1)<0? -(scale1) : (scale1))*iky)+1);
    ny1 = nk + nc + 1;
    osx2 = ((osx)>=0 || (int)(osx)==osx? (int)(osx) : (int)(osx)-1);
    osy1 = osx - osx2;
    if (scale1<0) osy1+=nc, osx2-=nc;
    if (skew1<0) osy1+=nk, osx2-=nk;
    if (ti1) {int t=ikx; ikx=iky; iky=t;};
    if (to1) {int t=nx1; nx1=ny1; ny1=t;};
    osy2 = osy - skew2 * osy1;
    ic1 = (RGBColor *) DXAllocateLocalZero(nx1 * ny1 * sizeof(*ic1));
    io1 = (float *) DXAllocateLocalZero(nx1 * ny1 * sizeof(*io1));
    if (!ic1 || !io1)
 return ERROR;
    DXMarkTimeLocal("overhead");
    rc = shear(ic, cstc, cmap,
   io, csto, omap,
   1.0,1.0,
   idx,idy,isx,isy,ikx,iky,
   0,0,0,0,
   (Pointer)ic1,type_colors,io1,
   0,osy1,nx1,ny1,
   ti1,scale1,skew1,to1);
    if (!rc) goto error;
    DXMarkTimeLocal("shear 1");
    rc = shear((Pointer)ic1, 0, NULL,
   (Pointer)io1, 0, NULL,
   cmul,omul,
   1,nx1,0,0,nx1,ny1,
   osz,dzdx,dzdy,clip,
   oc,otype,NULL,
   osx2,osy2,onx,ony,
   ti2,scale2,skew2,to2);
    if (!rc) goto error;
    DXMarkTimeLocal("shear 2");
    DXFree((Pointer)ic1);
    DXFree((Pointer)io1);
    return OK;
error:
    DXFree((Pointer)ic1);
    DXFree((Pointer)io1);
    return ERROR;
}
Error
_dxf_CompositePlane(struct buffer *b, float osx, float osy, float osz,
      int clip,
      float ux, float uy, float uz, float vx, float vy, float vz,
      Pointer ic, char cstc, RGBColor *cmap,
      Pointer io, char csto, float *omap,
      float cmul, float omul,
      int idx, int idy, int inx, int iny
) {
    return plane(ic, cstc, cmap, io, csto, omap, cmul, omul,
   idx, idy, inx, iny,
   ux, uy, uz, vx, vy, vz,
   (Pointer)b->u.big,
   b->pix_type==pix_fast? type_fast : type_big,
   b->width, b->height, osx, osy, osz, clip);
}
