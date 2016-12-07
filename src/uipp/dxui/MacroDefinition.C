/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"

#include <sys/types.h>

#if defined(HAVE_ERRNO_H)
#include <errno.h>
#endif

#if defined(HAVE_DIRENT_H)
#include <dirent.h>
#endif

#if defined(HAVE_IO_H)
#include <io.h>
#endif

#if defined(HAVE_SYS_STAT_H)
#include <sys/stat.h>
#endif

#include <stdio.h>

#define Ark Ark
#include <dx/arch.h>
#undef Ark

#include "lex.h"
#include "MacroDefinition.h"
#include "MacroNode.h"
#include "ListIterator.h"
#include "DXApplication.h"
#include "Network.h"
#include "ErrorDialogManager.h"
#include "WarningDialogManager.h"
#include "ParseMDF.h"
#include "ParameterDefinition.h"
#include "ToolSelector.h"
#include "EditorWindow.h"
#include "SaveMacroCommand.h"

#define OLD_DUMMY_DESCRIPTION_STRING "Generated dummy input"

#if HAVE_REGCOMP && HAVE_REGEX_H
/* prefer POSIX style regcomp(3) over obsolete versions */
extern "C" {
#include <regex.h>
}
#undef HAVE_RE_COMP
#undef HAVE_FINDFIRST
#elif  defined(HAVE_RE_COMP)
#undef HAVE_REGCMP
#undef HAVE_REGCOMP
#undef HAVE__FINDFIRST
extern "C" char *re_comp(char *s);
extern "C" int re_exec(char *);
#elif defined(HAVE_REGCMP)
#undef HAVE_REGCOMP
#undef HAVE__FINDFIRST
extern "C" char *regcmp(...);
extern "C" char *regex(char *, char *, ...);
#elif HAVE_REGCOMP && HAVE_REGEXP_H
extern "C" {
#include <regexp.h>
}
#undef HAVE_RE_COMP
#undef HAVE__FINDFIRST
#elif  defined(HAVE_RE_COMP)
#undef HAVE__FINDFIRST
extern "C" char *re_comp(char *s);
extern "C" int re_exec(char *);
#endif

MacroDefinition::MacroDefinition(boolean system) : 
    NodeDefinition()
{
    this->systemMacro = system;
    this->fileName = NULL;
    this->body = NULL;
    this->initialRead = FALSE;
    this->updatingServer = FALSE;
    this->category = 0;
    if (!system)
	this->saveCmd = new SaveMacroCommand("saveMacroCommand",
                                         theDXApplication->getCommandScope(),
                                         TRUE,
                                         this);
}
MacroDefinition::~MacroDefinition()
{
    if (this->saveCmd) delete this->saveCmd;

    if (this->body) 
    {
	this->body->setDefinition(NULL);
	theDXApplication->macroList.removeElement(this->body);
	if (!this->body->isDeleted())
	    delete this->body;
	this->body = NULL;
    }
    if (this->fileName)
	delete[] this->fileName;
}

void MacroDefinition::finishDefinition()
{
}


Node *MacroDefinition::newNode(Network *net, int instance)
{
    MacroNode *d = new MacroNode(this, net, instance);
    return d;
}

void MacroDefinition::reference(MacroNode *n)
{
    ListIterator li(this->referencingNodes);
    Node *listNode;
    while ( (listNode = (MacroNode*)li.getNext()) )
	if (n == listNode)
	    return;
    this->referencingNodes.appendElement(n);
}

void MacroDefinition::dereference(MacroNode *n)
{
    ListIterator li(this->referencingNodes);
    Node *listNode;
    while ( (listNode = (MacroNode*)li.getNext()) )
    {
	if (n == listNode)
	{
	    this->referencingNodes.deleteElement(li.getPosition()-1);
	    break;
	}
    }
//    if (this->referencingNodes.getSize() == 0 && this->body != NULL)
//    {
//	theDXApplication->macroList.removeElement(this->body);
//	delete this->body;
//	this->body = NULL;
//    }
}

