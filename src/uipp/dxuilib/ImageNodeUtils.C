/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"



#ifdef OS2
#include <stdlib.h>
#include <types.h>
#endif

#if defined(DXD_WIN) || defined(OS2)          //SMH get socket calls

#if 0
#if defined(windows) && defined(HAVE_WINSOCK_H)
#include <winsock.h>
#elif defined(HAVE_CYGWIN_SOCKET_H)
#include <cygwin/socket.h>
#elif defined(HAVE_SYS_SOCKET_H)
#include <sys/socket.h>
#endif
#endif

#undef send
#undef FLOAT
#undef PFLOAT
#endif

#include "ImageNode.h"
#include "StandIn.h"
#include "Ark.h"
#include "Parameter.h"
#include "DXApplication.h"
#include "Network.h"
#include "PickNode.h"
#include "ImageWindow.h"
#include "ListIterator.h"
#include "ImageDefinition.h"
#include "WarningDialogManager.h"
#include "DXPacketIF.h"

#include "DXVersion.h"

#if WORKSPACE_PAGES
#include "ProcessGroupManager.h"
#endif

#ifdef ABS_IN_MATH_H
# define abs __Dont_define_abs
#endif
#include <math.h>
#ifdef ABS_IN_MATH_H
# undef abs
#endif

String ImageNode::ImageMacroTxt[] = {
#   include "imagemac.h"
};
String ImageNode::DXMacroTxt[] = {
#   include "dxmac.h"
};
String ImageNode::VrmlMacroTxt[] = {
#   include "vrmlmac.h"
};
String ImageNode::GifMacroTxt[] = {
#   include "gifmac.h"
};

#include <ctype.h>

#define IMAGETAG		1
#define OBJECT                  2
#define WHERE                   3
#define USEVECTOR               4
#define TO                      5
#define FROM                    6
#define WIDTH                   7
#define RESOLUTION              8
#define ASPECT                  9
#define UP                      10
#define VIEWANGLE               11
#define PROJECTION              12
#define OPTIONS                 13
#define BUTTON_STATE            14
#define BUTTON_UP_APPROX        15
#define BUTTON_DOWN_APPROX      16
#define BUTTON_UP_DENSITY       17
#define BUTTON_DOWN_DENSITY     18
#define RENDER_MODE             19
#define DEFAULT_CAMERA          20
#define RESET_CAMERA            21
#define BACKGROUND              22
#define THROTTLE                23
#define RECENABLE               24
#define RECFILE                 25
#define RECFORMAT               26
#define RECRESOLUTION           27
#define RECASPECT               28
#define AAENABLE                29
#define AALABELS                30
#define AATICKS                 31
#define AACORNERS               32
#define AAFRAME                 33
#define AAADJUST                34
#define AACURSOR                35
#define AAGRID                  36
#define AACOLORS                37
#define AAANNOTATION            38
#define AALABELSCALE            39
#define AAFONT                  40
#define INTERACTIONMODE         41
#define IMAGENAME               42

//
// new AutoAxes parameters
//
#define AA_XTICK_LOCS           43
#define AA_YTICK_LOCS           44
#define AA_ZTICK_LOCS           45
#define AA_XTICK_LABELS         46
#define AA_YTICK_LABELS         47
#define AA_ZTICK_LABELS         48

//
// On behalf of Java Explorer
//
#define JAVA_OPTIONS		49

//
// These are input param numbers for the WebOptions macro.  They belong
// in WebOptionsNode.C but there isn't any way to make a subclass of Node
// just for a particular macro unless it's done in the style of ImageNode
// which seems like too much work in this case.  Since WebOptions is just
// a plain old macro created with the vpe, it's actually a MacroNode when
// the ui creates one.
//
#define WEB_FORMAT		1
#define WEB_FILE		2
#define WEB_ENABLED		3
#define WEB_COUNTERS		4
#define WEB_ORBIT		5
#define WEB_ID			6
#define WEB_IMGID		7
#define WED_ORBIT_DELTA		8


void ImageNode::setRecordEnable(int use, boolean send)
{
    char s[32];
    sprintf(s, "%d", use);
    this->setInputValue(RECENABLE, s, DXType::FlagType, send);
}

void ImageNode::setRecordResolution(int x, boolean send)
{
    char s[100];
    sprintf(s, "%d", x);
    this->setInputValue(RECRESOLUTION, s, DXType::IntegerType, send);
}

void ImageNode::setRecordAspect(double aspect, boolean send)
{
    char s[100];
    sprintf(s, "%f", aspect);
    this->setInputValue(RECASPECT, s, DXType::ScalarType, send);
}

void ImageNode::getRecordEnable(int &x)
{
    const char *s;

    if (this->isInputDefaulting(RECENABLE))
      s = this->getInputDefaultValueString(RECENABLE);
    else
      s = this->getInputValueString(RECENABLE);
    
    if (s)
      sscanf(s, "%d", &x);
}
    
void ImageNode::getRecordResolution(int &x, int &y)
{
    const char *s;
    if (this->isInputDefaulting(RECRESOLUTION))
      s = this->getInputDefaultValueString(RECRESOLUTION);
    else
      s = this->getInputValueString(RECRESOLUTION);

    if(!s || 
	EqualString(s,"NULL") ||
	EqualString(s,this->getInputDefaultValueString(RECRESOLUTION)) 
      )
      this->getResolution(x, y);
    else
    {
      double aspect, dy;
      int tmpy;
      sscanf(s, "%d", &x);
      this->getRecordAspect(aspect);
      dy = x * aspect;
      tmpy = (int)dy;
      if ((dy - tmpy) >= 0.5) tmpy++;
      y = tmpy;
    }
}

void ImageNode::getRecordAspect(double &a)
{
    const char *s;
    if (this->isInputDefaulting(RECASPECT))
      s = this->getInputDefaultValueString(RECASPECT);
    else
      s = this->getInputValueString(RECASPECT);

    if(!s || 
	EqualString(s,"NULL") ||
	EqualString(s,this->getInputDefaultValueString(RECASPECT))
       )
      this->getAspect(a);
    else
      sscanf(s, "%lg", &a);
}


void ImageNode::setRecordFile(const char *file, boolean send)
{
    if (EqualString(file, this->getInputDefaultValueString(RECFILE)))
	this->useDefaultInputValue(RECFILE, send);
    else
	this->setInputValue(RECFILE, file, DXType::StringType, send);
}

void ImageNode::setRecordFormat(const char *format, boolean send)
{
    if (EqualString(format, this->getInputDefaultValueString(RECFORMAT)))
	this->useDefaultInputValue(RECFORMAT, send);
    else
	this->setInputValue(RECFORMAT, format, DXType::StringType, send);
}

void ImageNode::getRecordFile(const char *&file)
{
    if (this->isInputDefaulting(RECFILE))
	file = this->getInputDefaultValueString(RECFILE);
    else
	file = this->getInputValueString(RECFILE);
}
void ImageNode::getRecordFormat(const char *&format)
{
    if (this->isInputDefaulting(RECFORMAT))
	format = this->getInputDefaultValueString(RECFORMAT);
    else
	format = this->getInputValueString(RECFORMAT);
}

void ImageNode::getResolution(int &x, int &y)
{
    const char *s;
    if (this->isInputDefaulting(RESOLUTION))
	s = this->getInputDefaultValueString(RESOLUTION);
    else
	s = this->getInputValueString(RESOLUTION);

    if(!s || EqualString(s,"NULL"))
	x = 640;
    else
	sscanf(s, "%d", &x);

    if (this->height > 0)
	y = this->height;
    else
    {
	double aspect, dy;
	int tmpy;
	this->getAspect(aspect);
	dy = x * aspect;
	tmpy = (int)dy;
	if ((dy - tmpy) >= 0.5) tmpy++;
	y = tmpy;
    }
}
void ImageNode::getWidth(double &w)
{
    const char *s;
    if (this->isInputDefaulting(WIDTH))
	s = this->getInputDefaultValueString(WIDTH);
    else
	s = this->getInputSetValueString(WIDTH);

    if(!s || EqualString(s,"NULL"))
	w = .53589838;		// 2*tan(15 degrees)
    else
	sscanf(s, "%lg", &w);
}
void ImageNode::getAspect(double &a)
{
    const char *s;
    if (this->isInputDefaulting(ASPECT))
	s = this->getInputDefaultValueString(ASPECT);
    else
	s = this->getInputValueString(ASPECT);

    if(!s || EqualString(s,"NULL"))
	a = 0.75;
    else
	sscanf(s, "%lg", &a);
}

void ImageNode::getBackgroundColor(const char *&color)
{
    color = this->getInputValueString(BACKGROUND);
}



//
// F L A G S   F L A G S   F L A G S   F L A G S   F L A G S   F L A G S   
// F L A G S   F L A G S   F L A G S   F L A G S   F L A G S   F L A G S   
//
boolean ImageNode::isSetAutoAxesFrame ()
{ return this->isSetAutoAxesFlag (AAFRAME); }

boolean ImageNode::isSetAutoAxesGrid ()
{ return this->isSetAutoAxesFlag (AAGRID); }

