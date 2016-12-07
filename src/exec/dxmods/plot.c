/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>

#include <dx/dx.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#if defined(HAVE_STRING_H)
#include <string.h>
#endif
#include "_autocolor.h"
#include "plot.h"
#include "autoaxes.h"

Error _dxfGetAnnotationColors(Object, Object,
                              RGBColor, RGBColor, RGBColor, RGBColor,
                              RGBColor,
                              RGBColor *, RGBColor *,
                              RGBColor *, RGBColor *, RGBColor *, int *);

extern Error _dxfCheckLocationsArray(Array, int *, float **); /* from libdx/axes.c */

struct legendargstruct {
    int islabel ;
    char *label;
    int ismark ;
    int scatter ;
    char *mark;
};

struct plotargstruct {
    int ismark;
    char *mark;
    int markevery;
    float markscale;
    int scatter;
    int column;
    int subfields;
    int fieldNumber;
};

struct arg {
    Object ino;
    int needtoclip;
    Point clippoint1;
    Point clippoint2;
    float epsilonx;
    float epsilony;
    float xlog;
    float ylog;
    int mark;
    char *marker;
    int markevery;
    float markscale;
    int scatter;
    int column;
    int siblingCount;
    int siblingNumber;
};

static RGBColor DEFAULTTICCOLOR = {1.0, 1.0, 0.0};
static RGBColor DEFAULTLABELCOLOR = {1.0, 1.0, 1.0};
static RGBColor DEFAULTAXESCOLOR = {1.0, 1.0, 0.0};
static RGBColor DEFAULTGRIDCOLOR = {0.3, 0.3, 0.3};
static RGBColor DEFAULTBACKGROUNDCOLOR = {0.05, 0.05, 0.05};

static Error GetPlotHeight(Object, int, float *, float *, float *);

static Error  plottask(Pointer);
static Error  DoPlot(Object,int,Point,Point,float,float, int, int,
                     struct plotargstruct *);
static Error  DrawLine(Array, Array, int *, int *, int, float, float,
                       int, RGBColor, Array);
static Error  PlotDepPositions(struct arg *);
static Error  PlotBarDepPositions(struct arg *);
static Error  PlotDepConnections(struct arg *);
static Object MakeLabel(char *, RGBColor, int, float *, float, Object,
                        int, int, char *, RGBColor);
static Error  MakeLegend(Object, Object, int *, float *, float, Object,
                         struct legendargstruct *, RGBColor);
static Error  DoPlotField(Object,int,Point,Point,float, float, int, int,
                          struct plotargstruct *);
static Error  GetLineColor(Object, RGBColor *);
static Error  AddX(Array, int *, Array, int *, float, float, float, float,
                   int, RGBColor, Array);
static Error  AddStar(Array, int *, Array, int *, float, float, float,
                      float, int, RGBColor, Array);
static Error  AddTri(Array, int *, Array, int *, float, float, float,
                     float, int, RGBColor, Array);
static Error  AddSquare(Array, int *, Array, int *, float, float, float,
                        float, int, RGBColor, Array);
static Error  AddCircle(Array, int *, Array, int *, float, float, float,
                        float, int, RGBColor, Array);
static Error  AddDiamond(Array, int *, Array, int *, float, float, float,
                         float, int, RGBColor, Array);
static Error  AddDot(Array, int *, Array, int *, float, float, float,
                     float, int, RGBColor, Array);
#if 0
static Error  DrawDashed(Array, Array, int *, int *, int,
                         float, float, float, float, float, float, float,
                         float *, int *);
#endif

#define E .0001                                                  /* fuzz */
#define EPS_SCALE     80                                         /* reduction for markers*/
#define FFLOOR(x) ((x)+E>0? (int)((x)+E) : (int)((x)+E)-1)       /* fuzzy floor */
#define FCEIL(x) ((x)-E>0? (int)((x)-E)+1 : (int)((x)-E))        /* fuzzy ceiling */

#define FLOOR(x) (x)>0? (int)(x) : (int)((x)-1)                  /* floor */
#define CEIL(x) (x)>0? (int)((x)+1) : (int)(x)                   /* ceiling */


#define ABS(x)      (((x) < 0.0) ? (- (x)) : (x))

static RGBColor DEFAULTCOLOR = {1.0, 1.0, 1.0};
static char nullstring[11] = "NULLSTRING";

/* this is a circle */
Point pointsarray[]={
                        { 1.00,  0.00,  0.00 },
                        { 0.81,  0.59,  0.00 },
                        { 0.31,  0.95,  0.00 },
                        { -0.31,  0.95,  0.00 },
                        { -0.81,  0.59,  0.00 },
                        { -1.00,  0.00,  0.00 },
                        { -0.81, -0.59,  0.00 },
                        { -0.31, -0.95,  0.00 },
                        { 0.31, -0.95,  0.00 },
                        { 0.81, -0.59,  0.00 }
                    };

