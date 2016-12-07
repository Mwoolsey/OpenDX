/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>



#include "defines.h"
#include <stdio.h>
#include <string.h>
#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif
#if defined(HAVE_MALLOC_H)
#include <malloc.h>
#endif
#include "ErrorDialogManager.h"
#include "WarningDialogManager.h"
#include "MBGenerate.h"
#include "MBApplication.h"

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

static int process_input(char *, Module *, Parameter **, Parameter **);
static int do_mdf(char *, Module *, Parameter **, Parameter **);
static int do_makefile(char *, Module *);
static int do_builder(char *, Module *, Parameter **, Parameter **);
static int cleanup(Module *m, Parameter **in, Parameter **out);

static int GetDXDataType(enum datatype, char **);
static int GetCDataType(enum datatype, char **);
static int GetDXDataShape(enum datashape, char **, char **);
static int GetElementTypeString(enum elementtype, char **);
static int copy_comments(char *, FILE *, char *, char *, char *);

#define MAX_IN	128
#define MAX_OUT 128

#define DO_MDF	0x1
#define DO_C    0x2
#define DO_MAKE 0x4

// Added COMMA_AND_NEWLINE to replace the use of a comma-separated list
// of statements in a cond?expr1:expr2 clause.  (The expr1 was the list
// and its contents were passed to a sprintf function, the outcome of
// which was unpredictable.) MST
#if defined(intelnt) || defined (WIN32)
#define NEWLINE "\r\n"
#define COMMA_AND_NEWLINE ",\r\n"
#else
#define NEWLINE "\n"
#define COMMA_AND_NEWLINE ",\n"
#endif

int Generate(int which, char *filename)
{
    Module module;
    Parameter *inputs[MAX_IN], *outputs[MAX_OUT];
    int i;
    char *c, *fnamecopy = (char *)malloc(strlen(filename) + 1);

    inputs[0] = outputs[0] = NULL;
    memset(&module, 0, sizeof(module));
    strcpy(fnamecopy, filename);

    for (i = 0; i < strlen(fnamecopy); i++)
	if (fnamecopy[i] == '.')
	{
	    fnamecopy[i] = '\0';
	    break;
	}
    
    if (! process_input(fnamecopy, &module, inputs, outputs))
	return 0;
    
    if (which & DO_MDF)
	if (! do_mdf(fnamecopy, &module, inputs, outputs))
	    return 0;
    
    if (which & DO_C)
	if (! do_builder(fnamecopy, &module, inputs, outputs))
	    return 0;
    
    if (which & DO_MAKE)
	if (! do_makefile(fnamecopy, &module))
	    return 0;
    
    if (! cleanup(&module, inputs, outputs))
	return 0;
    
    free(fnamecopy);
    return 1;
}

static int
cleanup(Module *m, Parameter **in, Parameter **out)
{
    Parameter **p;

    if (m->name)                free(m->name);
    if (m->category)            free(m->category);
    if (m->description)         free(m->description);
    if (m->outboard_host)       free(m->description);
    if (m->outboard_executable) free(m->outboard_executable);
    if (m->loadable_executable) free(m->loadable_executable);
    if (m->include_file)        free(m->include_file);

    for (p = in; *p != NULL; p++)
    {
	if ((*p)->name)          free((*p)->name);
	if ((*p)->description)   free((*p)->description);
	if ((*p)->default_value) free((*p)->default_value);
	if ((*p)->types)         free((*p)->types);
	free((char *)*p);
    }

    for (p = out; *p != NULL; p++)
    {
	if ((*p)->name)          free((*p)->name);
	if ((*p)->description)   free((*p)->description);
	if ((*p)->default_value) free((*p)->default_value);
	if ((*p)->types)         free((*p)->types);
	free((char *)*p);
    }

    return 1;
}

static int
do_makefile(char *basename, Module *mod)
{
    FILE      *fd = NULL;
    char      *buf = (char *)malloc(strlen(basename) + 7);
    char      *makefilename;
    char      *c;
    const char *uiroot = NULL;
    #define OBJ_EXT "$(OBJEXT)"

    if (! buf)
    {
	WarningMessage("allocation error");
	goto error;
    }

    sprintf(buf, "%s.make", basename);

    fd = fopen(buf, "w");
    if (! fd)
    {
	ErrorMessage("error opening output file");
	goto error;
    }

    if (! copy_comments(basename, fd, NULL, "# ", NULL))
	goto error;
    
    for (c = basename; *c; c++)
	if (*c == '/')
	    basename = c+1;

    uiroot = theMBApplication->getUIRoot();
    // This should not be necessary, but we'll be very
    // careful just before the 3.1 release
    if (!uiroot)
        uiroot = "/usr/local/dx";

    fprintf(fd, "SHELL = /bin/sh%s", NEWLINE);
    fprintf(fd, "BASE = %s%s%s",uiroot, NEWLINE, NEWLINE);

    fprintf(fd, "# need arch set, e.g. by%s", NEWLINE);
    fprintf(fd, "# setenv DXARCH `dx -whicharch`%s", NEWLINE);
    fprintf(fd, "include $(BASE)/lib_$(DXARCH)/arch.mak%s%s", NEWLINE, NEWLINE);

    if(mod->outboard_executable != NULL)
	fprintf(fd, "FILES_%s = %s.%s%s%s", basename, basename, OBJ_EXT, NEWLINE, NEWLINE);
    else if(mod->loadable_executable != NULL)
	fprintf(fd, "FILES_%s = user%s.%s %s.%s%s%s", 
                basename, basename, OBJ_EXT, basename, OBJ_EXT, NEWLINE, NEWLINE);
    else
	fprintf(fd, "FILES_%s = user%s.%s %s.%s%s%s", 
	    basename, basename, OBJ_EXT, basename, OBJ_EXT, NEWLINE, NEWLINE);

    fprintf(fd, "BIN = $(BASE)/bin%s%s", NEWLINE, NEWLINE);
    fprintf(fd, "#windows BIN = $(BASE)\\bin%s%s", NEWLINE, NEWLINE);
    fprintf(fd, "CFLAGS = -I./ -I$(BASE)/include $(DX_CFLAGS)%s%s", NEWLINE, NEWLINE);
    fprintf(fd, "LDFLAGS = -L$(BASE)/lib_$(DXARCH)%s%s", NEWLINE, NEWLINE);
    fprintf(fd, "LIBS = -lDX $(DX_GL_LINK_LIBS) $(DXEXECLINKLIBS)%s%s", NEWLINE, NEWLINE);
    fprintf(fd, "OLIBS = -lDXlite -lm%s%s", NEWLINE, NEWLINE);
    fprintf(fd, "BIN = $(BASE)/bin%s%s", NEWLINE, NEWLINE);


    fprintf(fd, "# create the necessary executable%s", NEWLINE);
    if(mod->outboard_executable != NULL)
    {
	fprintf(fd, "%s: $(FILES_%s) outboard.$(OBJEXT)%s",
		mod->outboard_executable,
		basename, NEWLINE);
	fprintf(fd, 
		"\t$(CC) $(DXABI) $(LDFLAGS) $(FILES_%s) outboard.$(OBJEXT) $(OLIBS) -o %s$(DOT_EXE_EXT)%s%s",
		basename,
		mod->outboard_executable, NEWLINE, NEWLINE);

	  fprintf(fd, "# how to make the outboard main routine%s", NEWLINE);
	  fprintf(fd, "outboard.$(OBJEXT): $(BASE)/lib/outboard.c%s", NEWLINE);
	  fprintf(fd, "\t%s%s%s%s",
	     "$(CC) $(DXABI) $(CFLAGS) -DUSERMODULE=m_",
	    mod->name,
	     " -c $(BASE)/lib/outboard.c", NEWLINE);
	  fprintf(fd, "%s%s", NEWLINE, NEWLINE);
    }
    else if(mod->loadable_executable != NULL)
    {   
       fprintf(fd, "%s: $(FILES_%s) %s",
               mod->loadable_executable, basename, NEWLINE);

       fprintf(fd, "\t$(SHARED_LINK) $(DXABI) $(DX_RTL_CFLAGS) $(LDFLAGS) -o %s user%s.$(OBJEXT) %s.$(OBJEXT) $(DX_RTL_LDFLAGS) $(SYSLIBS) $(DX_RTL_IMPORTS) $(DX_RTL_DXENTRY)%s%s",
               mod->loadable_executable, basename, basename, NEWLINE, NEWLINE);
#if 0
#ifdef alphax
       fprintf(fd, "\tld $(LDFLAGS) -o %s user%s.$(OBJEXT) %s.$(OBJEXT) $(SYSLIBS)%s%s",
               mod->loadable_executable, basename, basename, NEWLINE, NEWLINE);
#endif
#ifdef hp700
       fprintf(fd, "\tld $(LDFLAGS) -o %s user%s.$(OBJEXT) %s.$(OBJEXT) $(SYSLIBS)%s%s",
               mod->loadable_executable, basename, basename, NEWLINE, NEWLINE);
#endif
#ifdef ibm6000 
       fprintf(fd, "\tcc -o %s user%s.$(OBJEXT) %s.$(OBJEXT) -qmkshrobj -G  -bI:$(IMP)%s%s",
               mod->loadable_executable, basename, basename, NEWLINE, NEWLINE);
#endif
#ifdef DXD_WIN 
       fprintf(fd, "\t$(CC) $(CFLAGS) -o %s  user%s.$(OBJEXT) %s.$(OBJEXT)  $(SYSLIBS) $(OLIBS) %s%s",
               mod->loadable_executable, basename, basename, NEWLINE, NEWLINE);
#endif
#ifdef sgi 
       fprintf(fd, "\tcc $(LDFLAGS) -o %s user%s.$(OBJEXT) %s.$(OBJEXT) $(SYSLIBS)%s%s",
               mod->loadable_executable, basename, basename, NEWLINE, NEWLINE);
#endif
#ifdef solaris 
       fprintf(fd, "\tcc $(LDFLAGS) -o %s user%s.$(OBJEXT) %s.$(OBJEXT)%s%s",
               mod->loadable_executable, basename, basename, NEWLINE, NEWLINE);
#endif
#endif /* zero */
    }
    else
    {
	fprintf(fd, "dxexec: $(FILES_%s) %s",
		basename, NEWLINE);
	fprintf(fd, "\t$(CC) $(LDFLAGS) $(FILES_%s) $(LIBS) -o dxexec%s%s",
		basename, NEWLINE, NEWLINE);
#if 0
#if defined(DXD_WIN)  || defined(OS2)
	fprintf(fd, "dxexec.exe: $(FILES_%s) %s",
		basename, NEWLINE);
#else
	fprintf(fd, "dxexec: $(FILES_%s) %s",
		basename, NEWLINE);
#endif
#endif /* zero */

#if 0
#if   defined(DXD_WIN)
	fprintf(fd, "\t$(CC) (DXABI) $(LDFLAGS) $(FILES_%s) $(LIBS) -o dxexec.exe%s%s",
		basename, NEWLINE, NEWLINE);
#else
	fprintf(fd, "\t$(CC) $(DXABI) $(LDFLAGS) $(FILES_%s) $(LIBS) -o dxexec%s%s",
		basename, NEWLINE, NEWLINE);
#endif
#endif /* zero */
    }

      fprintf(fd, ".c.o: ; cc -c $(DXABI) $(DX_RTL_CFLAGS) $(CFLAGS) $*.c %s%s", NEWLINE, NEWLINE);
      fprintf(fd, ".C.o: ; cc -c $(DXABI) $(DX_RTL_CFLAGS) $(CFLAGS) $*.C %s%s", NEWLINE, NEWLINE);


    /* a target to run dx using the user module */

    fprintf(fd, "# a command to run the user module%s", NEWLINE);
    /* outboard module */
    if(mod->outboard_executable != NULL) {
      fprintf(fd, "run: %s %s", mod->outboard_executable, NEWLINE);
      fprintf(fd, "\tdx -edit -mdf %s.mdf &%s", basename, NEWLINE);
      fprintf(fd, "%s", NEWLINE);
    }
    else if(mod->loadable_executable != NULL) {
    /* runtime loadable module */
      fprintf(fd, "run: %s %s", mod->loadable_executable, NEWLINE);
      fprintf(fd, "\tdx -edit -mdf %s.mdf &%s", basename, NEWLINE);
      fprintf(fd, "%s", NEWLINE);
    }
    else {
    /* inboard module */
      fprintf(fd, "run: dxexec %s", NEWLINE);
      fprintf(fd, "\tdx -edit -exec ./dxexec -mdf %s.mdf &%s", basename, NEWLINE);
      fprintf(fd, "%s", NEWLINE);
    }

    fprintf(fd, "# make the user files%s", NEWLINE);
    fprintf(fd, "user%s.c: %s.mdf%s", basename, basename, NEWLINE);
  
#ifdef DXD_WIN    /* just to change forward and back slashes    */
    if(mod->loadable_executable != NULL) 
       fprintf(fd, "\t$(BIN)\\mdf2c -m %s.mdf > user%s.c%s", basename, basename, NEWLINE);
    else
       fprintf(fd, "\t$(BIN)\\mdf2c %s.mdf > user%s.c%s", basename, basename, NEWLINE);

#else
    if(mod->loadable_executable != NULL) 
       fprintf(fd, "\t$(BIN)/mdf2c -m %s.mdf > user%s.c%s", basename, basename, NEWLINE);
    else
       fprintf(fd, "\t$(BIN)/mdf2c %s.mdf > user%s.c%s", basename, basename, NEWLINE);
#endif

    fprintf(fd, "# kluge for when DXARCH isn't set%s", NEWLINE);
    fprintf(fd, "$(BASE)/lib_/arch.mak:%s", NEWLINE);
    makefilename=strrchr(buf,'/');
    if(!makefilename) makefilename = buf;
	else makefilename++;
    fprintf(fd, "\t(export DXARCH=`dx -whicharch` ; $(MAKE) -f %s )%s",makefilename, NEWLINE);
    fprintf(fd, "\techo YOU NEED TO SET DXARCH via dx -whicharch%s", NEWLINE);
    fclose(fd);
    free(buf);
    return 1;

error:
    if (fd)
    {
	fclose(fd);
	unlink(buf);
    }
    free(buf);
    return 0;
}

