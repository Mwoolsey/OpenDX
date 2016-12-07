/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"


#include "JavaNet.h"
#include "PageGroupManager.h"
#include "Command.h"
#include "NoUndoJavaNetCommand.h"

#ifdef OS2
#include <stdlib.h>
#include <types.h>
#endif
#include <stdio.h>	// For fprintf(), fseek() and rename()
#include <time.h>

#if defined(DXD_WIN) || defined(OS2)          //SMH get correct name for unlink for NT in stdio.h
#define unlink _unlink
#undef send             //SMH without stepping on things
#undef FLOAT
#undef PFLOAT
#else
#include <unistd.h>	// For unlink()
#endif
#include <errno.h>	// For errno
#include <sys/stat.h>

#include "DXStrings.h"
#include "List.h"
#include "ListIterator.h"

#include "ControlPanel.h"
#include "NodeDefinition.h"
#include "InteractorNode.h"
#include "DXLInputNode.h"
#include "DXLOutputNode.h"
#include "TransmitterNode.h"
#include "ReceiverNode.h"
#include "Ark.h"
#include "SymbolManager.h"
#include "DXVersion.h"
#include "ImageNode.h"
#include "PageGroupManager.h"
#include "DXApplication.h"
#include "ErrorDialogManager.h"
#include "WarningDialogManager.h"
#include "InfoDialogManager.h"

#include "Dictionary.h"

#define DXVIEW_CABFILE_ID "CLASSID=\"CLSID:983F4461-A4B0-11D1-BABA-000629685A02\""
#define DXVIEW_CABFILE_BASE "CODEBASE=\"DXView.cab#version=1,0,0,4\""

String JavaNet::UnsupportedTools[] = { 
	"Colormap", "Probe", "ProbeList", "WriteImage", 
	"ManageColormapEditor", "Message",
	"ManageControlPanel", "ManageImageWindow", "ManageSequencer", 
	NUL(char*) 
    };

//
// We use several special macros to do the java-exporting.  The inputs for
// those macros are numbered and we need the numbers:
//

JavaNet::JavaNet()
{
    this->base_name = NUL(char*);

    this->html_f = NUL(FILE*);
    this->make_f = NUL(FILE*);
    this->applet_f = NUL(FILE*);

    this->html_file = NUL(char*);
    this->make_file = NUL(char*);
    this->applet_file = NUL(char*);
    this->bean_file = NUL(char*);

    this->saveWebPageCmd = 
	new NoUndoJavaNetCommand("saveWebPageCommand", this->commandScope,
	    FALSE, this, NoUndoJavaNetCommand::SaveWebPage);

    this->saveAppletCmd = 
	new NoUndoJavaNetCommand("saveAppletCommand", this->commandScope,
	    FALSE, this, NoUndoJavaNetCommand::SaveApplet);

    this->saveBeanCmd = 
	new NoUndoJavaNetCommand("saveBeanCommand", this->commandScope,
	    TRUE, this, NoUndoJavaNetCommand::SaveBean);

}

JavaNet::~JavaNet()
{
    if (this->base_name) delete this->base_name;

    if (this->html_f) fclose(this->html_f);
    if (this->make_f) fclose(this->make_f);
    if (this->applet_f) fclose(this->applet_f);

    if (this->html_file) delete this->html_file;
    if (this->make_file) delete this->make_file;

    if (this->saveWebPageCmd) delete this->saveWebPageCmd;
    if (this->saveAppletCmd) delete this->saveAppletCmd;
    if (this->saveBeanCmd) delete this->saveBeanCmd;
}


boolean JavaNet::saveNetwork(const char* name, boolean force)
{
    boolean status = this->Network::saveNetwork(name, force);
    if ((status) && (this->isJavified())) {
	this->saveWebPageCmd->activate();
	this->saveAppletCmd->activate();
    }
    this->setOutputName(this->getFileName());
    return status;
}

boolean JavaNet::setOutputName(const char* bn)
{
    // Strip off any leading directory names so that we 
    // write into the current working directory.
    //
    if ((!bn) || (!bn[0])) return FALSE;
    this->base_name = GetFileBaseName(bn, ".net");
    char* dirn = GetDirname(this->getFileName());
    char* pathn = GetFullFilePath(dirn);
    delete dirn;
    dirn = NUL(char*);
    ASSERT(pathn);
    int path_len = 0;
    if (!IsBlankString(pathn)) path_len = strlen(pathn);


    //
    // Name the files we're going to use for writing.
    //
    const char* ext = ".html";
    if (this->html_file) delete this->html_file;
    this->html_file = new char[path_len + STRLEN(this->base_name) + STRLEN(ext) + 8];
    sprintf (this->html_file, "%s/%s%s", pathn, this->base_name, ext);

    ext = ".make";
    if (this->make_file) delete this->make_file;
    this->make_file = new char[path_len + STRLEN(this->base_name) + STRLEN(ext) + 8];
    sprintf (this->make_file, "%s/%s%s", pathn, this->base_name, ext);

    ext = ".java";
    if (this->applet_file) delete this->applet_file;
    this->applet_file = new char[path_len + STRLEN(this->base_name) + STRLEN(ext) + 8];
    sprintf (this->applet_file, "%s/%s%s", pathn, this->base_name, ext);

    ext = "Bean.java";
    if (this->bean_file) delete this->bean_file;
    this->bean_file = new char[path_len + STRLEN(this->base_name) + STRLEN(ext) + 8];
    sprintf (this->bean_file, "%s/%s%s", pathn, this->base_name, ext);

    delete pathn;

    return TRUE;
}

