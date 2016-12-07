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
#include <stdlib.h>
#include <math.h>
#include <errno.h>

#include "config.h"
#include "sfile.h"
#include "context.h"
#include "parse.h"
#include "yuiif.h"
#include "log.h"
#include "sysvars.h"
#include "lex.h"
#include "command.h"

#define U(x) (x)

static CmdBuf *cmdHead = NULL, *cmdTail = NULL;
static char *cmdPtr = NULL;

int
ExCheckParseBuffer()
{
    return cmdHead != NULL;
}

Error
ExParseBuffer(char *b)
{
    CmdBuf *c = NULL;
    int l = strlen(b);
    if (l)
    {
      c = (CmdBuf *)DXAllocate(sizeof(CmdBuf));
      if (! c)
          goto error;

      c->buf = (char *)DXAllocate(l+1);
      if (! c->buf)
          goto error;

      strcpy(c->buf, b);
      c->next = NULL;

      if (cmdTail)
          cmdTail->next = c;
      cmdTail = c;

      if (! cmdHead)
      {
          cmdHead = c;
          cmdPtr = c->buf;
      }
    }

    return OK;

error:
    if (c)
    {
        if (c->buf) DXFree((Pointer)c->buf);
      DXFree((Pointer)c);
    }
    return ERROR;      
}
 

static int
GetC(SFILE *yyin)
{
    if (cmdPtr)
    {
        int r = *cmdPtr++;

      if (*cmdPtr == '\0')
      {
          CmdBuf *t = cmdHead;

          cmdHead = cmdHead->next;
          if (cmdHead)
              cmdPtr = cmdHead->buf;
          else
              cmdPtr = NULL;

          if (cmdTail == t)
              cmdTail = NULL;

          DXFree((Pointer)t->buf);
          DXFree((Pointer)t);
      }

      return r;
    }
    else
        return SFILEGetChar(yyin);
} 

# define input()							\
(									\
    uipacketlen -= _dxd_exUIPacket ? 1 : 0,				\
    (									\
	(								\
	    yytchar = (yysptr > yysbuf)	? U(*--yysptr) : GetC(yyin)	\
	) == LINEFEED ? (						\
		      yylineno++,					\
		      yyCharno = yycharno,				\
		      yycharno = 0,					\
		      yytchar						\
		  )							\
	        : (							\
		      yyText[yycharno] = yytchar,			\
		      yycharno += (yycharno < YYLMAX - 1) ? 1 : 0,	\
		      yytchar						\
		  )							\
    ) == EOF ? 0							\
	     : yytchar							\
)
 
# define unput(c)		\
{				\
    if (_dxd_exUIPacket)		\
	uipacketlen++;		\
    yytchar = (c);		\
    if (yytchar == '\n')	\
    {				\
	yylineno--;		\
	yycharno = yyCharno;	\
    }				\
    else			\
	yycharno--;		\
    *yysptr++ = yytchar;	\
}
 
 
int _dxd_exUIPacket		= FALSE;
static int uipacketlen		= 0;
/*static int uipackettype		= 0;*/
 
SFILE	*yyin = NULL;
int	yylineno = 1;
int	yycharno = 0;
static int	yyCharno = 0;
char	*yyText	 = NULL;
int	yyLeng = 0;
 
static char	yysbuf[4];
static char	*yysptr		= NULL;
 
static int	reportNL	= FALSE;
 
 
/*
 * Reserved word table
 */

static
struct keytab
{
    char *name;
    int value;
} keytab [] =
{
    { "and",		L_AND },
    { "backward",	K_BACKWARD},		/* vcr */
    { "cancel",		K_CANCEL },
    { "else",		K_ELSE },
    { "false",		V_DXFALSE },
    { "forward",	K_FORWARD},		/* vcr */
    { "if",		K_IF },
    { "include",	K_INCLUDE },
    { "loop",		K_LOOP},		/* vcr */
    { "macro",		K_MACRO },
    { "not",		L_NOT },
    { "off",		K_OFF },		/* vcr */
    { "on",		K_ON },			/* vcr */
    { "or",		L_OR },
    { "palindrome",	K_PALINDROME},		/* vcr */
    { "pause",		K_PAUSE},		/* vcr */
    { "play",		K_PLAY},		/* vcr */
    { "quit",		K_QUIT },
    { "sequence",	K_VCR},			/* vcr */
    { "step",		K_STEP},		/* vcr */
    { "stop",		K_STOP},		/* vcr */
    { "then",		K_THEN },
    { "true",		V_DXTRUE }
};
 
 
/*
 * Packet type table
 */
