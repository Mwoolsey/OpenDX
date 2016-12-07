/* A Bison parser, made by GNU Bison 1.875.  */

/* Skeleton parser for Yacc-like parsing with Bison,
   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

/* As a special exception, when this file is copied by Bison into a
   Bison output file, you may use that output file without restriction.
   This special exception was added by the Free Software Foundation
   in version 1.24 of Bison.  */

/* Written by Richard Stallman by simplifying the original so called
   ``semantic'' parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 1

/* Using locations.  */
#define YYLSP_NEEDED 0



/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     L_OR = 258,
     L_AND = 259,
     L_NOT = 260,
     L_NEQ = 261,
     L_EQ = 262,
     L_GEQ = 263,
     L_LEQ = 264,
     L_GT = 265,
     L_LT = 266,
     A_MINUS = 267,
     A_PLUS = 268,
     A_MOD = 269,
     A_IDIV = 270,
     A_DIV = 271,
     A_TIMES = 272,
     A_EXP = 273,
     U_MINUS = 274,
     LEX_ERROR = 275,
     V_DXTRUE = 276,
     V_DXFALSE = 277,
     T_EOF = 278,
     T_EOL = 279,
     T_BAR = 280,
     T_LPAR = 281,
     T_RPAR = 282,
     T_LBRA = 283,
     T_RBRA = 284,
     T_LSQB = 285,
     T_RSQB = 286,
     T_ASSIGN = 287,
     T_COMMA = 288,
     T_DOT = 289,
     T_DOTDOT = 290,
     T_COLON = 291,
     T_SEMI = 292,
     T_PP = 293,
     T_MM = 294,
     T_RA = 295,
     K_BEGIN = 296,
     K_CANCEL = 297,
     K_DESCRIBE = 298,
     K_ELSE = 299,
     K_END = 300,
     K_IF = 301,
     K_INCLUDE = 302,
     K_LIST = 303,
     K_MACRO = 304,
     K_PRINT = 305,
     K_QUIT = 306,
     K_THEN = 307,
     K_BACKWARD = 308,
     K_FORWARD = 309,
     K_LOOP = 310,
     K_OFF = 311,
     K_ON = 312,
     K_PALINDROME = 313,
     K_PAUSE = 314,
     K_PLAY = 315,
     K_STEP = 316,
     K_STOP = 317,
     K_VCR = 318,
     P_INTERRUPT = 319,
     P_SYSTEM = 320,
     P_ACK = 321,
     P_MACRO = 322,
     P_FOREGROUND = 323,
     P_BACKGROUND = 324,
     P_ERROR = 325,
     P_MESSAGE = 326,
     P_INFO = 327,
     P_LINQ = 328,
     P_SINQ = 329,
     P_VINQ = 330,
     P_LRESP = 331,
     P_SRESP = 332,
     P_VRESP = 333,
     P_DATA = 334,
     P_COMPLETE = 335,
     P_IMPORT = 336,
     P_IMPORTINFO = 337,
     T_INT = 338,
     T_FLOAT = 339,
     T_STRING = 340,
     T_ID = 341,
     T_EXID = 342
   };
#endif
#define L_OR 258
#define L_AND 259
#define L_NOT 260
#define L_NEQ 261
#define L_EQ 262
#define L_GEQ 263
#define L_LEQ 264
#define L_GT 265
#define L_LT 266
#define A_MINUS 267
#define A_PLUS 268
#define A_MOD 269
#define A_IDIV 270
#define A_DIV 271
#define A_TIMES 272
#define A_EXP 273
#define U_MINUS 274
#define LEX_ERROR 275
#define V_DXTRUE 276
#define V_DXFALSE 277
#define T_EOF 278
#define T_EOL 279
#define T_BAR 280
#define T_LPAR 281
#define T_RPAR 282
#define T_LBRA 283
#define T_RBRA 284
#define T_LSQB 285
#define T_RSQB 286
#define T_ASSIGN 287
#define T_COMMA 288
#define T_DOT 289
#define T_DOTDOT 290
#define T_COLON 291
#define T_SEMI 292
#define T_PP 293
#define T_MM 294
#define T_RA 295
#define K_BEGIN 296
#define K_CANCEL 297
#define K_DESCRIBE 298
#define K_ELSE 299
#define K_END 300
#define K_IF 301
#define K_INCLUDE 302
#define K_LIST 303
#define K_MACRO 304
#define K_PRINT 305
#define K_QUIT 306
#define K_THEN 307
#define K_BACKWARD 308
#define K_FORWARD 309
#define K_LOOP 310
#define K_OFF 311
#define K_ON 312
#define K_PALINDROME 313
#define K_PAUSE 314
#define K_PLAY 315
#define K_STEP 316
#define K_STOP 317
#define K_VCR 318
#define P_INTERRUPT 319
#define P_SYSTEM 320
#define P_ACK 321
#define P_MACRO 322
#define P_FOREGROUND 323
#define P_BACKGROUND 324
#define P_ERROR 325
#define P_MESSAGE 326
#define P_INFO 327
#define P_LINQ 328
#define P_SINQ 329
#define P_VINQ 330
#define P_LRESP 331
#define P_SRESP 332
#define P_VRESP 333
#define P_DATA 334
#define P_COMPLETE 335
#define P_IMPORT 336
#define P_IMPORTINFO 337
#define T_INT 338
#define T_FLOAT 339
#define T_STRING 340
#define T_ID 341
#define T_EXID 342




/* Copy the first part of user declarations.  */
#line 1 "./yuiif.y"

/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/


#define DXD_ENABLE_SOCKET_POINTERS	/* define SFILE in arch.h, os2 */
#include <dx/dx.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <fcntl.h>
#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif
#include <string.h>
#ifndef DXD_LACKS_UNIX_UID
#include <pwd.h>
#endif

#if  defined(DXD_NON_UNIX_DIR_SEPARATOR)
#define DX_DIR_SEPARATOR ';'
#define DX_DIR_SEPARATOR_STR ";"
#else
#define DX_DIR_SEPARATOR ':'
#define DX_DIR_SEPARATOR_STR ":"
#endif

#include "exobject.h"
#include "config.h"
#include "parse.h"
#include "vcr.h"
#include "log.h"
#include "background.h"
#include "lex.h"
#include "command.h"

#include "sfile.h"

extern int	yylineno;
extern int	yycharno;
extern char	*yyText;
extern SFILE	*yyin;
extern int	yyLeng;
extern int	_dxd_exPPID;

extern int	_dxd_exRemote;


extern int 	_dxd_exUIPacket;

static int		*fnum;
static int		*linenum;
static int		*charnum;
static SFILE		**fps;
static char		**fname;

SFILE		*_dxd_exBaseFD = NULL;

int		_dxd_exParseError = 0;
node		*_dxd_exParseTree = NULL;
char		*_dxd_exParseMesg = NULL;

static int	data_len;

static int 	_pushInput	(char *name);
int		yyerror		(char *s);


#define	LIST_APPEND_CHECK(_l,_e)\
{\
    if (_l)\
	LIST_APPEND (_l, _e)\
    else\
	_dxd_exParseError = TRUE;\
}


static void one_dxrc (char *f)
{
    SFILE	*fp;

    if ((fp = fopen (f, "r")) != NULL)
    {
	fclose (fp);
	_pushInput (f);
    }
}

#define	DIFFERENT_FILES(_a,_b) \
(((_a).st_dev != (_b).st_dev) ||\
 ((_a).st_ino != (_b).st_ino) ||\
 (((_a).st_dev == 0) && ((_a).st_ino == 0)) ||\
 (((_b).st_dev == 0) && ((_b).st_ino == 0)))

void _dxf_ExReadDXRCFiles ()
{
    struct passwd	*pass;
    char		buf [1024];
    struct stat		curr;				/* local dir */
    struct stat		home;				/* home  dir */
    struct stat		sys;				/* system dir */
    struct stat		sys2;				/* system dir */
    int			n;
    char		*root;

    /*
     * Since the inputs get stacked we want them in reverse order.
     *
     * We use the stat calls to make sure that we don't include an init
     * file twice.  This could occur previously if $HOME == current dir,
     * etc.
     */

    /* If we are connected to the UI then this is called from the command
     * to get a license. The linefeed from the license command gets read
     * by the include in the dxrc system file. When we return we don't read
     * the linefeed from the license command and our char ptr is not reset.
     * this will cause a syntax error if the next command is a "$" command.
     */
    if(_dxd_exRemote)
        yycharno = 0;

    curr.st_dev = curr.st_ino = 0;
    home.st_dev = home.st_ino = 0;
    sys .st_dev = sys .st_ino = 0;

    buf[0] = '\0';

    if (getcwd (buf, 1022) == NULL)
	buf[0] = '\0';

    n = strlen (buf);
    sprintf (&buf[n], "/%s", DXRC);
    stat (buf, &curr);

    one_dxrc (DXRC);					/* local dir */

#if DXD_LACKS_UNIX_UID
    sprintf(buf, "C:\\%s", DXRC);
    stat(buf, &home);
    if(DIFFERENT_FILES (curr, home))
	one_dxrc (buf);					/* home dir */

#else
    if ((pass = getpwuid (geteuid ())) != NULL)
    {
	sprintf (buf, "%s/%s", pass->pw_dir, DXRC);
	stat (buf, &home);

	if (DIFFERENT_FILES (curr, home))
	    one_dxrc (buf);				/* home dir */
    }
#endif

    if ((root = (char *) getenv ("DXROOT")) != NULL)
    {
	sprintf (buf, "%s/%s", root, DXRC);
	stat (buf, &sys);

	if (DIFFERENT_FILES (curr, sys) &&
	    DIFFERENT_FILES (home, sys))
	    one_dxrc (buf);				/* system */

	sprintf (buf, "%s/lib/%s", root, SYSDXRC);
	stat (buf, &sys2);
        
	if (DIFFERENT_FILES (curr, sys2) &&
	    DIFFERENT_FILES (home, sys2) &&
	    DIFFERENT_FILES (sys, sys2))
	    one_dxrc (buf);				/* 2nd system */

    }
}


Error
_dxf_ExParseInit (char *name, SFILE *sf)
{
    int i;

    if (_dxf_ExInitLex() != OK)
	return (ERROR);

    fnum	= (int *)   DXAllocate (sizeof (int));
    linenum	= (int *)   DXAllocate (sizeof (int)    * MAXINCLUDES);
    charnum	= (int *)   DXAllocate (sizeof (int)    * MAXINCLUDES);
    fps		= (SFILE **)DXAllocate (sizeof (SFILE *)* MAXINCLUDES);
    fname	= (char **) DXAllocate (sizeof (char *) * MAXINCLUDES);

    if (fnum == NULL || linenum == NULL || charnum == NULL ||
	fps == NULL || fname == NULL)
	return (ERROR);

    for (i = 0; i < MAXINCLUDES; i++)
	if ((fname[i] = (char *) DXAllocate (sizeof (char) * 128)) == NULL)
	    return (ERROR);

    *fnum = 0;
    linenum[*fnum] = 1;
    charnum[*fnum] = 0;
    strcpy (fname[*fnum], name);
    fps[*fnum] = sf;
    yyin = fps[*fnum];
    _dxd_exBaseFD = sf;

    /*
     * can do .rc file processing here.
     * Unlicensed PC's will always load dxrc files here since they don't get "license" message.
     */
    _dxf_ExReadDXRCFiles ();

    return (OK);
}

#if 0
_parseCleanup()
{
    int i;

    /* close stdin */
    fclose (fps[0]);

    DXFree((Pointer) fnum);
    DXFree((Pointer) linenum);
    DXFree((Pointer) charnum);
    DXFree((Pointer) fps);

    for (i=0; i < MAXINCLUDES; i++)
	DXFree((Pointer)fname[i]);
    DXFree((Pointer)fname);

    _dxf_ExCleanupLex();
}
#endif


typedef struct
{
    char	*path;
    int		len;
} IPath;

void
_dxf_ExBeginInput()
{
    yyin = fps[*fnum];
    yylineno = linenum[*fnum];
    yycharno = charnum[*fnum];
}

void
_dxf_ExEndInput()
{
    linenum[*fnum] = yylineno;
    charnum[*fnum] = yycharno;
}