Error
m_Plot(Object *in, Object *out) {
    Pointer *axeshandle=NULL;
    int i, adjust, ticks, ticks2, tx, ty, num, background;
    int needtoclip=0, frame;
    float floatzero=0.0, labelscale=1.0;
    int xlog, ylog, ylog2, secondplot, intzero=0, intone=1;
    int numtickitems, rank, shape[32], *ticks_ptr, autoadjust;
    float aspect, ascent, descent, maxstring, cp[4], cp2[2], *actualcorners=NULL;
    float linepositionx, linepositiony, linelength, *ptr;
    float mindata, maxdata, minpos, maxpos;
    float minplotdata, maxplotdata, minplotpos, maxplotpos;
    float minplotdata2, maxplotdata2;
    float scalingx, scalingy;
    Object legendscreen=NULL, subin, scaled=NULL;
    Object ino=NULL, ino2=NULL, font=NULL, legend=NULL;
    Object olegend=NULL, outlegend=NULL, legendo=NULL;
    Object newcorners=NULL, newcorners2=NULL;
    int givencorners, givencorners2;
    Object tmpino=NULL, tmpino2=NULL;
    Group group=NULL;
    Field emptyfield=NULL;
    Matrix matrix, translation1, translation2, xform, scaling;
    Point clippoint1, clippoint2;
    Object outgroup=NULL;
    float xwidth, ywidth, ywidth2;
    char *labelx, *labely, *labely2, *labelz, *fontname;
    char *aspectstring;
    Class class;
    int   grid;
    Object withaxes=NULL, withaxes2=NULL;
    Type type;
    char *plottypex, *plottypey, *plottypey2;
    char ptypex[30], ptypey[30], ptypey2[30];
    char marker[30], plotlabel[60];
    Category category;
    float xscaling, yscaling, yscaling2=0, epsilon, epsilonx, epsilony;
    RGBColor ticcolor, labelcolor, axescolor, gridcolor, backgroundcolor;
    float *xptr=NULL, *list_ptr;
    Array xticklocations=NULL, yticklocations=NULL, y1ticklocations=NULL;
    int numlist, dofixedfontsize=0;
    float fixedfontsize=0.1;

    struct legendargstruct legendarg;
    struct plotargstruct plotarg;

    strcpy(marker, "");
    strcpy(plotlabel,"");

    legendarg.islabel=0;
    legendarg.label=plotlabel;
    legendarg.ismark=0;
    legendarg.mark=marker;
    legendarg.scatter=0;

    plotarg.ismark = 0;
    plotarg.mark = marker;
    plotarg.markevery = 1;
    plotarg.markscale = 1;
    plotarg.scatter = 0;
    plotarg.subfields = 1;


    /* note: attributes (such as markers) might be set at the composite
     * field level. Therefore attributes should be checked for at each
     * level of recursion, and if found, passed down */


    if (!in[0]) {
        DXSetError(ERROR_BAD_PARAMETER, "#10000","input");
        goto error;
    }

    if (!_dxfFieldWithInformation(in[0])) {
        out[0] = in[0];
        return OK;
    }

    if (!(class=DXGetObjectClass(in[0])))
        return ERROR;
    if ((class != CLASS_GROUP)&&
            (class != CLASS_FIELD)&&(class != CLASS_XFORM)) {
        DXSetError(ERROR_BAD_PARAMETER,"#10190","input");
        return ERROR;
    }

    if (!in[1]) {
        labelx = "x";
        labely = "y";
    } else {
        if (!DXExtractNthString(in[1], 0, &labelx)) {
            DXSetError(ERROR_BAD_PARAMETER, "#10204", "labels", 2);
            goto error;
        }
        if (!DXExtractNthString(in[1], 1, &labely)) {
            DXSetError(ERROR_BAD_PARAMETER, "#10204", "labels", 2);
            goto error;
        }
        if (DXExtractNthString(in[1], 2, &labelz)) {
            DXSetError(ERROR_BAD_PARAMETER, "#10204", "labels", 2);
            goto error;
        }
    }

    ticks = tx = ty = 0;
    if (!in[2]) {
        ticks = 10;
    } else {
        if (!(DXGetObjectClass(in[2])==CLASS_ARRAY)) {
            DXSetError(ERROR_BAD_PARAMETER, "#10012", "ticks", 2);
            goto error;
        }
        if (!DXGetArrayInfo((Array)in[2], &numtickitems, &type, &category,
                            &rank, shape)) {
            goto error;
        }
        if ((type!=TYPE_INT) || (category !=CATEGORY_REAL)) {
            DXSetError(ERROR_BAD_PARAMETER,"#10012","ticks", 2);
            goto error;
        }
        if (! ((rank == 0) || ((rank == 1)&&(shape[0]==1)))) {
            DXSetError(ERROR_BAD_PARAMETER,"#10012","ticks", 2);
            goto error;
        }

        if (numtickitems == 1) {
            if (!DXExtractInteger(in[2], &ticks)) {
                DXSetError(ERROR_BAD_PARAMETER,"#10012","ticks", 2);
                goto error;
            }
        } else if (numtickitems == 2) {
            ticks_ptr = (int *)DXGetArrayData((Array)in[2]);
            tx = ticks_ptr[0];
            ty = ticks_ptr[1];
            /*tz = 0.0;*/
        } else {
            DXSetError(ERROR_BAD_PARAMETER,"#10012", "ticks", 2);
            goto error;
        }
    }

    givencorners = 0;
    if (in[3]) {
        givencorners = 1;
        if (!DXExtractParameter(in[3], TYPE_FLOAT, 2, 2, (Pointer)cp)) {
            class = DXGetObjectClass(in[3]);
            if ((class != CLASS_FIELD)&& (class != CLASS_GROUP)) {
                DXSetError(ERROR_BAD_PARAMETER,"#10013", "corners", 2, 2);
                goto error;
            } else {
                if (!(DXStatistics(in[3], "positions",
                                   &minpos, &maxpos, NULL, NULL))) {
                    DXAddMessage("invalid corners object");
                    goto error;
                }
                if (!(DXStatistics(in[3], "data", &mindata, &maxdata, NULL, NULL))) {
                    DXAddMessage("invalid corners object");
                    goto error;
                }
                cp[0] = minpos;
                cp[1] = mindata;
                cp[2] = maxpos;
                cp[3] = maxdata;
                minplotpos = cp[0];
                maxplotpos = cp[2];
                minplotdata = cp[1];
                maxplotdata = cp[3];
            }
        }
    }

    /* good idea to check now if there's another plot */
    secondplot = 0;
    if (in[13]) {
        /* we have a second plot to put in the first one */
        secondplot = 1;
        class = DXGetObjectClass(in[13]);
        if ((class != CLASS_GROUP)&&
                (class != CLASS_FIELD)&&(class != CLASS_XFORM)) {
            DXSetError(ERROR_BAD_PARAMETER,"#10190", "input2");
            return ERROR;
        }
        /* assume that the limits will be different */
        needtoclip=1;
        /* make a group to hold both sets of lines, to get all the labels */
        group = DXNewGroup();
        if (!group)
            goto error;
        /* DXReference(in[0]); */
        if (!DXSetMember(group,NULL,in[0]))
            goto error;
        emptyfield = DXNewField();
        DXSetAttribute((Object)emptyfield,"label",(Object)DXNewString(nullstring));
        if (!DXSetMember(group,NULL,(Object)emptyfield))
            goto error;
        emptyfield = NULL;
        if (!DXSetMember(group,NULL,in[13]))
            goto error;
        /* get the x positions from the union of the two sets of lines */
        if (!(DXStatistics((Object)group,"positions",
                           &minplotpos, &maxplotpos, NULL, NULL))) {
            goto error;
        }
        /* get the y's from only this set */
        if (!(DXStatistics(in[0],"data",
                           &minplotdata, &maxplotdata, NULL, NULL)))
            goto error;
        if (!givencorners) {
            cp[0] = minplotpos;
            cp[1] = minplotdata;
            cp[2] = maxplotpos;
            cp[3] = maxplotdata;
        }
        givencorners = 1;
    } else {
        if (!givencorners) {
            if (!(DXStatistics(in[0],"positions",
                               &minplotpos, &maxplotpos, NULL, NULL)))
                goto error;
            if (!(DXStatistics(in[0],"data",
                               &minplotdata, &maxplotdata, NULL, NULL)))
                goto error;
            cp[0] = minplotpos;
            cp[1] = minplotdata;
            cp[2] = maxplotpos;
            cp[3] = maxplotdata;
        }
    }

    adjust =0;
    if (in[4]) {
        if (!DXExtractInteger(in[4], &adjust)) {
            DXSetError(ERROR_BAD_PARAMETER,"#10070", "adjust");
            goto error;
        }
        if ((adjust != 0)&&(adjust != 1)) {
            DXSetError(ERROR_BAD_PARAMETER,"#10070", "adjust");
            goto error;
        }
    }

    frame = 0;
    if (in[5]) {
        if (!DXExtractInteger(in[5], &frame)) {
            DXSetError(ERROR_BAD_PARAMETER, "#10040", "frame", 0, 2);
            goto error;
        }
        if ((frame < 0) || (frame > 2)) {
            DXSetError(ERROR_BAD_PARAMETER,"#10040", "frame", 0, 2);
            goto error;
        }
    }

    if (in[6]) {
        if (!DXExtractNthString(in[6], 0, &plottypex)) {
            DXSetError(ERROR_BAD_PARAMETER,"#10205", "type");
            goto error;
        }
        if (!DXExtractNthString(in[6], 1, &plottypey)) {
            plottypey = plottypex;
        }
    } else {
        plottypex="lin";
        plottypey="lin";
    }

    i= 0;
    while (i <= 29 && plottypex[i] != '\0') {
        ptypex[i]= tolower(plottypex[i]);
        i++;
    }
    ptypex[i] = '\0';
    i= 0;
    while (i <= 29 && plottypey[i] != '\0') {
        ptypey[i]= tolower(plottypey[i]);
        i++;
    }
    ptypey[i] = '\0';


    if (!strcmp(ptypex,"log10"))
        strcpy(ptypex, "log");
    if (!strcmp(ptypex,"linear"))
        strcpy(ptypex, "lin");

    if (!strcmp(ptypey,"log10"))
        strcpy(ptypey, "log");
    if (!strcmp(ptypey,"linear"))
        strcpy(ptypey, "lin");

    if (strcmp(ptypex,"lin")&&(strcmp(ptypex,"log"))) {
        DXSetError(ERROR_BAD_PARAMETER,"#10211", "type", "log, lin");
        goto error;
    }
    if (strcmp(ptypey,"lin")&&(strcmp(ptypey,"log"))) {
        DXSetError(ERROR_BAD_PARAMETER,"#10211", "type", "log, lin");
        goto error;
    }
    if (!strcmp(ptypex,"lin"))
        xlog = 0;
    else
        xlog = 1;
    if (!strcmp(ptypey,"lin"))
        ylog = 0;
    else
        ylog = 1;

    if (in[7]) {
        if (!DXExtractInteger(in[7], &grid)) {
            DXSetError(ERROR_BAD_PARAMETER,"#10040", "grid", 0, 3);
            goto error;
        }
        if ((grid < 0) || (grid > 3)) {
            DXSetError(ERROR_BAD_PARAMETER,"#10040", "grid", 0, 3);
            goto error;
        }
        /* if grid is set, adjust should be 1 */
        if ((adjust == 0)&&(grid>0))
            DXWarning("setting grid sets adjust parameter to be 1");
        adjust = 1;
    } else {
        grid = 0;
    }

    if (in[8]) {
        if (DXExtractString(in[8], &aspectstring)) {
            if (strcmp(aspectstring,"inherent")) {
                DXSetError(ERROR_BAD_PARAMETER,
                           "aspect must be a positive number or the string \'inherent\'");
                return ERROR;
            }
            /* if there's a second plot, then "inherent" doesn't make sense */
            if (!in[13])
                autoadjust = 0;
            else {
                DXWarning("Because a second plot is specified, inherent is overruled");
                autoadjust = 1;
                aspect = 1.0;
            }
        } else {
            if (!DXExtractFloat(in[8], &aspect)) {
                DXSetError(ERROR_BAD_PARAMETER,
                           "aspect must be a positive number or the string \'inherent\'");
                return ERROR;
            }
            if (aspect <= 0.0) {
                DXSetError(ERROR_BAD_PARAMETER,"#10090", "aspect");
                return ERROR;
            }
            autoadjust = 1;
        }
    } else {
        aspect = 1.0;
        autoadjust = 1;
    }

    /* in[9] is the colors list */
    /* in[10] is the color objects list */
    if (!_dxfGetAnnotationColors(in[9], in[10], DEFAULTTICCOLOR,
                                 DEFAULTLABELCOLOR, DEFAULTAXESCOLOR,
                                 DEFAULTGRIDCOLOR, DEFAULTBACKGROUNDCOLOR,
                                 &ticcolor, &labelcolor,
                                 &axescolor, &gridcolor, &backgroundcolor,
                                 &background))
        goto error;

    /* label scale */
    if (in[11]) {
        if (!DXExtractFloat(in[11], &labelscale)) {
            DXSetError(ERROR_BAD_PARAMETER,"#10090", "labelscale");
            goto error;
        }
        if (labelscale < 0) {
            DXSetError(ERROR_BAD_PARAMETER,"#10090", "labelscale");
            goto error;
        }
    }

    if (in[12]) {
        if (!DXExtractString(in[12], &fontname)) {
            DXSetError(ERROR_BAD_PARAMETER,"#10200","font");
            goto error;
        }
    } else {
        fontname = "standard";
    }

    if (in[14]) {
        /* label */
        if (!secondplot) {
            DXWarning("label2 specified without input2");
        }
        if (!DXExtractString(in[14], &labely2)) {
            DXSetError(ERROR_BAD_PARAMETER,"#10200", "label2");
            goto error;
        }
    } else {
        labely2 = "y2";
    }
    if (in[15]) {
        /*ticks*/
        if (!secondplot) {
            DXWarning("ticks2 specified without input2");
        }
        if (!DXExtractInteger(in[15], &ticks2)) {
            DXSetError(ERROR_BAD_PARAMETER,"#10030", "ticks2");
            goto error;
        }
    } else {
        ticks2 = 5;
    }

    givencorners2 = 0;
    if (in[16]) {
        if (!secondplot) {
            DXWarning("corners2 specified without input2");
        }
        givencorners2 = 1;
        if (!DXExtractParameter(in[16], TYPE_FLOAT, 2, 1, (Pointer)cp2)) {
            class = DXGetObjectClass(in[16]);
            if ((class != CLASS_FIELD)&& (class != CLASS_GROUP)) {
                DXSetError(ERROR_BAD_PARAMETER,"#10014", "corners2", 2);
                goto error;
            } else {
                if (!(DXStatistics(in[16], "data", &mindata, &maxdata, NULL, NULL))) {
                    DXAddMessage("invalid corners2 object");
                    goto error;
                }
                cp2[0] = mindata;
                cp2[1] = maxdata;
                minplotdata2 = mindata;
                maxplotdata2 = maxdata;
            }
        }
    } else {
        if (secondplot) {
            if (!(DXStatistics(in[13], "data", &minplotdata2,
                               &maxplotdata2,NULL, NULL))) {
                goto error;
            }
        }
    }

    if (in[17]) {
        if (!secondplot) {
            DXWarning("type2 specified without input2");
        }
        if (!DXExtractNthString(in[17], 0, &plottypey2)) {
            DXSetError(ERROR_BAD_PARAMETER,"#10200", "type2");
            goto error;
        }
    } else {
        plottypey2="lin";
    }
    if (secondplot) {
        i= 0;
        while (i <= 29 && plottypey2[i] != '\0') {
            ptypey2[i]= tolower(plottypey2[i]);
            i++;
        }
        ptypey2[i] = '\0';

        if (!strcmp(ptypey2,"log10"))
            strcpy(ptypey2, "log");
        if (!strcmp(ptypey,"linear"))
            strcpy(ptypey2, "lin");

        if (strcmp(ptypey2,"lin")&&(strcmp(ptypey2,"log"))) {
            DXSetError(ERROR_BAD_PARAMETER,"#10203", "type2", "log, lin");
            goto error;
        }
        if (!strcmp(ptypey2,"lin"))
            ylog2 = 0;
        else
            ylog2 = 1;
    }

    /* If specified, these should override the corners and the tics params */
    /* user-forced xtic locations */

    if (in[18]) {
        if (!(DXGetObjectClass(in[18])==CLASS_ARRAY)) {
            DXSetError(ERROR_BAD_PARAMETER,"xlocations must be a scalar list");
            return ERROR;
        }
        if (!_dxfCheckLocationsArray((Array) in[18], &num, &xptr))
            goto error;
        /* cp[0]=xptr[0];
           cp[2]=xptr[num-1]; XXX */
        DXFree((Pointer)xptr);
        xptr = NULL;
        xticklocations = (Array)in[18];
    }
    if (in[19]) {
        if (!(DXGetObjectClass(in[19])==CLASS_ARRAY)) {
            DXSetError(ERROR_BAD_PARAMETER,"y1locations must be a scalar list");
            return ERROR;
        }
        if (!_dxfCheckLocationsArray((Array) in[19], &num, &xptr))
            goto error;
        /* cp[1]=xptr[0];
           cp[3]=xptr[num-1]; */
        DXFree((Pointer)xptr);
        xptr = NULL;
        yticklocations = (Array)in[19];
    }
    if (in[20]) {
        if (!(DXGetObjectClass(in[20])==CLASS_ARRAY)) {
            DXSetError(ERROR_BAD_PARAMETER,"y2locations must be a scalar list");
            return ERROR;
        }
        if (!_dxfCheckLocationsArray((Array) in[20], &num, &xptr))
            goto error;
        /* cp2[0]=xptr[0];
           cp2[1]=xptr[num-1]; */
        DXFree((Pointer)xptr);
        xptr = NULL;
        y1ticklocations = (Array)in[20];
    }

    if (in[21]) {
        if (!(DXGetObjectClass(in[21])==CLASS_ARRAY)) {
            DXSetError(ERROR_BAD_PARAMETER,"xlabels must be a string list");
            return ERROR;
        }
        if (!DXGetArrayInfo((Array)in[21], &numlist, NULL, NULL, NULL, NULL))
            goto error;
        if (!in[18]) {
            /* need to make an array to use. It will go from 0 to n-1 */
            xticklocations = DXNewArray(TYPE_FLOAT,CATEGORY_REAL, 0);
            if (!xticklocations)
                goto error;
            list_ptr = DXAllocate(numlist*sizeof(float));
            if (!list_ptr)
                goto error;
            for (i=0; i<numlist; i++)
                list_ptr[i]=(float)i;
            if (!DXAddArrayData(xticklocations, 0, numlist, list_ptr))
                goto error;
            DXFree((Pointer)list_ptr);
        }
    }
    /* user-forced xtic labels; need to set in[19] */
    if (in[22]) {
        if (!(DXGetObjectClass(in[22])==CLASS_ARRAY)) {
            DXSetError(ERROR_BAD_PARAMETER,"y1labels must be a string list");
            return ERROR;
        }
        if (!DXGetArrayInfo((Array)in[22], &numlist, NULL, NULL, NULL, NULL))
            goto error;
        if (!in[19]) {
            /* need to make an array to use. It will go from 0 to n-1 */
            yticklocations = DXNewArray(TYPE_FLOAT,CATEGORY_REAL, 0);
            if (!yticklocations)
                goto error;
            list_ptr = DXAllocate(numlist*sizeof(float));
            if (!list_ptr)
                goto error;
            for (i=0; i<numlist; i++)
                list_ptr[i]=(float)i;
            if (!DXAddArrayData(yticklocations, 0, numlist, list_ptr))
                goto error;
            DXFree((Pointer)list_ptr);
        }
    }
    /* user-forced ytic labels; need to set in[20] */
    if (in[23]) {
        if (!(DXGetObjectClass(in[23])==CLASS_ARRAY)) {
            DXSetError(ERROR_BAD_PARAMETER,"y2labels must be a string list");
            return ERROR;
        }
        if (!DXGetArrayInfo((Array)in[23], &numlist, NULL, NULL, NULL, NULL))
            goto error;
        if (!in[20]) {
            /* need to make an array to use. It will go from 0 to n-1 */
            y1ticklocations = DXNewArray(TYPE_FLOAT,CATEGORY_REAL, 0);
            if (!y1ticklocations)
                goto error;
            list_ptr = DXAllocate(numlist*sizeof(float));
            if (!list_ptr)
                goto error;
            for (i=0; i<numlist; i++)
                list_ptr[i]=(float)i;
            if (!DXAddArrayData(y1ticklocations, 0, numlist, list_ptr))
                goto error;
            DXFree((Pointer)list_ptr);
        }
    }

    if (in[24]) {
        if (!DXExtractInteger(in[24], &dofixedfontsize)) {
            DXSetError(ERROR_DATA_INVALID,"usefixedfontsize must be either 0 or 1");
            goto error;
        }
        if ((dofixedfontsize != 0)&&(dofixedfontsize != 1)) {
            DXSetError(ERROR_DATA_INVALID,"usefixedfontsize must be either 0 or 1");
            goto error;
        }
    }
    if (in[25]) {
        if (!DXExtractFloat(in[25], &fixedfontsize)) {
            DXSetError(ERROR_DATA_INVALID,"fixedfontsize must be a positive scalar");
            goto error;
        }
        if (fixedfontsize < 0) {
            DXSetError(ERROR_DATA_INVALID,"fixedfontsize must be a positive scalar");
            goto error;
        }
    }


    /* First handle the first plot */
    /* check to see if clipping will be necessary */
    if (givencorners) {
        needtoclip = 1;
        if (!xlog)
            xwidth = cp[2]-cp[0];
        else
            xwidth = FCEIL(log10(cp[2]))-FFLOOR(log10(cp[0]));

        if (!ylog)
            ywidth = cp[3]-cp[1];
        else
            ywidth = FCEIL(log10(cp[3]))-FFLOOR(log10(cp[1]));

        if ((xwidth==0.0)||(ywidth==0.0)) {
            DXSetError(ERROR_DATA_INVALID,"#11835");
            goto error;
        }

        clippoint1 = DXPt(cp[0], cp[1], -1.0);
        clippoint2 = DXPt(cp[2], cp[3],  1.0);
    } else {
        if (!xlog) {
            xwidth=maxplotpos-minplotpos;
            if (xwidth == 0.0) {
                if (maxplotpos != 0)
                    xwidth = maxplotpos*(2*ZERO_HEIGHT_FRACTION);
                else
                    xwidth = 2;
                if (xwidth < 0)
                    xwidth = -xwidth;
            }
        } else {
            xwidth=FCEIL(log10(maxplotpos))-FFLOOR(log10(minplotpos));
            if (xwidth == 0.0) {
                xwidth =
                    FCEIL(log10(maxplotpos*(1.0+ZERO_HEIGHT_FRACTION))) -
                    FFLOOR(log10(minplotpos*(1.0-ZERO_HEIGHT_FRACTION)));
            }
            if (xwidth < 0)
                xwidth = -xwidth;
        }
        if (!GetPlotHeight(in[0], ylog, &ywidth, &maxplotdata, &minplotdata))
            goto error;

        if (!ylog) {
            if (ywidth == 0.0) {
                if (maxplotdata != 0.0)
                    ywidth = maxplotdata*(2*ZERO_HEIGHT_FRACTION);
                else
                    ywidth = 2;
                if (ywidth < 0)
                    ywidth = -ywidth;
            }
        } else {
            if (ywidth == 0.0) {
                ywidth =
                    FCEIL(log10(maxplotdata*(1.0+ZERO_HEIGHT_FRACTION)))
                    -FFLOOR(log10(minplotdata*(1.0-ZERO_HEIGHT_FRACTION)));
            }
            if (ywidth < 0)
                ywidth = -ywidth;
        }
        clippoint1 = DXPt(minplotpos,minplotdata,-1.0);
        clippoint2 = DXPt(maxplotpos,maxplotdata, 1.0);
    }


    /* need to get information about the transformation, if there is one */
    if (autoadjust) {
        if (DXGetObjectClass(in[0]) == CLASS_XFORM) {
            if (!DXGetXformInfo((Xform)in[0], &subin, &matrix)) {
                goto error;
            }
            /* we're about to give this guy a new top level xform */
            /* first call invalidiate connections */
            if (!DXInvalidateConnections(subin))
                goto error;
            tmpino = DXCopy(subin,COPY_STRUCTURE);
        } else {
            /* first call invalidiate connections */
            if (!DXInvalidateConnections(in[0]))
                goto error;
            tmpino = DXCopy(in[0],COPY_STRUCTURE);
        }
        scalingx = 1.0;
        scalingy = 1.0;

        if (!tmpino)
            goto error;
        /* this is the xform which will give us the right aspect ratio */
        /* it depends on whether the axes are log or lin */
        yscaling = aspect * xwidth *scalingx/ (ywidth * scalingy);
        xscaling = 1.0;
        matrix = DXScale(xscaling, yscaling, 1.0);
        if (!(ino = (Object)DXNewXform(tmpino,matrix)))
            goto error;
        tmpino = NULL;
    }
    /* else we are not autoadjusting */
    else {
        if (DXGetObjectClass(in[0]) == CLASS_XFORM) {
            if (!DXGetXformInfo((Xform)in[0], NULL, &matrix)) {
                goto error;
            }
            xscaling = matrix.A[0][0];
            yscaling = matrix.A[1][1];
        } else {
            xscaling = 1.0;
            yscaling = 1.0;
        }
        /* first call invalidiate connections */
        if (!DXInvalidateConnections(in[0]))
            goto error;
        ino = DXCopy(in[0],COPY_STRUCTURE);
        if (!ino)
            goto error;
    }

    /* once again, need to do diff things for logs */
    if (xlog)
        xwidth = xwidth*xscaling;
    if (ylog)
        ywidth = ywidth*yscaling;

    epsilon = xwidth/EPS_SCALE;
    epsilonx = epsilon/xscaling;
    epsilony = epsilon/yscaling;

    if (epsilon <=0.0) {
        DXSetError(ERROR_DATA_INVALID,"#11835");
        goto error;
    }

    if (!(DXCreateTaskGroup())) {
        goto error;
    }
    /* DoPlot() to do the recursive traversal. The AddTasks will
       be added here */
    if (!(DoPlot(ino,needtoclip,clippoint1,clippoint2, epsilonx, epsilony,
                 xlog, ylog, &plotarg)))
        goto error;
    if (!(DXExecuteTaskGroup())) {
        goto error;
    }

    /* figure out a good line length and position for caption */
    linepositionx = 0.95 ;
    linepositiony = 0.95 ;

    num=0;
    maxstring=0;
    /* linelength in pixels */
    linelength = 40;
    /* Now go back through to make the legend */
    legend = (Object)DXNewGroup();

    if (!strcmp(fontname,"standard")) {
        if (!(font=DXGetFont("variable", &ascent, &descent))) {
            return ERROR;
        }
    } else {
        if (!(font=DXGetFont(fontname, &ascent, &descent))) {
            return ERROR;
        }
    }

    if (!secondplot) {
        if (!(MakeLegend(ino, legend, &num, &maxstring, linelength, font,
                         &legendarg, labelcolor)))
            goto error;
    } else {
        if (!(MakeLegend((Object)group, legend, &num, &maxstring,linelength,font,
                         &legendarg, labelcolor)))
            goto error;
    }

    /* shift it */
    legendo =
        (Object)DXNewXform((Object)legend,
                           DXTranslate(DXPt(-(maxstring+linelength),0.0,0.0)));
    legend = NULL;

    if (!(legendscreen = (Object)DXNewScreen(legendo, SCREEN_VIEWPORT, 1))) {
        return ERROR;
    }
    legendo = NULL;

    outlegend =
        (Object)DXNewXform((Object)legendscreen, DXTranslate(DXPt(linepositionx,
                           linepositiony,
                           0)));
    legendscreen = NULL;
    /* make it immovable */
    olegend = (Object)DXNewScreen(outlegend, SCREEN_STATIONARY, 0);
    if (!olegend)
        goto error;
    outlegend = NULL;


    if (givencorners) {
        newcorners = (Object)DXNewArray(TYPE_FLOAT,CATEGORY_REAL,1,2);
        if (!newcorners)
            goto error;
        if (!DXAddArrayData((Array)newcorners,0,2,(Pointer)cp))
            goto error;
    }

    actualcorners = (float *)DXAllocateLocal(4*sizeof(float));
    if (!actualcorners)
        goto error;



    if ((frame==2)&&(secondplot))
        frame = 3;

    axeshandle = _dxfNew2DAxesObject();
    DXSetFloatAttribute((Object)ino, "fuzz", 8);

    if (!axeshandle)
        goto error;
    _dxfSet2DAxesCharacteristic(axeshandle, "OBJECT", (Pointer)&ino);
    _dxfSet2DAxesCharacteristic(axeshandle, "XLABEL", (Pointer)labelx);
    _dxfSet2DAxesCharacteristic(axeshandle, "YLABEL", (Pointer)labely);
    _dxfSet2DAxesCharacteristic(axeshandle, "LABELSCALE", (Pointer)&labelscale);
    _dxfSet2DAxesCharacteristic(axeshandle, "FONT", (Pointer)fontname);
    _dxfSet2DAxesCharacteristic(axeshandle,"TICKS", (Pointer)&ticks);
    _dxfSet2DAxesCharacteristic(axeshandle,"TICKSX", (Pointer)&tx);
    _dxfSet2DAxesCharacteristic(axeshandle,"TICKSY", (Pointer)&ty);
    if (newcorners)
        _dxfSet2DAxesCharacteristic(axeshandle, "CORNERS", (Pointer)&newcorners);
    _dxfSet2DAxesCharacteristic(axeshandle,"FRAME", (Pointer)&frame);
    _dxfSet2DAxesCharacteristic(axeshandle,"ADJUST", (Pointer)&adjust);
    _dxfSet2DAxesCharacteristic(axeshandle,"GRID", (Pointer)&grid);
    _dxfSet2DAxesCharacteristic(axeshandle,"ISLOGX", (Pointer)&xlog);
    _dxfSet2DAxesCharacteristic(axeshandle,"ISLOGY", (Pointer)&ylog);
    _dxfSet2DAxesCharacteristic(axeshandle,"MINTICKSIZE", (Pointer)&floatzero);
    _dxfSet2DAxesCharacteristic(axeshandle,"AXESLINES",(Pointer)&intone);
    _dxfSet2DAxesCharacteristic(axeshandle,"JUSTRIGHT",(Pointer)&intzero);
    _dxfSet2DAxesCharacteristic(axeshandle,"RETURNCORNERS",
                                (Pointer)actualcorners);
    _dxfSet2DAxesCharacteristic(axeshandle,"LABELCOLOR", (Pointer)&labelcolor);
    _dxfSet2DAxesCharacteristic(axeshandle,"TICKCOLOR", (Pointer)&ticcolor);
    _dxfSet2DAxesCharacteristic(axeshandle,"AXESCOLOR", (Pointer)&axescolor);
    _dxfSet2DAxesCharacteristic(axeshandle,"GRIDCOLOR", (Pointer)&gridcolor);
    _dxfSet2DAxesCharacteristic(axeshandle,"BACKGROUNDCOLOR",
                                (Pointer)&backgroundcolor);
    _dxfSet2DAxesCharacteristic(axeshandle,"BACKGROUND", (Pointer)&background);
    _dxfSet2DAxesCharacteristic(axeshandle,"XLOCS", (Pointer)xticklocations);
    _dxfSet2DAxesCharacteristic(axeshandle,"YLOCS", (Pointer)yticklocations);
    _dxfSet2DAxesCharacteristic(axeshandle,"XLABELS", (Pointer)in[21]);
    _dxfSet2DAxesCharacteristic(axeshandle,"YLABELS", (Pointer)in[22]);
    _dxfSet2DAxesCharacteristic(axeshandle,"DOFIXEDFONTSIZE", (Pointer)&dofixedfontsize);
    _dxfSet2DAxesCharacteristic(axeshandle,"FIXEDFONTSIZE", (Pointer)&fixedfontsize);

    if (DXGetError() != ERROR_NONE)
        goto error;

    if (!(withaxes=(Object)_dxfAxes2D(axeshandle)))
        goto error;
    _dxfFreeAxesHandle((Pointer)axeshandle);
    axeshandle=NULL;
    ino = NULL;


    _dxfHowManyTics(xlog, ylog, xscaling, 1.0/yscaling, xwidth, ywidth, 0,
                    &tx, &ty);

    outgroup = (Object)DXNewGroup();
    DXSetMember((Group)outgroup,NULL,withaxes);
    withaxes = NULL;
    /* XXX this gets set to null if no legend requested ? */
    DXSetMember((Group)outgroup,NULL,olegend);
    olegend = NULL;

    /* reset minplotpos and maxplotpos based on what axes did */
    /* XXX check sign? */
    minplotpos = actualcorners[0];
    maxplotpos = actualcorners[2];
    minplotdata = actualcorners[1];
    maxplotdata = actualcorners[3];
    xwidth = maxplotpos-minplotpos;

    /* Now handle the second plot */
    if (secondplot) {
        needtoclip=0;
        /* get the range of data for the second plot */
        if (!(DXStatistics(in[13],"data",
                           &minplotdata2, &maxplotdata2, NULL, NULL)))
            goto error;

        /* check to see if clipping will be necessary */
        if (givencorners2) {
            needtoclip = 1;
            /* x's are the same as the first plot */
            if (!ylog2)
                ywidth2 = cp2[1]-cp2[0];
            else
                ywidth2 = FCEIL(log10(cp2[1]))-FFLOOR(log10(cp2[0]));
            clippoint1 = DXPt(minplotpos,cp2[0],-1.0);
            clippoint2 = DXPt(maxplotpos,cp2[1], 1.0);
        } else {
            if (!GetPlotHeight(in[0], ylog2, &ywidth2, &maxplotdata2, &minplotdata2))
                goto error;
            clippoint1 = DXPt(minplotpos,minplotdata2,-1.0);
            clippoint2 = DXPt(maxplotpos,maxplotdata2, 1.0);
        }
        /* need to get information about the transformation, if there is one */
        if (autoadjust) {
            if (DXGetObjectClass(in[13]) == CLASS_XFORM) {
                if (!DXGetXformInfo((Xform)in[13], &subin, &matrix)) {
                    goto error;
                }
                /* we're about to give this guy a new top level xform */
                /* first call invalidiate connections */
                if (!DXInvalidateConnections(subin))
                    goto error;
                tmpino2 = DXCopy(subin,COPY_STRUCTURE);
            } else {
                if (!DXInvalidateConnections(in[13]))
                    goto error;
                tmpino2 = DXCopy(in[13],COPY_STRUCTURE);
            }
            scalingx = 1.0;
            scalingy = 1.0;

            if (!tmpino2)
                goto error;
            /* this is the xform which will give us the right aspect ratio */
            /* it depends on whether the axes are log or lin */
            yscaling2 = aspect * xwidth *scalingx/ (ywidth2 * scalingy);
            xscaling = 1.0;
            matrix = DXScale(xscaling, yscaling2, 1.0);
            if (!(ino2 = (Object)DXNewXform(tmpino2,matrix)))
                goto error;
            tmpino2 = NULL;
        }
        /* else we are not autoadjusting */
        else {
            if (DXGetObjectClass(in[13]) == CLASS_XFORM) {
                if (!DXGetXformInfo((Xform)in[13], NULL, &matrix)) {
                    goto error;
                }
                xscaling = matrix.A[0][0];
                yscaling = matrix.A[1][1];
            } else {
                xscaling = 1.0;
                yscaling = 1.0;
            }
            if (!DXInvalidateConnections(ino2))
                goto error;
            ino2 = DXCopy(in[13],COPY_STRUCTURE);
            if (!ino2)
                goto error;
        }

        /* once again, need to do diff things for logs */
        if (xlog)
            xwidth = xwidth*xscaling;
        if (ylog2)
            ywidth = ywidth*yscaling2;

        epsilon = xwidth/EPS_SCALE;
        epsilonx = epsilon/xscaling;
        epsilony = epsilon/yscaling2;

        if (epsilon <=0.0) {
            DXSetError(ERROR_DATA_INVALID,"#11835");
            goto error;
        }

        if (!(DXCreateTaskGroup())) {
            goto error;
        }
        newcorners2 = (Object)DXNewArray(TYPE_FLOAT,CATEGORY_REAL,1,2);
        if (!newcorners2)
            goto error;
        if (!DXAddArrayData((Array)newcorners2,0,2,NULL))
            goto error;
        ptr = (float *)DXGetArrayData((Array)newcorners2);
        ptr[0] = cp[0];
        if (givencorners2) {
            ptr[1] = cp2[0];
            ptr[3] = cp2[1];
        } else {
            ptr[1] = minplotdata2;
            ptr[3] = maxplotdata2;
        }
        ptr[2] = cp[2];

        /* DoPlot() to do the recursive traversal. The AddTasks will
           be added here */
        if (!(DoPlot(ino2,needtoclip,clippoint1,clippoint2, epsilonx, epsilony,
                     xlog, ylog2, &plotarg)))
            goto error;
        if (!(DXExecuteTaskGroup())) {
            goto error;
        }
        ty = ticks2;

        axeshandle = _dxfNew2DAxesObject();
        DXSetFloatAttribute((Object)ino2, "fuzz", 8);

        if (!axeshandle)
            goto error;
        _dxfSet2DAxesCharacteristic(axeshandle, "OBJECT", (Pointer)&ino2);
        _dxfSet2DAxesCharacteristic(axeshandle, "XLABEL", (Pointer)labelx);
        _dxfSet2DAxesCharacteristic(axeshandle, "YLABEL", (Pointer)labely2);
        _dxfSet2DAxesCharacteristic(axeshandle, "LABELSCALE", (Pointer)&labelscale);
        _dxfSet2DAxesCharacteristic(axeshandle, "FONT", (Pointer)fontname);
        _dxfSet2DAxesCharacteristic(axeshandle,"TICKS", (Pointer)&intzero);
        _dxfSet2DAxesCharacteristic(axeshandle,"TICKSX", (Pointer)&tx);
        _dxfSet2DAxesCharacteristic(axeshandle,"TICKSY", (Pointer)&ty);
        if (newcorners2)
            _dxfSet2DAxesCharacteristic(axeshandle, "CORNERS", (Pointer)&newcorners2);
        _dxfSet2DAxesCharacteristic(axeshandle,"FRAME", (Pointer)&intzero);
        _dxfSet2DAxesCharacteristic(axeshandle,"ADJUST", (Pointer)&adjust);
        _dxfSet2DAxesCharacteristic(axeshandle,"GRID", (Pointer)&intzero);
        _dxfSet2DAxesCharacteristic(axeshandle,"ISLOGX", (Pointer)&xlog);
        _dxfSet2DAxesCharacteristic(axeshandle,"ISLOGY", (Pointer)&ylog2);
        _dxfSet2DAxesCharacteristic(axeshandle,"MINTICKSIZE", (Pointer)&floatzero);
        _dxfSet2DAxesCharacteristic(axeshandle,"AXESLINES",(Pointer)&intone);
        _dxfSet2DAxesCharacteristic(axeshandle,"JUSTRIGHT",(Pointer)&intone);
        _dxfSet2DAxesCharacteristic(axeshandle,"RETURNCORNERS",
                                    (Pointer)actualcorners);
        _dxfSet2DAxesCharacteristic(axeshandle,"LABELCOLOR", (Pointer)&labelcolor);
        _dxfSet2DAxesCharacteristic(axeshandle,"TICKCOLOR", (Pointer)&ticcolor);
        _dxfSet2DAxesCharacteristic(axeshandle,"AXESCOLOR", (Pointer)&axescolor);
        _dxfSet2DAxesCharacteristic(axeshandle,"GRIDCOLOR", (Pointer)&gridcolor);
        /* set background to 0 for the second plot */
        background = 0;
        _dxfSet2DAxesCharacteristic(axeshandle,"BACKGROUND", (Pointer)&background);
        _dxfSet2DAxesCharacteristic(axeshandle,"XLOCS", (Pointer)xticklocations);
        _dxfSet2DAxesCharacteristic(axeshandle,"YLOCS", (Pointer)y1ticklocations);
        _dxfSet2DAxesCharacteristic(axeshandle,"XLABELS", (Pointer)in[21]);
        _dxfSet2DAxesCharacteristic(axeshandle,"YLABELS", (Pointer)in[23]);
        _dxfSet2DAxesCharacteristic(axeshandle,"DOFIXEDFONTSIZE", (Pointer)&dofixedfontsize);
        _dxfSet2DAxesCharacteristic(axeshandle,"FIXEDFONTSIZE", (Pointer)&fixedfontsize);
        if (DXGetError() != ERROR_NONE)
            goto error;

        if (!(withaxes2=(Object)_dxfAxes2D(axeshandle)))
            goto error;
        _dxfFreeAxesHandle((Pointer)axeshandle);
        axeshandle=NULL;


        /* now scale and translate to get the thing to line up with the first
         * plot DONT FORGET .85*/

        /* first translate to origin of y */
        translation1 = DXTranslate(DXPt(0.0,-actualcorners[1],0.0));
        /* now scale to make it the same height */
        scaling = DXScale(1.0,
                          (maxplotdata-minplotdata)/(actualcorners[3]-actualcorners[1]),
                          1.0);
        /* now translate to y level of first plot */
        translation2 = DXTranslate(DXPt(0.0, minplotdata, 0.0));
        xform = DXConcatenate(translation1, DXConcatenate(scaling, translation2));
        scaled = (Object)DXNewXform(withaxes2,xform);
        withaxes2 = NULL;
        DXSetMember((Group)outgroup,NULL,scaled);
        scaled = NULL;
    }

    /* successful return */
    /* scale the whole thing */
    out[0]=outgroup;
    _dxfFreeAxesHandle((Pointer)axeshandle);
    DXDelete((Object)legend);
    DXDelete((Object)withaxes);
    DXDelete((Object)withaxes2);
    DXDelete((Object)legendscreen);
    DXDelete((Object)outlegend);
    DXDelete((Object)legendo);
    DXDelete((Object)ino);
    DXDelete((Object)emptyfield);
    DXFree((Pointer)actualcorners);
    DXDelete((Object)newcorners);
    DXDelete((Object)newcorners2);
    DXDelete((Object)scaled);
    DXDelete((Object)font);
    DXDelete((Object)group);
    DXDelete((Object)tmpino);
    if (!in[18])
        DXDelete((Object)xticklocations);
    if (!in[19])
        DXDelete((Object)yticklocations);
    if (!in[20])
        DXDelete((Object)y1ticklocations);
    /* XXX delete tmpino? */
    return OK;



error:
    if (axeshandle)
        _dxfFreeAxesHandle((Pointer)axeshandle);
    DXDelete((Object)withaxes);
    DXDelete((Object)withaxes2);
    DXDelete((Object)legendscreen);
    DXDelete((Object)outlegend);
    DXDelete((Object)legend);
    DXDelete((Object)legendo);
    DXDelete((Object)tmpino);
    DXFree((Pointer)actualcorners);
    DXDelete((Object)tmpino2);
    DXDelete((Object)group);
    DXDelete((Object)newcorners2);
    DXDelete((Object)newcorners);
    DXDelete((Object)scaled);
    /* DXDelete((Object)ino); */
    DXDelete((Object)ino2);
    DXDelete((Object)emptyfield);
    /*
    DXDelete((Object)outgroup); */
    DXDelete((Object)font);
    if (!in[18])
        DXDelete((Object)xticklocations);
    if (!in[19])
        DXDelete((Object)yticklocations);
    if (!in[20])
        DXDelete((Object)y1ticklocations);
    return ERROR;
}