static struct keytab packtab[] =
{
    { "$ack",		P_ACK },
    { "$bac",		P_BACKGROUND },
    { "$com",		P_COMPLETE },
    { "$dat",		P_DATA },
    { "$err",		P_ERROR },
    { "$for",		P_FOREGROUND },
    { "$imi",		P_IMPORTINFO },
    { "$imp",		P_IMPORT },
    { "$inf",		P_INFO },
    { "$int",		P_INTERRUPT },
    { "$lin",		P_LINQ },
    { "$lre",		P_LRESP },
    { "$mac",		P_MACRO },
    { "$mes",		P_MESSAGE },
    { "$sin",		P_SINQ },
    { "$sre",		P_SRESP },
    { "$sys",		P_SYSTEM },
    { "$vin",		P_VINQ },
    { "$vre",		P_VRESP }
};
 
 
/*
 * Init the lexical analyzer by allocating the error text buffer.  Since
 * we are running on 1 processor everything else can be local.  We alloc
 * it here so that we don't waste the space on all of the other processors.
 */

Error _dxf_ExInitLex ()
{
    yyText = (char *) DXAllocateLocal (YYLMAX);
    if (yyText == NULL)
	return (ERROR);
    yysptr = yysbuf;
    return (OK);
}
 
/*
 * DXFree the global storage needed by the lexical analyzer
 */

void _dxf_ExCleanupLex ()
{
    DXFree ((Pointer) yyText);
}

/*
 * Keyword table comparison routine for binsearch
 */
#if DXD_OS2_SYSCALL
int _Optlink
#else
static int
#endif
keycomp(const void *a, const void *b)
{
    struct keytab *ka = (struct keytab *)a;
    struct keytab *kb = (struct keytab *)b;
    return (strcmp (ka->name, kb->name));
}
 
 
void _dxf_ExLexInit ()
{
    _dxd_exUIPacket	= FALSE;
    uipacketlen	= 0;
    reportNL	= TRUE;
}
 
 
/*
 * Pinpoint the location of an error.
 */
void _dxf_ExLexError ()
{
    int		i;
    char	buffer[2048];
    char	*p;
 
    yyText[yycharno] = '\0';
 
    p = buffer;
    for (i = 0; i < yycharno - 1; i++)
	*p++ = '-';
    *p = '\0';

    DXUIMessage ("ERROR", "%s", yyText);
    DXUIMessage ("ERROR", "%s%c", buffer, yycharno >= YYLMAX - 1 ? '>' : '^');
}
 
 
static char *pfserr = "Float value not representable in single precision:  %s";
static char *piserr = "%s value not representable as a%s32 bit integer:  %s";

static int
ParseFString (char *yytext, float *fp)
{
    double	dval;

    *fp = (float) 0.0;

    dval = atof (yytext);

    if (dval == 0.0)
	goto ok;

    if ((dval > 0.0) && (dval <  DXD_MIN_FLOAT || dval >  DXD_MAX_FLOAT))
	goto bad;

    if ((dval < 0.0) && (dval > -DXD_MIN_FLOAT || dval < -DXD_MAX_FLOAT))
	goto bad;

ok:
    *fp = (float) dval;
    return (TRUE);

bad:
    DXUIMessage ("ERROR", pfserr, yytext);
    *fp = (float) 0.0;
    return (FALSE);
}

/*
 * Returns TRUE if the number fit into an integer, FALSE otherwise and 
 * fills in the appropriate field in yylval (either .i or .f)
 *
 * $$$$
 * NOTE:  This assumes 32 bit integers
 * $$$$
 *
 * NO longer converts to floating point.
 */

