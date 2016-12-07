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

#include <stdio.h>

#include <limits.h>
#include "NumberP.h"
#include "Number.h"

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif


/*
 *  Declare constant values at limits for fitting text in string fields
 */

/*  Limits of 32 bit signed integers  */
#define maxlong INT_MAX
#define minlong INT_MIN

static double emax[] = { 1.0,  10.0,  1e2,   1e3,  1e4, 1e5,  1e6,   1e7, 1e8,
                           1e9,  1e10,  1e11, 1e12, 1e13, 1e14, 1e15, 1e16,
                           1e17, 1e18, 1e19, 1e20, 1e21, 1e22, 1e23, 1e24 };
static double emin[] = { 0.0, -1.0,  -10.0,  -1e2,  -1e3,  -1e4,
                          -1e5, -1e6,   -1e7,  -1e8,  -1e9, -1e10,
                         -1e11, -1e12, -1e13, -1e14, -1e15, -1e16,
                          -1e17, -1e18, -1e19, -1e20, -1e21, -1e22,
                          -1e23, -1e24 };


/*
 *  Declare locally defined and used subroutines
 */

/*  Prepare a string of the value in integer format			      */
static int	IString		(XmNumberWidget nw,
				 double		ival,
				 char*		string,
				 int		width,
				 char		*format);
/*  Prepare a string of the value in fixed point format			      */
static int	FString		(XmNumberWidget nw,
				 double		fval,
				 char*		string,
				 int		width,
				 int		points,
				 char*		format);
/*  Prepare a string of the value in whatever format shows optimal precision  */
static int	GString		(XmNumberWidget nw, double gval, char* string, int width);
/*  Prepare a string with an indication that the value cannot be represented  */
static void	MarkOverflow	(char* string,
				 int string_width,
				 char* label,
				 int label_width);


/*  Subroutine:	XmChangeNumberValue
 *  Purpose:	Input a new value and update the display (don't bother with
 *		a full-blown SetValues).
 */
void XmChangeNumberValue( XmNumberWidget nw, double value )
{
    /*  Update all three value types  */
    nw->number.value.d = value;
    if ( (nw->number.data_type == INTEGER) || (nw->number.data_type == FLOAT) )
	nw->number.value.f = (float)value;
    if (nw->number.data_type == INTEGER)
    {
	if( value >= 0 )
	    nw->number.value.i = (int)(value + 0.5);
	else
	    nw->number.value.i = (int)(value - 0.5);
    }
    XmShowNumberValue(nw);
}


/*  Subroutine:	XmShowNumberValue
 *  Effect:	Draws the number string on the screen
 */
void XmShowNumberValue( XmNumberWidget nw )
{
    int pad, char_cnt;
    char string[32];
    GC   gc;

    nw->number.visible = True;
    if( !XtIsRealized((Widget)nw) )
	return;
    if( nw->number.decimal_places == 0 )
	pad = IString(nw, nw->number.value.d, string,
		      nw->number.char_places, nw->number.format);
    else if( nw->editor.is_fixed )
	pad = FString(nw, nw->number.value.d, string, nw->number.char_places,
		      nw->number.decimal_places, nw->number.format);
    else
	pad = GString(nw, nw->number.value.d, string, nw->number.char_places);
    /*  Check for error return  */
    if( pad < 0 )
    {
	/*  Negative indicates display not possible in requested format  */
	if( pad == -1 )
	    /*  -1 returned when format successfully switched to G  */
	    pad = 0;
	else
	    /*  <-1 is -(number of chars in error display: e.g. "ovfl")  */
	    pad += nw->number.char_places;
    }
    char_cnt = nw->number.char_places - pad;
    XClearArea(XtDisplay(nw), XtWindow(nw),
		   nw->number.x, nw->number.y,
		   nw->number.width + nw->number.h_margin, 
		   nw->number.height, False);
    /*  Calculate the text position x coordinate and print  */
    if (nw->core.sensitive && nw->core.ancestor_sensitive)
    {
	gc = nw->number.gc;
    }
    else
    {
	gc = nw->number.inv_gc;
    }
    if( nw->number.center && (pad > 0) )
	XDrawImageString(XtDisplay(nw), XtWindow(nw), gc,
			 nw->number.x + ((pad * nw->number.char_width) / 2),
			 nw->number.y + nw->number.font_struct->ascent + 1,
			 string + pad, nw->number.char_places - pad);
    else
	XDrawImageString(XtDisplay(nw), XtWindow(nw), gc,
			 nw->number.x,
			 nw->number.y + nw->number.font_struct->ascent + 1,
			 string, nw->number.char_places);
    nw->number.last_char_cnt = char_cnt;
}