boolean ImageNode::isSetAutoAxesAdjust ()
{ return this->isSetAutoAxesFlag (AAADJUST); }

void ImageNode::setAutoAxesFrame (boolean state, boolean send)
{ this->setAutoAxesFlag (AAFRAME, state, send); }

void ImageNode::setAutoAxesGrid (boolean state, boolean send)
{ this->setAutoAxesFlag (AAGRID, state, send); }

void ImageNode::setAutoAxesAdjust (boolean state, boolean send)
{ this->setAutoAxesFlag (AAADJUST, state, send); }

boolean ImageNode::isSetAutoAxesFlag(int index)
{
    const char* v = this->getInputValueString(index);
    if (!v) return FALSE;
    if (EqualString(v,"NULL")) return FALSE;

    int val = 0;
    if (!sscanf(v,"%d",&val)) return FALSE;
    if (!val) return FALSE;
    return TRUE;
}

void ImageNode::setAutoAxesFlag (int index, boolean state, boolean send)
{
    if (state) this->setInputValue(index, "1", DXType::FlagType, send);
    else this->setInputValue(index, "0", DXType::FlagType, send);
}

//
// C U R S O R   C U R S O R   C U R S O R   C U R S O R   C U R S O R   
// C U R S O R   C U R S O R   C U R S O R   C U R S O R   C U R S O R   
//
boolean ImageNode::isAutoAxesCursorSet()
{
   return !EqualString(this->getInputValueString(AACURSOR),"NULL");
}

void ImageNode::unsetAutoAxesLabels(boolean send)
{
    this->useDefaultInputValue (AALABELS, send);
}

void ImageNode::unsetAutoAxesLabelScale(boolean send)
{
    this->useDefaultInputValue (AALABELSCALE, send);
}

void ImageNode::unsetAutoAxesFont(boolean send)
{
    this->useDefaultInputValue (AAFONT, send);
}

void ImageNode::unsetAutoAxesCursor(boolean send)
{
//    this->setInputValue(AACURSOR,"NULL", DXType::VectorType, send);
    this->useDefaultInputValue (AACURSOR, send);
}

void ImageNode::unsetAutoAxesEnable(boolean send)
{
    this->useDefaultInputValue (AAENABLE, send);
}

void ImageNode::unsetAutoAxesFrame(boolean send)
{
    this->useDefaultInputValue (AAFRAME, send);
}

void ImageNode::unsetAutoAxesGrid(boolean send)
{
    this->useDefaultInputValue (AAGRID, send);
}

void ImageNode::unsetAutoAxesAdjust(boolean send)
{
    this->useDefaultInputValue (AAADJUST, send);
}

void ImageNode::setAutoAxesCursor (double x, double y, double z, boolean send)
{
    char buf[512];
    sprintf (buf, "[ %f %f %f ]", x,y,z);
    this->setInputValue (AACURSOR, buf, DXType::VectorType, send);
}

void ImageNode::getAutoAxesCursor (double *x, double *y, double *z)
{
    *x = *y = *z = 0.0;

    if (!this->isAutoAxesCursorSet())  return;

    Parameter *p = this->getInputParameter(AACURSOR);
    ASSERT(p);
    int count = p->getComponentCount();

    //
    // USER Error?
    //
    if (count<=0) return ;

    int i;
    double dbl;
    for (i=1; i<=count; i++) {
	dbl = p->getVectorComponentValue(i);
        switch (i) {
	    case 1: *x = dbl; break;
	    case 2: *y = dbl; break;
	    case 3: *z = dbl; break;
	}
    }
}

//
// C O R N E R S  C O R N E R S  C O R N E R S  C O R N E R S  C O R N E R S
// C O R N E R S  C O R N E R S  C O R N E R S  C O R N E R S  C O R N E R S
//
void ImageNode::disableAutoAxesCorners(boolean send)
{
   this->useDefaultInputValue(AACORNERS,send);
}

boolean ImageNode::isAutoAxesCornersSet()
{
   return !EqualString(this->getInputValueString(AACORNERS),"NULL");
}

void ImageNode::unsetAutoAxesCorners(boolean send)
{
//    this->setInputValue(AACORNERS,"NULL", DXType::VectorListType, send);
    this->useDefaultInputValue (AACORNERS, send);
}

void ImageNode::setAutoAxesCorners(double dval[], boolean send)
{
    char buf[1024];   
    sprintf (buf, "{[ %f %f %f ] [ %f %f %f ]}", 
	dval[0], dval[1], dval[2], dval[3], dval[4], dval[5]);
    this->setInputValue(AACORNERS,buf, DXType::VectorListType, send);
}

void ImageNode::getAutoAxesCorners(double dval[])
{
    dval[0] = dval[1] = dval[2] = dval[3] = dval[4] = dval[5] = 0.0;

    if (!this->isAutoAxesCornersSet()) return;

    double *values;
    int vdim;
    const char *v = this->getInputValueString(AACORNERS);
    int nvec = DXValue::GetDoublesFromList(v, DXType::VectorListType, &values, &vdim); 

    if (vdim==0) return ;
    if (!values) return ;
    
    //
    // USER Error?
    //
    // If someone types a value into the cdb, what are the chances it will really
    // be 3x2? We'll use 0.0 as a fallback value.  The following line used to 
    // be 2 ASSERTs
    //
    if ((vdim > 3) || (nvec!= 2)) return ;

    switch (vdim) {
	case 3:
	   dval[0] = values[0];
	   dval[1] = values[1];
	   dval[2] = values[2];
	   dval[3] = values[3];
	   dval[4] = values[4];
	   dval[5] = values[5];
	   break;
	case 2:
	   dval[0] = values[0];
	   dval[1] = values[1];
	   dval[3] = values[2];
	   dval[4] = values[3];
	   break;
	case 1:
	   dval[0] = values[0];
	   dval[3] = values[1];
	   break;
    }
}



//
// S T R I N G L I S T   S T R I N G L I S T   S T R I N G L I S T   
// S T R I N G L I S T   S T R I N G L I S T   S T R I N G L I S T   
//
boolean ImageNode::isAutoAxesLabelsSet ()
{ return this->isAutoAxesStringListSet (AALABELS); }

int ImageNode::getAutoAxesLabels (char *sval[])
{ return this->getAutoAxesStringList (AALABELS, sval); }

boolean ImageNode::isAutoAxesAnnotationSet()
{ return this->isAutoAxesStringListSet (AAANNOTATION); }

int ImageNode::getAutoAxesAnnotation (char *sval[])
{ return this->getAutoAxesStringList (AAANNOTATION, sval); }

boolean ImageNode::isAutoAxesColorsSet ()
{ return this->isAutoAxesStringListSet (AACOLORS); }

boolean ImageNode::isAutoAxesXTickLabelsSet ()
{ return this->isAutoAxesStringListSet (AA_XTICK_LABELS); }

boolean ImageNode::isAutoAxesYTickLabelsSet ()
{ return this->isAutoAxesStringListSet (AA_YTICK_LABELS); }

boolean ImageNode::isAutoAxesZTickLabelsSet ()
{ return this->isAutoAxesStringListSet (AA_ZTICK_LABELS); }

int ImageNode::getAutoAxesColors (char *colors[])
{ return this->getAutoAxesStringList (AACOLORS, colors); }

int ImageNode::getAutoAxesXTickLabels (char *labels[])
{ return this->getAutoAxesStringList (AA_XTICK_LABELS, labels); }

int ImageNode::getAutoAxesYTickLabels (char *labels[])
{ return this->getAutoAxesStringList (AA_YTICK_LABELS, labels); }

int ImageNode::getAutoAxesZTickLabels (char *labels[])
{ return this->getAutoAxesStringList (AA_ZTICK_LABELS, labels); }

void ImageNode::setAutoAxesLabels (char *value, boolean send)
{ this->setAutoAxesStringList (AALABELS, value, send); }

void ImageNode::setAutoAxesAnnotation (char *value, boolean send)
{ this->setAutoAxesStringList (AAANNOTATION, value, send); }

void ImageNode::setAutoAxesColors (char *colors, boolean send)
{ this->setAutoAxesStringList (AACOLORS, colors, send); }

void ImageNode::setAutoAxesXTickLabels (char *labels, boolean send)
{ this->setAutoAxesStringList (AA_XTICK_LABELS, labels, send); }

void ImageNode::setAutoAxesYTickLabels (char *labels, boolean send)
{ this->setAutoAxesStringList (AA_YTICK_LABELS, labels, send); }

void ImageNode::setAutoAxesZTickLabels (char *labels, boolean send)
{ this->setAutoAxesStringList (AA_ZTICK_LABELS, labels, send); }

void ImageNode::unsetAutoAxesAnnotation (boolean send)
{ this->unsetAutoAxesStringList (AAANNOTATION, send); }

void ImageNode::unsetAutoAxesColors (boolean send)
{ this->unsetAutoAxesStringList (AACOLORS, send); }

void ImageNode::unsetAutoAxesXTickLabels (boolean send)
{ this->unsetAutoAxesStringList (AA_XTICK_LABELS, send); }

void ImageNode::unsetAutoAxesYTickLabels (boolean send)
{ this->unsetAutoAxesStringList (AA_YTICK_LABELS, send); }

