/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"



#include <errno.h>
#include <stdlib.h>
#ifdef OS2
#include <types.h>
#endif
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xm/Xm.h>
#include "../widgets/FFloat.h"		/* Define XmRDouble, XmRFloat */

/*  To register this converter call XmAddDoubleConverter just after	*
 *  XtInitialize (R3) or XtToolkitInitialize (R4) or the equivalent	*/

#ifndef X11R4

static void StringToDoubleConverter( XrmValue* args, Cardinal* nargs,
				     XrmValue* fromVal, XrmValue* toVal )
{
    static double result;
    char* rest;

    /*  Make sure the number of args is correct  */
    if( *nargs != 0 )
	XtWarning("String to Double conversion needs no arguments.");
    /*  Convert the string in the fromVal to a double  */
    result = strtod((char *)fromVal->addr, &rest);
    if( errno == ERANGE )
	XtStringConversionWarning((char *)fromVal->addr, "Double");
    else
    {
	toVal->size = sizeof(double);
	toVal->addr = (XtPointer)&result;
    }
}

/* ARGSUSED */
static void IntToDoubleConverter( XrmValue* args, Cardinal* nargs,
				  XrmValue* fromVal, XrmValue* toVal )
{
    static double result;

    /*  Make sure the number of args is correct  */
    if( *nargs != 0 )
	XtWarning("Int to Double conversion needs no arguments.");
    /*  Convert the string in the fromVal to a double  */
    result = (double)(*((int *)fromVal->addr));
    toVal->size = sizeof(double);
    toVal->addr = (XtPointer)&result;
}

/* ARGSUSED */
static void DoubleToIntConverter( XrmValue* args, Cardinal* nargs,
				  XrmValue* fromVal, XrmValue* toVal )
{
    static int result;

    /*  Make sure the number of args is correct  */
    if( *nargs != 0 )
	XtWarning("Double to Int conversion needs no arguments.");
    /*  Convert the string in the fromVal to a double  */
    if( (*(double *)fromVal->addr) >= 0.0 )
	result = (int)(*((double *)fromVal->addr) + 0.5);
    else
	result = (int)(*((double *)fromVal->addr) - 0.5);
    toVal->size = sizeof(int);
    toVal->addr = (XtPointer)&result;
}

/* ARGSUSED */
static void StringToFloatConverter( XrmValue* args, Cardinal* nargs,
				     XrmValue* fromVal, XrmValue* toVal )
{
    static float result;

    /*  Make sure the number of args is correct  */
    if( *nargs != 0 )
	XtWarning("String to Float conversion needs no arguments.");
    /*  Convert the string in the fromVal to a floating pt.  */
    if( sscanf((char *)fromVal->addr, "%f", &result) == 1 )
    {
	/*  Make the toVal point to the result.  */
	toVal->size = sizeof(float);
	toVal->addr = (XtPointer)&result;
    }
}

/* ARGSUSED */
static void IntToFloatConverter( XrmValue* args, Cardinal* nargs,
				 XrmValue* fromVal, XrmValue* toVal )
{
    static float result;

    /*  Make sure the number of args is correct  */
    if( *nargs != 0 )
	XtWarning("Int to Float conversion needs no arguments.");
    /*  Convert the int in the fromVal to a float  */
    result = (float)(*((int *)fromVal->addr));
    toVal->size = sizeof(float);
    toVal->addr = (XtPointer)&result;
}

/* ARGSUSED */
static void FloatToIntConverter( XrmValue* args, Cardinal* nargs,
				 XrmValue* fromVal, XrmValue* toVal )
{
    static int result;

    /*  Make sure the number of args is correct  */
    if( *nargs != 0 )
	XtWarning("Int to Float conversion needs no arguments.");
    /*  Convert the float in the fromVal to an int  */
    if( (*((float *)fromVal->addr)) >= 0.0 )
	result = (int)(*((float *)fromVal->addr) + 0.5);
    else
	result = (int)(*((float *)fromVal->addr) - 0.5);
    toVal->size = sizeof(int);
    toVal->addr = (XtPointer)&result;
}

/* ARGSUSED */
static void DoubleToFloatConverter( XrmValue* args, Cardinal* nargs,
				    XrmValue* fromVal, XrmValue* toVal )
{
    static float result;

    /*  Make sure the number of args is correct  */
    if( *nargs != 0 )
	XtWarning("Double to Float conversion needs no arguments.");
    /*  Convert the double in the fromVal to a float  */
    result = (float)(*((double *)fromVal->addr));
    toVal->size = sizeof(float);
    toVal->addr = (XtPointer)&result;
}

/* ARGSUSED */
static void FloatToDoubleConverter( XrmValue* args, Cardinal* nargs,
				    XrmValue* fromVal, XrmValue* toVal )
{
    static double result;

    /*  Make sure the number of args is correct  */
    if( *nargs != 0 )
	XtWarning("Float to Double conversion needs no arguments.");
    /*  Convert the float in the fromVal to a double  */
    result = (double)(*((float *)fromVal->addr));
    toVal->size = sizeof(double);
    toVal->addr = (XtPointer)&result;
}

