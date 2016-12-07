/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
#define SPLIT(z1,z2, p1,p2, v1,v2) {\
    float a = (nearPlane-z1) / (z2-z1); /* XXX */\
    float abar = 1.0-a;\
    RGBColor *c1, *c2;\
    float *o1, *o2;\
    xpos->x = p1->x*abar + p2->x*a;\
    xpos->y = p1->y*abar + p2->y*a;\
    xpos->z = nearPlane;\
    xpos++;\
    if (interp_colors) {\
	if (fcolors) {\
	    if (fcst) {\
		xfc->r = fcolors->r;\
		xfc->g = fcolors->g;\
		xfc->b = fcolors->b;\
	    } else {\
		if (cmap) {\
		    c1 = cmap + (((unsigned char *)fcolors)[v1]);\
		    c2 = cmap + (((unsigned char *)fcolors)[v2]);\
		} else {\
		    c1 = ((RGBColor *)fcolors) + v1;\
		    c2 = ((RGBColor *)fcolors) + v2;\
		}\
		xfc->r = c1->r*abar + c2->r*a;\
		xfc->g = c1->g*abar + c2->g*a;\
		xfc->b = c1->b*abar + c2->b*a;\
	    }\
	    xfc++;\
	}\
	if (bcolors) {\
	    if (bcst) {\
		xbc->r = bcolors->r;\
		xbc->g = bcolors->g;\
		xbc->b = bcolors->b;\
	    } else {\
		if (cmap) {\
		    c1 = cmap + (((unsigned char *)bcolors)[v1]);\
		    c2 = cmap + (((unsigned char *)bcolors)[v2]);\
		} else {\
		    c1 = ((RGBColor *)bcolors) + v1;\
		    c2 = ((RGBColor *)bcolors) + v2;\
		}\
		xbc->r = c1->r*abar + c2->r*a;\
		xbc->g = c1->g*abar + c2->g*a;\
		xbc->b = c1->b*abar + c2->b*a;\
	    }\
	    xbc++;\
	}\
	if (opacities) {\
	    if (ocst) {\
		*xop = *opacities;\
	    } else {\
		if (omap) {\
		    o1 = omap + (((unsigned char *)opacities)[v1]);\
		    o2 = omap + (((unsigned char *)opacities)[v2]);\
		} else {\
		    o1 = ((float *)opacities) + v1;\
		    o2 = ((float *)opacities) + v2;\
		}\
		*xop = (*o1)*abar + (*o2)*a;\
	    }\
	    xop++;\
	}\
    }\
    if (interp_normals) {\
	if (ncst) {\
	    xnrm->x = normals->x;\
	    xnrm->y = normals->y;\
	    xnrm->z = normals->z;\
	} else {\
	    Vector *n1 = normals + v1;\
	    Vector *n2 = normals + v2;\
	    xnrm->x = n1->x*abar + n2->x*a;\
	    xnrm->y = n1->y*abar + n2->y*a;\
	    xnrm->z = n1->z*abar + n2->z*a;\
	}\
	xnrm++;\
    }\
    nxpt++;\
}

#include <dxconfig.h>



#define ADDTRI(A,B,C) {\
    xtri->p = nxpt+(A);\
    xtri->q = nxpt+(B);\
    xtri->r = nxpt+(C);\
    if (!interp_colors) {\
	if (fcolors) {\
	    if (fcst)      *xfc = *fcolors;\
	    else if (cmap) *xfc = cmap[((unsigned char *)fcolors)[i]];\
	    else           *xfc = fcolors[i];\
	    xfc ++;\
	}\
	if (bcolors) {\
	    if (bcst)      *xbc = *bcolors;\
	    else if (cmap) *xbc = cmap[((unsigned char *)bcolors)[i]];\
	    else           *xbc = bcolors[i];\
	    xbc ++;\
	}\
	if (opacities) {\
	    if (ocst)      *xop = *opacities;\
	    else if (omap) *xop = omap[((unsigned char *)opacities)[i]];\
	    else           *xop = opacities[i];\
	    xop ++;\
	}\
    }\
    if (!interp_normals && normals) {\
	if (ncst) *xnrm++ = *normals;\
	else      *xnrm++ = normals[i];\
    }\
    if (indices)\
	xindices[nxtri] = indices[i];\
    xtri++;\
    nxtri++;\
}


#define ADDPT(v) {\
    *xpos++ = positions[v];\
    if (interp_colors) {\
	if (fcolors) {\
	    if (fcst)      *xfc = *fcolors;\
	    else if (cmap) *xfc = cmap[((unsigned char *)fcolors)[v]];\
	    else           *xfc = fcolors[v];\
	    xfc ++;\
	}\
	if (bcolors) {\
	    if (bcst)      *xbc = *bcolors;\
	    else if (cmap) *xbc = cmap[((unsigned char *)bcolors)[v]];\
	    else           *xbc = bcolors[v];\
	    xbc ++;\
	}\
	if (opacities) {\
	    if (ocst)      *xop = *opacities;\
	    else if (omap) *xop = omap[((unsigned char *)opacities)[v]];\
	    else           *xop = opacities[v];\
	    xop ++;\
	}\
    }\
    if (interp_normals) {\
	if (ncst) *xnrm++ = *normals;\
	else      *xnrm++ = normals[v];\
    }\
    nxpt++;\
}


#define TRYCLIP(z1,z2,z3, p1,p2,p3, v1,v2,v3)\
    if (z1>nearPlane) {\
	if (z2>nearPlane && z3>nearPlane)\
	    continue;\
	else if (z2>nearPlane) {\
	    SPLIT(z3,z1, p3,p1, v3,v1);		/*   3   */\
	    SPLIT(z3,z2, p3,p2, v3,v2);		/*  a b  */\
	    ADDPT(v3);				/* 1   2 */\
	    ADDTRI(-3,-2,-1);\
	} else if (z3>nearPlane) {\
	    SPLIT(z2,z3, p2,p3, v2,v3);		/*   2   */\
	    SPLIT(z2,z1, p2,p1, v2,v1);		/*  a b  */\
	    ADDPT(v2);				/* 3   1 */\
	    ADDTRI(-3,-2,-1);\
	} else {\
	    SPLIT(z1,z2, p1,p2, v1,v2);		/* 2   3 */\
	    SPLIT(z1,z3, p1,p3, v1,v3);		/*  a b  */\
	    ADDPT(v2);				/*   1   */\
	    ADDPT(v3);\
	    ADDTRI(-2,-3,-4);\
	    ADDTRI(-3,-2,-1);\
	}\
    }
