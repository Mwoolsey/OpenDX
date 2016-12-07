/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/*********************************************************************/
/*                     I.B.M. CONFIENTIAL                           */
/*********************************************************************/

#include <dxconfig.h>


/******************************************************************************
 * These functions were provided by HP with /usr/lib/starbase/demos/SBUTILS
 *
 * This file contains a set of example utility procedures; procedures that can
 * help a "window-smart" Starbase or PHIGS program determine information about
 * a device, and create image and overlay plane windows.  To use these
 * utilities, #include "wsutils.h" and compile this file and link the results
 * with your program.
 *
 ******************************************************************************/
/*
 $Header: /src/master/dx/src/exec/hwrender/starbase/wsutils.c,v 1.3 1999/05/10 15:45:39 gda Exp $
*/

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/XHPlib.h>
#include <stdio.h>
#include "hwDeclarations.h"
#include "wsutils.h"
#include "hwInteractor.h"

#ifndef STANDALONE
#include "hwMemory.h"
#endif

#include "hwPortLayer.h"

#include "hwDebug.h"

#define STATIC_GRAY	0x01
#define GRAY_SCALE	0x02
#define PSEUDO_COLOR	0x04
#define TRUE_COLOR	0x10
#define DIRECT_COLOR	0x11

/******************************************************************************
 *
 * GetXVisualInfo()
 *
 * This routine takes an X11 Display, screen number, and returns whether the
 * screen supports transparent overlays and three arrays:
 *
 *	1) All of the XVisualInfo struct's for the screen.
 *	2) All of the OverlayInfo struct's for the screen.
 *	3) An array of pointers to the screen's image plane XVisualInfo
 *	   structs.
 *
 * The code below obtains the array of all the screen's visuals, and obtains
 * the array of all the screen's overlay visual information.  It then processes
 * the array of the screen's visuals, determining whether the visual is an
 * overlay or image visual.
 *
 * If the routine sucessfully obtained the visual information, it returns zero.
 * If the routine didn't obtain the visual information, it returns non-zero.
 *
 ******************************************************************************/

int GetXVisualInfo(Display *display, int screen, 
	int *transparentOverlays, int *numVisuals, XVisualInfo **pVisuals,
	int *numOverlayVisuals, OverlayInfo **pOverlayVisuals,
	int *numImageVisuals, XVisualInfo ***pImageVisuals)

