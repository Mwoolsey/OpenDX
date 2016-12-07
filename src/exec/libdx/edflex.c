/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>

 
#include <dx/dx.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
 
#include "edf.h"
 
 
/* prototypes
 */

static int tohex(char c);
static Error set_lines(struct finfo *fp);
 
 
#define COMMENTCHAR '#'         /* rest of current input line ignored */

#if 1   /* use the macros for performance instead of a func call */
 
#define input()     (pp->byteoffset++, getc(fp->fd))
#define pushback(c) (ungetc(c, fp->fd), --pp->byteoffset)

#else

/* i was using these until i got the above macros to work - the comma
 * was screwing things up until i parenthesized them.
 */
static char input(FILE *fp, int *offset) {
    *offset++;
    return getc(fp);
}

static void pushback(char c, FILE *fp, int *offset)
{
    ungetc(c, fp);
    *offset--;
}
#endif

/* 
 * return the next token from the input stream.
 *
 * input is a pointer to a file structure, which contains an open file
 * pointer, current offset into the buffer, current lineno and input buffer.
 *
 * output is a pointer to a token structure, which contains a flag to
 * distinquish different classes of input (numbers, keywords, variables and 
 * errors), and the token value which is interpeted differently depending on 
 * the class of the input.
 *
 * CODE WHICH NEEDS TO BE ADDED:  
 *  support for [ x y z ] three vector numbers (also [x,y,z]?)
 *  \\ at end of line to continue strings
 */