List* JavaNet::MakeUnsupportedToolList(JavaNet* net)
{
    const char **p;
    List* unsup_list = NUL(List*);
    for (p = (const char**)JavaNet::UnsupportedTools ; *p ; p++) {
	List *list = net->makeNamedNodeList(*p);
	if (list) {
	    if (!unsup_list) unsup_list = new List;
	    unsup_list->appendElement(*p);
	    delete list;
	}
    }
    return unsup_list;
}

boolean JavaNet::requires(const char* format)
{
    List* img = this->makeClassifiedNodeList(ClassImageNode, FALSE);
    if (img == NUL(List*)) return FALSE;
    char tbuf[32];
    if (format[0] == '\"')
	strcpy (tbuf, format);
    else
	sprintf (tbuf, "\"%s\"", format);
    ListIterator it(*img);
    ImageNode* in;
    boolean needs = FALSE;
    while ((needs == FALSE) && (in = (ImageNode*)it.getNext())) {
	const char* fmt = in->getWebOptionsFormat();
	if (EqualString(fmt, tbuf)) needs = TRUE;
    }
    delete img;
    return needs;
}

//
//
//
boolean JavaNet::saveWebPage()
{
ListIterator it;

    if (this->isNetworkSavable() == FALSE)
	return FALSE;

    ASSERT (this->isMacro() == FALSE);

    this->setOutputName(this->getFileName());

    ASSERT ((this->html_file) && (this->make_file) && (this->applet_file));

    unlink (this->html_file);
    unlink (this->make_file);
    unlink (this->applet_file);

    //
    // Check for unsupported tools
    //
    List *unsup_list = JavaNet::MakeUnsupportedToolList(this);
    if (unsup_list) {
	char msg[4096];
	char tbuf[512];
	int msglen = 0;
	sprintf (tbuf,
	    "This net contains the following tools which\n"
	    "are not translated into java code:\n\n"
	);
	strcpy (&msg[msglen], tbuf);
	msglen+= strlen(tbuf);
	ListIterator it(*unsup_list);
	const char* cp;
	while ( (cp = (char*)it.getNext()) ) {
	    sprintf (tbuf, "%s\n", cp);
	    strcpy (&msg[msglen], tbuf);
	    msglen+= strlen(tbuf);
	}
	delete unsup_list;
	unsup_list = NUL(List*);
	WarningMessage (msg);
    }

#if 0
    //
    // Check for modules marked ASYNC.  If there is any, then we'll
    // request that our dxlink session be run in exec-on-change mode.
    // We'll only request eoc mode if the async module is marked
    // as either outboard or runtime-loadable.
    // Unmark all Nodes so we know who has been printed.
    // In order to take advantage of a dxsession in eoc mode, the applet
    // in the browser must poll for new images.  Enabling this code
    // requires that polling be added in ui++/java/dx/client
    //
    boolean contains_asyncs = FALSE;
    if (contains_asyncs == FALSE) {
	ListIterator it;
	Node* node;
	FOR_EACH_NETWORK_NODE (this, node, it) {
	    node->clearMarked();
	    NodeDefinition* nd = node->getDefinition();
	    if ((contains_asyncs==FALSE) && (nd->isMDFFlagASYNCHRONOUS()) &&
		((nd->isOutboard()) || (nd->isDynamicallyLoaded())))
		contains_asyncs = TRUE;
	}
    }
#endif

    List* framenodes = this->makeClassifiedNodeList(ClassImageNode, FALSE);
    int framecnt = (framenodes?framenodes->getSize():0);
    ASSERT (framecnt);


    //
    // HTML
    //
    this->html_f = fopen (this->html_file, "w");
    if (!this->html_f) {
	ErrorMessage ("Unable to open %s for writing.", this->html_file);
	return FALSE;
    }

    //
    // A pretty header for the generated html
    //
    fprintf (this->html_f, "%s\n", this->getHtmlHeader());

    int dx_major = this->getDXMajorVersion();
    int dx_minor = this->getDXMinorVersion();
    int dx_micro = this->getDXMicroVersion();
    char version_string[64]; 
    sprintf (version_string, "%d.%d.%d", dx_major,dx_minor,dx_micro);

    //
    // Loop over all non-empty panels asking each for its dimensions.
    // We'll use the max width,height when we generate an applet tag.
    //
    List* cps = this->getNonEmptyPanels();
    int applet_width = 300; 
    int applet_height = 350;
    if (cps) {
	applet_width = MAX(applet_width, (110 * (1+cps->getSize())));
	ControlPanel* cp;
	ListIterator it(*cps);
	int w,h;
	while ( (cp = (ControlPanel*)it.getNext()) ) {
	    cp->getWorkSpaceSize(&w,&h);
	    applet_width = MAX(applet_width, w+14);
	    applet_height = MAX(applet_height, h);
	}
	delete cps;
	cps = NUL(List*);
    }


    //
    //
    if (fprintf (this->html_f,
	"<!-- The following applet describes %s.net.  It produces a representation \n"
	"     of the visual program's control panels and provides execution control.\n"
	"-->\n"
	"<!-- It is required that codebase and archive applet tags be identical \n"
	"     within the html page for both control panels and image windows. If not, then\n"
	"     separate class loaders will be instantiated for the separate applets, and \n"
	"     the DXappplication will not be able to communicate with the image windows.\n"
	"     This manifests itself as any rendered images popping up in a separate \n"
	"     browser window.\n"
	"-->\n"
	"<APPLET\n"
	"\tCODE=\"%s.class\" width = %d height = %d\n"
	"\tCODEBASE=\"../\"\n"
	"\tARCHIVE=\"htmlpages/dx.jar,%s/%s.jar\"\n"
	"\tMAYSCRIPT\n"
	">\n"
	"\t<PARAM NAME=\"name\" VALUE=\"%s\">\n"
	"<!--    <PARAM NAME=cabbase VALUE=\"dx.cab\"> -->\n"
	"\t<PARAM NAME=NETNAME VALUE=\"%s.net\">\n"
	"\t<PARAM NAME=DXUIVERS VALUE=\"%s\">\n"
	"\t<PARAM NAME=BACKGROUND VALUE=\"[0.86, 0.86, 0.86]\">\n"
	"\t<PARAM NAME=EXECUTE_ON_CHANGE VALUE=\"true\">\n"
	"\t<!--\n"
	"\tIf you define this param, then the web page will let\n"
	"\tthe user control caching of images.  She would be able to\n"
	"\tassemble a series of rotations for example into a coherent view\n"
	"\t<PARAM NAME=CACHING_FEATURES VALUE=\"true\">\n"
	"\t-->\n",
	    this->base_name, this->base_name, applet_width, applet_height, 
	    theDXApplication->getUserHtmlDir(), this->base_name,
	    this->base_name, this->base_name, version_string) <= 0)
	return FALSE;
	
	// If the net contains DXLInput tools, then write an applet param tag
	// containing them.
	
	List* dxlis = this->makeClassifiedNodeList(ClassDXLInputNode);
	if(dxlis && dxlis->getSize() > 1) {
		fprintf(this->html_f, 
			"\t<!-- DXLInputs are passed to the network via the DXLInput parameter.\n"
			"\t     Any number of DXLInputs can be passed in using a one-line scripting\n"
			"\t     language command using compound assignments.\n"
			"\t-->\n");
	
		ListIterator iit(*dxlis);
		Node* din;
		fprintf(this->html_f, "\t<PARAM NAME=DXLInput VALUE='");
		while ( (din = (Node*)iit.getNext()) ) {
			if(strcmp(din->getLabelString(), "_java_control"))
				fprintf(this->html_f, "%s=%s; ", din->getLabelString(), din->getInputValueString(1));
		}
		fprintf(this->html_f, "'>\n");
	}
	

	fprintf(this->html_f,
		"</APPLET>\n"
		);

    int i;
    boolean wrl_commented = FALSE;
    for (i=1; i<=framecnt; i++) {
	ImageNode* n = (ImageNode*)framenodes->getElement(i);
	int width, height;
	n->getResolution(width, height);
	int f = n->getInstanceNumber();
	const char* ext = n->getWebOptionsFormat();
	char* head = NUL(char*);
	if (ext != NUL(char*)) {
	    head = DuplicateString(ext);
	    if (head[0] == '"') {
		ext = &head[1];
		int len = strlen(head);
		if (head[len-1] == '"') head[len-1] = '\0';
	    }
	} else {
	    ext = "gif";
	}

	if (EqualString(ext, "wrl")) {
	    if (!wrl_commented) {
		fprintf (this->html_f, 
		    "\n\n"
		    "<!-- This presents the vrml file generated from %s_%d in %s.net.\n"
		    "     If you remove this, then the web page will have no vrml \n"
		    "     initially.  After the first execution, the generated .wrl\n"
		    "     will appear in a separate browser window or it will be placed\n"
		    "     in a frame in your document named %s_%d.  Each successive\n"
		    "     execution will replace the previous vrml world.\n"
		    "-->\n",
		    n->getNameString(), n->getInstanceNumber(), this->base_name,
		    n->getNameString(), n->getInstanceNumber()
		);
		wrl_commented = TRUE;
	    }
	    fprintf (this->html_f, 
		"<embed\n"
		"\tsrc=\"%s%d.0.0.wrl\"\n"
		"\twidth = %d height = %d\n"
		">\n",
		this->base_name, f, width, height
	    );
	} else if (EqualString(ext, "dx")) {
	    fprintf (this->html_f, "<APPLET CODE=ActiveXFinder.class ID=AXF%d\n",
		n->getInstanceNumber());
	    fprintf (this->html_f, "    CODEBASE=\"../\"\n");
	    fprintf (this->html_f, "    MAYSCRIPT\n");
	    fprintf (this->html_f, "    WIDTH = 2 HEIGHT = 2\n");
	    fprintf (this->html_f, ">\n");
	    fprintf (this->html_f, "    <PARAM NAME=IMAGE_NODE value=\"Image_%d\">\n",
		n->getInstanceNumber());
	    fprintf (this->html_f, 
		"    <PARAM NAME=INITIAL_IMAGE VALUE=\"%s/%s%d.0.0.dx\">\n",
		theDXApplication->getUserHtmlDir(), this->base_name, 
		n->getInstanceNumber());
	    fprintf (this->html_f, 
		"<!-- <PARAM NAME=cabbase VALUE=\"htmlpages/dx.cab\"> -->\n");
	    fprintf (this->html_f, "</APPLET>\n");

	    fprintf (this->html_f, "<object ID=\"DXView\" width=%d height=%d\n",
		width, height);
	    fprintf (this->html_f, "    %s\n", DXVIEW_CABFILE_ID);
	    fprintf (this->html_f, "    %s>\n", DXVIEW_CABFILE_BASE);
	    fprintf (this->html_f, "</object>\n");

	    fprintf (this->html_f, "<script language=\"VBScript\">\n");
	    fprintf (this->html_f, "    <!--\n");
	    fprintf (this->html_f, "        Sub window_onLoad()\n");
	    fprintf (this->html_f, "        document.AXF%d.setCtrl DXView\n",
		n->getInstanceNumber());
	    fprintf (this->html_f, "        end sub\n");
	    fprintf (this->html_f, "    -->\n");
	    if (fprintf (this->html_f, "</script>\n") < 0) return FALSE;
	} else {
	    fprintf (this->html_f, 
		"<APPLET\n"
		"\tCODE=\"imageWindow.class\" WIDTH = %d HEIGHT = %d\n"
		"\tCODEBASE=\"../\"\n"
		"\tARCHIVE=\"htmlpages/dx.jar,%s/%s.jar\"\n"
		"\tMAYSCRIPT\n"
		">\n"
		"<!--    <PARAM NAME=cabbase VALUE=\"dx.cab\"> -->\n"
		"\t<PARAM NAME=IMAGE_NODE VALUE=\"Image_%d\">\n"
		"\t<PARAM NAME=INITIAL_IMAGE VALUE=\"%s/%s%d.0.0.%s\">\n",
		width, height, theDXApplication->getUserHtmlDir(),
		this->base_name, n->getInstanceNumber(),
		theDXApplication->getUserHtmlDir(), this->base_name, f, ext
	    );
	    if (n->isWebOptionsOrbit()) {
		fprintf (this->html_f,
		    "\t<PARAM NAME=OPEN_IN_ORBIT_MODE VALUE=\"true\">\n"
		);
	    }
	    fprintf (this->html_f, "</APPLET>\n");
       }
       if (head) delete head;
    }

    //
    // If the net contains DXLOutput tools, then write an applet tag 
    // corresponding to each one.  These applets will automatically be
    // associated with the DXLOutputs in DXLinkApplication.start()
    // in ui++/java/dx/net/DXLinkApplication.java
    //
    List* dxlos = this->makeClassifiedNodeList(ClassDXLOutputNode);
    if (dxlos) {
	fprintf (this->html_f, "\n<!--\n");
	fprintf (this->html_f, "\tApplet tags which handle DXLOutput tool\n");
	fprintf (this->html_f, "\toutput.  You are responsible for setting\n");
	fprintf (this->html_f, "\tappropriate size and location for these.\n");
	fprintf (this->html_f, "\tEach of these applets handles any number\n");
	fprintf (this->html_f, "\tof DXLOutputs.  It's up to you to specify\n");
	fprintf (this->html_f, "\tto the applet how many it should handle by\n");
	fprintf (this->html_f, "\tgiving the applet params named DXLOutput%%d\n");
	fprintf (this->html_f, "\tThe %%d goes from 0 to n and has nothing to\n");
	fprintf (this->html_f, "\twith instance numbers.\n");
	fprintf (this->html_f, "-->\n");

	ListIterator it(*dxlos);
	Node* don;
	while ( (don = (Node*)it.getNext()) ) {
	    fprintf (this->html_f, "<APPLET\n"
		"\tCODE=\"CaptionLabels.class\" WIDTH = 250 HEIGHT = 20\n"
		"\tCODEBASE=\"../\"\n"
		"\tARCHIVE=\"htmlpages/dx.jar,%s/%s.jar\"\n"
		"\tMAYSCRIPT\n"
		">\n"
		"<!--    <param name=cabbase value=\"htmlpages/dx.cab\"> -->\n"
		"\t<PARAM NAME=DXLOutput0 VALUE=\"%s\">\n"
		"\t<PARAM NAME=BACKGROUND VALUE=\"[0.86, 0.86, 0.86]\">\n"
		"\t<PARAM NAME=FOREGROUND VALUE=\"[0.0, 0.0, 0.0]\">\n",
		theDXApplication->getUserHtmlDir(), this->base_name,
		don->getLabelString()
	    );

	    fprintf (this->html_f, "</APPLET>\n");
	}

	delete dxlos;
    }

    fprintf (this->html_f, "</body>\n</html>\n");

    fclose (this->html_f);
    this->html_f = NUL(FILE*);

    //
    // Makefile
    //
    this->make_f = fopen (this->make_file, "w");
    if (this->make_f == NUL(FILE*)) {
	ErrorMessage ("Unable to open %s for writing.", this->make_file);
	return FALSE;
    }

    const char* uiroot = NULL;
    const char* resource = NULL;
    fprintf (this->make_f,
	"##MANDATORY TO SET DXARCH prior to invoking this makefile"
	"##\n"
	"## Certain parameters are set here based upon the dx build environment.\n"
	"## If other values are more appropriate, you can hand-edit this makefile\n"
	"## or you can set them in your $HOME/DX resource file as follows:\n"
	"## \tDX*cosmoDir: /some/where/vrml\n"
	"## \tDX*dxJarFile: /some/where/java/htmlpages/dx.jar\n"
	"## \tDX*userHtmlDir: /some/where/java/user\n"
	"## \tDX*userJarFile: /some/where/java/user/%s.jar\n"
	"## \tDX*jdkDir: /usr/jdk_base/lib/classes.zip\n"
	"## Note that a value for cosmoDir is required if your\n"
	"## web page uses vrml and may be left out otherwise.\n"
	"##\n\n", this->base_name
    );
    uiroot = theDXApplication->getUIRoot();
    if (!uiroot)
        uiroot = "/usr/local/dx";

    fprintf(this->make_f, "SHELL = /bin/sh\n\n");
    fprintf(this->make_f, "BASE = %s\n",uiroot);
    fprintf(this->make_f, "ARCH = %s\n\n", DXD_ARCHNAME);

    fprintf(this->make_f, "include $(BASE)/lib_$(ARCH)/arch.mak\n\n");


    const char* cosmoDir = theDXApplication->getCosmoDir();
    resource= theDXApplication->getDxJarFile();
    if(resource==NULL || resource[0]=='\0')resource="$(BASE)/java/htmlpages/dx.jar";
    fprintf (this->make_f, "JARFILE=%s\n", resource); 
    resource=theDXApplication->getJdkDir();
    if(resource==NULL || resource[0]=='\0')resource="$(DX_JAVA_CLASSPATH)";
    fprintf (this->make_f, "JDKFILE=%s\n", resource);
    char pathSep = ':';
#if defined(DXD_WIN)
    pathSep = ';';
#endif
    /* Allow override of CosmoDir resource if needed */
    if (cosmoDir[0]) {
	fprintf (this->make_f, "COSMO=%s\n", cosmoDir);
	fprintf (this->make_f, "JFLAGS=-classpath $(JDKFILE)%c$(JARFILE)%c$(COSMO)\n\n",
	    pathSep, pathSep);
    } else {
	fprintf (this->make_f, "JFLAGS=-classpath $(JDKFILE)%c$(JARFILE)\n\n",
	    pathSep);
    }

    fprintf (this->make_f, "JARS = \\\n");
    fprintf (this->make_f, "\t%s.jar\n\n", this->base_name);

    fprintf (this->make_f, "OBJS = \\\n");
    fprintf (this->make_f, "\t%s.class\n\n", this->base_name);

#if defined(DXD_WIN)
    fprintf (this->make_f, "MAKE = nmake -f\n");
#else
    fprintf (this->make_f, "MAKE = make -f\n");
#endif
    fprintf (this->make_f, ".%spage: $(JARS)\n\t$(MAKE) %s $(OBJS)\n\ttouch .%spage\n\n",
	this->base_name, this->make_file, this->base_name);

    fprintf (this->make_f, "%s.jar: $(OBJS)\n", this->base_name);
    fprintf (this->make_f, "\tjar cf %s.jar $(OBJS)\n\n", this->base_name);

    fprintf (this->make_f, "%s.class: %s.java\n", this->base_name, this->base_name);
    fprintf (this->make_f, "\tjavac $(JFLAGS) %s.java\n\n", this->base_name);

    fprintf (this->make_f, "\n\n");
    fprintf (this->make_f,
	"##\n"
	"## The following variables are set based upon dx-build-time settings.\n"
	"## You can hand edit in different values or you can override them\n"
	"## in your resource file $HOME/DX. For example:\n"
	"## \tDX*htmlDir: /usr/http/java\n"
	"## \tDX*userHtmlDir: user\n"
	"## \tDX*serverDir: /usr/admin/java/server\n"
	"##\n"
	"##\tYou will need to make corresponding changes to e.g. dxserver.paths\n"
    );
    resource=theDXApplication->getServerDir();
    if(resource==NULL || resource[0]=='\0')resource="$(BASE)/java/server";
    fprintf (this->make_f, "DXSERVER=%s\n", resource);
#if defined(DXD_WIN)
    fprintf (this->make_f, "DXSERVER_DIR=$(DXSERVER)\\pcnets\n");
#else
    fprintf (this->make_f, "DXSERVER_DIR=$(DXSERVER)/nets\n");
#endif

    char dirSep = '/';
#if defined(DXD_WIN)
    dirSep = '\\';
#endif
    resource=theDXApplication->getHtmlDir();
    if(resource==NULL || resource[0]=='\0')resource="$(BASE)/java";
    fprintf (this->make_f, "JAVADIR=%s", resource); 
    resource=theDXApplication->getUserHtmlDir();
    if(resource!=NULL && resource[0]!='\0')
	fprintf (this->make_f, "%c%s", dirSep , resource); 
    fprintf (this->make_f, "\n\n"); 

    fprintf (this->make_f,
	"##\n"
	"## This is the install rule.  You might add lines to copy macros\n"
	"## to $(DXSERVER)/usermacros or data to $(DXSERVER)/userdata\n"
	"##\n"
    );
#if defined(DXD_WIN)
    fprintf (this->make_f, "CP = copy\n");
#else
    fprintf (this->make_f, "CP = cp -f\n");
#endif
    fprintf (this->make_f, "install: .%spage\n", this->base_name);
    fprintf (this->make_f, "\t$(CP) %s.jar $(JAVADIR)\n", this->base_name);

    fprintf (this->make_f, "\t$(CP) %s.html %s\n", this->base_name, "$(JAVADIR)");

    for (i=1; i<=framecnt; i++) {
	ImageNode* n = (ImageNode*)framenodes->getElement(i);
	int f = n->getInstanceNumber();
	fprintf (this->make_f, "\t$(CP) %s%d.0.* %s\n", this->base_name,f,"$(JAVADIR)");
    }

    fprintf (this->make_f, "\t$(CP) %s.net $(DXSERVER_DIR)\n\n", this->base_name);

#if defined(DXD_WIN)
    fprintf (this->make_f, "RM = del\n");
#else
    fprintf (this->make_f, "RM = rm -f\n");
#endif
    fprintf (this->make_f, "\n\nclean:\n");
    fprintf (this->make_f, "\t$(RM) .%spage\n", this->base_name);
    fprintf (this->make_f, "\t$(RM) %s.jar\n", this->base_name);
    fprintf (this->make_f, "\t$(RM) %s.class\n", this->base_name);

    fprintf (this->make_f, "\t$(RM) $(DXSERVER_DIR)/%s.net\n", this->base_name);
    fprintf (this->make_f, "\t$(RM) %s/%s.class\n", "$(JAVADIR)", this->base_name);

    fprintf (this->make_f, "\t$(RM) %s/%s.html\n", "$(JAVADIR)", this->base_name);
    for (i=1; i<=framecnt; i++) {
	ImageNode* n = (ImageNode*)framenodes->getElement(i);
	int f = n->getInstanceNumber();
	fprintf (this->make_f, "\t$(RM) %s/%s%d.*\n", "$(JAVADIR)", this->base_name, f);
    }


    fclose(this->make_f);
    this->make_f = NUL(FILE*);

    if (framenodes)
	delete framenodes;

    if (this->netToApplet() == FALSE)
	return FALSE;

    DXExecCtl *dxc = theDXApplication->getExecCtl();
    if (!dxc) return FALSE;
    theDXApplication->resetServer();


    //
    // Set the java parameter in all ImageNodes, then execute Once
    //
    List* imgnodes = this->makeClassifiedNodeList(ClassImageNode);
    if (imgnodes) {
	it.setList(*imgnodes);
	ImageNode* bn;
	char* dirn = GetDirname(this->getFileName());
	char* pathn = GetFullFilePath(dirn);
	int len = 0;
	if (pathn) len = strlen(pathn);
	char *fname = new char[len + strlen(this->base_name) + 4];
	while ( (bn = (ImageNode*)it.getNext()) ) {
	    int f = bn->getInstanceNumber();
	    if (IsBlankString(pathn))
		sprintf (fname, "%s%d", this->base_name, f); 
	    else
		sprintf (fname, "%s/%s%d", pathn, this->base_name, f); 
	    bn->enableJava(fname, TRUE);
	}
	delete fname;
	if (pathn) delete pathn;
	if (dirn) delete dirn;

	dxc->executeOnce();

	it.setList(*imgnodes);
	while ( (bn = (ImageNode*)it.getNext()) ) 
	    bn->disableJava(TRUE);
	delete imgnodes;
    }
    return TRUE;
}



