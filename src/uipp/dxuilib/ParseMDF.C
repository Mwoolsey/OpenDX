/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"


#include <stdio.h>
#include <ctype.h>
#include <stdarg.h>
#include <string.h>

#include "lex.h"
#include "DXStrings.h"

#include "ParseMDF.h"

#include "DXType.h"
#include "NodeDefinition.h"
#include "ParameterDefinition.h"
#include "Dictionary.h"
#include "NDAllocatorDictionary.h"
#include "ErrorDialogManager.h"


/***
 *** State constants:
 ***/

#define ST_NONE			0
#define ST_MODULE		1
#define ST_CATEGORY		2
#define ST_DESCRIPTION		4
#define ST_INPUT		8
#define ST_REPEAT	 	16	
#define ST_OUTPUT		32	
#define ST_ERROR		64	
#define ST_SAW_REPEAT		128	
#define ST_FLAGS		256
#define ST_OUTBOARD		512	
#define ST_LOADABLE		1024	
#define ST_PACKAGE		2048	
#define ST_OPTIONS		4096	


#ifdef NOT_YET
/***
 *** Static pointer to MDF (for sorting purposes)
 ***/
static Dictionary* _mdf;
#endif

Dictionary *theDynamicPackageDictionary = new Dictionary;

/*****************************************************************************/
/* _ParsePackageLine -						     */
/*                                                                           */
/* Parses and processes the PACKAGE line.				     */
/*                                                                           */
/*****************************************************************************/

static
boolean _ParsePackageLine(char*      line,
			    int        lineNumber,
			    int        start)
{
    char *begin, *end; 

    ASSERT(line);
    ASSERT(lineNumber > 0);
    ASSERT(start >= 0);

    begin = &line[start];

    /*
     * Skip space.
     */
    SkipWhiteSpace(begin);

    /*
     * Parse package name.
     */
    end = begin;
    while (*end && !IsWhiteSpace(end) && (*end != '\n') && (*end != '\r'))
	end++;
    if (*end)
	end--;

    if  (end == begin)
	goto error;

    *end = '\0'; 
    theDynamicPackageDictionary->replaceDefinition(
		(const char*)begin,
		(const void*) NULL);

    return TRUE;

error:
    return FALSE;
}
/*****************************************************************************/
/* _ParseModuleLine -						     */
/*                                                                           */
/* Parses and processes the MODULE line.				     */
/*                                                                           */
/*****************************************************************************/

static
NodeDefinition *_ParseModuleLine(Dictionary    *mdf,
			    char*      line,
			    int        lineNumber,
			    int        start)
{
    int current;
    int temp;
    char *function;
    NodeDefinition *module = NUL(NodeDefinition*);

    ASSERT(mdf != NUL(Dictionary*));
    ASSERT(line);
    ASSERT(lineNumber > 0);
    ASSERT(start >= 0);

    current = start;

    /*
     * Skip space.
     */
    SkipWhiteSpace(line,current);

    /*
     * Parse module name.
     */
    temp = current;
    if (NOT IsRestrictedIdentifier(line, temp) ||
	    (temp != STRLEN(line) && !IsWhiteSpace(line, temp)))
    {
	ErrorMessage("Invalid %s: %s must start with a letter and contain "
		     "only letters and numbers (file %s line %d)",
	    "module name", line + current, "MDF", lineNumber);
 	goto error;
    }

    /*
     * Extract the module name.
     */
    function = &line[current];

    /*
     * Look to see if the module has already been defined.
     * remember the name index.
     */
    if (mdf->findDefinition(function))
    {
	ErrorMessage
	    ("Encountered another definition of module \"%s\" on line %d.",
	     function,
	     lineNumber);

    	goto error;
    }

    //
    // Get a new NodeDefinition by name 
    //
    module = theNDAllocatorDictionary->allocate(function);
    if (!module) {
	ErrorMessage("Can not find NodeDefinition allocator for module '%s'",
				function);
	    goto error;
    }
    module->setName(function);

    return module;

error:
    if (module) delete module;
    return NUL(NodeDefinition*);
}


#ifdef NOT_NEEDED
/*****************************************************************************/
/* _ParseCategoryLine -						     */
/*                                                                           */
/* Parses and processes the CATEGORY line.				     */
/*                                                                           */
/*****************************************************************************/

static
boolean _ParseCategoryLine(Dictionary*    mdf,
			      NodeDefinition* module,
			      char*      line,
			      int        lineNumber,
			      int        start)
{
    int current;
    int temp;

    ASSERT(mdf != NUL(Dictionary*));
    ASSERT(module != NUL(NodeDefinition*));
    ASSERT(line);
    ASSERT(lineNumber > 0);
    ASSERT(start >= 0);

    current = start;

    return TRUE;
}

#endif // NOT_NEEDED

boolean ParseMDFTypes(ParameterDefinition *param, char *p, int lineNumber)
{
    char buffer[1000];
    char *q;
    int current;
    char      save;
    DXType	*input_type = NULL;
    Type	type;

    /*
     * Begin a new type.
     */
    current = 0;

    SkipWhiteSpace(p);

    while(*p != '\0')
    {
	/*
	 * Demarcate a token.
	 */
	for (q = p; *q != ' ' AND *q != '\t' AND *q != '\0'; q++)
	    buffer[current++] = *q;
	buffer[current++] = ' ';

	save = *q;
	*q   = '\0';

	if (EqualString(p, "or"))
	{
	    /*
	     * Back up and erase " or " substring from end of buffer.
	     */
	    current -= 4;

	    /*
	     * Find a matching type.
	     */
	    buffer[current] = '\0';
	    if ((type = DXType::StringToType(buffer)) == DXType::UndefinedType)
	    {
		ErrorMessage
		    ("Erroneous parameter type encountered in line %d.",
		     lineNumber);
		if (input_type)
		    delete input_type;
		return FALSE;
	    }

	    /*
	     * Begin a new type and the type to the param list of type.
	     */
            input_type = new DXType(type);
	    boolean r = param->addType(input_type);
	    ASSERT(r);
	    input_type = NUL(DXType*);

	    current = 0;
	}

	/*
	 * Restore the saved character and skip space.
	 */
	*q = save;
	p = q; 
	SkipWhiteSpace(p);
    }

    /*
     * Find a matching type.
     */
    buffer[--current] = '\0';
    if ((type = DXType::StringToType(buffer)) == DXType::UndefinedType) 
    {
	ErrorMessage ("Erroneous parameter type encountered in line %d.",
	     lineNumber);
	if (input_type)
	    delete input_type;
	return FALSE;
    }
    /*
     * Begin a new type and add the type to the input list of type.
     */
    input_type = new DXType(type);
    boolean r  = param->addType(input_type);
    ASSERT(r);

    return TRUE;
}