static int
do_mdf(char *basename, Module *mod, Parameter **in, Parameter **out)
{
    FILE      *fd = NULL;
    char      *buf = (char *)malloc(strlen(basename) + 5);

    if (! buf)
    {
	ErrorMessage("allocation error");
	goto error;
    }

    sprintf(buf, "%s.mdf", basename);

    fd = fopen(buf, "w");
    if (! fd)
    {
	ErrorMessage("error opening output file");
	goto error;
    }

    if (! copy_comments(basename, fd, NULL, "# ", NULL))
	goto error;

    fprintf(fd, "MODULE %s%s", mod->name, NEWLINE);
    fprintf(fd, "CATEGORY %s%s", mod->category, NEWLINE);
    fprintf(fd, "DESCRIPTION %s%s", mod->description, NEWLINE);

    if (mod->outboard_persistent == TRUE ||
	mod->asynchronous  == TRUE       ||
	mod->pinned  == TRUE             ||
	mod->side_effect == TRUE)
    {
	int first = 1;
	fprintf(fd, "FLAGS");
	if (mod->outboard_persistent == TRUE)
	    fprintf(fd, "%sPERSISTENT", first++ ? " " : ", ");
	if (mod->asynchronous == TRUE)
	    fprintf(fd, "%sASYNC", first++ ? " " : ", ");
	if (mod->pinned == TRUE)
	    fprintf(fd, "%sPIN", first++ ? " " : ", ");
	if (mod->side_effect == TRUE)
	    fprintf(fd, "%sSIDE_EFFECT", first++ ? " " : ", ");
	fprintf(fd, "%s", NEWLINE);
    }

    if (mod->outboard_executable)
    {
	fprintf(fd, "OUTBOARD %s;", mod->outboard_executable);
	if (mod->outboard_host)
	    fprintf(fd, "%s", mod->outboard_host);
	fprintf(fd, "%s", NEWLINE);
    }

    if (mod->loadable_executable)
    {
	fprintf(fd, "LOADABLE %s;", mod->loadable_executable);
	fprintf(fd, "%s", NEWLINE);
    }

    while (*in)
    {
	char *c;
	fprintf(fd, "INPUT %s; ", (*in)->name);

	if ((*in)->types)
	{
	    char *buf, *b, *c;
	    int i, nc;

	    for (c = (*in)->types, nc = 0; *c; c++)
		if (*c == ',') nc++;
	  
            /* integer vector and integer vector list are not understood by
               the ui */ 
            if (!strcmp((*in)->types,"integer vector")) {
	        buf = b = (char *)malloc(strlen("vector") + 4*nc + 1);
	        for (c = "vector", nc = 0; *c; c++)
	           *b++ = *c;
            }
            else if (!strcmp((*in)->types,"integer vector list")) {
	        buf = b = (char *)malloc(strlen("vector list") + 4*nc + 1);
	        for (c = "vector list", nc = 0; *c; c++)
	           *b++ = *c;
            }
            else {
	        buf = b = (char *)malloc(strlen((*in)->types) + 4*nc + 1);
	        for (c = (*in)->types, nc = 0; *c; c++)
		    *b++ = *c;
            }
         

	    *b = '\0';

	    fprintf(fd, "%s; ", buf);

	    free(buf);
	}
	else if ((*in)->structure == VALUE)
	    fprintf(fd, "value; ");
	else if ((*in)->structure == GROUP_FIELD)
	    fprintf(fd, "group; ");
	else
	{
	    ErrorMessage("input %s has no STRUCTURE parameter\n", (*in)->name);
	    goto error;
	}
	if ((*in)->required == TRUE)
	    fprintf(fd, " (none);");
	else if ((*in)->descriptive == TRUE)
	    fprintf(fd, " (%s);", (*in)->default_value);
	else
	    fprintf(fd, " %s;", (*in)->default_value);
	fprintf(fd, " %s%s", (*in)->description, NEWLINE);
	in++;
    }

    while (*out)
    {
	char *c;
	fprintf(fd, "OUTPUT %s; ", (*out)->name);

	if ((*out)->types)
	{
	    char *buf, *b, *c;
	    int i, nc;

	    for (c = (*out)->types, nc = 0; *c; c++)
		if (*c == ',') nc++;
	    
	    buf = b = (char *)malloc(strlen((*out)->types) + 4*nc + 1);

	    for (c = (*out)->types, nc = 0; *c; c++)
		if (*c == ',')
		{
		    *b++ = ' ';
		    *b++ = 'o';
		    *b++ = 'r';
		    *b++ = ' ';
		}
		else
		    *b++ = *c;
	    
	    *b = '\0';

	    fprintf(fd, "%s; ", buf);

	    free(buf);
	}
	else if ((*out)->structure == VALUE)
	    fprintf(fd, "value; ");
	else if ((*out)->structure == GROUP_FIELD)
	    fprintf(fd, "group; ");
	else
	{
	    ErrorMessage("output %s has no STRUCTURE parameter\n", (*out)->name);
	    goto error;
	}
	fprintf(fd, " %s%s", (*out)->description, NEWLINE);
	out++;
    }
    
    fclose(fd);
    free(buf);
    return 1;

error:
    if (fd)
    {
	fclose(fd);
	unlink(buf);
    }
    free(buf);
    return 0;
}