static
Error
DoPlot(Object ino, int needtoclip, Point clippoint1, Point clippoint2,
       float epsilonx, float epsilony, int xlog, int ylog,
       struct plotargstruct *plotarg) {
    int i;
    int markevery, scatterflag=0, columnflag=0;
    float markscale;
    char *marker;
    Object subin, markobject, scatterobject, columnobject;
    Matrix matrix;
    struct plotargstruct localplotarg;

    if (!(memcpy(&localplotarg, plotarg, sizeof(localplotarg)))) {
        return ERROR;
    }


    /* determine the class of the object */
    switch (DXGetObjectClass(ino)) {
        case CLASS_FIELD:
        if (!(DoPlotField(ino, needtoclip, clippoint1, clippoint2, epsilonx,
                          epsilony, xlog, ylog, plotarg)))
            return ERROR;
        break;

        case CLASS_GROUP:
        scatterobject = DXGetAttribute((Object)ino,"scatter");
        if (scatterobject) {
            if (!DXExtractInteger(scatterobject, &scatterflag)) {
                DXSetError(ERROR_DATA_INVALID,
                           "scatter attribute must be 0 or 1");
                goto error;
            }
            if ((scatterflag!=0)&&(scatterflag!=1)) {
                DXSetError(ERROR_DATA_INVALID,
                           "scatter attribute must be 0 or 1");
                goto error;
            }
            localplotarg.scatter = scatterflag;
        }
        columnobject = DXGetAttribute((Object)ino,"column");
        if (columnobject) {
            if (!DXExtractInteger(columnobject, &columnflag)) {
                DXSetError(ERROR_DATA_INVALID,
                           "column attribute must be 0 or 1");
                goto error;
            }
            if ((columnflag!=0)&&(columnflag!=1)) {
                DXSetError(ERROR_DATA_INVALID,
                           "column attribute must be 0 or 1");
                goto error;
            }
            localplotarg.column = columnflag;
        }
        markobject = DXGetAttribute((Object)ino, "mark");
        if (markobject) {
            if (!DXExtractNthString(markobject, 0, &marker)) {
                DXSetError(ERROR_DATA_INVALID,"#10200", "mark");
                goto error;
            }
            localplotarg.ismark = 1;
            localplotarg.mark = marker;
        }

        /* if scatter is set, but no marker was given, make it a "x" */
        if ((scatterflag==1) && (!markobject)) {
            marker = "x";
            localplotarg.ismark = 1;
            localplotarg.mark = marker;
        }


        if (DXGetAttribute((Object)ino,"mark every")) {
            if (!DXExtractInteger(DXGetAttribute((Object)ino, "mark every"),
                                  &markevery)) {
                DXSetError(ERROR_DATA_INVALID,"#10020", "mark every attribute");
                goto error;
            }
            if (markevery < 0) {
                DXSetError(ERROR_DATA_INVALID,
                           "mark every attribute must non-negative");
                goto error;
            }
            localplotarg.markevery = markevery;
        }

        if (DXGetAttribute((Object)ino,"mark scale")) {
            if (!DXExtractFloat(DXGetAttribute((Object)ino, "mark scale"),
                                &markscale)) {
                DXSetError(ERROR_DATA_INVALID,"#10090", "mark scale attribute");
                goto error;
            }
            if (markscale <= 0) {
                DXSetError(ERROR_DATA_INVALID,"#10090", "mark scale attribute");
                goto error;
            }
            localplotarg.markscale = markscale;
        }

        switch (DXGetGroupClass((Group)ino)) {
            case CLASS_COMPOSITEFIELD:
            if (!(DoPlotField(ino, needtoclip, clippoint1, clippoint2, epsilonx,
                              epsilony, xlog, ylog, &localplotarg)))
                return ERROR;
            break;
            default:
            /* recursively traverse groups */
            DXGetMemberCount((Group)ino, &localplotarg.subfields);
            for (i=0; (subin=DXGetEnumeratedMember((Group)ino, i, NULL)); i++) {
            	localplotarg.fieldNumber = i;
                if (!(DoPlot((Object)subin, needtoclip, clippoint1, clippoint2,
                             epsilonx, epsilony, xlog, ylog, &localplotarg)))
                    return ERROR;
            }
            break;
        }
        break;
        case CLASS_XFORM:

        markobject = DXGetAttribute((Object)ino, "mark");
        if (markobject) {
            if (!DXExtractNthString(markobject, 0, &marker)) {
                DXSetError(ERROR_DATA_INVALID,"#10200", "mark");
                goto error;
            }
            localplotarg.ismark = 1;
            localplotarg.mark = marker;
        }

        scatterobject = DXGetAttribute((Object)ino,"scatter");
        if (scatterobject) {
            if (!DXExtractInteger(scatterobject, &scatterflag)) {
                DXSetError(ERROR_DATA_INVALID,
                           "scatter attribute must be 0 or 1");
                goto error;
            }
            if ((scatterflag!=0)&&(scatterflag!=1)) {
                DXSetError(ERROR_DATA_INVALID,
                           "scatter attribute must be 0 or 1");
                goto error;
            }
            localplotarg.scatter = scatterflag;
        }

        if ((scatterflag == 1)&&(!markobject)) {
            marker = "x";
            localplotarg.ismark = 1;
            localplotarg.mark = marker;
        }
        columnobject = DXGetAttribute((Object)ino,"column");
        if (columnobject) {
            if (!DXExtractInteger(columnobject, &columnflag)) {
                DXSetError(ERROR_DATA_INVALID,
                           "column attribute must be 0 or 1");
                goto error;
            }
            if ((columnflag!=0)&&(columnflag!=1)) {
                DXSetError(ERROR_DATA_INVALID,
                           "column attribute must be 0 or 1");
                goto error;
            }
            localplotarg.column = columnflag;
        }


        if (DXGetAttribute((Object)ino,"mark every")) {
            if (!DXExtractInteger(DXGetAttribute((Object)ino, "mark every"),
                                  &markevery)) {
                DXSetError(ERROR_DATA_INVALID,"#10020", "mark every attribute");
                goto error;
            }
            if (markevery < 0) {
                DXSetError(ERROR_DATA_INVALID,
                           "mark every attribute must be non-negative");
                goto error;
            }
            localplotarg.markevery = markevery;
        }
        if (DXGetAttribute((Object)ino,"mark scale")) {
            if (!DXExtractFloat(DXGetAttribute((Object)ino, "mark scale"),
                                &markscale)) {
                DXSetError(ERROR_DATA_INVALID, "#10090", "mark scale attribute");
                goto error;
            }
            if (markscale <= 0) {
                DXSetError(ERROR_DATA_INVALID,"#10090", "mark scale attribute");
                goto error;
            }
            localplotarg.markscale = markscale;
        }
        if (!(DXGetXformInfo((Xform)ino, &subin, &matrix)))
            return ERROR;
        if (!(DoPlot(subin, needtoclip, clippoint1, clippoint2, epsilonx,
                     epsilony, xlog, ylog, &localplotarg)))
            return ERROR;
        break;
        default:
        DXSetError(ERROR_BAD_PARAMETER,"#10190", "lines to be plotted");
        goto error;
    }
    return OK;
error:
    return ERROR;
}