Error _dxflexinput(struct finfo *fp)
{
    char nextc, *cp;
    int value;
    int incomment = 0;
    struct tinfo *tp;
    struct pinfo *pp;

    /* things for number conversions. */
    int onesign, onedot, oneexp;
    int isfloat, isdouble;
    char *pokehere;
    int done;
    double fvalue;
 
    if (!fp)
        return ERROR;
 
    tp = &fp->t;
    pp = &fp->p;

    /* if you are already at the end of the header or end of file,
     *  don't read further 
     */
    if (tp->class == ENDOFHEADER)
	return OK;

    if (tp->class == KEYWORD && tp->token.id == KW_END)
	return OK;

    /* clear tempbuf count, and keep track of what line we started on.
     */
    pp->incount = 0;
    _dxfsetprevline(fp);

    /* get the next token from the input file.
     */
    while(1) {
    
	/* next character
	 */
	nextc = input();

	/* end of input? 
	 */
	if (feof(fp->fd)) {
	    tp->class = ENDOFHEADER;
	    return OK;
	}
	
	/* new line? 
	 */
	if (nextc == '\n') {
	    fp->p.lineno++;

	    /* set byteoffset vs. line num here */
	    if (fp->p.lineno > fp->p.l.lcount)
		set_lines(fp);

	    incomment = 0;

	    /* return at start of next line? */
	    if (pp->tonextline) {
		tp->class = PUNCT;
		tp->subclass = NEWLINE;
		return OK;
	    }

	    continue;
	}
	
	/* already in a comment? 
	 */
	if (incomment) 
	    continue;

	/* start of a new comment?
	 */
	if (nextc == COMMENTCHAR) {
	    incomment = 1;
	    continue;
	}

	/* other whitespace? 
	 */
	if (isspace(nextc))
	    continue;
 
        /* binary? 
	 */
        if (!isprint(nextc)) {
	    pushback(nextc);
            tp->class = NOTASCII;
            return OK;
        }
            
	/* comma or semicolon?
	 * (can be used as number separators)
	 */
	if (nextc == ',' || nextc == ';') {

	    if (pp->skippunct)
		continue;

	    tp->class = PUNCT;
	    tp->subclass = (nextc == ',') ? COMMA : SEMICOLON;
	    return OK;
	}

	/* quoted string?
	 *  copy until matching close quote.  multiline strings must have
	 *  newlines escaped.  \c and \Xxx and \ooo char codes are ok.
	 */
	if (nextc == '"' || nextc == '\'') {

	    /* skip first char (the delimiter) and get the first one from
	     *  the string.
	     */
	    pp->delim = nextc;
	    cp = pp->inbuf;
	    nextc = input();

	    /* until matching delimiter or the buffer fills, read chars */
	    while ((nextc != pp->delim) && (cp < &pp->inbuf[MAXBUF-1])) {
		/* escape */
		if (nextc == '\\') {
		    int i; uint num; char tc;
		    switch (nextc = input()) {
		      /* standard ascii escape chars */
		      case 'a':  *cp++ = '\a'; 	break;
		      case 'b':  *cp++ = '\b'; 	break;
		      case 'f':  *cp++ = '\f';	break;
		      case 'n':  *cp++ = '\n';	break;
		      case 'r':  *cp++ = '\r';	break;
		      case 't':  *cp++ = '\t';	break;
		      case 'v':  *cp++ = '\v';	break;
		      case '\\': *cp++ = '\\';	break;
		      case '"':  *cp++ = '"';	break;
		      case '\'': *cp++ = '\'';	break;
		      case '?':  *cp++ = '?';	break;
		      /* hex codes - 0, 1 or 2 digits */
		      case 'x':
			tc = input();
			if (!isxdigit(tc)) {
			    pushback(tc);
			    *cp++ = 'x';
			    break;
			} 
			num = tohex(tc);
			tc = input();
			if (!isxdigit(tc))
			    pushback(tc);
			else {
			    num <<= 4;
			    num += tohex(tc);
			}
			*cp++ = (char)num;
			break;
		      /* octal codes - 1, 2 or 3 digits */
		      case '0': case '1': case '2': 
		      case '3': case '4': case '5': 
		      case '6': case '7': 
			num = (uint)(nextc) - (uint)'0';
			for (i=0; i<2; i++) {
			    tc = input();
			    if (!isdigit(tc) || tc == '8' || tc == '9')
				pushback(tc);
			    else {
				num <<= 3;
				num += (uint)(tc) - (uint)'0';
			    }
			}
			*cp++ = (char)num;
			break;
		      /* end of file */
		      case (char)EOF:  goto endofquote;
		      /* escaped end of line */
		      case '\n':
			fp->p.lineno++;
			/* set byteoffset vs. line num here */
			if (fp->p.lineno > fp->p.l.lcount)
			    set_lines(fp);

			break;
		      /* any other char */
		      default:
			*cp++ = nextc;  break;
		    }  
		} 
		else if (nextc == (char)EOF)
		    goto endofquote;
		
		else if (nextc == '\n')
		    goto endofquote;
		
		else
		    *cp++ = nextc;
		
		/* read next input char */
		nextc = input();
	    }
	    
	    /* terminate string in buffer */
	    *cp = '\0';
	    
	  endofquote:
	    /* if the current character isn't the close delimiter,
	     *  there is an error.
	     */
	    if(nextc != pp->delim) {
		tp->class = LEXERROR;
		if (cp >= &pp->inbuf[MAXBUF-1])
		    tp->subclass = BADLENGTH;
		else
		    tp->subclass = BADQUOTE;
		return ERROR;
	    } 

	    /* if you just want the string without putting it in the
	     * dictionary (like for string data arrays), return ok
	     * but with an invalid dictionary id.  before you call
	     * this routine again you must get the data from the token
	     * buffer because it will get overwritten by the next string.
	     * also set char count here because the string data routine
	     * needs it.
	     */
	    if (pp->nodict) {
		tp->class = STRING;
		tp->token.id = BADINDEX;
		pp->incount = cp - pp->inbuf;  /* plus or minus? */
		return OK;
	    }

	    value = _dxfputdict(fp->d, pp->inbuf);
	    if(value != BADINDEX) {
		tp->class = STRING;
		tp->token.id = value;
		return OK;
	    }
 
	    tp->class = LEXERROR;
	    tp->subclass = BADINPUT;
	    return ERROR;
	}

	/* other delimited string?
	 */
#if 0 
	/* there are currently no other delimiters defined, but if we
	 *  end up needing brackets or parens, here's where the code goes.
	 */
	if (nextc == '<' || nextc == '(' || nextc == '[' || nextc == '{') {
	    switch (nextc) {
	      case '<': pp->delim = '>'; tp->subclass = ANGLE; break;
	      case '(': pp->delim = ')'; tp->subclass = PAREN; break;
	      case '[': pp->delim = ']'; tp->subclass = SQUARE; break;
	      case '{': pp->delim = '}'; tp->subclass = CURLY; break;
	    }

	    /* skip first char (the delimiter) and get the first one from
	     *  the string.
	     */
	    pp->delim = nextc;
	    cp = pp->inbuf;
	    nextc = input();

	    /* until matching delimiter or the buffer fills, read chars */
	    while ((nextc != pp->delim) && (cp < &pp->inbuf[MAXBUF-1])) {
		*cp++ = nextc;
		if (nextc == '\n') {
		    fp->p.lineno++;
		    /* set byteoffset vs. line num here */
		    if (fp->p.lineno > fp->p.l.lcount)
			set_lines(fp);
		}
		nextc = input();
	    }
	    *cp = '\0';
 
	    if (nextc != pp->delim) {
		tp->class = LEXERROR;
		tp->subclass = BADBRACKET;
		return ERROR;
	    }
 
	    if (pp->nodict) {
		tp->class = BRACKET;
		tp->token.id = BADINDEX;
		pp->incount = cp - pp->inbuf;  
		return OK;
	    }

	    /* change this to make the class whatever the inside of the
	     * (), {}, <> or [] means.
	     */
	    value = _dxfputdict(fp->d, pp->inbuf);
	    if(value != BADINDEX) {
		tp->class = BRACKET;
		/* the subclass has already been set above */
		tp->token.id = value;
		return OK;
	    }
 
	    tp->class = LEXERROR;
	    tp->subclass = BADINPUT;
	    return ERROR;
	}
#endif

	/* here it starts to get tricky.  up to now i know what a token
	 *  is by looking at the first character.  but identifiers can
	 *  start with a number.  if the first char is a number, try to
	 *  parse it as a number until you get something which isn't
	 *  legal as a number.  here's where knowing if you are looking
	 *  for a string, number or either would be helpful.  
	 */ 

	/* here's where con ed shut off the power on me and i lost
	 *  about a page of editing.  drat them.
	 */

#define Is(c)    (nextc == c)
#define IsNot(c) (nextc != c)

	/* number?  (base 10 only, but exponential notation is ok).
	 */
	if (isdigit(nextc) || Is('-') || Is('+') || Is('.')) {
            onesign = 0;
            onedot = 0;
            oneexp = 0;
	    isfloat = 0;
	    isdouble = 0;
	    pokehere = NULL;
	    done = 0;
	    fvalue = 0.0;

	    /* if the next thing should be a string, don't try to parse
	     *  it as number - go directly to the string code.
	     */
	    if (pp->mustmatch == STRING)
		goto notnum;

	    cp = pp->inbuf;
	    while (!done) {
		switch (nextc) {
		  case '0': case '1': case '2': case '3': case '4':
		  case '5': case '6': case '7': case '8': case '9':
                    break;
		  case '-': case '+':
                    if (onesign) {
			tp->class = LEXERROR;
			tp->subclass = BADNUMBER;
			return ERROR;
                    }
                    onesign++;
		    break;
		  case '.': 
                    if (onedot) {
			tp->class = LEXERROR;
			tp->subclass = BADNUMBER;
			return ERROR;
                    }
                    onedot++;
		    isfloat++;
                    break;
                  case 'd': case 'D':
		    pokehere = cp;
		    isdouble++;
		    /* fall thru */
                  case 'e': case 'E':
                    if (oneexp) {
			tp->class = LEXERROR;
			tp->subclass = BADNUMBER;
			return ERROR;
                    }
                    oneexp++;
                    onesign = 0;
		    isfloat++;
		    break;
		  default:
		    done++;
		    continue;
		}
		*cp++ = nextc;
		nextc = input();
	    }
	    
	    /* if there is something which is part of the string but is
	     *  not a digit, this must not be a number.  reset the ptr
	     *  and try again.
	     */
	    /* NEEDS FIX !!!   should be isalpha? */
	    if (!isspace(nextc)) {
		/* allow a few things besides whitespace to terminate the #.
		 * anything else is assumed to be part of an unquoted string.
		 */
		if (IsNot(COMMENTCHAR) && IsNot(',') 
		    && IsNot(';') && IsNot((char)EOF)) {
		    *cp++ = nextc;
		    pp->incount = cp - pp->inbuf;
		    nextc = pp->inbuf[0];
		    goto notnum;
		}
	    } 
	    pushback(nextc);

	    *cp = '\0';

	    /* if we don't care about the value, don't do the conversion.
             */
	    if (pp->skipnum) {
		tp->class = NUMBER;
		return OK;
	    }
	    
	    tp->class = NUMBER;
	    if (isfloat) {
		if (isdouble)
		    *pokehere = 'e';
		fvalue = atof(pp->inbuf);
		if (!isdouble &&
		    ((fabs(fvalue) == 0.0) ||
		    ((fabs(fvalue) > DXD_MIN_FLOAT) && 
		     (fabs(fvalue) < DXD_MAX_FLOAT)))) {
		    tp->token.f = (float)fvalue;
		    tp->subclass = FLOAT;
		} else {
		    tp->token.d = fvalue;
		    tp->subclass = DOUBLE;
		} 
	    } else {
		value = atoi(pp->inbuf);
		tp->token.i = value;
		tp->subclass = INTEGER;
	    }
	    return OK;
	}
	
      notnum:
	
	/* keyword or unquoted string?
	 */
	if (isprint(nextc)) {
	    int bc = 1;

	    /* since we can get here from a partially parsed number,
	     *  this has to be able to get the next char either from
	     *  the token buffer or from the input file.
	     */
	    cp = &pp->inbuf[pp->incount > 0 ? pp->incount-1 : 0];

	    while (!isspace(nextc) && (cp < &pp->inbuf[MAXBUF-1])) {
		if (Is(','))
		    break;
		if (bc < pp->incount) {
		    nextc = pp->inbuf[bc];
		    bc++;
		} else {
		    *cp++ = nextc;
		    nextc = input();
		}
	    }
	    
	    *cp = '\0';
 
	    if (cp >= &pp->inbuf[MAXBUF-1]) {
                pushback(nextc);
		tp->class = LEXERROR;
		tp->subclass = BADLENGTH;
		return ERROR;
	    }

	    /* put the separator character back for next time
	     */
	    pushback(nextc);

	    /* check for keyword; return keyword id */
	    value = _dxfiskeyword(fp->d, pp->inbuf);
	    if (value != KW_NULL) {
		tp->token.id = value;
		tp->class = KEYWORD;

		/* some special case code here for end of header */
		if (value == KW_END) {
		    while (nextc != '\n') {
			if (feof(fp->fd) || !isprint(nextc))
			    break;
			
			nextc = input();
		    }
		    /* eat the newline */
		    if (nextc == '\n')
			input();
		}

		return OK;
	    }
 
	    if (pp->nodict) {
		tp->class = IDENTIFIER;
		tp->token.id = BADINDEX;
		return OK;
	    }

	    /* a filename, perhaps. */
	    /* string identifier - add it to the dictionary */
	    value = _dxfputdict(fp->d, pp->inbuf);
	    if (value != BADINDEX) {
		tp->class = IDENTIFIER;
		tp->token.id = value;
		return OK;
	    }
 
	    /* not a keyword or identifier we recognize - return error */
	    tp->class = LEXERROR;
	    tp->subclass = BADINPUT;
	    return ERROR;
	}
 
 
	/* not anything we recognize - return error.  don't try to skip it
         *  or recover.
	 */
	tp->class = LEXERROR;
	tp->subclass = BADINPUT;
	return ERROR;
    }
 
    /* notreached */
}
 