/* ARGSUSED */
void XmAddFloatConverters()
{
    static int init = 1;
    /*  Do installation only once, but allow multiple calls  */
    if( init )
    {
	XtAddConverter(XmRString, XmRDouble, StringToDoubleConverter,
		       (XtConvertArgList)NULL, (Cardinal)0);
	XtAddConverter(XmRInt, XmRDouble, IntToDoubleConverter,
		       (XtConvertArgList)NULL, (Cardinal)0);
	XtAddConverter(XmRDouble, XmRInt, DoubleToIntConverter,
		       (XtConvertArgList)NULL, (Cardinal)0);
	XtAddConverter(XmRString, XmRFloat, StringToFloatConverter,
		       (XtConvertArgList)NULL, (Cardinal)0);
	XtAddConverter(XmRInt, XmRFloat, IntToFloatConverter,
		       (XtConvertArgList)NULL, (Cardinal)0);
	XtAddConverter(XmRFloat, XmRInt, FloatToIntConverter,
		       (XtConvertArgList)NULL, (Cardinal)0);
	XtAddConverter(XmRDouble, XmRFloat, DoubleToFloatConverter,
		       (XtConvertArgList)NULL, (Cardinal)0);
	XtAddConverter(XmRFloat, XmRDouble, FloatToDoubleConverter,
		       (XtConvertArgList)NULL, (Cardinal)0);
	init = 0;
    }
}

#else

/* ARGSUSED */
static Boolean
  StringToDoubleConverter( Display *dpy, XrmValue* args, Cardinal* nargs,
			   XrmValue* fromVal, XrmValue* toVal,
			   XtPointer* converter_data )
{
    static double result;
    char* rest;

    /*  Make sure the number of args is correct  */
    if( *nargs != 0 )
	XtAppWarningMsg(XtDisplayToApplicationContext(dpy), "stringToDouble",
			"argsGiven", "XtToolkitError",
			"String to Double conversion needs no arguments",
			(String *)NULL, (Cardinal *)NULL);
    /*  Convert the string in the fromVal to a double  */
    if( errno == ERANGE )
    {
	XtDisplayStringConversionWarning(dpy, (char *)fromVal->addr, "Double");
	return False;
    }
    else
    {
	toVal->size = sizeof(double);
	toVal->addr = (XtPointer)&result;
	return True;
    }
}

/* ARGSUSED */
static Boolean
  IntToDoubleConverter( Display *dpy, XrmValue* args, Cardinal* nargs,
		        XrmValue* fromVal, XrmValue* toVal,
		        XtPointer* converter_data )
{
    static double result;

    /*  Make sure the number of args is correct  */
    if( *nargs != 0 )
	XtAppWarningMsg(XtDisplayToApplicationContext(dpy), "intToDouble",
			"argsGiven", "XtToolkitError",
			"Int to Double conversion needs no arguments",
			(String *)NULL, (Cardinal *)NULL);
    /*  Convert the string in the fromVal to a double  */
    result = (double)(*((int *)fromVal->addr));
    toVal->size = sizeof(double);
    toVal->addr = (XtPointer)&result;
    return True;
}

/* ARGSUSED */
static Boolean
  DoubleToIntConverter( Display *dpy, XrmValue* args, Cardinal* nargs,
			  XrmValue* fromVal, XrmValue* toVal,
			  XtPointer* converter_data )
{
    static int result;

    /*  Make sure the number of args is correct  */
    if( *nargs != 0 )
	XtAppWarningMsg(XtDisplayToApplicationContext(dpy), "DoubleToInt",
			"argsGiven", "XtToolkitError",
			"Double to Int conversion needs no arguments",
			(String *)NULL, (Cardinal *)NULL);
    /*  Convert the string in the fromVal to a double  */
    if( (*((double *)fromVal->addr)) >= 0.0 )
	result = (int)(*((double *)fromVal->addr) + 0.5);
    else
	result = (int)(*((double *)fromVal->addr) - 0.5);
    toVal->size = sizeof(int);
    toVal->addr = (XtPointer)&result;
    return True;
}

/* ARGSUSED */
static Boolean
  StringToFloatConverter( Display *dpy, XrmValue* args, Cardinal* nargs,
			   XrmValue* fromVal, XrmValue* toVal,
			   XtPointer* converter_data )
{
    static float result;

    /*  Make sure the number of args is correct  */
    if( *nargs != 0 )
	XtAppWarningMsg(XtDisplayToApplicationContext(dpy), "stringToFloat",
			"argsGiven", "XtToolkitError",
			"String to Float conversion needs no arguments",
			(String *)NULL, (Cardinal *)NULL);
    /*  Convert the string in the fromVal to a floating pt.  */
    if( sscanf((char *)fromVal->addr, "%f", &result) == 1 )
    {
	/*  Make the toVal point to the result.  */
	toVal->size = sizeof(float);
	toVal->addr = (XtPointer)&result;
	return True;
    }
    else
    {
	XtDisplayStringConversionWarning(dpy, (char *)fromVal->addr, "Float");
	return False;
    }
}