/*****************************************************************************/

//
// Parse 'attribute:%d' from the given line.
// Return TRUE on success and set *val to %d.
//
static boolean GetIntegerAttribute(const char *line, const char *attr, 
					int *val)
{
    const char *c = strstr(line,attr);
    if (!c)
	return FALSE;
    c += STRLEN(attr);
    while (*c && *c != ':') c++;
    if (!*c)
	return FALSE;
    c++;
    *val = atoi(c);
    return TRUE;
}

boolean _ParseParameterAttributes(ParameterDefinition *pd , const char *attr)
{
    int val;
    boolean v;

    if (GetIntegerAttribute(attr,"private",&val)) {
	v = (val == 1 ? FALSE : TRUE);
	pd->setViewability(v);
	if (!v)	pd->setDefaultVisibility(FALSE);
    } else if (GetIntegerAttribute(attr,"hidden",&val)) {
	v = (val == 1 ? FALSE : TRUE);
	pd->setDefaultVisibility(v);
    } 

    if (GetIntegerAttribute(attr,"visible",&val)) {
	boolean visible=FALSE, viewable=FALSE;
	if (val == 0) {
	    viewable = TRUE;
	    visible = FALSE;
	} else if (val == 1) {
	    viewable = TRUE;
	    visible = TRUE;
	} else if (val == 2) {
	    viewable = FALSE;
	    visible  = FALSE;
	}
	pd->setViewability(viewable);
	pd->setDefaultVisibility(visible);
    } 

    if (GetIntegerAttribute(attr,"cache",&val)) 
	pd->setDefaultCacheability((Cacheability)val);
    if (pd->isInput() && 
	GetIntegerAttribute(attr,"reroute",&val) &&
	(val >= 0))
	pd->setRerouteOutput(val+1);	// MDF uses 0 based indices.
    
    return TRUE;
}

/*****************************************************************************/
/* _ParseOUTBOARDLine -							     */
/*                                                                           */
/* Parses and processes the ' OUTBOARD "command" [ ; host ]' line.     */
/*                                                                           */
/*****************************************************************************/

static
boolean _ParseOutboardLine(NodeDefinition* module,
			   char*      line,
			   int        lineNumber,
			   int        start)
{
    int       current;
    int       temp;
    int       i;
    char*     substring[2];

    ASSERT(module != NUL(NodeDefinition*));
    ASSERT(line);
    ASSERT(lineNumber > 0);
    ASSERT(start >= 0);

    current = start;

    /*
     * Separate the two sections of the OUTBOARD string.
     */
    substring[0] = NULL;
    substring[1] = NULL;
    for (i = 0; i < 2; i++)
    {
	/*
	 * Skip space.
	 */
	SkipWhiteSpace(line,current);

	/*
	 * Tag the beginning of the substring.
	 */
	substring[i] = &line[current];

	/*
	 * Find the end of the substring.
	 */
	while (line[current] != ';' AND line[current] != '\0')
	    current++;

	/*
	 * Mark the end of the substring and remove trailing space.
	 */
	temp = current - 1;
	if (line[current] == ';')
	    line[current++] = '\0';
	else
	    break;

	while (line[temp] == ' ' OR line[temp] == '\t')
	{
	    line[temp] = '\0';
	    temp--;
	}
    }

#ifdef DEBUG
    char *s0 = substring[0];
    char *s1 = substring[1];
#endif
    if (!substring[0])
    {
	goto error;	
    }

    /*
     * Parse the first substring:  COMMAND with optional "'s 
     */
    if (substring[0][0] == '"') {
	i = STRLEN(substring[0]);
	if (substring[0][i-1] != '"') 
	    goto error;	
	substring[0][i-1] = '\0';
	i = 1;
    } else
	i = 0;

    module->setOutboardCommand(&substring[0][i]);
    if (substring[1])
        module->setDefaultOutboardHost(substring[1]);

    return TRUE;

error:
    ErrorMessage("Encountered an erroneous OUTBOARD specification on line %d.",
	 lineNumber);
    return FALSE;
}
/*****************************************************************************/
/* _ParseLOADABLELine -							     */
/*                                                                           */
/* Parses and processes the ' Loadable file'				     */
/*                                                                           */
/*****************************************************************************/

static
boolean _ParseLoadableLine(NodeDefinition* module,
			   char*      line,
			   int        lineNumber,
			   int        start)
{
    int       current;
    int       temp;
    int       i;
    char*     substring;

    ASSERT(module != NUL(NodeDefinition*));
    ASSERT(line);
    ASSERT(lineNumber > 0);
    ASSERT(start >= 0);

    current = start;

    /*
     * Separate the two sections of the LOADABLE string.
     */
    substring = NULL;
    for (i = 0; i < 1; i++)
    {
	/*
	 * Skip space.
	 */
	SkipWhiteSpace(line,current);

	/*
	 * Tag the beginning of the substring.
	 */
	substring = &line[current];

	/*
	 * Find the end of the substring.
	 */
	while (line[current] != ';' AND line[current] != '\0')
	    current++;

	/*
	 * Mark the end of the substring and remove trailing space.
	 */
	temp = current - 1;
	if (line[current] == ';')
	    line[current++] = '\0';
	else
	    break;

	while (line[temp] == ' ' OR line[temp] == '\t')
	{
	    line[temp] = '\0';
	    temp--;
	}
    }

#ifdef DEBUG
    char *s0 = substring;
#endif
    if (!substring)
    {
	goto error;	
    }
    
#ifdef DXD_NON_UNIX_DIR_SEPARATOR
    for (i=0; i<strlen(substring); i++)
    	if(substring[i] == '\\') substring[i] = '/';
#endif

    module->setDynamicLoadFile(substring);
    return TRUE;

error:
    ErrorMessage("Encountered an erroneous LOADABLE specification on line %d.",
	 lineNumber);
    return FALSE;
}
/*****************************************************************************/
/* _ParseInputLine -							     */
/*                                                                           */
/* Parses and processes the INPUT line.					     */
/*                                                                           */
/*****************************************************************************/