boolean MacroDefinition::printNetworkBody(FILE *f, PrintType ptype)
{
    if (!this->loadNetworkBody())
	return FALSE;
    return this->body->printNetwork(f,ptype);
}
boolean MacroDefinition::loadNetworkBody()
{
    if (this->body == NULL)
    {
	this->body = theDXApplication->newNetwork();
	this->body->setDefinition(this);
	this->initialRead = TRUE;
	boolean r = this->body->readNetwork(this->fileName);
	this->initialRead = FALSE;
        if (!r) {
            this->body->setDefinition(NULL);
            Network *n = this->body;
            this->body = NULL;
            delete n;
	    return FALSE;
        } else {
            theDXApplication->macroList.appendElement(this->body);
        }
    }
    return TRUE;
}
boolean MacroDefinition::updateServer()
{
    if (!this->loadNetworkBody())
	return FALSE;

    if (this->body && !this->updatingServer) {
	//
	// There is a recursive loop between DXExecCtl::updateMacros(),
	// Network::sendValues(), MacroNode::sendValues() and 
 	// this->updateServer() that we try and avoid here.   
	//
	this->updatingServer = TRUE;
	theDXApplication->getExecCtl()->updateMacros(FALSE);
	this->updatingServer = FALSE;
    }
    return (this->body != NULL);
}

boolean MacroDefinition::setNodeDefinitions(MacroDefinition *newDef)
{
    ListIterator li(this->referencingNodes);
    MacroNode *n;

    while ( (n = (MacroNode*)li.getNext()) )
    {
	n->setDefinition(newDef);
	n->updateDefinition();
	if (newDef != this)
	    newDef->reference(n);
    }

    return TRUE;
}

void MacroDefinition::setFileName(const char *n)
{
    char *fileName = DuplicateString(n);
    if (this->fileName)
	delete[] this->fileName;
    this->fileName = fileName;
}

