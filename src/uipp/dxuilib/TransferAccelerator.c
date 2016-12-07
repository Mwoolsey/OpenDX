/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999, 2002                             */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include "dxconfig.h"

#include <stdlib.h>
#include <X11/Xatom.h>
#include <X11/Intrinsic.h>
#include <X11/Shell.h>

#include <Xm/Xm.h>
#include <Xm/PushB.h>

#include "TransferAccelerator.h"
#include <Xm/XmP.h>

#if defined(HAVE_XMMAPKEYEVENTS)
extern int _XmMapKeyEvents(
                        register String str,
                        int **eventType, 
                        KeySym **keysym,
                        Modifiers **modifiers) ;
#else  /* (HAVE_XMMAPKEYEVENTS) */
extern Boolean _XmMapKeyEvent(String        string,
			int          *eventType,
			unsigned int *keysym,
			unsigned int *modifiers) ;
#endif /* (HAVE_XMMAPKEYEVENTS) */

/*
** Structure used to pass details of the accelerator to the event handler.
*/

typedef struct Accelerator_s *Accelerator_p ;
typedef struct Accelerator_s 
{
	KeyCode    keycode ;
	Modifiers  modifiers ;
	Widget     source ;
	String     action ;
} Accelerator_t ;

/*
** The event handler, installed onto the shells where we want actions replicated.
*/

/* ARGSUSED */
#ifndef   _NO_PROTO
static void HandleAccelerator(Widget widget, XtPointer client_data, XEvent *event, Boolean *cont)
#else  /* _NO_PROTO */
static void HandleAccelerator(widget, client_data, event, cont)
	Widget     widget ;
	XtPointer  client_data ;
	XEvent    *event ;
	Boolean   *cont ;
#endif /* _NO_PROTO */
{
	/*
	** Check that we have the right key combination 
	*/

	register Accelerator_p accelerator = (Accelerator_p) client_data ;

	if ((event->xkey.state == accelerator->modifiers) && (event->xkey.keycode == accelerator->keycode)) {
		if (XtIsSensitive(accelerator->source)) {
			XtCallActionProc(accelerator->source, accelerator->action, event, (String *) 0, (Cardinal) 0) ;
		}
	}
}

/*
** Free up memory associated with this event handler.
*/

/* ARGSUSED */
#ifndef   _NO_PROTO
static void FreeAccelerator(Widget widget, XtPointer client_data, XtPointer call_data)
#else  /* _NO_PROTO */
static void FreeAccelerator(widget, client_data, call_data)
	Widget     widget ;
	XtPointer  client_data ;
	XtPointer  call_data ;
#endif /* _NO_PROTO */
{
	register Accelerator_p accelerator = (Accelerator_p) client_data ;

	if (accelerator != (Accelerator_p) 0) {
		XtRemoveEventHandler(widget, KeyPressMask, False, HandleAccelerator, (XtPointer) accelerator) ;

		XtFree((char *) accelerator) ;

		/*
		** Because it is register
		*/

		accelerator = (Accelerator_p) 0 ;
	}
}

/*
** Installs an event handler for the given KeyCode/Modifier combination to call the associated action.
*/

#ifndef   _NO_PROTO
static Boolean InstallAccelerator(Widget shell, KeySym keysym, Modifiers modifiers, Widget source, String action)
#else  /* _NO_PROTO */
static Boolean InstallAccelerator(shell, keysym, modifiers, source, action)
	Widget    shell ;
	KeySym    keysym ;
	Modifiers modifiers ;
	Widget    source ;
	String    action ;
#endif /* _NO_PROTO */
{
	register Accelerator_p accelerator ;
	         KeyCode       keycode ;

	/*
	** Convert keysym to keycode and set up passive grab on shell 
	*/
	keycode = XKeysymToKeycode(XtDisplay(shell), keysym) ;

	if (keycode == 0) {
		return False ;
	}

	/*
	** Add an Asynchronous Gran onto the shell.
	*/

	XtGrabKey(shell, keycode, modifiers, False, GrabModeAsync, GrabModeAsync) ;

	/*
	** Set up event handler on shell 
	*/

	accelerator = (Accelerator_p) XtMalloc((unsigned) sizeof(Accelerator_t)) ;

	accelerator->keycode   = keycode ;
	accelerator->modifiers = modifiers ;
	accelerator->source    = source ;
	accelerator->action    = XtNewString(action) ;

	XtAddEventHandler(shell, KeyPressMask, False, HandleAccelerator, (XtPointer) accelerator) ;
	XtAddCallback(source, XmNdestroyCallback, FreeAccelerator, (XtPointer) accelerator) ;

	return True ;
}

/*
** The Public Interface to the above.
*/

#ifndef   _NO_PROTO
Boolean TransferAccelerator(Widget shell, Widget source, String action)
#else  /* _NO_PROTO */
Boolean TransferAccelerator(shell, source, action)
	Widget shell ;
	Widget source ;
	String action ;
#endif /* _NO_PROTO */
{
	/*
	** Installs an accelerator on shell to fire button source
	** using the same accelerator
	*/
	
	char     *accelerator = (char *) 0 ;
	Boolean   retcode     = False ;
	KeySym    keysym=0 ;
	Modifiers modifiers=0 ;

	/*
	** Fetch the accelerator for the given source of the event.
	*/
	
	XtVaGetValues(source, XmNaccelerator, &accelerator, NULL) ;
	
	if (accelerator != (char *) 0) {
		/*
		** Convert the accelerator to KeySym/Modifier combination.
		*/

#if defined(HAVE_XMMAPKEYEVENTS)
		int        count ;
		int	  *type_list;
		KeySym    *keysym_list ;
		Modifiers *modifiers_list ;
		
		count = _XmMapKeyEvents(accelerator, &type_list, &keysym_list, &modifiers_list) ;

		if (count > 0) {
			keysym    = *keysym_list ;
			modifiers = *modifiers_list ;
			retcode   = True ;
			XtFree((char*) type_list);
			XtFree((char*) keysym_list);
			XtFree((char*) modifiers_list);
		}

#else  /* (HAVE_XMMAPKEYEVENTS) */
		int       type ;
		retcode = _XmMapKeyEvent(accelerator, &type, (unsigned *) &keysym, 
                             &modifiers) ;
#endif /* (HAVE_XMMAPKEYEVENTS) */

		/*
		** Install the action.
		*/

		if (retcode == True) {
			retcode = InstallAccelerator(shell, keysym, modifiers, source, action) ;
		}

		XtFree(accelerator) ;
    	}
	
	return retcode ;
}
