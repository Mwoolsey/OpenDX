/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


#include <ctype.h>
#include <stdarg.h>
#include <string.h>
#include <dx/dx.h>
#include "function.h"
#include "config.h"
#include "pmodflags.h"
#include "d.h"
#include "_macro.h"
#include "log.h"
#include "parse.h"
#include "context.h"
#include "attribute.h"

typedef	char	EXC_4[4];

extern int DXGetErrorExit(); /* from libdx/message.c */
static node *ExExtractAttrs(char *attrstr);
static node *ExCreateAttrNode(char *attrstr, int attrval);

Error _dxf_ExFunctionDone ()
{
#if 0
    char	*key;
    int		n;
    node	**nodes	= NULL;
    EXC_4	*strs	= NULL;
    int		i;
#endif

    return (OK);
#if 0
    /*
     * Count up the number of defined modules.
     */

    _dxf_ExDictionaryBeginIterate (_dxd_exMacroDict);
    for (n = 0; _dxf_ExDictionaryIterate (_dxd_exMacroDict, &key) != NULL; n++)
	;
    _dxf_ExDictionaryEndIterate (_dxd_exMacroDict);

    /*
     * Get all of the modules.
     */

    nodes = (node **) DXAllocate (n * sizeof (node *));
    strs  = (EXC_4 *) DXAllocate (n * sizeof (EXC_4));
    if (nodes == NULL || strs == NULL)
	goto cleanup;
    ExZero (strs, n * sizeof (EXC_4));

    _dxf_ExDictionaryBeginIterate (_dxd_exMacroDict);
    for (i = 0; i < n; i++)
	nodes[i] = (node *) _dxf_ExDictionaryIterate (_dxd_exMacroDict, &key);
    _dxf_ExDictionaryEndIterate (_dxd_exMacroDict);

    /*
     * $$$$$ When we have more time we will generate the 3 letter tags
     * $$$$$ for the modules here.  Using the method ifdefed out below.
     */

cleanup:
    DXFree ((Pointer) nodes);
    DXFree ((Pointer) strs);
    return (OK);
#endif
}


#if 0
Error XXXExFunctionDone ()
{
    int			i, j, k, l;
    char		led[4];
    char		*name;
    int			last;

    /*
     * For each entry we create a label by first taking all of its upper
     * case letters and then filling in with the letters following the
     * last one.  We then check to see whether we have a collision or 
     * not.  If we do then we keep advancing the last letter untill either
     * there are none left or we no longer collide.
     */

    for (k = 0; k < uled; k++)
    {
	name = leds[k].name;
	led[0] = led[1] = led[2] = led[3] = '\000';

	for (i = 0, j = 0, last = 0; j < 3 && name[i] != '\000'; i++)
	{
	    if (isupper (name[i]))
	    {
		last = i;
		led[j++] = name[i];
	    }
	}

retry:
	for (i = last + 1; j < 3 && name[i] != '\000'; i++)
	{
	    last = i;
	    led[j++] = name[i];
	}

	for (l = 0; name[last] && l < k; l++)
	{
	    if (! strcmp (led, leds[l].led))
	    {
		led[--j] = '\000';
		goto retry;
	    }
	}
    }
}
#endif


int DXAddMacro (node *macro)
{
    node	*par;
    int		nin;
    int		nout;
    int		ret;

    if (macro == NULL)
    {
	DXWarning ("#4640");
	return (-1);
    }

    /* Count the number of input and output parameters */
    for (nin  = 0, par = macro->v.macro.in;  par; par = par->next, nin++)  ;
    for (nout = 0, par = macro->v.macro.out; par; par = par->next, nout++) ;
    macro->v.macro.nin  = nin;
    macro->v.macro.nout = nout;

    ret = _dxf_ExMacroInsert (macro->v.macro.id->v.id.id,
		        (EXObj) macro);

    return (ret);
}

/*
 * the non-varargs version of DXAddModule.
 *
 */


