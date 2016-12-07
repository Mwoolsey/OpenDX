/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


#include <dx/dx.h>

#if defined(HAVE_STRING_H)
#include <string.h>
#endif

#define M_ERROR   1
#define M_WARNING 2
#define M_MESSAGE 3

#define MAXBUFLEN 2048
#define MAXOUT    21

static Error parseit(char *inputstring, char *control, Object *out);
static void copyformat(char **from, char **to);

int
m_Parse(Object *in, Object *out)
{
    char *inputstring = NULL;
    char *control = NULL;
    int i;

    for (i=0; i<MAXOUT; i++)
	out[i] = NULL;

    if (!in[0] || !DXExtractString(in[0], &inputstring)) {
	DXSetError(ERROR_BAD_PARAMETER, "#10200", "input"); 
	return ERROR;
    }
    
    if (in[1]){
      if (!DXExtractString(in[1], &control)) {
	DXSetError(ERROR_BAD_PARAMETER, "#10200", "format"); 
	return ERROR;
      }
    }
    else 
      control= "%s";
    
    if (!parseit(inputstring, control, out))
	return ERROR;

    return OK;
}



static Error parseit(char *inputstring, char *control, Object *out)
{
    char *ccp, *icp, *tcp;
    Pointer tempspace = NULL;
    Pointer tempcontrol = NULL;
    int i, nmatched=0;
    int longlongflag, longflag, shortflag, skipflag;
    int bcount, knowtype;
    Object o;
    Type type=TYPE_BYTE;

    /* set them all to null so we can delete them all on error */
    for (i=0; i<MAXOUT; i++)
	out[i] = NULL;

    /* each individual variable can't be longer than the input str
     * but at least sizeof(double) needs to be allocated.  */
    tempspace = DXAllocate(MAX(sizeof(double), strlen(inputstring) + 1));
    if (!tempspace)
	goto error;

    /* each individual format spec can't be longer than the whole
     * control with appended %n */
    tempcontrol = DXAllocate(strlen(control) + 3);
    if (!tempcontrol)
	goto error;


    /* allocate enough memory to hold one value of any type.
     * go thru the string looking for formats.
     * use the real sscanf to parse the string.
     * then make an object of that type.
     */

    for (ccp = control, icp = inputstring, nmatched = 0; ccp && *ccp;  ) {

	/* consume the chars until the next %.  they have to match
	 *  the chars in the inputstring exactly (except whitespace
	 *  which matches any other whitespace).
	 */
	tcp = tempcontrol;
	*tcp = '\0';

	/* while not at complete end of format string, and not at a % char,
         * copy chars to a temp format string.  %% is ok - it matches
	 * a real % char, so it isn't the start of a substitution char.
	 */
	while (1) {
	    if (*ccp == '\0')
		break;
	    if (*ccp == '%') {
		if (*(ccp+1) == '%')
		    *tcp++ = *ccp++;
		else
		    break;
	    }
	    *tcp++ = *ccp++;
	}
	*tcp = '\0';
	
	/* if there was stuff before the next format... */
	if (((char *)tempcontrol)[0] != '\0') {
	    /* add a format which returns the number of chars consumed 
	     *  from the input string.
	     */
	    strcat(tcp, "%n");
	    if ((sscanf(icp, tempcontrol, &bcount) < 0) || (bcount <= 0)) {
		DXSetError(ERROR_BAD_PARAMETER,
			   "any character other than %% must match exactly");
		goto error;
	    }
	    icp += bcount;
	}

	/* at the end? */
	if (*ccp == '\0')
	    break;

	/* next char must be % */
	if (*ccp != '%') {
	    DXSetError(ERROR_DATA_INVALID, "next char not %%??");
	    goto error;
	}
	    
	ccp++;

	/* check for badly formed string - can't end with % */
	if (*ccp == '\0') {
	    DXSetError(ERROR_BAD_PARAMETER, 
		       "missing format character after %%");
	    goto error;
	}
	
	/* copy the chars until the next % or whitespace to the temp
	 * format buffer.
	 */
	tcp = tempcontrol;
	*tcp++ = '%';
	*tcp = '\0';
	copyformat(&ccp, &tcp);
	*tcp = '\0';
	
	/* if we did not get a format... */
	if (((char *)tempcontrol)[0] == '\0') {
	    DXSetError(ERROR_BAD_PARAMETER,
		       "missing format character %d", nmatched+1);
	    goto error;
	}
	
	/* add a format which returns how many chars we consumed from
	 *  the string.
	 */
	strcat(tcp, "%n");
	if ((sscanf(icp, tempcontrol, tempspace, &bcount) != 1) || (bcount <= 0)) {
	    DXSetError(ERROR_BAD_PARAMETER,
		       "format %d did not match input", nmatched+1);
	    goto error;
	}
	icp += bcount;
	
	
	/* now i have to figure out what type of conversion it was so i 
	 *  can make an array of the right type.  bother.
	 */
	tcp = tempcontrol;
	longlongflag = longflag = shortflag = skipflag = 0;
	knowtype = 0;
	while (*tcp != '\0') {
	    switch (*tcp) {
	      case 'l':
		longflag++;
		break;
	      case 'L':
		longlongflag++;
		break;
	      case '*':
		skipflag++;
		break;
	      case 'h':
		shortflag++;
		break;
	      case 'd':
	      case 'i':
	      case 'u':
	      case 'o':
	      case 'x':
	      case 'p':
	      case 'n':
	      case 'B':
		type = TYPE_INT;
		if (shortflag)
		    type = TYPE_SHORT;
		knowtype++;
		break;
	      case 'e':
	      case 'f':
	      case 'g':
		type = TYPE_FLOAT;
		if (longflag)
		    type = TYPE_DOUBLE;
		knowtype++;
		break;
	      case 'c':
	      case 's':
	      case 'S':
	      case '[':
		type = TYPE_STRING;
		knowtype++;
		break;

	      default:
		/* don't recognize the format yet.  keep trying */
		break;
	    }
	    if (knowtype)
		break;
	    tcp++;
	}

	/* skip flag */
	if (skipflag)
	    continue;


	/* make an array */
	if (type != TYPE_STRING) {
	    o = (Object)DXNewArray(type, CATEGORY_REAL, 0);
	    if (!o)
		goto error;
	    if (!DXAddArrayData((Array)o, 0, 1, tempspace)) {
		DXDelete(o);
		goto error;
	    }
	} else {
	    o = (Object)DXNewString((char *)tempspace);
	    if (!o)
		goto error;
	}

	/* set the output object */
	out[nmatched] = o;

	/* we successfully matched a format char. */
	nmatched++;

    }

    DXFree(tempspace);
    DXFree(tempcontrol);
    return OK;

  error:
    DXFree(tempspace);
    DXFree(tempcontrol);
    for (i=0; i<nmatched; i++) {
	DXDelete(out[i]);
	out[i] = NULL;
    }

    return ERROR;
}