/* debug and errors
 */
char *_dxfprtoken(struct tinfo *t, struct dict *d)
{
    static char cbuf[512];
 
    switch(t->class) {
      case ENDOFHEADER:
        sprintf(cbuf, "end of input");
        break;
      case NUMBER:
        switch(t->subclass) {
          case INTEGER:
            sprintf(cbuf, "integer: %d", t->token.i);
            break;
          case FLOAT:
            sprintf(cbuf, "float: %f", t->token.f);
            break;
          case DOUBLE:
            sprintf(cbuf, "double: %g", t->token.d);
            break;
          case CHAR:
            sprintf(cbuf, "byte: %x", (unsigned int)t->token.c);
            break;
          default:
            sprintf(cbuf, "bad number");
            break;
        }
        break;
      case KEYWORD:
        sprintf(cbuf, "keyword '%s'", _dxflookkeyword(t->token.id));
        break;
      case IDENTIFIER:
        sprintf(cbuf, "string '%s'", _dxfdictname(d, t->token.id));
        break;
      case STRING:
        sprintf(cbuf, "string '%s'", _dxfdictname(d, t->token.id));
        break;
      case LEXERROR:
        switch(t->subclass) {
          case BADINPUT:
            sprintf(cbuf, "bad input");
            break;
          case BADQUOTE:
            sprintf(cbuf, "missing quote");
            break;
          case BADNUMBER:
            sprintf(cbuf, "bad number");
            break;
	  case BADCOMMA:
	    sprintf(cbuf, "multiple commas");
	    break;
	  case BADBRACKET:
	    sprintf(cbuf, "missing bracket");
	    break;
	  case BADLENGTH:
	    sprintf(cbuf, "line too long");
	    break;
          default:
            sprintf(cbuf, "unknown error");
            break;
        }
        break;
      default:
        sprintf(cbuf, "bad input");
        break;
    }
 
    return cbuf;
}