static
boolean _ParseInputLine(Dictionary*    mdf,
			   NodeDefinition* module,
			   char*      line,
			   int        lineNumber,
			   int        start)
{
    ParameterDefinition *input = NUL(ParameterDefinition*);
    Symbol	symbol;
    int       current;
    int       temp;
    int       i;
    char*     p;
    char*     substring[4];
    char *begin_attributes, *end_attributes;

    ASSERT(mdf != NUL(Dictionary*));
    ASSERT(module != NUL(NodeDefinition*));
    ASSERT(line);
    ASSERT(lineNumber > 0);
    ASSERT(start >= 0);

    current = start;

    /*
     * Separate the four sections of the INPUT string.
     */
    for (i = 0; i < 4; i++)
    {
	/*
	 * Skip space.
	 */
	SkipWhiteSpace(line,current);

	/*
	 * Tag the beginning of the substring.
	 */
	substring[i] = &line[current];

	/*
	 * Find the end of the substring.
	 */
	while (line[current] != ';' AND line[current] != '\0')
	    current++;

	/*
	 * Mark the end of the substring and remove trailing space.
	 */
	if (line[current] == ';')
	    line[current++] = '\0';
	else
	    break;

	temp = current - 2;
	while (line[temp] == ' ' OR line[temp] == '\t')
	{
	    line[temp] = '\0';
	    temp--;
	}
    }

    if (i < 3)
    {
	ErrorMessage
	    ("Encountered an erroneous INPUT specification on line %d.",
	     lineNumber);

	goto error;	
    }

    /*
     * Parse the first substring:  NAME and optional attributes.
     */
    begin_attributes = (char *) strchr(substring[0],'[');
    if (begin_attributes) {
	char *p = begin_attributes-1;
	// strip trailing white space from name of parameter.
	while (IsWhiteSpace(p)) {
	    *p = '\0';
	    p--;
  	}
	*begin_attributes = '\0';	// Terminate identifier
	begin_attributes++;		// Point to beginning of attributes.

        end_attributes = strrchr(begin_attributes,']');
	if (end_attributes) 
	    *end_attributes = '\0';		// Terminate attributes.
	else
	    begin_attributes = NULL;
    }
    current = 0;
    if (NOT IsIdentifier(substring[0], current) ||
	    (current != STRLEN(substring[0]) &&
		!IsWhiteSpace(substring[0], current)))
    {
	ErrorMessage("Invalid %s: %s (file %s, line %d)",
	    "input name", substring[0], "MDF", lineNumber);
	goto error;	
    }
   
    /*
     * Place the name into the symbol table, and create a parameter with
     * the given name;
     */
    symbol = mdf->getSymbolManager()->registerSymbol(substring[0]);
    input = new ParameterDefinition(symbol);
    input->markAsInput();		// Set this as an input parameter
    input->setDefaultVisibility();	// Default visibility is on.

    // 
    //  Now that we have a parameter, parse the attributes if there are any.
    // 
	if (begin_attributes) {
		if (!_ParseParameterAttributes(input,begin_attributes)) {  
			ErrorMessage("Unrecognized input attribute(s) (%s) in MDF line %d\n",
				begin_attributes, lineNumber);
			goto error;	
		}
    }

    /*
     * Parse the second substring:  TYPE.
     */
    p = substring[1];
    if (!ParseMDFTypes(input, p, lineNumber))
	goto error;

    /*
     * Parse the third substring:  DEFAULT VALUE.
     */

    /*
     * If the value is equal to "(none)", then mark this parameter
     * as a required parameter.
     */
    
    if (substring[2][0] == '(') {	//  A descriptive value
	if (EqualString(substring[2], "(none)"))
	    input->setRequired();
	else
	    input->setNotRequired();
	input->setDescriptiveValue(substring[2]);
    } else if (!input->setDefaultValue(substring[2])) {
	    ErrorMessage(
		"Default value given on line %d not one of given types.",
		 lineNumber);
	    goto error;
    }




    /*
     * Parse the fourth substring:  DESCRIPTION.
     */
    input->setDescription(substring[3]);


    /*
     * One more input parameter processed...
     */
    module->addInput(input);
    
    return TRUE;

error:
    if (input) delete input;
    return FALSE;
}


/*****************************************************************************/
/* _ParseOutputLine -						     */
/*                                                                           */
/* Parses and processes the OUTPUT line.				     */
/*                                                                           */
/*****************************************************************************/