Error DXAddModuleV (char *name, PFI func, int flags, int nin, char *inlist[],
		    int nout, char *outlist[], char *exec, char *host)
{
    char		*par;
    int			i;
    node		*module;
    node		*id;
    node		*list;
    int			ret;
    char		*sname;
    char		*attrstart;
    int			argcnt;

    sname  = _dxf_ExCopyString (name);
    module = _dxf_ExPCreateNode (NT_MODULE);

    module->v.module.id      = _dxf_ExPCreateId (sname);
    module->v.module.def.func = func;
    module->v.module.flags   = flags;
    module->v.module.varargs = FALSE;

    /* 
     * handle inputs.
     *  new code added to optionally prepend private inputs,
     *  then the existing code which adds one input per input parm
     *  in the addmodule call, 
     *  then more new code to optionally append more private inputs.
     */

    argcnt = 0;
    list = NULL;

    /* 
     * if the module is outboard, you need five extra hidden inputs
     * to the module: the executable file, the host machine name, the
     * module flags, and the number of inputs and outputs to the 
     * outboard module.
     */
    if (flags & MODULE_OUTBOARD) 
    {
	id  = _dxf_ExPCreateId (_dxf_ExCopyString ("_outboard_name"));
	LIST_APPEND (list, id);
	argcnt++;

	id  = _dxf_ExPCreateId (_dxf_ExCopyString ("_outboard_host"));
	LIST_APPEND (list, id);
	argcnt++;

	id  = _dxf_ExPCreateId (_dxf_ExCopyString ("_outboard_flags"));
	LIST_APPEND (list, id);
	argcnt++;

	id  = _dxf_ExPCreateId (_dxf_ExCopyString ("_outboard_inputs"));
	LIST_APPEND (list, id);
	argcnt++;

	id  = _dxf_ExPCreateId (_dxf_ExCopyString ("_outboard_outputs"));
	LIST_APPEND (list, id);
	argcnt++;

	module->v.module.prehidden = 5;
    }

    /* Get the input names */
    for (i = 0; i < nin; i++)
    {
	/*
	 * Only allow the ... construct to be the last formal in the
	 * input declaration list.
	 */

	if (module->v.module.varargs)
	{
	    DXWarning ("#4650", name);
	    _dxf_ExPDestroyNode (module);
	    return (OK);
	}

	par = inlist[i];

	if (! strcmp (par, "..."))
	{
	    module->v.module.varargs = TRUE;
	    continue;			/* Don't save ... as a formal */
	}

	/* 
	 * look for parameter attributes:  parmname[attr:val,attr:val] 
         * and create an attribute node associated with the input parm.
	 */
        if ((attrstart = strchr(par, '['))) 
	{ 
	    id  = _dxf_ExPCreateId (_dxf_ExCopyStringN (par, attrstart-par));
	    id->attr = ExExtractAttrs (attrstart);
        } 
	else
	{
	    id  = _dxf_ExPCreateId (_dxf_ExCopyString (par));
	}

	LIST_APPEND (list, id);
	argcnt++;
    }

    /* 
     * if the module is asynchronous, you need two extra hidden inputs
     * to the module: the name and the value of a private input
     */
    if (flags & (MODULE_ASYNC | MODULE_ASYNCLOCAL)) 
    {
	id  = _dxf_ExPCreateId (_dxf_ExCopyString ("_asyncflag_name"));
	LIST_APPEND (list, id);
	argcnt++;

	id  = _dxf_ExPCreateId (_dxf_ExCopyString ("_asyncflag_value"));
	LIST_APPEND (list, id);
	argcnt++;

	module->v.module.posthidden = 2;
    }

    module->v.module.nin = argcnt;
    module->v.module.in  = list;


    /* 
     * output section.
     */

    /* Get the output names */
    argcnt = 0;
    list = NULL;

    for (i = 0; i < nout; i++)
    {
	par = outlist[i];

	/* 
	 * there isn't code here to handle varargs style outputs.
	 * it should probably be handled at some point.
	 */

	/* 
	 * look for parameter attributes:  parmname[attr:val,attr:val] 
         * and create an attribute node associated with the output parm.
	 */
        if ((attrstart = strchr(par, '['))) 
        { 
	    id  = _dxf_ExPCreateId (_dxf_ExCopyStringN (par, attrstart-par));
	    id->attr = ExExtractAttrs (attrstart);
        }
	else
	{
	    id  = _dxf_ExPCreateId (_dxf_ExCopyString (par));
	}
        
	LIST_APPEND (list, id);
	argcnt++;
    }

    module->v.module.nout = argcnt;
    module->v.module.out  = list;

    /*
     * now we can fill in the number of inputs and outputs.
     * there should be a way to pass these into any module which
     * has a variable number of parameters.  maybe as the first input/output?
     */

    if (flags & MODULE_OUTBOARD) 
    {
	char *h = host;

	if (!exec || !exec[0]) {
	    DXWarning ("#4652", name);
	    _dxf_ExPDestroyNode (module);
	    return (OK);
	}
	    
	if (!h || !h[0])
	    h = "localhost";

	/* 
	 * store the values for the first 5 inputs as the defaults.
	 */
	id = module->v.module.in;
	_dxf_ExEvaluateConstants (FALSE);

	id->v.id.dflt = _dxf_ExPCreateConst (TYPE_UBYTE, CATEGORY_REAL, 
					     strlen (exec) + 1, exec);

	id = id->next;
	id->v.id.dflt = _dxf_ExPCreateConst (TYPE_UBYTE, CATEGORY_REAL, 
					     strlen (h) + 1, h);

	id = id->next;
	id->v.id.dflt = _dxf_ExPCreateConst (TYPE_INT, CATEGORY_REAL, 
					     1, &module->v.module.flags);

	id = id->next;
	id->v.id.dflt = _dxf_ExPCreateConst (TYPE_INT, CATEGORY_REAL, 
					     1, &module->v.module.nin);

	id = id->next;
	id->v.id.dflt = _dxf_ExPCreateConst (TYPE_INT, CATEGORY_REAL, 
					     1, &module->v.module.nout);

	_dxf_ExEvaluateConstants (TRUE);


	/* if outboard is persistent and this is an MP architecture,
	 * the module must be PINNED to work correctly.
	 */
#ifdef DXD_IS_MP
	if (flags & MODULE_PERSISTENT)
	    module->v.module.flags |= MODULE_PIN;
#endif
	
	if ((flags & (MODULE_ASYNC | MODULE_ASYNCLOCAL)) && 
            !(flags & MODULE_PERSISTENT)) {
	    DXWarning("Outboard module must be PERSISTENT to be ASYNC; ASYNC flag ignored");
	    /* this could be an error; for now just continue */
	}
    }

    /* 
     * check here before you add the module that the func pointer isn't null.
     */

    if (func == NULL) {
	DXWarning ("#4654", name);
	_dxf_ExPDestroyNode (module);
	return (OK);
    }

    ret = _dxf_ExMacroInsert (module->v.module.id->v.id.id,
		        (EXObj) module);

    /*
     * Now that it's in the dictionary we don't need the reference that
     * _dxf_ExPCreateNode put on the module any more, e.g. we have transferred
     * ownership from the routine to the dictionary.
     */

    ExDelete (module);
    return (ret);
}