void ImageNode::unsetAutoAxesZTickLabels (boolean send)
{ this->unsetAutoAxesStringList (AA_ZTICK_LABELS, send); }

boolean ImageNode::isAutoAxesStringListSet(int index)
{
    const char *v = this->getInputValueString(index);
    return !EqualString(v,"NULL");
}

int ImageNode::getAutoAxesStringList (int index, char *sval[])
{
    //if (!this->isAutoAxesStringListSet(index)) return 0;
    const char *v;
    if (this->isAutoAxesStringListSet(index)) 
	v = this->getInputValueString(index);
    else
	v = this->getInputSetValueString(index);

    if ((!v) || (!v[0]) || (EqualString(v, "NULL"))) return 0;

    Parameter *p = this->getInputParameter(index);
    ASSERT(p);

    int count = 0;
    if (!p->hasValue())
	count = 0;
    else if (p->getValueType() & DXType::ListType)
	count = DXValue::GetListItemCount (v, DXType::StringListType);
    else if (p->getValueType() & DXType::StringType) {
	count = ((v = DXValue::CoerceValue (v, DXType::StringListType))? 1: 0);
    }
    if (!count) return 0;
    if (!sval) return count;

    int i,ndx;
    char *cp;
    ASSERT(sval);
    for (i=0,ndx=1; i<count; i++) {
	// This ASSERT doesn't need to be here.  NextListItem will
	// get memory for the string if the caller passes a null pointer.
	// ASSERT (sval[i]);
	cp = DXValue::NextListItem(v, &ndx, DXType::StringListType, sval[i]);
	if (!sval[i]) sval[i] = cp;
    }

    return count;
}


void ImageNode::setAutoAxesStringList (int index, char *value, boolean send)
{
    this->setInputValue(index, value, DXType::StringListType, send);
}

void ImageNode::unsetAutoAxesStringList(int index, boolean send)
{
//    this->setInputValue(index,"NULL", DXType::StringListType, send);
    this->useDefaultInputValue (index, send);
}

//
// T I C K S   T I C K S   T I C K S   T I C K S   T I C K S   T I C K S   
// T I C K S   T I C K S   T I C K S   T I C K S   T I C K S   T I C K S   
//
boolean ImageNode::isAutoAxesTicksSet()
{
   return !EqualString(this->getInputValueString(AATICKS),"NULL");
}

boolean ImageNode::isAutoAxesXTickLocsSet()
{
   return !EqualString(this->getInputValueString(AA_XTICK_LOCS),"NULL");
}

boolean ImageNode::isAutoAxesYTickLocsSet()
{
   return !EqualString(this->getInputValueString(AA_YTICK_LOCS),"NULL");
}

boolean ImageNode::isAutoAxesZTickLocsSet()
{
   return !EqualString(this->getInputValueString(AA_ZTICK_LOCS),"NULL");
}

int ImageNode::getAutoAxesTicksCount()
{
const char *v = this->getInputValueString(AATICKS);

    if (!this->isAutoAxesTicksSet()) return 0;

    Parameter *p = this->getInputParameter(AATICKS);
    ASSERT(p);
    if (p->getValueType() & DXType::ListType) {
	return DXValue::GetListItemCount (v, p->getValueType());
    } else if (p->getValueType() == DXType::IntegerType)
	return 1;
    else if (p->getValueType() == DXType::ScalarType)
	return 1;

    return 0;
}

void ImageNode::getAutoAxesTicks (int *t1, int *t2, int *t3)
{
    if (!this->isAutoAxesTicksSet()) return ;
    //
    // USER Error?
    //
    int cnt = this->getAutoAxesTicksCount();
    const char *v = this->getInputValueString(AATICKS);

    ASSERT (t1);
    ASSERT (t2);
    ASSERT (t3);

    Parameter *p = this->getInputParameter(AATICKS);
    ASSERT(p);

    if (p->getValueType() == DXType::IntegerListType) {
        char *cp;

	if (cnt >= 1) {
	    cp = DXValue::GetListItem (v, 1, DXType::IntegerListType); 
	    *t1 = atoi(cp);
	    delete cp;
	}
	if (cnt >= 2) {
	    cp = DXValue::GetListItem (v, 2, DXType::IntegerListType); 
	    *t2 = atoi(cp);
	    delete cp;
	}
	if (cnt >= 3) {
	    cp = DXValue::GetListItem (v, 3, DXType::IntegerListType); 
	    *t3 = atoi(cp);
	    delete cp;
	}
    }
}

void ImageNode::getAutoAxesTicks (int *t)
{
    if (!this->isAutoAxesTicksSet()) return ;

    ASSERT(t);
    Parameter *p = this->getInputParameter(AATICKS);
    ASSERT(p);

    if (DXType::IntegerType == p->getValueType()) {
	*t = (int)p->getIntegerValue();
    } else if (DXType::ScalarType == p->getValueType()) {
	*t = (int)p->getScalarValue();
    } else if (DXType::IntegerListType == p->getValueType()) {
	int t2, t3;
	this->getAutoAxesTicks(t, &t2, &t3);
    } else {
	//
	// USER Error?
	//
	*t = 0;
	return; 
    }
}

void ImageNode::getAutoAxesXTickLocs (double **t, int *size) 
{
    *size = 0;
    //if (!this->isAutoAxesXTickLocsSet()) return ;

    ASSERT(t);
    Parameter *p = this->getInputParameter(AA_XTICK_LOCS);
    ASSERT(p);

    if ((p->hasValue()) && (DXType::ScalarListType == p->getValueType())) {
	const char *v;
	if (this->isAutoAxesXTickLocsSet()) 
	    v = this->getInputValueString(AA_XTICK_LOCS);
	else
	    v = this->getInputSetValueString(AA_XTICK_LOCS);

	if ((!v) || (!v[0]) || (EqualString(v, "NULL"))) return ;

	int tuple;

	int cnt = DXValue::GetDoublesFromList (v, DXType::ScalarListType, t, &tuple);
	if (cnt) ASSERT (tuple == 1);
	*size = cnt;
    } else {
	//
	// USER Error?
	//
	return; 
    }
}

void ImageNode::getAutoAxesYTickLocs (double **t, int *size) 
{
    *size = 0;
    //if (!this->isAutoAxesYTickLocsSet()) return ;

    ASSERT(t);
    Parameter *p = this->getInputParameter(AA_YTICK_LOCS);
    ASSERT(p);

    if ((p->hasValue()) && (DXType::ScalarListType == p->getValueType())) {
	const char *v;
	if (this->isAutoAxesYTickLocsSet()) 
	    v = this->getInputValueString(AA_YTICK_LOCS);
	else
	    v = this->getInputSetValueString(AA_YTICK_LOCS);

	if ((!v) || (!v[0]) || (EqualString(v, "NULL"))) return ;

	int tuple;

	int cnt = DXValue::GetDoublesFromList (v, DXType::ScalarListType, t, &tuple);
	if (cnt) ASSERT (tuple == 1);
	*size = cnt;
    } else {
	//
	// USER Error?
	//
	return; 
    }
}

void ImageNode::getAutoAxesZTickLocs (double **t, int *size) 
{
    *size = 0;
    //if (!this->isAutoAxesZTickLocsSet()) return ;

    ASSERT(t);
    Parameter *p = this->getInputParameter(AA_ZTICK_LOCS);
    ASSERT(p);

    if ((p->hasValue()) && (DXType::ScalarListType == p->getValueType())) {
	const char *v;
	if (this->isAutoAxesZTickLocsSet()) 
	    v = this->getInputValueString(AA_ZTICK_LOCS);
	else
	    v = this->getInputSetValueString(AA_ZTICK_LOCS);

	if ((!v) || (!v[0]) || (EqualString(v, "NULL"))) return ;

	int tuple;

	int cnt = DXValue::GetDoublesFromList (v, DXType::ScalarListType, t, &tuple);
	if (cnt) ASSERT (tuple == 1);
	*size = cnt;
    } else {
	//
	// USER Error?
	//
	return; 
    }
}


void ImageNode::setAutoAxesTicks (int t1, int t2, int t3, boolean send)
{
char buf[128];
    sprintf (buf, "{ %d %d %d }", t1, t2, t3);
    this->setInputValue(AATICKS,buf, DXType::IntegerListType, send);
}

void ImageNode::setAutoAxesTicks (int t, boolean send)
{
char buf[32];
    sprintf (buf, "%d", t);
    this->setInputValue(AATICKS,buf, DXType::IntegerType, send);
}

void ImageNode::setAutoAxesXTickLocs (double *t, int size, boolean send)
{
    this->setAutoAxesTickLocs (AA_XTICK_LOCS, t, size, send);
}
void ImageNode::setAutoAxesTickLocs (int param, double *t, int size, boolean send)
{
    if (!size)
	this->setInputValue(param, "NULL", DXType::ScalarListType, send);
    else {
	int i, len, buflen = 0;
	char *buffer = new char[16*size];
	char tmpbuf[16];

	for (i=0; i<size; i++) {
	    if (i == 0) {
		strcpy (buffer, "{ ");
		buflen = strlen(buffer);
	    } else {
		strcpy (&buffer[buflen], ", "); buflen+= 2;
	    }

	    sprintf (tmpbuf, "%f", t[i]);
	    len = strlen(tmpbuf);
	    strcpy (&buffer[buflen], tmpbuf); buflen+= len;
	}
	strcpy (&buffer[buflen], " }"); buflen+= 2;
	this->setInputValue(param, buffer, DXType::ScalarListType, send);
	delete buffer;
    }
}