static
boolean _ParseOutputLine(Dictionary*    mdf,
			    NodeDefinition* module,
			    char*      line,
			    int        lineNumber,
			    int        start)
{
    ParameterDefinition* output = NUL(ParameterDefinition*);
    Symbol	symbol;
    int       current;
    int       temp;
    int       i;
    char*     p;
    char*     substring[3];

    ASSERT(mdf != NUL(Dictionary*));
    ASSERT(module != NUL(NodeDefinition*));
    ASSERT(line);
    ASSERT(lineNumber > 0);
    ASSERT(start >= 0);

    current = start;

    /*
     * Separate the three sections of the OUTPUT string.
     */
    for (i = 0; i < 3; i++)
    {
	/*
	 * Skip space.
	 */
	SkipWhiteSpace(line,current);

	/*
	 * Tag the beginning of the substring.
	 */
	substring[i] = &line[current];

	/*
	 * Find the end of the substring.
	 */
	while (line[current] != ';' AND line[current] != '\0')
	    current++;

	/*
	 * Mark the end of the substring and remove trailing space.
	 */
	if (line[current] == ';')
	    line[current++] = '\0';
	else
	    break;

	temp = current - 2;
	while (line[temp] == ' ' OR line[temp] == '\t')
	{
	    line[temp] = '\0';
	    temp--;
	}
    }

    if (i < 2)
    {
	ErrorMessage
	    ("Encountered an erroneous OUTPUT specification on line %d.",
	     lineNumber);

	goto error;
    }

    /*
     * Parse the first substring:  NAME.
     */
    char *begin_attributes, *end_attributes;
#if 0
    begin_attributes = strchr(substring[0],'[');
    end_attributes = strrchr(substring[0],']');
    if (begin_attributes && end_attributes) {
	*begin_attributes = '\0';	// Terminate identifier
	begin_attributes++;		// Point to beginning of attributes.
	*end_attributes = '\0';		// Terminate attributes.
    }
#else
    begin_attributes = (char *) strchr(substring[0],'[');
    if (begin_attributes) {
	char *p = begin_attributes-1;
	// strip trailing white space from name of parameter.
	while (IsWhiteSpace(p)) {
	    *p = '\0';
	    p--;
  	}
	*begin_attributes = '\0';	// Terminate identifier
	begin_attributes++;		// Point to beginning of attributes.

        end_attributes = strrchr(begin_attributes,']');
	if (end_attributes) 
	    *end_attributes = '\0';		// Terminate attributes.
	else
	    begin_attributes = NULL;
    }
#endif
    current = 0;
    if (NOT IsIdentifier(substring[0], current) ||
	    (current != STRLEN(substring[0]) &&
		!IsWhiteSpace(substring[0], current)))
    {
	ErrorMessage("Invalid %s: %s (file %s, line %d)",
	    "output name", substring[0], "MDF", lineNumber);
	goto error;
    }

    /*
     * Place the name into the parameter symbol table, and save its index.
     */
    symbol = theSymbolManager->registerSymbol(substring[0]);
    output = new ParameterDefinition(symbol);
    output->markAsOutput();			// Mark parameter as an output.
    output->setDefaultVisibility();		// Default visibility is on.

    // 
    //  Now that we have a parameter, parse the attributes if there are any.
    // 
    if (begin_attributes) {
        if (!_ParseParameterAttributes(output,begin_attributes)) {  
	    ErrorMessage("Unrecognized output attribute(s) (%s) in MDF line %d\n"
		, lineNumber);
	    goto error;	
	}
    }

    /*
     * Parse the second substring:  TYPE.
     */
    p = substring[1];
    if (!ParseMDFTypes(output, p, lineNumber))
	goto error;

    /*
     * Parse the third substring:  DESCRIPTION.
     */
    output->setDescription(substring[2]);

    module->addOutput(output);

    return TRUE;
error:
    if (output) delete output;

    return FALSE;
}


/*****************************************************************************/
/* _ParseOptionsLine -						     */
/*                                                                           */
/* Parses and processes the OPTIONS line.				     */
/*                                                                           */
/*****************************************************************************/

static
boolean _ParseOptionsLine(Dictionary*    mdf,
			      NodeDefinition* module,
			      char*      line,
			      int        lineNumber,
			      int        start)
{
    ASSERT(mdf != NUL(Dictionary*));
    ASSERT(module != NUL(NodeDefinition*));
    ASSERT(line);
    ASSERT(lineNumber > 0);
    ASSERT(start >= 0);

    int inum = module->getInputCount();
    char *p = &line[start];

    // assumes we're working on the most recently added param,
    // which should be an OK assumption.
    ParameterDefinition* pd = module->getInputDefinition(inum);
    return ParseMDFOptions (pd, p);
}


//
// Expose OPTIONS line parsing so that it can be called
// from MacroDefintion.
//
boolean ParseMDFOptions (ParameterDefinition* pd, char* p)
{
	/*
	* Parse Selection  values (separated by ';').
	*/

	char *value = new char[STRLEN(p) + 1];
	while (*p) { 
		memset(value, 0, STRLEN(p) + 1);
		SkipWhiteSpace(p);
		char *from = p, *to = value;
		while (*from && (*from != ';')) {
			if ((*from == '\\') && (*(from+1) == ';'))
				from++;
			*to = *from; 
			from++;
			to++;
		}
		if (to > value) {
			if(*from == ';')
				*(to--) = '\0';
			//
			// Clip trailing white space.
			//
			if(IsWhiteSpace(to))
				while ((to > value) && IsWhiteSpace(to))
					*(to--) = '\0';
			else if(*to)
				*(++to) = '\0';
		}
		pd->addValueOption(value);
		if(*from)
			p = from+1; 
		else
			p = from;
	}

	delete[] value;
	return TRUE;
}


/*****************************************************************************/
/* _ParseRepeatLine -						     */
/*                                                                           */
/* Parses and processes the REPEAT line.				     */
/*                                                                           */
/*****************************************************************************/

static
boolean _ParseRepeatLine(Dictionary*    mdf,
			      NodeDefinition* module,
			      char*      line,
			      int        lineNumber,
			      int        start,
			      int	io_state)
{
    int current;
    int temp;
    int value;
    int cnt;
    char *ioname;

    ASSERT(mdf != NUL(Dictionary*));
    ASSERT(module != NUL(NodeDefinition*));
    ASSERT(line);
    ASSERT(lineNumber > 0);
    ASSERT(start >= 0);
    ASSERT(io_state == ST_INPUT || io_state == ST_OUTPUT);


    current = start;

    /*
     * Skip space.
     */
    SkipWhiteSpace(line,current);

    /*
     * Parse repeat value.
     */
    temp = current;
    if (NOT IsInteger(line, temp))
    {
	ErrorMessage
	    ("Encountered error when expecting a repeat value on line %d.",
	     lineNumber);

	return FALSE;
    }

    value = atoi(&line[current]);

    if (io_state == ST_INPUT) {
	cnt = module->getInputCount();
        ioname = "input";
	module->setInputRepeatCount(value);
    } else {
	cnt = module->getOutputCount();
        ioname = "output";
	module->setOutputRepeatCount(value);
    }


    if (value > cnt)  
    {
	ErrorMessage
	    ("The repeat value on line %d is greater than the number of prior %s parameters.",
	     lineNumber, ioname);

        if (io_state == ST_INPUT)
	    module->setInputRepeatCount(0);
	else
	    module->setOutputRepeatCount(0);
	return FALSE;
    }
  
    return TRUE;
}