/* initialise the parsing routine.
 */ 
Error _dxfinitparse(struct finfo *f)
{
    f->p.inbuf = (char *)DXAllocateLocal(MAXBUF);
    if (!f->p.inbuf)
	return ERROR;

    f->p.lineno = 1;

    /* init line buffer here? */
    return _dxflexinput(f);

} 
 
/* reset to start of file.
 */
Error _dxfresetparse(struct finfo *f)
{
    fseek(f->fd, 0, 0);
    f->p.lineno = 1;
    f->t.class = -1;
 
    return _dxflexinput(f);
}

/* reset to a specific byte offset.
 */ 
Error _dxfsetparse(struct finfo *f, long byteoffset, int lines, int lineno)
{
    fseek(f->fd, byteoffset, 0);
    if (lines)
	f->p.lineno = lineno;

    f->t.class = -1;
 
    return _dxflexinput(f);
}

/* clean up any mem allocated for pinfo struct.
 */
void _dxfdeleteparse(struct finfo *f)
{
    if (!f)
	return;

    if (f->p.inbuf)
	DXFree(f->p.inbuf);

    /* free line buffer here */
}

/* set the type of the next thing to be parsed.
 */
void _dxfsetnexttype(struct finfo *f, int type)
{
    f->p.mustmatch = type;
}