{
    XVisualInfo	getVisInfo;		/* Paramters of XGetVisualInfo */
    int		mask;
    XVisualInfo	*pVis, **pIVis;		/* Faster, local copies */
    OverlayInfo	*pOVis;
    OverlayVisualPropertyRec	*pOOldVis;
    int		nVisuals, nOVisuals, nIVisuals;
    Atom	overlayVisualsAtom;	/* Parameters for XGetWindowProperty */
    Atom	actualType;
    unsigned long numLongs, bytesAfter;
    int		actualFormat;
    int		nImageVisualsAlloced;	/* Values to process the XVisualInfo */
    int		imageVisual;		/* array */


    ENTRY(("GetXVisualInfo(0x%x, %d, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x)",
	   display, screen, transparentOverlays, numVisuals, pVisuals,
	   numOverlayVisuals, pOverlayVisuals, numImageVisuals, pImageVisuals));

    /* First, get the list of visuals for this screen. */
    getVisInfo.screen = screen;
    mask = VisualScreenMask; 

    *pVisuals = XGetVisualInfo(display, mask, &getVisInfo, numVisuals);
    if ((nVisuals = *numVisuals) <= 0)
    {
      /* Return that the information wasn't sucessfully obtained: */
      EXIT(("1"));
      return(1);
    }
    pVis = *pVisuals;


    /* Now, get the overlay visual information for this screen.  To obtain
     * this information, get the SERVER_OVERLAY_VISUALS property.
     */
    overlayVisualsAtom = XInternAtom(display, "SERVER_OVERLAY_VISUALS", True);
    if (overlayVisualsAtom != None)
    {
	/* Since the Atom exists, we can request the property's contents.  The
	 * do-while loop makes sure we get the entire list from the X server.
	 */
	bytesAfter = 0;
	numLongs = sizeof(OverlayVisualPropertyRec) / 4;
	do
	{
	    numLongs += bytesAfter * 4;
	    XGetWindowProperty(display, RootWindow(display, screen),
		overlayVisualsAtom, 0, numLongs, False,
		overlayVisualsAtom, &actualType, &actualFormat,
		&numLongs, &bytesAfter, (unsigned char **)pOverlayVisuals);
	} while (bytesAfter > 0);


	/* Calculate the number of overlay visuals in the list. */
	*numOverlayVisuals = numLongs / (sizeof(OverlayVisualPropertyRec) / 4);
    }
    else
    {
	/* This screen doesn't have overlay planes. */
	*numOverlayVisuals = 0;
	*pOverlayVisuals = NULL;
	*transparentOverlays = 0;
    }


    /* Process the pVisuals array. */
    *numImageVisuals = 0;
    nImageVisualsAlloced = 1;
    pIVis = *pImageVisuals = (XVisualInfo **) tdmAllocateLocal(4);

    while (--nVisuals >= 0)
    {
	nOVisuals = *numOverlayVisuals;
	pOVis = *pOverlayVisuals;
	imageVisual = True;
	while (--nOVisuals >= 0)
	{
	    pOOldVis = (OverlayVisualPropertyRec *) pOVis;
	    if (pVis->visualid == pOOldVis->visualID)
	    {
		imageVisual = False;
		pOVis->pOverlayVisualInfo = pVis;
		if (pOVis->transparentType == TransparentPixel)
		    *transparentOverlays = 1;
	    }
	    pOVis++;
	}
	if (imageVisual)
	{
	    if ((*numImageVisuals += 1) > nImageVisualsAlloced)
	    {
	      nImageVisualsAlloced++ ;
	      *pImageVisuals = (XVisualInfo **)
		tdmReAllocate(*pImageVisuals, nImageVisualsAlloced * 4) ;
	      pIVis = *pImageVisuals + (*numImageVisuals - 1) ;
	    }
	    *pIVis++ = pVis;
	}
	pVis++;
    }


    /* Return that the information was sucessfully obtained: */
    EXIT(("0"));
    return(0);
} /* GetXVisualInfo() */

/******************************************************************************
 *
 * FindImagePlanesVisual()
 *
 * This routine attempts to find a visual to use to create an image planes
 * window based upon the information passed in.
 *
 * The "Hint" values give guides to the routine as to what the program wants.
 * The "depthFlexibility" value tells the routine how much the program wants
 * the actual "depthHint" specified.  If the program can't live with the
 * screen's image planes visuals, the routine returns non-zero, and the
 * "depthObtained" and "pImageVisualToUse" return parameters are NOT valid.
 * Otherwise, the "depthObtained" and "pImageVisualToUse" return parameters
 * are valid and the routine returns zero.
 *
 * NOTE: This is just an example of what can be done.  It may or may not be
 * useful for any specific application.
 *
 ******************************************************************************/