/*  Subroutine:	HideNumberValue
 *  Effect:	Makes value display dissappear by clearing to background color
 */
void XmHideNumberValue( XmNumberWidget nw )
{
    nw->number.visible = False;
    XClearArea(XtDisplay(nw), XtWindow(nw), nw->number.x, nw->number.y,
	       nw->number.width + nw->number.h_margin, 
	       nw->number.height, False);
}


/*  Subroutine:	SetNumberSize
 *  Purpose:	Check and adjust all sizes for consistency with value range
 *		and set dimension and format paramters.
 */
void SetNumberSize( XmNumberWidget nw, Dimension* width, Dimension* height )
{
register int max_chars, min_chars;

    if( !nw->number.font_struct )
    {
	nw->number.width = 0;
	nw->number.height = 0;
	*width = 0;
	*height = 0;
	return;
    }
    /*  Figure out how many places must be represented and make sure	*
     *  we have enough space						*/
    if( (!nw->editor.is_fixed) && 
	(nw->number.recompute_size) &&
	(nw->number.decimal_places < 0) )
    {
	/*  Specified equivalent of %g notation (best fit floating format)  */
	if( nw->number.value_minimum.d <= -1e10 )
		nw->number.char_places = 9;
	else if( (nw->number.value_minimum.d < 0.0) ||
		     (nw->number.value_maximum.d >= 1e10) )
		nw->number.char_places = 8;
	else if( nw->number.char_places < 7 )
		nw->number.char_places = 7;
    }
    else if( (!nw->editor.is_fixed) && 
             (nw->number.recompute_size) &&
             (nw->number.decimal_places >= 0) )
    {
        if( nw->number.value_minimum.d <= -1e10 )
                nw->number.char_places = nw->number.decimal_places + 8;
        else if( (nw->number.value_minimum.d < 0.0) ||
                     (nw->number.value_maximum.d >= 1e10) )
                nw->number.char_places = nw->number.decimal_places + 7;
        else nw->number.char_places = nw->number.decimal_places + 6;
    }
    else
    {
	if (nw->number.recompute_size)
	{
	    /*  Find the number of digits occupied by the two extremes  */
	    max_chars = 1;
	    while (TRUE)
	        {
	        if (emax[max_chars-1] > nw->number.value_maximum.d)
		    break;
	        max_chars++;
	        }
	    max_chars--;
	    min_chars = 1;
	    while (TRUE)
	        {
	        if (emin[min_chars-1] < nw->number.value_minimum.d)
		    break;
	        min_chars++;
	        }
	    min_chars--;
	    /*  Characters needed is value + decimal places, + dot (if used)  */
	    if( min_chars > max_chars )
	        max_chars = min_chars;
	    if( nw->number.decimal_places > 0 )
	        max_chars += (1 + nw->number.decimal_places);
	    /*If application did not specify enough places, use minimum needed*/
	    nw->number.char_places = MAX(3,max_chars);
	}
    }
    /*  Check reasonableness of decimal places specification  */
    if( nw->number.decimal_places > 0 )
    {
        if (nw->number.char_places < (nw->number.decimal_places + 3))
	    nw->number.char_places = nw->number.decimal_places + 3;
	(void)sprintf(nw->number.format, "%%%1d.%1df",
		      nw->number.char_places, nw->number.decimal_places);
    }
    else if( nw->number.decimal_places == 0 )
	(void)sprintf(nw->number.format, "%%%1dd", nw->number.char_places );
    /*  Figure out the sizes of things  */
    nw->number.char_width = XTextWidth(nw->number.font_struct, "5", 1);
    nw->number.dot_width = XTextWidth(nw->number.font_struct, ".", 1);
    if( nw->number.char_width != nw->number.dot_width )
	nw->number.proportional = True;
    nw->number.width = nw->number.char_places * nw->number.char_width;
    /*  Height is that of the font's bitmap */
    nw->number.height =
      nw->number.font_struct->ascent + nw->number.font_struct->descent + 2;
    nw->number.v_margin = 1;
    nw->number.h_margin = (nw->number.char_width)/2;
    {
	register int offset;

	/*  Compute overall rectangle size resulting following parameters  */
	offset = nw->primitive.highlight_thickness +
	  nw->primitive.shadow_thickness + nw->number.h_margin;
	*width = nw->number.width + offset + offset;
	offset = nw->primitive.highlight_thickness +
	  nw->primitive.shadow_thickness + nw->number.v_margin;
	*height = 1 + nw->number.height + offset + offset;
    }
}