/* set the type of the next thing to be parsed.
 */
void _dxfsetskipnum(struct finfo *f, int type)
{
    f->p.skipnum = type;
}

/* set the starting line number of the current object definition
 * (used for error messages)
 */
void _dxfsetstartline(struct finfo *f)
{
    f->p.startlineno = f->p.lineno;
}

int  _dxfgetstartline(struct finfo *f)
{
    return f->p.startlineno;
}

void _dxfsetprevline(struct finfo *f)
{
    f->p.prevlineno = f->p.lineno;
}

int _dxfgetprevline(struct finfo *f)
{
    return f->p.prevlineno;
}

int _dxfgetlineno(struct finfo *f)
{
    return f->p.lineno;
}

void _dxfsetdictstringmode(struct finfo *f, int value)
{
    f->p.nodict = value;
}

Error _dxfgetstringnodict(struct finfo *f)
{
    return get_string(f, NULL);
}

char *_dxfgetstringinfo(struct finfo *f, int *len)
{
    int class;

    next_class(f, &class);
    if (class != STRING)
	return NULL;

    if (len)
	*len = f->p.incount;

    return f->p.inbuf;
}

/* only skips whitespace and returns after the next newline 
 */
Error _dxfnextline(struct finfo *f)
{
    int rc;

    f->p.tonextline = 1;
    rc = _dxflexinput(f);
    f->p.tonextline = 0;

    return rc;
}