int FindImagePlanesVisual(Display *display, int screen, int numImageVisuals, 
	XVisualInfo **pImageVisuals, int sbCmapHint, int depthHint,
	int depthFlexibility, Visual **pImageVisualToUse, int *depthObtained)
{
    XVisualInfo	*pVisualInfoToUse;	/* The current best visual to use. */
    int		i;
    int		visMask;
    int		curDelta, newDelta;

    ENTRY(("FindImagePlanesVisual(0x%x, %d, %d, 0x%x, %d, %d, %d, 0x%x, 0x%x)",
	   display, screen, numImageVisuals, pImageVisuals, sbCmapHint,
	   depthHint, depthFlexibility, pImageVisualToUse, depthObtained));

    switch (sbCmapHint)
    {
	case SB_CMAP_TYPE_NORMAL:
	case SB_CMAP_TYPE_NORMAL | SB_CMAP_TYPE_MONOTONIC:
	case SB_CMAP_TYPE_NORMAL | SB_CMAP_TYPE_MONOTONIC | SB_CMAP_TYPE_FULL:
	case SB_CMAP_TYPE_NORMAL | SB_CMAP_TYPE_FULL:
	    visMask = GRAY_SCALE | PSEUDO_COLOR;
	    break;
	case SB_CMAP_TYPE_MONOTONIC:
	    visMask = STATIC_GRAY | GRAY_SCALE | PSEUDO_COLOR;
	    break;
	case SB_CMAP_TYPE_MONOTONIC | SB_CMAP_TYPE_FULL:
	    visMask = GRAY_SCALE | PSEUDO_COLOR;
	    break;
	case SB_CMAP_TYPE_FULL:
	    visMask = GRAY_SCALE | PSEUDO_COLOR | TRUE_COLOR | DIRECT_COLOR;
	    break;
	default:
	    /* The caller didn't specify a valid combination of CMAP_ type: */
	    EXIT(("1"));
	    return(1);
    } /* switch (sbCmapHint) */


    pVisualInfoToUse = NULL;

    for (i = 0 ; i < numImageVisuals ; i++)
    {
	switch (pImageVisuals[i]->class)
	{
	    case StaticGray:
		if (visMask & STATIC_GRAY)
		{
		    if (pVisualInfoToUse == NULL)
		    {
			if ((pImageVisuals[i]->depth == depthHint) ||
			    depthFlexibility)
			{
			    pVisualInfoToUse = pImageVisuals[i];
			}
		    }
		    else
		    {
			if (pImageVisuals[i]->depth == depthHint)
			    pVisualInfoToUse = pImageVisuals[i];
			else if (depthFlexibility)
			{
			    /* The basic hueristic used is to find the closest
			     * depth to "depthHint" that's also greater than
			     * "depthHint", or just the closest depth:
			     */
			    if ((curDelta = pVisualInfoToUse->depth -
				 depthHint) > 0)
			    {
				/* Only choose this new visual if it's also
				 * deeper than "depthHint" and closer to
				 * "depthHint" than the currently chosen visual:
				 */
				if (((newDelta = pImageVisuals[i]->depth -
				      depthHint) > 0) && (newDelta < curDelta))
				{
				    pVisualInfoToUse = pImageVisuals[i];
				}
			    }
			    else
			    {
				/* Choose this new visual if it's deeper than
				 * "depthHint" or closer to "depthHint" than
				 * the currently chosen visual:
				 */
				if (((newDelta = pImageVisuals[i]->depth -
				      depthHint) > 0) ||
				    (-newDelta < -curDelta))
				{
				    pVisualInfoToUse = pImageVisuals[i];
				}
			    }
			}
		    }
		}
		break;
	    case GrayScale:
		if (visMask & GRAY_SCALE)
		{
		    if (pVisualInfoToUse == NULL)
		    {
			if ((pImageVisuals[i]->depth == depthHint) ||
			    depthFlexibility)
			{
			    pVisualInfoToUse = pImageVisuals[i];
			}
		    }
		    else if (!((sbCmapHint & SB_CMAP_TYPE_FULL) &&
			       (pVisualInfoToUse->class == DirectColor)))
		    {
			if (pImageVisuals[i]->depth == depthHint)
			    pVisualInfoToUse = pImageVisuals[i];
			else if (depthFlexibility)
			{
			    /* The basic hueristic used is to find the closest
			     * depth to "depthHint" that's also greater than
			     * "depthHint", or just the closest depth:
			     */
			    if ((curDelta = pVisualInfoToUse->depth -
				 depthHint) > 0)
			    {
				/* Only choose this new visual if it's also
				 * deeper than "depthHint" and closer to
				 * "depthHint" than the currently chosen visual:
				 */
				if (((newDelta = pImageVisuals[i]->depth -
				      depthHint) > 0) && (newDelta < curDelta))
				{
				    pVisualInfoToUse = pImageVisuals[i];
				}
			    }
			    else
			    {
				/* Choose this new visual if it's deeper than
				 * "depthHint" or closer to "depthHint" than
				 * the currently chosen visual:
				 */
				if (((newDelta = pImageVisuals[i]->depth -
				      depthHint) > 0) ||
				    (-newDelta < -curDelta))
				{
				    pVisualInfoToUse = pImageVisuals[i];
				}
			    }
			}
		    }
		}
		break;
	    case PseudoColor:
		if (visMask & PSEUDO_COLOR)
		{
		    if (pVisualInfoToUse == NULL)
		    {
			if ((pImageVisuals[i]->depth == depthHint) ||
			    depthFlexibility)
			{
			    pVisualInfoToUse = pImageVisuals[i];
			}
		    }
		    else if (!((sbCmapHint & SB_CMAP_TYPE_FULL) &&
			       (pVisualInfoToUse->class == DirectColor)))
		    {
			if (pImageVisuals[i]->depth == depthHint)
			    pVisualInfoToUse = pImageVisuals[i];
			else if (depthFlexibility)
			{
			    /* The basic hueristic used is to find the closest
			     * depth to "depthHint" that's also greater than
			     * "depthHint", or just the closest depth:
			     */
			    if ((curDelta = pVisualInfoToUse->depth -
				 depthHint) > 0)
			    {
				/* Only choose this new visual if it's also
				 * deeper than "depthHint" and closer to
				 * "depthHint" than the currently chosen visual:
				 */
				if (((newDelta = pImageVisuals[i]->depth -
				      depthHint) > 0) && (newDelta < curDelta))
				{
				    pVisualInfoToUse = pImageVisuals[i];
				}
			    }
			    else
			    {
				/* Choose this new visual if it's deeper than
				 * "depthHint" or closer to "depthHint" than
				 * the currently chosen visual:
				 */
				if (((newDelta = pImageVisuals[i]->depth -
				      depthHint) > 0) ||
				    (-newDelta < -curDelta))
				{
				    pVisualInfoToUse = pImageVisuals[i];
				}
			    }
			}
		    }
		}
		break;
	    case StaticColor:
		/* Starbase doesn't work well with StaticColor visuals: */
		break;
	    case TrueColor:
		if (visMask & TRUE_COLOR)
		{
		    /* The only Starbase cmap type that TrueColor works with
		     * is SB_CMAP_TYPE_FULL, so we know that SB_CMAP_TYPE_FULL
		     * is what the program wants:
		     */
		    if (pVisualInfoToUse == NULL)
		    {
			if ((pImageVisuals[i]->depth == depthHint) ||
			    depthFlexibility)
			{
			    pVisualInfoToUse = pImageVisuals[i];
			}
		    }
		    /* This example code prefers DirectColor to TrueColor: */
		    else if (pVisualInfoToUse->class != DirectColor)
		    {
			if (pImageVisuals[i]->depth == depthHint)
			    pVisualInfoToUse = pImageVisuals[i];
			else if (depthFlexibility)
			{
			    /* The basic hueristic used is to find the closest
			     * depth to "depthHint" that's also greater than
			     * "depthHint", or just the closest depth:
			     */
			    if ((curDelta = pVisualInfoToUse->depth -
				 depthHint) > 0)
			    {
				/* Only choose this new visual if it's also
				 * deeper than "depthHint" and closer to
				 * "depthHint" than the currently chosen visual:
				 */
				if (((newDelta = pImageVisuals[i]->depth -
				      depthHint) > 0) && (newDelta < curDelta))
				{
				    pVisualInfoToUse = pImageVisuals[i];
				}
			    }
			    else
			    {
				/* Choose this new visual if it's deeper than
				 * "depthHint" or closer to "depthHint" than
				 * the currently chosen visual:
				 */
				if (((newDelta = pImageVisuals[i]->depth -
				      depthHint) > 0) ||
				    (-newDelta < -curDelta))
				{
				    pVisualInfoToUse = pImageVisuals[i];
				}
			    }
			}
		    }
		}
		break;
	    case DirectColor:
		if (visMask & DIRECT_COLOR)
		{
		    if (pVisualInfoToUse == NULL)
		    {
			if ((pImageVisuals[i]->depth == depthHint) ||
			    depthFlexibility)
			{
			    pVisualInfoToUse = pImageVisuals[i];
			}
		    }
		    else
		    {
			if (pImageVisuals[i]->depth == depthHint)
			    pVisualInfoToUse = pImageVisuals[i];
			else if (depthFlexibility)
			{
			    /* The basic hueristic used is to find the closest
			     * depth to "depthHint" that's also greater than
			     * "depthHint", or just the closest depth:
			     */
			    if ((curDelta = pVisualInfoToUse->depth -
				 depthHint) > 0)
			    {
				/* Only choose this new visual if it's also
				 * deeper than "depthHint" and closer to
				 * "depthHint" than the currently chosen visual:
				 */
				if (((newDelta = pImageVisuals[i]->depth -
				      depthHint) > 0) && (newDelta < curDelta))
				{
				    pVisualInfoToUse = pImageVisuals[i];
				}
			    }
			    else
			    {
				/* Choose this new visual if it's deeper than
				 * "depthHint" or closer to "depthHint" than
				 * the currently chosen visual:
				 */
				if (((newDelta = pImageVisuals[i]->depth -
				      depthHint) > 0) ||
				    (-newDelta < -curDelta))
				{
				    pVisualInfoToUse = pImageVisuals[i];
				}
			    }
			}
		    }
		}
		break;
	} /* switch (pImageVisuals[i]->class) */
    } /* for (i = 0 ; i < numImageVisuals ; i++) */


    if (pVisualInfoToUse != NULL)
    {
	*pImageVisualToUse = pVisualInfoToUse->visual;
	*depthObtained = pVisualInfoToUse->depth;
	EXIT(("0"));
	return(0);
    }
    else
    {
	/* Couldn't find an appropriate visual class: */
      EXIT(("1"));
      return(1);
    }
} /* FindImagePlanesVisual() */