static int
_pushInput (char *name)
{
    static int		initme = TRUE;
    static int		npaths = 0;
    static IPath	*ipaths	= NULL;
    int			i;
    int			len;
    SFILE		*sf;
    FILE		*fptr;
    char		buf[4096];

    while (initme)
    {
	char		*tmp1;
	char		*tmp2;
	int		size;

	buf[0] = 0;

	initme = FALSE;

	tmp1 = (char *) getenv ("DXINCLUDE");
        tmp2 = (char *) getenv ("DXMACROS");
	buf[0]='\0';
	if (tmp1 == NULL && tmp2 == NULL) 
	    break;

        if(tmp1 && *tmp1 != '\000') {
            strcpy(buf, tmp1);
            if(tmp2 && *tmp2 != '\000')
                strcat(buf, DX_DIR_SEPARATOR_STR);
        }
        if(tmp2 && *tmp2 != '\000') 
            strcat(buf, tmp2);

	tmp2 = _dxf_ExCopyStringLocal (buf);
        if (tmp2 == NULL) {
           DXWarning("Ignoring DXINCLUDE and DXMACROS environment variables\n");
           break;
        }
	for (tmp1 = tmp2, npaths = 1; *tmp1; )
	    if (*tmp1++ == DX_DIR_SEPARATOR)
		npaths++;
	size = npaths * sizeof (IPath);
	ipaths = (IPath *) DXAllocateLocal (size);
	if (ipaths == NULL)
	{
	    DXPrintError ("Can't initialize file inclusion path");
	    break;
	}
	ExZero (ipaths, size);
	for (tmp1 = tmp2, ipaths[0].path = tmp1, i = 1; i < npaths; tmp1++)
	{
	    if (*tmp1 == DX_DIR_SEPARATOR)
	    {
		*tmp1 = '\000';
		ipaths[i++].path = tmp1 + 1;
	    }
	}
	for (i = 0; i < npaths; i++)
	    ipaths[i].len = strlen (ipaths[i].path);
    }

    if (*fnum >= MAXINCLUDES - 1)
    {
	DXPrintError ("include:  input file nesting level exceeded");
	return ERROR;
    }

    fptr = fopen (name, "r");
    for (i = 0; fptr == NULL && i < npaths; i++)
    {
	len = ipaths[i].len;
	if (len <= 0)
	    continue;
	strcpy (buf, ipaths[i].path);
#if  defined(DXD_NON_UNIX_DIR_SEPARATOR)
	buf[len] = '\\';
#else
	buf[len] = '/';
#endif
	strcpy (buf + len + 1, name);
	fptr = fopen (buf, "r");
    }

    if (fptr == NULL && getenv("DXROOT"))
    {
#if  defined(DXD_NON_UNIX_DIR_SEPARATOR)
	sprintf(buf, "%s\\lib\\%s", getenv("DXROOT"), name);
#else
	sprintf(buf, "%s/lib/%s", getenv("DXROOT"), name);
#endif
	fptr = fopen (buf, "r");
    }

    if (fptr != NULL)
    {
        sf = FILEToSFILE(fptr);

	linenum[*fnum] = yylineno;
	charnum[*fnum] = yycharno;
	(*fnum)++;
	linenum[*fnum] = 1;
	charnum[*fnum] = 0;
	strncpy (fname[*fnum], name, 128);
	fname[*fnum][127] = '\0';
	fps[*fnum] = sf;
	_dxf_ExBeginInput ();
    }
    else
    {
	DXUIMessage ("ERROR", "include:  can't open file '%s'", name);
        /* in script mode running MP you will get 2 prompts on error
         * unless you flush out the newline after the error.
         */
        if(! _dxd_exRemote)
            _dxf_ExFlushNewLine();
    }

    return OK;
}

static char PopMessage[] = "< INCLUDED FILE > ";

static int
_popInput()
{
    char	*cp;
    char	*mp;
    int		i;

    yyerror (NULL);		/* reset error file name and line number */

    if (*fnum > 0)
    {
	closeSFILE(yyin);
	yyin = fps[--(*fnum)];
	yylineno = linenum[*fnum];
	yycharno = charnum[*fnum];
				/* obliterate include file text */
	for (i = 0, cp = yyText, mp = PopMessage; i < yycharno; i++)
	    *cp++ = *mp ? *mp++ : '.';

	return (0);
    }

    return (1);
}



/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

#if ! defined (YYSTYPE) && ! defined (YYSTYPE_IS_DECLARED)
#line 420 "./yuiif.y"
typedef union YYSTYPE {
    char		c;
    int			i;
    float		f;
    char		*s;
    void		*v;
    node		*n;
} YYSTYPE;
/* Line 191 of yacc.c.  */
#line 676 "y.tab.c"
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif



/* Copy the second part of user declarations.  */


/* Line 214 of yacc.c.  */
#line 688 "y.tab.c"

#if ! defined (yyoverflow) || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# if YYSTACK_USE_ALLOCA
#  define YYSTACK_ALLOC alloca
# else
#  ifndef YYSTACK_USE_ALLOCA
#   if defined (alloca) || defined (_ALLOCA_H)
#    define YYSTACK_ALLOC alloca
#   else
#    ifdef __GNUC__
#     define YYSTACK_ALLOC __builtin_alloca
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's `empty if-body' warning. */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
# else
#  if defined (__STDC__) || defined (__cplusplus)
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   define YYSIZE_T size_t
#  endif
#  define YYSTACK_ALLOC malloc
#  define YYSTACK_FREE free
# endif
#endif /* ! defined (yyoverflow) || YYERROR_VERBOSE */


#if (! defined (yyoverflow) \
     && (! defined (__cplusplus) \
	 || (YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  short yyss;
  YYSTYPE yyvs;
  };

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (short) + sizeof (YYSTYPE))				\
      + YYSTACK_GAP_MAXIMUM)

/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if 1 < __GNUC__
#   define YYCOPY(To, From, Count) \
      __builtin_memcpy (To, From, (Count) * sizeof (*(From)))
#  else
#   define YYCOPY(To, From, Count)		\
      do					\
	{					\
	  register YYSIZE_T yyi;		\
	  for (yyi = 0; yyi < (Count); yyi++)	\
	    (To)[yyi] = (From)[yyi];		\
	}					\
      while (0)
#  endif
# endif

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack)					\
    do									\
      {									\
	YYSIZE_T yynewbytes;						\
	YYCOPY (&yyptr->Stack, Stack, yysize);				\
	Stack = &yyptr->Stack;						\
	yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
	yyptr += yynewbytes / sizeof (*yyptr);				\
      }									\
    while (0)

#endif

#if defined (__STDC__) || defined (__cplusplus)
   typedef signed char yysigned_char;
#else
   typedef short yysigned_char;
#endif

/* YYFINAL -- State number of the termination state. */
#define YYFINAL  3
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   490

/* YYNTOKENS -- Number of terminals. */
#define YYNTOKENS  88
/* YYNNTS -- Number of nonterminals. */
#define YYNNTS  96
/* YYNRULES -- Number of rules. */
#define YYNRULES  198
/* YYNRULES -- Number of states. */
#define YYNSTATES  418

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   342

#define YYTRANSLATE(YYX) 						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const unsigned char yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
      85,    86,    87
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const unsigned short yyprhs[] =
{
       0,     0,     3,     4,     7,     9,    11,    13,    15,    18,
      21,    23,    25,    27,    29,    32,    35,    38,    41,    44,
      47,    50,    54,    58,    60,    62,    63,    74,    75,    86,
      87,    98,    99,   110,   111,   122,   123,   134,   135,   146,
     147,   158,   159,   170,   171,   182,   183,   194,   195,   206,
     207,   218,   219,   230,   231,   242,   243,   254,   255,   266,
     268,   270,   272,   276,   280,   284,   286,   288,   290,   292,
     294,   296,   298,   302,   306,   310,   314,   318,   322,   326,
     330,   334,   336,   348,   356,   358,   360,   363,   368,   370,
     374,   379,   383,   385,   387,   389,   392,   394,   397,   400,
     402,   404,   408,   412,   415,   418,   420,   422,   424,   426,
     430,   432,   434,   436,   440,   444,   450,   452,   454,   456,
     460,   462,   466,   468,   470,   474,   476,   478,   482,   484,
     488,   492,   496,   500,   504,   508,   512,   515,   517,   519,
     521,   523,   527,   531,   535,   541,   549,   551,   554,   557,
     561,   565,   570,   574,   578,   580,   583,   586,   590,   594,
     599,   601,   604,   608,   610,   612,   614,   616,   619,   621,
     623,   629,   635,   641,   647,   657,   667,   669,   672,   674,
     676,   679,   681,   683,   685,   687,   689,   691,   693,   694,
     696,   698,   701,   706,   708,   712,   714,   716,   717
};