boolean MacroDefinition::LoadMacroFile(FILE *f,
				       const char *fileName,
				       boolean replace,
				       boolean *wasMacro,
					boolean asSystemMacro)
{

    if (theDXApplication->inDebugMode())
	printf("read macro from %s\n", fileName);

    char *p, line[1000];
    boolean inMDF = FALSE;
    int lineNo = 0;
    MacroDefinition *md = NULL;
    NodeDefinition *nd = NULL;
    MacroDefinition *oldMd = NULL;
    SymbolManager  *symbolManager = theNodeDefinitionDictionary->
					getSymbolManager();
    // statefull-ness.  Remember which INPUT was added most recently so
    // that the OPTIONS keyword can associate with it.  Ensure the
    // most_recent_param is set to NULL if INPUT isn't followed
    // by an OPTIONS keyword.
    ParameterDefinition* most_recent_param = NULL;

#define EQUAL_STRING_SECOND_LEN(s1,s2) (EqualSubstring((s1), (s2), STRLEN(s2)))
    while(fgets(line, sizeof(line), f) == line)
    {
	// Convert possible dos-land carriage returns (ctrl-M) to white space
	int len = STRLEN(line);
	if (len > 1 && line[len-2] == '\15')
	{
	    line[len-2] = line[len-1];
	    line[len-1] = '\0';
	    len--;
	}
	lineNo++;
	if (EQUAL_STRING_SECOND_LEN(line, "// Begin MDF"))
	{
	    inMDF = TRUE;
	}
	else if (inMDF && EQUAL_STRING_SECOND_LEN(line, "// End MDF"))
	{
	    inMDF = FALSE;
	    break;
	}
	else if (!inMDF && EQUAL_STRING_SECOND_LEN(line, "// MODULE"))
	{
	    if (theDXApplication->inDebugMode())
		printf("Macro rejected\n");
	    if (wasMacro)
		*wasMacro = FALSE;
	    goto error;
	}
	else if (inMDF && EQUAL_STRING_SECOND_LEN(line, "// MODULE"))
	{
	    if (wasMacro)
		*wasMacro = TRUE;

	    char name[1000];
	    int items_parsed = sscanf(line, "// MODULE %[^\n]", name);
	    if (items_parsed != 1)
	    {
		ErrorMessage("Invalid MODULE comment at line %d of %s.", 
		    lineNo, fileName);
		goto error;
	    }

	    int index = 0;
	    if (!IsRestrictedIdentifier(name, index) ||
		(index != STRLEN(name) && !IsWhiteSpace(name, index)))
	    {
		ErrorMessage("Invalid macro name: %s must start with a letter "
			     "and contain only letters and numbers "
			     "at line %d of %s.", 
		    name, lineNo, fileName);
		goto error;
	    }

	    if (EqualString("main", name))
	    {
		ErrorMessage("The macro name must not be \"main\" "
			     "at line %d of %s.", lineNo, fileName);
		goto error;
	    }

	    nd =
		(NodeDefinition*)theNodeDefinitionDictionary->
		    findDefinition(name);
	    if (nd)
	    {
		if (!nd->isDerivedFromMacro())
		{
		    ErrorMessage("Standard module \"%s\" cannot be redefined.\n"
				 "Macro file \"%s\" not loaded.",
			name, fileName);
		    goto error;
		}
		else if (!replace)
		{
//		    WarningMessage("Macro \"%s\" is already defined.\n"
//				   "Macro file \"%s\" not loaded.",
//			name, fileName);
		    goto error;
		}
		oldMd = (MacroDefinition*)nd;
	    }
	    md = new MacroDefinition(asSystemMacro);
	    md->setName(name);
	    md->setFileName(fileName);
	    if (nd)
		md->setNextInstance(nd->newInstanceNumber());
	}
	else if (inMDF && EQUAL_STRING_SECOND_LEN(line, "// CATEGORY"))
	{
	    char cat[1000];
	    int items_parsed = sscanf(line, "// CATEGORY %[^\n]", cat);
	    if (items_parsed != 1)
	    {
		ErrorMessage("Invalid CATEGORY comment at line %d of %s.", 
		    lineNo, fileName);
		goto error;
	    }
	    md->setCategory(symbolManager->registerSymbol(cat));
	}
	else if (inMDF && EQUAL_STRING_SECOND_LEN(line, "// DESCRIPTION"))
	{
	    char desc[1000];
	    int items_parsed = sscanf(line, "// DESCRIPTION %[^\n]", desc);
	    if (items_parsed != 1)
	    {
		ErrorMessage("Invalid DESCRIPTION comment at line %d of %s.", 
		    lineNo, fileName);
		goto error;
	    }
	    md->setDescription(desc);
	}
	else if (inMDF && EQUAL_STRING_SECOND_LEN(line, "// INPUT"))
	{
	    char name[1000];
	    char types[1000];
	    char deflt[1000];
	    char descr[1000];
	    int items_parsed = sscanf(line,
		    "// INPUT %[^;]; %[^;]; %[^;]; %[^\n]",
		    name, types, deflt, descr);
	    if (items_parsed != 3 && items_parsed != 4)
	    {
		ErrorMessage("Invalid INPUT comment at line %d of %s.", 
		    lineNo, fileName);
		goto error;
	    }

	    int index = 0;
	    // ... to avoid millions of purify umrs
	    int len = STRLEN(name);
	    name[len] = name[len+1] = '\0';
	    if (!IsIdentifier(name, index) ||
		(index != STRLEN(name) && !IsWhiteSpace(name, index) &&
			name[index] != '['))
	    {
		ErrorMessage("Invalid parameter name at line %d of %s.", 
		    lineNo, fileName);
		goto error;
	    }
	    name[index++] = '\0';
  	    int visattr = 1;
	    if ( (p = strstr(&name[index],"visible:")) ) {
		p += strlen("visible:");
		visattr = atoi(p);
	    } 
		
	    ParameterDefinition *pd = new 
		ParameterDefinition(symbolManager->registerSymbol(name));
	    if (items_parsed == 4) {
		if (strstr(descr,DUMMY_DESCRIPTION_STRING) ||
		    strstr(descr,OLD_DUMMY_DESCRIPTION_STRING))
		    pd->setDummy(TRUE);
		else
		    pd->setDescription(descr);
	    }
	    switch (visattr) {
	  	case 0:	pd->setDefaultVisibility(FALSE); break;
	  	case 1:	pd->setDefaultVisibility(TRUE); break;
	  	case 2:	pd->setViewability(FALSE); break;
	    }
	    pd->markAsInput();
	    if (!ParseMDFTypes(pd, types, lineNo))
	    {
		delete pd;
		goto error;
	    }
	    
	    if (*deflt == '(')
	    {
		pd->setDescriptiveValue(deflt);
		if (EqualString(deflt,"(none)"))
		    pd->setRequired();
	    }
	    else if (!pd->setDefaultValue(deflt))
	    {
		ErrorMessage(
		    "Invalid default parameter value at line %d of %s.", 
		    lineNo, fileName);

		delete pd;
		pd = NULL;
		goto error;
	    }
	    md->addInput(pd);
	    most_recent_param = pd;
	}
	else if (most_recent_param &&  EQUAL_STRING_SECOND_LEN(line, "// OPTIONS"))
	{
	    // must convert trailing \n to whitespace because 
	    // ParseMDFOptions expects it that way.
	    int len = STRLEN(line);
	    if (line[len-1] == '\n') line[len-1] = '\0';
	    if (!ParseMDFOptions (most_recent_param, &line[11])) {
		ErrorMessage(
		    "Invalid parameter options values at line %d of %s.", 
		    lineNo, fileName);
	    }
	}
	else if (inMDF && EQUAL_STRING_SECOND_LEN(line, "// OUTPUT"))
	{
	    char name[1000];
	    char types[1000];
	    char descr[1000];
	    int items_parsed = sscanf(line,
		    "// OUTPUT %[^;]; %[^;]; %[^\n]",
		    name, types, descr);
	    if (items_parsed != 2 && items_parsed != 3)
	    {
		ErrorMessage("Invalid OUTPUT comment at line %d of %s.", 
		    lineNo, fileName);
		goto error;
	    }

	    int index = 0;
	    // ... to avoid millions of purify umrs
	    int len = STRLEN(name);
	    name[len] = name[len+1] = '\0';
	    if (!IsIdentifier(name, index) ||
		(index != STRLEN(name) && !IsWhiteSpace(name, index) &&
			name[index] != '['))
	    {
		ErrorMessage("Invalid parameter name at line %d of %s.", 
		    lineNo, fileName);
		goto error;
	    }

	    name[index++]='\0';
  	    int visattr = 1;
	    if ( (p = strstr(&name[index],"visible:")) ) {
		p += strlen("visible:");
		visattr = atoi(p);
	    } 

	    ParameterDefinition *pd = new 
		ParameterDefinition(symbolManager->registerSymbol(name));
	    if (items_parsed == 3) {
		if (strstr(descr,DUMMY_DESCRIPTION_STRING) ||
		    strstr(descr,OLD_DUMMY_DESCRIPTION_STRING))
		    pd->setDummy(TRUE);
		else
		    pd->setDescription(descr);
	    }
	    switch (visattr) {
	  	case 0:	pd->setDefaultVisibility(FALSE); break;
	  	case 1:	pd->setDefaultVisibility(TRUE); break;
	  	case 2:	pd->setViewability(FALSE); break;
	    }
	    pd->markAsOutput();
	    if (!ParseMDFTypes(pd, types, lineNo))
	    {
		delete pd;
		goto error;
	    }
	    
	    md->addOutput(pd);
	}
	else if (inMDF)
	{
	    WarningMessage("Encountered unrecognized MDF line "
			   "at line %d of `%s', ignored.",
		lineNo, fileName);
	}
	if (!EQUAL_STRING_SECOND_LEN(line, "// INPUT")) {
	    most_recent_param = NULL;
	}
    }

    if (!md)
	goto error;

    md->completeDefinition();

    if (!asSystemMacro) {
	if (oldMd)
	{
	    oldMd->setNodeDefinitions(md);
	    ToolSelector::RemoveTool(oldMd->getCategorySymbol(),
				     oldMd->getNameSymbol());
	    //
	    // Check if the macro is changed.
	    //
	    if (oldMd->body &&oldMd->saveCmd && 
		oldMd->body->saveToFileRequired())
	    {
		((SaveMacroCommand*)oldMd->saveCmd)->setNext(NULL);
		oldMd->saveCmd->execute();
	    }
	    else
		delete oldMd;
	}
    }

    theNodeDefinitionDictionary->replaceDefinition(md->getNameString(), md);

    if (!asSystemMacro) {
	ToolSelector::AddTool(md->getCategorySymbol(),
			      md->getNameSymbol(),
			      (void *)md);
	ToolSelector::UpdateCategoryListWidget();
    }


    if (theDXApplication->inDebugMode())
	printf("Macro %s accepted\n", md->getNameString());

    return TRUE;

error:
    if (md)
	delete md;
    return FALSE;
}

