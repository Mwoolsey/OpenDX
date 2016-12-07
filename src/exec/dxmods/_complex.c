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
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "_compute.h"
#include "_compoper.h"
#include "_compparse.h"

/* If this char is 0, return 0, else space on */
#define input() (*_dxfccsptr++)
#define unput() (_dxfccsptr--)

static char *_dxfccsbuf;
static char *_dxfccsptr;

/*
 * these are key words used for operators (NOT functions!)
 */
static struct keytab
{
    char *name;
    int value;
} keytab [] =
{
    { "cross",		T_CROSS },
    { "dot",		T_DOT },
    { "mod",		T_MOD }
};

void
_dxfccinput(char *buffer)
{
#if defined(_CCDEBUG)
    extern int _dxfccdebug;
#endif

    _dxfccsbuf = buffer;
    _dxfccsptr = _dxfccsbuf;

#if defined(_CCDEBUG)
    _dxfccdebug = 1;
#endif
}

int
_dxfcclexerror()
{
    return (_dxfccsptr - _dxfccsbuf);
}

/*
 * analyze the input and return the next lexical token
 */
static int lastToken = T_EOF;
#define LEX_RETURN(t) return (lastToken = (t));

int
_dxfcclex(YYSTYPE *lvalp)
{
    int c;
    char _dxfcctext[MAXTOKEN];
    int _dxfccleng;
    int i;
    double d;
    int    isDouble=0;

    for (;;) {
	_dxfccleng = 0;

	c = input();

	switch (c) {
	case 0:
	    LEX_RETURN(T_EOF);
	case '(':
	    LEX_RETURN(T_LPAR);
	case ')':
	    LEX_RETURN(T_RPAR);
	case '{':
	    LEX_RETURN(T_LBRA);
	case '}':
	    LEX_RETURN(T_RBRA);
	case '[':
	    LEX_RETURN(T_LSQB);
	case ']':
	    LEX_RETURN(T_RSQB);
	case ',':
	    LEX_RETURN(T_COMMA);
	case '|':
	    c = input();
	    if (c == '|')
		LEX_RETURN(T_LOR);
	    unput();
	    LEX_RETURN('|');
	case '&':
	    c = input();
	    if (c == '&')
		LEX_RETURN(T_LAND);
	    unput();
	    LEX_RETURN('&');
	case '!':
	    c = input();
	    if (c == '=')
		LEX_RETURN(T_NE);
	    unput();
	    LEX_RETURN(T_LNOT);
	case '<':
	    c = input();
	    if (c == '=')
		LEX_RETURN(T_LE);
	    unput();
	    LEX_RETURN(T_LT);
	case '>':
	    c = input();
	    if (c == '=')
		LEX_RETURN(T_GE);
	    unput();
	    LEX_RETURN(T_GT);
	case '=':
	    c = input();
	    if (c == '=')
		LEX_RETURN(T_EQ);
	    unput();
	    LEX_RETURN(T_ASSIGN);
	case '?':
	    LEX_RETURN(T_QUEST);
	case ':':
	    LEX_RETURN(T_COLON);
	case ';':
	    LEX_RETURN(T_SEMI);
	case '+':
	    LEX_RETURN(T_PLUS);
	case '-':
	    LEX_RETURN(T_MINUS);
	case '*':
	    c = input();
	    if (c == '*')
		LEX_RETURN(T_EXP);
	    unput();
	    LEX_RETURN(T_TIMES);
	case '/':
	    LEX_RETURN(T_DIV);
	case '^':
	    LEX_RETURN(T_EXP);
	case '%':
	    LEX_RETURN(T_MOD);
	case '$':			/* input variables */
	    c = input();
	    while (isdigit(c)) {
		_dxfcctext[_dxfccleng++] = c;
		c = input();
	    }
	    _dxfcctext[_dxfccleng] = '\0';
	    lvalp->i = atoi(_dxfcctext);
	    unput();
	    if (lvalp->i >= MAX_INPUTS) {
		DXSetError(ERROR_BAD_PARAMETER, "#12070",
			 lvalp->i, MAX_INPUTS);
		LEX_RETURN(ERROR);
	    }
	    else {
		LEX_RETURN(T_INPUT);
	    }
	case ' ':			/* white space */
	case '\t':
	    break;
	case '\'':
	    {
		int i=0;
	        while ((c=input())) {
		    if (c == '\'') { 
			if (!(c = input()) || (c != '\'')) {
			    unput();
			    c = '\'';
			    break;
			}
		    }
		    lvalp->s[i++] = c;
		}
		lvalp->s[i] = '\0';
		if (c != '\'') {
		    DXSetError(ERROR_BAD_PARAMETER, "Compute string has unmatched '");
		    LEX_RETURN(ERROR);
		}
	    }
	    LEX_RETURN(T_STRING);
	case '.':			/* logical or float */
	    if (lastToken != T_INPUT && lastToken != T_RPAR && lastToken != T_NAME) {
		c = input();
		if (isdigit(c)) {
		    _dxfcctext[_dxfccleng++] = '.';
		    goto fraction;
		}
		unput();
	    }
	    LEX_RETURN(T_PERIOD);
	case '0':	/* octal or hex input */
	    _dxfcctext[_dxfccleng++] = c;
	    c = input();
	    /* Process hex input.  if there are more than 8 hex digits
	     * (non-zero), then this won't fit in a 32 bit int. 
	     */
	    if (c == 'x' || c == 'X') {
		do {
		    _dxfcctext[_dxfccleng++] = c;
		    c = input();
		} while (isxdigit(c));
		_dxfcctext[_dxfccleng] = '\0';
		if (_dxfccleng - strspn(_dxfcctext, "0x") > 8) {
		    DXSetError(ERROR_BAD_PARAMETER, "#12080", _dxfcctext);
		    LEX_RETURN(ERROR);
		}
		lvalp->i = strtol(_dxfcctext, NULL, 0);
	    }
	    /* 
	     * Process octal, becoming float if we get ., e, or E
	     * Note that we don't use strtol here because that doesn't
	     * understand the C sense of '8' and '9' as octal digits.
	     * If the we are about to multiply this number by 8, and it
	     * is greater than or equal to 4Gig >> 3, then it's an error.
	     */
	    else {
		lvalp->i = 0;
		while (isdigit(c)) {
		    if (c == '8' || c == '9')
			DXWarning ("bad octal digit `%c'", c);
		    _dxfcctext[_dxfccleng++] = c;
		    /* Check overflow (including carying the high bit of 8&9) */
		    if ((lvalp->i + ((c - '0') >> 3)) >= 0x20000000) {
			DXSetError(ERROR_BAD_PARAMETER, "#12081", _dxfcctext);
			LEX_RETURN(ERROR);
		    }
		    lvalp->i = lvalp->i * 8 + (c - '0');
		    c = input();
		}
		if (c == '.') {
		    goto fraction;
		}
		else if (c == 'e' || c == 'E') 
		    goto exponent;
	    }
	    /*
	     * Got something we didn't understand, quit (already converted)
	     */
	    unput();
	    LEX_RETURN(T_INT);
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9': /* Decimal input */
	    isDouble = 0;
	    do {
		_dxfcctext[_dxfccleng++] = c;
		c = input();
	    } while (isdigit(c));
	    if (c == '.') {
		goto fraction;
	    }
	    else if (c == 'e' || c == 'E')
		goto exponent;
	    else if (c == 'd' || c == 'D') {
		c = 'e';
		isDouble = 1;
		goto exponent;
	    }
	    unput();
	    _dxfcctext[_dxfccleng] = '\0';
	    if (strlen(_dxfcctext) > 10 || 
		(strlen(_dxfcctext) == 10 && strcmp(_dxfcctext, "2147483647") > 0)) {
		DXSetError(ERROR_BAD_PARAMETER, "#12082", _dxfcctext);
		LEX_RETURN(ERROR);
	    }
	    lvalp->i = strtol(_dxfcctext, NULL, 0);
	    LEX_RETURN(T_INT);

fraction:
	    do {
		_dxfcctext[_dxfccleng++] = c;
		c = input();
		if (c == 'e' || c == 'E')
		    goto exponent;
		else if (c == 'd' || c == 'D')
		{
		    c = 'e';
		    isDouble = 1;
		    goto exponent;
		}
	    } while (isdigit(c));
	    unput();
	    _dxfcctext[_dxfccleng] = '\0';
	    d = atof(_dxfcctext);
	    if ((d > 0 && d > DXD_MAX_FLOAT) ||
		    (d < 0 && d < -DXD_MAX_FLOAT)) {
		DXSetError(ERROR_BAD_PARAMETER, "#12085", _dxfcctext);
		LEX_RETURN(ERROR);
	    }
	    else if ((d > 0 && d < DXD_MIN_FLOAT) ||
		    (d < 0 && d > -DXD_MIN_FLOAT)) {
		DXSetError(ERROR_BAD_PARAMETER, "#12085", _dxfcctext);
		LEX_RETURN(ERROR);
	    }
	    lvalp->f = d;
	    LEX_RETURN(T_FLOAT);
exponent:
	    /* Jam the 'e' in place and get the next thing.  If it's the sign,
	     * shove it in too.  Then, suck up all digits.
	     */
	    _dxfcctext[_dxfccleng++] = c;
	    c = input();
	    if (c == '+' || c == '-') {
		_dxfcctext[_dxfccleng++] = c;
		c = input();
	    }
	    while (isdigit(c)) {
		_dxfcctext[_dxfccleng++] = c;
		c = input();
	    }
	    unput();
	    _dxfcctext[_dxfccleng] = '\0';
	    d = atof(_dxfcctext);
	    if (!isDouble) {
		if ((d > 0 && d > DXD_MAX_FLOAT) ||
			(d < 0 && d < -DXD_MAX_FLOAT)) {
		    DXSetError(ERROR_BAD_PARAMETER, "#12085", _dxfcctext);
		    LEX_RETURN(ERROR);
		}
		else if ((d > 0 && d < DXD_MIN_FLOAT) ||
			(d < 0 && d > -DXD_MIN_FLOAT)) {
		    DXSetError(ERROR_BAD_PARAMETER, "#12085", _dxfcctext);
		    LEX_RETURN(ERROR);
		}
		lvalp->f = d;
		LEX_RETURN(T_FLOAT);
	    }
	    else {
		lvalp->d = d;
		LEX_RETURN(T_DOUBLE);
	    }
	default:
	    /*
	     * most likely an identifier
	     */
	    if (isalpha(c)) {
		do {
		    _dxfcctext[_dxfccleng++] = c;
		    c = input();
		} while(isalpha(c) || isdigit(c) || c == '_');
		unput();
		_dxfcctext[_dxfccleng] = '\0';

		/* check for keywords */
		for (i=0; i < sizeof(keytab) / sizeof(keytab[0]); i++)
		    if (strcmp(keytab[i].name, _dxfcctext) == 0)
			LEX_RETURN(keytab[i].value);

		strncpy (lvalp->s, _dxfcctext, MAX_PARSE_STRING_SIZE);
		LEX_RETURN(T_NAME);
	    }
	    else {
		DXSetError(ERROR_BAD_PARAMETER, "#12095", c);
		LEX_RETURN(ERROR);
	    }
	}
    }
}