/*  Subroutine:	IString
 *  Purpose:	Create an integer string, include clipping mark if appropriate
 *  Returns:	-4=ovfl, -1=Gformat, else the number of leading blank spaces.
 *  Note:	to be called with full column width
 *  Note:	2<=width<=10
 */
static int IString( XmNumberWidget nw, double ival, char* string, int width, char *format )
#ifdef EXPLAIN
     double ival;	/* Value to be printed				 */
     char* string;	/* The output (ival as string)			 */
     int width;		/* The number of character places in the output	 */
     char* format;	/* String with default %d format		 */
#endif
{
    int blank;

    if( ival > 0 ) {
	if( (ival >= emax[width]) || (ival > maxlong) ) {
	    if( (width >= 6) || (ival < emax[width]) ||
		((ival < emax[10]) && (width >= 5)) )
	    {
		GString(nw, (double)ival, string, width);
		return -1;
	    } else {
		MarkOverflow(string, width, "ovfl", 4);
		return -4;
	    }
	}
    }
    else if( (ival <= emin[width]) || (ival < minlong) ) {
	if( (width >= 7) || (ival > emin[width]) ||
	    ((ival > emin[9]) && (width >= 6)) )
	{
	    GString(nw, (double)ival, string, width);
	    return -1;
	} else {
	    MarkOverflow(string, width, "ovfl", 4);
	    return -4;
	}
    }
    if( ival < 0.0 )
	(void)sprintf(string, format, (long)(ival - 0.5));
    else
	(void)sprintf(string, format, (long)(ival + 0.5));
    for( blank=0; string[blank] == ' '; blank++ );
    return blank;
}


/*  Subroutine:	FString
 *  Effect:	Place string of the value in fixed point format.  If the number
 *		cannot be properly represented, fall back to G format if
 *		possible, else represent an error state (ovfl).
 *  Returns:	-4=ovfl, -1=Gformat, else the number of leading blank spaces.
 */
static int FString( XmNumberWidget nw, double fval, char* string,
		     int width, int points, char* format )
#ifdef EXPLAIN
     double fval;	/* Value to be printed				 */
     char *string;	/* The output (ival as string)			 */
     int width;		/* The number of character places in the output	 */
#endif
{
    int blank, size;

    size = width - (points - 1);
    if( fval > 0 )
    {
	if( fval >= emax[size] )
	{
	    if( (width >= 6) || (fval < emax[width]) ||
		((fval < emax[10]) && (width >= 5)) )
	    {
		GString(nw, fval, string, width);
		return -1;
	    }
	    else
	    {
		MarkOverflow(string, width, "ovfl", 4);
		return -4;
	    }
	}
    }
    else if( fval <= emin[size] )
    {
	if( (width >= 7) || (fval > emin[width]) ||
	    ((fval > emin[9]) && (width >= 6)) )
	{
	    GString(nw, fval, string, width);
	    return -1;
	}
	else
	{
	    MarkOverflow(string, width, "ovfl", 4);
	    return -4;
	}
    }
    (void)sprintf(string, format, fval);
    string[width] = '\0';
    for( blank=0; string[blank] == ' '; blank++ );
    return blank;
}