//
// Load all .net files in the given directory that are macros.
// If replace is TRUE, then replace any current definitions with the
// new one, otherwise ignore the .net file.
// If errmsg is not NULL and an error occurs then, no error messages are 
// posted, and instead a string buffer is allocated to hold the error 
// message that would have been posted and returned.  The returned 
// string must be freed by the caller.
//
boolean MacroDefinition::LoadMacroDirectories(const char *path, 
					boolean replace, char **errmsg,
					boolean asSystemMacro)
{
   boolean wasEncoded;
   boolean return_code = TRUE;

    if (path == NULL)
	return TRUE;

#ifndef DXD_NON_UNIX_ENV_SEPARATOR
#define SEP_CHAR ':'
#else
#define SEP_CHAR ';'
#endif

    char *originalString = DuplicateString(path);
    char *sptr = originalString;

    if (errmsg)
	*errmsg = NULL;

    while(sptr)
    {
	char *nsptr = sptr;
	char *sep = strchr(nsptr, SEP_CHAR);
	if (sep)
	{
	    *sep = '\0';
	    sptr = sep + 1;
	}
	else
	{
	    sptr = NULL;
	}

#if defined(HAVE_OPENDIR) && defined(HAVE_DIRENT_H)
        DIR* d = opendir(nsptr);
	if (!d)
#elif defined(HAVE_SYS_STAT_H)
	struct STATSTRUCT b;
	int d = STATFUNC(nsptr, &b);
	if (d == -1)
#else
	No directory tools?
#endif

	{
 	    char *errtxt = "Failed opening directory %s: %s";
	    if (errmsg) {
		int size = STRLEN(errtxt) + STRLEN(nsptr) + 256;
		char *p;
		if (!*errmsg) {
		    *errmsg = (char*)MALLOC(size);
		    p = *errmsg;
		} else {
		    int errmsg_size = STRLEN(*errmsg);
		    *errmsg = (char*)REALLOC(*errmsg,errmsg_size + size + 2);
		    p = *errmsg + errmsg_size; 
		    *p = '\n';
		    p++;
		}
		sprintf(p,errtxt,nsptr, strerror(errno));
	    } else {
		ErrorMessage(errtxt, nsptr, strerror(errno));
	    }
	    return_code = FALSE;
	}
	else
	{
#if defined(HAVE_REGCOMP) && defined(HAVE_REGEX_H)

	    regex_t net_file;
	    ASSERT(regcomp(&net_file, ".[.]*\\.net$", REG_NOSUB) == 0);

	    struct dirent *entry;
	    while ( (entry = readdir(d)) )
	    {
	        boolean exists = regexec(&net_file, entry->d_name, 0, NULL, 0);
		if (exists == 0)

#elif defined(HAVE_REGCOMP) && defined(HAVE_REGEXP_H)

	    char *net_file = (char *)regcomp(".[.]*\\.net$");
	    ASSERT(net_file != NULL);

	    struct dirent *entry;
	    while (entry = readdir(d))
	    {
		boolean exists = regexec((regexp *)net_file, entry->d_name);
		if (exists)

#elif defined(HAVE_REGCMP)

	    char *net_file = regcmp(".[.]*\\.net$", NULL);
	    ASSERT(net_file != NULL);

	    struct dirent *entry;
	    while (entry = readdir(d))
	    {
		boolean exists = regex(net_file, entry->d_name) != NULL;
		if (exists)

#elif defined(HAVE_RE_COMP)

	    char *net_file = re_comp(".[.]*\\.net$");
	    ASSERT(net_file == NULL);

	    struct dirent *entry;
	    while (entry = readdir(d))
	    {
		boolean exists = re_exec(entry->d_name) > 0;
		if (exists)

#elif defined(HAVE__FINDFIRST)

	    char *srch_string2 = new char[STRLEN(nsptr) + 10];  
	    strcpy(srch_string2,nsptr);                        
	    if (strlen(srch_string2)>0) {
		char c = srch_string2[strlen(srch_string2)-1];
		if (c != '/' && c != '\\' && c != ':')
		    strcat(srch_string2, "/");                   
	    }
	    strcat(srch_string2, "*.net");

            struct _finddata_t entry;
            long handle = _findfirst(srch_string2,&entry);
            int exists = (handle == -1L) ? -1: 0;
	    while (exists == 0)
#endif       
	    {
		    char path[1000];
		    strcpy(path, nsptr);
		    strcat(path, "/");

#if defined(HAVE__FINDFIRST)
                    strcat(path, entry.name);
#else
                    strcat(path, entry->d_name);
#endif

		    char *ignore = NULL;
		    FILE *f = Network::OpenNetworkFILE(path, &wasEncoded, &ignore); 
		    if (ignore) delete[] ignore;
		    if (f == NULL)
		    {
			char *errtxt = "Failed to load macro file %s: %s";
			if (errmsg) {
			    int size = STRLEN(errtxt) + STRLEN(path) + 256;
			    char *p;
			    if (!*errmsg) {
				*errmsg = (char*)MALLOC(size);
				p = *errmsg;
			    } else {
				int errmsg_size = STRLEN(*errmsg);
				*errmsg = (char*)REALLOC(*errmsg,
							errmsg_size + size + 2);
				p = *errmsg + errmsg_size; 
				*p = '\n';
				p++;
			    }
			    sprintf(p,errtxt,path, strerror(errno));
			} else {
			    ErrorMessage(errtxt, path, strerror(errno));
	    		}
			return_code = FALSE;
		    }
		    else
		    {
			MacroDefinition::LoadMacroFile(f, path, 
					replace, NULL, asSystemMacro);
			Network::CloseNetworkFILE(f, wasEncoded);
		    }
#if defined(HAVE__FINDFIRST)
                    exists=_findnext(handle,&entry);
	    }
            _findclose(handle);
	    delete[] srch_string2;
#elif defined(HAVE_REGCOMP) && defined(HAVE_REGEX_H)
		}
	    }
	    closedir(d);
	    regfree(&net_file);
#elif defined(HAVE_REGCOMP) && defined(HAVE_REGEXP_H)
		}
	    }
	    closedir(d);
#elif defined(HAVE_RE_COMP)
		}
	    }
	    closedir(d);
#elif defined(HAVE_REGCMP)
		}
	    }
	    closedir(d);