static int
do_builder(char *basename, Module *mod, Parameter **in, Parameter **out)
{
    FILE       *fd = NULL;
    int        i, nin, nout, n_GF_in, n_GF_out, open;
    char       *buf = (char *)malloc(strlen(basename) + 4);

    if (! buf)
    {
	ErrorMessage("allocation error");
	goto error;
    }

    sprintf(buf, "%s.c", basename);

    fd = fopen(buf, "w");
    if (! fd)
    {
	ErrorMessage("error opening output file");
	goto error;
    }

    if (! copy_comments(basename, fd, "/*\n", " * ", " */\n"))
	goto error;

    for (nin  = 0; in[nin] != NULL; nin++);
    for (nout = 0; out[nout] != NULL; nout++);

fprintf(fd, "/*%s", NEWLINE);
fprintf(fd, " * Automatically generated on \"%s.mb\" by DX Module Builder%s",
				basename, NEWLINE);
fprintf(fd, " */%s%s", NEWLINE, NEWLINE);

fprintf(fd, "/* define your pre-dx.h include file for inclusion here*/ %s", NEWLINE);
fprintf(fd, "#ifdef PRE_DX_H%s", NEWLINE);
fprintf(fd, "#include \"%s_predx.h\"%s",mod->name, NEWLINE);
fprintf(fd, "#endif%s", NEWLINE);
fprintf(fd, "#include \"dx/dx.h\"%s", NEWLINE);
fprintf(fd, "/* define your post-dx.h include file for inclusion here*/ %s", NEWLINE);
fprintf(fd, "#ifdef POST_DX_H%s", NEWLINE);
fprintf(fd, "#include \"%s_postdx.h\"%s",mod->name, NEWLINE);
fprintf(fd, "#endif%s", NEWLINE);
fprintf(fd, "%s", NEWLINE);
fprintf(fd, "static Error traverse(Object *, Object *);%s", NEWLINE);
fprintf(fd, "static Error doLeaf(Object *, Object *);%s", NEWLINE);
fprintf(fd, "%s", NEWLINE);
fprintf(fd, "/*%s", NEWLINE);
fprintf(fd, " * Declare the interface routine.%s", NEWLINE);
fprintf(fd, " */%s", NEWLINE);
fprintf(fd, "int%s%s_worker(%s", NEWLINE, mod->name, NEWLINE);

    open = 0;

    if (in[0]->positions == GRID_REGULAR)
    {
	open = 1;
fprintf(fd, "    int, int, int *, float *, float *");
    }
    else if (in[0]->positions == GRID_IRREGULAR)
    {
	open = 1;
fprintf(fd, "    int, int, float *");
    }

    if (in[0]->connections == GRID_REGULAR)
    {
fprintf(fd, "%s   int, int, int *", (open)?COMMA_AND_NEWLINE:"");
	open = 1;
    }
    else if (in[0]->connections == GRID_IRREGULAR)
    {
fprintf(fd, "%s    int, int, int *", (open)?COMMA_AND_NEWLINE:"");
	open = 1;
    }

    for (i = 0; i < nin; i++)
    { 
	char *type;
	GetCDataType(in[i]->datatype, &type);
fprintf(fd, "%s    int, %s *", (open)?COMMA_AND_NEWLINE:"", type);
	open = 1;
    }

    for (i = 0; i < nout; i++)
    { 
	char *type;
	GetCDataType(out[i]->datatype, &type);
fprintf(fd, "%s    int, %s *", (open)?COMMA_AND_NEWLINE:"", type);
	open = 1;
    }

fprintf(fd, ");%s%s", NEWLINE, NEWLINE);

fprintf(fd, "#if defined (__cplusplus) || defined (c_plusplus)%s", NEWLINE);
fprintf(fd, "extern \"C\"%s", NEWLINE);
fprintf(fd, "#endif%s", NEWLINE);
fprintf(fd, "Error%sm_%s(Object *in, Object *out)%s", NEWLINE, mod->name, NEWLINE);
fprintf(fd, "{%s", NEWLINE);
fprintf(fd, "  int i;%s", NEWLINE);
fprintf(fd, "%s", NEWLINE);
fprintf(fd, "  /*%s", NEWLINE);
fprintf(fd, "   * Initialize all outputs to NULL%s", NEWLINE);
fprintf(fd, "   */%s", NEWLINE);
    if (nout == 1)
fprintf(fd, "  out[0] = NULL;%s", NEWLINE);
    else if (nout > 1)
    {
fprintf(fd, "  for (i = 0; i < %d; i++)%s", nout, NEWLINE);
fprintf(fd, "    out[i] = NULL;%s", NEWLINE);
    }

fprintf(fd, "%s", NEWLINE);

fprintf(fd, "  /*%s", NEWLINE);
fprintf(fd, "   * Error checks: required inputs are verified.%s", NEWLINE);
fprintf(fd, "   */%s%s", NEWLINE, NEWLINE);

    n_GF_in = 0;
    for (i = 0; i < nin; i++)
    {
	if (in[i]->structure == GROUP_FIELD)
	    n_GF_in ++;

	if (in[i]->required == TRUE || i == 0)
	{
fprintf(fd, "  /* Parameter \"%s\" is required. */%s", in[i]->name, NEWLINE);
fprintf(fd, "  if (in[%d] == NULL)%s", i, NEWLINE);
fprintf(fd, "  {%s", NEWLINE);
fprintf(fd, "    DXSetError(ERROR_MISSING_DATA, ");
fprintf(fd, "\"\\\"%s\\\" must be specified\");%s", in[i]->name, NEWLINE);
fprintf(fd, "    return ERROR;%s", NEWLINE);
fprintf(fd, "  }%s%s", NEWLINE, NEWLINE);
	}
    }

    n_GF_out = 0;
    for (i = 0; i < nout; i++)
    {
fprintf(fd, "%s", NEWLINE);

	if (out[i]->structure == GROUP_FIELD)
	{
	    n_GF_out ++;

fprintf(fd, "  /*%s", NEWLINE);
fprintf(fd, "   * Since output \"%s\" is structure Field/Group, it initially%s", out[i]->name, NEWLINE);
fprintf(fd, "   * is a copy of input \"%s\".%s", in[0]->name, NEWLINE);
fprintf(fd, "   */%s", NEWLINE);
fprintf(fd, "  out[%d] = DXCopy(in[0], COPY_STRUCTURE);%s", i, NEWLINE);
fprintf(fd, "  if (! out[%d])%s", i, NEWLINE);
fprintf(fd, "    goto error;%s%s", NEWLINE, NEWLINE);
fprintf(fd, "  /*%s", NEWLINE);
fprintf(fd, "   * If in[0] was an array, then no copy is actually made - Copy %s", NEWLINE);
fprintf(fd, "   * returns a pointer to the input object.  Since this can't be written to%s", NEWLINE);
fprintf(fd, "   * we postpone explicitly copying it until the leaf level, when we'll need%s", NEWLINE);
fprintf(fd, "   * to be creating writable arrays anyway.%s", NEWLINE);
fprintf(fd, "   */%s", NEWLINE);
fprintf(fd, "  if (out[%d] == in[0])%s", i, NEWLINE);
fprintf(fd, "    out[%d] = NULL;%s", i, NEWLINE);
	}
	else if (out[i]->structure == VALUE)
	{
	    char *t;
	    char *r, *s;
	    char *l;

	    if (! GetDXDataType(out[i]->datatype, &t))
	    {
		ErrorMessage("cannot create array without DATATYPE");
		goto error;
	    }

	    if (! GetDXDataShape(out[i]->datashape, &r, &s))
	    {
		ErrorMessage("cannot create array without DATASHAPE");
		goto error;
	    }

#if 0
	    switch(out[i]->counts)
	    {
		case COUNTS_1: l = "1"; break;
		default:
		    ErrorMessage("cannot create array without COUNTS");
		    goto error;
	    }
#endif
	    l = "1";

fprintf(fd, "  /*%s", NEWLINE);
fprintf(fd, "   * Output \"%s\" is a value; set up an appropriate array%s", out[i]->name, NEWLINE);
fprintf(fd, "   */%s", NEWLINE);
fprintf(fd, "  out[%d] = (Object)DXNewArray(%s, CATEGORY_REAL, %s, %s);%s", i, t, r, s, NEWLINE);
fprintf(fd, "  if (! out[%d])%s", i, NEWLINE);
fprintf(fd, "    goto error;%s", NEWLINE);
fprintf(fd, "  if (! DXAddArrayData((Array)out[%d], 0, %s, NULL))%s", i, l, NEWLINE);
fprintf(fd, "    goto error;%s", NEWLINE);
fprintf(fd, "  memset(DXGetArrayData((Array)out[%d]), 0, %s*DXGetItemSize((Array)out[%d]));%s", i, l, i, NEWLINE);
fprintf(fd, "%s", NEWLINE);
	}
    }

fprintf(fd, "%s", NEWLINE);

fprintf(fd, "  /*%s", NEWLINE);
fprintf(fd, "   * Call the hierarchical object traversal routine%s", NEWLINE);
fprintf(fd, "   */%s", NEWLINE);
fprintf(fd, "  if (!traverse(in, out))%s", NEWLINE);
fprintf(fd, "    goto error;%s", NEWLINE);

fprintf(fd, "%s", NEWLINE);

fprintf(fd, "  return OK;%s", NEWLINE);
fprintf(fd, "%s", NEWLINE);
fprintf(fd, "error:%s", NEWLINE);
fprintf(fd, "  /*%s", NEWLINE);
fprintf(fd, "   * On error, any successfully-created outputs are deleted.%s", NEWLINE);
fprintf(fd, "   */%s", NEWLINE);
fprintf(fd, "  for (i = 0; i < %d; i++)%s", nout, NEWLINE);
fprintf(fd, "  {%s", NEWLINE);
fprintf(fd, "    if (in[i] != out[i])%s", NEWLINE);
fprintf(fd, "      DXDelete(out[i]);%s", NEWLINE);
fprintf(fd, "    out[i] = NULL;%s", NEWLINE);
fprintf(fd, "  }%s", NEWLINE);
fprintf(fd, "  return ERROR;%s", NEWLINE);
fprintf(fd, "}%s", NEWLINE);

fprintf(fd, "%s%s", NEWLINE, NEWLINE);
fprintf(fd, "static Error%s", NEWLINE);
fprintf(fd, "traverse(Object *in, Object *out)%s", NEWLINE);
fprintf(fd, "{%s", NEWLINE);
fprintf(fd, "  switch(DXGetObjectClass(in[0]))%s", NEWLINE);
fprintf(fd, "  {%s", NEWLINE);
fprintf(fd, "    case CLASS_FIELD:%s", NEWLINE);
fprintf(fd, "    case CLASS_ARRAY:%s", NEWLINE);
fprintf(fd, "    case CLASS_STRING:%s", NEWLINE);
fprintf(fd, "      /*%s", NEWLINE);
fprintf(fd, "       * If we have made it to the leaf level, call the leaf handler.%s", NEWLINE);
fprintf(fd, "       */%s", NEWLINE);
fprintf(fd, "      if (! doLeaf(in, out))%s", NEWLINE);
fprintf(fd, "  	     return ERROR;%s", NEWLINE);
fprintf(fd, "%s", NEWLINE);
fprintf(fd, "      return OK;%s", NEWLINE);
fprintf(fd, "%s", NEWLINE);
fprintf(fd, "    case CLASS_GROUP:%s", NEWLINE);
fprintf(fd, "    {%s", NEWLINE);
fprintf(fd, "      int   i, j;%s", NEWLINE);
fprintf(fd, "      int   memknt;%s", NEWLINE);
fprintf(fd, "      Class groupClass  = DXGetGroupClass((Group)in[0]);%s", NEWLINE);
fprintf(fd, "%s", NEWLINE);
fprintf(fd, "      DXGetMemberCount((Group)in[0], &memknt);%s", NEWLINE);
fprintf(fd, "%s", NEWLINE);
if (n_GF_in > 1)
{
fprintf(fd, "      /*%s", NEWLINE);
fprintf(fd, "       * All inputs that are not NULL and are type Field/Group must%s", NEWLINE);
fprintf(fd, "       * match the structure of input[0].  Verify that this is so.%s", NEWLINE);
fprintf(fd, "       */%s", NEWLINE);
    for (i = 1; i < nin; i++)
	if (in[i]->structure == GROUP_FIELD)
	{
fprintf(fd, "       if (in[%d] &&%s", i, NEWLINE);
fprintf(fd, "            (DXGetObjectClass(in[%d]) != CLASS_GROUP ||%s", i, NEWLINE);
fprintf(fd, "             DXGetGroupClass((Group)in[%d])  != groupClass  ||%s", i, NEWLINE);
fprintf(fd, "             !DXGetMemberCount((Group)in[%d], &i) || i != memknt))%s", i, NEWLINE);
fprintf(fd, "	      {%s", NEWLINE);
fprintf(fd, "  		DXSetError(ERROR_DATA_INVALID,%s", NEWLINE);
fprintf(fd, "	            \"structure of \\\"%s\\\" doesn't match that of \\\"%s\\\"\");%s",
			    in[i]->name, in[0]->name, NEWLINE);
fprintf(fd, "  	         return ERROR;%s", NEWLINE);
fprintf(fd, "         }%s%s", NEWLINE, NEWLINE);
	}
}
else
fprintf(fd, "%s", NEWLINE);

fprintf(fd, "       /*%s", NEWLINE);
fprintf(fd, "        * Create new in and out lists for each child%s", NEWLINE);
fprintf(fd, "        * of the first input. %s", NEWLINE);
fprintf(fd, "        */%s", NEWLINE);
fprintf(fd, "        for (i = 0; i < memknt; i++)%s", NEWLINE);
fprintf(fd, "        {%s", NEWLINE);
fprintf(fd, "          Object new_in[%d], new_out[%d];%s", nin, nout, NEWLINE);
fprintf(fd, "%s", NEWLINE);

fprintf(fd, "         /*%s", NEWLINE);
fprintf(fd, "          * For all inputs that are Values, pass them to %s", NEWLINE);
fprintf(fd, "          * child object list.  For all that are Field/Group, get %s", NEWLINE);
fprintf(fd, "          * the appropriate decendent and place it into the%s", NEWLINE);
fprintf(fd, "          * child input object list.%s", NEWLINE);
fprintf(fd, "          */%s%s", NEWLINE, NEWLINE);
for (i = 0; i < nin; i++)
    if (in[i]->structure == VALUE)
    {
fprintf(fd, "          /* input \"%s\" is Value */%s", in[i]->name, NEWLINE);
fprintf(fd, "          new_in[%d] = in[%d];%s%s", i, i, NEWLINE, NEWLINE);
    }
    else if (in[i]->structure == GROUP_FIELD)
    {
fprintf(fd, "          /* input \"%s\" is Field/Group */%s", in[i]->name, NEWLINE);
fprintf(fd, "          if (in[%d])%s", i, NEWLINE);
fprintf(fd, "            new_in[%d] = DXGetEnumeratedMember((Group)in[%d], i, NULL);%s", i, i, NEWLINE);
fprintf(fd, "          else%s", NEWLINE);
fprintf(fd, "            new_in[%d] = NULL;%s", i, NEWLINE);
fprintf(fd, "%s", NEWLINE);
    }

fprintf(fd, "         /*%s", NEWLINE);
fprintf(fd, "          * For all outputs that are Values, pass them to %s", NEWLINE);
fprintf(fd, "          * child object list.  For all that are Field/Group,  get%s", NEWLINE);
fprintf(fd, "          * the appropriate decendent and place it into the%s", NEWLINE);
fprintf(fd, "          * child output object list.  Note that none should%s", NEWLINE);
fprintf(fd, "          * be NULL (unlike inputs, which can default).%s", NEWLINE);
fprintf(fd, "          */%s%s", NEWLINE, NEWLINE);
for (i = 0; i < nout; i++)
    if (out[i]->structure == VALUE)
    {
fprintf(fd, "          /* output \"%s\" is Value */%s", out[i]->name, NEWLINE);
fprintf(fd, "          new_out[%d] = out[%d];%s%s", i, i, NEWLINE, NEWLINE);
    }
    else
    {
fprintf(fd, "          /* output \"%s\" is Field/Group */%s", out[i]->name, NEWLINE);
fprintf(fd, "          new_out[%d] = DXGetEnumeratedMember((Group)out[%d], i, NULL);%s%s", i, i, NEWLINE, NEWLINE);
    }

fprintf(fd, "          if (! traverse(new_in, new_out))%s", NEWLINE);
fprintf(fd, "            return ERROR;%s", NEWLINE);
fprintf(fd, "%s", NEWLINE);

if (n_GF_out)
{
fprintf(fd, "         /*%s", NEWLINE);
fprintf(fd, "          * Now for each output that is not a Value, replace%s", NEWLINE);
fprintf(fd, "          * the updated child into the object in the parent.%s", NEWLINE);
fprintf(fd, "          */%s%s", NEWLINE, NEWLINE);
    for (i = 0; i < nout; i++)
	if (out[i]->structure == GROUP_FIELD)
	{
fprintf(fd, "          /* output \"%s\" is Field/Group */%s", out[i]->name, NEWLINE);
fprintf(fd, "          DXSetEnumeratedMember((Group)out[%d], i, new_out[%d]);%s%s", i, i, NEWLINE, NEWLINE);
	}
}

fprintf(fd, "        }%s", NEWLINE);
fprintf(fd, "      return OK;%s", NEWLINE);
fprintf(fd, "    }%s", NEWLINE);
fprintf(fd, "%s", NEWLINE);
fprintf(fd, "    case CLASS_XFORM:%s", NEWLINE);
fprintf(fd, "    {%s", NEWLINE);
fprintf(fd, "      int    i, j;%s", NEWLINE);
fprintf(fd, "      Object new_in[%d], new_out[%d];%s", nin, nout, NEWLINE);
fprintf(fd, "%s", NEWLINE);

if (n_GF_in > 1)
{
fprintf(fd, "      /*%s", NEWLINE);
fprintf(fd, "       * All inputs that are not NULL and are type Field/Group must%s", NEWLINE);
fprintf(fd, "       * match the structure of input[0].  Verify that this is so.%s", NEWLINE);
fprintf(fd, "       */%s", NEWLINE);
    for (i = 1; i < nin; i++)
	if (in[i]->structure == GROUP_FIELD)
	{
fprintf(fd, "      if (in[%d] && DXGetObjectClass(in[%d]) != CLASS_XFORM)%s", i, i, NEWLINE);
fprintf(fd, "      {%s", NEWLINE);
fprintf(fd, "        DXSetError(ERROR_DATA_INVALID,%s", NEWLINE);
fprintf(fd, "	            \"structure of \\\"%s\\\" doesn't match that of \\\"%s\\\"\");%s",
			    in[i]->name, in[0]->name, NEWLINE);
fprintf(fd, "        return ERROR;%s", NEWLINE);
fprintf(fd, "      }%s%s", NEWLINE, NEWLINE);
	}
}
else
fprintf(fd, "%s", NEWLINE);

fprintf(fd, "      /*%s", NEWLINE);
fprintf(fd, "       * Create new in and out lists for the decendent of the%s", NEWLINE);
fprintf(fd, "       * first input.  For inputs and outputs that are Values%s", NEWLINE);
fprintf(fd, "       * copy them into the new in and out lists.  Otherwise%s", NEWLINE);
fprintf(fd, "       * get the corresponding decendents.%s", NEWLINE);
fprintf(fd, "       */%s%s", NEWLINE, NEWLINE);
for (i = 0; i < nin; i++)
    if (in[i]->structure == VALUE)
    {
fprintf(fd, "      /* input \"%s\" is Value */%s", in[i]->name, NEWLINE);
fprintf(fd, "      new_in[%d] = in[%d];%s%s", i, i, NEWLINE, NEWLINE);
    }
    else
    {
fprintf(fd, "      /* input \"%s\" is Field/Group */%s", in[i]->name, NEWLINE);
fprintf(fd, "      if (in[%d])%s", i, NEWLINE);
fprintf(fd, "        DXGetXformInfo((Xform)in[%d], &new_in[%d], NULL);%s", i, i, NEWLINE);
fprintf(fd, "      else%s", NEWLINE);
fprintf(fd, "        new_in[%d] = NULL;%s", i, NEWLINE);
fprintf(fd, "%s", NEWLINE);
    }
fprintf(fd, "      /*%s", NEWLINE);
fprintf(fd, "       * For all outputs that are Values, copy them to %s", NEWLINE);
fprintf(fd, "       * child object list.  For all that are Field/Group,  get%s", NEWLINE);
fprintf(fd, "       * the appropriate decendent and place it into the%s", NEWLINE);
fprintf(fd, "       * child output object list.  Note that none should%s", NEWLINE);
fprintf(fd, "       * be NULL (unlike inputs, which can default).%s", NEWLINE);
fprintf(fd, "       */%s%s", NEWLINE, NEWLINE);
for (i = 0; i < nout; i++)
    if (out[i]->structure == VALUE)
    {
fprintf(fd, "      /* output \"%s\" is Value */%s", out[i]->name, NEWLINE);
fprintf(fd, "      new_out[%d] = out[%d];%s%s", i, i, NEWLINE, NEWLINE);
    }
    else
    {
fprintf(fd, "      /* output \"%s\" is Field/Group */%s", out[i]->name, NEWLINE);
fprintf(fd, "      DXGetXformInfo((Xform)out[%d], &new_out[%d], NULL);%s%s", i,i, NEWLINE, NEWLINE);
    }
fprintf(fd, "      if (! traverse(new_in, new_out))%s", NEWLINE);
fprintf(fd, "        return ERROR;%s", NEWLINE);
fprintf(fd, "%s", NEWLINE);

if (n_GF_out)
{
fprintf(fd, "      /*%s", NEWLINE);
fprintf(fd, "       * Now for each output that is not a Value replace%s", NEWLINE);
fprintf(fd, "       * the updated child into the object in the parent.%s", NEWLINE);
fprintf(fd, "       */%s%s", NEWLINE, NEWLINE);
    for (i = 0; i < nout; i++)
	if (out[i]->structure == GROUP_FIELD) 
	{
fprintf(fd, "      /* output \"%s\" is Field/Group */%s", out[i]->name, NEWLINE);
fprintf(fd, "      DXSetXformObject((Xform)out[%d], new_out[%d]);%s%s", i, i, NEWLINE, NEWLINE);
	}
}

fprintf(fd, "      return OK;%s", NEWLINE);
fprintf(fd, "    }%s", NEWLINE);
fprintf(fd, "%s", NEWLINE);
fprintf(fd, "    case CLASS_SCREEN:%s", NEWLINE);
fprintf(fd, "    {%s", NEWLINE);
fprintf(fd, "      int    i, j;%s", NEWLINE);
fprintf(fd, "      Object new_in[%d], new_out[%d];%s", nin, nout, NEWLINE);
fprintf(fd, "%s", NEWLINE);
if (n_GF_in > 1)
{
fprintf(fd, "      /*%s", NEWLINE);
fprintf(fd, "       * All inputs that are not NULL and are type Field/Group must%s", NEWLINE);
fprintf(fd, "       * match the structure of input[0].  Verify that this is so.%s", NEWLINE);
fprintf(fd, "       */%s%s", NEWLINE, NEWLINE);
    for (i = 1; i < nin; i++)
	if (in[i]->structure == GROUP_FIELD)
	{
fprintf(fd, "       if (in[%d] && DXGetObjectClass(in[%d]) != CLASS_SCREEN)%s", i, i, NEWLINE);
fprintf(fd, "       {%s", NEWLINE);
fprintf(fd, "           DXSetError(ERROR_DATA_INVALID,%s", NEWLINE);
fprintf(fd, "	            \"structure of \\\"%s\\\" doesn't match that of \\\"%s\\\"\");%s",
			    in[i]->name, in[0]->name, NEWLINE);
fprintf(fd, "           return ERROR;%s", NEWLINE);
fprintf(fd, "       }%s%s", NEWLINE, NEWLINE);
	}
}
else
fprintf(fd, "%s", NEWLINE);

fprintf(fd, "      /*%s", NEWLINE);
fprintf(fd, "       * Create new in and out lists for the decendent of the%s", NEWLINE);
fprintf(fd, "       * first input.  For inputs and outputs that are Values%s", NEWLINE);
fprintf(fd, "       * copy them into the new in and out lists.  Otherwise%s", NEWLINE);
fprintf(fd, "       * get the corresponding decendents.%s", NEWLINE);
fprintf(fd, "       */%s%s", NEWLINE, NEWLINE);
for (i = 0; i < nin; i++)
    if (in[i]->structure == VALUE)
    {
fprintf(fd, "      /* input \"%s\" is Value */%s", in[i]->name, NEWLINE);
fprintf(fd, "      new_in[%d] = in[%d];%s%s", i, i, NEWLINE, NEWLINE);
    }
    else
    {
fprintf(fd, "      /* input \"%s\" is Field/Group */%s", in[i]->name, NEWLINE);
fprintf(fd, "      if (in[%d])%s", i, NEWLINE);
fprintf(fd, "        DXGetScreenInfo((Screen)in[%d], &new_in[%d], NULL, NULL);%s",i,i, NEWLINE);
fprintf(fd, "      else%s", NEWLINE);
fprintf(fd, "        new_in[%d] = NULL;%s%s", i, NEWLINE,NEWLINE);
    }
fprintf(fd, "%s", NEWLINE);
fprintf(fd, "      /*%s", NEWLINE);
fprintf(fd, "       * For all outputs that are Values, copy them to %s", NEWLINE);
fprintf(fd, "       * child object list.  For all that are Field/Group,  get%s", NEWLINE);
fprintf(fd, "       * the appropriate decendent and place it into the%s", NEWLINE);
fprintf(fd, "       * child output object list.  Note that none should%s", NEWLINE);
fprintf(fd, "       * be NULL (unlike inputs, which can default).%s", NEWLINE);
fprintf(fd, "       */%s%s", NEWLINE, NEWLINE);
for (i = 0; i < nout; i++)
    if (out[i]->structure == VALUE)
    {
fprintf(fd, "       /* output \"%s\" is Value */%s", out[i]->name, NEWLINE);
fprintf(fd, "       new_out[%d] = out[%d];%s%s", i, i, NEWLINE, NEWLINE);
    }
    else
    {
fprintf(fd, "       /* output \"%s\" is Field/Group */%s", out[i]->name, NEWLINE);
fprintf(fd, "       DXGetScreenInfo((Screen)out[%d], &new_out[%d], NULL, NULL);%s%s", i,i, NEWLINE, NEWLINE);
    }

fprintf(fd, "       if (! traverse(new_in, new_out))%s", NEWLINE);
fprintf(fd, "         return ERROR;%s", NEWLINE);
fprintf(fd, "%s", NEWLINE);

if (n_GF_out)
{
fprintf(fd, "      /*%s", NEWLINE);
fprintf(fd, "       * Now for each output that is not a Value, replace%s", NEWLINE);
fprintf(fd, "       * the updated child into the object in the parent.%s", NEWLINE);
fprintf(fd, "       */%s%s", NEWLINE, NEWLINE);
for (i = 0; i < nout; i++)
    if (out[i]->structure == GROUP_FIELD)
    {
fprintf(fd, "      /* output \"%s\" is Field/Group */%s", out[i]->name, NEWLINE);
fprintf(fd, "       DXSetScreenObject((Screen)out[%d], new_out[%d]);%s%s", i,i, NEWLINE, NEWLINE);
    }
}

fprintf(fd, "       return OK;%s", NEWLINE);
fprintf(fd, "     }%s", NEWLINE);
fprintf(fd, "%s", NEWLINE);
fprintf(fd, "     case CLASS_CLIPPED:%s", NEWLINE);
fprintf(fd, "     {%s", NEWLINE);
fprintf(fd, "       int    i, j;%s", NEWLINE);
fprintf(fd, "       Object new_in[%d], new_out[%d];%s", nin, nout, NEWLINE);
fprintf(fd, "%s", NEWLINE);
if (n_GF_in > 1)
{
fprintf(fd, "      /*%s", NEWLINE);
fprintf(fd, "       * All inputs that are not NULL and are type Field/Group must%s", NEWLINE);
fprintf(fd, "       * match the structure of input[0].  Verify that this is so.%s", NEWLINE);
fprintf(fd, "       */%s%s", NEWLINE, NEWLINE);
    for (i = 1; i < nin; i++)
	if (in[i]->structure == GROUP_FIELD)
	{
fprintf(fd, "       if (in[%d] && DXGetObjectClass(in[%d]) != CLASS_CLIPPED)%s", i, i, NEWLINE);
fprintf(fd, "       {%s", NEWLINE);
fprintf(fd, "           DXSetError(ERROR_DATA_INVALID,%s", NEWLINE);
fprintf(fd, "               \"mismatching Field/Group objects\");%s", NEWLINE);
fprintf(fd, "           return ERROR;%s", NEWLINE);
fprintf(fd, "       }%s%s", NEWLINE, NEWLINE);
	}
}
else
fprintf(fd, "%s", NEWLINE);

for (i = 0; i < nin; i++)
    if (in[i]->structure == VALUE)
    {
fprintf(fd, "       /* input \"%s\" is Value */%s", in[i]->name, NEWLINE);
fprintf(fd, "       new_in[%d] = in[%d];%s%s", i, i, NEWLINE, NEWLINE);
    }
    else
    {
fprintf(fd, "       /* input \"%s\" is Field/Group */%s", in[i]->name, NEWLINE);
fprintf(fd, "       if (in[%d])%s", i, NEWLINE);
fprintf(fd, "         DXGetClippedInfo((Clipped)in[%d], &new_in[%d], NULL);%s",i,i, NEWLINE);
fprintf(fd, "       else%s", NEWLINE);
fprintf(fd, "         new_in[%d] = NULL;%s%s", i, NEWLINE, NEWLINE);
    }
fprintf(fd, "%s", NEWLINE);
fprintf(fd, "      /*%s", NEWLINE);
fprintf(fd, "       * For all outputs that are Values, copy them to %s", NEWLINE);
fprintf(fd, "       * child object list.  For all that are Field/Group,  get%s", NEWLINE);
fprintf(fd, "       * the appropriate decendent and place it into the%s", NEWLINE);
fprintf(fd, "       * child output object list.  Note that none should%s", NEWLINE);
fprintf(fd, "       * be NULL (unlike inputs, which can default).%s", NEWLINE);
fprintf(fd, "       */%s%s", NEWLINE, NEWLINE);
for (i = 0; i < nout; i++)
    if (out[i]->structure == VALUE)
    {
fprintf(fd, "       /* output \"%s\" is Value */%s", out[i]->name, NEWLINE);
fprintf(fd, "       new_out[%d] = out[%d];%s%s", i, i, NEWLINE, NEWLINE);
    }
    else
    {
fprintf(fd, "       /* output \"%s\" is Field/Group */%s", out[i]->name, NEWLINE);
fprintf(fd, "       DXGetClippedInfo((Clipped)out[%d], &new_out[%d], NULL);%s%s", i,i, NEWLINE, NEWLINE);
    }

fprintf(fd, "       if (! traverse(new_in, new_out))%s", NEWLINE);
fprintf(fd, "         return ERROR;%s", NEWLINE);
fprintf(fd, "%s", NEWLINE);

if (n_GF_out)
{
fprintf(fd, "      /*%s", NEWLINE);
fprintf(fd, "       * Now for each output that is not a Value, replace%s", NEWLINE);
fprintf(fd, "       * the updated child into the object in the parent.%s", NEWLINE);
fprintf(fd, "       */%s%s", NEWLINE, NEWLINE);
for (i = 0; i < nout; i++)
    if (out[i]->structure == GROUP_FIELD)
    {
fprintf(fd, "       /* output \"%s\" is Field/Group */%s", out[i]->name, NEWLINE);
fprintf(fd, "       DXSetClippedObjects((Clipped)out[%d], new_out[%d], NULL);%s%s", i, i, NEWLINE, NEWLINE);
    }
}

fprintf(fd, "       return OK;%s", NEWLINE);
fprintf(fd, "     }%s", NEWLINE);
fprintf(fd, "%s", NEWLINE);
fprintf(fd, "     default:%s", NEWLINE);
fprintf(fd, "     {%s", NEWLINE);
fprintf(fd, "       DXSetError(ERROR_BAD_CLASS, ");
fprintf(fd, "\"encountered in object traversal\");%s", NEWLINE);
fprintf(fd, "       return ERROR;%s", NEWLINE);
fprintf(fd, "     }%s", NEWLINE);
fprintf(fd, "  }%s", NEWLINE);
fprintf(fd, "}%s", NEWLINE);
fprintf(fd, "%s", NEWLINE);


fprintf(fd, "static int%s", NEWLINE);
fprintf(fd, "doLeaf(Object *in, Object *out)%s", NEWLINE);
fprintf(fd, "{%s", NEWLINE);
fprintf(fd, "  int i, result=0;%s", NEWLINE);
fprintf(fd, "  Array array;%s", NEWLINE);
fprintf(fd, "  Field field;%s", NEWLINE);
fprintf(fd, "  Pointer *in_data[%d], *out_data[%d];%s", nin, nout, NEWLINE);
fprintf(fd, "  int in_knt[%d], out_knt[%d];%s", nin, nout, NEWLINE);
fprintf(fd, "  Type type;%s", NEWLINE);
fprintf(fd, "  Category category;%s", NEWLINE);
fprintf(fd, "  int rank, shape;%s", NEWLINE);
fprintf(fd, "  Object attr, src_dependency_attr = NULL;%s", NEWLINE);
fprintf(fd, "  char *src_dependency = NULL;%s", NEWLINE);
  if (in[0]->elementtype != ELT_NOT_REQUIRED)
  {
fprintf(fd, "  Object element_type_attr;%s", NEWLINE);
fprintf(fd, "  char *element_type;%s", NEWLINE);
  }

  if (in[0]->positions == GRID_REGULAR)
  {
fprintf(fd, "  /*%s", NEWLINE);
fprintf(fd, "   * Regular positions info%s", NEWLINE);
fprintf(fd, "   */%s", NEWLINE);
fprintf(fd, "  int p_knt = -1, p_dim, *p_counts = NULL;%s", NEWLINE);
fprintf(fd, "  float *p_origin = NULL, *p_deltas = NULL;%s", NEWLINE);
  }
  else if (in[0]->positions == GRID_IRREGULAR)
  {
fprintf(fd, "  /*%s", NEWLINE);
fprintf(fd, "   * Irregular positions info%s", NEWLINE);
fprintf(fd, "   */%s", NEWLINE);
fprintf(fd, "  int p_knt, p_dim;%s", NEWLINE);
fprintf(fd, "  float *p_positions;%s", NEWLINE);
  }
  else
fprintf(fd, "  int p_knt = -1;%s", NEWLINE);

  if (in[0]->connections == GRID_REGULAR)
  {
fprintf(fd, "  /*%s", NEWLINE);
fprintf(fd, "   * Regular connections info%s", NEWLINE);
fprintf(fd, "   */%s", NEWLINE);
fprintf(fd, "  int c_knt, c_nv, c_dim, *c_counts = NULL;%s", NEWLINE);
  }
  else if (in[0]->connections == GRID_IRREGULAR)
  {
fprintf(fd, "  /*%s", NEWLINE);
fprintf(fd, "   * Irregular connections info%s", NEWLINE);
fprintf(fd, "   */%s", NEWLINE);
fprintf(fd, "  int c_knt, c_dim, c_nv;%s", NEWLINE);
fprintf(fd, "  float *c_connections;%s", NEWLINE);
  }
  else
fprintf(fd, "  int c_knt = -1;%s", NEWLINE);

fprintf(fd, "%s", NEWLINE);

if (in[0]->connections != GRID_NOT_REQUIRED ||
    in[0]->positions   != GRID_NOT_REQUIRED)
{ 
fprintf(fd, "  /*%s", NEWLINE);
fprintf(fd, "   * positions and/or connections are required, so the first must%s", NEWLINE);
fprintf(fd, "   * be a field.%s", NEWLINE);
fprintf(fd, "   */%s", NEWLINE);
fprintf(fd, "  if (DXGetObjectClass(in[0]) != CLASS_FIELD)%s", NEWLINE);
fprintf(fd, "  {%s", NEWLINE);
fprintf(fd, "      DXSetError(ERROR_DATA_INVALID,%s", NEWLINE);
fprintf(fd, "           \"positions and/or connections unavailable in array object\");%s", NEWLINE);
fprintf(fd, "      goto error;%s", NEWLINE);
fprintf(fd, "  }%s", NEWLINE);
fprintf(fd, "  else%s", NEWLINE);
}
else
fprintf(fd, "  if (DXGetObjectClass(in[0]) == CLASS_FIELD)%s", NEWLINE);

fprintf(fd, "  {%s", NEWLINE);
fprintf(fd, "%s", NEWLINE);
fprintf(fd, "    field = (Field)in[0];%s", NEWLINE);
fprintf(fd, "%s", NEWLINE);
fprintf(fd, "    if (DXEmptyField(field))%s", NEWLINE);
fprintf(fd, "      return OK;%s", NEWLINE);
fprintf(fd, "%s", NEWLINE);
fprintf(fd, "    /* %s", NEWLINE);
fprintf(fd, "     * Determine the dependency of the source object's data%s", NEWLINE);
fprintf(fd, "     * component.%s", NEWLINE);
fprintf(fd, "     */%s", NEWLINE);
fprintf(fd, "    src_dependency_attr = DXGetComponentAttribute(field, \"data\", \"dep\");%s", NEWLINE);
fprintf(fd, "    if (! src_dependency_attr)%s", NEWLINE);
fprintf(fd, "    {%s", NEWLINE);
fprintf(fd, "      DXSetError(ERROR_MISSING_DATA, \"\\\"%s\\\" data component is missing a dependency attribute\");%s", in[0]->name, NEWLINE);
fprintf(fd, "      goto error;%s", NEWLINE);
fprintf(fd, "    }%s", NEWLINE);
fprintf(fd, "%s", NEWLINE);
fprintf(fd, "    if (DXGetObjectClass(src_dependency_attr) != CLASS_STRING)%s", NEWLINE);
fprintf(fd, "    {%s", NEWLINE);
fprintf(fd, "      DXSetError(ERROR_BAD_CLASS, \"\\\"%s\\\" dependency attribute\");%s", in[0]->name, NEWLINE);
fprintf(fd, "      goto error;%s", NEWLINE);
fprintf(fd, "    }%s", NEWLINE);
fprintf(fd, "%s", NEWLINE);
fprintf(fd, "    src_dependency = DXGetString((String)src_dependency_attr);%s", NEWLINE);
fprintf(fd, "%s", NEWLINE);
fprintf(fd, "    array = (Array)DXGetComponentValue(field, \"positions\");%s", NEWLINE);
fprintf(fd, "    if (! array)%s", NEWLINE);
fprintf(fd, "    {%s", NEWLINE);
fprintf(fd, "      DXSetError(ERROR_BAD_CLASS, \"\\\"%s\\\" contains no positions component\");%s", in[0]->name, NEWLINE);
fprintf(fd, "      goto error;%s", NEWLINE);
fprintf(fd, "    }%s", NEWLINE);
fprintf(fd, "%s", NEWLINE);
    if (in[0]->positions == GRID_REGULAR)
    {
fprintf(fd, "    /* %s", NEWLINE);
fprintf(fd, "     * Input[0] should have regular positions.  First check%s", NEWLINE);
fprintf(fd, "     * that they are, and while you're at it, get the%s", NEWLINE);
fprintf(fd, "     * dimensionality so we can size arrays later.%s", NEWLINE);
fprintf(fd, "     */%s", NEWLINE);
fprintf(fd, "    if (! DXQueryGridPositions(array, &p_dim, NULL, NULL, NULL))%s", NEWLINE);
fprintf(fd, "    {%s", NEWLINE);
fprintf(fd, "      DXSetError(ERROR_BAD_CLASS, \"\\\"%s\\\" positions component is not regular\");%s", in[0]->name, NEWLINE);
fprintf(fd, "      goto error;%s", NEWLINE);
fprintf(fd, "    }%s", NEWLINE);
fprintf(fd, "%s", NEWLINE);
fprintf(fd, "    /* %s", NEWLINE);
fprintf(fd, "     * Allocate arrays for position counts, origin and deltas.%s", NEWLINE);
fprintf(fd, "     * Check that the allocations worked, then get the info.%s", NEWLINE);
fprintf(fd, "     */%s", NEWLINE);
fprintf(fd, "    p_counts = (int   *)DXAllocate(p_dim*sizeof(int));%s", NEWLINE);
fprintf(fd, "    p_origin = (float *)DXAllocate(p_dim*sizeof(float));%s", NEWLINE);
fprintf(fd, "    p_deltas = (float *)DXAllocate(p_dim*p_dim*sizeof(float));%s", NEWLINE);
fprintf(fd, "    if (! p_counts || ! p_origin || ! p_deltas)%s", NEWLINE);
fprintf(fd, "      goto error;%s", NEWLINE);
fprintf(fd, "%s", NEWLINE);
fprintf(fd, "    DXQueryGridPositions(array, NULL, p_counts, p_origin, p_deltas);%s", NEWLINE);
fprintf(fd, "    DXGetArrayInfo(array, &p_knt, NULL, NULL, NULL, NULL);%s", NEWLINE);
fprintf(fd, "%s", NEWLINE);
    }
    else if (in[0]->positions == GRID_IRREGULAR)
    {
fprintf(fd, "    /* %s", NEWLINE);
fprintf(fd, "     * The user requested irregular positions.  So we%s", NEWLINE);
fprintf(fd, "     * get the count, the dimensionality and a pointer to the%s", NEWLINE);
fprintf(fd, "     * explicitly enumerated positions.  If the positions%s", NEWLINE);
fprintf(fd, "     * are in fact regular, this will expand them.%s", NEWLINE);
fprintf(fd, "     */%s", NEWLINE);
fprintf(fd, "    DXGetArrayInfo(array, &p_knt, NULL, NULL, NULL, &p_dim);%s", NEWLINE);
fprintf(fd, "%s", NEWLINE);
fprintf(fd, "    p_positions = (float *)DXGetArrayData(array);%s", NEWLINE);
fprintf(fd, "    if (! p_positions)%s", NEWLINE);
fprintf(fd, "      goto error;%s", NEWLINE);
fprintf(fd, "%s", NEWLINE);
    }
    else
fprintf(fd, "    DXGetArrayInfo(array, &p_knt, NULL, NULL, NULL, NULL);%s", NEWLINE);

    if (in[0]->connections == GRID_REGULAR ||
        in[0]->connections == GRID_IRREGULAR ||
	in[0]->elementtype != ELT_NOT_REQUIRED)
    {
fprintf(fd, "    array = (Array)DXGetComponentValue(field, \"connections\");%s", NEWLINE);
fprintf(fd, "    if (! array)%s", NEWLINE);
fprintf(fd, "    {%s", NEWLINE);
fprintf(fd, "      DXSetError(ERROR_BAD_CLASS, \"\\\"%s\\\" contains no connections component\");%s", in[0]->name, NEWLINE);
fprintf(fd, "      goto error;%s", NEWLINE);
fprintf(fd, "    }%s", NEWLINE);
fprintf(fd, "%s", NEWLINE);

	if (in[0]->elementtype != ELT_NOT_REQUIRED)
	{
	    char *str;

	    if (! GetElementTypeString(in[0]->elementtype, &str))
		goto error;

fprintf(fd, "    /*%s", NEWLINE);
fprintf(fd, "     * Check that the field's element type matches that requested%s", NEWLINE);
fprintf(fd, "     */%s", NEWLINE);
fprintf(fd, "    element_type_attr = DXGetAttribute((Object)array, \"element type\");%s", NEWLINE);
fprintf(fd, "    if (! element_type_attr)%s", NEWLINE);
fprintf(fd, "    {%s", NEWLINE);
fprintf(fd, "        DXSetError(ERROR_DATA_INVALID,%s", NEWLINE);
fprintf(fd, "            \"input \\\"%s\\\" has no element type attribute\");%s", in[0]->name, NEWLINE);
fprintf(fd, "        goto error;%s", NEWLINE);
fprintf(fd, "    }%s%s", NEWLINE, NEWLINE);
fprintf(fd, "    if (DXGetObjectClass(element_type_attr) != CLASS_STRING)%s", NEWLINE);
fprintf(fd, "    {%s", NEWLINE);
fprintf(fd, "        DXSetError(ERROR_DATA_INVALID,%s", NEWLINE);
fprintf(fd, "            \"input \\\"%s\\\" element type attribute is not a string\");%s", in[0]->name, NEWLINE);
fprintf(fd, "        goto error;%s", NEWLINE);
fprintf(fd, "    }%s%s", NEWLINE, NEWLINE);
fprintf(fd, "    if (strcmp(DXGetString((String)element_type_attr), \"%s\"))%s", str, NEWLINE);
fprintf(fd, "    {%s", NEWLINE);
fprintf(fd, "        DXSetError(ERROR_DATA_INVALID,%s", NEWLINE);
fprintf(fd, "            \"input \\\"%s\\\" invalid element type\");%s", in[0]->name, NEWLINE);
fprintf(fd, "        goto error;%s", NEWLINE);
fprintf(fd, "    }%s%s", NEWLINE, NEWLINE);
	}

	if (in[0]->connections == GRID_REGULAR)
	{
fprintf(fd, "    /* %s", NEWLINE);
fprintf(fd, "     * Input[0] should have regular connections.  First check%s", NEWLINE);
fprintf(fd, "     * that they are, and while you're at it, get the%s", NEWLINE);
fprintf(fd, "     * dimensionality so we can size an array later.%s", NEWLINE);
fprintf(fd, "     */%s", NEWLINE);
fprintf(fd, "    if (! DXQueryGridConnections(array, &c_dim, NULL))%s", NEWLINE);
fprintf(fd, "    {%s", NEWLINE);
fprintf(fd, "      DXSetError(ERROR_BAD_CLASS, \"\\\"%s\\\" connections component is not regular\");%s", in[0]->name, NEWLINE);
fprintf(fd, "      goto error;%s", NEWLINE);
fprintf(fd, "    }%s", NEWLINE);
fprintf(fd, "%s", NEWLINE);
fprintf(fd, "    /* %s", NEWLINE);
fprintf(fd, "     * Allocate arrays for connections counts.%s", NEWLINE);
fprintf(fd, "     * Check that the allocation worked, then get the info.%s", NEWLINE);
fprintf(fd, "     */%s", NEWLINE);
fprintf(fd, "    c_counts = (int   *)DXAllocate(c_dim*sizeof(int));%s", NEWLINE);
fprintf(fd, "    if (! c_counts)%s", NEWLINE);
fprintf(fd, "      goto error;%s", NEWLINE);
fprintf(fd, "%s", NEWLINE);
fprintf(fd, "    DXQueryGridConnections(array, NULL, c_counts);%s", NEWLINE);
fprintf(fd, "    DXGetArrayInfo(array, &c_knt, NULL, NULL, NULL, &c_nv);%s", NEWLINE);
fprintf(fd, "%s", NEWLINE);
	}
	else if (in[0]->connections == GRID_IRREGULAR)
	{
fprintf(fd, "    /* %s", NEWLINE);
fprintf(fd, "     * The user requested irregular connections.  So we%s", NEWLINE);
fprintf(fd, "     * get the count, the dimensionality and a pointer to the%s", NEWLINE);
fprintf(fd, "     * explicitly enumerated elements.  If the positions%s", NEWLINE);
fprintf(fd, "     * are in fact regular, this will expand them.%s", NEWLINE);
fprintf(fd, "     */%s", NEWLINE);
fprintf(fd, "    DXGetArrayInfo(array, &c_knt, NULL, NULL, NULL, &c_nv);%s", NEWLINE);
fprintf(fd, "%s", NEWLINE);
fprintf(fd, "    c_connections = (float *)DXGetArrayData(array);%s", NEWLINE);
fprintf(fd, "    if (! c_connections)%s", NEWLINE);
fprintf(fd, "      goto error;%s", NEWLINE);
fprintf(fd, "%s", NEWLINE);
	}
	else
fprintf(fd, "    DXGetArrayInfo(array, &c_knt, NULL, NULL, NULL, NULL);%s", NEWLINE);
    }
    else
    {
fprintf(fd, "    /* %s", NEWLINE);
fprintf(fd, "     * If there are connections, get their count so that%s", NEWLINE);
fprintf(fd, "     * connections-dependent result arrays can be sized.%s", NEWLINE);
fprintf(fd, "     */%s", NEWLINE);
fprintf(fd, "    array = (Array)DXGetComponentValue(field, \"connections\");%s", NEWLINE);
fprintf(fd, "    if (array)%s", NEWLINE);
fprintf(fd, "        DXGetArrayInfo(array, &c_knt, NULL, NULL, NULL, NULL);%s", NEWLINE);
    }

fprintf(fd, "  }%s", NEWLINE);

  for (i = 0; i < nin; i++)
  {
    char *type, *rank, *shape;

    if (! GetDXDataType(in[i]->datatype, &type) ||
          ! GetDXDataShape(in[i]->datashape, &rank, &shape))
          goto error;


fprintf(fd, "  /*%s", NEWLINE);
fprintf(fd, "   * If the input argument is not NULL then we get the %s", NEWLINE);
fprintf(fd, "   * data array: either the object itself, if its an %s", NEWLINE);
fprintf(fd, "   * array, or the data component if the argument is a field%s", NEWLINE);
fprintf(fd, "   */%s", NEWLINE);
fprintf(fd, "  if (! in[%d])%s", i, NEWLINE);
fprintf(fd, "  {%s", NEWLINE);
fprintf(fd, "    array = NULL;%s", NEWLINE);
fprintf(fd, "    in_data[%d] = NULL;%s", i, NEWLINE);
fprintf(fd, "    in_knt[%d] = NULL;%s", i, NEWLINE);
fprintf(fd, "  }%s", NEWLINE);
fprintf(fd, "  else%s", NEWLINE);
fprintf(fd, "  {%s", NEWLINE);
fprintf(fd, "    if (DXGetObjectClass(in[%d]) == CLASS_ARRAY)%s", i, NEWLINE);
fprintf(fd, "    {%s", NEWLINE);
fprintf(fd, "      array = (Array)in[%d];%s", i, NEWLINE);
fprintf(fd, "    }%s", NEWLINE);
fprintf(fd, "    else if (DXGetObjectClass(in[%d]) == CLASS_STRING)%s", i, NEWLINE);
fprintf(fd, "    {%s", NEWLINE);
fprintf(fd, "      in_data[%d] = (Pointer *)DXGetString((String)in[%d]);%s",i,i, NEWLINE);
fprintf(fd, "      in_knt[%d] = 1;%s",i, NEWLINE);
fprintf(fd, "    }%s", NEWLINE);
fprintf(fd, "    else%s", NEWLINE);
fprintf(fd, "    {%s", NEWLINE);
fprintf(fd, "      if (DXGetObjectClass(in[%d]) != CLASS_FIELD)%s", i, NEWLINE);
fprintf(fd, "      {%s", NEWLINE);
fprintf(fd, "        DXSetError(ERROR_BAD_CLASS, \"\\\"%s\\\" should be a field\");%s", in[i]->name, NEWLINE);
fprintf(fd, "        goto error;%s", NEWLINE);
fprintf(fd, "      }%s", NEWLINE);
fprintf(fd, "%s", NEWLINE);
fprintf(fd, "      array = (Array)DXGetComponentValue((Field)in[%d], \"data\");%s", i, NEWLINE);
fprintf(fd, "      if (! array)%s", NEWLINE);
fprintf(fd, "      {%s", NEWLINE);
fprintf(fd, "        DXSetError(ERROR_MISSING_DATA, \"\\\"%s\\\" has no data component\");%s", in[i]->name, NEWLINE);
fprintf(fd, "        goto error;%s", NEWLINE);
fprintf(fd, "      }%s", NEWLINE);
fprintf(fd, "%s", NEWLINE);
fprintf(fd, "      if (DXGetObjectClass((Object)array) != CLASS_ARRAY)%s", NEWLINE);
fprintf(fd, "      {%s", NEWLINE);
fprintf(fd, "        DXSetError(ERROR_BAD_CLASS, \"data component of \\\"%s\\\" should be an array\");%s", in[i]->name, NEWLINE);
fprintf(fd, "        goto error;%s", NEWLINE);
fprintf(fd, "      }%s", NEWLINE);
fprintf(fd, "    }%s", NEWLINE);
fprintf(fd, "%s", NEWLINE);

      if (in[i]->dependency != NO_DEPENDENCY)
      {
fprintf(fd, "    /* %s", NEWLINE);
fprintf(fd, "     * get the dependency of the data component%s", NEWLINE);
fprintf(fd, "     */%s", NEWLINE);
fprintf(fd, "    attr = DXGetAttribute((Object)array, \"dep\");%s", NEWLINE);
fprintf(fd, "    if (! attr)%s", NEWLINE);
fprintf(fd, "    {%s", NEWLINE);
fprintf(fd, "      DXSetError(ERROR_MISSING_DATA, \"data component of \\\"%s\\\" has no dependency\");%s", in[i]->name, NEWLINE);
fprintf(fd, "      goto error;%s", NEWLINE);
fprintf(fd, "    }%s", NEWLINE);
fprintf(fd, "%s", NEWLINE);
fprintf(fd, "    if (DXGetObjectClass(attr) != CLASS_STRING)%s", NEWLINE);
fprintf(fd, "    {%s", NEWLINE);
fprintf(fd, "      DXSetError(ERROR_BAD_CLASS, \"dependency attribute of data component of \\\"%s\\\"\");%s", in[i]->name, NEWLINE);
fprintf(fd, "      goto error;%s", NEWLINE);
fprintf(fd, "    }%s", NEWLINE);
fprintf(fd, "%s", NEWLINE);
	if (in[i]->dependency == DEP_INPUT && i != 0)
	{
fprintf(fd, "  /*%s", NEWLINE);
fprintf(fd, "   * The dependency of this arg should match input[0].%s", NEWLINE);
fprintf(fd, "   */%s", NEWLINE); 
fprintf(fd, "    if (strcmp(src_dependency, DXGetString((String)attr)))%s", NEWLINE);
fprintf(fd, "    {%s", NEWLINE);
fprintf(fd, "      DXSetError(ERROR_DATA_INVALID, \"data dependency of \\\"%s\\\" must match \\\"%s\\\"\");%s",
						in[i]->name, in[0]->name, NEWLINE);
fprintf(fd, "      goto error;%s", NEWLINE);
fprintf(fd, "    }%s", NEWLINE);
	}
	else if (in[i]->dependency == DEP_POSITIONS)
	{
fprintf(fd, "  /*%s", NEWLINE);
fprintf(fd, "   * The dependency of this arg should be positions%s", NEWLINE);
fprintf(fd, "   */%s", NEWLINE); 
fprintf(fd, "    if (strcmp(\"positions\", DXGetString((String)attr)))%s", NEWLINE);
fprintf(fd, "    {%s", NEWLINE);
fprintf(fd, "      DXSetError(ERROR_DATA_INVALID, \"data dependency of \\\"%s\\\" must be positions\");%s",
						in[i]->name, NEWLINE);
fprintf(fd, "      goto error;%s", NEWLINE);
fprintf(fd, "    }%s", NEWLINE);
	}
	else if (in[i]->dependency == DEP_CONNECTIONS)
	{
fprintf(fd, "  /*%s", NEWLINE);
fprintf(fd, "   * The dependency of this arg should be connections%s", NEWLINE);
fprintf(fd, "   */%s", NEWLINE); 
fprintf(fd, "    if (strcmp(\"connections\", DXGetString((String)attr)))%s", NEWLINE);
fprintf(fd, "    {%s", NEWLINE);
fprintf(fd, "      DXSetError(ERROR_DATA_INVALID, \"data dependency of \\\"%s\\\" must be connections\");%s",
						in[i]->name, NEWLINE);
fprintf(fd, "      goto error;%s", NEWLINE);
fprintf(fd, "    }%s", NEWLINE);
	}
      }
fprintf(fd, "%s", NEWLINE);
fprintf(fd, "    if (DXGetObjectClass(in[%d]) != CLASS_STRING)", i);
fprintf(fd, "    {%s", NEWLINE);
fprintf(fd, "       DXGetArrayInfo(array, &in_knt[%d], &type, &category, &rank, &shape);%s", i, NEWLINE);
fprintf(fd, "       if (type != %s || category != CATEGORY_REAL ||%s", type, NEWLINE);

  if (! strcmp(rank, "0"))
fprintf(fd, "             !((rank == 0) || ((rank == 1)&&(shape == 1))))%s", NEWLINE);
  else
fprintf(fd, "           rank != %s || shape != %s)%s", rank, shape, NEWLINE);

fprintf(fd, "       {%s", NEWLINE);
fprintf(fd, "         DXSetError(ERROR_DATA_INVALID, \"input \\\"%s\\\"\");%s", in[i]->name, NEWLINE);
fprintf(fd, "         goto error;%s", NEWLINE);
fprintf(fd, "       }%s", NEWLINE);
fprintf(fd, "%s", NEWLINE);
fprintf(fd, "       in_data[%d] = DXGetArrayData(array);%s", i, NEWLINE);
fprintf(fd, "       if (! in_data[%d])%s", i, NEWLINE);
fprintf(fd, "          goto error;%s", NEWLINE);
fprintf(fd, "%s", NEWLINE);
fprintf(fd, "    }%s", NEWLINE);
fprintf(fd, "  }%s", NEWLINE);
    }

  for (i = 0; i < nout; i++)
  {
    char *type, *rank, *shape;

    if (! GetDXDataType(out[i]->datatype, &type) ||
	! GetDXDataShape(out[i]->datashape, &rank, &shape))
	goto error;

    if (out[i]->structure == VALUE)
    {
fprintf(fd, "  if (! out[%d])%s", i, NEWLINE);
fprintf(fd, "  {%s", NEWLINE);
fprintf(fd, "    DXSetError(ERROR_INTERNAL, \"Value output %d (\\\"%s\\\") is NULL\");%s", i, out[i]->name, NEWLINE);
fprintf(fd, "    goto error;%s", NEWLINE);
fprintf(fd, "  }%s", NEWLINE);
fprintf(fd, "  if (DXGetObjectClass(out[%d]) != CLASS_ARRAY)%s", i, NEWLINE);
fprintf(fd, "  {%s", NEWLINE);
fprintf(fd, "    DXSetError(ERROR_INTERNAL, \"Value output %d (\\\"%s\\\") is not an array\");%s", i, out[i]->name, NEWLINE);
fprintf(fd, "    goto error;%s", NEWLINE);
fprintf(fd, "  }%s", NEWLINE);
fprintf(fd, "%s", NEWLINE);
fprintf(fd, "  array = (Array)out[%d];%s", i, NEWLINE);
fprintf(fd, "%s", NEWLINE);
fprintf(fd, "  DXGetArrayInfo(array, &out_knt[%d], &type, &category, &rank, &shape);%s", i, NEWLINE);
fprintf(fd, "  if (type != %s || category != CATEGORY_REAL ||%s", type, NEWLINE);
	if (out[i]->datashape == SCALAR)
fprintf(fd, "      !((rank == 0) || ((rank == 1)&&(shape == 1))))%s", NEWLINE);
	else
fprintf(fd, "      rank != %s || shape != %s)%s", rank, shape, NEWLINE);
fprintf(fd, "  {%s", NEWLINE);
fprintf(fd, "    DXSetError(ERROR_DATA_INVALID, \"Value output \\\"%s\\\" has bad type\");%s", out[i]->name, NEWLINE);
fprintf(fd, "    goto error;%s", NEWLINE);
fprintf(fd, "  }%s", NEWLINE);
fprintf(fd, "%s", NEWLINE);
    }
    else
    {
fprintf(fd, "  /*%s", NEWLINE);
fprintf(fd, "   * Create an output data array typed according to the%s", NEWLINE);
fprintf(fd, "   * specification given%s", NEWLINE);
fprintf(fd, "   */%s", NEWLINE);
fprintf(fd, "  array = DXNewArray(%s, CATEGORY_REAL, %s, %s);%s", type, rank, shape, NEWLINE);
fprintf(fd, "  if (! array)%s", NEWLINE);
fprintf(fd, "    goto error;%s", NEWLINE);
fprintf(fd, "%s", NEWLINE);
      if (out[i]->dependency == DEP_INPUT)
      {
fprintf(fd, "  /*%s", NEWLINE);
fprintf(fd, "   * Set the dependency of the array to the same as the first input%s", NEWLINE);
fprintf(fd, "   */%s", NEWLINE);
fprintf(fd, "  if (src_dependency_attr != NULL)%s", NEWLINE);
fprintf(fd, "    if (! DXSetAttribute((Object)array, \"dep\", src_dependency_attr))%s", NEWLINE);
fprintf(fd, "      goto error;%s%s", NEWLINE, NEWLINE);
fprintf(fd, "  /*%s", NEWLINE);
fprintf(fd, "   * The size and dependency of this output data array will %s", NEWLINE);
fprintf(fd, "   * match that of input[0]%s", NEWLINE);
fprintf(fd, "   */%s", NEWLINE);
fprintf(fd, "  out_knt[%d] = in_knt[0];%s", i, NEWLINE);
      }
      else if (out[i]->dependency == DEP_POSITIONS)
      {
fprintf(fd, "  /*%s", NEWLINE);
fprintf(fd, "   * This output data array will be dep positions - and sized%s", NEWLINE);
fprintf(fd, "   * appropriately - if the appropriate size is known%s", NEWLINE);
fprintf(fd, "   */%s", NEWLINE);
fprintf(fd, "  if (p_knt == -1)%s", NEWLINE);
fprintf(fd, "  {%s", NEWLINE);
fprintf(fd, "    DXSetError(ERROR_DATA_INVALID,%s", NEWLINE);
fprintf(fd, "      \"cannot make output \\\"%s\\\" dep on positions because no positions were found in input[0]\");%s", out[i]->name, NEWLINE);
fprintf(fd, "    goto error;%s", NEWLINE);
fprintf(fd, "  }%s%s", NEWLINE, NEWLINE);
fprintf(fd, "  out_knt[%d] = p_knt;%s", i, NEWLINE);
fprintf(fd, "%s", NEWLINE);
fprintf(fd, "  if (! DXSetAttribute((Object)array, \"dep\", (Object)DXNewString(\"positions\")))%s", NEWLINE);
fprintf(fd, "    goto error;%s", NEWLINE);
      }
      else if (out[i]->dependency == DEP_CONNECTIONS)
      {
fprintf(fd, "  /*%s", NEWLINE);
fprintf(fd, "   * This output data array will be dep connections - and sized%s", NEWLINE);
fprintf(fd, "   * appropriately - if the appropriate size is known%s", NEWLINE);
fprintf(fd, "   */%s", NEWLINE);
fprintf(fd, "  if (c_knt == -1)%s", NEWLINE);
fprintf(fd, "  {%s", NEWLINE);
fprintf(fd, "    DXSetError(ERROR_DATA_INVALID,%s", NEWLINE);
fprintf(fd, "      \"cannot make output \\\"%s\\\" dep on connections because no connections were found in input \\\"%s\\\"\");%s", out[i]->name, in[i]->name, NEWLINE);
fprintf(fd, "    goto error;%s", NEWLINE);
fprintf(fd, "  }%s%s", NEWLINE, NEWLINE);
fprintf(fd, "  out_knt[%d] = c_knt;%s", i, NEWLINE);
fprintf(fd, "%s", NEWLINE);
fprintf(fd, "  if (! DXSetAttribute((Object)array, \"dep\", (Object)DXNewString(\"connections\")))%s", NEWLINE);
fprintf(fd, "    goto error;%s", NEWLINE);
      }
      else
      {
	ErrorMessage("field parameter must have a dependency set%s", NEWLINE);
	goto error;
      }

fprintf(fd, "  /*%s", NEWLINE);
fprintf(fd, "   * Actually allocate the array data space%s", NEWLINE);
fprintf(fd, "   */%s", NEWLINE);
fprintf(fd, "  if (! DXAddArrayData(array, 0, out_knt[%d], NULL))%s", i, NEWLINE);
fprintf(fd, "    goto error;%s", NEWLINE);
fprintf(fd, "%s", NEWLINE);
fprintf(fd, "  /*%s", NEWLINE);
fprintf(fd, "   * If the output vector slot is not NULL, then it better be a field, and%s", NEWLINE);
fprintf(fd, "   * we'll add the new array to it as its data component (delete any prior%s", NEWLINE);
fprintf(fd, "   * data component so that its attributes won't overwrite the new component's)%s", NEWLINE);
fprintf(fd, "   * Otherwise, place the new array into the out vector.%s", NEWLINE);
fprintf(fd, "   */%s", NEWLINE);
fprintf(fd, "  if (out[%d])%s", i, NEWLINE);
fprintf(fd, "  {%s", NEWLINE);
fprintf(fd, "    if (DXGetObjectClass(out[%d]) != CLASS_FIELD)%s", i, NEWLINE);
fprintf(fd, "    {%s", NEWLINE);
fprintf(fd, "      DXSetError(ERROR_INTERNAL, \"non-field object found in output vector\");%s", NEWLINE);
fprintf(fd, "      goto error;%s", NEWLINE);
fprintf(fd, "    }%s%s", NEWLINE, NEWLINE);
fprintf(fd, "    if (DXGetComponentValue((Field)out[%d], \"data\"))%s", i, NEWLINE);
fprintf(fd, "      DXDeleteComponent((Field)out[%d], \"data\");%s%s", i, NEWLINE, NEWLINE);
fprintf(fd, "    if (! DXSetComponentValue((Field)out[%d], \"data\", (Object)array))%s", i, NEWLINE);
fprintf(fd, "      goto error;%s", NEWLINE);
fprintf(fd, "%s", NEWLINE);
fprintf(fd, "  }%s", NEWLINE);
fprintf(fd, "  else%s", NEWLINE);
fprintf(fd, "    out[%d] = (Object)array;%s", i, NEWLINE);
    }

fprintf(fd, "  /*%s", NEWLINE);
fprintf(fd, "   * Now get the pointer to the contents of the array%s", NEWLINE);
fprintf(fd, "   */%s", NEWLINE);
fprintf(fd, "  out_data[%d] = DXGetArrayData(array);%s", i, NEWLINE);
fprintf(fd, "  if (! out_data[%d])%s", i, NEWLINE);
fprintf(fd, "    goto error;%s", NEWLINE);
fprintf(fd, "%s", NEWLINE);
  }
fprintf(fd, "  /*%s", NEWLINE);
fprintf(fd, "   * Call the user's routine.  Check the return code.%s", NEWLINE);
fprintf(fd, "   */%s", NEWLINE);
fprintf(fd, "  result = %s_worker(%s", mod->name, NEWLINE);

    open = 0;

    if (in[0]->positions == GRID_REGULAR)
    {
fprintf(fd, "    p_knt, p_dim, p_counts, p_origin, p_deltas");
	open = 1;
    }
    else if (in[0]->positions == GRID_IRREGULAR)
    {
fprintf(fd, "    p_knt, p_dim, (float *)p_positions");
	open = 1;
    }

    if (in[0]->connections == GRID_REGULAR)
    {
fprintf(fd, "%s    c_knt, c_nv, c_counts", (open)?COMMA_AND_NEWLINE:"");
	open = 1;
    }
    else if (in[0]->connections == GRID_IRREGULAR)
    {
fprintf(fd, "%s    c_knt, c_nv, (int *)c_connections", (open)?COMMA_AND_NEWLINE:"");
	open = 1;
    }

    for (i = 0; i < nin; i++)
    { 
	char *type;
	GetCDataType(in[i]->datatype, &type);
fprintf(fd, "%s    in_knt[%d], (%s *)in_data[%d]", (open)?COMMA_AND_NEWLINE:"", i, type, i);
	open = 1;
    }

    for (i = 0; i < nout; i++)
    { 
	char *type;
	GetCDataType(out[i]->datatype, &type);
fprintf(fd, "%s    out_knt[%d], (%s *)out_data[%d]", (open)?COMMA_AND_NEWLINE:"", i, type, i);
	open = 1;
    }

fprintf(fd, ");%s%s", NEWLINE, NEWLINE);
fprintf(fd, "  if (! result)%s", NEWLINE);
fprintf(fd, "     if (DXGetError()==ERROR_NONE)%s", NEWLINE);
fprintf(fd, "        DXSetError(ERROR_INTERNAL, \"error return from user routine\");%s", NEWLINE);
fprintf(fd, "%s", NEWLINE);
fprintf(fd, "  /*%s", NEWLINE);
fprintf(fd, "   * In either event, clean up allocated memory%s", NEWLINE);
fprintf(fd, "   */%s", NEWLINE);
fprintf(fd, "%s", NEWLINE);
fprintf(fd, "error:%s", NEWLINE);
if (in[0]->positions == GRID_REGULAR)
{
fprintf(fd, "  /*%s", NEWLINE);
fprintf(fd, "   * Free the arrays allocated for the regular positions%s", NEWLINE);
fprintf(fd, "   * counts, origin and deltas%s", NEWLINE);
fprintf(fd, "   */%s", NEWLINE);
fprintf(fd, "  DXFree((Pointer)p_counts);%s", NEWLINE);
fprintf(fd, "  DXFree((Pointer)p_origin);%s", NEWLINE);
fprintf(fd, "  DXFree((Pointer)p_deltas);%s", NEWLINE);
}
if (in[0]->connections == GRID_REGULAR)
{
fprintf(fd, "  /*%s", NEWLINE);
fprintf(fd, "   * Free the arrays allocated for the regular connections%s", NEWLINE);
fprintf(fd, "   * counts%s", NEWLINE);
fprintf(fd, "   */%s", NEWLINE);
fprintf(fd, "  DXFree((Pointer)c_counts);%s", NEWLINE);
}
fprintf(fd, "  return result;%s", NEWLINE);
fprintf(fd, "}%s%s", NEWLINE, NEWLINE);

fprintf(fd, "int%s%s_worker(\n", NEWLINE, mod->name);
    
    open = 0;

    if (in[0]->positions == GRID_REGULAR)
    {
fprintf(fd, "    int p_knt, int p_dim, int *p_counts, float *p_origin, float *p_deltas");
	open = 1;
    }
    else if (in[0]->positions == GRID_IRREGULAR)
    {
fprintf(fd, "    int p_knt, int p_dim, float *p_positions");
	open = 1;
    }

    if (in[0]->connections == GRID_REGULAR)
    {
fprintf(fd, "%s    int c_knt, int c_nv, int *c_counts", (open)?COMMA_AND_NEWLINE:"");
	open = 1;
    }
    else if (in[0]->connections == GRID_IRREGULAR)
    {
fprintf(fd, "%s    int c_knt, int c_nv, int *c_connections", (open)?COMMA_AND_NEWLINE:"");
	open = 1;
    }

    for (i = 0; i < nin; i++)
    { 
	char *type;
	GetCDataType(in[i]->datatype, &type);
fprintf(fd, "%s    int %s_knt, %s *%s_data", (open)?COMMA_AND_NEWLINE:"", in[i]->name, type, in[i]->name);
	open = 1;
    }

    for (i = 0; i < nout; i++)
    { 
	char *type;
	GetCDataType(out[i]->datatype, &type);
fprintf(fd, "%s    int %s_knt, %s *%s_data", (open)?COMMA_AND_NEWLINE:"", out[i]->name, type, out[i]->name);
	open = 1;
    }

fprintf(fd, ")%s{%s", NEWLINE, NEWLINE);
fprintf(fd, "  /*%s", NEWLINE);
fprintf(fd, "   * The arguments to this routine are:%s", NEWLINE);
fprintf(fd, "   *%s", NEWLINE);

    if (in[0]->positions == GRID_REGULAR)
    {
fprintf(fd, "   *  p_knt:           total count of input positions%s", NEWLINE);
fprintf(fd, "   *  p_dim:           dimensionality of input positions%s", NEWLINE);
fprintf(fd, "   *  p_counts:        count along each axis of regular positions grid%s", NEWLINE);
fprintf(fd, "   *  p_origin:        origin of regular positions grid%s", NEWLINE);
fprintf(fd, "   *  p_deltas:        regular positions delta vectors%s", NEWLINE);
    }
    else if (in[0]->positions == GRID_IRREGULAR)
    {
fprintf(fd, "   *  p_knt:           total count of input positions%s", NEWLINE);
fprintf(fd, "   *  p_dim:           dimensionality of input positions%s", NEWLINE);
fprintf(fd, "   *  p_positions:     pointer to positions list%s", NEWLINE);
    }

    if (in[0]->connections == GRID_REGULAR)
    {
fprintf(fd, "   *  c_knt:           total count of input connections elements%s", NEWLINE);
fprintf(fd, "   *  c_nv:            number of vertices per element%s", NEWLINE);
fprintf(fd, "   *  c_counts:        vertex count along each axis of regular positions grid%s", NEWLINE);
    }
    else if (in[0]->connections == GRID_IRREGULAR)
    {
fprintf(fd, "   *  c_knt:           total count of input connections elements%s", NEWLINE);
fprintf(fd, "   *  c_nv:            number of vertices per element%s", NEWLINE);
fprintf(fd, "   *  c_connections:   pointer to connections list%s", NEWLINE);
    }

fprintf(fd, "   *%s", NEWLINE);
fprintf(fd, "   * The following are inputs and therefore are read-only.  The default%s", NEWLINE);
fprintf(fd, "   * values are given and should be used if the knt is 0.%s", NEWLINE);
fprintf(fd, "   *%s", NEWLINE);
    for (i = 0; i < nin; i++)
    {
fprintf(fd, "   * %s_knt, %s_data:  count and pointer for input \"%s\"%s", in[i]->name, in[i]->name, in[i]->name, NEWLINE);
	if (in[i]->default_value)
	    if (in[i]->descriptive == TRUE)
fprintf(fd, "   *                   descriptive default value is \"%s\"%s", in[i]->default_value, NEWLINE);
	    else
fprintf(fd, "   *                   non-descriptive default value is \"%s\"%s", in[i]->default_value, NEWLINE);
        else
fprintf(fd, "   *                   no default value given.%s", NEWLINE);
    }

fprintf(fd, "   *%s", NEWLINE);
fprintf(fd, "   *  The output data buffer(s) are writable.%s", NEWLINE);
fprintf(fd, "   *  The output buffer(s) are preallocated based on%s", NEWLINE);
fprintf(fd, "   *     the dependency (positions or connections),%s", NEWLINE);
fprintf(fd, "   *     the size of the corresponding positions or%s", NEWLINE);
fprintf(fd, "   *     connections component in the first input, and%s", NEWLINE);
fprintf(fd, "   *     the data type.%s", NEWLINE);
fprintf(fd, "   *%s", NEWLINE);
    for (i = 0; i < nout; i++)
    {
fprintf(fd, "   * %s_knt, %s_data:  count and pointer for output \"%s\"%s",
				out[i]->name, out[i]->name, out[i]->name, NEWLINE);
    }
fprintf(fd, "   */%s%s", NEWLINE, NEWLINE);
fprintf(fd, "  /*%s", NEWLINE);
fprintf(fd, "   * User's code goes here%s", NEWLINE);
fprintf(fd, "   */%s", NEWLINE);
fprintf(fd, "     %s", NEWLINE);
fprintf(fd, "     %s", NEWLINE);
/* was an include file specified? */
if(mod->include_file != NULL)
	fprintf(fd, "#include \"%s\" %s%s", mod->include_file, NEWLINE, NEWLINE);
fprintf(fd, "  /*%s", NEWLINE);
fprintf(fd, "   * successful completion%s", NEWLINE);
fprintf(fd, "   */%s", NEWLINE);
fprintf(fd, "   return 1;%s", NEWLINE);
fprintf(fd, "     %s", NEWLINE);
fprintf(fd, "  /*%s", NEWLINE);
fprintf(fd, "   * unsuccessful completion%s", NEWLINE);
fprintf(fd, "   */%s", NEWLINE);
fprintf(fd, "error:%s", NEWLINE);
fprintf(fd, "   return 0;%s", NEWLINE);
fprintf(fd, "  %s}%s", NEWLINE, NEWLINE);

    fclose(fd);
    free(buf);

    return 1;

error:
    if (fd)
    {
	fclose(fd);
	unlink(buf);
    }
    free(buf);

    return 0;
}