Error _dxfendnotascii(struct finfo *f)
{
    return _dxflexinput(f);
}

/* 
 * set offset of end of header --  first character after the newline
 *  following the 'end' keyword. 
 */
Error _dxfset_headerend(struct finfo *f)
{
    f->headerend = ftell(f->fd);
    return OK;
}

static Error set_lines(struct finfo *f)
{
    return OK;
}


/* token info stuff */

/* return the info about the current token without reading ahead.
 *  sets class, subclass and id if the pointers are not null.
 */
Error _dxfnexttoken(struct finfo *f, int *class, int *subclass, int *id)
{
#if DEBUG_MSG
    if (DXQueryDebug("T"))
	DXDebug("T", "last token = %s", _dxfprtoken(&f->t, f->d));
#endif

    if (class)
	*class = f->t.class;
 
    if (subclass)
	*subclass = f->t.subclass;
 
    if (id)
	*id = f->t.token.id;
 
    return f->t.class == LEXERROR ? ERROR : OK;
}
 

/* return OK if all the input parms specified match.
 *  does NOT read ahead.
 */
Error _dxfisnexttoken(struct finfo *f, int *class, int *subclass, int *id)
{
#if DEBUG_MSG
    if (DXQueryDebug("T"))
	DXDebug("T", "last token = %s", _dxfprtoken(&f->t, f->d));
#endif

    if (class && *class != f->t.class)
	return ERROR;
 
    if (subclass && *subclass != f->t.subclass)
	return ERROR;
 
    if (id && *id != f->t.token.id)
	return ERROR;
 
    return OK;
}
 
 
 
/* return id of next thing if the classes and subclasses match.
 *  (in contrast to the next routine which should be used if any
 *  subclass is ok and you just need to know what it is)
 */
Error _dxfmatchsaysub(struct finfo *f, int class, int subclass, int *id)
{
    Error rc = OK;
 
#if DEBUG_MSG
    if (DXQueryDebug("T")) {
	DXDebug("T", "looking for class %d, subclass %d", class, subclass);
	DXDebug("T", "last token = %s", _dxfprtoken(&f->t, f->d));
    }
#endif
 
    if ((f->t.class == class) && (f->t.subclass == subclass)) {

	if (id)
	    *id = f->t.token.id;

        rc = _dxflexinput(f);
        return rc;
    }
 
    return ERROR;
}
 
 
/* return subclass and id of next thing if the class matches.
 *  (in contrast to the previous routine which should be used if you
 *  only expect a specific subclass here).
 */
Error _dxfmatchanysub(struct finfo *f, int class, int *subclass, int *id)
{
    Error rc = OK;
 
#if DEBUG_MSG
    if (DXQueryDebug("T")) {
	DXDebug("T", "looking for class %d", class);
	DXDebug("T", "last token = %s", _dxfprtoken(&f->t, f->d));
    }
#endif
 
    if (f->t.class == class) {

	if (subclass)
	    *subclass = f->t.subclass;

	if (id)
	    *id = f->t.token.id;

        rc = _dxflexinput(f);
        return rc;
    }
 
    return ERROR;
}
 
 
/* return subclass and numeric value
 */
Error _dxfmatchnumber(struct finfo *f, int *subclass, Token *id)
{
    Error rc = OK;
 
#if DEBUG_MSG
    if (DXQueryDebug("T")) {
	DXDebug("T", "looking for number");
	DXDebug("T", "last token = %s", _dxfprtoken(&f->t, f->d));
    }
#endif
 
    if (f->t.class == NUMBER) {

	if (subclass)
	    *subclass = f->t.subclass;

	if (id)
	    *id = f->t.token;

        rc = _dxflexinput(f);
        return rc;
    }
 
    return ERROR;
}
 
 
/* return ok if next thing matches both class and id match. 
 *  parse ahead unless noparse flag is set.
 */
