/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>



#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <dx/dx.h>


#define MAXPARMS   22
#define MAXP       120
#define BUFFERSIZE  2048
#define MAXLEN      2000

#define atEnd() ((bp-buffer) > MAXLEN)

static Object format_object(Object *o, char *format);
static Object format_field(Object *o, char *format);
static String format_one(Object *in, char *format);

static int tohex(char c);


Error
m_Format(Object *in, Object *out)
{
    char *cp;

    out[0] = NULL;

    if (!in[0]) {
	DXSetError(ERROR_BAD_PARAMETER, "#10000", "template string");
	return ERROR;
    }

    if (!DXExtractString(in[0], &cp)) {
	DXSetError(ERROR_BAD_PARAMETER, "#10200", "template");
	return ERROR;
    }
    
    
    /* if all inputs are arrays or strings, just do a single format.
     * else, copy the inputs and recurse.  (needs code here)
    out[0] = format_object(in+1, cp);
     */
    
    out[0] = (Object)format_one(in+1, cp);
    
    return (out[0] ? OK : ERROR);
}

/*
static Error sigh.
 */

static Object format_object(Object *o, char *format)
{
    Object newo, subo;
    Matrix m;
    Object subo2;
    int fixed, z;
    int i;
    
    if (o[0] == NULL)
	return (Object)format_one(o, format);

    
    switch (DXGetObjectClass(o[0])) {
      case CLASS_GROUP:
	/* for each member */
	for (i=0; (subo = DXGetEnumeratedMember((Group)*o, i, NULL)); i++) {
	    if (!format_object(&subo, format))
		continue;
	}
	break;
	
      case CLASS_FIELD:
	/* even empty fields have to be copied */
	if (DXEmptyField((Field)*o))
	    return DXCopy(*o, COPY_STRUCTURE);
	
	if (!(newo = DXCopy(*o, COPY_STRUCTURE)))
            return NULL;
	
	if (!(*o = (Object)format_field(&newo, format))) {
	    DXDelete(newo);
	    return NULL;
	}
	break;

      case CLASS_STRING:
      case CLASS_ARRAY:
	return (Object)format_one(o, format);

      case CLASS_XFORM:
	if (!DXGetXformInfo((Xform)*o, &subo, &m))
	    return NULL;

	if (!(newo=format_object(&subo, format)))
	    return NULL;

	*o = (Object)DXNewXform(newo, m);
	break;


      case CLASS_SCREEN:
	if (!DXGetScreenInfo((Screen)*o, &subo, &fixed, &z))
	    return NULL;

	if (!(newo=format_object(&subo, format)))
	    return NULL;

	*o = (Object)DXNewScreen(newo, fixed, z);
	break;


      case CLASS_CLIPPED:
	if (!DXGetClippedInfo((Clipped)*o, &subo, &subo2))
	    return NULL;

	if (!(newo=format_object(&subo, format)))
	    return NULL;

	*o = (Object)DXNewClipped(newo, subo2);
	break;

	
      default:
	/* any remaining objects can't contain fields, so we can safely
	 *  pass them through unchanged.
	 */
	break;

    }

    return *o;

}


static Object format_field(Object *in, char *cp)
{

    /* get the data component and alloc space for a list of str ptrs. */
    
    /* for each entry, call format_one and collect the strings. */
    
    /* call makestringlist and make that the new data component */

    return (Object)format_one(in, cp);
}  