const char* JavaNet::getHtmlHeader()
{
    const char* header = 
    "<!DOCTYPE HTML PUBLIC \"-//IETF//DTD HTML 3//EN\">"
	"<html><head>\n"
	"<title>Open Visualization Java Explorer</title>\n"
	"</head>\n"
	"<body bgcolor=\"#dddddd\">\n"
	"<hr>\n"
	"<p>\n";

    return header;
}


boolean JavaNet::netToApplet()
{
#ifdef BETA_VERSION
    char* ateb = "Beta";
#else
    char* ateb = "";
#endif

    this->applet_f = fopen(this->applet_file, "w");
    if (!this->applet_f) {
	ErrorMessage ("Unable to open %s for writing.", this->applet_file);
	return FALSE;
    }

    time_t t = time((time_t*)NULL);
    fprintf (this->applet_f, "//\n");
    fprintf (this->applet_f, "// Applet file generated automatically \n");
    fprintf (this->applet_f, "// time: %s", ctime(&t));
    fprintf (this->applet_f, "// DX version: %d.%d.%d (format), %d.%d.%d (DX%s)\n//\n",
	this->getNetMajorVersion(),
	this->getNetMinorVersion(),
	this->getNetMicroVersion(),
	this->getDXMajorVersion(),
	this->getDXMinorVersion(),
	this->getDXMicroVersion(),
	ateb);
    fprintf (this->applet_f, "// Java version 1.1\n");
    fprintf (this->applet_f, "//\n");

    fprintf (this->applet_f, "import dx.net.*;\n");
    fprintf (this->applet_f, "import dx.runtime.*;\n");
    fprintf (this->applet_f, "import java.util.*;\n");
    fprintf (this->applet_f, "import java.awt.*;\n");
    fprintf (this->applet_f, "\n");
    if (this->requires("wrl")) {
	fprintf (this->applet_f, 
	    "//\n"
	    "// These are required in order to access the TouchSensor\n"
	    "// and ProximitySensor in cosmo.  They're included here\n"
	    "// because a WebOptions macro has its format input set to \"wrl\"\n"
	    "//\n"
	    "public class %s extends WRLApplication  {\n", 
	    this->base_name);
    } else
	fprintf (this->applet_f, 
	    "public class %s extends DXLinkApplication {\n", this->base_name);
    fprintf (this->applet_f, "\n");
    fprintf (this->applet_f, "    public void readNetwork() {\n");


    //
    // Print each startup control panel as java
    //
    ListIterator it;
    List* cps = this->getNonEmptyPanels();
    if (cps) {
	ControlPanel* cp;
	it.setList(*cps);
	while ( (cp = (ControlPanel*)it.getNext()) ) {
	    if (cp->printAsJava(this->applet_f) == FALSE) {
		fclose(this->applet_f);
		this->applet_f = NUL(FILE*);
		return FALSE;
	    }
	}
	delete cps;
	cps = NUL(List*);
    }

    //
    // Print each Node as java
    //
    Node* n;
    FOR_EACH_NETWORK_NODE(this, n, it) {
	if (n->printAsJava(this->applet_f) == FALSE) {
	    fclose (this->applet_f);
	    this->applet_f = NUL(FILE*);
	    return FALSE;
	}
    }

    fprintf (this->applet_f, "    }\n");
    fprintf (this->applet_f, "}\n");
    fclose (this->applet_f);
    this->applet_f = NUL(FILE*);
    return TRUE;
}