#elif defined(HAVE_REGCMP)
		}
	    }
	    free(net_file);
	    closedir(d);
#endif
	}
    }

    delete[] originalString;
    return return_code;
}

//
// If errmsg is not NULL and an error occurs then, no error messages are 
// posted, and instead a string buffer is allocated to hold the error 
// message that would have been posted and returned.  The returned 
// string must be freed by the caller.
//
boolean MacroDefinition::LoadMacro(const char *fileName, char **errmsg,
					boolean asSystemMacro)
{
    char *netFile = Network::FilenameToNetname(fileName);
    boolean return_code = TRUE;
    boolean wasEncoded;

    if (errmsg)	
	*errmsg = NULL;

    FILE *f = Network::OpenNetworkFILE(netFile, &wasEncoded, errmsg); 

 
    if (f == NULL) {
	return_code = FALSE;
    } else {
	boolean wasMacro;
	MacroDefinition::LoadMacroFile(f, netFile, TRUE, 
						&wasMacro, asSystemMacro);
	Network::CloseNetworkFILE(f, wasEncoded);
	if (!wasMacro) {
	    char *errtxt = "File %s doesn't contain a macro and was not loaded";
	    if (errmsg) {
		*errmsg = new char[STRLEN(errtxt) + STRLEN(netFile)];
		sprintf(*errmsg,errtxt,netFile);
	    } else {
		ErrorMessage(errtxt, netFile);
	    }
	    return_code = FALSE;
	}
    }
    delete netFile;

    return return_code;
}