static
Error
MakeLegend(Object ino, Object legend, int *num, float *maxstring,
           float linelength, Object font, struct legendargstruct *legendarg,
           RGBColor labelcolor) {
    int i, scatterflag=0;
    Object subin, plotlabel, labelfield, markobject, scatterobject;
    Matrix matrix;
    RGBColor color;
    struct legendargstruct locallegendarg;
    char *labelstring;
    char *marker;

    if (!(memcpy(&locallegendarg, legendarg, sizeof(locallegendarg)))) {
        return ERROR;
    }



    /* determine the class of the object */
    switch (DXGetObjectClass(ino)) {

        case CLASS_FIELD:
        /* now collect any options which may be set */
        plotlabel = (DXGetAttribute((Object)ino,"label"));
        if (plotlabel)  {
            if (!(DXExtractNthString(plotlabel, 0, &labelstring))) {
                DXSetError(ERROR_BAD_PARAMETER,"#10200", "label attribute");
                return ERROR;
            }
            if (!strcmp(labelstring,nullstring)) {
                *num = *num+1;
                return OK;
            }
            locallegendarg.islabel = 1;
            locallegendarg.label = labelstring;
        }

        if (DXGetAttribute((Object)ino,"mark")) {
            locallegendarg.ismark = 1;
            if (!DXExtractNthString(DXGetAttribute(ino,"mark"), 0, &marker)) {
                DXSetError(ERROR_BAD_PARAMETER,"#10200", "label attribute");
                return ERROR;
            }
            locallegendarg.mark = marker;
        }

        scatterobject = DXGetAttribute((Object)ino,"scatter");
        if (scatterobject) {
            if (!DXExtractInteger(scatterobject, &scatterflag)) {
                DXSetError(ERROR_DATA_INVALID,
                           "scatter attribute must be 0 or 1");
                return ERROR;
            }
            if ((scatterflag!=0)&&(scatterflag!=1)) {
                DXSetError(ERROR_DATA_INVALID,
                           "scatter attribute must be 0 or 1");
                return ERROR;
            }
            locallegendarg.scatter = scatterflag;
            if ((scatterflag==1)&&(!DXGetAttribute((Object)ino,"mark"))) {
                locallegendarg.ismark = 1;
                locallegendarg.mark = "x";
            }
        }



        if (locallegendarg.islabel) {
            *num = *num+1;
            if (!(GetLineColor(ino,&color)))
                return ERROR;
            if (!(labelfield=MakeLabel(locallegendarg.label, color, *num, maxstring,
                                       linelength, font, locallegendarg.ismark,
                                       locallegendarg.scatter,
                                       locallegendarg.mark,
                                       labelcolor)))
                return ERROR;
            DXSetMember((Group)legend, NULL,labelfield);
            labelfield = NULL;
        }
        break;

        case CLASS_GROUP:
        switch (DXGetGroupClass((Group)ino)) {
            case CLASS_COMPOSITEFIELD:
            /* now collect any options which may be set */
            plotlabel = (DXGetAttribute((Object)ino,"label"));
            if (plotlabel)  {
                if (!(DXExtractNthString(plotlabel, 0, &labelstring))) {
                    DXSetError(ERROR_BAD_PARAMETER,"#10200", "label attribute");
                    return ERROR;
                }
                locallegendarg.islabel = 1;
                locallegendarg.label = labelstring;
            }

            if (DXGetAttribute((Object)ino,"mark")) {
                if (!DXExtractNthString(DXGetAttribute((Object)ino,"mark"),
                                        0, &marker)) {
                    DXSetError(ERROR_BAD_PARAMETER,"#10200", "mark attribute");
                    return ERROR;
                }
                locallegendarg.ismark = 1;
                locallegendarg.mark = marker;
            }
            if (locallegendarg.islabel) {
                /* just make one label for a composite field */
                *num = *num+1;
                if (!(GetLineColor(ino,&color)))
                    return ERROR;
                if (!(labelfield=MakeLabel(locallegendarg.label, color, *num,
                                           maxstring, linelength, font,
                                           locallegendarg.ismark,
                                           locallegendarg.scatter,
                                           locallegendarg.mark, labelcolor)))
                    return ERROR;
                DXSetMember((Group)legend,NULL,labelfield);
                labelfield = NULL;
            }
            break;

            /* don't know about this yet
            case CLASS_SERIES:
            */

            /* generic group */
            default:
            /* recursively traverse groups */
            /* now collect any options which may be set */
            plotlabel = (DXGetAttribute((Object)ino,"label"));
            if (plotlabel)  {
                if (!(DXExtractNthString(plotlabel, 0, &labelstring))) {
                    DXSetError(ERROR_BAD_PARAMETER,"#10200", "label attribute");
                    return ERROR;
                }
                locallegendarg.islabel = 1;
                locallegendarg.label = labelstring;
            }
            if (!(GetLineColor(ino,&color)))
                return ERROR;
            markobject = DXGetAttribute((Object)ino,"mark");
            if (markobject) {
                if (!DXExtractNthString(markobject, 0, &marker)) {
                    DXSetError(ERROR_DATA_INVALID,"#10200", "mark attribute");
                    return ERROR;
                }
                locallegendarg.ismark=1;
                locallegendarg.mark=marker;
            }
            scatterobject = DXGetAttribute((Object)ino,"scatter");
            if (scatterobject) {
                if (!DXExtractInteger(scatterobject, &scatterflag)) {
                    DXSetError(ERROR_DATA_INVALID,
                               "scatter attribute must be 0 or 1");
                    return ERROR;
                }
                if ((scatterflag!=0)&&(scatterflag!=1)) {
                    DXSetError(ERROR_DATA_INVALID,
                               "scatter attribute must be 0 or 1");
                    return ERROR;
                }
                locallegendarg.scatter = scatterflag;
                if ((scatterflag==1)&&(!markobject)) {
                    locallegendarg.ismark = 1;
                    locallegendarg.mark = "x";
                }
            }

            for (i=0; (subin=DXGetEnumeratedMember((Group)ino, i, NULL)); i++) {
                if (!MakeLegend(subin, legend, num, maxstring, linelength, font,
                                &locallegendarg, labelcolor))
                    return ERROR;
            }
            break;
        }
        break;
        case CLASS_XFORM:
        /* now collect any options which may be set */
        plotlabel = (DXGetAttribute((Object)ino,"label"));
        if (!(DXGetXformInfo((Xform)ino, &subin, &matrix)))
            return ERROR;
        if (plotlabel)  {
            if (!(DXExtractNthString(plotlabel, 0, &labelstring))) {
                DXSetError(ERROR_BAD_PARAMETER,"#10200", "label attribute");
                return ERROR;
            }
            locallegendarg.islabel = 1;
            locallegendarg.label = labelstring;
        }
        if (!(GetLineColor(ino,&color)))
            return ERROR;
        markobject = DXGetAttribute((Object)ino, "mark");

        if (markobject) {
            if (!DXExtractNthString(markobject, 0, &marker)) {
                DXSetError(ERROR_DATA_INVALID,"#10200","mark attribute");
                return ERROR;
            }
            locallegendarg.ismark = 1;
            locallegendarg.mark = marker;
        }

        scatterobject = DXGetAttribute((Object)ino,"scatter");
        if (scatterobject) {
            if (!DXExtractInteger(scatterobject, &scatterflag)) {
                DXSetError(ERROR_DATA_INVALID,
                           "scatter attribute must be 0 or 1");
                return ERROR;
            }
            if ((scatterflag!=0)&&(scatterflag!=1)) {
                DXSetError(ERROR_DATA_INVALID,
                           "scatter attribute must be 0 or 1");
                return ERROR;
            }
            locallegendarg.scatter = scatterflag;
            if ((scatterflag==1)&&(!markobject)) {
                locallegendarg.ismark=1;
                locallegendarg.mark = "x";
            }
        }



        if (!(MakeLegend(subin, legend, num, maxstring, linelength, font,
                         &locallegendarg,labelcolor)))
            return ERROR;
        break;
        default:
        break;
    }
    return OK;
}



static Error plottask(Pointer p) {
    struct arg *arg = (struct arg *)p;
    char depatt[30];
    Object ino;
    Array adata, apos, adatanew=NULL, aposnew=NULL;
    int xlog, ylog, rank, shape[30], numitems, i;
    Type type;
    InvalidComponentHandle handle = NULL;
    Category category;
    Point clippoint1, clippoint2;
    float *new_ptr, *old_ptr=NULL;
    ubyte *old_ubyte_ptr=NULL;
    short *old_short_ptr=NULL;
    int *old_int_ptr=NULL;
    uint *old_uint_ptr=NULL;
    ushort *old_ushort_ptr=NULL;
    byte *old_byte_ptr=NULL;
    double *old_double_ptr=NULL;

    ino = arg->ino;
    xlog = arg->xlog;
    ylog = arg->ylog;
    clippoint1 = arg->clippoint1;
    clippoint2 = arg->clippoint2;



    if (!(adata = (Array)DXGetComponentValue((Field)ino, "data"))) {
        DXSetError(ERROR_BAD_PARAMETER,"#10240", "input","data");
        goto error;
    }

    if (DXGetComponentAttribute((Field)ino, "data", "dep"))
        strcpy(depatt,DXGetString((String)DXGetComponentAttribute((Field)ino,
                                  "data","dep")));
    else {
        DXSetError(ERROR_DATA_INVALID,"data component is missing dep attribute");
        goto error;
    }


    handle = DXCreateInvalidComponentHandle(ino,NULL,depatt);
    if (!handle)
        goto error;


    if (ylog) {
        /* change the clippoint */
        if ((clippoint1.y > 0.0)&&(clippoint2.y > 0.0)) {
            clippoint1.y = log10(clippoint1.y);
            clippoint2.y = log10(clippoint2.y);
        } else {
            DXSetError(ERROR_DATA_INVALID,"#10531", "for log plot, corners");
            goto error;
        }
        arg->clippoint1 = clippoint1;
        arg->clippoint2 = clippoint2;
        /* make a new data array */
        DXGetArrayInfo(adata,&numitems, &type, &category, &rank, shape);
        if (!((rank==0)||((rank==1)&&(shape[0]==1)))) {
            DXSetError(ERROR_DATA_INVALID,"#10081", "data component");
            goto error;
        }
        if (category != CATEGORY_REAL) {
            DXSetError(ERROR_DATA_INVALID,
                       "data component must be category real");
            goto error;
        }
        /* if it's log, the data component has got to end up as float */
        if (ylog)
            adatanew = DXNewArrayV(TYPE_FLOAT,category,rank,shape);
        else
            adatanew = DXNewArrayV(type,category,rank,shape);
        if (!DXAddArrayData(adatanew,0,numitems,NULL))
            goto error;
        new_ptr = (float *)DXGetArrayData(adatanew);
        switch (type) {
            case (TYPE_UBYTE):
                        old_ubyte_ptr = (unsigned char *)DXGetArrayData(adata);
            break;
            case (TYPE_SHORT):
                        old_short_ptr = (short *)DXGetArrayData(adata);
            break;
            case (TYPE_INT):
                        old_int_ptr = (int *)DXGetArrayData(adata);
            break;
            case (TYPE_FLOAT):
                        old_ptr = (float *)DXGetArrayData(adata);
            break;
            case (TYPE_DOUBLE):
                        old_double_ptr = (double *)DXGetArrayData(adata);
            break;
            case (TYPE_BYTE):
                        old_byte_ptr = (byte *)DXGetArrayData(adata);
            break;
            case (TYPE_USHORT):
                        old_ushort_ptr = (ushort *)DXGetArrayData(adata);
            break;
            case (TYPE_UINT):
                        old_uint_ptr = (uint *)DXGetArrayData(adata);
            break;
            default:
            DXSetError(ERROR_DATA_INVALID,"#11829", "data");
            goto error;
        }



for (i=0; i<numitems;i++) {
            /* only check if it's a valid point */
            if (DXIsElementValid(handle, i)) {
                switch (type) {
                    case (TYPE_UBYTE):
                                new_ptr[i] = log10((float)old_ubyte_ptr[i]);
                    break;
                    case (TYPE_SHORT):
                                if (old_short_ptr[i]>0.0)
                                    new_ptr[i] = log10((float)old_short_ptr[i]);
                else {
                            DXSetError(ERROR_DATA_INVALID,
                                       "#11860", "data");
                            goto error;
                        }
                    break;
                    case (TYPE_INT):
                                if (old_int_ptr[i]>0.0)
                                    new_ptr[i] = log10((float)old_int_ptr[i]);
                else {
                            DXSetError(ERROR_DATA_INVALID,
                                       "#11860", "data");
                            goto error;
                        }
                    break;
                    case (TYPE_FLOAT):
                                if (old_ptr[i] > 0.0)
                                    new_ptr[i] = log10(old_ptr[i]);
                else {
                            DXSetError(ERROR_DATA_INVALID,
                                       "#11860", "data");
                            goto error;
                        }
                    break;
                    case (TYPE_DOUBLE):
                                if (old_double_ptr[i]>0.0)
                                    new_ptr[i] = log10((float)old_double_ptr[i]);
                else {
                            DXSetError(ERROR_DATA_INVALID,
                                       "#11860", "data");
                            goto error;
                        }
                    break;
                    case (TYPE_BYTE):
                                if (old_byte_ptr[i]>0.0)
                                    new_ptr[i] = log10((float)old_byte_ptr[i]);
                else {
                            DXSetError(ERROR_DATA_INVALID,
                                       "#11860", "data");
                            goto error;
                        }
                    break;
                    case (TYPE_USHORT):
                                new_ptr[i] = log10((float)old_ushort_ptr[i]);
                    break;
                    case (TYPE_UINT):
                                new_ptr[i] = log10((float)old_uint_ptr[i]);
                    break;
                    default:
                    DXSetError(ERROR_DATA_INVALID,"#11829", "data");
                    goto error;
                }
    } else {
                new_ptr[i] = 0.0;
            }
        }
        DXSetComponentValue((Field)ino,"data",(Object)adatanew);
        arg->ino = ino;
        adatanew=NULL;
    }
    if (xlog) {
        /* change the clippoint */
        if ((clippoint1.x > 0.0)&&(clippoint2.x > 0.0)) {
            clippoint1.x = log10(clippoint1.x);
            clippoint2.x = log10(clippoint2.x);
        } else {
            DXSetError(ERROR_DATA_INVALID,"#10531", "for log plot, corners");
            goto error;
        }
        arg->clippoint1 = clippoint1;
        arg->clippoint2 = clippoint2;
        /* make a new positions array */
        apos = (Array)DXGetComponentValue((Field)ino,"positions");
        if (!apos) {
            DXSetError(ERROR_MISSING_DATA,"#10250", "input","positions");
            goto error;
        }
        DXGetArrayInfo(apos,&numitems, &type, &category, &rank, shape);
        if (!((rank==0)||((rank==1)&&(shape[0]==1)))) {
            DXSetError(ERROR_DATA_INVALID, "#11363", "positions component", 1);
            goto error;
        }
        if ((type != TYPE_FLOAT)||(category != CATEGORY_REAL)) {
            DXSetError(ERROR_DATA_INVALID,
                       "positions component must be type float category real");
            goto error;
        }
        aposnew = DXNewArrayV(type,category,rank,shape);
        if (!DXAddArrayData(aposnew,0,numitems,NULL))
            goto error;
        new_ptr = (float *)DXGetArrayData(aposnew);
        old_ptr = (float *)DXGetArrayData(apos);
        for (i=0; i<numitems;i++) {
            if (DXIsElementValid(handle, i)) {
                if (old_ptr[i]>0.0)
                    new_ptr[i] = log10(old_ptr[i]);
                else {
                    DXSetError(ERROR_DATA_INVALID, "#10531", "for log plot, positions");
                    goto error;
                }
            } else {
                new_ptr[i]=0.0;
            }
        }
        DXSetComponentValue((Field)ino,"positions",(Object)aposnew);
        arg->ino = ino;
        aposnew=NULL;
    }
    if (!(DXGetString((String)DXGetComponentAttribute((Field)ino,
                      "data","dep")))) {
        DXSetError (ERROR_DATA_INVALID, "#10241", "data");
        goto error;
    }
    strcpy(depatt,DXGetString((String)DXGetComponentAttribute((Field)ino,
                              "data","dep")));
    if (!strcmp(depatt,"connections")) {
        if (!PlotDepConnections(arg))
            goto error;
    } else if (!strcmp(depatt,"positions")) {
        if(arg->column == 1) {
            if (!PlotBarDepPositions(arg))
                goto error;
        } else {
            if (!PlotDepPositions(arg))
                goto error;
        }
    } else {
        DXSetError(ERROR_DATA_INVALID,"#11250", "data");
        goto error;
    }
    DXDelete((Object)adatanew);
    DXDelete((Object)aposnew);
    DXFreeInvalidComponentHandle(handle);
    return OK;
error:
    DXDelete((Object)adatanew);
    DXDelete((Object)aposnew);
    DXFreeInvalidComponentHandle(handle);
    return ERROR;
}