Error _dxfmatchid(struct finfo *f, int class, int id, int noparse)
{
    Error rc = OK;
 
#if DEBUG_MSG
    if (DXQueryDebug("T")) {
	DXDebug("T", "looking for class %d, id %d", class, id);
	DXDebug("T", "last token = %s", _dxfprtoken(&f->t, f->d));
    }
#endif
 
    if ((f->t.class == class) && (f->t.token.id == id)) {

	if (!noparse)
	    rc = _dxflexinput(f);

	return rc;
    }
 
    return ERROR;
}
 
/* read tokens until either another object keyword or end keyword or error.
 */
Error _dxfskip_object(struct finfo *f)
{
    Error rc = OK;
 
#if DEBUG_MSG
    if (DXQueryDebug("T")) {
	DXDebug("T", "looking for next object token");
	DXDebug("T", "last token = %s", _dxfprtoken(&f->t, f->d));
    }
#endif
    
    f->p.skipnum = 1;
    while (rc != ERROR) {
	if (f->t.class != KEYWORD && f->t.class != ENDOFHEADER) {
            rc = _dxflexinput(f);
            continue;
	}
	
	if (f->t.class == ENDOFHEADER) {
	    f->p.skipnum = 0;
	    return OK;
	}

        if (f->t.token.id != KW_OBJECT && f->t.token.id != KW_END) {
            rc = _dxflexinput(f);
            continue;
	}
	
	f->p.skipnum = 0;
	return rc;
    } 
    
    f->p.skipnum = 0;
    return ERROR;
}
 
 
/* read tokens until either another object keyword or 
 *  attribute keyword or end keyword or error.
 */
Error _dxfskip_object_or_attr(struct finfo *f)
{
    Error rc = OK;
 
#if DEBUG_MSG
    if (DXQueryDebug("T")) {
	DXDebug("T", "looking for next object or attribute token");
	DXDebug("T", "last token = %s", _dxfprtoken(&f->t, f->d));
    }
#endif
    
    f->p.skipnum = 1;
    while (rc != ERROR) {
	if (f->t.class != KEYWORD && f->t.class != ENDOFHEADER) {
            rc = _dxflexinput(f);
            continue;
	}
	
	if (f->t.class == ENDOFHEADER) {
	    f->p.skipnum = 0;
	    return OK;
	}

        if (f->t.token.id != KW_OBJECT && 
	    f->t.token.id != KW_ATTRIBUTE && 
	    f->t.token.id != KW_END) {
            rc = _dxflexinput(f);
            continue;
	}
	
	f->p.skipnum = 0;
	return rc;
    } 
    
    f->p.skipnum = 0;
    return ERROR;
}
 
 
 
/* some routines to promote ints to floats, chars to ints, etc.
 */
Error _dxfmatchbyte(struct finfo *f, byte *cp)
{
    Error rc = OK;
 
#if DEBUG_MSG
    if (DXQueryDebug("T")) {
	DXDebug("T", "looking for a byte");
	DXDebug("T", "last token = %s", _dxfprtoken(&f->t, f->d));
    }
#endif
 
    if(f->t.class == NUMBER && f->t.subclass == INTEGER) {
	if (f->t.token.i > DXD_MAX_BYTE || f->t.token.i < DXD_MIN_BYTE)
	    return ERROR;
	
	*cp = (byte)f->t.token.i;
        rc = _dxflexinput(f);
        return rc;
    } 
 
    return ERROR;
}

Error _dxfmatchubyte(struct finfo *f, ubyte *cp)
{
    Error rc = OK;
 
#if DEBUG_MSG
    if (DXQueryDebug("T")) {
	DXDebug("T", "looking for an unsigned byte");
	DXDebug("T", "last token = %s", _dxfprtoken(&f->t, f->d));
    }
#endif
 
    if(f->t.class == NUMBER && f->t.subclass == INTEGER) {
	if (f->t.token.i > DXD_MAX_UBYTE || f->t.token.i < 0)
	    return ERROR;
	
	*cp = (ubyte)f->t.token.i;
        rc = _dxflexinput(f);
        return rc;
    } 
 
    return ERROR;
}

 
Error _dxfmatchshort(struct finfo *f, short *sip)
{
    Error rc = OK;
 
#if DEBUG_MSG
    if (DXQueryDebug("T")) {
	DXDebug("T", "looking for a short int");
	DXDebug("T", "last token = %s", _dxfprtoken(&f->t, f->d));
    }
#endif
 
    if(f->t.class == NUMBER && f->t.subclass == INTEGER) {
	if (f->t.token.i > DXD_MAX_SHORT || f->t.token.i < DXD_MIN_SHORT)
	    return ERROR;
	
	*sip = (short)f->t.token.i;
        rc = _dxflexinput(f);
        return rc;
    } 
    
    return ERROR;
}
 
 
 