/* copy chars from one string to another as long as you are in a format
 * definition. 
 */
static void copyformat(char **from, char **to)
{
    while (**from) {
	switch (**from) {

	  /* chars which end the format */
	  case '\0':
	  case ' ':
	  case '\t':
	  case '\n':
	  case '\r':
	  case '\f':
	  case '%':
	    return;
	    
	  /* final char in format spec */
	  case 'd':
	  case 'i':
	  case 'u':
	  case 'o':
	  case 'x':
	  case 'p':
	  case 'e':
	  case 'f':
	  case 'g':
	  case 'c':
	  case 's':
	  case 'S':
	  case 'n':
	  case 'B':
	    *(*to)++ = *(*from)++;
	    return;

	  /* until closing ], except if ] is first char, or ^] */
	  case '[':
	    *(*to)++ = *(*from)++;
	    if (**from == '^')
		*(*to)++ = *(*from)++;
	    if (**from == ']')
		*(*to)++ = *(*from)++;
	    while (**from != ']' && **from != '\0')
		*(*to)++ = *(*from)++;

	    *(*to)++ = *(*from)++;
	    return;
	    
	  /* modifiers which don't determine the format yet */
	  case 'l':
	  case 'L':
	  case 'h':
	  case '*':
	  case '.':
	  case '0': case '1': case '2': case '3': case '4': 
	  case '5': case '6': case '7': case '8': case '9':
	  default:
	    break;
	}
	*(*to)++ = *(*from)++;
    }

    return;
}