static Error PlotDepConnections(struct arg *arg) {
    Array apos, acolors, aposnew, aconnew, acolorsnew, adata;
    Array acon, colormap;
    Array invalid;
    InvalidComponentHandle invalidhandle=NULL;
    Type colorstype;
    Category category;
    int rank, shape[32];
    char coldepatt[30];
    float *p_pos, *p_posnew;
    float *p_data_f=NULL;
    int *p_data_i=NULL;
    uint *p_data_ui=NULL;
    short *p_data_s=NULL;
    ushort *p_data_us=NULL;
    ubyte *p_data_ub=NULL;
    byte *p_data_b=NULL;
    double *p_data_d=NULL;
    Type type;
    float dataval;
    int *dp_conn, numitems;
    int poscount, datacount, conncount, i, needtoclip, count_p, count_c;
    int clipregcolors;
    int nooutput=0;
    RGBColor *color_ptr=NULL, color= {0,0,0}, origin;
    Point clippoint1, clippoint2, newpoint;
    Quadrilateral newconquad;
    Object ino;
    char *color_ubyte_ptr=NULL;
    RGBColor defaultcolor = DEFAULTCOLOR;

    aposnew=NULL;
    aconnew=NULL;
    acolorsnew=NULL;
    clipregcolors=0;

    ino = arg->ino;
    needtoclip = arg->needtoclip;
    clippoint1 = arg->clippoint1;
    clippoint2 = arg->clippoint2;
    /*epsilonx = arg->epsilonx;*/
    /*epsilony = arg->epsilony;*/

    /* check for invalid component */
    invalid = (Array)DXGetComponentValue((Field)ino,"invalid connections");
    if (invalid) {
        invalidhandle = DXCreateInvalidComponentHandle((Object)ino,NULL,
                        "connections");
        if (!invalidhandle)
            goto error;
    }

    /* extract, typecheck, and get the data from the ``positions''
       component */
    apos = (Array) DXGetComponentValue((Field)ino, "positions");
    if (!apos) {
        DXSetError(ERROR_MISSING_DATA, "#10250", "input" ,"positions");
        return ERROR;
    }
    if ((!DXTypeCheck(apos, TYPE_FLOAT, CATEGORY_REAL, 1, 1))&&
            (!DXTypeCheck(apos, TYPE_FLOAT, CATEGORY_REAL, 0, NULL))) {
        DXSetError(ERROR_BAD_TYPE, "#11364", "positions", 1);
        return ERROR;
    }
    p_pos = (float *) DXGetArrayData(apos);
    /* find out how many positions there are */
    if (!(DXGetArrayInfo(apos, &poscount, NULL, NULL, NULL, NULL))) {
        return ERROR;
    }

    /* check the colors component, if present */
    if (DXGetComponentValue((Field)ino,"colors")) {
        if (!(DXGetString((String)DXGetComponentAttribute((Field)ino,
                          "colors","dep")))) {
            DXSetError (ERROR_DATA_INVALID, "#10241", "colors");
            goto error;
        }
        strcpy(coldepatt,DXGetString((String)DXGetComponentAttribute((Field)ino,
                                     "colors","dep")));
        if (strcmp(coldepatt,"connections")) {
            DXSetError(ERROR_DATA_INVALID, "#10360", "data", "colors");
            goto error;
        }
    }

    /* Now make a brand-new positions array. It will be two dimensional */
    if (!(aposnew = DXNewArray(TYPE_FLOAT, CATEGORY_REAL, 1, 2))) {
        return ERROR;
    }
    /* Now get a pointer to the data component of the input object */
    if (!(adata = (Array)DXGetComponentValue((Field)ino, "data"))) {
        DXSetError(ERROR_BAD_PARAMETER,"#10250", "input", "data");
        goto error;
    }
    if (!DXGetArrayInfo(adata,NULL, &type,&category,&rank,shape))
        goto error;
    if (!((rank==0) ||((rank==1)&&(shape[0]==1)))) {
        DXSetError(ERROR_DATA_INVALID,"#11363", "data", 1);
        goto error;
    }

    switch (type) {
        case (TYPE_UBYTE):
                    if (!(p_data_ub = (ubyte *)DXGetArrayData(adata)))
                        goto error;
        break;
        case (TYPE_SHORT):
                    if (!(p_data_s = (short *)DXGetArrayData(adata)))
                        goto error;
        break;
        case (TYPE_INT):
                    if (!(p_data_i = (signed int *)DXGetArrayData(adata)))
                        goto error;
        break;
        case (TYPE_FLOAT):
                    if (!(p_data_f = (float *)DXGetArrayData(adata)))
                        goto error;
        break;
        case (TYPE_DOUBLE):
                    if (!(p_data_d = (double *)DXGetArrayData(adata)))
                        goto error;
        break;
        case (TYPE_BYTE):
                    if (!(p_data_b = (byte *)DXGetArrayData(adata)))
                        goto error;
        break;
        case (TYPE_USHORT):
                    if (!(p_data_us = (ushort *)DXGetArrayData(adata)))
                        goto error;
        break;
        case (TYPE_UINT):
                    if (!(p_data_ui = (uint *)DXGetArrayData(adata)))
                        goto error;
        break;
        default:
        DXSetError(ERROR_DATA_INVALID,"#11829", "data");
        goto error;
    }

    if (!DXGetArrayInfo(adata, &datacount, NULL, NULL, NULL, NULL))
        goto error;
if (datacount != (poscount-1)) {
        DXSetError(ERROR_BAD_PARAMETER, "#11161",
                   "connection-dependent data items", datacount,
                   "(#positions-1)", poscount-1);
        goto error;
    }

    /* if there's an invalid component, we need to check every point */
    if (invalid)
        needtoclip = 1;



    if (!needtoclip) {
        if (!(acon = (Array)DXGetComponentValue((Field)ino,"connections"))) {
            DXSetError(ERROR_DATA_INVALID, "#10251", "data", "connections");
            goto error;
        }
        if (!(DXGetArrayInfo(acon, &conncount, NULL, NULL, NULL, NULL))) {
            return ERROR;
        }
        /* make quads */
        aconnew = (Array)DXNewArray(TYPE_INT,CATEGORY_REAL,1,4);
        aconnew = DXAddArrayData(aconnew,0,conncount,NULL);
        if (!acon)
            return ERROR;
        dp_conn = (int *)DXGetArrayData(aconnew);
        for (i=0; i<conncount; i++) {
            dp_conn[4*i]   = 4*i;
            dp_conn[4*i+1] = 4*i+1;
            dp_conn[4*i+2] = 4*i+2;
            dp_conn[4*i+3] = 4*i+3;
        }

        if (!DXSetComponentValue((Field)ino, "connections", (Object)aconnew)) {
            goto error;
        }
        aconnew = NULL;
        if (!DXSetComponentAttribute((Field)ino, "connections",
                                     "ref", (Object)DXNewString("positions"))) {
            goto error;
        }
        if (!DXSetComponentAttribute((Field)ino, "connections",
                                     "element type", (Object)DXNewString("quads"))) {
            goto error;
        }
        if (!DXAddArrayData(aposnew,0,(4*conncount),NULL))
            goto error;

        /* get a pointer to the new positions component */
        if (!(p_posnew = (float *)DXGetArrayData(aposnew))) {
            goto error;
        }

        /* create the output positions where x is the original position, and
           y is the data value (essentially doing a rubbersheet)  */
        for (i=0; i<conncount; i++)  {
            switch (type) {
                case (TYPE_UBYTE):
                            dataval = (float)p_data_ub[i];
                break;
                case (TYPE_SHORT):
                            dataval = (float)p_data_s[i];
                break;
                case (TYPE_INT):
                            dataval = (float)p_data_i[i];
                break;
                case (TYPE_FLOAT):
                            dataval = (float)p_data_f[i];
                break;
                case (TYPE_DOUBLE):
                            dataval = (float)p_data_d[i];
                break;
                case (TYPE_BYTE):
                            dataval = (float)p_data_b[i];
                break;
                case (TYPE_USHORT):
                            dataval = (float)p_data_us[i];
                break;
                case (TYPE_UINT):
                            dataval = (float)p_data_ui[i];
                break;
                default:
                DXSetError(ERROR_DATA_INVALID,"#11829", "data");
                goto error;
            }

            p_posnew[8*i] = p_pos[i];
            p_posnew[8*i+1] = 0.0;
            p_posnew[8*i+2] = p_pos[i];
            p_posnew[8*i+3] = dataval;
            p_posnew[8*i+4] = p_pos[i+1];
            p_posnew[8*i+5] = 0.0;
            p_posnew[8*i+6] = p_pos[i+1];
            p_posnew[8*i+7] = dataval;
        }
if (!(DXSetComponentValue((Field)ino, "positions", (Object)aposnew))) {
            goto error;
        }
        aposnew = NULL;
    }
    /* else there's clipping to be done */
    else {
        if ((acolors = (Array)DXGetComponentValue((Field)ino, "colors"))) {
            DXGetArrayInfo(acolors, &numitems, &colorstype, &category, &rank, shape);
            if (!DXQueryConstantArray(acolors, NULL, NULL)) {
                clipregcolors = 0;
                if (colorstype == TYPE_FLOAT) {
                    if ((rank != 1)||(shape[0] != 3)) {
                        DXSetError(ERROR_DATA_INVALID,"#11363", "colors", 3);
                        goto error;
                    }
                    color_ptr = (RGBColor *)DXGetArrayData(acolors);
                } else if (colorstype == TYPE_UBYTE) {
                    if (!((rank==0)||((rank==1)&&(shape[0]==1)))) {
                        DXSetError(ERROR_DATA_INVALID,"#11363", "byte colors", 1);
                        goto error;
                    }
                    color_ubyte_ptr = (char *)DXGetArrayData(acolors);
                    colormap = (Array)DXGetComponentValue((Field)ino,"color map");
                    if (!colormap) {
                        DXSetError(ERROR_MISSING_DATA, "#13050");
                        goto error;
                    }
                    color_ptr = (RGBColor *)DXGetArrayData(colormap);
                } else {
                    DXSetError(ERROR_DATA_INVALID,"#11838", "colors component");
                    goto error;
                }
            } else {
                clipregcolors = 1;
                DXQueryConstantArray((Array)acolors, NULL, (Pointer)&origin);
            }
            /* remove the existing colors component */
            if (!DXRemove((Object)ino,"colors"))
                goto error;
        }

        if (!(acon = (Array)DXGetComponentValue((Field)ino,"connections"))) {
            DXSetError(ERROR_DATA_INVALID, "#10251", "data", "connections");
            goto error;
        }
        if (!(DXGetArrayInfo(acon, &conncount, NULL, NULL, NULL, NULL))) {
            return ERROR;
        }
        aconnew = DXNewArray(TYPE_INT,CATEGORY_REAL, 1, 4);
        count_c = 0;
        count_p = 0;
        for (i=0; i<conncount; i++) {
            if (( !(invalid && DXIsElementInvalidSequential(invalidhandle, i))) &&
                    (p_pos[i]>=clippoint1.x && p_pos[i]<=clippoint2.x &&
                     p_pos[i+1]>=clippoint1.x && p_pos[i+1]<=clippoint2.x )) {

                if (acolors && !clipregcolors) {
                    switch (colorstype) {
                        case (TYPE_FLOAT):
                                    color = color_ptr[i];
                        break;
                        case (TYPE_UBYTE):
                                    color = color_ptr[(int)color_ubyte_ptr[i]];
                        break;
                        default:
                        break;
                    }
                    if (!DXAddColor((Field)ino, count_c, color))
                        goto error;
                }

                /* now add the current points */
                /* XXX mindata? */
                newpoint = DXPt(p_pos[i], clippoint1.y,  0.0);
                if (!DXAddArrayData(aposnew,  count_p, 1, (Pointer)&newpoint))
                    goto error;
                count_p ++;
        switch (type) {
                    case (TYPE_UBYTE):
                                dataval = (float)p_data_ub[i];
                    break;
                    case (TYPE_SHORT):
                                dataval = (float)p_data_s[i];
                    break;
                    case (TYPE_INT):
                                dataval = (float)p_data_i[i];
                    break;
                    case (TYPE_FLOAT):
                                dataval = (float)p_data_f[i];
                    break;
                    case (TYPE_DOUBLE):
                                dataval = (float)p_data_d[i];
                    break;
                    case (TYPE_BYTE):
                                dataval = (float)p_data_b[i];
                    break;
                    case (TYPE_USHORT):
                                dataval = (float)p_data_us[i];
                    break;
                    case (TYPE_UINT):
                                dataval = (float)p_data_ui[i];
                    break;
                    default:
                    DXSetError(ERROR_DATA_INVALID,"#11829", "data");
                    goto error;
                }
                if (dataval < clippoint1.y)
                    newpoint = DXPt(p_pos[i], clippoint1.y,  0.0);
                else if (dataval > clippoint2.y)
                    newpoint = DXPt(p_pos[i], clippoint2.y,  0.0);
                else
                    newpoint = DXPt(p_pos[i], dataval,  0.0);
                if (!DXAddArrayData(aposnew, count_p, 1, (Pointer)&newpoint))
                    goto error;
                count_p ++;
                newpoint = DXPt(p_pos[i+1], clippoint1.y,  0.0);
                if (!DXAddArrayData(aposnew, count_p, 1, (Pointer)&newpoint))
                    goto error;
                count_p ++;
                if (dataval < clippoint1.y)
                    newpoint = DXPt(p_pos[i+1], clippoint1.y,  0.0);
                else if (dataval > clippoint2.y)
                    newpoint = DXPt(p_pos[i+1], clippoint2.y,  0.0);
                else
                    newpoint = DXPt(p_pos[i+1], dataval,  0.0);
                if (!DXAddArrayData(aposnew, count_p, 1, (Pointer)&newpoint))
                    goto error;
                count_p ++;
                newconquad = DXQuad(count_p-4, count_p-3, count_p-2, count_p-1);
                if (!(DXAddArrayData(aconnew, count_c, 1, (Pointer)&newconquad)))
                    goto error;
                count_c++;
            }
        }
        DXSetComponentValue((Field)ino,"positions",(Object)aposnew);
        aposnew = NULL;
        DXSetComponentValue((Field)ino,"connections",(Object)aconnew);
        aconnew = NULL;
        DXSetComponentAttribute((Field)ino,"connections","ref",
                                (Object)DXNewString("positions"));
        DXSetComponentAttribute((Field)ino,"connections","element type",
                                (Object)DXNewString("quads"));
        if (!clipregcolors)
            DXSetComponentAttribute((Field)ino,"colors","dep",
                                    (Object)DXNewString("connections"));
        conncount = count_c;
        if (conncount == 0)
            nooutput=1;
        else
            nooutput =0;
    }

    if (nooutput)
        DXWarning("Plot: corners exclude entire line");

    /* check if there's a colors component already */
if ((!(DXGetComponentValue((Field)ino, "colors")))&&(!clipregcolors)) {
        /* if it doesn't give it one */
        acolorsnew = (Array)DXNewConstantArray(conncount, (Pointer)&defaultcolor,
                                               TYPE_FLOAT, CATEGORY_REAL, 1, 3);
        if (!(DXSetComponentValue((Field)ino,"colors",(Object)acolorsnew))) {
            goto error;
        }
        acolorsnew = NULL;
        if (!(DXSetComponentAttribute((Field)ino,"colors", "dep",
                                      (Object)DXNewString("connections")))) {
            goto error;
        }
    } else {
        if (clipregcolors) {
            acolorsnew = (Array)DXNewConstantArray(conncount, (Pointer)&origin,
                                                   TYPE_FLOAT, CATEGORY_REAL, 1, 3);
            if (!(DXSetComponentValue((Field)ino,"colors",(Object)acolorsnew))) {
                goto error;
            }
            acolorsnew = NULL;
            if (!(DXSetComponentAttribute((Field)ino,"colors", "dep",
                                          (Object)DXNewString("connections")))) {
                goto error;
            }
        }
    }

    if (!DXChangedComponentValues((Field)ino,"positions"))
        goto error;
    if (!DXChangedComponentValues((Field)ino,"colors"))
        goto error;
    if (!DXChangedComponentValues((Field)ino,"connections"))
        goto error;
    /* delete the data component because #positions may be different */
    DXDeleteComponent((Field)ino,"data");
    if (!(DXEndField((Field)ino)))
        goto error;

    if (!DXFreeInvalidComponentHandle(invalidhandle))
        goto error;
    return OK;

error:
    DXDelete((Object)aposnew);
    DXFreeInvalidComponentHandle(invalidhandle);
    DXDelete((Object)aconnew);
    DXDelete((Object)acolorsnew);
    return ERROR;
}

static Error PlotBarDepPositions(struct arg *arg) {
    Array apos, acolors, aposnew, aconnew, acolorsnew, adata;
    Array colormap;
    Array invalid;
    InvalidComponentHandle invalidhandle=NULL;
    Type colorstype;
    Category category;
    int rank, shape[32];
    float *p_pos, *p_posnew, pos_delta;
    float *p_data_f=NULL;
    int *p_data_i=NULL;
    uint *p_data_ui=NULL;
    short *p_data_s=NULL;
    ushort *p_data_us=NULL;
    ubyte *p_data_ub=NULL;
    byte *p_data_b=NULL;
    double *p_data_d=NULL;
    Type type;
    float dataval;
    int *dp_conn, numitems;
    int poscount, datacount, i, needtoclip, count_p;
    int clipregcolors, regcolors, gotcolors;
    int nooutput=0;
    RGBColor *p_colors=NULL, *p_colorsnew=NULL, color = {0,0,0}, origin;
    Point clippoint1, clippoint2;
    Object ino;
    char *p_ubyte_colors=NULL;
    int siblingCount = arg->siblingCount;
    int siblingNumber = arg->siblingNumber;
    float width;

    aposnew=NULL;
    aconnew=NULL;
    acolorsnew=NULL;
    clipregcolors=0;
    regcolors=0;
    gotcolors=0;

    ino = arg->ino;
    needtoclip = arg->needtoclip;
    clippoint1 = arg->clippoint1;
    clippoint2 = arg->clippoint2;
    /*epsilonx = arg->epsilonx;*/
    /*epsilony = arg->epsilony;*/

    /* check for invalid component */
    invalid = (Array)DXGetComponentValue((Field)ino,"invalid positions");
    if (invalid) {
        invalidhandle = DXCreateInvalidComponentHandle((Object)ino,NULL,
                        "positions");
        if (!invalidhandle)
            goto error;
    }

    /* extract, typecheck, and get the data from the ``positions''
       component */
    apos = (Array) DXGetComponentValue((Field)ino, "positions");
    if (!apos) {
        DXSetError(ERROR_MISSING_DATA, "#10250", "input" ,"positions");
        return ERROR;
    }
    if ((!DXTypeCheck(apos, TYPE_FLOAT, CATEGORY_REAL, 1, 1))&&
            (!DXTypeCheck(apos, TYPE_FLOAT, CATEGORY_REAL, 0, NULL))) {
        DXSetError(ERROR_BAD_TYPE, "#11364", "positions", 1);
        return ERROR;
    }
    p_pos = (float *) DXGetArrayData(apos);
    /* find out how many positions there are */
    if (!(DXGetArrayInfo(apos, &poscount, NULL, NULL, NULL, NULL))) {
        return ERROR;
    }
    
    pos_delta = abs(p_pos[1]-p_pos[0]);

    /* check the colors component, if present */
    if ((acolors = (Array)DXGetComponentValue((Field)ino, "colors"))) {
        DXGetArrayInfo(acolors, &numitems, &colorstype, &category, &rank, shape);
        if (!DXQueryConstantArray(acolors,NULL,NULL)) {
            regcolors = 0;
            switch (colorstype) {
                case (TYPE_FLOAT):
                    if ((rank!=1)||(shape[0]!=3)) {
                        DXSetError(ERROR_DATA_INVALID,"#11363", "colors", 3);
                        goto error;
                    }
                    p_colors = (RGBColor *)DXGetArrayData(acolors);
                    break;
                case (TYPE_UBYTE):
                    if (!((rank==0)||((rank==1)&&(shape[0]==1)))) {
                        DXSetError(ERROR_DATA_INVALID,"#11363", "byte colors", 1);
                        goto error;
                    }
                    p_ubyte_colors = (char *)DXGetArrayData(acolors);
                    colormap = (Array)DXGetComponentValue((Field)ino,"color map");
                    if (!colormap) {
                         DXSetError(ERROR_MISSING_DATA, "#13050");
                         goto error;
                    }
                    p_colors = (RGBColor *)DXGetArrayData(colormap);
                    break;
                default:
                    DXSetError(ERROR_DATA_INVALID,"#11838", "colors component");
                    goto error;
            }
            acolorsnew = DXNewArray(TYPE_FLOAT,CATEGORY_REAL,1,3);
            
        } else {
            regcolors = 1;
            gotcolors = 1;
        }
    } else {
        /* No current colors component */
        /* Create a constant array dep positions with default color */
		regcolors = 1;
		gotcolors = 0;
    }

    /* Now make a brand-new positions array. It will be two dimensional */
    if (!(aposnew = DXNewArray(TYPE_FLOAT, CATEGORY_REAL, 1, 2))) {
        return ERROR;
    }
    /* Now get a pointer to the data component of the input object */
    if (!(adata = (Array)DXGetComponentValue((Field)ino, "data"))) {
        DXSetError(ERROR_BAD_PARAMETER,"#10250", "input", "data");
        goto error;
    }
    if (!DXGetArrayInfo(adata, &datacount, &type, &category, &rank, shape))
        goto error;
    if (category != CATEGORY_REAL) {
        DXSetError(ERROR_DATA_INVALID, "data are not category real");
        goto error;
    }
    if (!((rank==0) ||((rank==1)&&(shape[0]==1)))) {
        DXSetError(ERROR_DATA_INVALID,"#11363", "data", 1);
        goto error;
    }

    switch (type) {
        case (TYPE_UBYTE):
            if (!(p_data_ub = (ubyte *)DXGetArrayData(adata)))
                goto error;
            break;
        case (TYPE_SHORT):
            if (!(p_data_s = (short *)DXGetArrayData(adata)))
                goto error;
            break;
        case (TYPE_INT):
            if (!(p_data_i = (signed int *)DXGetArrayData(adata)))
                goto error;
            break;
        case (TYPE_FLOAT):
            if (!(p_data_f = (float *)DXGetArrayData(adata)))
                goto error;
            break;
        case (TYPE_DOUBLE):
            if (!(p_data_d = (double *)DXGetArrayData(adata)))
                goto error;
            break;
        case (TYPE_BYTE):
            if (!(p_data_b = (byte *)DXGetArrayData(adata)))
                goto error;
            break;
        case (TYPE_USHORT):
            if (!(p_data_us = (ushort *)DXGetArrayData(adata)))
                goto error;
            break;
        case (TYPE_UINT):
            if (!(p_data_ui = (uint *)DXGetArrayData(adata)))
                goto error;
            break;
        default:
            DXSetError(ERROR_DATA_INVALID,"#11829", "data");
            goto error;
    }

    if(datacount != poscount) {
        DXSetError(ERROR_BAD_PARAMETER, "#11161", "position-dependent data items",
                   datacount, "number of positions", poscount);
        goto error;
    }

    /* if there's an invalid component, we need to check every point */
    if (invalid)
        needtoclip = 1;

	if (!DXAddArrayData(aposnew,0,(4*poscount),NULL))
		goto error;

	/* get a pointer to the new positions component */
	if (!(p_posnew = (float *)DXGetArrayData(aposnew))) {
		goto error;
	}
	
	if(!regcolors) {
		if (!DXAddArrayData(acolorsnew,0,(4*poscount),NULL))
			goto error;
			
		if (!(p_colorsnew = (RGBColor *)DXGetArrayData(acolorsnew))) {
			goto error;
		}
	}

	/* create the output positions where x is the original position, and
	   y is the data value (essentially doing a rubbersheet)  */
	
	count_p = 0;
	
	for (i=0; i<poscount; i++)  {

//         if (( !(invalid && DXIsElementInvalidSequential(invalidhandle, i))) &&
//                    (p_pos[i]>=clippoint1.x && p_pos[i]<=clippoint2.x)) {
		switch (type) {
			case (TYPE_UBYTE):
						dataval = (float)p_data_ub[i];
			break;
			case (TYPE_SHORT):
						dataval = (float)p_data_s[i];
			break;
			case (TYPE_INT):
						dataval = (float)p_data_i[i];
			break;
			case (TYPE_FLOAT):
						dataval = (float)p_data_f[i];
			break;
			case (TYPE_DOUBLE):
						dataval = (float)p_data_d[i];
			break;
			case (TYPE_BYTE):
						dataval = (float)p_data_b[i];
			break;
			case (TYPE_USHORT):
						dataval = (float)p_data_us[i];
			break;
			case (TYPE_UINT):
						dataval = (float)p_data_ui[i];
			break;
			default:
			DXSetError(ERROR_DATA_INVALID,"#11829", "data");
			goto error;
		}


        /* Construct the new positions that are the corners of the column of
           for each data value. Make them a width so that there is space between
           consecutive groupings. The sibling information gives us the groupings
           of fields to group the columns. pos_delta is the difference between
           two columns.
         */
                                       
        width =  pos_delta/(siblingCount+1);
        
		p_posnew[8*count_p] = p_pos[i]+width*((siblingCount/-2.0)+(siblingNumber));
		p_posnew[8*count_p+1] = 0.0;
		p_posnew[8*count_p+2] = p_pos[i]+width*((siblingCount/-2.0)+(siblingNumber));
		p_posnew[8*count_p+3] = dataval;
		p_posnew[8*count_p+4] = p_pos[i]+width*((siblingCount/-2.0)+(siblingNumber+1));
		p_posnew[8*count_p+5] = 0.0;
		p_posnew[8*count_p+6] = p_pos[i]+width*((siblingCount/-2.0)+(siblingNumber+1));
		p_posnew[8*count_p+7] = dataval;
		
		if(!regcolors) {
			switch (colorstype) {
			case (TYPE_FLOAT) :
				color = p_colors[i];
				break;
			case (TYPE_UBYTE) :
				color = p_colors[(int)p_ubyte_colors[i]];
				break;
			default:
				break;
			}
			p_colorsnew[4*count_p] = color;
			p_colorsnew[4*count_p+1] = color;
			p_colorsnew[4*count_p+2] = color;
			p_colorsnew[4*count_p+3] = color;
		}
		count_p++;

//		}
	}
	
    if (!(DXSetComponentValue((Field)ino, "positions", (Object)aposnew))) {
		goto error;
	}
	aposnew = NULL;

	/* make quads */
	aconnew = (Array)DXNewArray(TYPE_INT,CATEGORY_REAL,1,4);
	aconnew = DXAddArrayData(aconnew,0,count_p,NULL);
	if (!aconnew)
		return ERROR;
	dp_conn = (int *)DXGetArrayData(aconnew);
	for (i=0; i<count_p; i++) {
		dp_conn[4*i]   = 4*i;
		dp_conn[4*i+1] = 4*i+1;
		dp_conn[4*i+2] = 4*i+2;
		dp_conn[4*i+3] = 4*i+3;
	}

	if (!DXSetComponentValue((Field)ino, "connections", (Object)aconnew)) {
		goto error;
	}
	aconnew = NULL;
	if (!DXSetComponentAttribute((Field)ino, "connections",
								 "ref", (Object)DXNewString("positions"))) {
		goto error;
	}
	if (!DXSetComponentAttribute((Field)ino, "connections",
								 "element type", (Object)DXNewString("quads"))) {
		goto error;
	}

    /* check the colors component, if present */
    if (regcolors == 1) {
        if(gotcolors == 0) {
            /* No current colors component */
            /* Create a constant array dep positions with default color */
            origin = DEFAULTCOLOR;
        } else {
            DXQueryConstantArray((Array)acolors, NULL, (Pointer)&origin);
        }
        acolorsnew = (Array)DXNewConstantArray((count_p*4), (Pointer)&origin,
                                                   TYPE_FLOAT, CATEGORY_REAL, 1, 3);
    } 

    if (nooutput)
        DXWarning("Plot: corners exclude all columns.");

    if (!(DXSetComponentValue((Field)ino,"colors",(Object)acolorsnew))) {
        goto error;
    }
    acolorsnew = NULL;
    if (!(DXSetComponentAttribute((Field)ino,"colors", "dep",
                                  (Object)DXNewString("positions")))) {
        goto error;
    }

    if (!DXChangedComponentValues((Field)ino,"positions"))
        goto error;
    if (!DXChangedComponentValues((Field)ino,"colors"))
        goto error;
    if (!DXChangedComponentValues((Field)ino,"connections"))
        goto error;
    /* delete the data component because #positions may be different */
    DXDeleteComponent((Field)ino,"data");
    DXDeleteComponent((Field)ino,"invalid positions");
    DXDeleteComponent((Field)ino,"invalid connections");
    if (!(DXEndField((Field)ino)))
        goto error;

    if (!DXFreeInvalidComponentHandle(invalidhandle))
        goto error;
    return OK;

error:
    DXDelete((Object)aposnew);
    DXFreeInvalidComponentHandle(invalidhandle);
    DXDelete((Object)aconnew);
    DXDelete((Object)acolorsnew);
    return ERROR;
}