void ImageNode::setAutoAxesYTickLocs (double *t, int size, boolean send)
{
    this->setAutoAxesTickLocs (AA_YTICK_LOCS, t, size, send);
}

void ImageNode::setAutoAxesZTickLocs (double *t, int size, boolean send)
{
    this->setAutoAxesTickLocs (AA_ZTICK_LOCS, t, size, send);
}

void ImageNode::unsetAutoAxesTicks (boolean send)
{
    this->useDefaultInputValue (AATICKS, send);
}

void ImageNode::unsetAutoAxesXTickLocs (boolean send)
{
    this->useDefaultInputValue (AA_XTICK_LOCS, send);
}

void ImageNode::unsetAutoAxesYTickLocs (boolean send)
{
    this->useDefaultInputValue (AA_YTICK_LOCS, send);
}

void ImageNode::unsetAutoAxesZTickLocs (boolean send)
{
    this->useDefaultInputValue (AA_ZTICK_LOCS, send);
}

void ImageNode::unsetRecordResolution (boolean send)
{
    this->useDefaultInputValue (RECRESOLUTION, send);
}

void ImageNode::unsetRecordAspect (boolean send)
{
    this->useDefaultInputValue (RECASPECT, send);
}

void ImageNode::unsetRecordFormat (boolean send)
{
    this->useDefaultInputValue (RECFORMAT, send);
}

void ImageNode::unsetRecordFile (boolean send)
{
    this->useDefaultInputValue (RECFILE, send);
}

void ImageNode::unsetRecordEnable (boolean send)
{
    this->useDefaultInputValue (RECENABLE, send);
}

//
// S T R I N G   S T R I N G   S T R I N G   S T R I N G   S T R I N G   
// S T R I N G   S T R I N G   S T R I N G   S T R I N G   S T R I N G   
//
boolean ImageNode::isAutoAxesFontSet ()
{ return this->isAutoAxesStringSet (AAFONT); }

int ImageNode::getAutoAxesFont (char *sval)
{ return this->getAutoAxesString (AAFONT, sval); }

void ImageNode::setAutoAxesFont (char *value, boolean send)
{ this->setAutoAxesString (AAFONT, value, send); }

boolean ImageNode::isAutoAxesStringSet(int index)
{
   return !EqualString(this->getInputValueString(index),"NULL");
}

void ImageNode::setAutoAxesString (int index, char *value, boolean send)
{
    this->setInputValue(index, value, DXType::StringType, send);
}

void ImageNode::unsetAutoAxesString (int index, boolean send)
{
    this->setInputValue(index, "NULL", DXType::StringType, send);
}

int ImageNode::getAutoAxesString (int index, char *sval)
{
const char *v = this->getInputValueString(index);

    sval[0] = '\0';
    sscanf (v, "\"%[^\"]\"", sval);
    return strlen(sval);
}


//
// I N T E G E R   I N T E G E R   I N T E G E R   I N T E G E R   
// I N T E G E R   I N T E G E R   I N T E G E R   I N T E G E R   
//
int ImageNode::getAutoAxesEnable()
{ return this->getAutoAxesInteger (AAENABLE); }

void ImageNode::setAutoAxesEnable(int d, boolean send)
{ this->setAutoAxesInteger (AAENABLE, d, send); }

int ImageNode::getAutoAxesInteger(int index)
{
    const char* v = this->getInputValueString(index);
    if (!v) return 0;
    if (EqualString(v,"NULL")) return 0;

    int val = 0;
    sscanf(v,"%d",&val);
    return val;
}

void ImageNode::setAutoAxesInteger (int index, int d, boolean send)
{
char buf[64];
    sprintf (buf, "%d", d);
    this->setInputValue(index, buf, DXType::IntegerType, send);
}



//
// D O U B L E   D O U B L E   D O U B L E   D O U B L E   D O U B L E   
// D O U B L E   D O U B L E   D O U B L E   D O U B L E   D O U B L E   
//
boolean ImageNode::isAutoAxesLabelScaleSet()
{ return this->isAutoAxesDoubleSet (AALABELSCALE); }

double ImageNode::getAutoAxesLabelScale ()
{ return this->getAutoAxesDouble (AALABELSCALE); }

void ImageNode::setAutoAxesLabelScale (double d, boolean send)
{ this->setAutoAxesDouble (AALABELSCALE, d, send); }

boolean ImageNode::isAutoAxesDoubleSet(int index)
{
   return !EqualString(this->getInputValueString(index),"NULL");
}

boolean ImageNode::isRenderModeSet()
{
    return !this->isInputDefaulting(RENDER_MODE);
}

boolean ImageNode::isButtonUpApproxSet()
{
    return !this->isInputDefaulting(BUTTON_UP_APPROX);
}

boolean ImageNode::isButtonDownApproxSet()
{
    return !this->isInputDefaulting(BUTTON_DOWN_APPROX);
}

boolean ImageNode::isButtonUpDensitySet()
{
    return !this->isInputDefaulting(BUTTON_UP_DENSITY);
}

boolean ImageNode::isButtonDownDensitySet()
{
    return !this->isInputDefaulting(BUTTON_DOWN_DENSITY);
}

double ImageNode::getAutoAxesDouble(int index)
{
    const char* v = this->getInputValueString(index);
    if (!v) return 0.0;
    if (EqualString(v,"NULL")) return 0.0;

    double val = 0.0;
    sscanf(v,"%lf",&val);
    return val;
}

void ImageNode::setAutoAxesDouble (int index, double d, boolean send)
{
char buf[64];
    sprintf (buf, "%f", d);
    this->setInputValue(index, buf, DXType::ScalarType, send);
}




boolean ImageNode::isAutoAxesEnableConnected ()
{
    return this->isInputConnected(AAENABLE);
}
boolean ImageNode::isAutoAxesCornersConnected ()
{
    return this->isInputConnected(AACORNERS);
}
boolean ImageNode::isAutoAxesCursorConnected ()
{
    return this->isInputConnected(AACURSOR);
}
boolean ImageNode::isAutoAxesFrameConnected ()
{
    return this->isInputConnected(AAFRAME);
}
boolean ImageNode::isAutoAxesGridConnected ()
{
    return this->isInputConnected(AAGRID);
}
boolean ImageNode::isAutoAxesAdjustConnected ()
{
    return this->isInputConnected(AAADJUST);
}
boolean ImageNode::isAutoAxesAnnotationConnected ()
{
    return this->isInputConnected(AAANNOTATION);
}
boolean ImageNode::isAutoAxesLabelsConnected ()
{
    return this->isInputConnected(AALABELS);
}
boolean ImageNode::isAutoAxesColorsConnected ()
{
    return this->isInputConnected(AACOLORS);
}
boolean ImageNode::isAutoAxesFontConnected ()
{
    return this->isInputConnected(AAFONT);
}
boolean ImageNode::isAutoAxesTicksConnected ()
{
    return this->isInputConnected(AATICKS);
}
boolean ImageNode::isAutoAxesXTickLocsConnected ()
{
    return this->isInputConnected(AA_XTICK_LOCS);
}
boolean ImageNode::isAutoAxesYTickLocsConnected ()
{
    return this->isInputConnected(AA_YTICK_LOCS);
}
boolean ImageNode::isAutoAxesZTickLocsConnected ()
{
    return this->isInputConnected(AA_ZTICK_LOCS);
}
boolean ImageNode::isAutoAxesXTickLabelsConnected ()
{
    return this->isInputConnected(AA_XTICK_LABELS);
}
boolean ImageNode::isAutoAxesYTickLabelsConnected ()
{
    return this->isInputConnected(AA_YTICK_LABELS);
}
boolean ImageNode::isAutoAxesZTickLabelsConnected ()
{
    return this->isInputConnected(AA_ZTICK_LABELS);
}
boolean ImageNode::isAutoAxesLabelScaleConnected ()
{
    return this->isInputConnected(AALABELSCALE);
}
boolean ImageNode::isBGColorConnected()
{
    return this->isInputConnected(BACKGROUND);
}
boolean ImageNode::isThrottleConnected()
{
    return this->isInputConnected(THROTTLE);
}
boolean ImageNode::isRecordEnableConnected()
{
    return this->isInputConnected(RECENABLE);
}
boolean ImageNode::isRecordFileConnected()
{
    return this->isInputConnected(RECFILE);
}
boolean ImageNode::isRecordFormatConnected()
{
    return this->isInputConnected(RECFORMAT);
}
boolean ImageNode::isRecordResolutionConnected()
{
    return this->isInputConnected(RECRESOLUTION);
}
boolean ImageNode::isRecordAspectConnected()
{
    return this->isInputConnected(RECASPECT);
}
boolean ImageNode::isInteractionModeConnected()
{
    return this->isInputConnected(INTERACTIONMODE);
}