/* YYRHS -- A `-1'-separated list of the rules' RHS. */
static const short yyrhs[] =
{
      89,     0,    -1,    -1,    90,    91,    -1,   137,    -1,   130,
      -1,    94,    -1,    92,    -1,    47,    85,    -1,    47,    86,
      -1,    51,    -1,    23,    -1,    24,    -1,   171,    -1,    63,
     137,    -1,    59,   171,    -1,    60,   171,    -1,    61,   171,
      -1,    62,   171,    -1,    54,   171,    -1,    53,   171,    -1,
      58,    93,   171,    -1,    55,    93,   171,    -1,    57,    -1,
      56,    -1,    -1,    25,   167,    25,    64,    25,   167,    95,
      25,   112,    25,    -1,    -1,    25,   167,    25,    65,    25,
     167,    96,    25,   113,    25,    -1,    -1,    25,   167,    25,
      66,    25,   167,    97,    25,   116,    25,    -1,    -1,    25,
     167,    25,    67,    25,   167,    98,    25,   117,    25,    -1,
      -1,    25,   167,    25,    68,    25,   167,    99,    25,   118,
      25,    -1,    -1,    25,   167,    25,    69,    25,   167,   100,
      25,   119,    25,    -1,    -1,    25,   167,    25,    70,    25,
     167,   101,    25,   182,    25,    -1,    -1,    25,   167,    25,
      71,    25,   167,   102,    25,   120,    25,    -1,    -1,    25,
     167,    25,    72,    25,   167,   103,    25,   121,    25,    -1,
      -1,    25,   167,    25,    73,    25,   167,   104,    25,   122,
      25,    -1,    -1,    25,   167,    25,    74,    25,   167,   105,
      25,   123,    25,    -1,    -1,    25,   167,    25,    75,    25,
     167,   106,    25,   124,    25,    -1,    -1,    25,   167,    25,
      76,    25,   167,   107,    25,   125,    25,    -1,    -1,    25,
     167,    25,    77,    25,   167,   108,    25,   126,    25,    -1,
      -1,    25,   167,    25,    78,    25,   167,   109,    25,   127,
      25,    -1,    -1,    25,   167,    25,    79,    25,   167,   110,
      25,   128,    25,    -1,    -1,    25,   167,    25,    81,    25,
     167,   111,    25,   129,    25,    -1,   167,    -1,   114,    -1,
     115,    -1,   114,    33,   115,    -1,   174,    32,   155,    -1,
     180,    36,   183,    -1,   130,    -1,   137,    -1,    92,    -1,
     137,    -1,    42,    -1,   183,    -1,   183,    -1,   181,    36,
     179,    -1,   181,    36,   152,    -1,   181,    36,   179,    -1,
     181,    36,   152,    -1,   181,    36,   179,    -1,   181,    36,
     177,    -1,   181,    36,   183,    -1,   181,    36,   141,    -1,
     181,    36,   141,    -1,   183,    -1,    49,   174,    26,   131,
     172,    40,    26,   177,   172,   142,   134,    -1,    49,   174,
      26,   131,   172,   142,   134,    -1,   176,    -1,   132,    -1,
     133,   142,    -1,   132,    33,   133,   142,    -1,   174,    -1,
     174,    32,   155,    -1,   174,    32,    12,   155,    -1,    28,
     135,   173,    -1,   176,    -1,   136,    -1,   137,    -1,   136,
     137,    -1,   134,    -1,   138,   171,    -1,   147,   171,    -1,
     139,    -1,   140,    -1,   178,    32,   147,    -1,   178,    32,
     152,    -1,   174,    38,    -1,   174,    39,    -1,   176,    -1,
     140,    -1,   176,    -1,   143,    -1,    30,   144,    31,    -1,
     176,    -1,   145,    -1,   146,    -1,   145,    33,   146,    -1,
     174,    36,   155,    -1,   174,    26,   148,   172,   142,    -1,
     176,    -1,   149,    -1,   150,    -1,   149,    33,   150,    -1,
     151,    -1,   174,    32,   151,    -1,   153,    -1,   153,    -1,
     152,    33,   153,    -1,   155,    -1,   174,    -1,    26,   153,
     172,    -1,   154,    -1,   153,    18,   153,    -1,   153,    17,
     153,    -1,   153,    16,   153,    -1,   153,    15,   153,    -1,
     153,    14,   153,    -1,   153,    13,   153,    -1,   153,    12,
     153,    -1,    12,   153,    -1,   161,    -1,   158,    -1,   156,
      -1,   170,    -1,    28,   159,   173,    -1,    28,   157,   173,
      -1,    28,   160,   173,    -1,    28,   162,    35,   162,   173,
      -1,    28,   162,    35,   162,    36,   162,   173,    -1,   158,
      -1,   157,   158,    -1,    12,   158,    -1,   157,    12,   158,
      -1,   157,    33,   158,    -1,   157,    33,    12,   158,    -1,
      30,   159,    31,    -1,    30,   157,    31,    -1,   161,    -1,
     159,   161,    -1,    12,   161,    -1,   159,    12,   161,    -1,
     159,    33,   161,    -1,   159,    33,    12,   161,    -1,   170,
      -1,   160,   170,    -1,   160,    33,   170,    -1,   163,    -1,
     164,    -1,   165,    -1,   163,    -1,    12,   163,    -1,   167,
      -1,   169,    -1,    26,   166,    33,   166,   172,    -1,    26,
     166,    33,   168,   172,    -1,    26,   168,    33,   166,   172,
      -1,    26,   168,    33,   168,   172,    -1,    26,   166,    33,
     166,    33,   166,    33,   166,   172,    -1,    26,   168,    33,
     168,    33,   168,    33,   168,   172,    -1,   167,    -1,    12,
     167,    -1,    83,    -1,   169,    -1,    12,   169,    -1,    84,
      -1,    85,    -1,    37,    -1,    27,    -1,    29,    -1,    86,
      -1,    87,    -1,    -1,   176,    -1,   178,    -1,   174,   142,
      -1,   178,    33,   174,   142,    -1,   175,    -1,   179,    33,
     175,    -1,   167,    -1,   167,    -1,    -1,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const unsigned short yyrline[] =
{
       0,   611,   611,   611,   636,   637,   638,   639,   640,   646,
     655,   662,   678,   682,   688,   694,   704,   709,   714,   724,
     729,   734,   739,   746,   750,   758,   757,   770,   769,   779,
     778,   788,   787,   797,   796,   806,   805,   815,   814,   825,
     824,   834,   833,   843,   842,   852,   851,   861,   860,   870,
     869,   879,   878,   888,   887,   897,   896,   906,   905,   920,
     927,   930,   931,   934,   941,   948,   955,   956,   963,   968,
     981,   988,   995,   998,   999,  1002,  1003,  1010,  1013,  1016,
    1023,  1026,  1035,  1042,  1050,  1051,  1054,  1059,  1068,  1069,
    1073,  1084,  1094,  1095,  1098,  1102,  1114,  1115,  1119,  1130,
    1131,  1134,  1140,  1144,  1155,  1168,  1169,  1177,  1178,  1181,
    1187,  1188,  1191,  1195,  1201,  1211,  1222,  1223,  1226,  1230,
    1238,  1242,  1248,  1255,  1259,  1267,  1268,  1269,  1273,  1276,
    1280,  1284,  1288,  1292,  1296,  1300,  1304,  1317,  1318,  1319,
    1320,  1323,  1327,  1331,  1335,  1341,  1350,  1351,  1356,  1360,
    1366,  1371,  1379,  1383,  1392,  1393,  1398,  1402,  1408,  1413,
    1421,  1422,  1427,  1434,  1435,  1436,  1439,  1440,  1446,  1451,
    1458,  1467,  1476,  1485,  1496,  1507,  1520,  1521,  1527,  1530,
    1531,  1537,  1540,  1550,  1557,  1564,  1571,  1577,  1584,  1593,
    1594,  1597,  1602,  1609,  1610,  1613,  1616,  1623,  1642
};
#endif

#if YYDEBUG || YYERROR_VERBOSE
/* YYTNME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals. */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "L_OR", "L_AND", "L_NOT", "L_NEQ", "L_EQ", 
  "L_GEQ", "L_LEQ", "L_GT", "L_LT", "A_MINUS", "A_PLUS", "A_MOD", 
  "A_IDIV", "A_DIV", "A_TIMES", "A_EXP", "U_MINUS", "LEX_ERROR", 
  "V_DXTRUE", "V_DXFALSE", "T_EOF", "T_EOL", "T_BAR", "T_LPAR", "T_RPAR", 
  "T_LBRA", "T_RBRA", "T_LSQB", "T_RSQB", "T_ASSIGN", "T_COMMA", "T_DOT", 
  "T_DOTDOT", "T_COLON", "T_SEMI", "T_PP", "T_MM", "T_RA", "K_BEGIN", 
  "K_CANCEL", "K_DESCRIBE", "K_ELSE", "K_END", "K_IF", "K_INCLUDE", 
  "K_LIST", "K_MACRO", "K_PRINT", "K_QUIT", "K_THEN", "K_BACKWARD", 
  "K_FORWARD", "K_LOOP", "K_OFF", "K_ON", "K_PALINDROME", "K_PAUSE", 
  "K_PLAY", "K_STEP", "K_STOP", "K_VCR", "P_INTERRUPT", "P_SYSTEM", 
  "P_ACK", "P_MACRO", "P_FOREGROUND", "P_BACKGROUND", "P_ERROR", 
  "P_MESSAGE", "P_INFO", "P_LINQ", "P_SINQ", "P_VINQ", "P_LRESP", 
  "P_SRESP", "P_VRESP", "P_DATA", "P_COMPLETE", "P_IMPORT", 
  "P_IMPORTINFO", "T_INT", "T_FLOAT", "T_STRING", "T_ID", "T_EXID", 
  "$accept", "start", "@1", "top", "vcr", "switch", "packet", "@2", "@3", 
  "@4", "@5", "@6", "@7", "@8", "@9", "@10", "@11", "@12", "@13", "@14", 
  "@15", "@16", "@17", "@18", "interrupt_data", "system_data", "system_s", 
  "system", "ack_data", "macro_data", "foreground_data", 
  "background_data", "message_data", "info_data", "l_inquiry_data", 
  "s_inquiry_data", "v_inquiry_data", "l_response_data", 
  "s_response_data", "v_response_data", "data_data", "import_data", 
  "macro", "formal_s0", "formal_s", "formal", "block", "statement_s0", 
  "statement_s", "statement", "assignment", "f_assignment", 
  "s_assignment", "s_assignment_0", "attributes_0", "attributes", 
  "attribute_s0", "attribute_s", "attribute", "function_call", 
  "argument_s0", "argument_s", "argument", "value", "expression_s", 
  "expression", "arithmetic", "constant", "list", "tensor_s", "tensor", 
  "scalar_s", "string_s", "scalar", "nreal", "real", "complex", 
  "quaternion", "nint", "int", "nfloat", "float", "string", "semicolon", 
  "rightparen", "rightbracket", "id", "ex_id", "empty", "id_s0", "id_s", 
  "ex_id_s", "message_id", "handle", "error_data", "data", 0
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const unsigned short yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295,   296,   297,   298,   299,   300,   301,   302,   303,   304,
     305,   306,   307,   308,   309,   310,   311,   312,   313,   314,
     315,   316,   317,   318,   319,   320,   321,   322,   323,   324,
     325,   326,   327,   328,   329,   330,   331,   332,   333,   334,
     335,   336,   337,   338,   339,   340,   341,   342
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const unsigned char yyr1[] =
{
       0,    88,    90,    89,    91,    91,    91,    91,    91,    91,
      91,    91,    91,    91,    92,    92,    92,    92,    92,    92,
      92,    92,    92,    93,    93,    95,    94,    96,    94,    97,
      94,    98,    94,    99,    94,   100,    94,   101,    94,   102,
      94,   103,    94,   104,    94,   105,    94,   106,    94,   107,
      94,   108,    94,   109,    94,   110,    94,   111,    94,   112,
     113,   114,   114,   115,   116,   117,   118,   118,   119,   119,
     120,   121,   122,   123,   123,   124,   124,   125,   126,   127,
     128,   129,   130,   130,   131,   131,   132,   132,   133,   133,
     133,   134,   135,   135,   136,   136,   137,   137,   137,   138,
     138,   139,   140,   140,   140,   141,   141,   142,   142,   143,
     144,   144,   145,   145,   146,   147,   148,   148,   149,   149,
     150,   150,   151,   152,   152,   153,   153,   153,   153,   154,
     154,   154,   154,   154,   154,   154,   154,   155,   155,   155,
     155,   156,   156,   156,   156,   156,   157,   157,   157,   157,
     157,   157,   158,   158,   159,   159,   159,   159,   159,   159,
     160,   160,   160,   161,   161,   161,   162,   162,   163,   163,
     164,   164,   164,   164,   165,   165,   166,   166,   167,   168,
     168,   169,   170,   171,   172,   173,   174,   175,   176,   177,
     177,   178,   178,   179,   179,   180,   181,   182,   183
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const unsigned char yyr2[] =
{
       0,     2,     0,     2,     1,     1,     1,     1,     2,     2,
       1,     1,     1,     1,     2,     2,     2,     2,     2,     2,
       2,     3,     3,     1,     1,     0,    10,     0,    10,     0,
      10,     0,    10,     0,    10,     0,    10,     0,    10,     0,
      10,     0,    10,     0,    10,     0,    10,     0,    10,     0,
      10,     0,    10,     0,    10,     0,    10,     0,    10,     1,
       1,     1,     3,     3,     3,     1,     1,     1,     1,     1,
       1,     1,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     1,    11,     7,     1,     1,     2,     4,     1,     3,
       4,     3,     1,     1,     1,     2,     1,     2,     2,     1,
       1,     3,     3,     2,     2,     1,     1,     1,     1,     3,
       1,     1,     1,     3,     3,     5,     1,     1,     1,     3,
       1,     3,     1,     1,     3,     1,     1,     3,     1,     3,
       3,     3,     3,     3,     3,     3,     2,     1,     1,     1,
       1,     3,     3,     3,     5,     7,     1,     2,     2,     3,
       3,     4,     3,     3,     1,     2,     2,     3,     3,     4,
       1,     2,     3,     1,     1,     1,     1,     2,     1,     1,
       5,     5,     5,     5,     9,     9,     1,     2,     1,     1,
       2,     1,     1,     1,     1,     1,     1,     1,     0,     1,
       1,     2,     4,     1,     3,     1,     1,     0,     0
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const unsigned char yydefact[] =
{
       2,     0,     0,     1,    11,    12,     0,   188,   183,     0,
       0,    10,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   186,     3,     7,     6,     5,    96,     4,     0,    99,
     100,     0,    13,   188,     0,   178,     0,     0,    93,    94,
      92,     8,     9,     0,    20,    19,    24,    23,     0,     0,
      15,    16,    17,    18,    14,    97,    98,   188,   188,   103,
     104,   191,   108,   107,     0,     0,     0,   185,    91,    95,
     188,    22,    21,     0,     0,     0,     0,   181,   182,     0,
     117,   118,   120,   122,   128,   125,   139,   138,   137,   163,
     164,   165,   168,   169,   140,   126,   116,     0,   111,   112,
       0,   110,   101,   102,   123,   126,   188,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    85,   188,    88,    84,   136,
     126,     0,     0,     0,   168,     0,   169,     0,     0,     0,
     146,     0,     0,   154,     0,   163,   160,     0,     0,     0,
     184,   188,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   109,     0,     0,     0,   192,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   188,     0,    86,     0,   168,   169,   127,
       0,     0,   148,   156,   163,     0,   176,   179,     0,     0,
     147,   142,     0,     0,   155,   141,     0,   161,   143,     0,
     153,   152,   115,   119,   135,   134,   133,   132,   131,   130,
     129,   121,   113,   114,   124,    25,    27,    29,    31,    33,
      35,    37,    39,    41,    43,    45,    47,    49,    51,    53,
      55,    57,     0,     0,   188,     0,    89,     0,     0,     0,
       0,   177,   180,   149,     0,   150,   157,     0,   158,   162,
       0,     0,   166,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     188,    83,    87,    90,     0,   170,   171,   172,     0,   173,
     151,   159,   167,     0,   144,     0,     0,     0,     0,     0,
       0,   197,   198,   198,     0,     0,     0,     0,     0,     0,
       0,   198,   188,   189,     0,   190,     0,     0,     0,     0,
       0,     0,    59,     0,    60,    61,     0,     0,   195,     0,
       0,    65,    67,     0,    66,    69,     0,    68,     0,     0,
      70,     0,    71,     0,   196,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    81,
     188,     0,     0,   145,    26,    28,     0,     0,    30,   198,
      32,    34,    36,    38,    40,    42,    44,     0,    46,     0,
      48,     0,    50,   188,    52,   198,    54,   188,    56,   188,
      58,     0,     0,     0,    62,    63,    64,   187,   193,    72,
      73,    74,    75,    76,    77,    78,   106,    79,   188,   105,
       0,    80,    82,   174,   175,     0,     0,   194
};

/* YYDEFGOTO[NTERM-NUM]. */
static const short yydefgoto[] =
{
      -1,     1,     2,    22,    23,    48,    24,   263,   264,   265,
     266,   267,   268,   269,   270,   271,   272,   273,   274,   275,
     276,   277,   278,   279,   321,   323,   324,   325,   327,   330,
     333,   336,   339,   341,   343,   346,   348,   350,   352,   354,
     356,   358,    25,   124,   125,   126,    26,    37,    38,    27,
      28,    29,    30,   407,    61,    62,    97,    98,    99,    31,
      79,    80,    81,    82,   103,   104,    84,    85,    86,   139,
      87,   141,   142,    88,   144,    89,    90,    91,   133,    92,
     135,    93,    94,    32,   151,    68,   130,   398,    63,   314,
      34,   399,   329,   345,   338,   340
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -316
static const short yypact[] =
{
    -316,    30,   308,  -316,  -316,  -316,   -68,     0,  -316,    79,
      -6,  -316,    46,    46,   121,   121,    46,    46,    46,    46,
       0,  -316,  -316,  -316,  -316,  -316,  -316,  -316,    46,  -316,
    -316,    46,  -316,   228,   172,  -316,    77,    96,     0,  -316,
    -316,  -316,  -316,   106,  -316,  -316,  -316,  -316,    46,    46,
    -316,  -316,  -316,  -316,  -316,  -316,  -316,   220,    -6,  -316,
    -316,  -316,  -316,  -316,   220,    -6,   382,  -316,  -316,  -316,
      -6,  -316,  -316,   220,   233,   227,    83,  -316,  -316,   116,
     113,  -316,  -316,   334,  -316,  -316,  -316,  -316,  -316,  -316,
    -316,  -316,  -316,  -316,  -316,   122,  -316,   148,   125,  -316,
     170,  -316,  -316,   150,   334,   186,   191,   203,   212,   222,
     224,   235,   237,   239,   240,   243,   244,   245,   246,   251,
     252,   256,   262,   263,   116,   200,   191,   192,  -316,  -316,
    -316,   220,   404,   257,   266,   268,   274,     1,    10,    37,
    -316,    31,    25,  -316,   254,   273,  -316,     1,   111,   309,
    -316,   191,   220,   220,   220,   220,   220,   220,   220,   220,
     220,  -316,    -6,   189,   220,  -316,   -68,   -68,   -68,   -68,
     -68,   -68,   -68,   -68,   -68,   -68,   -68,   -68,   -68,   -68,
     -68,   -68,   -68,   108,    -6,  -316,   208,   276,   281,  -316,
      10,    10,  -316,  -316,   285,   147,  -316,  -316,   299,    11,
    -316,  -316,    22,    35,  -316,  -316,   253,  -316,  -316,    47,
    -316,  -316,  -316,  -316,   226,   226,   312,   312,   312,   312,
    -316,  -316,  -316,  -316,   334,  -316,  -316,  -316,  -316,  -316,
    -316,  -316,  -316,  -316,  -316,  -316,  -316,  -316,  -316,  -316,
    -316,  -316,   311,   313,   191,   189,  -316,    44,   116,   116,
      94,  -316,  -316,  -316,   299,  -316,  -316,    22,  -316,  -316,
     147,    15,  -316,   314,   318,   319,   328,   329,   331,   333,
     335,   340,   348,   349,   350,   351,   352,   355,   358,   361,
      -6,  -316,  -316,  -316,    -8,  -316,  -316,  -316,    33,  -316,
    -316,  -316,  -316,    47,  -316,   -68,    -6,   -68,   339,   344,
      54,  -316,  -316,  -316,   -68,   -68,   -68,   -68,   -68,   -68,
     -68,  -316,   191,  -316,   116,   356,   -68,   357,   307,   362,
      96,   371,  -316,   375,   368,  -316,   378,   386,  -316,   387,
     400,  -316,  -316,   401,  -316,  -316,   403,  -316,   413,   414,
    -316,   415,  -316,   416,  -316,   393,   417,   407,   437,   428,
     440,   430,   442,   432,   444,   434,   446,   436,   448,  -316,
     191,    -8,    33,  -316,  -316,  -316,    -6,   189,  -316,  -316,
    -316,  -316,  -316,  -316,  -316,  -316,  -316,   388,  -316,   199,
    -316,   199,  -316,    -6,  -316,  -316,  -316,    -6,  -316,    -6,
    -316,   313,   116,   116,  -316,  -316,  -316,  -316,  -316,   441,
     150,   441,   150,   441,  -316,  -316,  -316,  -316,   184,  -316,
     219,  -316,  -316,  -316,  -316,   388,   220,  -316
};

/* YYPGOTO[NTERM-NUM].  */
static const short yypgoto[] =
{
    -316,  -316,  -316,  -316,   177,   462,  -316,  -316,  -316,  -316,
    -316,  -316,  -316,  -316,  -316,  -316,  -316,  -316,  -316,  -316,
    -316,  -316,  -316,  -316,  -316,  -316,  -316,   112,  -316,  -316,
    -316,  -316,  -316,  -316,  -316,  -316,  -316,  -316,  -316,  -316,
    -316,  -316,   181,  -316,  -316,   296,  -236,  -316,  -316,    -4,
    -316,  -316,  -315,    92,  -105,  -316,  -316,  -316,   320,   419,
    -316,  -316,   332,   325,  -281,    16,  -316,  -157,  -316,   410,
     -36,   411,  -316,   -50,  -189,   -58,  -316,  -316,  -158,    18,
    -181,   -62,   -61,   396,  -113,  -104,    -2,    73,    -5,   107,
    -267,  -220,  -316,   127,  -316,  -261
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -181
static const short yytable[] =
{
      33,   165,    40,    39,   316,    33,   223,   281,    43,   248,
     250,   183,   136,   315,   146,    35,    54,   145,    33,   189,
     261,   185,   195,   254,    36,   143,   143,   138,     7,   246,
       3,    76,   247,   249,    69,   201,    33,   205,   208,   140,
     140,    76,   342,   202,    67,   318,   212,   257,   138,   198,
     359,   293,    96,   101,    67,    95,   100,   138,   206,   260,
      67,   138,   105,   106,   203,   128,    67,    76,   127,   188,
     199,   150,   406,    83,   406,    35,   197,   284,   243,   194,
      21,   207,     7,     8,    35,    77,    21,   193,   283,   129,
     132,   204,   134,    35,    77,   147,   335,   193,   400,   204,
     402,   192,    66,   200,   320,    35,    77,   319,   396,   138,
      78,   192,   200,    76,    35,    77,   315,    77,    35,    77,
     410,   150,   410,   198,   405,    67,   317,   288,   197,   197,
      35,    77,    70,   252,   285,   286,   287,   289,    58,   282,
      21,    76,   210,   150,   199,   259,   152,   129,   242,   187,
      95,   262,   256,   258,   160,   412,   196,   294,   162,   401,
     100,   403,   253,   255,    41,    42,    35,    77,    83,   214,
     215,   216,   217,   218,   219,   220,    83,    46,    47,   161,
     224,   393,   127,   164,   225,   226,   227,   228,   229,   230,
     231,   232,   233,   234,   235,   236,   237,   238,   239,   240,
     241,   360,   292,   392,    64,    65,   163,   291,   196,   196,
     395,    73,    57,   251,    58,   138,   363,    75,   290,    76,
     245,    58,    59,    60,   186,    74,   197,    75,   166,    76,
      35,    77,    73,   184,   138,   262,    75,   167,    76,   137,
     155,   156,   157,   158,   159,   131,    74,   168,    75,   169,
      76,   416,    65,   138,    57,   391,   252,    76,    58,    74,
     170,    75,   171,    76,   172,   173,    59,    60,   174,   175,
     176,   177,    35,    77,    78,   313,   178,   179,   312,   413,
     414,   180,    35,    77,    78,    21,   397,   181,   182,   209,
     190,    35,    77,    78,   326,   334,   337,    33,    33,  -176,
     197,   191,   196,    35,    77,    78,    21,  -179,  -166,  -177,
      35,    77,    78,   322,  -180,   328,    35,    77,    78,    21,
    -167,   202,   344,   344,   344,   344,   344,   344,   344,    76,
     159,     4,     5,     6,   251,   138,     7,   280,    78,   295,
     211,     7,   203,   296,   297,     8,   153,   154,   155,   156,
     157,   158,   159,   298,   299,     9,   300,    10,   301,    11,
     302,    12,    13,    14,   326,   303,    15,    16,    17,    18,
      19,    20,     7,   304,   305,   306,   307,   308,   313,   196,
     309,   312,   409,   310,   409,   408,   311,   408,    10,    65,
     361,    77,    35,    77,    21,   362,   364,    12,    13,    14,
     365,   366,    15,    16,    17,    18,    19,    20,    44,    45,
     367,   368,    50,    51,    52,    53,   153,   154,   155,   156,
     157,   158,   159,   369,    55,   370,   371,    56,   372,   377,
      21,   150,   347,   349,   351,   353,   355,   357,   373,   374,
     375,   376,   378,   379,    71,    72,   107,   108,   109,   110,
     111,   112,   113,   114,   115,   116,   117,   118,   119,   120,
     121,   122,   380,   123,   381,   382,   383,   384,   385,   386,
     387,   388,   389,   390,   415,   397,   332,    49,   394,   331,
     244,   411,   222,   102,   213,   221,   148,   149,   417,     0,
     404
};

static const short yycheck[] =
{
       2,   106,     7,     7,    12,     7,   163,   243,    10,   190,
     191,   124,    74,   280,    75,    83,    20,    75,    20,   132,
     209,   126,    12,    12,     6,    75,    76,    26,    28,   186,
       0,    30,   190,   191,    38,   139,    38,   141,   142,    75,
      76,    30,   303,    12,    29,    12,   151,    12,    26,    12,
     311,    36,    57,    58,    29,    57,    58,    26,    33,    12,
      29,    26,    64,    65,    33,    70,    29,    30,    70,   131,
      33,    27,   387,    57,   389,    83,   138,    33,   183,   137,
      86,   142,    28,    37,    83,    84,    86,   137,   245,    73,
      74,   141,    74,    83,    84,    12,    42,   147,   379,   149,
     381,   137,    25,   139,   293,    83,    84,   288,   369,    26,
      85,   147,   148,    30,    83,    84,   383,    84,    83,    84,
     387,    27,   389,    12,   385,    29,   284,    33,   190,   191,
      83,    84,    26,   195,   247,   248,   249,   250,    30,   244,
      86,    30,    31,    27,    33,   206,    33,   131,    40,   131,
     152,   209,   202,   203,    32,   391,   138,   261,    33,   379,
     162,   381,   198,   199,    85,    86,    83,    84,   152,   153,
     154,   155,   156,   157,   158,   159,   160,    56,    57,    31,
     164,   362,   184,    33,   166,   167,   168,   169,   170,   171,
     172,   173,   174,   175,   176,   177,   178,   179,   180,   181,
     182,   314,   260,   361,    32,    33,    36,   257,   190,   191,
     367,    12,    26,   195,    30,    26,   320,    28,   254,    30,
      12,    30,    38,    39,    32,    26,   288,    28,    25,    30,
      83,    84,    12,    33,    26,   293,    28,    25,    30,    12,
      14,    15,    16,    17,    18,    12,    26,    25,    28,    25,
      30,    32,    33,    26,    26,   360,   318,    30,    30,    26,
      25,    28,    25,    30,    25,    25,    38,    39,    25,    25,
      25,    25,    83,    84,    85,   280,    25,    25,   280,   392,
     393,    25,    83,    84,    85,    86,    87,    25,    25,    35,
      33,    83,    84,    85,   296,   299,   300,   299,   300,    33,
     362,    33,   284,    83,    84,    85,    86,    33,    35,    33,
      83,    84,    85,   295,    33,   297,    83,    84,    85,    86,
      35,    12,   304,   305,   306,   307,   308,   309,   310,    30,
      18,    23,    24,    25,   316,    26,    28,    26,    85,    25,
      31,    28,    33,    25,    25,    37,    12,    13,    14,    15,
      16,    17,    18,    25,    25,    47,    25,    49,    25,    51,
      25,    53,    54,    55,   366,    25,    58,    59,    60,    61,
      62,    63,    28,    25,    25,    25,    25,    25,   383,   361,
      25,   383,   387,    25,   389,   387,    25,   389,    49,    33,
      33,    84,    83,    84,    86,    33,    25,    53,    54,    55,
      25,    33,    58,    59,    60,    61,    62,    63,    12,    13,
      32,    25,    16,    17,    18,    19,    12,    13,    14,    15,
      16,    17,    18,    36,    28,    25,    25,    31,    25,    36,
      86,    27,   305,   306,   307,   308,   309,   310,    25,    25,
      25,    25,    25,    36,    48,    49,    64,    65,    66,    67,
      68,    69,    70,    71,    72,    73,    74,    75,    76,    77,
      78,    79,    25,    81,    36,    25,    36,    25,    36,    25,
      36,    25,    36,    25,    33,    87,   299,    15,   366,   298,
     184,   389,   162,    64,   152,   160,    76,    76,   415,    -1,
     383
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const unsigned char yystos[] =
{
       0,    89,    90,     0,    23,    24,    25,    28,    37,    47,
      49,    51,    53,    54,    55,    58,    59,    60,    61,    62,
      63,    86,    91,    92,    94,   130,   134,   137,   138,   139,
     140,   147,   171,   174,   178,    83,   167,   135,   136,   137,
     176,    85,    86,   174,   171,   171,    56,    57,    93,    93,
     171,   171,   171,   171,   137,   171,   171,    26,    30,    38,
      39,   142,   143,   176,    32,    33,    25,    29,   173,   137,
      26,   171,   171,    12,    26,    28,    30,    84,    85,   148,
     149,   150,   151,   153,   154,   155,   156,   158,   161,   163,
     164,   165,   167,   169,   170,   174,   176,   144,   145,   146,
     174,   176,   147,   152,   153,   174,   174,    64,    65,    66,
      67,    68,    69,    70,    71,    72,    73,    74,    75,    76,
      77,    78,    79,    81,   131,   132,   133,   174,   176,   153,
     174,    12,   153,   166,   167,   168,   169,    12,    26,   157,
     158,   159,   160,   161,   162,   163,   170,    12,   157,   159,
      27,   172,    33,    12,    13,    14,    15,    16,    17,    18,
      32,    31,    33,    36,    33,   142,    25,    25,    25,    25,
      25,    25,    25,    25,    25,    25,    25,    25,    25,    25,
      25,    25,    25,   172,    33,   142,    32,   167,   169,   172,
      33,    33,   158,   161,   163,    12,   167,   169,    12,    33,
     158,   173,    12,    33,   161,   173,    33,   170,   173,    35,
      31,    31,   142,   150,   153,   153,   153,   153,   153,   153,
     153,   151,   146,   155,   153,   167,   167,   167,   167,   167,
     167,   167,   167,   167,   167,   167,   167,   167,   167,   167,
     167,   167,    40,   142,   133,    12,   155,   166,   168,   166,
     168,   167,   169,   158,    12,   158,   161,    12,   161,   170,
      12,   162,   163,    95,    96,    97,    98,    99,   100,   101,
     102,   103,   104,   105,   106,   107,   108,   109,   110,   111,
      26,   134,   142,   155,    33,   172,   172,   172,    33,   172,
     158,   161,   163,    36,   173,    25,    25,    25,    25,    25,
      25,    25,    25,    25,    25,    25,    25,    25,    25,    25,
      25,    25,   174,   176,   177,   178,    12,   166,    12,   168,
     162,   112,   167,   113,   114,   115,   174,   116,   167,   180,
     117,   130,    92,   118,   137,    42,   119,   137,   182,   120,
     183,   121,   183,   122,   167,   181,   123,   181,   124,   181,
     125,   181,   126,   181,   127,   181,   128,   181,   129,   183,
     172,    33,    33,   173,    25,    25,    33,    32,    25,    36,
      25,    25,    25,    25,    25,    25,    25,    36,    25,    36,
      25,    36,    25,    36,    25,    36,    25,    36,    25,    36,
      25,   142,   166,   168,   115,   155,   183,    87,   175,   179,
     152,   179,   152,   179,   177,   183,   140,   141,   174,   176,
     178,   141,   134,   172,   172,    33,    32,   175
};

#if ! defined (YYSIZE_T) && defined (__SIZE_TYPE__)
# define YYSIZE_T __SIZE_TYPE__
#endif
#if ! defined (YYSIZE_T) && defined (size_t)
# define YYSIZE_T size_t
#endif
#if ! defined (YYSIZE_T)
# if defined (__STDC__) || defined (__cplusplus)
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# endif
#endif
#if ! defined (YYSIZE_T)
# define YYSIZE_T unsigned int
#endif

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		(-2)
#define YYEOF		0

#define YYACCEPT	goto yyacceptlab
#define YYABORT		goto yyabortlab
#define YYERROR		goto yyerrlab1

/* Like YYERROR except do call yyerror.  This remains here temporarily
   to ease the transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */

#define YYFAIL		goto yyerrlab

#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)					\
do								\
  if (yychar == YYEMPTY && yylen == 1)				\
    {								\
      yychar = (Token);						\
      yylval = (Value);						\
      yytoken = YYTRANSLATE (yychar);				\
      YYPOPSTACK;						\
      goto yybackup;						\
    }								\
  else								\
    { 								\
      yyerror ("syntax error: cannot back up");\
      YYERROR;							\
    }								\
while (0)

#define YYTERROR	1
#define YYERRCODE	256

/* YYLLOC_DEFAULT -- Compute the default location (before the actions
   are run).  */

#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)         \
  Current.first_line   = Rhs[1].first_line;      \
  Current.first_column = Rhs[1].first_column;    \
  Current.last_line    = Rhs[N].last_line;       \
  Current.last_column  = Rhs[N].last_column;
#endif

/* YYLEX -- calling `yylex' with the right arguments.  */

#ifdef YYLEX_PARAM
# define YYLEX yylex (&yylval, YYLEX_PARAM)
#else
# define YYLEX yylex (&yylval)
#endif

/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)			\
do {						\
  if (yydebug)					\
    YYFPRINTF Args;				\
} while (0)

# define YYDSYMPRINT(Args)			\
do {						\
  if (yydebug)					\
    yysymprint Args;				\
} while (0)

# define YYDSYMPRINTF(Title, Token, Value, Location)		\
do {								\
  if (yydebug)							\
    {								\
      YYFPRINTF (stderr, "%s ", Title);				\
      yysymprint (stderr, 					\
                  Token, Value);	\
      YYFPRINTF (stderr, "\n");					\
    }								\
} while (0)

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (cinluded).                                                   |
`------------------------------------------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yy_stack_print (short *bottom, short *top)
#else
static void
yy_stack_print (bottom, top)
    short *bottom;
    short *top;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (/* Nothing. */; bottom <= top; ++bottom)
    YYFPRINTF (stderr, " %d", *bottom);
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)				\
do {								\
  if (yydebug)							\
    yy_stack_print ((Bottom), (Top));				\
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yy_reduce_print (int yyrule)
#else
static void
yy_reduce_print (yyrule)
    int yyrule;
#endif
{
  int yyi;
  unsigned int yylineno = yyrline[yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %u), ",
             yyrule - 1, yylineno);
  /* Print the symbols being reduced, and their result.  */
  for (yyi = yyprhs[yyrule]; 0 <= yyrhs[yyi]; yyi++)
    YYFPRINTF (stderr, "%s ", yytname [yyrhs[yyi]]);
  YYFPRINTF (stderr, "-> %s\n", yytname [yyr1[yyrule]]);
}

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (yydebug)				\
    yy_reduce_print (Rule);		\
} while (0)

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YYDSYMPRINT(Args)
# define YYDSYMPRINTF(Title, Token, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef	YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   SIZE_MAX < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#if YYMAXDEPTH == 0
# undef YYMAXDEPTH
#endif

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif



#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined (__GLIBC__) && defined (_STRING_H)
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
static YYSIZE_T
#   if defined (__STDC__) || defined (__cplusplus)
yystrlen (const char *yystr)
#   else
yystrlen (yystr)
     const char *yystr;
#   endif
{
  register const char *yys = yystr;

  while (*yys++ != '\0')
    continue;

  return yys - yystr - 1;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined (__GLIBC__) && defined (_STRING_H) && defined (_GNU_SOURCE)
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
static char *
#   if defined (__STDC__) || defined (__cplusplus)
yystpcpy (char *yydest, const char *yysrc)
#   else
yystpcpy (yydest, yysrc)
     char *yydest;
     const char *yysrc;
#   endif
{
  register char *yyd = yydest;
  register const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

#endif /* !YYERROR_VERBOSE */



#if YYDEBUG
/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yysymprint (FILE *yyoutput, int yytype, YYSTYPE *yyvaluep)
#else
static void
yysymprint (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE *yyvaluep;
#endif
{
  /* Pacify ``unused variable'' warnings.  */
  (void) yyvaluep;

  if (yytype < YYNTOKENS)
    {
      YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
# ifdef YYPRINT
      YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# endif
    }
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);

  switch (yytype)
    {
      default:
        break;
    }
  YYFPRINTF (yyoutput, ")");
}

#endif /* ! YYDEBUG */
/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yydestruct (int yytype, YYSTYPE *yyvaluep)
#else
static void
yydestruct (yytype, yyvaluep)
    int yytype;
    YYSTYPE *yyvaluep;
#endif
{
  /* Pacify ``unused variable'' warnings.  */
  (void) yyvaluep;

  switch (yytype)
    {

      default:
        break;
    }
}


/* Prevent warnings from -Wmissing-prototypes.  */

#ifdef YYPARSE_PARAM
# if defined (__STDC__) || defined (__cplusplus)
int yyparse (void *YYPARSE_PARAM);
# else
int yyparse ();
# endif
#else /* ! YYPARSE_PARAM */
#if defined (__STDC__) || defined (__cplusplus)
int yyparse (void);
#else
int yyparse ();
#endif
#endif /* ! YYPARSE_PARAM */






/*----------.
| yyparse.  |
`----------*/

#ifdef YYPARSE_PARAM
# if defined (__STDC__) || defined (__cplusplus)
int yyparse (void *YYPARSE_PARAM)
# else
int yyparse (YYPARSE_PARAM)
  void *YYPARSE_PARAM;
# endif
#else /* ! YYPARSE_PARAM */
#if defined (__STDC__) || defined (__cplusplus)
int
yyparse (void)
#else
int
yyparse ()

#endif
#endif
{
  /* The lookahead symbol.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;

/* Number of syntax errors so far.  */
int yynerrs;

  register int yystate;
  register int yyn;
  int yyresult;
  /* Number of tokens to shift before error messages enabled.  */
  int yyerrstatus;
  /* Lookahead token as an internal (translated) token number.  */
  int yytoken = 0;

  /* Three stacks and their tools:
     `yyss': related to states,
     `yyvs': related to semantic values,
     `yyls': related to locations.

     Refer to the stacks thru separate pointers, to allow yyoverflow
     to reallocate them elsewhere.  */

  /* The state stack.  */
  short	yyssa[YYINITDEPTH];
  short *yyss = yyssa;
  register short *yyssp;

  /* The semantic value stack.  */
  YYSTYPE yyvsa[YYINITDEPTH];
  YYSTYPE *yyvs = yyvsa;
  register YYSTYPE *yyvsp;



#define YYPOPSTACK   (yyvsp--, yyssp--)

  YYSIZE_T yystacksize = YYINITDEPTH;

  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;


  /* When reducing, the number of symbols on the RHS of the reduced
     rule.  */
  int yylen;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY;		/* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */

  yyssp = yyss;
  yyvsp = yyvs;

  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed. so pushing a state here evens the stacks.
     */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
	/* Give user a chance to reallocate the stack. Use copies of
	   these so that the &'s don't force the real ones into
	   memory.  */
	YYSTYPE *yyvs1 = yyvs;
	short *yyss1 = yyss;


	/* Each stack pointer address is followed by the size of the
	   data in use in that stack, in bytes.  This used to be a
	   conditional around just the two extra args, but that might
	   be undefined if yyoverflow is a macro.  */
	yyoverflow ("parser stack overflow",
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),

		    &yystacksize);

	yyss = yyss1;
	yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyoverflowlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
	goto yyoverflowlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
	yystacksize = YYMAXDEPTH;

      {
	short *yyss1 = yyss;
	union yyalloc *yyptr =
	  (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
	if (! yyptr)
	  goto yyoverflowlab;
	YYSTACK_RELOCATE (yyss);
	YYSTACK_RELOCATE (yyvs);

#  undef YYSTACK_RELOCATE
	if (yyss1 != yyssa)
	  YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;


      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
		  (unsigned long int) yystacksize));

      if (yyss + yystacksize - 1 <= yyssp)
	YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", yystate));

  goto yybackup;

/*-----------.
| yybackup.  |
`-----------*/
yybackup:

/* Do appropriate processing given the current state.  */
/* Read a lookahead token if we need one and don't already have one.  */
/* yyresume: */

  /* First try to decide what to do without reference to lookahead token.  */

  yyn = yypact[yystate];
  if (yyn == YYPACT_NINF)
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid lookahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = YYLEX;
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YYDSYMPRINTF ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yyn == 0 || yyn == YYTABLE_NINF)
	goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  if (yyn == YYFINAL)
    YYACCEPT;

  /* Shift the lookahead token.  */
  YYDPRINTF ((stderr, "Shifting token %s, ", yytname[yytoken]));

  /* Discard the token being shifted unless it is eof.  */
  if (yychar != YYEOF)
    yychar = YYEMPTY;

  *++yyvsp = yylval;


  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  yystate = yyn;
  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- Do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     `$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 2:
#line 611 "./yuiif.y"
    {
		    _dxd_exParseTree  = NULL;
		    _dxd_exParseMesg  = NULL;
		    _dxd_exParseError = FALSE;
		    _dxf_ExLexInit ();
		    _dxf_ExEvaluateConstants (FALSE);
		}
    break;

  case 3:
#line 619 "./yuiif.y"
    {
		    if (_dxd_exParseError)
		    {
			_dxd_exParseTree = NULL;
			if (_dxd_exParseMesg != NULL)
			    yyerror (_dxd_exParseMesg);
			YYABORT;
		    }
		    else
		    {
			_dxd_exParseTree = yyvsp[0].n;
			_dxf_ExEvaluateConstants (TRUE);
			YYACCEPT;
		    }
		}
    break;

  case 8:
#line 641 "./yuiif.y"
    {
		    _pushInput (yyvsp[0].s);
		    DXFree ((Pointer) yyvsp[0].s);
		    yyval.n = NULL;
		}
    break;

  case 9:
#line 647 "./yuiif.y"
    {
                    /* needed to zero out char count here because */
                    /* parser gets a bit confused                 */
                    yycharno = 0;
		    _pushInput (yyvsp[0].s);
		    DXFree ((Pointer) yyvsp[0].s);
		    yyval.n = NULL;
		}
    break;

  case 10:
#line 656 "./yuiif.y"
    {
		    *_dxd_exTerminating = TRUE;
		    _dxf_ExVCRCommand (VCR_C_FLAG, VCR_F_LOOP, 0);
		    _dxf_ExBackgroundCommand (BG_CANCEL, NULL);
		    yyval.n = NULL;
		}
    break;

  case 11:
#line 663 "./yuiif.y"
    {
		    /* if eof on stdin, exit cleanly */
		    if (_popInput ())
		    {
                        /* if we are connected to UI kill running graph */
                        if(_dxd_exRemote) {
                            _dxf_ExExecCommandStr ("kill");
                            _dxf_ExVCRCommand (VCR_C_STOP, 0, 0);
                        }
			*_dxd_exTerminating = TRUE;
			_dxf_ExVCRCommand (VCR_C_FLAG, VCR_F_LOOP, 0);
			_dxf_ExBackgroundCommand (BG_CANCEL, NULL);
		    }
		    yyval.n = NULL;
		}
    break;

  case 12:
#line 679 "./yuiif.y"
    {
		    yyval.n = NULL;
		}
    break;

  case 13:
#line 683 "./yuiif.y"
    {
		    yyval.n = NULL;
		}
    break;

  case 14:
#line 689 "./yuiif.y"
    {
		    _dxf_ExEvaluateConstants (TRUE);
		    _dxf_ExVCRCommand (VCR_C_TREE, (long) yyvsp[0].n, 0);
		    yyval.n = NULL;
		}
    break;

  case 15:
#line 695 "./yuiif.y"
    {
		    _dxf_ExExecCommandStr ("kill");
		    _dxf_ExVCRCommand (VCR_C_PAUSE, 0, 0);
#if 0
                   /* let UI handle this, removed 7/20/93 */
		    _dxf_ExBackgroundCommand (BG_CANCEL, NULL);
#endif
		    yyval.n = NULL;
		}
    break;

  case 16:
#line 705 "./yuiif.y"
    {
		    _dxf_ExVCRCommand (VCR_C_PLAY, 0, 0);
		    yyval.n = NULL;
		}
    break;

  case 17:
#line 710 "./yuiif.y"
    {
		    _dxf_ExVCRCommand (VCR_C_STEP, 0, 0);
		    yyval.n = NULL;
		}
    break;

  case 18:
#line 715 "./yuiif.y"
    {
		    _dxf_ExExecCommandStr ("kill");
		    _dxf_ExVCRCommand (VCR_C_STOP, 0, 0);
#if 0
                    /* let UI handle this, removed 7/20/93 */
		    _dxf_ExBackgroundCommand (BG_CANCEL, NULL);
#endif
		    yyval.n = NULL;
		}
    break;

  case 19:
#line 725 "./yuiif.y"
    {
		    _dxf_ExVCRCommand (VCR_C_DIR, VCR_D_FORWARD, 0);
		    yyval.n = NULL;
		}
    break;

  case 20:
#line 730 "./yuiif.y"
    {
		    _dxf_ExVCRCommand (VCR_C_DIR, VCR_D_BACKWARD, 0);
		    yyval.n = NULL;
		}
    break;

  case 21:
#line 735 "./yuiif.y"
    {
		    _dxf_ExVCRCommand (VCR_C_FLAG, VCR_F_PALIN, yyvsp[-1].i);
		    yyval.n = NULL;
		}
    break;

  case 22:
#line 740 "./yuiif.y"
    {
		    _dxf_ExVCRCommand (VCR_C_FLAG, VCR_F_LOOP, yyvsp[-1].i);
		    yyval.n = NULL;
		}
    break;

  case 23:
#line 747 "./yuiif.y"
    {
		    yyval.i = TRUE;
		}
    break;

  case 24:
#line 751 "./yuiif.y"
    {
		    yyval.i = FALSE;
		}
    break;

  case 25:
#line 758 "./yuiif.y"
    {
	    _dxf_ExUIPacketActive (yyvsp[-4].i, PK_INTERRUPT, yyvsp[0].i);
	}
    break;

  case 26:
#line 762 "./yuiif.y"
    {
	    _dxf_ExExecCommandStr ("kill");
	    _dxf_ExVCRCommand (VCR_C_STOP, 0, 0);
	    _dxf_ExBackgroundCommand (BG_CANCEL, NULL);
	    yyval.n = _dxf_ExPCreatePacket (PK_INTERRUPT, yyvsp[-8].i, yyvsp[-4].i, NULL);
	}
    break;

  case 27:
#line 770 "./yuiif.y"
    {
	    _dxf_ExUIPacketActive (yyvsp[-4].i, PK_SYSTEM, yyvsp[0].i);
	}
    break;

  case 28:
#line 774 "./yuiif.y"
    {
	    yyval.n = NULL;
	}
    break;

  case 29:
#line 779 "./yuiif.y"
    {
	    _dxf_ExUIPacketActive (yyvsp[-4].i, PK_ACK, yyvsp[0].i);
	}
    break;

  case 30:
#line 783 "./yuiif.y"
    {
	    yyval.n = NULL;
	}
    break;

  case 31:
#line 788 "./yuiif.y"
    {
	    _dxf_ExUIPacketActive (yyvsp[-4].i, PK_MACRO, yyvsp[0].i);
	}
    break;

  case 32:
#line 792 "./yuiif.y"
    {
	    yyval.n = _dxf_ExPCreatePacket (PK_MACRO, yyvsp[-8].i, yyvsp[-4].i, yyvsp[-1].n);
	}
    break;

  case 33:
#line 797 "./yuiif.y"
    {
	    _dxf_ExUIPacketActive (yyvsp[-4].i, PK_FOREGROUND, yyvsp[0].i);
	}
    break;

  case 34:
#line 801 "./yuiif.y"
    {
	    yyval.n = _dxf_ExPCreatePacket (PK_FOREGROUND, yyvsp[-8].i, yyvsp[-4].i, yyvsp[-1].n);
	}
    break;

  case 35:
#line 806 "./yuiif.y"
    {
	    _dxf_ExUIPacketActive (yyvsp[-4].i, PK_BACKGROUND, yyvsp[0].i);
	}
    break;

  case 36:
#line 810 "./yuiif.y"
    {
	    yyval.n = _dxf_ExPCreatePacket (PK_BACKGROUND, yyvsp[-8].i, yyvsp[-4].i, yyvsp[-1].n);
	}
    break;

  case 37:
#line 815 "./yuiif.y"
    {
	    _dxf_ExUIPacketActive (yyvsp[-4].i, PK_ERROR, yyvsp[0].i);
	    data_len = yyvsp[0].i;
	}
    break;

  case 38:
#line 820 "./yuiif.y"
    {
	    yyval.n = NULL;
	}
    break;

  case 39:
#line 825 "./yuiif.y"
    {
	    _dxf_ExUIPacketActive (yyvsp[-4].i, PK_MESSAGE, yyvsp[0].i);
	}
    break;

  case 40:
#line 829 "./yuiif.y"
    {
	    yyval.n = NULL;
	}
    break;

  case 41:
#line 834 "./yuiif.y"
    {
	    _dxf_ExUIPacketActive (yyvsp[-4].i, PK_INFO, yyvsp[0].i);
	}
    break;

  case 42:
#line 838 "./yuiif.y"
    {
	    yyval.n = NULL;
	}
    break;

  case 43:
#line 843 "./yuiif.y"
    {
	    _dxf_ExUIPacketActive (yyvsp[-4].i, PK_LINQ, yyvsp[0].i);
	}
    break;

  case 44:
#line 847 "./yuiif.y"
    {
	    yyval.n = NULL;
	}
    break;

  case 45:
#line 852 "./yuiif.y"
    {
	    _dxf_ExUIPacketActive (yyvsp[-4].i, PK_SINQ, yyvsp[0].i);
	}
    break;

  case 46:
#line 856 "./yuiif.y"
    {
	    yyval.n = NULL;
	}
    break;

  case 47:
#line 861 "./yuiif.y"
    {
	    _dxf_ExUIPacketActive (yyvsp[-4].i, PK_VINQ, yyvsp[0].i);
	}
    break;

  case 48:
#line 865 "./yuiif.y"
    {
	    yyval.n = NULL;
	}
    break;

  case 49:
#line 870 "./yuiif.y"
    {
	    _dxf_ExUIPacketActive (yyvsp[-4].i, PK_LRESP, yyvsp[0].i);
	}
    break;

  case 50:
#line 874 "./yuiif.y"
    {
	    yyval.n = NULL;
	}
    break;

  case 51:
#line 879 "./yuiif.y"
    {
	    _dxf_ExUIPacketActive (yyvsp[-4].i, PK_SRESP, yyvsp[0].i);
	}
    break;

  case 52:
#line 883 "./yuiif.y"
    {
	    yyval.n = NULL;
	}
    break;

  case 53:
#line 888 "./yuiif.y"
    {
	    _dxf_ExUIPacketActive (yyvsp[-4].i, PK_VRESP, yyvsp[0].i);
	}
    break;

  case 54:
#line 892 "./yuiif.y"
    {
	    yyval.n = NULL;
	}
    break;

  case 55:
#line 897 "./yuiif.y"
    {
	    _dxf_ExUIPacketActive (yyvsp[-4].i, PK_DATA, yyvsp[0].i);
	}
    break;

  case 56:
#line 901 "./yuiif.y"
    {
	    yyval.n = NULL;
	}
    break;

  case 57:
#line 906 "./yuiif.y"
    {
	    _dxf_ExUIPacketActive (yyvsp[-4].i, PK_IMPORT, yyvsp[0].i);
	    data_len = yyvsp[0].i;
	}
    break;

  case 58:
#line 911 "./yuiif.y"
    {
	    yyval.n = _dxf_ExPCreatePacket (PK_IMPORT, yyvsp[-8].i, yyvsp[-4].i, yyvsp[-1].n);
	}
    break;

  case 68:
#line 964 "./yuiif.y"
    {
		    _dxf_ExBackgroundCommand (BG_STATEMENT, yyvsp[0].n);
		    yyval.n = NULL;
		}
    break;

  case 69:
#line 969 "./yuiif.y"
    {
		    _dxf_ExExecCommandStr ("kill");
		    _dxf_ExVCRCommand (VCR_C_STOP, 0, 0);
		    _dxf_ExBackgroundCommand (BG_CANCEL, NULL);
		    yyval.n = NULL;
		}
    break;

  case 82:
#line 1038 "./yuiif.y"
    {
		    yyval.n = _dxf_ExPCreateMacro (yyvsp[-9].n, yyvsp[-7].n, yyvsp[-3].n, yyvsp[0].n);
		    yyval.n->attr = yyvsp[-1].n;
		}
    break;

  case 83:
#line 1044 "./yuiif.y"
    {
		    yyval.n = _dxf_ExPCreateMacro (yyvsp[-5].n, yyvsp[-3].n, NULL, yyvsp[0].n);
		    yyval.n->attr = yyvsp[-1].n;
		}
    break;

  case 86:
#line 1055 "./yuiif.y"
    {
		    yyvsp[-1].n->attr = yyvsp[0].n;
		    LIST_CREATE (yyvsp[-1].n);
		}
    break;

  case 87:
#line 1060 "./yuiif.y"
    {
		    yyvsp[-1].n->attr = yyvsp[0].n;
		    LIST_APPEND_CHECK (yyvsp[-3].n, yyvsp[-1].n);
		    if (! _dxd_exUIPacket)
			yyerrok;
		}
    break;

  case 89:
#line 1070 "./yuiif.y"
    {
		    yyvsp[-2].n->v.id.dflt = yyvsp[0].n;
		}
    break;

  case 90:
#line 1074 "./yuiif.y"
    {
		    yyvsp[-3].n->v.id.dflt = _dxf_ExPNegateConst (yyvsp[0].n);
		}
    break;

  case 91:
#line 1085 "./yuiif.y"
    {
		    yyval.n = yyvsp[-1].n;
		}
    break;

  case 94:
#line 1099 "./yuiif.y"
    {
		    LIST_CREATE (yyvsp[0].n);
		}
    break;

  case 95:
#line 1103 "./yuiif.y"
    {
		    LIST_APPEND_CHECK (yyvsp[-1].n, yyvsp[0].n);
		    if (! _dxd_exUIPacket)
			yyerrok;
		}
    break;

  case 97:
#line 1116 "./yuiif.y"
    {
		    LIST_CREATE (yyvsp[-1].n);
		}
    break;

  case 98:
#line 1120 "./yuiif.y"
    {
		    LIST_CREATE (yyvsp[-1].n);
		}
    break;

  case 101:
#line 1135 "./yuiif.y"
    {
		    yyval.n = _dxf_ExPCreateAssign (yyvsp[-2].n, yyvsp[0].n);
		}
    break;

  case 102:
#line 1141 "./yuiif.y"
    {
		    yyval.n = _dxf_ExPCreateAssign (yyvsp[-2].n, yyvsp[0].n);
		}
    break;

  case 103:
#line 1145 "./yuiif.y"
    {
		    node	*mm;
		    int		val = 1;

		    mm = _dxf_ExPCreateConst (TYPE_INT, CATEGORY_REAL,
				       1, (Pointer) &val);
		    mm = _dxf_ExPCreateArith (AO_PLUS, yyvsp[-1].n, mm);
		    EXO_reference ((EXO_Object) yyvsp[-1].n);
		    yyval.n = _dxf_ExPCreateAssign (yyvsp[-1].n, mm);
		}
    break;

  case 104:
#line 1156 "./yuiif.y"
    {
		    node	*mm;
		    int		val = 1;

		    mm = _dxf_ExPCreateConst (TYPE_INT, CATEGORY_REAL,
				       1, (Pointer) &val);
		    mm = _dxf_ExPCreateArith (AO_MINUS, yyvsp[-1].n, mm);
		    EXO_reference ((EXO_Object) yyvsp[-1].n);
		    yyval.n = _dxf_ExPCreateAssign (yyvsp[-1].n, mm);
		}
    break;

  case 109:
#line 1182 "./yuiif.y"
    {
		    yyval.n = yyvsp[-1].n;
		}
    break;

  case 112:
#line 1192 "./yuiif.y"
    {
		    LIST_CREATE (yyvsp[0].n);
		}
    break;

  case 113:
#line 1196 "./yuiif.y"
    {
		    LIST_APPEND_CHECK (yyvsp[-2].n, yyvsp[0].n);
		}
    break;

  case 114:
#line 1202 "./yuiif.y"
    {
		    yyval.n = _dxf_ExPCreateAttribute (yyvsp[-2].n, yyvsp[0].n);
		}
    break;

  case 115:
#line 1212 "./yuiif.y"
    {
		    yyval.n = _dxf_ExPCreateCall (yyvsp[-4].n, yyvsp[-2].n);
		    yyval.n->attr = yyvsp[0].n;
		}
    break;

  case 118:
#line 1227 "./yuiif.y"
    {
		    LIST_CREATE (yyvsp[0].n);
		}
    break;

  case 119:
#line 1231 "./yuiif.y"
    {
		    LIST_APPEND_CHECK (yyvsp[-2].n, yyvsp[0].n);
		    if (! _dxd_exUIPacket)
			yyerrok;
		}
    break;

  case 120:
#line 1239 "./yuiif.y"
    {
		    yyval.n = _dxf_ExPCreateArgument (NULL, yyvsp[0].n);
		}
    break;

  case 121:
#line 1243 "./yuiif.y"
    {
		    yyval.n = _dxf_ExPCreateArgument (yyvsp[-2].n, yyvsp[0].n);
		}
    break;

  case 123:
#line 1256 "./yuiif.y"
    {
		    LIST_CREATE (yyvsp[0].n);
		}
    break;

  case 124:
#line 1260 "./yuiif.y"
    {
		    LIST_APPEND_CHECK (yyvsp[-2].n, yyvsp[0].n);
		    if (! _dxd_exUIPacket)
			yyerrok;
		}
    break;

  case 127:
#line 1270 "./yuiif.y"
    {
		    yyval.n = yyvsp[-1].n;
		}
    break;

  case 129:
#line 1277 "./yuiif.y"
    {
		    yyval.n = _dxf_ExPCreateArith (AO_EXP, yyvsp[-2].n, yyvsp[0].n);
		}
    break;

  case 130:
#line 1281 "./yuiif.y"
    {
		    yyval.n = _dxf_ExPCreateArith (AO_TIMES, yyvsp[-2].n, yyvsp[0].n);
		}
    break;

  case 131:
#line 1285 "./yuiif.y"
    {
		    yyval.n = _dxf_ExPCreateArith (AO_DIV, yyvsp[-2].n, yyvsp[0].n);
		}
    break;

  case 132:
#line 1289 "./yuiif.y"
    {
		    yyval.n = _dxf_ExPCreateArith (AO_IDIV, yyvsp[-2].n, yyvsp[0].n);
		}
    break;

  case 133:
#line 1293 "./yuiif.y"
    {
		    yyval.n = _dxf_ExPCreateArith (AO_MOD, yyvsp[-2].n, yyvsp[0].n);
		}
    break;

  case 134:
#line 1297 "./yuiif.y"
    {
		    yyval.n = _dxf_ExPCreateArith (AO_PLUS, yyvsp[-2].n, yyvsp[0].n);
		}
    break;

  case 135:
#line 1301 "./yuiif.y"
    {
		    yyval.n = _dxf_ExPCreateArith (AO_MINUS, yyvsp[-2].n, yyvsp[0].n);
		}
    break;

  case 136:
#line 1305 "./yuiif.y"
    {
		    if (yyvsp[0].n->type == NT_CONSTANT)
			yyval.n = _dxf_ExPNegateConst (yyvsp[0].n);
		    else
			yyval.n = _dxf_ExPCreateArith (AO_NEGATE, yyvsp[0].n, NULL);
		}
    break;

  case 141:
#line 1324 "./yuiif.y"
    {
		    yyval.n = yyvsp[-1].n;
		}
    break;

  case 142:
#line 1328 "./yuiif.y"
    {
		    yyval.n = yyvsp[-1].n;
		}
    break;

  case 143:
#line 1332 "./yuiif.y"
    {
		    yyval.n = yyvsp[-1].n;
		}
    break;

  case 144:
#line 1336 "./yuiif.y"
    {
		    yyval.n = _dxf_ExPDotDotList (yyvsp[-3].n, yyvsp[-1].n, NULL);
		    _dxf_ExPDestroyNode (yyvsp[-3].n);
		    _dxf_ExPDestroyNode (yyvsp[-1].n);
		}
    break;

  case 145:
#line 1342 "./yuiif.y"
    {
		    yyval.n = _dxf_ExPDotDotList (yyvsp[-5].n, yyvsp[-3].n, yyvsp[-1].n);
		    _dxf_ExPDestroyNode (yyvsp[-5].n);
		    _dxf_ExPDestroyNode (yyvsp[-3].n);
		    _dxf_ExPDestroyNode (yyvsp[-1].n);
		}
    break;

  case 147:
#line 1352 "./yuiif.y"
    {
		    yyval.n = _dxf_ExAppendConst (yyvsp[-1].n, yyvsp[0].n);
		    _dxf_ExPDestroyNode (yyvsp[0].n);
		}
    break;

  case 148:
#line 1357 "./yuiif.y"
    {
		    yyval.n = _dxf_ExPNegateConst (yyvsp[0].n);
		}
    break;

  case 149:
#line 1361 "./yuiif.y"
    {
		    yyvsp[0].n = _dxf_ExPNegateConst (yyvsp[0].n);
		    yyval.n = _dxf_ExAppendConst (yyvsp[-2].n, yyvsp[0].n);
		    _dxf_ExPDestroyNode (yyvsp[0].n);
		}
    break;

  case 150:
#line 1367 "./yuiif.y"
    {
		    yyval.n = _dxf_ExAppendConst (yyvsp[-2].n, yyvsp[0].n);
		    _dxf_ExPDestroyNode (yyvsp[0].n);
		}
    break;

  case 151:
#line 1372 "./yuiif.y"
    {
		    yyvsp[0].n = _dxf_ExPNegateConst (yyvsp[0].n);
		    yyval.n = _dxf_ExAppendConst (yyvsp[-3].n, yyvsp[0].n);
		    _dxf_ExPDestroyNode (yyvsp[0].n);
		}
    break;

  case 152:
#line 1380 "./yuiif.y"
    {
		    yyval.n = _dxf_ExPExtendConst (yyvsp[-1].n);
		}
    break;

  case 153:
#line 1384 "./yuiif.y"
    {
		    yyval.n = _dxf_ExPExtendConst (yyvsp[-1].n);
		}
    break;

  case 155:
#line 1394 "./yuiif.y"
    {
		    yyval.n = _dxf_ExAppendConst (yyvsp[-1].n, yyvsp[0].n);
		    _dxf_ExPDestroyNode (yyvsp[0].n);
		}
    break;

  case 156:
#line 1399 "./yuiif.y"
    {
		    yyval.n = _dxf_ExPNegateConst (yyvsp[0].n);
		}
    break;

  case 157:
#line 1403 "./yuiif.y"
    {
		    yyvsp[0].n = _dxf_ExPNegateConst (yyvsp[0].n);
		    yyval.n = _dxf_ExAppendConst (yyvsp[-2].n, yyvsp[0].n);
		    _dxf_ExPDestroyNode (yyvsp[0].n);
		}
    break;

  case 158:
#line 1409 "./yuiif.y"
    {
		    yyval.n = _dxf_ExAppendConst (yyvsp[-2].n, yyvsp[0].n);
		    _dxf_ExPDestroyNode (yyvsp[0].n);
		}
    break;

  case 159:
#line 1414 "./yuiif.y"
    {
		    yyvsp[0].n = _dxf_ExPNegateConst (yyvsp[0].n);
		    yyval.n = _dxf_ExAppendConst (yyvsp[-3].n, yyvsp[0].n);
		    _dxf_ExPDestroyNode (yyvsp[0].n);
		}
    break;

  case 161:
#line 1423 "./yuiif.y"
    {
		    yyval.n = _dxf_ExAppendConst (yyvsp[-1].n, yyvsp[0].n);
		    _dxf_ExPDestroyNode (yyvsp[0].n);
		}
    break;

  case 162:
#line 1428 "./yuiif.y"
    {
		    yyval.n = _dxf_ExAppendConst (yyvsp[-2].n, yyvsp[0].n);
		    _dxf_ExPDestroyNode (yyvsp[0].n);
		}
    break;

  case 167:
#line 1441 "./yuiif.y"
    {
		    yyval.n = _dxf_ExPNegateConst (yyvsp[0].n);
		}
    break;

  case 168:
#line 1447 "./yuiif.y"
    {
		    yyval.n = _dxf_ExPCreateConst (TYPE_INT,   CATEGORY_REAL,
					1, (Pointer) &(yyvsp[0].i));
		}
    break;

  case 169:
#line 1452 "./yuiif.y"
    {
		    yyval.n = _dxf_ExPCreateConst (TYPE_FLOAT, CATEGORY_REAL,
					1, (Pointer) &(yyvsp[0].f));
		}
    break;

  case 170:
#line 1459 "./yuiif.y"
    {
		    int		c[2];

		    c[0] = yyvsp[-3].i;
		    c[1] = yyvsp[-1].i;
		    yyval.n = _dxf_ExPCreateConst (TYPE_INT,   CATEGORY_COMPLEX,
					1, (Pointer) c);
		}
    break;

  case 171:
#line 1468 "./yuiif.y"
    {
		    float	c[2];

		    c[0] = (float) yyvsp[-3].i;
		    c[1] = yyvsp[-1].f;
		    yyval.n = _dxf_ExPCreateConst (TYPE_FLOAT, CATEGORY_COMPLEX,
					1, (Pointer) c);
		}
    break;

  case 172:
#line 1477 "./yuiif.y"
    {
		    float	c[2];

		    c[0] = yyvsp[-3].f;
		    c[1] = (float) yyvsp[-1].i;
		    yyval.n = _dxf_ExPCreateConst (TYPE_FLOAT, CATEGORY_COMPLEX,
					1, (Pointer) c);
		}
    break;

  case 173:
#line 1486 "./yuiif.y"
    {
		    float	c[2];

		    c[0] = yyvsp[-3].f;
		    c[1] = yyvsp[-1].f;
		    yyval.n = _dxf_ExPCreateConst (TYPE_FLOAT, CATEGORY_COMPLEX,
					1, (Pointer) c);
		}
    break;

  case 174:
#line 1497 "./yuiif.y"
    {
		    int		c[4];

		    c[0] = yyvsp[-7].i;
		    c[1] = yyvsp[-5].i;
		    c[2] = yyvsp[-3].i;
		    c[3] = yyvsp[-1].i;
		    yyval.n = _dxf_ExPCreateConst (TYPE_INT,   CATEGORY_QUATERNION,
				       1, (Pointer) c);
		}
    break;

  case 175:
#line 1508 "./yuiif.y"
    {
		    float	c[4];

		    c[0] = yyvsp[-7].f;
		    c[1] = yyvsp[-5].f;
		    c[2] = yyvsp[-3].f;
		    c[3] = yyvsp[-1].f;
		    yyval.n = _dxf_ExPCreateConst (TYPE_FLOAT, CATEGORY_QUATERNION,
				       1, (Pointer) c);
		}
    break;

  case 177:
#line 1522 "./yuiif.y"
    {
		    yyval.i = - yyvsp[0].i;
		}
    break;

  case 180:
#line 1532 "./yuiif.y"
    {
		    yyval.f = - yyvsp[0].f;
		}
    break;

  case 182:
#line 1541 "./yuiif.y"
    {
                    /* should this be TYPE_STRING eventually? */
		    yyval.n = _dxf_ExPCreateConst (TYPE_UBYTE, CATEGORY_REAL,
				       yyLeng + 1, (Pointer) yyvsp[0].s);
		    DXFree ((Pointer) yyvsp[0].s);
		}
    break;

  case 183:
#line 1551 "./yuiif.y"
    {
		    if (! _dxd_exUIPacket)
			yyerrok;
		}
    break;

  case 184:
#line 1558 "./yuiif.y"
    {
		    if (! _dxd_exUIPacket)
			yyerrok;
		}
    break;

  case 185:
#line 1565 "./yuiif.y"
    {
		    if (! _dxd_exUIPacket)
			yyerrok;
		}
    break;

  case 186:
#line 1572 "./yuiif.y"
    {
		    yyval.n = _dxf_ExPCreateId (yyvsp[0].s);
		}
    break;

  case 187:
#line 1578 "./yuiif.y"
    {
		    yyval.n = _dxf_ExPCreateExid (yyvsp[0].s);
		}
    break;

  case 188:
#line 1584 "./yuiif.y"
    {
		    yyval.n = NULL;
		}
    break;

  case 191:
#line 1598 "./yuiif.y"
    {
		    yyvsp[-1].n->attr = yyvsp[0].n;
		    LIST_CREATE (yyvsp[-1].n);
		}
    break;

  case 192:
#line 1603 "./yuiif.y"
    {
		    yyvsp[-1].n->attr = yyvsp[0].n;
		    LIST_APPEND_CHECK (yyvsp[-3].n, yyvsp[-1].n);
		}
    break;

  case 197:
#line 1623 "./yuiif.y"
    {
		    Pointer buffer;

		    if (data_len)
		    {
		        buffer = DXAllocate (data_len + 1);
			yygrabdata (buffer, data_len);
			DXUIMessage("ERROR", buffer);
			DXFree(buffer);
		    }
		    else
		    {
			_dxd_exParseError = TRUE;
		    }
		    yyval.n = NULL;
		}
    break;

  case 198:
#line 1642 "./yuiif.y"
    {
		    Pointer buffer;

		    buffer = DXAllocate (data_len + 1);
		    if (buffer)
		    {
			yygrabdata (buffer, data_len);
			yyval.n = _dxf_ExPCreateData (buffer, data_len);
		    }
		    else
		    {
			_dxd_exParseError = TRUE;
			yyval.n = NULL;
		    }
		}
    break;


    }

/* Line 991 of yacc.c.  */
#line 3082 "y.tab.c"

  yyvsp -= yylen;
  yyssp -= yylen;


  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;


  /* Now `shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*------------------------------------.
| yyerrlab -- here on detecting error |
`------------------------------------*/
yyerrlab:
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if YYERROR_VERBOSE
      yyn = yypact[yystate];

      if (YYPACT_NINF < yyn && yyn < YYLAST)
	{
	  YYSIZE_T yysize = 0;
	  int yytype = YYTRANSLATE (yychar);
	  char *yymsg;
	  int yyx, yycount;

	  yycount = 0;
	  /* Start YYX at -YYN if negative to avoid negative indexes in
	     YYCHECK.  */
	  for (yyx = yyn < 0 ? -yyn : 0;
	       yyx < (int) (sizeof (yytname) / sizeof (char *)); yyx++)
	    if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
	      yysize += yystrlen (yytname[yyx]) + 15, yycount++;
	  yysize += yystrlen ("syntax error, unexpected ") + 1;
	  yysize += yystrlen (yytname[yytype]);
	  yymsg = (char *) YYSTACK_ALLOC (yysize);
	  if (yymsg != 0)
	    {
	      char *yyp = yystpcpy (yymsg, "syntax error, unexpected ");
	      yyp = yystpcpy (yyp, yytname[yytype]);

	      if (yycount < 5)
		{
		  yycount = 0;
		  for (yyx = yyn < 0 ? -yyn : 0;
		       yyx < (int) (sizeof (yytname) / sizeof (char *));
		       yyx++)
		    if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
		      {
			const char *yyq = ! yycount ? ", expecting " : " or ";
			yyp = yystpcpy (yyp, yyq);
			yyp = yystpcpy (yyp, yytname[yyx]);
			yycount++;
		      }
		}
	      yyerror (yymsg);
	      YYSTACK_FREE (yymsg);
	    }
	  else
	    yyerror ("syntax error; also virtual memory exhausted");
	}
      else
#endif /* YYERROR_VERBOSE */
	yyerror ("syntax error");
    }



  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
	 error, discard it.  */

      /* Return failure if at end of input.  */
      if (yychar == YYEOF)
        {
	  /* Pop the error token.  */
          YYPOPSTACK;
	  /* Pop the rest of the stack.  */
	  while (yyss < yyssp)
	    {
	      YYDSYMPRINTF ("Error: popping", yystos[*yyssp], yyvsp, yylsp);
	      yydestruct (yystos[*yyssp], yyvsp);
	      YYPOPSTACK;
	    }
	  YYABORT;
        }

      YYDSYMPRINTF ("Error: discarding", yytoken, &yylval, &yylloc);
      yydestruct (yytoken, &yylval);
      yychar = YYEMPTY;

    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab2;


/*----------------------------------------------------.
| yyerrlab1 -- error raised explicitly by an action.  |
`----------------------------------------------------*/
yyerrlab1:

  /* Suppress GCC warning that yyerrlab1 is unused when no action
     invokes YYERROR.  */
#if defined (__GNUC_MINOR__) && 2093 <= (__GNUC__ * 1000 + __GNUC_MINOR__)
  __attribute__ ((__unused__))
#endif


  goto yyerrlab2;


/*---------------------------------------------------------------.
| yyerrlab2 -- pop states until the error token can be shifted.  |
`---------------------------------------------------------------*/
yyerrlab2:
  yyerrstatus = 3;	/* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (yyn != YYPACT_NINF)
	{
	  yyn += YYTERROR;
	  if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYTERROR)
	    {
	      yyn = yytable[yyn];
	      if (0 < yyn)
		break;
	    }
	}

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
	YYABORT;

      YYDSYMPRINTF ("Error: popping", yystos[*yyssp], yyvsp, yylsp);
      yydestruct (yystos[yystate], yyvsp);
      yyvsp--;
      yystate = *--yyssp;

      YY_STACK_PRINT (yyss, yyssp);
    }

  if (yyn == YYFINAL)
    YYACCEPT;

  YYDPRINTF ((stderr, "Shifting error token, "));

  *++yyvsp = yylval;


  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturn;

/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturn;

#ifndef yyoverflow
/*----------------------------------------------.
| yyoverflowlab -- parser overflow comes here.  |
`----------------------------------------------*/
yyoverflowlab:
  yyerror ("parser stack overflow");
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
  return yyresult;
}


#line 1659 "./yuiif.y"


int
yyerror (char *s)
{
    static char prevfile[256];
    static int	prevline	= -1;
    int		new_line	= FALSE;
    char	*name;
    char	buf[8192];

    if (s == NULL)
    {
	prevline    = -1;
	prevfile[0] = '\000';
	return ERROR;
    }

    if (prevline == -1)
	prevfile[0] = '\000';

    name = fname[*fnum];
    _dxd_exParseError = TRUE;

    /*
     * ALWAYS suppress multiple messages about the same line.
     */

    if (prevline != yylineno || strcmp (prevfile, name))
    {
	strncpy (prevfile, name, 255);
	prevfile[255] = '\000';
	prevline = yylineno;
	yyText[yycharno] = '\000';
	new_line = TRUE;
    }

    /*
     * Send a 1 line message to the UI with column number, actually
     * draw out an arrow when talking directly to the user.
     */


    if (new_line)
    {
	if (! isprint (yyText[yycharno - 1]))
	{
	    sprintf (buf, "%s (non-printable character 0x%x)",
		     s, yyText[yycharno - 1]);
	    s = buf;
	}

	if (_dxd_exRemote)
	{
	    DXUIMessage ("ERROR", "%s[%d,%d]:  %s:  %s",
		       name, yylineno, yycharno, s, yyText);
	}
	else
	{
	    DXUIMessage ("ERROR", "%s[%d]: %s", name, yylineno, s);
	    _dxf_ExLexError ();
	}
    }

    /* in script mode running MP you will get 2 prompts on error
     * unless you flush out the newline after the error.
     */
    if (_dxd_exUIPacket)
	_dxf_ExUIFlushPacket ();
    else if(! _dxd_exRemote)
        _dxf_ExFlushNewLine();

    return ERROR;
}