/*
 * usage:
 *  DXAddModule (name, func, flags, nin, in1, ..., inn, nout, out1, ..., outn)
 * or if flags has the OUTBOARD flag set:
 *  DXAddModule (name, func, flags, nin, in1, ..., inn, nout, out1, ..., outn, 
 *               exec, host)
 *
 *
 */



/*VARARGS*/
Error DXAddModule (char *name, ...)
{
    va_list		args;
    PFI			func;
    int			flags;
    int			nin;
    char 		**inlist;
    int			nout;
    char		**outlist;
    char 		*exec;
    char		*host;
    int			i;

    va_start (args,name);

    func  = va_arg (args,PFI);
    flags = va_arg (args,int);

    nin = va_arg (args,int);
    inlist = DXAllocateLocalZero(nin * sizeof(char **));
    if (!inlist)
	return ERROR;

    for (i=0; i<nin; i++)
	inlist[i] = va_arg (args,char *);

    nout = va_arg (args,int);
    outlist = DXAllocateLocalZero(nout * sizeof(char **));
    if (!outlist)
	return ERROR;

    for (i=0; i<nout; i++)
	outlist[i] = va_arg (args,char *);

    if (flags & MODULE_OUTBOARD) {
	exec = va_arg (args,char *);
	host = va_arg (args,char *);
    } else
	exec = host = NULL;

    va_end (args);

    return DXAddModuleV (name, func, flags, nin, inlist, nout, outlist,
			 exec, host);

}