/*****************************************************************************/
/* _FinishNodeDefinition -						     */
/*                                                                           */
/* Return TRUE if the module was added to the mdf dictionary 		     */ 
/*                                                                           */
/*                                                                           */
/*****************************************************************************/

static
boolean _FinishNodeDefinition(Dictionary*    mdf,
			   NodeDefinition* module)
{

    ASSERT(mdf != NUL(Dictionary*));
    ASSERT(module != NUL(NodeDefinition*));

    Symbol nameSym = module->getNameSymbol();

    //
    // Allow, the Get and Set modules to be in the mdf.  They can be placed 
    // there even if there is not category, which allows us to change to
    // using GetLocal/Global and SetLocal/Global instead. 
    // See GlobalLocalNode.h.
    //

    if (module->getCategorySymbol() || 
	(nameSym == NDAllocatorDictionary::GetNodeNameSymbol) || 
	(nameSym == NDAllocatorDictionary::SetNodeNameSymbol)) {
	module->completeDefinition();
	mdf->addDefinition(nameSym,module);
	return TRUE;
    } else {
	return FALSE;
    }
    
}


/*****************************************************************************/
/* ReadMDF -								     */
/*                                                                           */
/* Read and parse MDF from open file stream.				     */
/*                                                                           */
/*****************************************************************************/