static Error PlotDepPositions(struct arg *arg) {
    Object markobject=NULL, linetypeobject=NULL, scatterobject=NULL;
    Array apos, acolors, aposnew, aconnew, acolorsnew, adata;
    Array colormap;
    Array invalid;
    InvalidComponentHandle invalidhandle=NULL;
    Type colorstype, datatype;
    Category category, datacat;
    float dataval;
    int splat = 0, markcount = 0, rank, shape[32],numitems;
    int datarank, datashape[8];
    int lastnewindex=0;
    char  *linetype="solid", coldepatt[30];
    short *p_data_s=NULL;
    ushort *p_data_us=NULL;
    double *p_data_d=NULL;
    byte *p_data_b=NULL;
    ubyte *p_data_ub=NULL;
    float *p_data_f=NULL;
    int *p_data_i=NULL;
    uint *p_data_ui=NULL;
    float *p_pos;
    int poscount, datacount, i, count_p, count_c;
    int thisok, lastok, regcolors;
    int nooutput=0;
    RGBColor color, origin, *p_colors=NULL;
    char *p_ubyte_colors=NULL;
    Point clippoint1, clippoint2, newpoint;
    Object ino;
    float epsilonx, epsilony;
    int mark, markevery, scatterflag=0;
    float markscale;
    char *marker;

    aposnew=NULL;
    aconnew=NULL;
    acolorsnew=NULL;
    regcolors=0;

    ino = arg->ino;
    /*needtoclip = arg->needtoclip;*/
    clippoint1 = arg->clippoint1;
    clippoint2 = arg->clippoint2;
    epsilonx = arg->epsilonx;
    epsilony = arg->epsilony;
    mark = arg->mark;
    markevery = arg->markevery;
    marker = arg->marker;
    markscale = arg->markscale;
    scatterflag = arg->scatter;

    /* check for an invalid component */
    invalid = (Array)DXGetComponentValue((Field)ino, "invalid positions");
    if (invalid) {
        invalidhandle = DXCreateInvalidComponentHandle((Object)ino,NULL,
                        "positions");
        if (!invalidhandle)
            goto error;
    }


    /*stepon = 4*epsilonx;*/
    /*stepoff = stepon/2.0;*/
    /*factor = epsilony/epsilonx;*/

    /* extract, typecheck, and get the data from the ``positions''
       component */
    apos = (Array) DXGetComponentValue((Field)ino, "positions");
    if (!apos) {
        DXSetError(ERROR_MISSING_DATA, "#10250", "input", "positions");
        return ERROR;
    }
    if ((!DXTypeCheck(apos, TYPE_FLOAT, CATEGORY_REAL, 1, 1))&&
            (!DXTypeCheck(apos, TYPE_FLOAT, CATEGORY_REAL, 0, NULL))) {
        DXSetError(ERROR_BAD_TYPE, "#11364", "positions", 1);
        return ERROR;
    }

    /* find out how many positions there are */
    if (!(DXGetArrayInfo(apos, &poscount, NULL, NULL, NULL, NULL))) {
        return ERROR;
    }

    p_pos = (float *) DXGetArrayData(apos);
    /* check the colors component, if present */
    if (DXGetComponentValue((Field)ino,"colors")) {
        if (!(DXGetString((String)DXGetComponentAttribute((Field)ino,
                          "colors","dep")))) {
            DXSetError (ERROR_DATA_INVALID, "#10241", "colors");
            goto error;
        }
        strcpy(coldepatt,DXGetString((String)DXGetComponentAttribute((Field)ino,
                                     "colors","dep")));
        if (strcmp(coldepatt,"positions")) {
            DXSetError(ERROR_DATA_INVALID, "#10360", "data", "colors");
            goto error;
        }
    }
    /* Now make a brand-new positions array. It will be two dimensional */
    if (!(aposnew = DXNewArray(TYPE_FLOAT, CATEGORY_REAL, 1, 2))) {
        return ERROR;
    }
    /* Now get a pointer to the data component of the input object */
    if (!(adata = (Array)DXGetComponentValue((Field)ino, "data"))) {
        DXSetError(ERROR_BAD_PARAMETER,"#10250", "input","data");
        goto error;
    }
    if (!DXGetArrayInfo(adata, &datacount, &datatype, &datacat, &datarank,
                        datashape))
        goto error;
    if (datacat != CATEGORY_REAL) {
        DXSetError(ERROR_DATA_INVALID,"data are not category real");
        goto error;
    }
    if (!((datarank==0)||((datarank==1)&&(datashape[0]==1)))) {
        DXSetError(ERROR_DATA_INVALID,"#11363", "data",1);
        goto error;
    }
    switch (datatype) {
        case (TYPE_UBYTE):
                    if (!(p_data_ub = (ubyte *)DXGetArrayData(adata)))
                        goto error;
        break;
        case (TYPE_SHORT):
                    if (!(p_data_s = (short *)DXGetArrayData(adata)))
                        goto error;
        break;
        case (TYPE_INT):
                    if (!(p_data_i = (signed int *)DXGetArrayData(adata)))
                        goto error;
        break;
        case (TYPE_FLOAT):
                    if (!(p_data_f = (float *)DXGetArrayData(adata)))
                        goto error;
        break;
        case (TYPE_DOUBLE):
                    if (!(p_data_d = (double *)DXGetArrayData(adata)))
                        goto error;
        break;
        case (TYPE_BYTE):
                    if (!(p_data_b= (byte *)DXGetArrayData(adata)))
                        goto error;
        break;
        case (TYPE_USHORT):
                    if (!(p_data_us= (ushort *)DXGetArrayData(adata)))
                        goto error;
        break;
        case (TYPE_UINT):
                    if (!(p_data_ui= (uint *)DXGetArrayData(adata)))
                        goto error;
        break;
        default:
        DXSetError(ERROR_DATA_INVALID,"#11829", "data");
        goto error;
    }

    /* first set things up using the marks coming in from above */
    if (mark)
        splat = 1;
    /* override if things are set at the field level */
    markobject = DXGetAttribute((Object)ino, "mark");
    scatterobject = DXGetAttribute((Object)ino,"scatter");
if (scatterobject) {
        if (!DXExtractInteger(scatterobject, &scatterflag)) {
            DXSetError(ERROR_DATA_INVALID,
                       "scatter attribute must be 0 or 1");
            goto error;
        }
        if ((scatterflag!=0)&&(scatterflag!=1)) {
            DXSetError(ERROR_DATA_INVALID,
                       "scatter attribute must be 0 or 1");
            goto error;
        }
    }

    if ((scatterflag == 1)&&(!markobject)) {
        marker = "x";
        splat = 1;
    }



    if (markobject) {
        splat = 1;
        if (!DXExtractNthString(markobject, 0, &marker)) {
            DXSetError(ERROR_DATA_INVALID,"#10200", "mark");
            goto error;
        }
    }
    if (DXGetAttribute((Object)ino,"mark every")) {
        if (!DXExtractInteger(DXGetAttribute((Object)ino, "mark every"),
                              &markevery)) {
            DXSetError(ERROR_DATA_INVALID, "#10020", "mark every attribute");
            goto error;
        }
        if (markevery < 0) {
            DXSetError(ERROR_DATA_INVALID,
                       "mark every attribute must non-negative");
            goto error;
        }
    }

    if (DXGetAttribute((Object)ino,"mark scale")) {
        if (!DXExtractFloat(DXGetAttribute((Object)ino, "mark scale"),
                            &markscale)) {
            DXSetError(ERROR_DATA_INVALID,"#10090", "mark scale attribute");
            goto error;
        }
        if (markscale <= 0) {
            DXSetError(ERROR_DATA_INVALID,"#10090", "mark scale attribute");
            goto error;
        }
    }

    epsilonx = epsilonx*markscale;
    epsilony = epsilony*markscale;
    linetypeobject = DXGetAttribute((Object)ino, "linetype");
    if (linetypeobject) {
        if (!DXExtractNthString(linetypeobject, 0, &linetype)) {
            DXSetError(ERROR_DATA_INVALID,"#10200", "linetype");
            goto error;
        }
    }
    if (strcmp(linetype,"solid")) {
        DXWarning("Plot: only solid lines supported");
        linetype = "solid";
    }

    /*needtoclip = 1;*/
    if (datacount != poscount) {
        DXSetError(ERROR_BAD_PARAMETER, "#11161", "position-dependent data items",
                   datacount, "number of positions", poscount);
        goto error;
    }

    if ((acolors = (Array)DXGetComponentValue((Field)ino, "colors"))) {
        DXGetArrayInfo(acolors, &numitems, &colorstype, &category, &rank, shape);
        if (!DXQueryConstantArray(acolors,NULL,NULL)) {
            regcolors = 0;
            switch (colorstype) {
                case (TYPE_FLOAT):
                    if ((rank!=1)||(shape[0]!=3)) {
                        DXSetError(ERROR_DATA_INVALID,"#11363", "colors", 3);
                        goto error;
                    }
                p_colors = (RGBColor *)DXGetArrayData(acolors);
                break;
                case (TYPE_UBYTE):
                    if (!((rank==0)||((rank==1)&&(shape[0]==1)))) {
                        DXSetError(ERROR_DATA_INVALID,"#11363", "byte colors", 1);
                        goto error;
                    }
                p_ubyte_colors = (char *)DXGetArrayData(acolors);
                colormap = (Array)DXGetComponentValue((Field)ino,"color map");
                if (!colormap) {
                    DXSetError(ERROR_MISSING_DATA, "#13050");
                    goto error;
                }
                p_colors = (RGBColor *)DXGetArrayData(colormap);
                break;
                default:
                DXSetError(ERROR_DATA_INVALID,"#11838", "colors component");
                goto error;
            }
            acolorsnew = DXNewArray(TYPE_FLOAT,CATEGORY_REAL,1,3);
        } else {
            regcolors = 1;
            DXQueryConstantArray((Array)acolors, NULL, (Pointer)&origin);
        }
    } else {
        /* we'll add the default color */
        regcolors = 1;
        origin = DEFAULTCOLOR;
    }
    aconnew = DXNewArray(TYPE_INT,CATEGORY_REAL, 1, 2);
    count_p = 0;
    count_c = 0;
    lastok = 0;
    thisok = 0;
    color = DXRGB(0.0, 0.0, 0.0);
    for (i=0; i<poscount; i++) {
        switch (datatype) {
            case (TYPE_UBYTE):
                        dataval = (float)p_data_ub[i];
            break;
            case (TYPE_SHORT):
                        dataval = (float)p_data_s[i];
            break;
            case (TYPE_INT):
                        dataval = (float)p_data_i[i];
            break;
            case (TYPE_FLOAT):
                        dataval = (float)p_data_f[i];
            break;
            case (TYPE_DOUBLE):
                        dataval = (float)p_data_d[i];
            break;
            case (TYPE_BYTE):
                        dataval = (float)p_data_b[i];
            break;
            case (TYPE_USHORT):
                        dataval = (float)p_data_us[i];
            break;
            case (TYPE_UINT):
                        dataval = (float)p_data_ui[i];
            break;
            default:
            DXSetError(ERROR_DATA_INVALID,"#11829", "data");
            goto error;
        }

        if ( (!(invalid && DXIsElementInvalidSequential(invalidhandle, i))) &&
                p_pos[i]>=clippoint1.x && p_pos[i]<=clippoint2.x &&
        dataval>=clippoint1.y && dataval<=clippoint2.y) {
            thisok = 1;
            if (!regcolors)
                switch (colorstype) {
                    case (TYPE_FLOAT):
                                color = p_colors[i];
                    break;
                    case (TYPE_UBYTE):
                                color = p_colors[(int)p_ubyte_colors[i]];
                    break;
                    default:
                    break;
                }
            /* now add the current point */
    if ((thisok)&&(lastok)) {
                /* first draw the line from old point to where I am now */
                /* only if scatter != 1 */
                if (scatterflag != 1) {
                    if (!DrawLine(aconnew, aposnew, &count_c, &count_p,
                                  lastnewindex, p_pos[i], dataval,
                                  regcolors, color, acolorsnew))
                        goto error;
                }
            }
            /* else don't draw the line, just the point */
            else {
                newpoint = DXPt(p_pos[i], dataval, 0.0);
                if (!DXAddArrayData(aposnew, count_p, 1, (Pointer)&newpoint))
                    goto error;
                if (!regcolors) {
                    if (!DXAddArrayData(acolorsnew, count_p, 1, (Pointer)&color))
                        goto error;
                }
                count_p++;
            }
            /*lastorigindex = i;*/
            lastnewindex = count_p-1;
            if (splat) {
                if ((markcount % markevery )==0) {
                    if (!strcmp(marker,"x")) {
                        if (!AddX(aposnew, &count_p, aconnew, &count_c, p_pos[i],
                                  dataval, epsilonx, epsilony,regcolors,color,
                                  acolorsnew ))
                            goto error;
                    } else if (!strcmp(marker,"triangle")) {
                        if (!AddTri(aposnew, &count_p, aconnew, &count_c, p_pos[i],
                                    dataval, epsilonx, epsilony,regcolors,color,
                                    acolorsnew))
                            goto error;
                    } else if (!strcmp(marker,"square")) {
                        if (!AddSquare(aposnew, &count_p, aconnew, &count_c,
                                       p_pos[i], dataval, epsilonx, epsilony,
                                       regcolors, color, acolorsnew))
                            goto error;
                    } else if (!strcmp(marker,"circle")) {
                        if (!AddCircle(aposnew, &count_p, aconnew, &count_c,
                                       p_pos[i], dataval, epsilonx, epsilony,
                                       regcolors, color, acolorsnew))
                            goto error;
                    } else if (!strcmp(marker,"star")) {
                        if (!AddStar(aposnew, &count_p, aconnew, &count_c, p_pos[i],
                                     dataval, epsilonx, epsilony, regcolors,
                                     color, acolorsnew))
                            goto error;
                    } else if (!strcmp(marker,"diamond")) {
                        if (!AddDiamond(aposnew, &count_p, aconnew, &count_c,
                                        p_pos[i], dataval, epsilonx, epsilony,
                                        regcolors, color, acolorsnew))
                            goto error;
                    } else if (!strcmp(marker,"dot")) {
                        if (!AddDot(aposnew, &count_p, aconnew, &count_c,
                                    p_pos[i], dataval, epsilonx, epsilony,
                                    regcolors, color, acolorsnew))
                            goto error;
                    } else if (!strcmp(marker,"")) {}
                    else {
                        DXSetError(ERROR_BAD_PARAMETER,"#10203", "mark",
                                   "x, triangle, square, circle, star, dots, diamond");
                        goto error;
                    }
                }
                markcount++;
            }
            lastok=1;
        } else {
            lastok = 0;
        }
    }
    DXSetComponentValue((Field)ino,"positions",(Object)aposnew);
    aposnew = NULL;
    DXSetComponentValue((Field)ino,"connections",(Object)aconnew);
    aconnew = NULL;
    DXSetComponentAttribute((Field)ino,"connections","ref",
                            (Object)DXNewString("positions"));
    DXSetComponentAttribute((Field)ino,"connections","element type",
                            (Object)DXNewString("lines"));


    poscount = count_p;
    if (poscount == 0)
        nooutput=1;
    else
        nooutput =0;
    if (nooutput) {
        DXWarning("Plot: corners exclude entire line");
        DXDelete((Object)ino);
        ino = (Object)DXNewField();
    }

    /* delete the invalid positions and invalid connections */

    DXDeleteComponent((Field)ino,"invalid positions");
    DXDeleteComponent((Field)ino,"invalid connections");


    /* check if there's a colors component already */
    if (regcolors) {
        acolorsnew = (Array)DXNewConstantArray(poscount, (Pointer)&origin,
                                               TYPE_FLOAT, CATEGORY_REAL, 1, 3);
        if (!(DXSetComponentValue((Field)ino,"colors",(Object)acolorsnew))) {
            goto error;
        }
        acolorsnew = NULL;
        if (!(DXSetComponentAttribute((Field)ino,"colors", "dep",
                                      (Object)DXNewString("positions")))) {
            goto error;
        }
    } else {
        DXSetComponentValue((Field)ino,"colors", (Object)acolorsnew);
        acolorsnew = NULL;
        DXSetComponentAttribute((Field)ino,"colors","dep",
                                (Object)DXNewString("positions"));
    }
    if (!DXChangedComponentValues((Field)ino,"positions"))
        goto error;
    if (!DXChangedComponentValues((Field)ino,"colors"))
        goto error;
    if (!DXChangedComponentValues((Field)ino,"connections"))
        goto error;
    if (!DXChangedComponentValues((Field)ino,"invalid connections"))
        goto error;
    if (!DXChangedComponentValues((Field)ino,"invalid positions"))
        goto error;
    /* delete the data component because #positions may be different */
    DXDeleteComponent((Field)ino,"data");
    if (!(DXEndField((Field)ino)))
        goto error;

    if (!DXFreeInvalidComponentHandle(invalidhandle))
        goto error;
    return OK;

error:
    DXFreeInvalidComponentHandle(invalidhandle);
    DXDelete((Object)aposnew);
    DXDelete((Object)aconnew);
    DXDelete((Object)acolorsnew);
    return ERROR;
}