static int
ParseIString (int base, char *yytext, int yyleng, int *ip, float *fp)
{
    int		ival	= 0;
    int		i;
    int		c;

    *ip = 0;

    switch (base)
    {
	case  8:
	    for (i = 1; i < yyleng; i++)
	    {
		if (ival & 0xe0000000)		/* about to overflow? */
		    goto f8;
		ival <<= 3;
		c = yytext[i];
		if (c == '8' || c == '9')
		{
		    DXWarning ("#4360", c);
		    if (ival == 0xfffffff8)
			goto f8;
		}
		ival += c - '0';
	    }
	    *ip = ival;
	    return (TRUE);
f8:
#if 0
    {
    float	fval	= 0;
	    for (i = 1; i < yyleng; i++)
	    {
		fval *= (float) 8;
		c = yytext[i];
		if (c == '8' || c == '9')
		    DXWarning ("#4360", c);
		fval += (float) (c - '0');
	    }
	    DXWarning ("#4520", "octal");
	    *fp = fval;
     }
#endif
	    DXUIMessage ("ERROR", piserr, "Octal", " ", yytext);
	    return (FALSE);

	case 10:
	    if (yyleng <  10)
		goto i10;
	    if (yyleng == 10)
	    {
		switch (strcmp (yytext, "2147483647"))	/* MAXINT */
		{
		    case -1:
		    case 0:
			goto i10;
		    case 1:
			break;
		}
	    }
#if 0
f10:
	    for (i = 0; i < yyleng; i++)
	    {
		fval *= (float) 10;
		c = yytext[i];
		fval += (float) (c - '0');
	    }
	    DXWarning ("#4530");
	    *fp = fval;
#endif
	    DXUIMessage ("ERROR", piserr, "Decimal", " signed ", yytext);
	    return (FALSE);
i10:
	    for (i = 0; i < yyleng; i++)
	    {
		ival *= 10;
		c = yytext[i];
		ival += c - '0';
	    }
	    *ip = ival;
	    return (TRUE);

	case 16:
	    for (i = 2; i < yyleng; i++)
	    {
		if (ival & 0xf0000000)		/* about to overflow? */
		    goto f16;
		ival <<= 4;
		c = yytext[i];
		if (isdigit (c))
		    ival += c - '0';
		else if (islower (c))
		    ival += c - 'a' + 10;
		else
		    ival += c - 'A' + 10;
	    }
	    *ip = ival;
	    return (TRUE);
f16:
#if 0
	    for (i = 2; i < yyleng; i++)
	    {
		fval *= (float) 16;
		c = yytext[i];
		if (isdigit (c))
		    fval += (float) (c - '0');
		else if (islower (c))
		    fval += (float) (c - 'a' + 10);
		else
		    fval += (float) (c - 'A' + 10);
	    }
	    DXWarning ("#4520", "hex");
	    *fp = fval;
#endif
	    DXUIMessage ("ERROR", piserr, "Hex", " ", yytext);
	    return (FALSE);

	default:
	    *ip = 0;
	    *fp = (float) 0;
	    return (FALSE);
    }
}


/*
 * Parse the input and return a completed token.  The value (if any)
 * associated with the token is stored in 'yylval'
 */

#if 0
#define LEXIN	DXMarkTimeLocal ("> lex")
#define LEXOUT	DXMarkTimeLocal ("< lex")
#else
#define LEXIN
#define LEXOUT
#endif

#if 0
#define	PRINTV(v)	printf ("lex:  %d\n", v); fflush (stdout)
#else
#define	PRINTV(v)
#endif

#define	MYreturn(v)	{LEXOUT; PRINTV(v); reportNL = FALSE; return (v);}