static String format_one(Object *in, char *cp)
{
    int input;
    int nitems;
    int i, num;
    float f;
    double d;
    char buffer[BUFFERSIZE];
    char *bp;
    char *s;
    char format[256];
    Type type;
    void *p;

    input = 0;
    bp = buffer;
    while (*cp && !atEnd()) {
	switch (*cp) {
	  case '%':
	    cp++;
	    /* 
	     * check for %% - formats as a single percent sign.
	     */
	    if (*cp == '%') {
		*bp++ = *cp++;
		break;
	    }
	    /*
	     * copy format control information (eg. "%3.2f", copy "%3.2")
	     */
	    i = 0;
	    format[i++] = '%';
	    if (*cp == '-')
		format[i++] = *cp++;
	    while (isdigit(*cp))
		format[i++] = *cp++;
	    if (*cp == '.') {
		/* meta-data stuff should go here too */
		format[i++] = *cp++;
		while (isdigit(*cp))
		    format[i++] = *cp++;
	    }
	    /*
	     * process specific formatting command
	     */
	    switch (*cp) {
	      /*
	       * string (%s)
	       */
	      case 's':
		format[i++] = 's';
		format[i] = '\0';
		if (!DXExtractNthString(in[input], 0, &s)) {
		    DXSetError(ERROR_BAD_PARAMETER, "#10000", "string");
		    return ERROR;
		}

		for (i = 0; DXExtractNthString(in[input], i, &s); i++) {
		    if (i > 0) {
			*bp++ = ' ';
		    }
		    sprintf(bp, format, s);
		    while (*bp) bp++;
		    if (atEnd())
			break;
		}
		input++;
		cp++;
		break;

	      /*
	       * character (%c)
	       */
	      case 'c':
		format[i++] = 'c';
		format[i] = '\0';
		if (DXExtractNthString(in[input], 0, &s)) {
		    for (i = 0; DXExtractNthString(in[input], i, &s); i++) {
			if (i > 0) {
			    *bp++ = ' ';
			}
			sprintf(bp, format, s[0]);
			while (*bp) bp++;
			if (atEnd())
			    break;
		    }
		} else if (DXExtractInteger(in[input], &i)) {
		    sprintf(bp, format, i);
		    while (*bp)	bp++;
		    if (atEnd())
			break;
		}
		else if (DXQueryParameter(in[input], TYPE_INT, 1, &nitems)) {
		    int *ip;
		    
		    ip = (int *)DXAllocateLocal(sizeof(int) * nitems);
		    if (ip == NULL)
			return ERROR;

		    if (!DXExtractParameter(in[input], TYPE_INT, 1, nitems,
					  (Pointer)ip)) {
			DXFree((Pointer)ip);
			DXErrorReturn(ERROR_INTERNAL, 
				    "couldn't extract integer list");
		    }
		    *bp++ = '{';
		    *bp++ = ' ';
		    for(i = 0; i < nitems * 1; i++) {
			sprintf(bp, format, ip[i]);
			while (*bp) bp++;
			*bp++ = ' ';
			if (atEnd())
			    break;
		    }
		    *bp++ = '}';
		    DXFree((Pointer)ip);
		}
		else {
		    DXSetError(ERROR_BAD_PARAMETER, "#10000", "character");
		    return ERROR;
		}


		input++;
		cp++;
		break;

	      /*
	       * integer (%d)
	       */
	      case 'd':
		format[i++] = 'd';
		format[i] = '\0';
		if (DXExtractInteger(in[input], &i)) {
		    sprintf(bp, format, i);
		    while (*bp)	bp++;
		    if (atEnd())
			break;
		}
		else if (DXQueryParameter(in[input], TYPE_INT, 1, &nitems)) {
		    int *ip;
		    
		    ip = (int *)DXAllocateLocal(sizeof(int) * nitems);
		    if (ip == NULL)
			return ERROR;

		    if (!DXExtractParameter(in[input], TYPE_INT, 1, nitems,
					  (Pointer)ip)) {
			DXFree((Pointer)ip);
			DXErrorReturn(ERROR_INTERNAL, 
				    "couldn't extract integer list");
		    }
		    *bp++ = '{';
		    *bp++ = ' ';
		    for(i = 0; i < nitems * 1; i++) {
			sprintf(bp, format, ip[i]);
			while (*bp) bp++;
			*bp++ = ' ';
			if (atEnd())
			    break;
		    }
		    *bp++ = '}';
		    DXFree((Pointer)ip);
		}
		else {
		    DXSetError(ERROR_BAD_PARAMETER, "#10000", "integer");
		    return ERROR;
		}

		input++;
		cp++;
		break;

	      /*
	       * float or double (%f, %e or %g)
	       */
	      case 'e':
	      case 'f':
	      case 'g':
		format[i++] = *cp;
		format[i] = '\0';
		if (DXExtractFloat(in[input], &f)) {
		    sprintf(bp, format, f);
		    while (*bp) bp++;
		    if (atEnd())
			break;
		} 
		else if (DXExtractParameter(in[input],TYPE_DOUBLE,0,1,&d)) {
		    sprintf(bp,format,d);
		    while (*bp) bp++;
		    if (atEnd())
			break;
		}
		else if (DXQueryParameter(in[input], TYPE_DOUBLE, 1, &nitems)) {
		    if (DXQueryParameter(in[input], TYPE_FLOAT, 1, &nitems)) {
		       type=TYPE_FLOAT;
		       p = DXAllocateLocal(sizeof(float) * nitems);
		    }
		    else {
		       type = TYPE_DOUBLE;
		       p = DXAllocateLocal(sizeof(double) * nitems);
		    }

		    if (p == NULL)
			return ERROR;

		    if (!DXExtractParameter(in[input], type, 1,
					  nitems, (Pointer)p)) {
			DXFree((Pointer)p);
			DXErrorReturn(ERROR_INTERNAL, 
				    "couldn't extract float list");
		    }
		    *bp++ = '{';
		    *bp++ = ' ';
		    for(i=0; i<nitems*1; i++)
			{
			    if (type==TYPE_FLOAT)
			        sprintf(bp, format, ((float *)p)[i]);
			    else
			        sprintf(bp, format, ((double *)p)[i]);
			    while (*bp) bp++;
			    *bp++ = ' ';
			    if (atEnd())
				break;
			}
		    *bp++ = '}';
		    DXFree((Pointer)p);
		} 
		else if (DXQueryParameter(in[input], TYPE_DOUBLE, 2, &nitems)) {
		    if (DXQueryParameter(in[input], TYPE_FLOAT, 2, &nitems)) {
		        type = TYPE_FLOAT;
			p = DXAllocateLocal(sizeof(float) * 2 * nitems);
		    }
		    else {
		        type = TYPE_DOUBLE; 
		        p = DXAllocateLocal(sizeof(double) * 2 * nitems);
		    }

		    if (p == NULL)
			return ERROR;

		    if (!DXExtractParameter(in[input], type , 2,
					  nitems, (Pointer)p)) {
			DXFree((Pointer)p);
			DXErrorReturn(ERROR_INTERNAL,
				    "couldn't extract vector list");
		    }
		    if (nitems > 1) {
			*bp++ = '{';
			*bp++ = ' ';
		    }
		    for(i = 0; i < nitems; i++) {
			*bp++ = '[';
		        if (type == TYPE_FLOAT)
			    sprintf(bp, format, ((float *)p)[i*2]);
			else
			    sprintf(bp, format, ((double *)p)[i*2]);
			while (*bp) bp++;
			*bp++ = ',';
			*bp++ = ' ';
		        if (type == TYPE_FLOAT)
			    sprintf(bp, format, ((float *)p)[i*2 + 1]);
			else
			    sprintf(bp, format, ((double *)p)[i*2 + 1]);
			while (*bp) bp++;
			*bp++ = ']';
			if (nitems > 1)
			    *bp++ = ' ';
			if (atEnd())
			    break;
		    }
		    if (nitems > 1)
			*bp++ = '}';

		    DXFree((Pointer)p);
		} 
		else if (DXQueryParameter(in[input], TYPE_FLOAT, 3, &nitems)) {
		    if (DXQueryParameter(in[input], TYPE_FLOAT, 3, &nitems)) {
		        type = TYPE_FLOAT;
			p = DXAllocateLocal(sizeof(float) * 3 * nitems);
		    }
		    else {
		        type = TYPE_DOUBLE; 
		        p = DXAllocateLocal(sizeof(double) * 3 * nitems);
		    }
		    
		    if (p == NULL)
			return ERROR;

		    if (!DXExtractParameter(in[input], type, 3,
					  nitems, (Pointer)p)) {
			DXFree((Pointer)p);
			DXErrorReturn(ERROR_INTERNAL,
				    "couldn't extract vector list");
		    }
		    if (nitems > 1) {
			*bp++ = '{';
			*bp++ = ' ';
		    }
		    for(i = 0; i < nitems; i++) {
			*bp++ = '[';
		        if (type == TYPE_FLOAT)
			    sprintf(bp, format, ((float *)p)[i*3]);
			else
			    sprintf(bp, format, ((double *)p)[i*3]);
			while (*bp) bp++;
			*bp++ = ',';
			*bp++ = ' ';
		        if (type == TYPE_FLOAT)
			    sprintf(bp, format, ((float *)p)[i*3 + 1]);
			else
			    sprintf(bp, format, ((double *)p)[i*3 + 1]);
			while (*bp) bp++;
			*bp++ = ',';
			*bp++ = ' ';
		        if (type == TYPE_FLOAT)
			    sprintf(bp, format, ((float *)p)[i*3 + 2]);
			else
			    sprintf(bp, format, ((double *)p)[i*3 + 2]);
			while (*bp) bp++;
			*bp++ = ']';
			if (nitems > 1)
			    *bp++ = ' ';
			if (atEnd())
			    break;
		    }
		    if (nitems > 1)
			*bp++ = '}';
		    
		    DXFree((Pointer)p);
		}
		else {
		    DXSetError(ERROR_BAD_PARAMETER, "#10000", 
			     "floating point number");
		    return ERROR;
		}
		input++;
		cp++;
		break;
		
	      /* 
	       * any other character following a % is copied through unchanged,
	       *  and doesn't update the arg count.
	       */
	      default:
		*bp++ = *cp++;
		break;
	    }
            if (input >= MAXPARMS) {
                DXSetError(ERROR_BAD_PARAMETER, 
                         "too many substitution characters");
                return ERROR;
            }
	    break;
	    
	  case '\\':
	    cp++;
	    /* 
	     * check for \\ - formats as a real backslash
	     */
	    if (*cp == '\\') {
		*bp++ = *cp++;
		break;
	    }
	    /* 
	     * other things which can follow a backslash...
	     */
	    switch (*cp) {
	      /* the standard escapes */
	      case 'a':  *bp++ = '\a'; 	cp++;  break;
	      case 'b':  *bp++ = '\b'; 	cp++;  break;
	      case 'f':  *bp++ = '\f';	cp++;  break;
	      case 'n':  *bp++ = '\n';	cp++;  break;
	      case 'r':  *bp++ = '\r';	cp++;  break;
	      case 't':  *bp++ = '\t';	cp++;  break;
	      case 'v':  *bp++ = '\v';	cp++;  break;
	      case '\\': *bp++ = '\\';	cp++;  break;
	      case '"':  *bp++ = '"';	cp++;  break;
	      case '\'': *bp++ = '\'';	cp++;  break;
	      case '?':  *bp++ = '?';	cp++;  break;

              /* hex codes - 0, 1 or 2 digits */
	      case 'x':
		cp++;
		if (!isxdigit(*cp)) {
		    *bp++ = 'x';
		    break;
		} 
		num = tohex(*cp++);
		if (isxdigit(*cp)) {
		    num <<= 4;
		    num += tohex(*cp++);
		}
		*bp++ = (char)num;
		break;

	      /* octal codes - 1, 2 or 3 digits */
	      case '0': case '1': case '2': 
	      case '3': case '4': case '5': 
	      case '6': case '7': 
		num = (uint)(*cp++) - (uint)'0';
		for (i=0; i<2; i++) {
		    char tc = *cp;
		    if (isdigit(tc) && tc != '8' && tc != '9') {
			num <<= 3;
			num += (uint)(*cp++) - (uint)'0';
		    }
		}
		*bp++ = (char)num;
		break;
		
	      /* end of file */
	      case (char)EOF:
		*bp++ = '\0'; 
		break;

	      /* escaped end of line */
	      case '\n':
		cp++;
		break;

		/* any other char */
	      default:
		*bp++ = *cp++;  
		break;
	    }  
	    break;
	    
	    /*
	     * any character in the string which isn't preceeded by % gets
	     *  copied here unchanged.
	     */
	  default:
	    *bp++ = *cp++;
	    break;
	}
    }

    if (atEnd()) {
	DXSetError(ERROR_BAD_PARAMETER, 
		   "formatted output exceeds maximum format buffer length");
	return ERROR;
    }
    *bp = '\0';

    return (String)DXNewString(buffer);
}


/* check 'c' before calling this with isxdigit() because this does no
 * error checking.
 */
static int tohex(char c)
{
    /* A-F */
    if (isupper(c))
        return 10 + (uint)(c) - (uint)'A';

    /* a-f */
    if (islower(c))
        return 10 + (uint)(c) - (uint)'a';

    /* 0-9 */
    return (uint)(c) - (uint)'0';

}