boolean ImageNode::isRenderModeConnected()
{
    return this->isInputConnected(RENDER_MODE);
}
boolean ImageNode::isButtonUpApproxConnected()
{
    return this->isInputConnected(BUTTON_UP_APPROX);
}
boolean ImageNode::isButtonDownApproxConnected()
{
    return this->isInputConnected(BUTTON_DOWN_APPROX);
}
boolean ImageNode::isButtonUpDensityConnected()
{
    return this->isInputConnected(BUTTON_UP_DENSITY);
}
boolean ImageNode::isButtonDownDensityConnected()
{
    return this->isInputConnected(BUTTON_DOWN_DENSITY);
}


/*
 * These tell us whether the view control dialog and image name dialog
 * should be disabled - tab down but not connected.
 */
boolean ImageNode::isViewControlInputSet()
{
    return this->isInputDefaulting(INTERACTIONMODE);
}

boolean ImageNode::isImageNameInputSet()
{
    return this->isInputDefaulting(IMAGENAME);
}

boolean ImageNode::isRecordEnableSet()
{
    return !this->isInputDefaulting(RECENABLE);
}

boolean ImageNode::isRecordFileSet()
{
    return !this->isInputDefaulting(RECFILE);
}

boolean ImageNode::isRecordFormatSet()
{
    return !this->isInputDefaulting(RECFORMAT);
}

boolean ImageNode::isRecordResolutionSet()
{
    return !this->isInputDefaulting(RECRESOLUTION);
}

boolean ImageNode::isRecordAspectSet()
{
    return !this->isInputDefaulting(RECASPECT);
}

boolean ImageNode::isRecFileInputSet()
{
#if 0
    return this->isInputDefaulting(RECENABLE) ||
	   this->isInputDefaulting(RECFILE)   ||
	   this->isInputDefaulting(RECFORMAT);
#else
    return TRUE;
#endif

}



boolean ImageNode::SendString(void* callbackData, PacketIFCallback cb, FILE* f, char* s, boolean viasocket) 
{ 
    char *_s = s;
    if (!viasocket) {
	if (fputs(_s, f) < 0)
	    return FALSE;
	if (cb)
	    (*cb)(callbackData, _s);
    } else {
	DXPacketIF* pif = theDXApplication->getPacketIF();
	pif->sendBytes(_s);
    }
    return TRUE;
}

void ImageNode::FormatMacro (FILE* f, PacketIFCallback cb, void* cbd, String mac[], boolean viasocket)
{
	int i = 0;
#   define FMBUFSIZE 90
	char tmpbuf[FMBUFSIZE];
	int buflen = 0;
	while (mac[i]) {
		int length = strlen(mac[i]);
		if ((buflen + length) >= FMBUFSIZE - 1) {
			if (buflen > 0) {
				tmpbuf[buflen++] = '\n'; tmpbuf[buflen] = '\0';
				SendString (cbd, cb, f, tmpbuf, viasocket);
				//fwrite (tmpbuf, 1, buflen, f);
				//fflush(f);
				buflen = 0;
			} else {
				SendString (cbd, cb, f, mac[i], viasocket);
				i++;
				//fwrite (mac[i++], 1, length, f);
				//fflush(f);
			}
		} else {
			strcpy (&tmpbuf[buflen], mac[i]);
			i++;
			buflen+= length;
		}
	}
	if (buflen > 0) {
		tmpbuf[buflen++] = '\n'; tmpbuf[buflen] = '\0';
		SendString (cbd, cb, f, tmpbuf, viasocket);
		//fwrite (tmpbuf, 1, buflen, f);
		//fflush(f);
	}
}

boolean ImageNode::sendMacro(DXPacketIF *pif)
{
    void *cbdata;
    PacketIFCallback cb = pif->getEchoCallback(&cbdata);
    FILE* f = pif->getFILE();
    boolean viasocket = TRUE;

    if (this->getNetwork()->isJavified()) {
	pif->sendMacroStart();
	FormatMacro(f, cb, cbdata, ImageNode::GifMacroTxt, viasocket);
	pif->sendMacroEnd();

	pif->sendMacroStart();
	FormatMacro(f, cb, cbdata, ImageNode::VrmlMacroTxt, viasocket);
	pif->sendMacroEnd();

	pif->sendMacroStart();
	FormatMacro(f, cb, cbdata, ImageNode::DXMacroTxt, viasocket);
	pif->sendMacroEnd();
    }

    pif->sendMacroStart();
    boolean sts = this->printMacro(pif->getFILE(), cb, cbdata, TRUE);
    pif->sendMacroEnd();

    return sts;
}


boolean ImageNode::printMacro(FILE *f,
			      PacketIFCallback cb,
                               void *cbdata,
                               boolean viasocket)