static Error DrawLine(Array aconnew, Array aposnew, int *count_c,
                      int *count_p, int lastindex, float x, float y,
                      int regcolors, RGBColor color, Array acolorsnew) {
    Point newpoint;
    Line newcon;
    int tmp_p, tmp_c;

    tmp_p = *count_p;
    tmp_c = *count_c;

    newpoint = DXPt(x, y,0.0);
    if (!DXAddArrayData(aposnew, tmp_p, 1, (Pointer)&newpoint))
        goto error;
    if (!regcolors)
        if (!DXAddArrayData(acolorsnew, tmp_p, 1, (Pointer)&color))
            goto error;
    if (tmp_p > 0)
        newcon = DXLn(lastindex, tmp_p);
    if (!(DXAddArrayData(aconnew, tmp_c, 1, (Pointer)&newcon)))
        goto error;
    tmp_c++;
    tmp_p++;
    *count_p = tmp_p;
    *count_c = tmp_c;

    return OK;

error:
    return ERROR;
}


#if 0
static Error DrawDashed(Array aconnew, Array aposnew,
                        int *count_c, int *count_p, int lastindex,
                        float x, float y, float lastx, float lasty,
                        float stepon, float stepoff, float factor,
                        float *used, int *isline) {
    Point newpoint, direction, lastpoint, endpoint;
    float tmp_stepon, tmp_stepoff, angle, tmp_used;
    Line line;
    int tmp_p, tmp_c, num, i, first = 1;
    double dist, nextlen, nextdist, fraction;
    float scalefactor;


    tmp_p = *count_p;
    tmp_c = *count_c;
    lastpoint = DXPt(lastx, lasty, 0.0);
    endpoint = DXPt(x,y,0.0);

    dist = sqrt((x-lastx)*(x-lastx) + (y-lasty)*(y-lasty));
    if (dist > 0.0) {
        direction = DXMul(DXPt(x-lastx, y-lasty,0.0), 1.0/dist);
        angle = atan2(direction.y, direction.x);
        /* if this thing is going to be scaled, I need to worry about the
           line lengths; ie. the angle at which they run. Since the scaling
           will affect the length of the dashes */
        scalefactor = ABS(cos(angle)) + factor*(ABS(sin(angle)));
        tmp_stepon = stepon*scalefactor;
        tmp_stepoff = stepoff*scalefactor;
        tmp_used = *used*scalefactor;
        while (1) {
            nextdist = DXLength(DXSub(lastpoint, endpoint));
            direction = DXMul(DXSub(endpoint,lastpoint), 1.0/nextdist);
            nextlen = *isline?(tmp_stepon-tmp_used):(tmp_stepoff-tmp_used);
            *used = 0.0;
            tmp_used = 0.0;
            if (nextlen < nextdist) {
                newpoint = DXAdd(lastpoint, DXMul(direction, nextlen));
                if (!DXAddArrayData(aposnew, tmp_p, 1, (Pointer)&newpoint))
                    goto error;
                tmp_p++;
                lastpoint = newpoint;
                if (*isline) {
                    line = DXLn(lastindex, tmp_p-1);
                    if (!DXAddArrayData(aconnew, tmp_c, 1, (Pointer)&line))
                        goto error;
                    tmp_c++;
                }
                lastindex = tmp_p-1;
                *isline = !(*isline);
            } else {
                newpoint = endpoint;
                lastpoint = newpoint;
                if (!DXAddArrayData(aposnew, tmp_p, 1, (Pointer)&newpoint))
                    goto error;
                tmp_p++;
                if (*isline) {
                    line = DXLn(lastindex, tmp_p-1);
                    if (!DXAddArrayData(aconnew, tmp_c, 1, (Pointer)&line))
                        goto error;
                    tmp_c++;
                }
                lastindex = tmp_p-1;
                goto done;
            }
        }
    }
done:
    *used = nextdist;
    *count_p = tmp_p;
    *count_c = tmp_c;

    return OK;

error:
    return ERROR;
}
#endif

static
Error
AddDiamond(Array aposnew, int *count_p, Array aconnew, int *count_c,
           float x, float y, float epsilonx, float epsilony,
           int regcolors, RGBColor color, Array acolorsnew) {

    Point newpoint;
    Line  newcon;
    int tmp_p, tmp_c;

    tmp_p = *count_p;
    tmp_c = *count_c;
    newpoint = DXPt(x, y-1.2*epsilony, 0.0);
    if (!DXAddArrayData(aposnew, tmp_p, 1, (Pointer)&newpoint))
        goto error;
    if (!regcolors)
        if (!DXAddArrayData(acolorsnew, tmp_p, 1, (Pointer)&color))
            goto error;
    tmp_p++;
    newpoint = DXPt(x-1.2*epsilonx, y, 0.0);
    if (!DXAddArrayData(aposnew, tmp_p, 1, (Pointer)&newpoint))
        goto error;
    if (!regcolors)
        if (!DXAddArrayData(acolorsnew, tmp_p, 1, (Pointer)&color))
            goto error;
    tmp_p++;
    newpoint = DXPt(x, y+1.2*epsilony, 0.0);
    if (!DXAddArrayData(aposnew, tmp_p, 1, (Pointer)&newpoint))
        goto error;
    if (!regcolors)
        if (!DXAddArrayData(acolorsnew, tmp_p, 1, (Pointer)&color))
            goto error;
    tmp_p++;
    newpoint = DXPt(x+1.2*epsilonx, y, 0.0);
    if (!DXAddArrayData(aposnew, tmp_p, 1, (Pointer)&newpoint))
        goto error;
    if (!regcolors)
        if (!DXAddArrayData(acolorsnew, tmp_p, 1, (Pointer)&color))
            goto error;
    tmp_p++;
    newcon = DXLn(tmp_p-4, tmp_p-3);
    if (!(DXAddArrayData(aconnew, tmp_c, 1, (Pointer)&newcon)))
        goto error;
    tmp_c++;
    newcon = DXLn(tmp_p-3, tmp_p-2);
    if (!(DXAddArrayData(aconnew, tmp_c, 1, (Pointer)&newcon)))
        goto error;
    tmp_c++;
    newcon = DXLn(tmp_p-2, tmp_p-1);
    if (!(DXAddArrayData(aconnew, tmp_c, 1, (Pointer)&newcon)))
        goto error;
    tmp_c++;
    newcon = DXLn(tmp_p-1, tmp_p-4);
    if (!(DXAddArrayData(aconnew, tmp_c, 1, (Pointer)&newcon)))
        goto error;
    tmp_c++;
    *count_p = tmp_p;
    *count_c = tmp_c;
    return OK;

error:
    return ERROR;
}


static
Error
AddSquare(Array aposnew, int *count_p, Array aconnew, int *count_c,
          float x, float y, float epsilonx, float epsilony,
          int regcolors, RGBColor color, Array acolorsnew) {

    Point newpoint;
    Line  newcon;
    int tmp_p, tmp_c;

    tmp_p = *count_p;
    tmp_c = *count_c;
    newpoint = DXPt(x-epsilonx, y-epsilony, 0.0);
    if (!DXAddArrayData(aposnew, tmp_p, 1, (Pointer)&newpoint))
        goto error;
    if (!regcolors)
        if (!DXAddArrayData(acolorsnew, tmp_p, 1, (Pointer)&color))
            goto error;
    tmp_p++;
    newpoint = DXPt(x-epsilonx, y+epsilony, 0.0);
    if (!DXAddArrayData(aposnew, tmp_p, 1, (Pointer)&newpoint))
        goto error;
    if (!regcolors)
        if (!DXAddArrayData(acolorsnew, tmp_p, 1, (Pointer)&color))
            goto error;
    tmp_p++;
    newpoint = DXPt(x+epsilonx, y+epsilony, 0.0);
    if (!DXAddArrayData(aposnew, tmp_p, 1, (Pointer)&newpoint))
        goto error;
    if (!regcolors)
        if (!DXAddArrayData(acolorsnew, tmp_p, 1, (Pointer)&color))
            goto error;
    tmp_p++;
    newpoint = DXPt(x+epsilonx, y-epsilony, 0.0);
    if (!DXAddArrayData(aposnew, tmp_p, 1, (Pointer)&newpoint))
        goto error;
    if (!regcolors)
        if (!DXAddArrayData(acolorsnew, tmp_p, 1, (Pointer)&color))
            goto error;
    tmp_p++;
    newcon = DXLn(tmp_p-4, tmp_p-3);
    if (!(DXAddArrayData(aconnew, tmp_c, 1, (Pointer)&newcon)))
        goto error;
    tmp_c++;
    newcon = DXLn(tmp_p-3, tmp_p-2);
    if (!(DXAddArrayData(aconnew, tmp_c, 1, (Pointer)&newcon)))
        goto error;
    tmp_c++;
    newcon = DXLn(tmp_p-2, tmp_p-1);
    if (!(DXAddArrayData(aconnew, tmp_c, 1, (Pointer)&newcon)))
        goto error;
    tmp_c++;
    newcon = DXLn(tmp_p-1, tmp_p-4);
    if (!(DXAddArrayData(aconnew, tmp_c, 1, (Pointer)&newcon)))
        goto error;
    tmp_c++;
    *count_p = tmp_p;
    *count_c = tmp_c;
    return OK;

error:
    return ERROR;
}

static
Error
AddCircle(Array aposnew, int *count_p, Array aconnew, int *count_c,
          float x, float y, float epsilonx, float epsilony,
          int regcolors, RGBColor color, Array acolorsnew) {

    Point newpoint;
    Line  newcon;
    int tmp_p, tmp_c, i;


    tmp_p = *count_p;
    tmp_c = *count_c;
    for (i=0; i< 10; i++) {
        newpoint = DXPt(x+pointsarray[i].x*epsilonx,
                        y+pointsarray[i].y*epsilony,
                        0.0);
        if (!DXAddArrayData(aposnew, tmp_p, 1, (Pointer)&newpoint))
            goto error;
        if (!regcolors)
            if (!DXAddArrayData(acolorsnew, tmp_p, 1, (Pointer)&color))
                goto error;
        tmp_p++;
    }
    for (i=0; i<9; i++) {
        newcon = DXLn(tmp_p-10+i, tmp_p-9+i);
        if (!(DXAddArrayData(aconnew, tmp_c, 1, (Pointer)&newcon)))
            goto error;
        tmp_c++;
    }
    newcon = DXLn(tmp_p-1, tmp_p-10);
    if (!(DXAddArrayData(aconnew, tmp_c, 1, (Pointer)&newcon)))
        goto error;
    tmp_c++;
    *count_p = tmp_p;
    *count_c = tmp_c;
    return OK;

error:
    return ERROR;
}


static
Error
AddTri(Array aposnew, int *count_p, Array aconnew, int *count_c,
       float x, float y, float epsilonx, float epsilony,
       int regcolors, RGBColor color, Array acolorsnew) {

    Point newpoint;
    Line  newcon;
    int tmp_p, tmp_c;

    tmp_p = *count_p;
    tmp_c = *count_c;
    newpoint = DXPt(x, y+epsilony, 0.0);
    if (!DXAddArrayData(aposnew, tmp_p, 1, (Pointer)&newpoint))
        goto error;
    if (!regcolors)
        if (!DXAddArrayData(acolorsnew, tmp_p, 1, (Pointer)&color))
            goto error;
    tmp_p++;
    newpoint = DXPt(x-epsilonx, y-epsilony, 0.0);
    if (!DXAddArrayData(aposnew, tmp_p, 1, (Pointer)&newpoint))
        goto error;
    if (!regcolors)
        if (!DXAddArrayData(acolorsnew, tmp_p, 1, (Pointer)&color))
            goto error;
    tmp_p++;
    newpoint = DXPt(x+epsilonx, y-epsilony, 0.0);
    if (!DXAddArrayData(aposnew, tmp_p, 1, (Pointer)&newpoint))
        goto error;
    if (!regcolors)
        if (!DXAddArrayData(acolorsnew, tmp_p, 1, (Pointer)&color))
            goto error;
    tmp_p++;
    newcon = DXLn(tmp_p-3, tmp_p-2);
    if (!(DXAddArrayData(aconnew, tmp_c, 1, (Pointer)&newcon)))
        goto error;
    tmp_c++;
    newcon = DXLn(tmp_p-2, tmp_p-1);
    if (!(DXAddArrayData(aconnew, tmp_c, 1, (Pointer)&newcon)))
        goto error;
    tmp_c++;
    newcon = DXLn(tmp_p-1, tmp_p-3);
    if (!(DXAddArrayData(aconnew, tmp_c, 1, (Pointer)&newcon)))
        goto error;
    tmp_c++;
    *count_p = tmp_p;
    *count_c = tmp_c;
    return OK;

error:
    return ERROR;
}
static
Error
AddDot(Array aposnew, int *count_p, Array aconnew, int *count_conn,
       float x, float y, float epsilonx, float epsilony,
       int regcolors, RGBColor color, Array acolorsnew) {

    Point newpoint;
    Line  newcon;
    int tmp_p, tmp_c;

    tmp_p = *count_p;
    tmp_c = *count_conn;
    newpoint = DXPt(x, y, 0.0);
    if (!DXAddArrayData(aposnew, tmp_p, 1, (Pointer)&newpoint))
        goto error;
    if (!regcolors)
        if (!DXAddArrayData(acolorsnew, tmp_p, 1, (Pointer)&color))
            goto error;
    tmp_p++;
    newpoint = DXPt(x, y, 0.0);
    if (!DXAddArrayData(aposnew, tmp_p, 1, (Pointer)&newpoint))
        goto error;
    if (!regcolors)
        if (!DXAddArrayData(acolorsnew, tmp_p, 1, (Pointer)&color))
            goto error;
    tmp_p++;


    newcon = DXLn(tmp_p-2, tmp_p-1);
    if (!(DXAddArrayData(aconnew, tmp_c, 1, (Pointer)&newcon)))
        goto error;
    tmp_c++;

    *count_p = tmp_p;
    *count_conn = tmp_c;
    return OK;

error:
    return ERROR;
}


static
Error
AddStar(Array aposnew, int *count_p, Array aconnew, int *count_conn,
        float x, float y, float epsilonx, float epsilony,
        int regcolors, RGBColor color, Array acolorsnew) {

    Point newpoint;
    Line  newcon;
    int tmp_p, tmp_c;

    tmp_p = *count_p;
    tmp_c = *count_conn;
    newpoint = DXPt(x-epsilonx, y-epsilony, 0.0);
    if (!DXAddArrayData(aposnew, tmp_p, 1, (Pointer)&newpoint))
        goto error;
    if (!regcolors)
        if (!DXAddArrayData(acolorsnew, tmp_p, 1, (Pointer)&color))
            goto error;
    tmp_p++;
    newpoint = DXPt(x+epsilonx, y+epsilony, 0.0);
    if (!DXAddArrayData(aposnew, tmp_p, 1, (Pointer)&newpoint))
        goto error;
    if (!regcolors)
        if (!DXAddArrayData(acolorsnew, tmp_p, 1, (Pointer)&color))
            goto error;
    tmp_p++;
    newpoint = DXPt(x+epsilonx, y-epsilony, 0.0);
    if (!DXAddArrayData(aposnew, tmp_p, 1, (Pointer)&newpoint))
        goto error;
    if (!regcolors)
        if (!DXAddArrayData(acolorsnew, tmp_p, 1, (Pointer)&color))
            goto error;
    tmp_p++;
    newpoint = DXPt(x-epsilonx, y+epsilony, 0.0);
    if (!DXAddArrayData(aposnew, tmp_p, 1, (Pointer)&newpoint))
        goto error;
    if (!regcolors)
        if (!DXAddArrayData(acolorsnew, tmp_p, 1, (Pointer)&color))
            goto error;
    tmp_p++;
    newpoint = DXPt(x, y-epsilony, 0.0);
    if (!DXAddArrayData(aposnew, tmp_p, 1, (Pointer)&newpoint))
        goto error;
    if (!regcolors)
        if (!DXAddArrayData(acolorsnew, tmp_p, 1, (Pointer)&color))
            goto error;
    tmp_p++;
    newpoint = DXPt(x, y+epsilony, 0.0);
    if (!DXAddArrayData(aposnew, tmp_p, 1, (Pointer)&newpoint))
        goto error;
    if (!regcolors)
        if (!DXAddArrayData(acolorsnew, tmp_p, 1, (Pointer)&color))
            goto error;
    tmp_p++;
    newpoint = DXPt(x-epsilonx, y, 0.0);
    if (!DXAddArrayData(aposnew, tmp_p, 1, (Pointer)&newpoint))
        goto error;
    if (!regcolors)
        if (!DXAddArrayData(acolorsnew, tmp_p, 1, (Pointer)&color))
            goto error;
    tmp_p++;
    newpoint = DXPt(x+epsilonx, y, 0.0);
    if (!DXAddArrayData(aposnew, tmp_p, 1, (Pointer)&newpoint))
        goto error;
    if (!regcolors)
        if (!DXAddArrayData(acolorsnew, tmp_p, 1, (Pointer)&color))
            goto error;
    tmp_p++;
    newcon = DXLn(tmp_p-8, tmp_p-7);
    if (!(DXAddArrayData(aconnew, tmp_c, 1, (Pointer)&newcon)))
        goto error;
    tmp_c++;
    newcon = DXLn(tmp_p-6, tmp_p-5);
    if (!(DXAddArrayData(aconnew, tmp_c, 1, (Pointer)&newcon)))
        goto error;
    tmp_c++;
    newcon = DXLn(tmp_p-4, tmp_p-3);
    if (!(DXAddArrayData(aconnew, tmp_c, 1, (Pointer)&newcon)))
        goto error;
    tmp_c++;
    newcon = DXLn(tmp_p-2, tmp_p-1);
    if (!(DXAddArrayData(aconnew, tmp_c, 1, (Pointer)&newcon)))
        goto error;
    tmp_c++;

    *count_p = tmp_p;
    *count_conn = tmp_c;
    return OK;

error:
    return ERROR;
}