/*-----------------------------------------------------*/
/*  Extract all attributes that are relevant to the    */
/*  exec and create attribute nodes for each one       */
/*-----------------------------------------------------*/
static node *ExExtractAttrs(char *attrstr)
{
   char *curr_attr;
   int tmp;
   node *attrlist = NULL;
   node *attr_node;
   char formatstr[MAX_ATTRLEN+3];

   if ((curr_attr=strstr(attrstr, ATTR_DIREROUTE)))  {
       strcpy (formatstr, ATTR_DIREROUTE);
       strcat (formatstr, ":%d");
       sscanf (curr_attr, formatstr, &tmp);
       attr_node = ExCreateAttrNode (ATTR_DIREROUTE, tmp);
       LIST_APPEND (attrlist, attr_node);
   }

   if ((curr_attr=strstr(attrstr, ATTR_CACHE)))  {
       strcpy (formatstr, ATTR_CACHE);
       strcat (formatstr, ":%d");
       sscanf (curr_attr, formatstr, &tmp);
       attr_node = ExCreateAttrNode (ATTR_CACHE, tmp);
       LIST_APPEND (attrlist, attr_node);
   }

   if ((curr_attr=strstr(attrstr, ATTR_RERUNKEY)))  {
       strcpy (formatstr, ATTR_RERUNKEY);
       strcat (formatstr, ":%d");
       sscanf (curr_attr, formatstr, &tmp);
       attr_node = ExCreateAttrNode (ATTR_RERUNKEY, tmp);
       LIST_APPEND (attrlist, attr_node);
   }

   return (attrlist);
}

static node *ExCreateAttrNode(char *attrstr, int attrval)
{
   node		*id;
   node		*val;
  
   id  = _dxf_ExPCreateId (_dxf_ExCopyString (attrstr));

   _dxf_ExEvaluateConstants (FALSE);
   val = _dxf_ExPCreateConst (TYPE_INT, CATEGORY_REAL, 1, (Pointer) &attrval); 
   _dxf_ExEvaluateConstants (TRUE);

   return _dxf_ExPCreateAttribute (id, val);
}

typedef struct cm_args {
    char *name;
    int nin;
    ModuleInput *in;
    int nout;
    ModuleOutput *out;
} cm_args;

Error CallModuleTask(Pointer Parg) 
{
    cm_args *arg = (cm_args *)Parg;
    return(DXCallModule(arg->name, arg->nin, arg->in, arg->nout, arg->out));
}

void DXModSetObjectInput(ModuleInput *in, char *name, Object obj)
{
    in->name = name;
    in->value = obj;
}

void DXModSetObjectOutput(ModuleOutput *out, char *name, Object *obj)
{
    out->name = name;
    out->value = obj;
}

Object DXModSetIntegerInput(ModuleInput *in, char *name, int n)
{
    Object ret_array = NULL;

    ret_array = (Object)DXMakeInteger(n);
    DXModSetObjectInput(in, name, ret_array);
    return(ret_array);
}

Object DXModSetFloatInput(ModuleInput *in, char *name, float f)
{
    Object ret_array = NULL;

    ret_array = (Object)DXMakeFloat(f);
    DXModSetObjectInput(in, name, ret_array);
    return(ret_array);
}

Object DXModSetStringInput(ModuleInput *in, char *name, char *s)
{
    Object ret_string = NULL;

    ret_string = (Object)DXMakeString(s);
    DXModSetObjectInput(in, name, ret_string);
    return(ret_string);
}

Error 
CallModuleSetupTask(char *name, int nin,  ModuleInput *in,
			 int nout, ModuleOutput *out)
{
    cm_args *cmargs = NULL;
    int i, ret;

    ret = ERROR;
    cmargs = DXAllocate(sizeof(cm_args));
    if(cmargs == NULL)
        goto error_return;
    cmargs->name = _dxf_ExCopyString(name);
    if(cmargs->name == NULL)
        goto error_return;
    cmargs->nin = nin;
    cmargs->in = (ModuleInput *)DXAllocate(nin * sizeof(ModuleInput));
    if(cmargs->in == NULL)
        goto error_return;
    for(i = 0; i < nin; i++) 
        cmargs->in[i].name = NULL;
    for(i = 0; i < nin; i++) {
        cmargs->in[i].name = _dxf_ExCopyString(in[i].name);
        cmargs->in[i].value = in[i].value;
    }
    cmargs->nout = nout;
    cmargs->out = (ModuleOutput *)DXAllocate(nout * sizeof(ModuleOutput));
    for(i = 0; i < nout; i++) {
        cmargs->out[i].name = NULL;
        cmargs->out[i].value = NULL;
    }    
    for(i = 0; i < nout; i++) {
        cmargs->out[i].name = _dxf_ExCopyString(out[i].name);
        cmargs->out[i].value = (Object *)DXAllocate(sizeof(Object *));
        if(cmargs->out[i].value == NULL)
            goto error_return;
    }
    ret = _dxf_ExRunOn(1, CallModuleTask, (Pointer)cmargs, 0);
    /* copy outputs to original structure. */
    for(i = 0; i < nout; i++) 
        *out[i].value = *cmargs->out[i].value;

error_return:
    if(cmargs) {
        DXFree(cmargs->name);
        if(cmargs->in) 
            for(i = 0; i < nin; i++) 
                DXFree(cmargs->in[i].name);
        DXFree(cmargs->in);
        if(cmargs->out) 
            for(i = 0; i < nout; i++) {
                DXFree(cmargs->out[i].name);
                DXFree(cmargs->out[i].value);
            }
        DXFree(cmargs->out);
    }
    DXFree(cmargs);
    return(ret);
}