{

    char buf[256];
    enum Cacheability cacheability = this->getInternalCacheability();
    int cacheflag = (cacheability == InternalsNotCached) ? 0 :
		    (cacheability == InternalsFullyCached) ? 1 : 2;

    //
    // If the network is intended for use under DXServer use the
    // new image macro.  Using the old image macro is a cheap way
    // of avoiding bugs in the new image macro.  A better way
    // to handle this is to make sure the new one doesn't have any
    // bugs in it.  Problem is the new is so big that it will
    // probably be a performance hit.
    //
    if (this->getNetwork()->isJavified()) {
	FormatMacro(f, cb, cbdata, ImageNode::ImageMacroTxt, viasocket);
    } else {
	void* cbd = cbdata;
	SendString(cbd, cb, f, "macro Image(\n", viasocket);
	SendString(cbd, cb, f, "        id,\n", viasocket);
	SendString(cbd, cb, f, "        object,\n", viasocket);
	SendString(cbd, cb, f, "        where,\n", viasocket);
	SendString(cbd, cb, f, "        useVector,\n", viasocket);
	SendString(cbd, cb, f, "        to,\n", viasocket);
	SendString(cbd, cb, f, "        from,\n", viasocket);
	SendString(cbd, cb, f, "        width,\n", viasocket);
	SendString(cbd, cb, f, "        resolution,\n", viasocket);
	SendString(cbd, cb, f, "        aspect,\n", viasocket);
	SendString(cbd, cb, f, "        up,\n", viasocket);
	SendString(cbd, cb, f, "        viewAngle,\n", viasocket);
	SendString(cbd, cb, f, "        perspective,\n", viasocket);
	SendString(cbd, cb, f, "        options,\n", viasocket);
	SendString(cbd, cb, f, "        buttonState = 1,\n", viasocket);
	SendString(cbd, cb, f, "        buttonUpApprox = \"none\",\n", viasocket);
	SendString(cbd, cb, f, "        buttonDownApprox = \"none\",\n", viasocket);
	SendString(cbd, cb, f, "        buttonUpDensity = 1,\n", viasocket);
	SendString(cbd, cb, f, "        buttonDownDensity = 1,\n", viasocket);
	SendString(cbd, cb, f, "        renderMode = 0,\n", viasocket);
	SendString(cbd, cb, f, "        defaultCamera,\n", viasocket);
	SendString(cbd, cb, f, "        reset,\n", viasocket);
	SendString(cbd, cb, f, "        backgroundColor,\n", viasocket);
	SendString(cbd, cb, f, "        throttle,\n", viasocket);
	SendString(cbd, cb, f, "        RECenable = 0,\n", viasocket);
	SendString(cbd, cb, f, "        RECfile,\n", viasocket);
	SendString(cbd, cb, f, "        RECformat,\n", viasocket);
	SendString(cbd, cb, f, "        RECresolution,\n", viasocket);
	SendString(cbd, cb, f, "        RECaspect,\n", viasocket);
	SendString(cbd, cb, f, "        AAenable = 0,\n", viasocket);
	SendString(cbd, cb, f, "        AAlabels,\n", viasocket);
	SendString(cbd, cb, f, "        AAticks,\n", viasocket);
	SendString(cbd, cb, f, "        AAcorners,\n", viasocket);
	SendString(cbd, cb, f, "        AAframe,\n", viasocket);
	SendString(cbd, cb, f, "        AAadjust,\n", viasocket);
	SendString(cbd, cb, f, "        AAcursor,\n", viasocket);
	SendString(cbd, cb, f, "        AAgrid,\n", viasocket);
	SendString(cbd, cb, f, "        AAcolors,\n", viasocket);
	SendString(cbd, cb, f, "        AAannotation,\n", viasocket);
	SendString(cbd, cb, f, "        AAlabelscale,\n", viasocket);
	SendString(cbd, cb, f, "        AAfont,\n", viasocket);
	SendString(cbd, cb, f, "        interactionMode,\n", viasocket);
	SendString(cbd, cb, f, "        title,\n", viasocket);
	SendString(cbd, cb, f, "        AAxTickLocs,\n", viasocket);
	SendString(cbd, cb, f, "        AAyTickLocs,\n", viasocket);
	SendString(cbd, cb, f, "        AAzTickLocs,\n", viasocket);
	SendString(cbd, cb, f, "        AAxTickLabels,\n", viasocket);
	SendString(cbd, cb, f, "        AAyTickLabels,\n", viasocket);
	SendString(cbd, cb, f, "        AAzTickLabels,\n", viasocket);
	SendString(cbd, cb, f, "        webOptions) -> (\n", viasocket);
	SendString(cbd, cb, f, "        object,\n", viasocket);
	SendString(cbd, cb, f, "        camera,\n", viasocket);
	SendString(cbd, cb, f, "        where)\n", viasocket);

	//
	// Begin macro body.
	//
	SendString(cbd, cb, f, "{\n", viasocket);

    #if 0
	SendString(cbd, cb, f, "    Echo(\n", viasocket);
	SendString(cbd, cb, f, "        id,\n", viasocket);
	SendString(cbd, cb, f, "        backgroundColor,\n", viasocket);
	SendString(cbd, cb, f, "        throttle,\n", viasocket);
	SendString(cbd, cb, f, "        RECenable,\n", viasocket);
	SendString(cbd, cb, f, "        RECfile,\n", viasocket);
	SendString(cbd, cb, f, "        RECformat,\n", viasocket);
	SendString(cbd, cb, f, "        AAenable,\n", viasocket);
	SendString(cbd, cb, f, "        AAlabels,\n", viasocket);
	SendString(cbd, cb, f, "        AAticks,\n", viasocket);
	SendString(cbd, cb, f, "        AAcorners,\n", viasocket);
	SendString(cbd, cb, f, "        AAframe,\n", viasocket);
	SendString(cbd, cb, f, "        AAadjust,\n", viasocket);
	SendString(cbd, cb, f, "        AAcursor,\n", viasocket);
	SendString(cbd, cb, f, "        AAgrid,\n", viasocket);
	SendString(cbd, cb, f, "        AAcolors,\n", viasocket);
	SendString(cbd, cb, f, "        AAannotation,\n", viasocket);
	SendString(cbd, cb, f, "        AAlabelscale,\n", viasocket);
	SendString(cbd, cb, f, "        AAfont,\n", viasocket);
	SendString(cbd, cb, f, "        AAxTickLocs,\n", viasocket);
	SendString(cbd, cb, f, "        AAyTickLocs,\n", viasocket);
	SendString(cbd, cb, f, "        AAzTickLocs,\n", viasocket);
	SendString(cbd, cb, f, "        AAxTickLabels,\n", viasocket);
	SendString(cbd, cb, f, "        AAyTickLabels,\n", viasocket);
	SendString(cbd, cb, f, "        AAzTickLabels,\n", viasocket);
	SendString(cbd, cb, f, "        interactionMode,\n", viasocket);
	SendString(cbd, cb, f, "        title) [instance: 1];\n", viasocket);
    #endif

	SendString(cbd, cb, f, "    ImageMessage(\n", viasocket);
	SendString(cbd, cb, f, "        id,\n", viasocket);
	SendString(cbd, cb, f, "        backgroundColor,\n", viasocket);
	SendString(cbd, cb, f, "        throttle,\n", viasocket);
	SendString(cbd, cb, f, "        RECenable,\n", viasocket);
	SendString(cbd, cb, f, "        RECfile,\n", viasocket);
	SendString(cbd, cb, f, "        RECformat,\n", viasocket);
	SendString(cbd, cb, f, "        RECresolution,\n", viasocket);
	SendString(cbd, cb, f, "        RECaspect,\n", viasocket);
	SendString(cbd, cb, f, "        AAenable,\n", viasocket);
	SendString(cbd, cb, f, "        AAlabels,\n", viasocket);
	SendString(cbd, cb, f, "        AAticks,\n", viasocket);
	SendString(cbd, cb, f, "        AAcorners,\n", viasocket);
	SendString(cbd, cb, f, "        AAframe,\n", viasocket);
	SendString(cbd, cb, f, "        AAadjust,\n", viasocket);
	SendString(cbd, cb, f, "        AAcursor,\n", viasocket);
	SendString(cbd, cb, f, "        AAgrid,\n", viasocket);
	SendString(cbd, cb, f, "        AAcolors,\n", viasocket);
	SendString(cbd, cb, f, "        AAannotation,\n", viasocket);
	SendString(cbd, cb, f, "        AAlabelscale,\n", viasocket);
	SendString(cbd, cb, f, "        AAfont,\n", viasocket);
	SendString(cbd, cb, f, "        AAxTickLocs,\n", viasocket);
	SendString(cbd, cb, f, "        AAyTickLocs,\n", viasocket);
	SendString(cbd, cb, f, "        AAzTickLocs,\n", viasocket);
	SendString(cbd, cb, f, "        AAxTickLabels,\n", viasocket);
	SendString(cbd, cb, f, "        AAyTickLabels,\n", viasocket);
	SendString(cbd, cb, f, "        AAzTickLabels,\n", viasocket);
	SendString(cbd, cb, f, "        interactionMode,\n", viasocket);
	SendString(cbd, cb, f, "        title,\n", viasocket);
	SendString(cbd, cb, f, "        renderMode,\n", viasocket);
	SendString(cbd, cb, f, "        buttonUpApprox,\n", viasocket);
	SendString(cbd, cb, f, "        buttonDownApprox,\n", viasocket);
	SendString(cbd, cb, f, "        buttonUpDensity,\n", viasocket);
	sprintf(buf,"        buttonDownDensity) [instance: 1, cache: %d];\n", cacheflag);
	SendString(cbd, cb, f, buf, viasocket);

	//
	// Generate a call to Camera or AutoCamera module.
	//
	SendString(cbd, cb, f, "    autoCamera =\n", viasocket);
	SendString(cbd, cb, f, "        AutoCamera(\n", viasocket);
	SendString(cbd, cb, f, "            object,\n", viasocket);
	SendString(cbd, cb, f, "            \"front\",\n", viasocket);
	SendString(cbd, cb, f, "            object,\n", viasocket);
	SendString(cbd, cb, f, "            resolution,\n", viasocket);
	SendString(cbd, cb, f, "            aspect,\n", viasocket);
	SendString(cbd, cb, f, "            [0,1,0],\n", viasocket);
	SendString(cbd, cb, f, "            perspective,\n", viasocket);
	SendString(cbd, cb, f, "            viewAngle,\n", viasocket);
	sprintf(buf,"            backgroundColor) [instance: 1, cache: %d];\n",
								    cacheflag);
	SendString(cbd, cb, f, buf, viasocket);

	SendString(cbd, cb, f, "    realCamera =\n", viasocket);
	SendString(cbd, cb, f, "        Camera(\n", viasocket);
	SendString(cbd, cb, f, "            to,\n", viasocket);
	SendString(cbd, cb, f, "            from,\n", viasocket);
	SendString(cbd, cb, f, "            width,\n", viasocket);
	SendString(cbd, cb, f, "            resolution,\n", viasocket);
	SendString(cbd, cb, f, "            aspect,\n", viasocket);
	SendString(cbd, cb, f, "            up,\n", viasocket);
	SendString(cbd, cb, f, "            perspective,\n", viasocket);
	SendString(cbd, cb, f, "            viewAngle,\n", viasocket);
	sprintf(buf,"            backgroundColor) [instance: 1, cache: %d];\n",
								    cacheflag);
	SendString(cbd, cb, f, buf, viasocket);

	SendString(cbd, cb, f, "    coloredDefaultCamera = \n", viasocket);
	SendString(cbd, cb, f, "	 UpdateCamera(defaultCamera,\n", viasocket);
	sprintf(buf,"            background=backgroundColor) [instance: 1, cache: %d];\n",
								    cacheflag);
	SendString(cbd, cb, f, buf, viasocket);

	SendString(cbd, cb, f, "    nullDefaultCamera =\n", viasocket);
	SendString(cbd, cb, f, "        Inquire(defaultCamera,\n", viasocket);
	sprintf(buf,"            \"is null + 1\") [instance: 1, cache: %d];\n",
								    cacheflag);
	SendString(cbd, cb, f, buf, viasocket);

	SendString(cbd, cb, f, "    resetCamera =\n", viasocket);
	SendString(cbd, cb, f, "        Switch(\n", viasocket);
	SendString(cbd, cb, f, "            nullDefaultCamera,\n", viasocket);
	SendString(cbd, cb, f, "            coloredDefaultCamera,\n", viasocket);
	sprintf(buf,"            autoCamera) [instance: 1, cache: %d];\n",
								    cacheflag);
	SendString(cbd, cb, f, buf, viasocket);

	SendString(cbd, cb, f, "    resetNull = \n", viasocket);
	SendString(cbd, cb, f, "        Inquire(\n", viasocket);
	SendString(cbd, cb, f, "            reset,\n", viasocket);
	sprintf(buf,"            \"is null + 1\") [instance: 2, cache: %d];\n",
								    cacheflag);
	SendString(cbd, cb, f, buf, viasocket);

	SendString(cbd, cb, f, "    reset =\n", viasocket);
	SendString(cbd, cb, f, "        Switch(\n", viasocket);
	SendString(cbd, cb, f, "            resetNull,\n", viasocket);
	SendString(cbd, cb, f, "            reset,\n", viasocket);
	sprintf(buf,"            0) [instance: 2, cache: %d];\n", cacheflag);
	SendString(cbd, cb, f, buf, viasocket);

	SendString(cbd, cb, f, "    whichCamera =\n", viasocket);
	SendString(cbd, cb, f, "        Compute(\n", viasocket);
	SendString(cbd, cb, f, "            \"($0 != 0 || $1 == 0) ? 1 : 2\",\n", viasocket);
	SendString(cbd, cb, f, "            reset,\n", viasocket);
	sprintf(buf,"            useVector) [instance: 1, cache: %d];\n",
								    cacheflag);
	SendString(cbd, cb, f, buf, viasocket);


	SendString(cbd, cb, f, "    camera = Switch(\n", viasocket);
	SendString(cbd, cb, f, "            whichCamera,\n", viasocket);
	SendString(cbd, cb, f, "            resetCamera,\n", viasocket);
	sprintf(buf,"            realCamera) [instance: 3, cache: %d];\n",
								    cacheflag);
	SendString(cbd, cb, f, buf, viasocket);

	/*
	 * Generate a call to AutoAxes module.
	 */
	SendString(cbd, cb, f, "    AAobject =\n", viasocket);
	SendString(cbd, cb, f, "        AutoAxes(\n", viasocket);
	SendString(cbd, cb, f, "            object,\n", viasocket);
	SendString(cbd, cb, f, "            camera,\n", viasocket);
	SendString(cbd, cb, f, "            AAlabels,\n", viasocket);
	SendString(cbd, cb, f, "            AAticks,\n", viasocket);
	SendString(cbd, cb, f, "            AAcorners,\n", viasocket);
	SendString(cbd, cb, f, "            AAframe,\n", viasocket);
	SendString(cbd, cb, f, "            AAadjust,\n", viasocket);
	SendString(cbd, cb, f, "            AAcursor,\n", viasocket);
	SendString(cbd, cb, f, "            AAgrid,\n", viasocket);
	SendString(cbd, cb, f, "            AAcolors,\n", viasocket);
	SendString(cbd, cb, f, "            AAannotation,\n", viasocket);
	SendString(cbd, cb, f, "            AAlabelscale,\n", viasocket);
	SendString(cbd, cb, f, "            AAfont,\n", viasocket);
	SendString(cbd, cb, f, "            AAxTickLocs,\n", viasocket);
	SendString(cbd, cb, f, "            AAyTickLocs,\n", viasocket);
	SendString(cbd, cb, f, "            AAzTickLocs,\n", viasocket);
	SendString(cbd, cb, f, "            AAxTickLabels,\n", viasocket);
	SendString(cbd, cb, f, "            AAyTickLabels,\n", viasocket);
	sprintf(buf,"            AAzTickLabels) [instance: 1, cache: %d];\n", cacheflag);
	SendString(cbd, cb, f, buf, viasocket);

	SendString(cbd, cb, f, "    switchAAenable = Compute(\"$0+1\",\n", viasocket);
	sprintf(buf,"	     AAenable) [instance: 2, cache: %d];\n", cacheflag);
	SendString(cbd, cb, f, buf, viasocket);
       
	SendString(cbd, cb, f, "    object = Switch(\n", viasocket);
	SendString(cbd, cb, f, "	     switchAAenable,\n", viasocket);
	SendString(cbd, cb, f, "	     object,\n", viasocket);
	sprintf(buf,"	     AAobject) [instance:4, cache: %d];\n", cacheflag);
	SendString(cbd, cb, f, buf, viasocket);

	/*
	 * Generate a call to the Approximation Switch module.
	 */
	SendString(cbd, cb, f, "    SWapproximation_options =\n", viasocket);
	SendString(cbd, cb, f, "        Switch(\n", viasocket); 
	SendString(cbd, cb, f, "            buttonState,\n", viasocket);
	SendString(cbd, cb, f, "            buttonUpApprox,\n", viasocket);
	sprintf(buf,"            buttonDownApprox) [instance: 5, cache: %d];\n",
								    cacheflag);
	SendString(cbd, cb, f, buf, viasocket);

	/*
	 * Generate a call to the Density Switch module.
	 */
	SendString(cbd, cb, f, "    SWdensity_options =\n", viasocket);
	SendString(cbd, cb, f, "        Switch(\n", viasocket); 
	SendString(cbd, cb, f, "            buttonState,\n", viasocket);
	SendString(cbd, cb, f, "            buttonUpDensity,\n", viasocket);
	sprintf(buf,"            buttonDownDensity) [instance: 6, cache: %d];\n",
								    cacheflag);
	SendString(cbd, cb, f, buf, viasocket);

	/*
	 * Generate a call to the Approximation Format module.
	 */
	SendString(cbd, cb, f, "    HWapproximation_options =\n", viasocket);
	SendString(cbd, cb, f, "        Format(\n", viasocket); 
	SendString(cbd, cb, f, "            \"%s,%s\",\n", viasocket);
	SendString(cbd, cb, f, "            buttonDownApprox,\n", viasocket);
	sprintf(buf,"            buttonUpApprox) [instance: 1, cache: %d];\n",
								    cacheflag);
	SendString(cbd, cb, f, buf, viasocket);

	/*
	 * Generate a call to the Density Format module.
	 */
	SendString(cbd, cb, f, "    HWdensity_options =\n", viasocket);
	SendString(cbd, cb, f, "        Format(\n", viasocket); 
	SendString(cbd, cb, f, "            \"%d,%d\",\n", viasocket);
	SendString(cbd, cb, f, "            buttonDownDensity,\n", viasocket);
	sprintf(buf,"            buttonUpDensity) [instance: 2, cache: %d];\n",
								    cacheflag);
	SendString(cbd, cb, f, buf, viasocket);

	SendString(cbd, cb, f, "    switchRenderMode = Compute(\n", viasocket);
	SendString(cbd, cb, f, "	     \"$0+1\",\n", viasocket);
	sprintf(buf,"	     renderMode) [instance: 3, cache: %d];\n",
								    cacheflag);
	SendString(cbd, cb, f, buf, viasocket);

	SendString(cbd, cb, f, "    approximation_options = Switch(\n", viasocket);
	SendString(cbd, cb, f, "	     switchRenderMode,\n", viasocket);
	SendString(cbd, cb, f, "            SWapproximation_options,\n", viasocket);
	SendString(cbd, cb, f, "	     HWapproximation_options)", viasocket);
	sprintf(buf,              " [instance: 7, cache: %d];\n", cacheflag);
	SendString(cbd, cb, f, buf, viasocket);

	SendString(cbd, cb, f, "    density_options = Switch(\n", viasocket);
	SendString(cbd, cb, f, "	     switchRenderMode,\n", viasocket);
	SendString(cbd, cb, f, "            SWdensity_options,\n", viasocket);
	sprintf(buf,"            HWdensity_options) [instance: 8, cache: %d];\n",
								    cacheflag);
	SendString(cbd, cb, f, buf, viasocket);
	SendString(cbd, cb, f, "    renderModeString = Switch(\n", viasocket);
	SendString(cbd, cb, f, "            switchRenderMode,\n", viasocket);
	SendString(cbd, cb, f, "            \"software\",\n", viasocket);
	sprintf(buf,"            \"hardware\")[instance: 9, cache: %d];\n",
								    cacheflag);
	SendString(cbd, cb, f, buf, viasocket);

	SendString(cbd, cb, f, "    object_tag = Inquire(\n", viasocket);
	SendString(cbd, cb, f, "            object,\n", viasocket);
	sprintf(buf,"            \"object tag\")[instance: 3, cache: %d];\n",
								  cacheflag);
	SendString(cbd, cb, f, buf, viasocket);

	/*
	 * Generate a call to Options module.
	 */
	SendString(cbd, cb, f, "    annoted_object =\n", viasocket);
	SendString(cbd, cb, f, "        Options(\n", viasocket); 
	SendString(cbd, cb, f, "            object,\n", viasocket);
	SendString(cbd, cb, f, "            \"send boxes\",\n", viasocket);
	SendString(cbd, cb, f, "            0,\n", viasocket);
	SendString(cbd, cb, f, "            \"cache\",\n", viasocket);
	sprintf(buf,"            %d,\n", cacheflag == InternalsFullyCached ? 1 : 0);
	SendString(cbd, cb, f, buf, viasocket);
	SendString(cbd, cb, f, "            \"object tag\",\n", viasocket);
	SendString(cbd, cb, f, "            object_tag,\n", viasocket);
	SendString(cbd, cb, f, "            \"ddcamera\",\n", viasocket);
	SendString(cbd, cb, f, "            whichCamera,\n", viasocket);
	SendString(cbd, cb, f, "            \"rendering approximation\",\n", viasocket);
	SendString(cbd, cb, f, "            approximation_options,\n", viasocket);
	SendString(cbd, cb, f, "            \"render every\",\n", viasocket);
	SendString(cbd, cb, f, "            density_options,\n", viasocket);
	SendString(cbd, cb, f, "            \"button state\",\n", viasocket);
	SendString(cbd, cb, f, "            buttonState,\n", viasocket);
	SendString(cbd, cb, f, "            \"rendering mode\",\n", viasocket);
	sprintf(buf,"            renderModeString) [instance: 1, cache: %d];\n",
								    cacheflag);
	SendString(cbd, cb, f, buf, viasocket);

	SendString(cbd, cb, f, "    RECresNull =\n", viasocket);
	SendString(cbd, cb, f, "        Inquire(\n", viasocket);
	SendString(cbd, cb, f, "            RECresolution,\n", viasocket);
	sprintf(buf,"            \"is null + 1\") [instance: 4, cache: %d];\n",
								    cacheflag);
	SendString(cbd, cb, f, buf, viasocket);

	SendString(cbd, cb, f, "    ImageResolution =\n", viasocket);
	SendString(cbd, cb, f, "        Inquire(\n", viasocket);
	SendString(cbd, cb, f, "            camera,\n", viasocket);
	sprintf(buf,"            \"camera resolution\") [instance: 5, cache: %d];\n",
								    cacheflag);
	SendString(cbd, cb, f, buf, viasocket);
		    
	SendString(cbd, cb, f, "    RECresolution =\n", viasocket);
	SendString(cbd, cb, f, "        Switch(\n", viasocket);
	SendString(cbd, cb, f, "            RECresNull,\n", viasocket);
	SendString(cbd, cb, f, "            RECresolution,\n", viasocket);
	sprintf(buf,"            ImageResolution) [instance: 10, cache: %d];\n",
								    cacheflag);
	SendString(cbd, cb, f, buf, viasocket);

	SendString(cbd, cb, f, "    RECaspectNull =\n", viasocket);
	SendString(cbd, cb, f, "        Inquire(\n", viasocket);
	SendString(cbd, cb, f, "            RECaspect,\n", viasocket);
	sprintf(buf,"            \"is null + 1\") [instance: 6, cache: %d];\n",
								    cacheflag);
	SendString(cbd, cb, f, buf, viasocket);


	SendString(cbd, cb, f, "    ImageAspect =\n", viasocket);
	SendString(cbd, cb, f, "        Inquire(\n", viasocket);
	SendString(cbd, cb, f, "            camera,\n", viasocket);
	sprintf(buf,"            \"camera aspect\") [instance: 7, cache: %d];\n",
								    cacheflag);
	SendString(cbd, cb, f, buf, viasocket);
		    
	SendString(cbd, cb, f, "    RECaspect =\n", viasocket);
	SendString(cbd, cb, f, "        Switch(\n", viasocket);
	SendString(cbd, cb, f, "            RECaspectNull,\n", viasocket);
	SendString(cbd, cb, f, "            RECaspect,\n", viasocket);
	sprintf(buf,"            ImageAspect) [instance: 11, cache: %d];\n",
								    cacheflag);
	SendString(cbd, cb, f, buf, viasocket);

	SendString(cbd, cb, f, "    switchRECenable = Compute(\n", viasocket);
	SendString(cbd, cb, f, "          \"$0 == 0 ? 1 : (($2 == $3) && ($4 == $5)) ? ($1 == 1 ? 2 : 3) : 4\",\n", viasocket); 
	SendString(cbd, cb, f, "            RECenable,\n", viasocket);
	SendString(cbd, cb, f, "            switchRenderMode,\n", viasocket);
	SendString(cbd, cb, f, "            RECresolution,\n", viasocket);
	SendString(cbd, cb, f, "            ImageResolution,\n", viasocket);
	SendString(cbd, cb, f, "            RECaspect,\n", viasocket);
	sprintf(buf,"	     ImageAspect) [instance: 4, cache: %d];\n",
								    cacheflag);
	SendString(cbd, cb, f, buf, viasocket);

	SendString(cbd, cb, f, "    NoRECobject, RECNoRerenderObject, RECNoRerHW, RECRerenderObject = "
			    "Route(switchRECenable, annoted_object);\n", viasocket);

	/*
	 * If no recording is specified, just use Display
	 */
	SendString(cbd, cb, f, "    Display(\n", viasocket);
	SendString(cbd, cb, f, "        NoRECobject,\n", viasocket);
	SendString(cbd, cb, f, "        camera,\n", viasocket);
	SendString(cbd, cb, f, "        where,\n", viasocket);
	sprintf(buf,"        throttle) [instance: 1, cache: %d];\n", cacheflag);
	SendString(cbd, cb, f, buf, viasocket);

	/*
	 * If recording is used, but no rerendering is required,
	 * use Render followed by DIsplay and Write Image
	 */
	SendString(cbd, cb, f, "    image =\n", viasocket);
	SendString(cbd, cb, f, "        Render(\n", viasocket);
	SendString(cbd, cb, f, "            RECNoRerenderObject,\n", viasocket);
	sprintf(buf,"            camera) [instance: 1, cache: %d];\n", cacheflag);
	SendString(cbd, cb, f, buf, viasocket);

	SendString(cbd, cb, f, "    Display(\n", viasocket);
	SendString(cbd, cb, f, "        image,\n", viasocket);
	SendString(cbd, cb, f, "        NULL,\n", viasocket);
	SendString(cbd, cb, f, "        where,\n", viasocket);
	sprintf(buf,"        throttle) [instance: 2, cache: %d];\n", cacheflag);
	SendString(cbd, cb, f, buf, viasocket);

	SendString(cbd, cb, f, "    WriteImage(\n", viasocket);
	SendString(cbd, cb, f, "        image,\n", viasocket);
	SendString(cbd, cb, f, "        RECfile,\n", viasocket);
	sprintf(buf,"        RECformat) [instance: 1, cache: %d];\n", cacheflag);
	SendString(cbd, cb, f, buf, viasocket);


	/*
	 * If recording is used in a hardware window, but no rerendering is required,
	 * use Display and ReadImageWindow.
	 */
	SendString(cbd, cb, f, "    rec_where = Display(\n", viasocket);
	SendString(cbd, cb, f, "        RECNoRerHW,\n", viasocket);
	SendString(cbd, cb, f, "        camera,\n", viasocket);
	SendString(cbd, cb, f, "        where,\n", viasocket);
	sprintf(buf,"        throttle) [instance: 1, cache: %d];\n", 0/*cacheflag*/);
	SendString(cbd, cb, f, buf, viasocket);

	SendString(cbd, cb, f, "    rec_image = ReadImageWindow(\n", viasocket);
	sprintf(buf,"        rec_where) [instance: 1, cache: %d];\n", cacheflag);
	SendString(cbd, cb, f, buf, viasocket);

	SendString(cbd, cb, f, "    WriteImage(\n", viasocket);
	SendString(cbd, cb, f, "        rec_image,\n", viasocket);
	SendString(cbd, cb, f, "        RECfile,\n", viasocket);
	sprintf(buf,"        RECformat) [instance: 1, cache: %d];\n", cacheflag);
	SendString(cbd, cb, f, buf, viasocket);

	/*
	 * If recording, and rerendering *is* required, use Display 
	 * for the image window and Render and WriteImage along with
	 * the updated camera for the saved image
	 */

	SendString(cbd, cb, f, "    RECupdateCamera =\n", viasocket);
	SendString(cbd, cb, f, "	UpdateCamera(\n", viasocket);
	SendString(cbd, cb, f, "	    camera,\n", viasocket);
	SendString(cbd, cb, f, "	    resolution=RECresolution,\n", viasocket);
	sprintf(buf,"	    aspect=RECaspect) [instance: 2, cache: %d];\n",
								    cacheflag);
	SendString(cbd, cb, f, buf, viasocket);

	SendString(cbd, cb, f, "    Display(\n", viasocket);
	SendString(cbd, cb, f, "        RECRerenderObject,\n", viasocket);
	SendString(cbd, cb, f, "        camera,\n", viasocket);
	SendString(cbd, cb, f, "        where,\n", viasocket);
	sprintf(buf,"        throttle) [instance: 1, cache: %d];\n", cacheflag);
	SendString(cbd, cb, f, buf, viasocket);

	SendString(cbd, cb, f, "    RECRerenderObject =\n", viasocket);
	SendString(cbd, cb, f, "	ScaleScreen(\n", viasocket);
	SendString(cbd, cb, f, "	    RECRerenderObject,\n", viasocket);
	SendString(cbd, cb, f, "	    NULL,\n", viasocket);
	SendString(cbd, cb, f, "	    RECresolution,\n", viasocket);
	sprintf(buf,"	    camera) [instance: 1, cache: %d];\n", cacheflag);
	SendString(cbd, cb, f, buf, viasocket);

	SendString(cbd, cb, f, "    image =\n", viasocket);
	SendString(cbd, cb, f, "        Render(\n", viasocket);
	SendString(cbd, cb, f, "            RECRerenderObject,\n", viasocket);
	sprintf(buf,"            RECupdateCamera) [instance: 2, cache: %d];\n",
								    cacheflag);
	SendString(cbd, cb, f, buf, viasocket);

	SendString(cbd, cb, f, "    WriteImage(\n", viasocket);
	SendString(cbd, cb, f, "        image,\n", viasocket);
	SendString(cbd, cb, f, "        RECfile,\n", viasocket);
	sprintf(buf,"        RECformat) [instance: 2, cache: %d];\n", cacheflag);
	SendString(cbd, cb, f, buf, viasocket);



	//
	// End macro body.
	//
	SendString(cbd, cb, f, "}\n", viasocket);
    }

    return TRUE;

}