int yylex(YYSTYPE *lvalp)
{
    int		c;
    struct	keytab *keyword;
    struct	keytab id;
    char	yytext[YYLMAX];
    int		yyleng;
    int		yytchar;
    int		i;
    char	dbg[2048];
    char	*prompt;
    int		ret;
 
    LEXIN;
    /*
     * Loop until we have a valid token.
     */
    for (;;)
    {
	yyleng = 0;
 
	c = input();
 
	switch (c)
	{
	    case -1:	
	    case 0:
	    case 26: /* CTRL Z */
                /* When running MP one of the other processors could have
                 * put the socket for the UI in non-blocking mode.
                 * The MP Parallel Master blocks in this code waiting for
                 * input. If the socket was changed to non-blocking it would
                 * return here. We want to continue and get another input.
                 */
                if(errno == EAGAIN)
                    break;
                MYreturn(T_EOF);
         
	    case '|':
		c = input();
		if (c == '|')
		    MYreturn (L_OR);
		unput(c);
		MYreturn (T_BAR);

	    case '(':		MYreturn (T_LPAR);
	    case ')':		MYreturn (T_RPAR);
	    case '{':		MYreturn (T_LBRA);
	    case '}':		MYreturn (T_RBRA);
	    case '[':		MYreturn (T_LSQB);
	    case ']':		MYreturn (T_RSQB);
	    case '=':		MYreturn (T_ASSIGN);
	    case ',':		MYreturn (T_COMMA);
	    case ';':		MYreturn (T_SEMI);


	    case ':':
		c = input();
		if (c == '=') MYreturn (T_ASSIGN);
		unput(c);
		MYreturn (T_COLON);

	    case '<':
		c = input();
		if (c == '-') MYreturn (T_ASSIGN);
		if (c == '=') MYreturn (L_LEQ);
		if (c == '>') MYreturn (L_NEQ);
		unput(c);
		MYreturn (L_LT);

	    case '+':
		c = input();
		if (c == '+') MYreturn (T_PP);
		unput(c);
		MYreturn (A_PLUS);

	    case '-':
		c = input();
		if (c == '-') MYreturn (T_MM);
		if (c == '>') MYreturn (T_RA);
		unput(c);
		MYreturn (A_MINUS);

	    case '*':
		c = input();
		if (c == '*') MYreturn (A_EXP);
		unput(c);
		MYreturn (A_TIMES);

	    case '/':
		c = input();
		if (c == '/')		/* comment to end-of-line */
		{
		    for (c = input(); c != '\n'; c = input())
			continue;
		    unput(c);
		    break;
		}
		unput(c);
		MYreturn (A_DIV);

	    case '^':			MYreturn (A_EXP);
	    case '%':			MYreturn (A_MOD);

	    case '&':
		c = input();
		if (c == '&')		MYreturn (L_AND);
		unput(c);
		MYreturn ('&');

	    case '?':			MYreturn (T_EXID);

	    /*
	     * Packet types and out of band commands
	     */

	    case '$':
		if (yycharno == 1)
		{
		    for (i = 0, c = input (); c != '\n'; c = input ())
			dbg[i++] = (char) c;
		    unput (c);
		    dbg[i] = 0;
		    _dxf_ExExecCommandStr (dbg);
		    break;
		}

		do
		{
		    yytext[yyleng++] = c;
		    c = input();
		} while (islower(c));
		yytext[yyleng] = '\0';
		unput(c);
		id.name = yytext;
		keyword = (struct keytab *)bsearch((char *)&id,
				    (char *)packtab,
				    sizeof(packtab) / sizeof(packtab[0]),
				    sizeof(packtab[0]), keycomp);
		if (keyword)
		    MYreturn (keyword->value);
		DXWarning ("#4540");
		_dxf_ExLexError();
		break;

	    case '#':			/* comment to end-of-line */
		for (c = input(); c != '\n'; c = input())
		    continue;
		unput(c);
		break;

	    case ' ':			/* white space */
	    case '\r':
	    case '\t':
	    case '\v':
		break;

	    case '\n':
		if (reportNL)
		    return (T_EOL);
		else
		    if ((_dxd_exIsatty || _dxd_exRshInput) && !SFILECharReady(yyin))
		    {
			prompt = _dxf_ExPromptGet(PROMPT_ID_CPROMPT);
			printf (prompt? prompt: EX_CPROMPT);
			fflush (stdout);
		    }
		    break;
 
	    case '"':			/* quoted strings */
	    {
		int		hitlimit  = FALSE;
		int		truncated = FALSE;
		int		dosFile = TRUE;

		/* check to see if this will be a DOS filename */

		c = input();
		if(isalpha(c)) { 
			char cc;
			cc = input();
			unput(cc);
			if(cc == ':')
				dosFile = FALSE;
		}
 
		unput(c);

		for (c = input(); c != '"' && c != '\n'; c = input())
		{
		    if (hitlimit)
			truncated = TRUE;
 
		    if (yyleng < YYLMAX_1)
		    {
			    if (c == '\\' && dosFile == TRUE)
			{
			    c = input ();
 
			    switch (c)
			    {
				case 'n':  yytext[yyleng++] = '\n';  break;
				case 't':  yytext[yyleng++] = '\t';  break;
				case 'v':  yytext[yyleng++] = '\v';  break;
				case 'b':  yytext[yyleng++] = '\b';  break;
				case 'r':  yytext[yyleng++] = '\r';  break;
				case 'f':  yytext[yyleng++] = '\14'; break;
				case 'a':  yytext[yyleng++] = '\07'; break;
				case '\\': yytext[yyleng++] = '\\';  break;
				case '?':  yytext[yyleng++] = '\?';  break;
				case '\'': yytext[yyleng++] = '\'';  break;
				case '"':  yytext[yyleng++] = '\"';  break;
				case '\n':                           break;
				case 'x':
				{
				    int		i;
 
				    c = input ();
				    if (isxdigit (c))
					i = isdigit (c) ? c - '0'
					                : tolower (c) - 'a';
				    else
				    {
					/* nothing valid after 'x' */
					unput (c);
					MYreturn (LEX_ERROR);
				    }
				    c = input ();
				    if (isxdigit (c))
				    {
				    	i *= 16;
					i += isdigit (c) ? c - '0'
					                 : tolower (c) - 'a';
				    }
				    else
					unput (c);
				
				    yytext[yyleng++] = (char) i;
				}
				break;
 
				case '0':
				case '1':
				case '2':
				case '3':
				case '4':
				case '5':
				case '6':
				case '7':
				case '8':
				case '9':
				{
				    int		i;
				    
				    i = c - '0';
				    
				    c = input ();
				    if (isdigit (c))
				    {
					i *= 8;
					i += c - '0';
					if (c == '8' || c == '9')
		                            DXWarning ("#4360", c);
				    }
				    else
				    {
				        unput (c);
				        yytext[yyleng++] = (char) i;
				        break;
				    }
				        
				    c = input ();
				    if (isdigit (c))
				    {
					i *= 8;
					i += c - '0';
					if (c == '8' || c == '9')
		                            DXWarning ("#4360", c);
				    }
				    else
				        unput (c);
				        
				    yytext[yyleng++] = (char) i;
				}
				break;
				
				default:
				    /* nothing valid after '\' */
				    unput (c);
				    MYreturn (LEX_ERROR);
			    }
			}
			else
			    yytext[yyleng++] = c;
		    }
 
		    if (yyleng == YYLMAX_1)
			hitlimit = TRUE;
		}
 
		yytext[yyleng] = '\0';
 
		if (truncated)
		    DXWarning ("#4550", "string");
 
		if (c == '\n')
		{
		    /*
		    DXWarning ("#4560");
		    */
		    unput (c);
		    MYreturn (LEX_ERROR);
		}
		lvalp->s = _dxf_ExStrSave (yytext);
		yyLeng = strlen (yytext);
		MYreturn (T_STRING);
	    }

	    case '.':			/* logical or float */
		c = input();
		if (isdigit (c))
		{
		    yytext[yyleng++] = '.';
		    goto fraction;
		}

		if (c == '.')
		    MYreturn (T_DOTDOT);

		unput (c);
		MYreturn (T_DOT);

	    case '0':			/* octal or hex number */
		yytext[yyleng++] = c;
		c = input();
		if (c == 'x' || c == 'X')
		{
		    do
		    {
			yytext[yyleng++] = c;
			c = input();
		    } while (isxdigit(c));
		    unput(c);
		    yytext[yyleng] = '\0';
 
		    /* Only found "0x" or "0X" but nothing valid after that */
		    if (yyleng == 2)
		    {
			DXWarning ("#4570", "hex");
			MYreturn (LEX_ERROR);
		    }
 
		    ret = ParseIString (16, yytext, yyleng,
					&lvalp->i, &lvalp->f);
		    if (! ret)
			_dxd_exParseError = TRUE;
		    MYreturn (T_INT);
		}
		else if (c >= '0' && c <= '9')
		    goto octal;
		else if (c == '.')
		    goto fraction;
		else if (c == 'e' || c == 'E')
		    goto exponent;
		unput (c);

		lvalp->i = 0;
		MYreturn (T_INT);

	    case '1':			/* possible octal number */
	    case '2':
	    case '3':
	    case '4':
	    case '5':
	    case '6':
	    case '7':
		if (yyleng == 0)
		    goto decimal;

octal:
		do
		{
		    yytext[yyleng++] = c;
		    c = input();
		    if (c == '.')
			goto fraction;
		    if (c == 'e' || c == 'E')
			goto exponent;
		} while (isdigit(c));
		unput (c);
		yytext[yyleng] = '\0';

		ret = ParseIString (8, yytext, yyleng, &lvalp->i, &lvalp->f);
		if (! ret)
		    _dxd_exParseError = TRUE;
		MYreturn (T_INT);
 
	    case '8':
	    case '9':
decimal:
		do
		{
		    yytext[yyleng++] = c;
		    c = input();
		    if (c == '.')
			goto fraction;
		    if (c == 'e' || c == 'E')
			goto exponent;
		} while (isdigit(c));
		unput(c);
		yytext[yyleng] = '\0';
		ret = ParseIString (10, yytext, yyleng, &lvalp->i, &lvalp->f);
		if (! ret)
		    _dxd_exParseError = TRUE;
		MYreturn (T_INT);

fraction:
		do
		{
		    yytext[yyleng++] = c;
		    c = input ();
		    if (c == 'e' || c == 'E')
			goto exponent;
		} while (isdigit (c));

		if (c == '.')
		    MYreturn (LEX_ERROR);

		unput (c);
		yytext[yyleng] = '\0';
		goto floating;

exponent:
		do
		{
		    yytext[yyleng++] = c;
		    c = input ();
		    if (c == '-' || c == '+')
			goto exponent2;
		} while (isdigit (c));
		goto exponent3;

exponent2:
		do
		{
		    yytext[yyleng++] = c;
		    c = input();
		} while (isdigit (c));
 
exponent3:
		unput (c);
		yytext[yyleng] = '\0';
		if (! isdigit (yytext[yyleng - 1]))
		{
		    DXWarning ("#4580");
		    MYreturn (LEX_ERROR);
		}

floating:
		ret = ParseFString (yytext, &lvalp->f);
		if (! ret)
		    _dxd_exParseError = TRUE;
		MYreturn (T_FLOAT);
 
default:				/* probably an identifier */
		if (isalpha(c) || c == '_' || c == '@')
		{
		    int		hitlimit  = FALSE;
		    int		truncated = FALSE;
 
		    do
		    {
			if (hitlimit)
			    truncated = TRUE;
 
			if (yyleng < YYLMAX_1)
			    yytext[yyleng++] = c;
 
			if (yyleng == YYLMAX_1)
			    hitlimit = TRUE;
 
			c = input();
		    } while(isalpha(c) ||
			    isdigit(c) ||
			    c == '_'   ||
			    c == '@'   ||
			    c == '\'' );
		    unput(c);
		    yytext[yyleng] = '\0';
 
		    if (truncated)
			DXWarning ("#4550", "identifier");
 
		    /* check for keyword */
		    id.name = yytext;
		    keyword = (struct keytab *)bsearch((char *)&id,
				    (char *)keytab,
				    sizeof(keytab) / sizeof(keytab[0]),
				    sizeof(keytab[0]), keycomp);
		    if (keyword)
			MYreturn (keyword->value);
 
		    lvalp->s = _dxf_ExStrSave (yytext);
		    MYreturn (T_ID);
		}
		MYreturn (c);
	}
    }
}
 
void yygrabdata(char *buffer, int len)
{
    int yytchar;
 
    while (len-- > 0)
	 *buffer++ = input();
    *buffer = '\0';
}
 
 
void _dxf_ExUIPacketActive (int id, int t, int n)
{
    _dxd_exUIPacket     = TRUE;
    /*uipackettype = t;*/
    uipacketlen  = n + 2;	/* +2 for the |'s around the packet's data */
 
    _dxd_exContext->graphId = id;	/* remember packet id for parse messages */
 
    yycharno = 0;
    yyCharno = 0;
}
 
void _dxf_ExUIFlushPacket ()
{
    int		yytchar;
 
    while (uipacketlen > 0)
        input ();
}

void _dxf_ExFlushNewLine()
{
    int yytchar;
    int junk;
  
    junk = input();
    if(junk != '\n')
        unput(junk);
}
 