Error 
DXCallModule(char *name, int nin,  ModuleInput *in,
			 int nout, ModuleOutput *out)
{
    node		*module;
    node		*formal;
    int			 i;
    int			 index;
    int			 anyNamed;
    Object		*inputs = NULL;
    Object		*outputs = NULL;
    int			retval = OK;
    int			errorexit;

    if(exJID != 1)
        return(CallModuleSetupTask(name, nin, in, nout, out));

    module = (node*)_dxf_ExMacroSearch(name);

    if (module == NULL)
    {
	DXSetError(ERROR_BAD_PARAMETER, "#8370", name);
	return ERROR;
    }
    if (module->type != NT_MODULE || module->v.function.def.func == NULL)
    {
	DXSetError(ERROR_BAD_PARAMETER, "#8370", name);
	return ERROR;
    }

    if (nin > module->v.function.nin)
    {
	DXSetError(ERROR_BAD_PARAMETER, "#8380", name, module->v.function.nin);
	return ERROR;
    }
    if (nout > module->v.function.nout)
    {
	DXSetError(ERROR_BAD_PARAMETER, "#8381", name, module->v.function.nout);
	return ERROR;
    }

    inputs = (Object*)DXAllocateLocalZero(
	module->v.function.nin * sizeof (Object));
    outputs = (Object*)DXAllocateLocalZero(
	module->v.function.nout * sizeof (Object));

    anyNamed = 0;
    for (i = 0; i < nin; ++i)
    {
	if (in[i].name == NULL)
	{
	    if (anyNamed)
	    {
		DXSetError(ERROR_BAD_PARAMETER, "#8390", name);
                retval = ERROR;
		goto deref;
	    }
	    inputs[i] = in[i].value;
	}
	else
	{
	    anyNamed = 1;
	    for (index = 0, formal = module->v.function.in;
		 formal;
		 formal = formal->next, ++index)
	    {
		if (strcmp(in[i].name, formal->v.id.id) == 0) {
		    inputs[index] = in[i].value;
		    break;
		}
	    }
	    if (formal == NULL)
	    {
		DXSetError(ERROR_BAD_PARAMETER, "#8400", in[i].name, name);
                retval = ERROR;
		goto deref;
	    }
	}
    }

    for (i = 0; i < module->v.function.nin; ++i)
        if (inputs[i]) 
            DXReference(inputs[i]);

    /* some modules set error when they do not want to print or exit */
    /* save error exit state so we can reset after module is called. */
    errorexit = DXGetErrorExit();
    DXSetErrorExit(0);

    retval = (*module->v.function.def.func)(inputs, outputs);

    DXSetErrorExit(errorexit);

    if(retval == ERROR) {
        if(errorexit == 1)
            DXPrintError(name);
        else if(errorexit >= 2)
            DXErrorExit(name);
	goto deref;
    }

    anyNamed = 0;
    for (i = 0; i < nout; ++i)
    {
	if (out[i].name == NULL)
	{
	    if (anyNamed) {
		DXSetError(ERROR_BAD_PARAMETER, "#8391", name);
                retval = ERROR;
		goto deref;
	    }
	    *out[i].value = outputs[i];
	    outputs[i] = NULL;
	}
	else
	{
	    anyNamed = 1;
	    for (index = 0, formal = module->v.function.out;
		 formal;
		 formal = formal->next, ++index)
	    {
		if (strcmp(out[i].name, formal->v.id.id) == 0) {
		    *out[i].value = outputs[index];
		    outputs[index] = NULL;
		    break;
		}
	    }
	    if (formal == NULL)
	    {
		DXSetError(ERROR_BAD_PARAMETER, "#8401", out[i].name, name);
                retval = ERROR;
		goto deref;
	    }
	}
    }

    /* delete unused outputs */

    if (outputs) {
	for (i = 0; i < module->v.function.nout; ++i)
	    if (outputs[i]) 
		DXDelete(outputs[i]);
    }

deref:
    if(out) {
	for (i = 0; i < nout; ++i)
            DXReference(*out[i].value);
    }
    for (i = 0; i < module->v.function.nin; ++i)
       DXDelete(inputs[i]);
    if (out) {
	for (i = 0; i < nout; ++i)
	    if (out[i].value) 
		DXUnreference(*out[i].value);
    }

    DXFree((Pointer)inputs);
    DXFree((Pointer)outputs);

    return retval;
}