boolean ReadMDF(Dictionary* mdf, FILE*   input, boolean uionly)
{
	NodeDefinition *module;
	int        state;
	int        start;
	int        end;
	Symbol	category;
	int        line_number;
	boolean    parsed_flags, checked_flags=FALSE, finished;
	boolean    parsed_category,  parsed_description, parsed_outboard;
	boolean    checked_category=FALSE, checked_description=FALSE, checked_outboard=FALSE; 
	boolean	parsed_loadable, checked_loadable=FALSE;
	boolean	checked_repeat=FALSE;
	boolean    get_another_line;
	char*      p;
	char       line[2048];
	int		last_iostate;


	ASSERT(mdf != NUL(Dictionary*));
	ASSERT(input);

	module           = NUL(NodeDefinition*);
	line_number      = 0;
	state            = ST_PACKAGE;
	last_iostate = ST_NONE;

	finished         = FALSE;
	get_another_line = TRUE;
	parsed_description = parsed_outboard =  parsed_loadable =
		parsed_category = parsed_flags = FALSE;

	for (;;)
	{
		if (get_another_line)
		{

			checked_description = parsed_description;
			checked_repeat	= /*parsed_repeat =*/ FALSE; 
			checked_loadable	= parsed_loadable;
			checked_outboard	= parsed_outboard;
			checked_category	= parsed_category;
			checked_flags	= parsed_flags;

			for (;;)
			{
				/*
				* Get a line to parse.
				*/
				p = fgets(line, sizeof(line), input);
				ASSERT(STRLEN(line) < 2048);
				line_number++;

				/*
				* If no more lines, exit.
				*/
				if (p == NUL(char*))
				{
					finished = TRUE;
					break;
				}

				/*
				* Skip leading whitespace.
				*/
				for(start = 0;
					line[start] == ' ' OR line[start] == '\t' OR
					line[start] == '\n' OR line[start] == '\r';
				start++)
					;

				/*
				* If the string is not all spaces and  not a comment exit.
				*/
				if (line[start] != '\0' AND line[start] != '#')
					break;
			}

			if (NOT finished)
			{
				/*
				* Remove trailing whitespace.
				*/
				for(end = STRLEN(line) - 1;
					line[end] == ' ' OR line[end] == '\t' OR line[end] == '\n' OR line[end] == '\r';
					end--)
					;

				line[end + 1] = '\0';
			}
		}
		else
		{
			get_another_line = TRUE;
		}

		/*
		* If no more input, i.e. finished, then exit loop.
		*/
		if (finished)
			break;

		/*
		* Otherwise, parse according to the current state.
		*/
		switch(state)
		{
		case ST_PACKAGE:
			/*
			* Parse the standalone "PACKAGE" keyword.
			*/
			if (IsToken(line, "PACKAGE", start)) {
				if (!_ParsePackageLine(line,line_number,start)) {
					ErrorMessage
						("Encountered error when parsing \"PACKAGE\" keyword "
						"on line %d.",
						line_number);
					goto error;
				}
				// Continue to try and parse PACKAGE until we move to MODULE	
			} else {
				/*
				* Don't get another line yet...
				*/
				get_another_line = FALSE;
				state = ST_MODULE;
			}


			break;

		case ST_MODULE:
			/*
			* Parse "MODULE" keyword.
			*/
			if (NOT IsToken(line, "MODULE", start))
			{
				ErrorMessage
					("Encountered error when expecting \"MODULE\" keyword "
					"on line %d.",
					line_number);

				goto error;
			}

			/*
			* If a module definition has already begun, end it here.
			*/
			if (module)
			{
				if (!_FinishNodeDefinition(mdf, module))
					delete module;
			}

			checked_description = parsed_description = FALSE;
			checked_outboard	= parsed_outboard = FALSE;
			checked_loadable	= parsed_loadable = FALSE;
			checked_category	= parsed_category = FALSE;
			checked_flags	= parsed_flags    = FALSE;
			checked_repeat	= /*parsed_repeat   =*/ FALSE;

			/*
			* Add the module index to the module index list.
			*/

			if ( (module = _ParseModuleLine(mdf, line, line_number, start)) )
			{
				/*
				* Next, parse category line.
				*/
				state = ST_CATEGORY;
				if (uionly)
					module->setUILoadedOnly();
			}
			else
			{
				goto error;
			}
			break;

		case ST_CATEGORY:
			/*
			* Parse "CATEGORY" keyword.
			*/
			checked_category = TRUE;
			if (IsToken(line, "CATEGORY", start))
			{
				/*
				* Skip space.
				*/
				SkipWhiteSpace(line,start);

				/*
				* Add category name to the category name symbol table and
				* remember the category index.
				*/
				category = theSymbolManager->registerSymbol(&line[start]);
				if (!module) {
					ErrorMessage
						("Encountered unexpected \"CATEGORY\" on line %d.",
						line_number);
					goto error;
				}

				module->setCategory(category);
				parsed_category = TRUE;

				if (!parsed_description)
					state = ST_DESCRIPTION;
				else if (!parsed_outboard)
					state = ST_OUTBOARD;
				else if (!parsed_flags)
					state = ST_FLAGS;
				else
					state = ST_INPUT;
			}
			else
			{
				/*
				* No category specified; e.g., mark category field as such.
				*/
				module->setCategory(0);

				/*
				* Don't get another line yet...
				*/
				get_another_line = FALSE;
				if (!checked_description)
					state = ST_DESCRIPTION;
				else if (!checked_outboard)
					state = ST_OUTBOARD;
				else if (!checked_flags)
					state = ST_FLAGS;
				else
					state = ST_INPUT;
			}


			break;


		case ST_DESCRIPTION:

			checked_description = TRUE;

			/*
			* Parse "DESCRIPTION" keyword.
			*/
			if (IsToken(line, "DESCRIPTION", start))
			{
				/*
				* Skip space.
				*/
				SkipWhiteSpace(line,start);

				/*
				* Store away the description string.
				*/
				module->setDescription(&line[start]);
				parsed_description = TRUE;

				if (!parsed_category)
					state = ST_CATEGORY;
				else if (!parsed_outboard)
					state = ST_OUTBOARD;
				else if (!parsed_flags)
					state = ST_FLAGS;
				else
					state = ST_INPUT;
			}
			else
			{
				/*
				* Don't get another line yet...
				*/
				get_another_line = FALSE;
				if (!checked_category)
					state = ST_CATEGORY;
				else if (!checked_outboard)
					state = ST_OUTBOARD;
				else if (!checked_flags)
					state = ST_FLAGS;
				else
					state = ST_INPUT;
			}

			break;

		case ST_LOADABLE:

			checked_loadable = TRUE;
			/*
			* Parse "LOADABLE" keyword.
			*/
			if (IsToken(line, "LOADABLE", start)) 
			{
				/*
				* Skip space.
				*/
				SkipWhiteSpace(line,start);

				/*
				* Store away the description string.
				*/
				if (!_ParseLoadableLine(module, line, line_number, start))
					goto error;
				parsed_loadable = TRUE;

				if (!parsed_category)
					state = ST_CATEGORY;
				else if (!parsed_description)
					state = ST_DESCRIPTION;
				else if (!parsed_flags)
					state = ST_FLAGS;
				else
					state = ST_INPUT;
			}
			else
			{
				/*
				* Don't get another line yet...
				*/
				get_another_line = FALSE;

				if (!checked_category)
					state = ST_CATEGORY;
				else if (!checked_description)
					state = ST_DESCRIPTION;
				else if (!checked_flags)
					state = ST_FLAGS;
				else if (!checked_outboard)
					state = ST_OUTBOARD;
				else
					state = ST_INPUT;
			}
			break;

		case ST_OUTBOARD:

			checked_outboard = TRUE;
			/*
			* Parse "OUTBOARD" keyword.
			*/
			if (IsToken(line, "OUTBOARD", start)) 
			{
				/*
				* Skip space.
				*/
				SkipWhiteSpace(line,start);

				/*
				* Store away the description string.
				*/
				if (!_ParseOutboardLine(module, line, line_number, start))
					goto error;
				parsed_outboard = TRUE;

				if (!parsed_category)
					state = ST_CATEGORY;
				else if (!parsed_description)
					state = ST_DESCRIPTION;
				else if (!parsed_flags)
					state = ST_FLAGS;
				else
					state = ST_INPUT;
			}
			else
			{
				/*
				* Don't get another line yet...
				*/
				get_another_line = FALSE;

				if (!checked_loadable)
					state = ST_LOADABLE;
				else if (!checked_category)
					state = ST_CATEGORY;
				else if (!checked_description)
					state = ST_DESCRIPTION;
				else if (!checked_flags)
					state = ST_FLAGS;
				else
					state = ST_INPUT;
			}
			break;

		case ST_FLAGS:

			checked_flags = TRUE;
			/*
			* Parse "FLAGS" keyword.
			*/
			if (IsToken(line, "FLAGS", start)) {

				parsed_flags = TRUE;

				if (strstr(line,"SWITCH")) module->setMDFFlagSWITCH();
				if (strstr(line,"ERR_CONT")) module->setMDFFlagERR_CONT();
				if (strstr(line,"PIN")) module->setMDFFlagPIN();
				if (strstr(line,"SIDE_EFFECT")) module->setMDFFlagSIDE_EFFECT();
				if (strstr(line,"PERSISTENT")) module->setMDFFlagPERSISTENT();
				if (strstr(line,"ASYNC")) module->setMDFFlagASYNCHRONOUS();
				if (strstr(line,"REROUTABLE")) module->setMDFFlagREROUTABLE();
				if (strstr(line,"REACH")) module->setMDFFlagREACH();
				if (strstr(line,"LOOP")) module->setMDFFlagLOOP();

				if (!parsed_category)
					state = ST_CATEGORY;
				else if (!parsed_description)
					state = ST_DESCRIPTION;
				else if (!parsed_outboard)
					state = ST_OUTBOARD;
				else
					state = ST_INPUT;
			}
			else
			{
				/*
				* Don't get another line yet...
				*/
				get_another_line = FALSE;

				if (!checked_category)
					state = ST_CATEGORY;
				else if (!checked_description)
					state = ST_DESCRIPTION;
				else if (!checked_outboard)
					state = ST_OUTBOARD;
				else
					state = ST_INPUT;
			}
			break;

		case ST_INPUT:
			last_iostate = ST_INPUT;
			if (IsToken(line, "INPUT", start))
			{
				if (_ParseInputLine(mdf, module, line, line_number, start))
				{
					/*
					* Continue to parse input line.
					*/
					state = ST_INPUT;
				}
				else
				{
					goto error;
				}
			}
			else
			{
				/*
				* Don't get another line yet...
				*/
				get_another_line = FALSE;

				/*
				* If not successful, try parsing repeat line.
				*/
				if (checked_repeat)
					state = ST_OUTPUT;
				else
					state = ST_OPTIONS;

			}
			break;


		case ST_OUTPUT:
			last_iostate = ST_OUTPUT;
			if (IsToken(line, "OUTPUT", start))
			{
				if (_ParseOutputLine(mdf, module, line, line_number, start))
				{
					/*
					* Continue to parse output line.
					*/
					state = ST_OUTPUT;
				}
				else
				{
					goto error;
				}
			}
			else
			{
				/*
				* Don't get another line yet...
				*/
				get_another_line = FALSE;

				/*
				* If not successful, assume end of module definition...
				*/
				state = ST_REPEAT;
			}
			break;

		case ST_REPEAT: 
			checked_repeat = TRUE;
			if (IsToken(line, "REPEAT", start))
			{
				if (_ParseRepeatLine(mdf, module, line, line_number, 
					start, last_iostate))
				{
					/*
					* Next, parse output or module line.
					*/
					if (last_iostate == ST_INPUT)  {
						state = ST_OUTPUT;
					} else if (last_iostate == ST_OUTPUT)  {
						state = ST_PACKAGE;
					} else
						ASSERT(0);
					/*parsed_repeat = TRUE;*/
					get_another_line = TRUE;
				}
				else
				{
					goto error;
				}
			}
			else
			{
				/*
				* Don't get another line yet...
				*/
				get_another_line = FALSE;

				/*
				* If not successful, try parsing output or module line.
				*/
				if (last_iostate == ST_INPUT) {
					state = ST_INPUT;
				} else if (last_iostate == ST_OUTPUT) { 
					state = ST_PACKAGE;
				} else 
					ASSERT(0); 
			}
			break;

		case ST_OPTIONS: 
			ASSERT(last_iostate == ST_INPUT);
			if (IsToken(line, "OPTIONS", start))
			{
				if (_ParseOptionsLine(mdf, module, line, line_number, start))
				{
					/*
					* Next, parse another line of selections
					*/
					state = ST_OPTIONS;
					get_another_line = TRUE;
				}
				else
				{
					goto error;
				}
			}
			else
			{
				/*
				* Don't get another line yet...
				*/
				get_another_line = FALSE;

				/*
				* If not successful, try parsing output or module line.
				*/
				if (!checked_repeat)
					state = ST_REPEAT;
				else
					state = ST_INPUT;
			}
			break;


		default:
			ASSERT(FALSE);
		}
	}

	/*
	* Finish the module definition.
	*/
	if (module)
	{
		if (!_FinishNodeDefinition(mdf, module))
			delete module;
	}

	return TRUE;

error:
	if (module) delete module;

	return FALSE;
}