Error _dxfmatchushort(struct finfo *f, ushort *sip)
{
    Error rc = OK;
 
#if DEBUG_MSG
    if (DXQueryDebug("T")) {
	DXDebug("T", "looking for an unsigned short int");
	DXDebug("T", "last token = %s", _dxfprtoken(&f->t, f->d));
    }
#endif
 
    if(f->t.class == NUMBER && f->t.subclass == INTEGER) {
	if (f->t.token.i > DXD_MAX_USHORT || f->t.token.i < 0)
	    return ERROR;
	
	*sip = (short)f->t.token.i;
        rc = _dxflexinput(f);
        return rc;
    } 
    
    return ERROR;
}
 

Error _dxfmatchint(struct finfo *f, int *ip)
{
    Error rc = OK;
 
#if DEBUG_MSG
    if (DXQueryDebug("T")) {
	DXDebug("T", "looking for an integer");
	DXDebug("T", "last token = %s", _dxfprtoken(&f->t, f->d));
    }
#endif
 
    if(f->t.class == NUMBER && f->t.subclass == INTEGER) {
	*ip = f->t.token.i;
        rc = _dxflexinput(f);
        return rc;
    } 
 
    return ERROR;
}
 
Error _dxfmatchuint(struct finfo *f, uint *ip)
{
    Error rc = OK;
 
#if DEBUG_MSG
    if (DXQueryDebug("T")) {
	DXDebug("T", "looking for an unsigned integer");
	DXDebug("T", "last token = %s", _dxfprtoken(&f->t, f->d));
    }
#endif
 
    if(f->t.class == NUMBER && f->t.subclass == INTEGER) {
	*ip = (uint)f->t.token.i;
        rc = _dxflexinput(f);
        return rc;
    } 
 
    return ERROR;
}
 
Error _dxfmatchfloat(struct finfo *f, float *fp)
{
    Error rc = OK;
 
#if DEBUG_MSG
    if (DXQueryDebug("T")) {
	DXDebug("T", "looking for a float");
	DXDebug("T", "last token = %s", _dxfprtoken(&f->t, f->d));
    }
#endif
 
    if(f->t.class == NUMBER && f->t.subclass == FLOAT) {
	*fp = f->t.token.f;
        rc = _dxflexinput(f);
        return rc;
    } 
 
    if(f->t.class == NUMBER && f->t.subclass == INTEGER) {
	*fp = (float)f->t.token.i;
        rc = _dxflexinput(f);
        return rc;
    } 

    return ERROR;
}
 
 
Error _dxfmatchdouble(struct finfo *f, double *dp)
{
    Error rc = OK;
 
#if DEBUG_MSG
    if (DXQueryDebug("T")) {
	DXDebug("T", "looking for a double");
	DXDebug("T", "last token = %s", _dxfprtoken(&f->t, f->d));
    }
#endif
 
    if(f->t.class == NUMBER && f->t.subclass == DOUBLE) {
	*dp = f->t.token.d;
	rc = _dxflexinput(f);
        return rc;
    } 
 
    if(f->t.class == NUMBER && f->t.subclass == FLOAT) {
	*dp = (double)f->t.token.f;
        rc = _dxflexinput(f);
        return rc;
    } 
 
    if(f->t.class == NUMBER && f->t.subclass == INTEGER) {
	*dp = (double)f->t.token.i;
        rc = _dxflexinput(f);
        return rc;
    } 
 
    return ERROR;
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