/* ARGSUSED */
static Boolean
  IntToFloatConverter( Display *dpy, XrmValue* args, Cardinal* nargs,
		       XrmValue* fromVal, XrmValue* toVal,
		       XtPointer* converter_data )
{
    static float result;

    /*  Make sure the number of args is correct  */
    if( *nargs != 0 )
	XtAppWarningMsg(XtDisplayToApplicationContext(dpy), "intToFloat",
			"argsGiven", "XtToolkitError",
			"Int to Float conversion needs no arguments",
			(String *)NULL, (Cardinal *)NULL);
    /*  Convert the string in the fromVal to a double  */
    result = (float)(*((int *)fromVal->addr));
    toVal->size = sizeof(float);
    toVal->addr = (XtPointer)&result;
    return True;
}

/* ARGSUSED */
static Boolean
  FloatToIntConverter( Display *dpy, XrmValue* args, Cardinal* nargs,
		       XrmValue* fromVal, XrmValue* toVal,
		       XtPointer* converter_data )
{
    static int result;

    /*  Make sure the number of args is correct  */
    if( *nargs != 0 )
	XtAppWarningMsg(XtDisplayToApplicationContext(dpy), "floatToInt",
			"argsGiven", "XtToolkitError",
			"Float to Int conversion needs no arguments",
			(String *)NULL, (Cardinal *)NULL);
    /*  Convert the string in the fromVal to a double  */
    if( (*(float *)fromVal->addr) >= 0.0 )
	result = (int)(*((float *)fromVal->addr) + 0.5);
    else
	result = (int)(*((float *)fromVal->addr) - 0.5);
    toVal->size = sizeof(int);
    toVal->addr = (XtPointer)&result;
    return True;
}

/* ARGSUSED */
static Boolean
  DoubleToFloatConverter( Display *dpy, XrmValue* args, Cardinal* nargs,
			  XrmValue* fromVal, XrmValue* toVal,
			  XtPointer* converter_data )
{
    static float result;

    /*  Make sure the number of args is correct  */
    if( *nargs != 0 )
	XtAppWarningMsg(XtDisplayToApplicationContext(dpy), "doubleToFloat",
			"argsGiven", "XtToolkitError",
			"Double to Float conversion needs no arguments",
			(String *)NULL, (Cardinal *)NULL);
    /*  Convert the string in the fromVal to a double  */
    result = (float)(*((double *)fromVal->addr));
    toVal->size = sizeof(float);
    toVal->addr = (XtPointer)&result;
    return True;
}

/* ARGSUSED */
static Boolean
  FloatToDoubleConverter( Display *dpy, XrmValue* args, Cardinal* nargs,
			  XrmValue* fromVal, XrmValue* toVal,
			  XtPointer* converter_data )
{
    static double result;

    /*  Make sure the number of args is correct  */
    if( *nargs != 0 )
	XtAppWarningMsg(XtDisplayToApplicationContext(dpy), "floatToDouble",
			"argsGiven", "XtToolkitError",
			"Float to Double conversion needs no arguments",
			(String *)NULL, (Cardinal *)NULL);
    /*  Convert the string in the fromVal to a double  */
    result = (double)(*((float *)fromVal->addr));
    toVal->size = sizeof(double);
    toVal->addr = (XtPointer)&result;
    return True;
}


void XmAddFloatConverters()
{
    static int init = 1;
    /*  Do installation only once, but allow multiple calls  */
    if( init )
    {
	XtSetTypeConverter(XmRString, XmRDouble, StringToDoubleConverter,
			   (XtConvertArgList)NULL, (Cardinal)0, XtCacheNone,
			   (XtDestructor)NULL);
	XtSetTypeConverter(XmRInt, XmRDouble, IntToDoubleConverter,
			   (XtConvertArgList)NULL, (Cardinal)0, XtCacheNone,
			   (XtDestructor)NULL);
	XtSetTypeConverter(XmRDouble, XmRInt, DoubleToIntConverter,
			   (XtConvertArgList)NULL, (Cardinal)0, XtCacheNone,
			   (XtDestructor)NULL);
	XtSetTypeConverter(XmRString, XmRFloat, StringToFloatConverter,
			   (XtConvertArgList)NULL, (Cardinal)0, XtCacheNone,
			   (XtDestructor)NULL);
	XtSetTypeConverter(XmRInt, XmRFloat, IntToFloatConverter,
			   (XtConvertArgList)NULL, (Cardinal)0, XtCacheNone,
			   (XtDestructor)NULL);
	XtSetTypeConverter(XmRFloat, XmRInt, FloatToIntConverter,
			   (XtConvertArgList)NULL, (Cardinal)0, XtCacheNone,
			   (XtDestructor)NULL);
	XtSetTypeConverter(XmRDouble, XmRFloat, DoubleToFloatConverter,
			   (XtConvertArgList)NULL, (Cardinal)0, XtCacheNone,
			   (XtDestructor)NULL);
	XtSetTypeConverter(XmRFloat, XmRDouble, FloatToDoubleConverter,
			   (XtConvertArgList)NULL, (Cardinal)0, XtCacheNone,
			   (XtDestructor)NULL);
	init = 0;
    }
}

#endif