void MacroDefinition::openMacro()
{
    if (this->updateServer()) {
	EditorWindow *e = this->body->getEditor();
	if (e == NULL)
	    e = theDXApplication->newNetworkEditor(this->body);
	if (e) {
	    e->manage();
	    XMapRaised(XtDisplay(e->getRootWidget()), 
			XtWindow(e->getRootWidget()));
	}
    }
}

boolean MacroDefinition::removeIODef(List *l, ParameterDefinition *pd)
{
    boolean result = l->removeElement((void*)pd);
    if (!this->initialRead)
	this->setNodeDefinitions(this);
    return result;
}
boolean MacroDefinition::addIODef(List *l, ParameterDefinition *pd)
{
    boolean result = TRUE;
    if (!this->initialRead) {
        result = this->NodeDefinition::addIODef(l, pd) &&
		 this->setNodeDefinitions(this);
    }
    return result;
}
boolean MacroDefinition::replaceIODef(List *l,
				      ParameterDefinition *newPd,
				      ParameterDefinition *pd)
{
    int position = l->getPosition((void*)pd);
    if (position == 0)
	return FALSE;
    boolean result = l->removeElement((void*)pd);
    if (result)
    {
	result = l->insertElement((void*)newPd, position);
    }

    int  i;
    //
    // Remove trailing DUMMY parameters.
    //
    if (result) {
	for (i = l->getSize(); (i > 0); i--)
	{
	    pd = (ParameterDefinition*)l->getElement(i);
	    if (pd->isDummy()) {
		if (l->removeElement((void*)pd))
		    delete pd;
	    } else
		break;
	}
    }
    if (!this->initialRead)
	this->setNodeDefinitions(this);

    return result;
}