boolean LoadMDFFile(const char *file, const char *mdftype, Dictionary *mdf,
					boolean uionly)
{
	FILE *input;
	boolean parsed;

	if (!file || !*file)
		return TRUE;

	input = fopen(file, "r");
	if (input != NUL(FILE*))
	{
		parsed = ReadMDF(mdf, input, uionly);
		fclose(input);	    

		if (NOT parsed)
		{
			ErrorMessage("Error found in %s module definition file \"%s\".",
				mdftype,file);
			return FALSE;
		}
	}
	else
	{
		ErrorMessage ("Cannot open %s module description file \"%s\".", 
			mdftype,file);
		return FALSE;
	}
	return TRUE;
}

#ifdef NOT_YET
boolean ReadMDFFiles(const char *root, Dictionary *mdf)
{

    FILE*   input;
    boolean parsed;
    char    pathname[256];


    /*
     * Load system module description file.
     */
    sprintf(pathname, "%s/lib/dx.mdf", root);

    if (!LoadMDFFile(pathname,"system",mdf))
	return FALSE;

    /*
     * Load UI module description file.
     */
    sprintf(pathname, "%s/ui/ui.mdf", root);

    if (!LoadMDFFile(pathname,"UI",mdf))
	return FALSE;

    /*
     * Load user module description file, if defined.
     */
    if (program->user_module)
    {
	input = fopen(program->user_module, "r");
	if (input != NUL(FILE*))
	{
	    parsed = ReadMDF(program->mdf, input);
	    fclose(input);	    

	    if (NOT parsed)
	    {
		ErrorMessage
		    ("Error found in user module definition file \"%s\".",
		     program->user_module);
		return FALSE;
	    }
	}
	else
	{
	    ErrorMessage
		("Cannot open user module description file \"%s\".",
		 program->user_module);
	    return FALSE;
	}
    }

    /*
     * Need to initialize MDF after all the module description files have
     * been parsed, and all the macro definitions have been added.
     */
    InitializeMDF(program->mdf);

    /*
     * Install the macros.
     */
    if (program->macros)
    {
	uipInstallMacros(program);
    }

    /*
     * Return successfully.
     */
    return TRUE;
}
#endif // NOT_YET

#ifdef NOT_YET
/*****************************************************************************/
/* _CompareModules -							     */
/*                                                                           */
/* Compares two module name strings.					     */
/*                                                                           */
/*****************************************************************************/

static
int _CompareModules(int* first,
		       int* second)
{
    ASSERT(first);
    ASSERT(second);

    return
	uiuStrcmp(_mdf->module[*first].function, _mdf->module[*second].function);
}


/*****************************************************************************/
/* SortMDF -								     */
/*                                                                           */
/* Sorts an MDF list by name.                                                */
/*                                                                           */
/*****************************************************************************/

void SortMDF(Dictionary* mdf)
{
    ASSERT(mdf != NUL(Dictionary*));

    /*
     * Sort the module list in alphabetical order.
     */
    _mdf = mdf;
    qsort(mdf->module_index, mdf->n_modules, sizeof(int), _CompareModules);
}



/*****************************************************************************/
/* InitializeMDF -							     */
/*                                                                           */
/* Initializes an MDF after all the various module description files have    */
/* been read and parsed.						     */
/*                                                                           */
/*****************************************************************************/