#define COPYSTRING(dst, src)					\
{								\
    (dst) = (char *)malloc(strlen((src))+1);			\
    if ((dst) == NULL) {					\
	ErrorMessage("allocation error");			\
	goto error;						\
    }								\
								\
    strcpy((dst), (src));					\
}

#define TRUEFALSE(dst, src)					\
{								\
    if (! strcmp((src), "TRUE"))				\
	(dst) = TRUE;						\
    else if (! strcmp((src), "FALSE"))				\
	(dst) = FALSE;						\
    else {							\
	ErrorMessage("invalid true/false: %s\n", src);	\
	goto error;						\
    }								\
}

static int
copy_comments(char *basename, FILE *out, char *hdr, char *ldr, char *trlr)
{
    FILE *in = NULL;
    int  first_comment = 1, comment_found = 0;
    int  first_char = 1, is_comment;
    int  i, c;
    char *buf = (char *)malloc(strlen(basename) + 4);

    if (! buf)
    {
	ErrorMessage("allocation error");
	goto error;
    }

    sprintf(buf, "%s.mb", basename);

    in = fopen(buf, "r");
    free(buf);
    buf = 0;
    if (! in)
    {
	ErrorMessage("error opening mb file");
	return 0;
    }

    while (! feof(in))
    {
	c = getc(in);

	if (first_char)
	    is_comment = (c == '#');
	
	if (is_comment)
	{
	    comment_found = 1;

	    if (first_comment && hdr)
	    {
		for(i = 0; hdr[i] != '\0'; i++)
		    if (! putc(hdr[i], out))
			return 0;
		first_comment = 0;
	    }

	    if (first_char && ldr)
	    {
		for(i = 0; ldr[i] != '\0'; i++)
		    if (! putc(ldr[i], out))
			return 0;
	    }
	    else
		if (! putc(c, out))
		    return 0;
	}
	
	if (c == '\n')
	    first_char = 1;
	else
	    first_char = 0;
    }

    if (comment_found && trlr)
    {
	for(i = 0; trlr[i] != '\0'; i++)
	    if (! putc(trlr[i], out))
		return 0;
	first_comment = 0;
    }

    fclose(in);
    return 1;

error:
    if (in)
        fclose(in);
    return 0;
}