//
// Get the Nth input that is not a dummy parameter definition.
//
ParameterDefinition *MacroDefinition::getNonDummyIODefinition(List *l, int n)
{
   ASSERT(n > 0);
   ASSERT(l);

   ParameterDefinition *pd=NULL;
   ListIterator iterator(*l);
   int count = 0;

   while ((count != n) &&
	  (pd = (ParameterDefinition*)iterator.getNext())) {
	if (!pd->isDummy())
	    count++;
   }
   if (count != n)
	pd = NULL;

   return pd;
}
//
// Find the first available spot to place a new parameter in the given
// list (expected to be either inputDefs or outputDefs). If there are dummy
// parameters in the list, then the index of the first dummy is returned.
// If no dummies, then N+1 is returned, where N is the current number of
// items in the list.
//
int MacroDefinition::getFirstAvailableIOPosition(List *l)
{
   ASSERT(l);

   ParameterDefinition *pd;
   ListIterator iterator(*l);
   int n = 0;
   boolean found_dummy = FALSE;

   while ((pd = (ParameterDefinition*)iterator.getNext())) {
	n++;
	if (pd->isDummy()) {
	   found_dummy = TRUE;
	   break; 
	}
   }
   if (!found_dummy)
	n++;

   ASSERT(n>0);
   return n;
}

boolean MacroDefinition::setNetwork(Network *net)
{
    this->body = net;
    this->initialRead = FALSE;

    return TRUE;
}