/******************************************************************************
 *
 * CreateImagePlanesWindow()
 *
 * This routine creates an image planes window, potentially creates a colormap
 * for the window to use, and sets the window's standard properties, based
 * upon the information passed in to the routine.  While "created," the window
 * has not been mapped.
 *
 * If the routine suceeds, it returns zero and the return parameters
 * "imageWindow", "imageColormap" and "mustFreeImageColormap" are valid.
 * Otherwise, the routine returns non-zero and the return parameters are
 * NOT valid.
 *
 * NOTE: This is just an example of what can be done.  It may or may not be
 * useful for any specific application.
 *
 ******************************************************************************/

int CreateImagePlanesWindow(Display *display, int screen, Window parentWindow,
	int windowX, int windowY, int windowWidth, int windowHeight,
	int windowDepth, Visual *pImageVisualToUse,
	int argc, char *argv[], char *windowName, char *iconName,
	Window *imageWindow, Colormap *imageColormap, 
	int *mustFreeImageColormap)
{
    XSetWindowAttributes winAttributes;	/* Attributes for window creation */
    XSizeHints	hints;

    ENTRY(("CreateImagePlanesWindow(0x%x, %d, 0x%x, %d, %d, %d, %d, %d,
0x%x, %d, 0x%x, \"%s\", \"%s\", 0x%x, 0x%x, 0x%x)",
	   display, screen, parentWindow,
	   windowX, windowY, windowWidth, windowHeight, windowDepth,
	   pImageVisualToUse, argc, argv, windowName, iconName,
	   imageWindow, imageColormap,  mustFreeImageColormap));

    if (pImageVisualToUse == DefaultVisual(display, screen))
    {
	*mustFreeImageColormap = False;
	*imageColormap = winAttributes.colormap = DefaultColormap(display,
								  screen);
	winAttributes.background_pixel = BlackPixel(display, screen);
	winAttributes.border_pixel = WhitePixel(display, screen);
    }
    else
    {
	XColor		actualColor, databaseColor;

	*mustFreeImageColormap = True;
	*imageColormap = winAttributes.colormap =
	    XCreateColormap(display, RootWindow(display, screen),
			    pImageVisualToUse, AllocNone);
	XAllocNamedColor(display, winAttributes.colormap, "Black",
			 &actualColor, &databaseColor);
	winAttributes.background_pixel = actualColor.pixel;
	XAllocNamedColor(display, winAttributes.colormap, "White",
			 &actualColor, &databaseColor);
	winAttributes.border_pixel = actualColor.pixel;
    }
    winAttributes.event_mask = ExposureMask;
    winAttributes.backing_store = WhenMapped ;

    *imageWindow = XCreateWindow(display, parentWindow,
				 0, 0, windowWidth, windowHeight, 2,
				 windowDepth, InputOutput, pImageVisualToUse,
				 (CWBackPixel | CWColormap |
				  CWBorderPixel | CWEventMask | CWBackingStore),
				 &winAttributes);

    hints.flags = (USSize | USPosition);
    hints.x = windowX;
    hints.y = windowY;
    hints.width  = windowWidth;
    hints.height = windowHeight;

    XSetStandardProperties(display, *imageWindow, (char *)windowName, iconName,
			   None, argv, argc, &hints);

    EXIT((""));
    return(0);
} /* CreateImagePlanesWindow() */