boolean JavaNet::printMacroReferences(FILE *f, boolean inline_define,
    PacketIFCallback ec, void *ecd)
{
    if (this->Network::printMacroReferences (f, inline_define, ec, ecd) == FALSE)
	return FALSE;

    if (this->isJavified()) {
	if (fprintf (f, 
	    "//\n"
	    "// Macros used when running under control of Java Explorer\n"
	    "//\n"
	    "include \"gifmac.net\";\n"
	    "include \"vrmlmac.net\";\n"
	    "include \"dxmac.net\";\n"
	    ) < 0) return FALSE;
    }

    return TRUE;
}

//
// Does the net contain at least 1 WebOptions and at least 1 ImageNode
//
boolean JavaNet::isJavified()
{
    if (this->isMacro()) return FALSE;
    List* wo = this->makeNamedNodeList("WebOptions");
    if (wo == NUL(List*)) return FALSE;
    int wsize = wo->getSize();
    delete wo;
    if (wsize <= 0) return FALSE;

    List* img = this->makeClassifiedNodeList(ClassImageNode, FALSE);
    if (img == NUL(List*)) return FALSE;
    ListIterator it(*img);
    boolean all_javified = TRUE;
    ImageNode* in;
    while ((all_javified) && (in = (ImageNode*)it.getNext())) {
	if (in->isJavified() == FALSE)
	    all_javified = FALSE;
    }

    delete img;
    return all_javified;
}

