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


#define M_ERROR   1
#define M_WARNING 2
#define M_MESSAGE 3

#define MAXBUFLEN 2048

static int strcmp_lc(char *a, char *b);

int
m_Message(Object *in, Object *out)
{
    char *usermess = NULL;
    char *messname = NULL;
    int messtype;
    int ispopup;


    if (!in[0])
	return OK;

    if (!DXExtractString(in[0], &usermess)) {
	DXSetError(ERROR_BAD_PARAMETER, "#10200", "message"); 
	return ERROR;
    }

    if (in[1]) {
	if (!DXExtractString(in[1], &messname)) {
	    DXSetError(ERROR_BAD_PARAMETER, "#10200", "message type"); 
	    return ERROR;
	}
	
	if (!strcmp_lc(messname, "error"))
	    messtype = M_ERROR;
	else if (!strcmp_lc(messname, "warning"))
	    messtype = M_WARNING;
	else if (!strcmp_lc(messname, "message"))
	    messtype = M_MESSAGE;
	else {
	    DXSetError(ERROR_BAD_PARAMETER, 
		       "%s must be one of 'error', 'warning' or 'message'",
		       messname);
	    return ERROR;
	}
    } else
	messtype = M_MESSAGE;

    if (in[2]) {
	if (!DXExtractInteger(in[2], &ispopup)) {
	    DXSetError(ERROR_BAD_PARAMETER, "#10070", "popup"); 
	    return ERROR;
	}

	if (ispopup != 0 && ispopup != 1) {
	    DXSetError(ERROR_BAD_PARAMETER, "#10070", "popup");
	    return ERROR;
	}
    } else
	ispopup = 0;


    switch (messtype) {
      case M_ERROR:
	if (ispopup)
	    DXUIMessage("ERROR POPUP", usermess);
	else
	    DXUIMessage("ERROR", usermess);
	break;

      case M_WARNING:
	if (ispopup)
	    DXUIMessage("WARNING POPUP", usermess);
	else
	    DXWarning(usermess);
	break;

      case M_MESSAGE:
	if (ispopup)
	    DXUIMessage("MESSAGE POPUP", usermess);
	else
	    DXMessage(usermess);
	break;
    }

    return OK;
}


/* given a string and a lower case string, do a strcmp ignoring the
 *  case of the first string.  return 1 if not equal, 0 if equal.
 */
static int strcmp_lc(char *a, char *b)
{
    if (!a || !b)
	return 1;

    while(*a && *b) {
	if (*a != *b && (!isupper(*a) || tolower(*a) != *b))
	    return 1;

	a++, b++;
    }
    
    if (!*a && !*b)
	return 0;

    return 1;
}