static
Error
AddX(Array aposnew, int *count_p, Array aconnew, int *count_conn,
     float x, float y, float epsilonx, float epsilony,
     int regcolors, RGBColor color, Array acolorsnew) {

    Point newpoint;
    Line  newcon;
    int tmp_p, tmp_c;

    tmp_p = *count_p;
    tmp_c = *count_conn;
    newpoint = DXPt(x-epsilonx, y-epsilony, 0.0);
    if (!DXAddArrayData(aposnew, tmp_p, 1, (Pointer)&newpoint))
        goto error;
    if (!regcolors)
        if (!DXAddArrayData(acolorsnew, tmp_p, 1, (Pointer)&color))
            goto error;
    tmp_p++;
    newpoint = DXPt(x+epsilonx, y+epsilony, 0.0);
    if (!DXAddArrayData(aposnew, tmp_p, 1, (Pointer)&newpoint))
        goto error;
    if (!regcolors)
        if (!DXAddArrayData(acolorsnew, tmp_p, 1, (Pointer)&color))
            goto error;
    tmp_p++;
    newpoint = DXPt(x+epsilonx, y-epsilony, 0.0);
    if (!DXAddArrayData(aposnew, tmp_p, 1, (Pointer)&newpoint))
        goto error;
    if (!regcolors)
        if (!DXAddArrayData(acolorsnew, tmp_p, 1, (Pointer)&color))
            goto error;
    tmp_p++;
    newpoint = DXPt(x-epsilonx, y+epsilony, 0.0);
    if (!DXAddArrayData(aposnew, tmp_p, 1, (Pointer)&newpoint))
        goto error;
    if (!regcolors)
        if (!DXAddArrayData(acolorsnew, tmp_p, 1, (Pointer)&color))
            goto error;
    tmp_p++;
    newcon = DXLn(tmp_p-4, tmp_p-3);
    if (!(DXAddArrayData(aconnew, tmp_c, 1, (Pointer)&newcon)))
        goto error;
    tmp_c++;
    newcon = DXLn(tmp_p-2, tmp_p-1);
    if (!(DXAddArrayData(aconnew, tmp_c, 1, (Pointer)&newcon)))
        goto error;
    tmp_c++;

    *count_p = tmp_p;
    *count_conn = tmp_c;
    return OK;

error:
    return ERROR;
}

static
Error
DoPlotField(Object ino, int needtoclip, Point clippoint1, Point clippoint2,
            float epsilonx, float epsilony, int xlog, int ylog,
            struct plotargstruct *plotarg) {
    struct arg arg;
    int i;
    Object oo;

    switch (DXGetObjectClass(ino)) {
        case CLASS_FIELD:
        /* set up the argument block */
        arg.ino = (Object)ino;
        arg.needtoclip = needtoclip;
        arg.clippoint1 = clippoint1;
        arg.clippoint2 = clippoint2;
        arg.epsilonx = epsilonx;
        arg.epsilony = epsilony;
        arg.xlog = xlog;
        arg.ylog = ylog;
        arg.mark = plotarg->ismark;
        arg.markevery = plotarg->markevery;
        arg.marker = plotarg->mark;
        arg.markscale = plotarg->markscale;
        arg.scatter = plotarg->scatter;
        arg.column = plotarg->column;
        arg.siblingCount = plotarg->subfields;
        arg.siblingNumber = plotarg->fieldNumber;
        if (!(DXAddTask(plottask,(Pointer)&arg,sizeof(arg),0.0))) {
            return ERROR;
        }
        break;

        case CLASS_GROUP:
        DXGetMemberCount((Group)ino, &i);
        plotarg->subfields = i;
        /* recursively traverse groups */
        for (i=0; (oo=DXGetEnumeratedMember((Group)ino, i, NULL)); i++) {
            plotarg->fieldNumber = i;
            if (!DoPlotField((Object)oo, needtoclip, clippoint1, clippoint2,
                             epsilonx, epsilony, xlog, ylog, plotarg))
                return ERROR;
        }
        break;
        default:
        break;
    }
    return OK;
}

static Error _dxfSetTextColor(Field f, RGBColor color) {
    Array pos, col;
    int numpos;

    pos = (Array)DXGetComponentValue(f,"positions");
    DXGetArrayInfo(pos,&numpos,NULL,NULL,NULL,NULL);
    col = (Array)DXNewConstantArray(numpos, (Pointer)&color, TYPE_FLOAT,
                                    CATEGORY_REAL, 1, 3);
    DXSetComponentValue(f,"colors",(Object)col);
    return OK;

}



static
Object
MakeLabel(char *label, RGBColor color, int num, float *maxstring,
          float linelength, Object font, int mark, int scatter,
          char *marker, RGBColor labelcolor) {
    Object labeltext, outo=NULL, outgroup, outline, geotext;
    Array linepositions=NULL, lineconnections=NULL, linecolors=NULL;
    float width, xpos, ypos;
    Point *dp;
    float epsilon;
    int *dc, numpoints, numconn, num_mark_pos=0, num_mark_con=0, count_pos;
    int linepoints, lineconns;
    int   count_con, regcolors;
    RGBColor dummy;
    Array acolors=NULL;


    dummy = DXRGB(0.0, 0.0, 0.0);

    if (mark) {
        if (!strcmp(marker,"x")) {
            num_mark_pos = 4;
            num_mark_con = 2;
        } else if (!strcmp(marker,"triangle")) {
            num_mark_pos = 3;
            num_mark_con = 3;
        } else if (!strcmp(marker,"square")) {
            num_mark_pos = 4;
            num_mark_con = 4;
        } else if (!strcmp(marker,"circle")) {
            num_mark_pos = 10;
            num_mark_con = 10;
        } else if (!strcmp(marker,"dot")) {
            num_mark_pos = 2;
            num_mark_con = 1;
        } else if (!strcmp(marker,"star")) {
            num_mark_pos = 8;
            num_mark_con = 4;
        } else if (!strcmp(marker,"diamond")) {
            num_mark_pos = 4;
            num_mark_con = 4;
        } else if (!strcmp(marker,"")) {}
        else {
            DXSetError(ERROR_DATA_INVALID,"#10203", "mark",
                       "x, triangle, square, circle, star, diamond");
            goto error;
        }
    }

    /* here we need to put the legend in */
    /* it will be in pixel units, and will become a screen object at the */
    /* top level */

    geotext = DXGeometricText(label, font, &width);
    _dxfSetTextColor((Field)geotext,labelcolor);
    if (!(labeltext = (Object)DXNewXform(geotext, DXScale(15, 15, 1)))) {
        return NULL;
    }

    if (width*15 > *maxstring)
        *maxstring=width*15;

    xpos = linelength;
    epsilon = linelength/8;
    ypos = -(num-1)*20;
    outo =
        (Object)DXNewXform((Object)labeltext, DXTranslate(DXPt(xpos,ypos-3,0)));

    if (scatter) {
        linepoints = 0;
        lineconns = 0;
    } else {
        linepoints = 2;
        lineconns = 1;
    }

    if (mark) {
        numpoints = linepoints + num_mark_pos;
        numconn = lineconns + num_mark_con;
    } else {
        numpoints = linepoints;
        numconn = lineconns;
    }

    /* XXX check out legends for data dep connections */

    /* always reg colors for the legend */
    regcolors = 1;

    linepositions = DXNewArray(TYPE_FLOAT,CATEGORY_REAL, 1, 3);
    if (!DXAddArrayData(linepositions,0,numpoints,NULL))
        goto error;
    if (!(dp = (Point *)DXGetArrayData(linepositions)))
        goto error;

    /* draw the line */
    if (!scatter) {
        dp[0] = DXPt(0, ypos, 0.0);
        dp[1] = DXPt(linelength*0.9,ypos,0.0);
    }

    lineconnections = DXNewArray(TYPE_INT,CATEGORY_REAL, 1, 2);
    if (!DXAddArrayData(lineconnections,0,numconn,NULL))
        goto error;
    dc = (int *)DXGetArrayData(lineconnections);

    if (!scatter) {
        dc[0]=0;
        dc[1]=1;
    }

    if (mark) {
        count_pos=linepoints;
        count_con=lineconns;
        if (!strcmp(marker,"x")) {
            AddX(linepositions, &count_pos, lineconnections, &count_con,
                 linelength*.45, ypos, epsilon, epsilon, regcolors, dummy,
                 acolors);
        } else if (!strcmp(marker,"triangle")) {
            AddTri(linepositions, &count_pos, lineconnections, &count_con,
                   linelength*.45, ypos, epsilon, epsilon, regcolors, dummy,
                   acolors);
        } else if (!strcmp(marker,"square")) {
            AddSquare(linepositions, &count_pos, lineconnections, &count_con,
                      linelength*.45, ypos, epsilon, epsilon,regcolors, dummy,
                      acolors);
        } else if (!strcmp(marker,"circle")) {
            AddCircle(linepositions, &count_pos, lineconnections, &count_con,
                      linelength*.45, ypos, epsilon, epsilon,regcolors, dummy,
                      acolors);
        } else if (!strcmp(marker,"star")) {
            AddStar(linepositions, &count_pos, lineconnections, &count_con,
                    linelength*.45, ypos, epsilon, epsilon,regcolors, dummy,
                    acolors);
        } else if (!strcmp(marker,"diamond")) {
            AddDiamond(linepositions, &count_pos, lineconnections, &count_con,
                       linelength*.45, ypos, epsilon, epsilon,regcolors, dummy,
                       acolors);
        }
    }
    linecolors = (Array)DXNewConstantArray(numpoints, (Pointer)&color,
                                           TYPE_FLOAT, CATEGORY_REAL, 1, 3);
    outline = (Object)DXNewField();
    DXSetComponentValue((Field)outline,"positions",(Object)linepositions);
    linepositions = NULL;
    DXSetComponentValue((Field)outline,"connections",(Object)lineconnections);
    lineconnections = NULL;
    DXSetComponentValue((Field)outline,"colors",(Object)linecolors);
    linecolors = NULL;
    DXSetComponentAttribute((Field)outline,"connections","element type",
                            (Object)DXNewString("lines"));
    DXSetComponentAttribute((Field)outline,"connections","ref",
                            (Object)DXNewString("positions"));
    DXEndField((Field)outline);

    outgroup = (Object)DXNewGroup();
    DXSetMember((Group)outgroup,NULL,outline);
    outline = NULL;
    DXSetMember((Group)outgroup,NULL,outo);
    outo = NULL;

    return outgroup;

error:
    DXDelete((Object)linepositions);
    DXDelete((Object)lineconnections);
    DXDelete((Object)linecolors);
    return NULL;
}


/*
 
static 
  Error 
  GetIntersection(Point clippoint1, Point clippoint2, float thispos, 
                  float thisdata, float lastpos, float lastdata)
{
  Point p1, p2, p3, p4;
 
  if (thispos[i]>=clippoint1.x && thispos[i]<=clippoint2.x &&
          thisdata[i]>=clippoint1.y && thisdata[i]<=clippoint2.y) {
    if 
  }
  else {
  }
*/

static
Error
GetLineColor(Object ino, RGBColor *color) {
    Array colorcomp;
    Field oo;
    RGBColor *p_color;

    switch (DXGetObjectClass(ino)) {
        case CLASS_FIELD:
        colorcomp = (Array)DXGetComponentValue((Field)ino,"colors");
        if (!colorcomp)
            *color = DXRGB(1.0, 1.0, 1.0);
        else {
            if (!DXQueryConstantArray(colorcomp,NULL,NULL)) {
                /* let's use the first color in the array */
                p_color = (RGBColor *)DXGetArrayData(colorcomp);
                *color = p_color[0];
            } else {
                DXQueryConstantArray((Array)colorcomp, NULL, (Pointer)color);
            }
        }
        break;
        case CLASS_GROUP:
        /* XXX what's going on here? */
        /* get the first color */
        oo = DXGetPart(ino, 0);
        colorcomp = (Array)DXGetComponentValue((Field)oo,"colors");
        if (!colorcomp)
            *color = DXRGB(1.0, 1.0, 1.0);
        else {
            if (!DXQueryConstantArray(colorcomp, NULL,NULL)) {
                /* let's use the first color in the array */
                p_color = (RGBColor *)DXGetArrayData(colorcomp);
                *color = p_color[0];
            } else {
                DXQueryConstantArray((Array)colorcomp, NULL, (Pointer)color);
            }
        }
        break;
        case CLASS_XFORM:
        /* get the first color */
        oo = DXGetPart(ino, 0);
        colorcomp = (Array)DXGetComponentValue((Field)oo,"colors");
        if (!colorcomp)
            *color = DXRGB(1.0, 1.0, 1.0);
        else {
            if (!DXQueryConstantArray(colorcomp,NULL,NULL)) {
                /* let's use the first color in the array */
                p_color = (RGBColor *)DXGetArrayData(colorcomp);
                *color = p_color[0];
            } else {
                DXQueryConstantArray((Array)colorcomp, NULL, (Pointer)color);
            }
        }
        break;
        default:
        break;
    }
    return OK;
}


Error _dxfGetAnnotationColors(Object colors, Object which,
                              RGBColor defaultticcolor,
                              RGBColor defaultlabelcolor,
                              RGBColor defaultaxescolor,
                              RGBColor defaultgridcolor,
                              RGBColor defaultbackgroundcolor,
                              RGBColor *ticcolor, RGBColor *labelcolor,
                              RGBColor *axescolor, RGBColor *gridcolor,
                              RGBColor *backgroundcolor,
                              int *background) {
    RGBColor *colorlist =NULL;
    int i, numcolors, numcolorobjects;
    char *colorstring, *newcolorstring;


    /* in[9] is the colors list */
    /* first set up the default colors */
    *ticcolor = defaultticcolor;
    *labelcolor = defaultlabelcolor;
    *axescolor = defaultaxescolor;
    *gridcolor = defaultgridcolor;
    /* by default, we don't make a background */
    *background = 0;
    *backgroundcolor = defaultbackgroundcolor;
    if (colors) {
        if (DXExtractNthString(colors,0,&colorstring)) {
            /* it's a list of strings */
            /* first figure out how many there are */
            if (!_dxfHowManyStrings(colors, &numcolors))
                goto error;
            /* allocate space for the 3-vectors */
            colorlist = DXAllocateLocal(numcolors*sizeof(RGBColor));
            if (!colorlist)
                goto error;
            for (i=0; i<numcolors;i++) {
                DXExtractNthString(colors,i,&colorstring);
                if (!DXColorNameToRGB(colorstring, &colorlist[i])) {
                    _dxfLowerCase(colorstring, &newcolorstring);
                    if (!strcmp(newcolorstring,"clear")) {
                        DXResetError();
                        colorlist[i]=DXRGB(-1.0, -1.0, -1.0);
                    } else {
                        goto error;
                    }
                    DXFree((Pointer)newcolorstring);
                }
            }
        } else {
            if (!DXQueryParameter(colors, TYPE_FLOAT, 3, &numcolors)) {
                DXSetError(ERROR_BAD_PARAMETER, "#10052", "colors");
                goto error;
            }
            /* it's a list of 3-vectors */
            colorlist = DXAllocateLocal(numcolors*sizeof(RGBColor));
            if (!colorlist)
                goto error;
            if (!DXExtractParameter(colors, TYPE_FLOAT, 3, numcolors,
                                    (Pointer)colorlist))
                goto error;
        }
    }
    if (!which) {
        if (colors) {
            /* all objects get whatever color was there */
            if (numcolors != 1) {
                DXSetError(ERROR_BAD_PARAMETER, "#11839");
                goto error;
            }
            if (colorlist[0].r != -1 || colorlist[0].g != -1 ||
                    colorlist[0].b != -1) {
                *ticcolor = colorlist[0];
                *labelcolor = colorlist[0];
                *axescolor = colorlist[0];
                *gridcolor = colorlist[0];
            } else {
                DXSetError(ERROR_BAD_PARAMETER, "#11811");
                goto error;
            }
        }
    } else {
        /* a list of strings was specified */
        /* figure out how many */
        if (!_dxfHowManyStrings(which, &numcolorobjects)) {
            DXSetError(ERROR_BAD_PARAMETER,"#10205", "annotation");
            goto error;
        }
        if (numcolors==1) {
            for (i=0; i< numcolorobjects; i++) {
                DXExtractNthString(which, i, &colorstring);
                _dxfLowerCase(colorstring, &newcolorstring);
                if ((colorlist[0].r==-1 && colorlist[0].g==-1 &&
                        colorlist[0].b==-1) && strcmp(newcolorstring,"background")) {
                    DXSetError(ERROR_BAD_PARAMETER, "#11811");
                    goto error;
                }
                if (!strcmp(newcolorstring, "ticks"))
                    *ticcolor = colorlist[0];
                else if (!strcmp(newcolorstring, "labels"))
                    *labelcolor = colorlist[0];
                else if (!strcmp(newcolorstring,"axes"))
                    *axescolor=colorlist[0];
                else if (!strcmp(newcolorstring,"grid"))
                    *gridcolor=colorlist[0];
                else if (!strcmp(newcolorstring,"background")) {
                    if (colorlist[0].r==-1 &&
                            colorlist[0].g==-1 &&
                            colorlist[0].b==-1)
                        *background = 0;
                    else {
                        *backgroundcolor = colorlist[0];
                        *background=1;
                    }
                } else if (!strcmp(newcolorstring,"all")) {
                    *ticcolor = colorlist[0];
                    *labelcolor = colorlist[0];
                    *axescolor=colorlist[0];
                    *gridcolor=colorlist[0];
                } else {
                    DXSetError(ERROR_BAD_PARAMETER, "#10203", "annotation objects",
                               "ticks, labels, axes, grid, background");
                    goto error;
                }
                DXFree((Pointer)newcolorstring);
            }
        } else {
            if (numcolors != numcolorobjects) {
                DXSetError(ERROR_BAD_PARAMETER,"#11813");
                goto error;
            }
            for (i=0; i< numcolorobjects; i++) {
                DXExtractNthString(which, i, &colorstring);
                _dxfLowerCase(colorstring, &newcolorstring);
                if ((colorlist[i].r==-1 && colorlist[i].g==-1 &&
                        colorlist[i].b==-1) && strcmp(newcolorstring,"background")) {
                    DXSetError(ERROR_BAD_PARAMETER, "#11811");
                    goto error;
                }
                if (!strcmp(newcolorstring, "ticks"))
                    *ticcolor = colorlist[i];
                else if (!strcmp(newcolorstring, "labels"))
                    *labelcolor = colorlist[i];
                else if (!strcmp(newcolorstring,"axes"))
                    *axescolor=colorlist[i];
                else if (!strcmp(newcolorstring,"grid"))
                    *gridcolor=colorlist[i];
                else if (!strcmp(newcolorstring,"background")) {
                    if (colorlist[0].r ==-1 &&
                            colorlist[0].g ==-1 &&
                            colorlist[0].b ==-1) {
                        *background=0;
                    } else {
                        *background = 1;
                        *backgroundcolor = colorlist[i];
                    }
                } else {
                    DXSetError(ERROR_BAD_PARAMETER, "#10203", "annotation",
                               "ticks, labels, axes, grid, background");
                    goto error;
                }
                DXFree((Pointer)newcolorstring);
            }
        }
    }
    DXFree((Pointer)colorlist);
    return OK;
error:
    DXFree((Pointer)colorlist);
    return ERROR;
}

Error _dxfHowManyStrings(Object stringlist, int *howmany) {
    int i;
    char *string;

    i=0;
    *howmany = 0;
    if (!DXExtractNthString(stringlist,0,&string)) {
        return ERROR;
    }

    while (DXExtractNthString(stringlist, i, &string)) {
        i++;
    }
    *howmany = i;

    return OK;
}

static Error GetPlotHeight(Object obj, int ylog, float *ywidth,
                           float *plotmax, float *plotmin) {
    float newmin = 1;
    int i;
    Field part;

    /* need to recurse on obj to each field. Treat dep connections and
       dep positions differently */

    /* these are the starting points. It may be larger if there are
       dep connections plots here */
    if (!ylog)
        *ywidth = *plotmax-*plotmin;
    else
        *ywidth = FCEIL(log10(*plotmax))-FFLOOR(log10(*plotmin));


    for (i=0; (part = DXGetPart(obj, i)); i++) {
        if (!strcmp("connections",DXGetString((String)DXGetComponentAttribute((Field)part,"data","dep"))))
            newmin = 0;
    }

    if (newmin == 0) {
        if (!ylog) {
            *ywidth = *plotmax;
            *plotmin=0;
        } else {
            *ywidth = FCEIL(log10(*plotmax));
            *plotmin=0;
        }
    }

    return OK;
}