void JavaNet::changeExistanceWork(Node *n, boolean adding)
{
    this->Network::changeExistanceWork(n,adding);
    this->saveWebPageCmd->deactivate();
    this->saveAppletCmd->deactivate();
}


boolean JavaNet::saveApplet()
{
    if (this->isJavified() == FALSE) 
	return FALSE;
    if (this->isNetworkSavable() == FALSE)
	return FALSE;
    if (this->applet_file == NUL(char*)) 
	this->setOutputName(this->getFileName());
    ASSERT (this->applet_file);
    unlink (this->applet_file);
    return this->netToApplet();
}

boolean JavaNet::saveBean()
{
Node* n;
ListIterator it;
#ifdef BETA_VERSION
    char* ateb = "Beta";
#else
    char* ateb = "";
#endif

    if (this->nodeList.getSize() <= 0) return FALSE;

    this->setOutputName(this->getFileName());
    this->bean_f = fopen(this->bean_file, "w");
    if (!this->bean_f) {
	return FALSE;
    }

    fprintf (this->bean_f, "//\n");
    fprintf (this->bean_f, "// Bean file generated automatically \n");
    fprintf (this->bean_f, "// DX version: %d.%d.%d (format), %d.%d.%d (DX%s)\n//\n",
	this->getNetMajorVersion(),
	this->getNetMinorVersion(),
	this->getNetMicroVersion(),
	this->getDXMajorVersion(),
	this->getDXMinorVersion(),
	this->getDXMicroVersion(),
	ateb);
    fprintf (this->bean_f, "// Java version 1.1\n");
    fprintf (this->bean_f, "//\n");

    fprintf (this->bean_f, "class %sBean extends java.awt.Panel {\n", this->base_name);
    fprintf (this->bean_f, "	private String netFile;\n");
    fprintf (this->bean_f, "	private imageCanvas rmi;\n");
    fprintf (this->bean_f, "	private String host;\n");
    fprintf (this->bean_f, "	private String command;\n");
    fprintf (this->bean_f, "\n\n");
    fprintf (this->bean_f, "    public %sBean()  {\n", this->base_name);
    fprintf (this->bean_f, "	this.netFile = null;\n");
    fprintf (this->bean_f, "	this.rmi = null;\n");
    fprintf (this->bean_f, "	this.host = null;\n");
    fprintf (this->bean_f, "	this.command = null;\n");
    fprintf (this->bean_f, "	this.setLayout(null);\n");

    //
    // Print each Node's bean initialization inside the constructor
    //
    FOR_EACH_NETWORK_NODE(this, n, it) {
	if (n->printAsBeanInitCall(this->bean_f) == FALSE) {
	    fclose (this->bean_f);
	    this->bean_f = NUL(FILE*);
	    return FALSE;
	}
    }

    fprintf (this->bean_f, "    }\n");
    fprintf (this->bean_f, "    public %sBean(java.awt.LayoutManager layout) {\n", this->base_name);
    fprintf (this->bean_f, "	super(layout);\n");
    fprintf (this->bean_f, "	this.netFile = null;\n");
    fprintf (this->bean_f, "	this.rmi = null;\n");
    fprintf (this->bean_f, "	this.host = null;\n");
    fprintf (this->bean_f, "	this.command = null;\n");
    //
    // Print each Node's bean initialization inside the constructor
    //
    FOR_EACH_NETWORK_NODE(this, n, it) {
	if (n->printAsBeanInitCall(this->bean_f) == FALSE) {
	    fclose (this->bean_f);
	    this->bean_f = NUL(FILE*);
	    return FALSE;
	}
    }
    fprintf (this->bean_f, "    }\n\n");

    FOR_EACH_NETWORK_NODE(this, n, it) {
	if (n->printAsBean(this->bean_f) == FALSE) {
	    fclose (this->bean_f);
	    this->bean_f = NUL(FILE*);
	    return FALSE;
	}
    }
    fprintf (this->bean_f, "\n\n");

    fprintf (this->bean_f, "    public String getCommand() { return this.command; }\n");
    fprintf (this->bean_f, "    public String getHost() { return this.host; }\n");
    fprintf (this->bean_f, "    public String getNetFile() { \n");
    fprintf (this->bean_f, "	if (this.rmi != null)\n");
    fprintf (this->bean_f, "	    this.netFile = this.rmi.getNetFile();\n");
    fprintf (this->bean_f, "	return this.netFile; \n");
    fprintf (this->bean_f, "    }\n");
    fprintf (this->bean_f, "    public void setCommand(String cmd) {\n");
    fprintf (this->bean_f, "	this.command = cmd;\n");
    fprintf (this->bean_f, "	if (this.rmi != null) return ;\n");
    fprintf (this->bean_f, "	if ((this.host != null) & (this.command != null)) {\n");
    fprintf (this->bean_f, "	    this.rmi = new imageCanvas();\n");
    fprintf (this->bean_f, "	    java.awt.Dimension d = this.getSize();\n");
    fprintf (this->bean_f, "	    this.add(this.rmi);\n");
    fprintf (this->bean_f, "	    if (this.netFile != null)\n");
    fprintf (this->bean_f, "		this.rmi.setNetFile(this.netFile);\n");
    fprintf (this->bean_f, "	    this.rmi.setSize(d.width, d.height);\n");
    fprintf (this->bean_f, "	} \n");
    fprintf (this->bean_f, "    }\n");
    fprintf (this->bean_f, "    public void setHost(String host) {\n");
    fprintf (this->bean_f, "	this.host = host;\n");
    fprintf (this->bean_f, "	if (this.rmi != null) return ;\n");
    fprintf (this->bean_f, "	if ((this.host != null) & (this.command != null)) {\n");
    fprintf (this->bean_f, "	    this.rmi = new imageCanvas();\n");
    fprintf (this->bean_f, "	    java.awt.Dimension d = this.getSize();\n");
    fprintf (this->bean_f, "	    this.add(this.rmi);\n");
    fprintf (this->bean_f, "	    if (this.netFile != null)\n");
    fprintf (this->bean_f, "		this.rmi.setNetFile(this.netFile);\n");
    fprintf (this->bean_f, "	    this.rmi.setSize(d.width, d.height);\n");
    fprintf (this->bean_f, "	} \n");
    fprintf (this->bean_f, "    }\n");
    fprintf (this->bean_f, "    public void setNetFile(String net) {\n");
    fprintf (this->bean_f, "	this.netFile = net;\n");
    fprintf (this->bean_f, "	if (this.rmi == null) return ;\n");
    fprintf (this->bean_f, "	if (this.netFile == null) return ;\n");
    fprintf (this->bean_f, "	this.rmi.setNetFile(this.netFile);\n");
    fprintf (this->bean_f, "    }\n");
    fprintf (this->bean_f, "}\n");

    fclose (this->bean_f);
    this->bean_f = NUL(FILE*);
    return TRUE;
}

Command* JavaNet::getSaveBeanCommand()
{
    return NUL(Command*);
    //return this->saveBeanCmd;
}