static char *fform[] = { "%.0f", "%.1f", "%.2f", "%.3f", "%.4f", "%.5f",
			     "%.6f", "%.7f", "%.8f", "%.9f", "%.10f",
			     "%.11f", "%.12f", "%.13f", "%.14f", "%.15f",
			     "%.16f", "%.17f", "%.18f", "%.19f", "%.20f",
			     "%.21f", "%.22f", "%.23f", "%.24f", "%.25f" };
static char *eform[] = { "%.0e", "%.1e", "%.2e", "%.3e", "%.4e", "%.5e",
			     "%.6e", "%.7e", "%.8e", "%.9e", "%.10e",
			     "%.11e", "%.12e", "%.13e", "%.14e", "%.15e",
			     "%.16e", "%.17e", "%.18e", "%.19e", "%.20e",
			     "%.21e", "%.22e", "%.23e", "%.24e", "%.25e" };
/*  Subroutine:	GString
 *  Purpose:	Print the real value in best format for character space given
 *  Returns:	0 (no leading space).  Returning a value is for compatability.
 *  Note:	8<=width<=15 (7 makes -1e-10\0, 16 goes beyond lookup arrays)
 *  Note:	width characters beyond string[width] may get messed up
 *  Method:	For speed, compares and formats get values through lookups
 */
static int GString( XmNumberWidget nw, double gval, char* string, int width )
#ifdef EXPLAIN
     double gval;	/* value to print				*/
     char *string;	/* string to recieve number (see note 2 above)	*/
     int width;		/* i: number of characters to use		*/