void InitializeMDF(UimMDF* mdf)
{
    UimModule* module;
    int        i;
    int        k;

    ASSERT(mdf);

    /*
     * Sort the module list in alphabetical order.
     */
    _mdf = mdf;
    qsort(mdf->module_index, mdf->n_modules, sizeof(int), _CompareModules);

    /*
     * Save special UI module names and categories (indices).
     */
    uiuStringTableFind(mdf->name_table, "Integer",     &mdf->name.integer);
    uiuStringTableFind(mdf->name_table, "Scalar",      &mdf->name.scalar);
    uiuStringTableFind(mdf->name_table, "Vector",      &mdf->name.vector);
    uiuStringTableFind(mdf->name_table, "Value",       &mdf->name.value);
    uiuStringTableFind(mdf->name_table, "String",      &mdf->name.string);
    uiuStringTableFind(mdf->name_table, "Selector",    &mdf->name.selector);
    uiuStringTableFind(mdf->name_table, "IntegerList", &mdf->name.integer_list);
    uiuStringTableFind(mdf->name_table, "ScalarList",  &mdf->name.scalar_list);
    uiuStringTableFind(mdf->name_table, "VectorList",  &mdf->name.vector_list);
    uiuStringTableFind(mdf->name_table, "ValueList",   &mdf->name.value_list);
    uiuStringTableFind(mdf->name_table, "StringList",  &mdf->name.string_list);

    uiuStringTableFind
	(mdf->category_table, "Interactor", &mdf->name.interactors);

    uiuStringTableFind(mdf->name_table, "Colormap",    &mdf->name.colormap);
    uiuStringTableFind(mdf->name_table, "Image",       &mdf->name.image);
    uiuStringTableFind(mdf->name_table, "Input",       &mdf->name.input);
    uiuStringTableFind(mdf->name_table, "Output",      &mdf->name.output);
    uiuStringTableFind(mdf->name_table, "Receiver",    &mdf->name.receiver);
    uiuStringTableFind(mdf->name_table, "Sequencer",   &mdf->name.sequencer);
    uiuStringTableFind(mdf->name_table, "Transmitter", &mdf->name.transmitter);
    uiuStringTableFind(mdf->name_table, "VCR",         &mdf->name.vcr);

    uiuStringTableFind(mdf->name_table, "Display",     &mdf->name.display);
    uiuStringTableFind(mdf->name_table, "ProbeList",   &mdf->name.probe_list);
    uiuStringTableFind(mdf->name_table, "Probe",       &mdf->name.probe);

#ifdef EPIC
    uiuStringTableFind(mdf->name_table, "Epic", &mdf->name.epic);
#else
    mdf->name.epic = -1;
#endif

#ifdef CAD
    uiuStringTableFind(mdf->name_table, "Navigate", &mdf->name.navigate);
#else
    mdf->name.navigate = -1;
#endif

    /*
     * Adjust certain parameters of various modules to be invisible.
     */
    FOR_EACH_MODULE(module, i, mdf)
    {
	if (module->dialog_type == CONF_DIALOG_COMPUTE)
	{
	    module->input[0].visible = FALSE;
	}
	else if (module->name == mdf->name.colormap)
	{
	    module->input[0].visible = FALSE;
	    module->input[1].visible = FALSE;
	    module->input[2].visible = FALSE;
	    module->input[3].visible = FALSE;
	}
	else if (module->name == mdf->name.image)
	{
	    /*
	     * Make every parameter, except the second one, invisible.
	     */
	    for (k = 0; k < module->n_inputs; k++)
	    {
		if (k != 1)
		{
		    module->input[k].visible = FALSE;
		}
	    }
	}
	else if (module->name == mdf->name.navigate)
	{
	    /*
	     * Make most parameters invisible.
	     */
	    for (k = 0; k < module->n_inputs; k++)
	    {
		if (k != 1 AND k != 29 AND k != 30)
		{
		    module->input[k].visible = FALSE;
		}
	    }
	}
    }
}
#endif // NOT_YET

#ifdef COMMENT

/*****************************************************************************/
/* _uimPrintMDF -							     */
/*                                                                           */
/* Prints binary form of module descriptions in ASCII.  prints only one      */
/* module name, if supplied; otherwise prints the entire table of modules.   */
/*                                                                           */
/*****************************************************************************/

static
void _uimPrintMDF(UimMDF* mdf,
		  FILE*   output)
{
    UimModule* module;
    UimParam*  parameter;
    int	       i;
    int        j;
    int        k;

    ASSERT(mdf);
    ASSERT(output);

    FOR_EACH_MODULE(module, i, mdf)
    {
	fprintf(output,	"MODULE %s\n",module->function);

	fprintf(output,
		"CATEGORY %s\n",
	        uiuStringTableGet(mdf->category_table, module->category));

	if (module->description)
	{
	    fprintf(output, "DESCRIPTION %s\n", module->description);
	}

	FOR_EACH_MODULE_INPUT(parameter, j, module)
	{
	    fprintf(output,
		    "INPUT %s; ",
		    uiuStringTableGet(mdf->label_table, parameter->label));
		    
	    for (k = 0; k < parameter->type_count; k++)
	    {
		fputs(uimTypeToString(module->type[parameter->type + k]),
		      output);

		if (k < parameter->type_count - 1)
		{
		    fputs(" or ", output);
		}
	    }
	    
	    if (parameter->default_value)
	    {
		fprintf(output, "; %s", parameter->default_value);
	    }

	    if (parameter->description)
	    {
		fprintf(output, "; %s", parameter->description);
	    }

	    fputc('\n', output);
	}

	if (module->n_repeats)
	{
	    fprintf(output, "REPEAT %d\n", module->n_repeats);
	}

	FOR_EACH_MODULE_OUTPUT(parameter, j, module)
	{
	    fprintf(output,
		    "OUTPUT %s; ",
		    uiuStringTableGet(mdf->label_table, parameter->label));
		    
	    for (k = 0; k < parameter->type_count; k++)
	    {
		fputs(uimTypeToString(module->type[parameter->type + k]),
		      output);

		if (k < parameter->type_count - 1)
		{
		    fputs(" or ", output);
		}
	    }
	    
	    if (parameter->description)
	    {
		fprintf(output, "; %s", parameter->description);
	    }

	    fputc('\n', output);
	}

	fputc('\n', output);
    }
}

#endif