#define MAXLINE	    16384
char linebuf[MAXLINE];

#define WHITESPACE(l) ((l) == ' ' || (l) == '\t')

char *
_dxf_getline(FILE *fd)
{
    int i;
    int c;

    if (feof(fd))
	return NULL;
    
    do
    {
	c = getc(fd);
    } while (c != EOF && WHITESPACE(c));

    if (c == EOF)
	return NULL;
    
    for (i = 0; i < MAXLINE; i++, c = getc(fd))
    {
	linebuf[i] = c;

	if (c == EOF || c == '\n')
	{
	    while (i > 0 && WHITESPACE(linebuf[i-1]))
	      i--;
		
	    linebuf[i] = '\0';
	    break;
	}
    
    }

    if (i == MAXLINE)
    {
	ErrorMessage("line exceeds max length");
	return NULL;
    }

    return linebuf;
}

static void
parseline(char *line, char **name, char **value)
{
    *name = line;
    
    while (!WHITESPACE(*line) && *line != '=')
	line ++;
    
    *line = '\0';
    line++;

    while (WHITESPACE(*line) || *line == '=')
	line ++;
    
    *value = line;
}
    

#define ISTOKEN(a,b) (!strcmp(a, b))

static int
process_input(char *basename, Module *mod, Parameter **in, Parameter **out)
{
    FILE      *fd = NULL;
    int       nin = 0, nout = 0;
    int       linenum;
    char      *line, *name, *value;
    Parameter *parameter = NULL;
    char      *buf = (char *)malloc(strlen(basename) + 4);

    if (! buf)
    {
	ErrorMessage("allocation error");
	goto error;
    }

    sprintf(buf, "%s.mb", basename);

    mod->name                = NULL;
    mod->category            = NULL;
    mod->description         = NULL;
    mod->outboard_executable = NULL;
    mod->include_file        = NULL;
    mod->outboard_persistent = UNKNOWN;
    mod->asynchronous        = UNKNOWN;
    mod->pinned              = UNKNOWN;
    mod->side_effect         = UNKNOWN;

    in[0] = NULL; out[0] = NULL;

    fd = fopen(buf, "r");
    free(buf);
    buf = 0;
    if (! fd)
    {
	ErrorMessage("error opening file: %s\n", basename);
	goto error;
    }

    linenum = -1;
    while (NULL != (line = _dxf_getline(fd)))
    {
	linenum ++;

	if (line[0] == '\0')
	    continue;
	
	if (line[0] == '#')
	    continue;
	
	/*  FIXME  */
	/*if (line[-1] == -1)
	  goto error;*/

	parseline(line, &name, &value);

	if (ISTOKEN(name, "MODULE_NAME")) {
	    COPYSTRING(mod->name, value);
	}
	else if (ISTOKEN(name, "CATEGORY")) {
	    COPYSTRING(mod->category, value);
	}
	else if (ISTOKEN(name, "MODULE_DESCRIPTION")) {
	    COPYSTRING(mod->description, value);
	}
	else if (ISTOKEN(name, "OUTBOARD_HOST")) {
	    COPYSTRING(mod->outboard_host, value);
	}
	else if (ISTOKEN(name, "OUTBOARD_EXECUTABLE")) {
	    COPYSTRING(mod->outboard_executable, value);
	}
	else if (ISTOKEN(name, "LOADABLE_EXECUTABLE")) {
	    COPYSTRING(mod->loadable_executable, value);
	}
	else if (ISTOKEN(name, "INCLUDE_FILE")) {
	    COPYSTRING(mod->include_file, value);
	}
	else if (ISTOKEN(name, "OUTBOARD_PERSISTENT")) {
	    TRUEFALSE(mod->outboard_persistent, value);
	}
	else if (ISTOKEN(name, "ASYNCHRONOUS")) {
	    TRUEFALSE(mod->asynchronous, value);
	}
	else if (ISTOKEN(name, "PINNED")) {
	    TRUEFALSE(mod->pinned, value);
	}
	else if (ISTOKEN(name, "SIDE_EFFECT")) {
	    TRUEFALSE(mod->side_effect, value);
	}
	else if (ISTOKEN(name, "LANGUAGE")) {
	    if (ISTOKEN(value, "C"))
		mod->language = C;
	    else if (ISTOKEN(value, "Fortran"))
		mod->language = FORTRAN;
	    else
	    {
		ErrorMessage("unrecognized LANGUAGE: %s\n", value);
		return 0;
	    }
	}

	else if (ISTOKEN(name, "INPUT"))
	{
	    in[nin] = (Parameter *)malloc(sizeof(Parameter));
	    if (in[nin] == NULL)
	    {
		ErrorMessage("allocation error");
		goto error;
	    }

	    parameter = in[nin];
	    nin ++;

	    COPYSTRING(parameter->name, value);

	    parameter->description   = NULL;
	    parameter->required      = UNKNOWN;
	    parameter->default_value = NULL;
	    parameter->types         = NULL;
	    parameter->structure     = NO_STRUCTURE;
	    parameter->datatype      = NO_DATATYPE;
	    parameter->datashape     = NO_DATASHAPE;
	    parameter->counts        = NO_COUNTS;
	    parameter->positions     = GRID_NOT_REQUIRED;
	    parameter->connections   = GRID_NOT_REQUIRED;
	    parameter->elementtype   = ELT_NOT_REQUIRED;
	    parameter->dependency    = NO_DEPENDENCY;
	}
	else if (ISTOKEN(name, "OUTPUT"))
	{
	    out[nout] = (Parameter *)malloc(sizeof(Parameter));
	    if (out[nout] == NULL)
	    {
		ErrorMessage("allocation error");
		goto error;
	    }

	    parameter = out[nout];
	    nout ++;

	    COPYSTRING(parameter->name, value);

	    parameter->description   = NULL;
	    parameter->required      = UNKNOWN;
	    parameter->default_value = NULL;
	    parameter->types         = NULL;
	    parameter->structure     = NO_STRUCTURE;
	    parameter->datatype      = NO_DATATYPE;
	    parameter->datashape     = NO_DATASHAPE;
	    parameter->counts        = NO_COUNTS;
	    parameter->positions     = GRID_NOT_REQUIRED;
	    parameter->connections   = GRID_NOT_REQUIRED;
	    parameter->elementtype   = ELT_NOT_REQUIRED;
	    parameter->dependency    = NO_DEPENDENCY;
	}
	else if (ISTOKEN(name, "DESCRIPTION"))
	{
	    if (! parameter) 
	    {
		ErrorMessage("syntax error line %d\n DESCRIPTION encountered outside of parameter definition", linenum); 
		goto error;
	    }
	    COPYSTRING(parameter->description, value);
	}
	else if (ISTOKEN(name, "DESCRIPTIVE"))
	{
	    if (! parameter) 
	    {
		ErrorMessage("syntax error line %d\n DESCRIPTIVE encountered outsideof parameter definition", linenum);
		goto error;
	    }
	    TRUEFALSE(parameter->descriptive, value);
	}
	else if (ISTOKEN(name, "REQUIRED"))
	{
	    if (! parameter) 
	    {
		ErrorMessage("syntax error line %d\n REQUIRED encountered outsideof parameter definition", linenum);
		goto error;
	    }
	    TRUEFALSE(parameter->required, value);
	}
	else if (ISTOKEN(name, "DEFAULT_VALUE"))
	{
	    if (! parameter) 
	    {
		ErrorMessage("syntax error line %d\nDEFAULT_VALUE encountered outside of parameter definition", linenum);
		goto error;
	    }
	    COPYSTRING(parameter->default_value, value);
	}
	else if (ISTOKEN(name, "TYPES"))
	{
	    if (! parameter) 
	    {
		ErrorMessage("syntax error line %d\nTYPES encountered outsideof parameter definition", linenum);
		goto error;
	    }
	    COPYSTRING(parameter->types, value);
	}
	else if (ISTOKEN(name, "STRUCTURE"))
	{
	    if (! parameter) 
	    {
		ErrorMessage("syntax error line %d\nSTRUCTURE encountered outside of parameter definition", linenum);
		goto error;
	    }

	    if (ISTOKEN(value, "Value"))
		parameter->structure = VALUE;
	    else if (ISTOKEN(value, "Field/Group"))
		parameter->structure = GROUP_FIELD;
	    else
	    {   
		ErrorMessage("unknown STRUCTURE: %s\n", value);
		goto error;
	    }
	}
	else if (ISTOKEN(name, "DATA_TYPE"))
	{
	    if (! parameter) 
	    {
		ErrorMessage("syntax error line %d\nDATA_TYPE encountered outside of parameter definition", linenum);
		goto error;
	    }

	    if (ISTOKEN(value, "float"))
		parameter->datatype = FLOAT;
	    else if (ISTOKEN(value, "double"))
		parameter->datatype = DOUBLE;
	    else if (ISTOKEN(value, "int"))
		parameter->datatype = INT;
	    else if (ISTOKEN(value, "uint"))
		parameter->datatype = UINT;
	    else if (ISTOKEN(value, "short"))
		parameter->datatype = SHORT;
	    else if (ISTOKEN(value, "ushort"))
		parameter->datatype = USHORT;
	    else if (ISTOKEN(value, "byte"))
		parameter->datatype = BYTE;
	    else if (ISTOKEN(value, "ubyte"))
		parameter->datatype = UBYTE;
	    else if (ISTOKEN(value, "string"))
		parameter->datatype = STRING;
	    else
	    {   
		ErrorMessage("unknown DATATYPE: %s\n", value);
		goto error;
	    }
	}
	else if (ISTOKEN(name, "DATA_SHAPE"))
	{
	    if (! parameter) 
	    {
		ErrorMessage("syntax error line %dDATA_SHAPE encountered outside of parameter definition\n", linenum);
		goto error;
	    }

	    if (ISTOKEN(value, "Scalar"))
		parameter->datashape = SCALAR;
	    else if (ISTOKEN(value, "1-vector"))
		parameter->datashape = VECTOR_1;
	    else if (ISTOKEN(value, "2-vector"))
		parameter->datashape = VECTOR_2;
	    else if (ISTOKEN(value, "3-vector"))
		parameter->datashape = VECTOR_3;
	    else if (ISTOKEN(value, "4-vector"))
		parameter->datashape = VECTOR_4;
	    else if (ISTOKEN(value, "5-vector"))
		parameter->datashape = VECTOR_5;
	    else if (ISTOKEN(value, "6-vector"))
		parameter->datashape = VECTOR_6;
	    else if (ISTOKEN(value, "7-vector"))
		parameter->datashape = VECTOR_7;
	    else if (ISTOKEN(value, "8-vector"))
		parameter->datashape = VECTOR_8;
	    else if (ISTOKEN(value, "9-vector"))
		parameter->datashape = VECTOR_9;
	    else
	    {   
		ErrorMessage("unknown DATASHAPE: %s\n", value);
		goto error;
	    }
	}
	else if (ISTOKEN(name, "ELEMENT_TYPE"))
	{
	    if (! parameter) 
	    {
		ErrorMessage("syntax error line %d\nELEMENT_TYPE encountered outside of parameter definition", linenum);
		goto error;
	    }

	    if (ISTOKEN(value, "Lines"))
		parameter->elementtype = ELT_LINES;
	    else if (ISTOKEN(value, "Triangles"))
		parameter->elementtype = ELT_TRIANGLES;
	    else if (ISTOKEN(value, "Quads"))
		parameter->elementtype = ELT_QUADS;
	    else if (ISTOKEN(value, "Tetrahedra"))
		parameter->elementtype = ELT_TETRAHEDRA;
	    else if (ISTOKEN(value, "Cubes"))
		parameter->elementtype = ELT_CUBES;
	    else if (ISTOKEN(value, "Not required"))
		parameter->elementtype = ELT_NOT_REQUIRED;
	    else
	    {   
		ErrorMessage("unknown ELEMENT_TYPE: %s\n", value);
		goto error;
	    }
	}
	else if (ISTOKEN(name, "COUNTS"))
	{
	    if (! parameter) 
	    {
		ErrorMessage("syntax error line %d\nCOUNTS encountered outside of parameter definition", linenum);
		goto error;
	    }

	    if (ISTOKEN(value, "1"))
		parameter->counts = COUNTS_1;
	    else if (ISTOKEN(value, "same as input"))
		parameter->counts = COUNTS_SAME_AS_INPUT;
	    else
	    {   
		ErrorMessage("unknown COUNTS: %s\n", value);
		goto error;
	    }
	}
	else if (ISTOKEN(name, "POSITIONS"))
	{
	    if (! parameter) 
	    {
		ErrorMessage("syntax error line %d\nPOSITIONS encountered outside of parameter definition", linenum);
		goto error;
	    }

	    if (ISTOKEN(value, "Regular"))
		parameter->positions = GRID_REGULAR;
	    else if (ISTOKEN(value, "Irregular"))
		parameter->positions = GRID_IRREGULAR;
	    else if (ISTOKEN(value, "Not required"))
		parameter->positions = GRID_NOT_REQUIRED;
	    else
	    {   
		ErrorMessage("unknown POSITIONS: %s\n", value);
		goto error;
	    }
	}
	else if (ISTOKEN(name, "CONNECTIONS"))
	{
	    if (! parameter) 
	    {
		ErrorMessage("syntax error line %d\nCONNECTIONS encountered outside of parameter definition", linenum);
		goto error;
	    }

	    if (ISTOKEN(value, "Regular"))
		parameter->connections = GRID_REGULAR;
	    else if (ISTOKEN(value, "Irregular"))
		parameter->connections = GRID_IRREGULAR;
	    else if (ISTOKEN(value, "Not required"))
		parameter->connections = GRID_NOT_REQUIRED;
	    else
	    {   
		ErrorMessage("unknown CONNECTIONS: %s\n", value);
		goto error;
	    }
	}
	else if (ISTOKEN(name, "DEPENDENCY"))
	{
	    if (! parameter) 
	    {
		ErrorMessage("syntax error line %d\nDEPENDENCY encountered outside of parameter definition", linenum);
		goto error;
	    }

	    if (ISTOKEN(value, "No dependency"))
		parameter->dependency = NO_DEPENDENCY;
	    else if (ISTOKEN(value, "Positions only"))
		parameter->dependency = DEP_POSITIONS;
	    else if (ISTOKEN(value, "Connections only"))
		parameter->dependency = DEP_CONNECTIONS;
	    else if (ISTOKEN(value, "Positions or connections"))
		parameter->dependency = DEP_INPUT;
	    else
	    {   
		ErrorMessage("unknown DEPENDENCY: %s\n", value);
		goto error;
	    }
	}
	else
	{
	    ErrorMessage("unrecognized keyword on line %d: %s\n",
			linenum, name);
	    goto error;
	}
    }

    in[nin]   = NULL;
    out[nout] = NULL;

    fclose(fd);

    return 1;

error:
    if (fd)
	fclose(fd);
    return 0;
}