/* if we are redefining a module, make sure that it matches the old
 *  definition exactly for a compiled in module, or make sure the
 *  entry point is provided for a loadable module, or make sure it's
 *  an outboard module.
 */

Error 
DXCompareModule(char *name, PFI func, int flags, int nin, char *inlist[],
		            int nout, char *outlist[], char *exec, char *host)
{
    node		*prev_defn;

    /* does this add a reference? */
    prev_defn = (node*)_dxf_ExMacroSearch(name);

    if (prev_defn == NULL)
    {
	DXSetError(ERROR_BAD_PARAMETER, "%s is not defined", name);
	goto error;
    }

    if (flags != prev_defn->v.function.flags)
    {
	DXSetError(ERROR_BAD_PARAMETER, 
		   "inconsistent definition of %s for %s: %d not same as %d",
		   "flags", name, flags, prev_defn->v.function.flags);
	goto error;
    }
    if (nin != prev_defn->v.function.nin)
    {
	DXSetError(ERROR_BAD_PARAMETER, 
		   "inconsistent definition of %s for %s: %d not same as %d",
		   "number of inputs", name, nin, prev_defn->v.function.nin);
	goto error;
    }
    if (nout != prev_defn->v.function.nout)
    {
	DXSetError(ERROR_BAD_PARAMETER, 
		   "inconsistent definition of %s for %s: %d not same as %d",
		   "number of outputs", name, nout, prev_defn->v.function.nout);
	goto error;
    }


#if 0
    {
    int			n;
    char		*par;
    int			i;
    node		*module;
    node		*id;
    node		*list;
    char		*sname;
    char		*attrstart;
    int			argcnt;

    /* fix this to be sure the input names and attributes match what's
     *  already been defined 
     */

    argcnt = 0;
    list = NULL;


    /* Get the input names */
    for (i = 0; i < nin; i++)
    {
	/*
	 * Only allow the ... construct to be the last formal in the
	 * input declaration list.
	 */

	if (module->v.module.varargs)
	{
	    DXWarning ("#4650", name);
	    _dxf_ExPDestroyNode (module);
	    return (OK);
	}

	par = inlist[i];

	if (! strcmp (par, "..."))
	{
	    module->v.module.varargs = TRUE;
	    continue;			/* Don't save ... as a formal */
	}

	/* 
	 * look for parameter attributes:  parmname[attr:val,attr:val] 
         * and create an attribute node associated with the input parm.
	 */
        if (attrstart = strchr (par, '[')) 
	{ 
	    id  = _dxf_ExPCreateId (_dxf_ExCopyStringN (par, attrstart-par));
	    id->attr = ExExtractAttrs (attrstart);
        } 
	else
	{
	    id  = _dxf_ExPCreateId (_dxf_ExCopyString (par));
	}

	LIST_APPEND (list, id);
	argcnt++;
    }

    module->v.module.nin = argcnt;
    module->v.module.in  = list;


    /* 
     * output section.
     */

    /* Get the output names */
    argcnt = 0;
    list = NULL;

    for (i = 0; i < nout; i++)
    {
	par = outlist[i];

	/* there was attribute code here which i should put back
         * to verify the attrs match.
	 */

	LIST_APPEND (list, id);
	argcnt++;
    }

    module->v.module.nout = argcnt;
    module->v.module.out  = list;
    }
#endif

    ExDelete (prev_defn);
    return OK;

  error:
    ExDelete (prev_defn);
    return ERROR;
}