#endif
{
int len;
int pad;
int i;
char format[128];
double epsilon;

    if (gval == 0.0)
    {
	if (nw->number.decimal_places < 0)
	{
	    (void)strcpy(string, "0.0");
	}
	else
	{
	    (void)sprintf(format, "%%.%df", nw->number.decimal_places);
	    (void)sprintf(string, format, gval);
	}
	pad = 0;
	len = strlen(string);
	if (len < width)
	{
	    pad = width - len;
	    for (i = 0; i < len+1; i++)
	    {
		string[(len - i) + pad] = string[len - i];
	    }
	    for (i = 0; i < pad; i++)
	    {
		string[i] = ' ';
	    }
	}
	string[width] = '\0';
	return (pad);
    }
    epsilon = nw->number.cross_under * nw->number.cross_under;
    if( gval >= 0.0 )
    {
	if((gval + epsilon < nw->number.cross_under) || 
	   (gval >= nw->number.cross_over ))
	{
	    /*  Outside range for simple %f format  */
	    if ((nw->editor.is_fixed == False) && 	
		(nw->number.decimal_places >= 0))
	    {
		(void)sprintf(string, eform[nw->number.decimal_places], gval);
		pad = 0;
		len = strlen(string);
		if (len < width)
		{
		    pad = width - len;
		    for (i = 0; i < len+1; i++)
		    {
			string[(len - i) + pad] = string[len - i];
		    }
		    for (i = 0; i < pad; i++)
		    {
			string[i] = ' ';
		    }
		}
		string[width] = '\0';
		return (pad);
	    }
	    else
	    {
		if( (gval >= 1e-9) && (gval < 1e10) )
		{
		    /* Inside range for 1 digit exponent (move digit 2 over) */
		    if( gval >= 1.0 )
		    {
			/*  Positive number with 1 digit positive exponent  */
			(void)sprintf(string, eform[width-4], gval);
			string[width-1] = string[width+1];
		    }
		    else
		    {
			/*  Positive number with 1 digit negative exponent  */
			(void)sprintf(string, eform[width-5], gval);
			string[width-1] = string[width];
		    }
		}
		else if( gval >= 1.0 )
		{
		    /*  Positive number with 2 digit positive exponent  */
		    (void)sprintf(string, eform[width-5], gval);
		    string[width-2] = string[width-1];
		    string[width-1] = string[width];
		}
		else
		    /*  Positive number with 2 digit negative exponent  */
		    (void)sprintf(string, eform[width-6], gval);
	    }
	}
	else
	    /*  Positive number in fixed point notation  */
	    if (nw->number.decimal_places >= 0)
	    {
		(void)sprintf(string, fform[nw->number.decimal_places], gval);
		pad = 0;
		len = strlen(string);
		if (len < width)
		{
		    pad = width - len;
		    for (i = 0; i < len+1; i++)
		    {
			string[(len - i) + pad] = string[len - i];
		    }
		    for (i = 0; i < pad; i++)
		    {
			string[i] = ' ';
		    }
		}
		string[width] = '\0';
		return (pad);
	    }
	    else
		(void)sprintf(string, fform[width], gval);
    }
    else
    {
	if((gval - epsilon > -nw->number.cross_under) || 
	   (gval <= -nw->number.cross_over))
	{
	    if ((nw->editor.is_fixed == False) && 	
		(nw->number.decimal_places >= 0))
	    {
		(void)sprintf(string, eform[nw->number.decimal_places], gval);
		pad = 0;
		len = strlen(string);
		if (len < width)
		{
		    pad = width - len;
		    for (i = 0; i < len+1; i++)
		    {
			string[(len - i) + pad] = string[len - i];
		    }
		    for (i = 0; i < pad; i++)
		    {
			string[i] = ' ';
		    }
		}
		string[width] = '\0';
		return (pad);
	    }
	    else
	    {
		if( (gval <= -1e-99) && (gval > -1e100) )
		{
		    /* Inside range for 1 digit exponent (move digit 2 over)  */
		    if( gval > -1.0 )
		    {
			/*  Negative number with 1 digit negative exponent  */
			(void)sprintf(string, eform[width-6], gval);
			string[width-1] = string[width];
		    }
		    else
		    {
			/*  Negative number with 1 digit positive exponent  */
			(void)sprintf(string, eform[width-5], gval);
			string[width-1] = string[width+1];
		    }
		}
		if( (gval <= -1e-9) && (gval > -1e10) )
		{
		    /*  Inside range for 1 digit exponent (move digit 2 over) */
		    if( gval > -1.0 )
		    {
			/*  Negative number with 1 digit negative exponent  */
			(void)sprintf(string, eform[width-6], gval);
			string[width-1] = string[width];
		    }
		    else
		    {
			/*  Negative number with 1 digit positive exponent  */
			(void)sprintf(string, eform[width-5], gval);
			string[width-1] = string[width+1];
		    }
		}
		else if( gval <= -1.0 )
		{
		    /*  Negative number with 2 digit positive exponent  */
		    (void)sprintf(string, eform[width-6], gval);
		    string[width-2] = string[width-1];
		    string[width-1] = string[width];
		}
		else
		    /*  Negative number with 2 digit negative exponent  */
		    (void)sprintf(string, eform[width-7], gval);
	    }
	}
	else
	    /*  Negative number in fixed point notation  */
	    if (nw->number.decimal_places >= 0)
	    {
		(void)sprintf(string, fform[nw->number.decimal_places], gval);
		pad = 0;
		len = strlen(string);
		if (len < width)
		{
		    pad = width - len;
		    for (i = 0; i < len+1; i++)
		    {
			string[(len - i) + pad] = string[len - i];
		    }
		    for (i = 0; i < pad; i++)
		    {
			string[i] = ' ';
		    }
		}
		string[width] = '\0';
		return (pad);
	    }
	    else
		(void)sprintf(string, fform[width], gval);
    }
    string[width] = '\0';
    return 0;
}


/*  Subroutine:	MarkOverflow
 *  Effect:	Copies message of label, right justified onto string
 */
static void MarkOverflow( char* string, int string_width,
			  char* label, int label_width )
#ifdef EXPLAIN
     char* string;
     int string_width;
     char* label;
     int label_width;
#endif
{
    int i, j;

    for( i=label_width-1, j=string_width-1; i>=0; i--, j-- )
	string[j] = label[i];
    for( ; j>=0; j-- )
    string[j] = ' ';
    string[string_width] = '\0';
}

#if defined(__cplusplus) || defined(c_plusplus)
 }
#endif