static int
GetDXDataType(enum datatype t, char **type)
{
    switch(t)
    {
	case DOUBLE: *type = "TYPE_DOUBLE"; break;
	case FLOAT:  *type = "TYPE_FLOAT";  break;
	case INT:    *type = "TYPE_INT";    break;
	case UINT:   *type = "TYPE_UINT";   break;
	case SHORT:  *type = "TYPE_SHORT";  break;
	case USHORT: *type = "TYPE_USHORT"; break;
	case BYTE:   *type = "TYPE_BYTE";   break;
	case UBYTE:  *type = "TYPE_UBYTE";  break;
	case STRING: *type = "TYPE_STRING";  break;
	default:
	    ErrorMessage("cannot create array without DATATYPE");
	    return 0;
    }

    return 1;
}


static int
GetCDataType(enum datatype t, char **type)
{
    switch(t)
    {
	case DOUBLE: *type = "double"; 		break;
	case FLOAT:  *type = "float";  		break;
	case INT:    *type = "int";    		break;
	case UINT:   *type = "unsigned int";    break;
	case SHORT:  *type = "short"; 		break;
	case USHORT: *type = "unsigned short";	break;
	case BYTE:   *type = "byte";  		break;
	case UBYTE:  *type = "ubyte";  		break;
	case STRING: *type = "char"; 		break;
	default:
	    ErrorMessage("cannot create array without DATATYPE");
	    return 0;
    }

    return 1;
}

static int
GetDXDataShape(enum datashape t, char **r, char **s)
{
    switch(t)
    {  
	case SCALAR:   *r = "0"; *s = "0"; break;
	case VECTOR_1: *r = "1"; *s = "1"; break;
	case VECTOR_2: *r = "1"; *s = "2"; break;
	case VECTOR_3: *r = "1"; *s = "3"; break;
	case VECTOR_4: *r = "1"; *s = "4"; break;
	case VECTOR_5: *r = "1"; *s = "5"; break;
	case VECTOR_6: *r = "1"; *s = "6"; break;
	case VECTOR_7: *r = "1"; *s = "7"; break;
	case VECTOR_8: *r = "1"; *s = "8"; break;
	case VECTOR_9: *r = "1"; *s = "9"; break;
	default:
	    ErrorMessage("cannot create array without DATASHAPE");
	    return 0;
    }

    return 1;
}


static int
GetElementTypeString(enum elementtype t, char **r)
{
    switch(t)
    {  
	case ELT_LINES:      *r = "lines";      break;
	case ELT_TRIANGLES:  *r = "triangles";  break;
	case ELT_QUADS:      *r = "quads";      break;
	case ELT_TETRAHEDRA: *r = "tetrahedra"; break;
	case ELT_CUBES:      *r = "cubes";      break;
	default:
	    ErrorMessage("no element type string available");
	    return 0;
    }

    return 1;
}

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

