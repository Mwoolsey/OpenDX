/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"


#include "ErrorDialogManager.h"
#include "DXApplication.h"
#include "DXVersion.h"
#include "DXWindow.h"
#include "Command.h"
#include "DXExecCtl.h"
#include "List.h"
#include "ListIterator.h"
#include "Network.h"
#include "Node.h"
#include "SequencerNode.h"
#include "ImageNode.h"
#include "ColormapNode.h"
#include "DisplayNode.h"
#include "ImageWindow.h"
#include "EditorWindow.h"
#include "ProbeNode.h"
#include "PickNode.h"
#include "DXLInputNode.h"
#include "CmdEntry.h"
#include "ControlPanel.h"
#include "MacroDefinition.h"
#include "ViewControlDialog.h"
#include "DXLinkHandler.h"
#include <ctype.h>
#include <stdarg.h>

/*
 * What follows is a set of routines to handle the DX Application commands.
 * It's essentially unchanged.
 */

#define QUALIFIER_NONE   0
#define QUALIFIER_LABEL  1
#define QUALIFIER_TITLE  2

static List *MakeQualifiedNodeList(const char *classname, char *p, 
					boolean includeSubclasses)
{
    int   qualifierType;
    char *qualifier=NULL;
    List *result = NUL(List *);

    if (!p || !*p)
	qualifierType = QUALIFIER_NONE;

    else if (EqualSubstring(p, "title=", 6))
    {
	qualifierType = QUALIFIER_TITLE;
	qualifier     = p+6;
    }
    else if (EqualSubstring(p, "label=", 6))
    {
	qualifierType = QUALIFIER_LABEL;
	qualifier     = p+6;
    }
    else
    {
	qualifierType = QUALIFIER_LABEL;
	qualifier     = p;
    }

    if (qualifierType == QUALIFIER_NONE)
    {
	result =
	  theDXApplication->network->makeClassifiedNodeList(classname, 
						includeSubclasses);
    }
    else if (qualifierType == QUALIFIER_LABEL)
    {
	Node *node;
#if 00
	List *list = 
	  theDXApplication->network->makeLabelledNodeList(qualifier);


	result = new List;

	if (list)
	{
	    ListIterator it(*list);
	    while (NULL != (node = (Node *)it.getNext()))
		if (node->isA(classname))
		    result->appendElement((const void *)node);
	    
	    delete list;
	}
#else
	List *list = 
	  theDXApplication->network->makeClassifiedNodeList(classname,
							includeSubclasses);

	if (list)
	{
	    ListIterator it(*list);
	    while ( (node = (Node *)it.getNext()) )
		if (EqualString(node->getLabelString(),qualifier)) {
		    if (!result)
			result = new List;
		    result->appendElement((const void *)node);
		}
	    
	    delete list;
	}

#endif
    }
    else
    {

	List *list =
	  theDXApplication->network->makeClassifiedNodeList(classname, 
					includeSubclasses);

	if (list)
	{
	    Node *node;
	    ListIterator it(*list);
	    while (NULL != (node = (Node *)it.getNext()))
		if (EqualString(qualifier, node->getTitle())) {
		    if (!result)
			result = new List;
		    result->appendElement((const void *)node);
		}

	    delete list;
	}

    }

    return result;
}

boolean DXLinkHandler::OpenColormapEditor(const char *c, int id, void *va)
{
    char *p;

    for (p = (char *)c; p && *p && isspace(*p); p++);

    List *list = MakeQualifiedNodeList(ClassColormapNode, p, FALSE);

    if (!list)
    {
	DXLinkHandler *a = (DXLinkHandler*)va;
	char buffer[1024];
	sprintf(buffer,"OpenColormapEditor: invalid qualifier (%s)", p);
	a->sendPacket(DXPacketIF::PKTERROR,id,buffer);
	return FALSE;
    }

    ListIterator it(*list);
    ColormapNode *cnode;
    
    while (NULL != (cnode = (ColormapNode *)it.getNext())) {
	DXWindow *win = cnode->getDXWindow();;
	if (!win) 
	    cnode->openDefaultWindow(NULL);
	else if (!win->isManaged())
	    win->manage(); 
    }

    delete list;
							
    return TRUE;
}

boolean DXLinkHandler::CloseColormapEditor(const char *c, int id, void *va)
{
    char *p;

    for (p = (char *)c; p && *p && isspace(*p); p++);

    List *list = MakeQualifiedNodeList(ClassColormapNode, p, FALSE);

    if (!list)
    {
	DXLinkHandler *a = (DXLinkHandler*)va;
	char buffer[1024];
	sprintf(buffer,"CloseColormapEditor: invalid qualifier (%s)", p);
	a->sendPacket(DXPacketIF::PKTERROR,id,buffer);
	return FALSE;
    }

    ListIterator it(*list);
    ColormapNode *cnode;
    
    while (NULL != (cnode = (ColormapNode *)it.getNext()))
    {
	DXWindow *win = cnode->getDXWindow();
	if (win)
	    win->unmanage();
    }

    delete list;
							
    return TRUE;
}

boolean DXLinkHandler::SetHWRendering(const char *c, int id, void *va)
{
    DXLinkHandler *dxlh = (DXLinkHandler*)va;
    return dxlh->setRenderMode(c, id,FALSE);
}
boolean DXLinkHandler::SetSWRendering(const char *c, int id, void *va)
{
    DXLinkHandler *dxlh = (DXLinkHandler*)va;
    return dxlh->setRenderMode(c, id, TRUE);
}
boolean DXLinkHandler::setRenderMode(const char *msg, int id, boolean swmode)
{
    char *p;
    ImageNode *node;

    for (p = (char *)msg; p && *p && isspace(*p); p++);

    List *list = MakeQualifiedNodeList(ClassImageNode, p, FALSE);

    if (! list)
    {
	char buffer[1024];
	sprintf(buffer, "SetRenderMode: "
			"unable to set Image render mode (%s)", msg);
	this->sendPacket(DXPacketIF::PKTERROR,id,buffer);
	return FALSE;
    }

    ListIterator it(*list);
    while (NULL != (node = (ImageNode *)it.getNext()))
    {
	ImageWindow *win = (ImageWindow*)node->getDXWindow();
	if (win) 
	    win->setSoftware(swmode);
    }
    delete list;
    return TRUE;

}

boolean DXLinkHandler::OpenImage(const char *c, int id, void *va)
{
    char *p;
    Node *node;

    for (p = (char *)c; p && *p && isspace(*p); p++);

    List *list = MakeQualifiedNodeList(ClassDisplayNode, p, TRUE);

    if (! list)
    {
	DXLinkHandler *a = (DXLinkHandler*)va;
	char buffer[1024];
	sprintf(buffer, "OpenImage: unable to open image (%s)", c);
	a->sendPacket(DXPacketIF::PKTERROR,id,buffer);
	return FALSE;
    }

    ListIterator it(*list);

    while (NULL != (node = (Node *)it.getNext()))
    {
	DXWindow *win = node->getDXWindow();
	if (!win) 
	    ((DisplayNode*)node)->openImageWindow();
	else  
	    win->manage();
    }

    delete list;

    return TRUE;
}

boolean DXLinkHandler::CloseImage(const char *c, int id, void *va)
{
    char *p;
    Node *node;

    for (p = (char *)c; p && *p && isspace(*p); p++);

    List *list = MakeQualifiedNodeList(ClassDisplayNode, p, TRUE);

    if (! list)
    {
	DXLinkHandler *a = (DXLinkHandler*)va;
	char buffer[1024];
	sprintf(buffer, "OpenImage: unable to open image (%s)", c);
	a->sendPacket(DXPacketIF::PKTERROR,id, buffer);
	return FALSE;
    }

    ListIterator it(*list);

    while (NULL != (node = (Node *)it.getNext()))
    {
	DXWindow *win = node->getDXWindow();
	if (win)
	    win->unmanage();
    }

    delete list;

    return TRUE;
}

boolean DXLinkHandler::OpenSequencer(const char *c, int id, void *va)
{
    SequencerNode *seq = theDXApplication->network->sequencer;

    if (seq) {
	DXWindow *win = seq->getDXWindow();
	if (!win) 
	    seq->openDefaultWindow(theDXApplication->getRootWidget());
	else if (!win->isManaged())
	    win->manage();
    }

    return TRUE;
}

boolean DXLinkHandler::CloseSequencer(const char *c, int id, void *va)
{
    SequencerNode *seq = theDXApplication->network->sequencer;

    if (seq)
    {
	DXWindow *win = seq->getDXWindow();
	if (win)
	    win->unmanage();
    }

    return TRUE;
}


boolean DXLinkHandler::OpenVPE(const char *c, int id, void *va)
{
    EditorWindow *editor = theDXApplication->network->getEditor();

    if (! editor)
	editor = theDXApplication->newNetworkEditor(theDXApplication->network);

    if (! editor)
    {
	DXLinkHandler *a = (DXLinkHandler*)va;
	a->sendPacket(DXPacketIF::PKTERROR, id, 
					"cannot create editor");
	return FALSE;
    }

    if (!editor->isManaged()) {
	editor->manage();

	XMapRaised(XtDisplay(editor->getRootWidget()),
		   XtWindow(editor->getRootWidget()));
    }

    return TRUE;

}

boolean DXLinkHandler::CloseVPE(const char *c, int id, void *va)
{
    EditorWindow *editor = theDXApplication->network->getEditor();

    if (! editor)
	return TRUE;

    editor->unmanage();

    return TRUE;

}

/*
    This call is apparently written to accomodate a file string
    of the form "Myprog.net Mycfg.cfg" rather than just "Myprog.net"
    Thus it looks for 2 strings separated by a space.  This causes
    a problem for the PC since directories may have a space in them.
    Fortunately this ability to specify the .cfg file isn't included
    in the docs for the DXLLoadVisualProgram call, so removing this feature
    from the PC version shouldn't cause any problems - but I'll leave it
    in the Unix code.  F Suits 5/97
    Oh - it appears it always had a bug because the line that says *d = '\0';
    used to say *d = '0';
    
    I believe this to be a problem. UN*X machines can also send paths
    with spaces for file names. MacOS X users will commonly do this; thus
    I will rewrite it to look to see if two files are being passed in (for
    a .net and a .cfg); if not, then just remove leading and trailing spaces.
*/
boolean DXLinkHandler::OpenNetwork(const char *c, int id, void *va)
{
    char *buf = new char[strlen(c)+1];
    char *d, *e, *cfg, *net;
	int len=0;

	if( !c ) 
	  return TRUE;

	e = buf;
	strcpy(buf, c);
	/* remove leading spaces */

    if (isspace(*e))
	while (isspace(*e++));
	
	if(! *e)
	  return TRUE;
	
	/* remove trailing spaces */
	
	len = strlen(e);
	d = (len<=0?e:e+len-1);
	
	if (isspace(*d)) {
	  while(isspace(*d--));
	  d++; *d = '\0';
	}
	
    d = net = buf;

	/* Now determine if there are two filenames passed in */
	
	if (strstr(buf, "net") != NULL && strstr(buf, "cfg") != NULL) {
	
	    /* Use old routine to get filenames for both net and cfg. File
	       names cannot contain spaces. 
	     */

        do {
	       d++;
        } while (*d && !isspace(*d));

        *d++ = '\0';
        e=d;

        if (*e && isspace(*e))
	        while (isspace(*e++));

        if (! *e)
	        cfg = NULL;
        else
        {
	        cfg = e;
        }
	} else {
		cfg = NULL;
	}
	
    theDXApplication->setBusyCursor(TRUE);

    boolean r;
    if (!theDXApplication->openFile(net, cfg))
    {
	DXLinkHandler *a = (DXLinkHandler*)va;
	char buffer[1024];
	sprintf(buffer, "Error opening network file %s", c);
	a->sendPacket(DXPacketIF::PKTERROR, id, buffer);
	r = FALSE;
    }
    else
    {
	r = TRUE;
    }
    theDXApplication->setBusyCursor(FALSE);

    delete buf;
    return r;
}

boolean DXLinkHandler::SaveNetwork(const char *c, int id, void *va)
{
    char *buf = new char[strlen(c)+1];
    char *d, *net;

    if (isspace(*c))
	while (isspace(*c++));

    if (! *c)
	net = NULL;
    else
    {
	d = net = buf;
	do {
	    *d++ = *c++;
	} while (*c && !isspace(*c));

	*d++ = '\0';

	if (*c && isspace(*c))
	    while (isspace(*c++));

	if (*c)
	{
	    /*cfg = d;*/
	    do {
		*d++ = *c++;
	    } while (*c && !isspace(*c));

	    *d = '0';
	}
	/*else
	    cfg = NULL;*/
    }

    theDXApplication->setBusyCursor(TRUE);
    boolean r;
    if (!theDXApplication->saveFile(net, FALSE))
    {
	DXLinkHandler *a = (DXLinkHandler*)va;
	char buffer[1024];
	if (net)
	    sprintf(buffer, "Error saving network file %s", net);
	else
	    sprintf(buffer, "Error saving network file %s",
		theDXApplication->network->getFileName());
	a->sendPacket(DXPacketIF::PKTERROR, id, buffer);
	r = FALSE;
    }
    else
    {
	r = TRUE;
    }
    theDXApplication->setBusyCursor(FALSE);

    delete buf;
    return r;
}

boolean DXLinkHandler::OpenNetworkNoReset(const char *c, int id, void *va)
{
    char *buf = new char[strlen(c)+1];
    char *d, *cfg, *net;

    if (isspace(*c))
	while (isspace(*c++));

    if (! *c)
	return TRUE;

    d = net = buf;
    do {
	*d++ = *c++;
    } while (*c && !isspace(*c));

    *d++ = '\0';

    if (*c && isspace(*c))
	while (isspace(*c++));

    if (! *c)
	cfg = NULL;
    else
    {
	cfg = d;
	do {
	    *d++ = *c++;
	} while (*c && !isspace(*c));

	*d = '0';
    }

    boolean r;
    if (!theDXApplication->openFile(net, cfg, 0))
    {
	DXLinkHandler *a = (DXLinkHandler*)va;
	char buffer[1024];
	sprintf(buffer, "Error opening network file %s", c);
	a->sendPacket(DXPacketIF::PKTERROR, id, buffer);
	r = FALSE;
    }
    else
    {
	r = TRUE;
    }

    delete buf;
    return r;
}

boolean DXLinkHandler::OpenConfig(const char *c, int id, void *va)
{
    boolean r;

    theDXApplication->setBusyCursor(TRUE);
    if (!theDXApplication->network->openCfgFile(c, TRUE))
    {
	DXLinkHandler *a = (DXLinkHandler*)va;
	char buffer[1024];
	sprintf(buffer, "Error openning configuration file %s", c);
	a->sendPacket(DXPacketIF::PKTERROR, id, buffer);
	r = FALSE;
    }
    else
	r = TRUE;
    theDXApplication->setBusyCursor(FALSE);

    return r;
}

boolean DXLinkHandler::StallUntil(const char *c, int id, void *va)
{
    DXLinkHandler *a = (DXLinkHandler*)va;
    PacketIF *pif = a->getPacketIF();

    if (getenv("DX_STALL") != NULL)
	return FALSE;

    if (pif->isPacketHandlingStalled())
	return FALSE;

    if (strstr(c, "idle"))  {
	pif->stallPacketHandling(DXLinkHandler::DestallOnNoExecution,
                                theDXApplication->getExecCtl());
    } else if (strstr(c,"count")) {
	int count = 0;
	if (sscanf(c,"count %d",&count) == 1) {
	    a->stallCount = count;
	    pif->stallPacketHandling(DXLinkHandler::StallNTimes,a);
	}
    }
    return TRUE;
}

boolean DXLinkHandler::StallNTimes(void *data)
{
    DXLinkHandler *dlh = (DXLinkHandler*)data; 

    if (getenv("DX_STALL") != NULL)
	return TRUE;

    ASSERT(dlh->stallCount > 0);
    dlh->stallCount--;
    return dlh->stallCount == 0;
}

boolean DXLinkHandler::DestallOnNoExecution(void *data)
{
    DXExecCtl *ctl  = (DXExecCtl*)data;

    if (getenv("DX_STALL") != NULL)
	return TRUE;

    return !ctl->isExecuting();
}

//
// Don't handle packets of the packet interface associated with this
// LinkHandler until a packet comes across the primary packet interface
// (to the exec) that says it is done executing. 
//
void DXLinkHandler::stallForExecutionIfRequired()
{
    PacketIF *pif = this->getPacketIF();

    if (getenv("DX_STALL") != NULL)
	return;

    if ((pif != theDXApplication->getPacketIF()) && 
	!pif->isPacketHandlingStalled())
	pif->stallPacketHandling(DestallOnNoExecution,
				theDXApplication->getExecCtl());
}


boolean DXLinkHandler::SaveConfig(const char *c, int id, void *va)
{
    boolean r;
    theDXApplication->setBusyCursor(TRUE);
    if (!theDXApplication->network->saveCfgFile(c))
    {
	DXLinkHandler *a = (DXLinkHandler*)va;
	char buffer[1024];
	sprintf(buffer, "Error saving configuration file %s", c);
	a->sendPacket(DXPacketIF::PKTERROR, id, buffer);
	r = FALSE;
    }
    else
	r = TRUE;
    theDXApplication->setBusyCursor(FALSE);
    return r;
}
//
// Expect in c a string with 'var = value' format.  The var is used to
// find a DXLInputNode in the main network.  If one is found with the
// same label, then set its input (and send it), otherwise, just send
// the assigment to the executive.
//
boolean DXLinkHandler::SetGlobalValue(const char *c, int id, void *va)
{
    Network *n = theDXApplication->network;
    Node *node = NULL;
    char label[128];
    int length;

    sscanf(c, "%s = %n", label, &length);
    const char *value = c+length;
    
    //
    // Look for a DXLInput tool that matches the given label. If found,
    // then set its 1st input (which is passed to its output).
    //
    List *l;
    if ( (l = n->makeClassifiedNodeList(ClassDXLInputNode,FALSE)) ) {
        ListIterator iter(*l);
	while ( (node = (Node*)iter.getNext()) ) {
	    if (EqualString(node->getLabelString(),label)) {
		node->setInputValue(1,value);
		break;
	    }
	}
	delete l;
    }

    //
    // If we didn't find a DXLInput tool, then we just send the assignment
    // to the executive.  NOTE, that this handles the case where the user
    // has used Receivers instead of DXLInput nodes, with the exception
    // that connection to the server must exist for the assigment to take
    // effect (i.e. DXLInputs save the value for a later connection).
    //
    if (!node)  {
	DXPacketIF *pif = theDXApplication->getPacketIF();
	if (pif) {
	    char *s = new char[STRLEN(value) + STRLEN(label) + 8]; 
	    sprintf(s,"%s = %s;",label,value);
	    pif->send(DXPacketIF::FOREGROUND, s);
	    delete s;
	}
    }

#if 0 // For now we will take this out so that DXLink apps can throw values at
      // the exec the same way dxui does, with the exec handling them as
      // fast as it can.  11/16/95 - dawood
    
    //
    // If we were in exec-on-change, then we just caused an execution,
    // lets be sure to wait until it is completed, before handling 
    // packets from the dxlink application.
    //
    DXLinkHandler *a = (DXLinkHandler*)va;
    if (theDXApplication->getExecCtl()->inExecOnChange())
	a->stallForExecutionIfRequired();
#endif

    return TRUE;
    
}

//
// This method is probably never used, as it is not required by the
// currently exposed functions in libDXL.a.
//
boolean DXLinkHandler::SetTabValue(const char *c, int id, void *va)
{
    char macro[100];
    char module[100];
    int  inst;
    int  paramInd;
    int  length;
    DXLinkHandler *a = (DXLinkHandler*)va;
    char inout[10];
    char buffer[1024];

    sscanf(c, "%[^_]_%[^_]_%d_%[^_]_%d = %n",
	    macro, module, &inst, inout, &paramInd, &length);
    const char *value = c+length;

    Network *n = theDXApplication->network;
    ListIterator li(theDXApplication->macroList);
    for (; n && !EqualString(n->getNameString(), macro);
	     n = (Network*)li.getNext())
	;
    if (!n)
    {
	sprintf(buffer, "Macro %s not found", macro);
	ErrorMessage(buffer);
	a->sendPacket(DXPacketIF::PKTERROR, id, buffer);
	return FALSE;
    }

    Node *node = n->getNode(module, inst);
    if (node == NULL)
    {
	sprintf(buffer,"Module %s:%d not found", module, inst);
	ErrorMessage(buffer);
	a->sendPacket(DXPacketIF::PKTERROR, id, buffer);
	return FALSE;
    }
    if ((EqualString(inout, "in") &&
	    node->setInputValue(paramInd, value) == DXType::UndefinedType) ||
	(EqualString(inout, "out") &&
	    node->setOutputValue(paramInd, value) == DXType::UndefinedType))
    {
	sprintf(buffer,"Value \"%s\" not valid for %sput %d of %s",
	    value, inout, paramInd, module);
	ErrorMessage(buffer);
	a->sendPacket(DXPacketIF::PKTERROR, id, buffer);
	return FALSE;
    }

    //
    // If we were in exec-on-change, then we just caused an execution,
    // lets be sure to wait until it is completed, before handling 
    // packets from the dxlink application.
    //
    if (theDXApplication->getExecCtl()->inExecOnChange())
	a->stallForExecutionIfRequired();

    return TRUE;
}

boolean DXLinkHandler::Terminate(const char *, int, void *)
{
    theDXApplication->disconnectFromApplication(TRUE);

    return TRUE;
}

boolean DXLinkHandler::Disconnect(const char *, int , void *)
{
    theDXApplication->disconnectFromApplication(FALSE);

    return TRUE;
}

boolean DXLinkHandler::QueryValue(const char *c, int id, void *va)
{
    char macro[100];
    char module[100];
    int  inst;
    int  paramInd;
    DXLinkHandler *a = (DXLinkHandler*)va;
    char inout[10];
    char buffer[1024];

    sscanf(c, "%[^_]_%[^_]_%d_%[^_]_%d",
	    macro, module, &inst, inout, &paramInd);

    Network *n = theDXApplication->network;
    ListIterator li(theDXApplication->macroList);
    for (; n && !EqualString(n->getNameString(), macro);
	     n = (Network*)li.getNext())
	;
    if (!n)
    {
	sprintf(buffer,"Macro %s not found", macro);
	ErrorMessage(buffer);
	a->sendPacket(DXPacketIF::PKTERROR,id, buffer); 
	return FALSE;
    }

    Node *node = n->getNode(module, inst);
    if (node == NULL)
    {
	sprintf(buffer,"Module %s:%d not found", module, inst);
	ErrorMessage(buffer);
	a->sendPacket(DXPacketIF::PKTERROR, id, buffer);
	return FALSE;
    }
    const char *value;
    if (EqualString(inout, "in"))
    {
	if (node->isInputDefaulting(paramInd))
	    value = node->getInputDefaultValueString(paramInd);
	else
	    value = node->getInputValueString(paramInd);
    }
    else
	value = node->getOutputValueString(paramInd);

    a->sendPacket(DXPacketIF::LRESPONSE, id, value);
    return TRUE;
}

boolean DXLinkHandler::ConnectToServer(const char *c, int id, void *va)
{
    int port;

    sscanf(c, "%d", &port);
    theDXApplication->connectToServer(port);

    return TRUE;
}

boolean DXLinkHandler::StartServer(const char *c, int id, void *va)
{
    theDXApplication->setBusyCursor(TRUE);
    theDXApplication->startServer();
    theDXApplication->setBusyCursor(FALSE);
    return TRUE;
}

boolean DXLinkHandler::Sync(const char *c, int id, void *va)
{
    DXLinkHandler *a = (DXLinkHandler*)va;
    a->sendPacket(DXPacketIF::LRESPONSE,id);
    return TRUE;
}
class SyncData {
   public:
        int id;
        DXLinkHandler *dxlh;
};

void DXLinkHandler::SyncCB(void *clientData, int id, void *line)
{
    SyncData *sd = (SyncData*)clientData; 
    sd->dxlh->sendPacket(DXPacketIF::LRESPONSE,sd->id);
    delete sd;
}

boolean DXLinkHandler::SyncExec(const char *, int id, void *va)
{
    DXPacketIF *pif = theDXApplication->getPacketIF();
    DXLinkHandler *a = (DXLinkHandler*)va;

    if (pif) {
	SyncData *sd = new SyncData;
	sd->id = id;
	sd->dxlh = a;
	pif->send(DXPacketIF::FOREGROUND, "Executive(\"nop\");",
					  SyncCB,
					  (void*)sd);
    } else
	a->sendPacket(DXPacketIF::PKTERROR, id, "no ui-executive connection");

    return TRUE;
}

boolean DXLinkHandler::OpenControlPanel(const char *c, int id, void *va)
{
    List *l;

    l = theDXApplication->network->makeNamedControlPanelList(c);

    if (l)
    {
	ControlPanel *cp;
	ListIterator it(*l);

	while (NULL != (cp = (ControlPanel *)it.getNext()))
	    if (!cp->isManaged()) 
		cp->manage();
        delete l;
    }
    

    return TRUE;
}

boolean DXLinkHandler::CloseControlPanel(const char *c, int id, void *va)
{
    List *l;

    l = theDXApplication->network->makeNamedControlPanelList(c);

    if (l)
    {
	ControlPanel *cp;
	ListIterator it(*l);

	while (NULL != (cp = (ControlPanel *)it.getNext()))
	    if (cp->getRootWidget())
		cp->unmanage();
    }
    
    delete l;

    return TRUE;
}

boolean DXLinkHandler::ResetServer(const char *c, int id, void *va)
{
    // DXLinkHandler *a = (DXLinkHandler*)va;
    theDXApplication->resetServer(); 
    return TRUE;
}
boolean DXLinkHandler::Version(const char *c, int id, void *va)
{
    DXLinkHandler *a = (DXLinkHandler*)va;
    char buffer[1024];
    sprintf(buffer,"UI version: %d %d %d\n",
		DX_MAJOR_VERSION, DX_MINOR_VERSION, DX_MICRO_VERSION);
    a->sendPacket(DXPacketIF::INFORMATION, id, buffer); 
    return TRUE;
}
boolean DXLinkHandler::ResendParameters(const char *c, int id, void *va)
{
    List *l;
    DXLinkHandler *a = (DXLinkHandler*)va;

    l = theDXApplication->network->makeClassifiedNodeList(NULL);
    if (! l)
    {
	a->sendPacket(DXPacketIF::PKTERROR, id, "no network?");
	return FALSE;
    }

    Node *node;
    ListIterator it(*l);
    int found = 0;

    while (NULL != (node = (Node *)it.getNext()))
	if (EqualString(c, node->getLabelString()))
	{
	    found = 1;

	    for (int i = 1; i <= node->getInputCount(); i++)
		node->setInputDirty(i);
	}
    
    if (! found)
    {
	char buffer[1024];
	sprintf(buffer, "no nodes with label %s found", c);
	a->sendPacket(DXPacketIF::PKTERROR, id, buffer);
	return FALSE;
    }

    theDXApplication->network->setDirty();

    return TRUE;
}

boolean DXLinkHandler::SetProbePoint(const char *c, int id, void *va)
{
    List *l;
    DXLinkHandler *a = (DXLinkHandler*)va;
    char probeName[100];
    float x, y, z;
    int n;

    l = theDXApplication->network->makeClassifiedNodeList(ClassProbeNode,FALSE);
    if (! l)
    {
	a->sendPacket(DXPacketIF::PKTERROR, id, "no probes?");
	return FALSE;
    }

    sscanf(c, "%s %d [%f, %f, %f]", probeName, &n, &x, &y, &z);

    n = n == 1 ? -1 : 0;

    ListIterator it(*l);
    ProbeNode *pn;

    while (NULL != (pn = (ProbeNode *)it.getNext()))
	if (EqualString(probeName, pn->getLabelString()))
	{
	    pn->setCursorValue(n, (double)x, (double)y, (double)z);
	    break;
	}

    //
    // If we were in exec-on-change, then we just caused an execution,
    // lets be sure to wait until it is completed, before handling 
    // packets from the dxlink application.
    //
    if (theDXApplication->getExecCtl()->inExecOnChange())
	a->stallForExecutionIfRequired();
    
    return TRUE;
}

boolean DXLinkHandler::SelectProbe(const char *c, int id, void *va)
{
    List *images = NUL(List *);
    List *probes = NUL(List *);
    char arg0[256], arg1[256];
    int n;
    DXLinkHandler *a = (DXLinkHandler*)va;

    n = sscanf(c, "%s %s", arg0, arg1);

    if (n == 0)
    {
	a->sendPacket(DXPacketIF::PKTERROR, 
				id, "missing probe name"); 
	return FALSE;
    }

    if (EqualSubstring(arg0, "probe=", 6))
	probes = MakeQualifiedNodeList(ClassProbeNode, arg0+6, FALSE);
    else
	probes = MakeQualifiedNodeList(ClassProbeNode, arg0, FALSE);
    
    if (! probes || 0 == probes->getSize())
    {
	a->sendPacket(DXPacketIF::PKTERROR, id,
		"SelectProbe: no probes in network");
	if (probes) delete probes;
	return FALSE;
    }
    
    if (n == 2)
    {
	if (EqualSubstring(arg0, "image", 5))
	    images = MakeQualifiedNodeList(ClassImageNode, arg1+6, FALSE);
	else
	    images = MakeQualifiedNodeList(ClassImageNode, arg1, FALSE);
    }
    else
	images =
	  theDXApplication->network->makeClassifiedNodeList(ClassImageNode);

    if (! images || 0 == images->getSize())
    {
	char buffer[1024];
	if (n == 1) {
	    a->sendPacket(DXPacketIF::PKTERROR, id,
		"SelectProbe: no images in network");
	} else {
	    sprintf(buffer, 
		"SelectProbe: no images matching %s in network", arg1);
	    a->sendPacket(DXPacketIF::PKTERROR, id, buffer);
	}
	if (images) delete images;
	if (probes) delete probes;
	return FALSE;
    }

    ListIterator itp(*probes);
    ProbeNode *pn;

    while (NULL != (pn = (ProbeNode *)itp.getNext()))
    {
	int pi = pn->getInstanceNumber();
	ListIterator iti(*images);
	ImageNode *in;

	while (NULL != (in = (ImageNode *)iti.getNext()))
	{
	    ImageWindow *iw = (ImageWindow *)in->getDXWindow();
	    iw->selectProbeByInstance(pi);
	}
    }

    if (images) delete images;
    if (probes) delete probes;

    //
    // If we were in exec-on-change, then we just caused an execution,
    // lets be sure to wait until it is completed, before handling 
    // packets from the dxlink application.
    //
    if (theDXApplication->getExecCtl()->inExecOnChange())
	a->stallForExecutionIfRequired();

    return TRUE;
}

boolean DXLinkHandler::SetInteractionMode(const char *c, int id, void *va)
{
    List *l = NUL(List *);
    char qualifier0[256];
    char qualifier1[256];
    char mode[256];
    int n;
    DXLinkHandler *a = (DXLinkHandler*)va;

    n = sscanf(c, "%s %s %s", mode, qualifier0, qualifier1);

    if (n >= 2)
    {
	if (EqualSubstring(qualifier0, "image", 5))
	    l = MakeQualifiedNodeList(ClassImageNode, qualifier0+6, FALSE);
	else
	    l = MakeQualifiedNodeList(ClassImageNode, qualifier0, FALSE);
	
	if (!l || l->getSize() == 0)
	{
	    char buffer[1024];
	    sprintf(buffer,
			"SetInteractionMode: no images matching %sin network",
		 	qualifier0);
	    a->sendPacket(DXPacketIF::PKTERROR,id, buffer);
	    if (l) delete l;
	    return FALSE;
	}
    }
    else
    {
	l = 
	  theDXApplication->network->makeClassifiedNodeList(ClassImageNode);

	if (!l || l->getSize() == 0)
	{
	    a->sendPacket(DXPacketIF::PKTERROR,id, 
			"SetInteractionMode: no images in network");
	    if (l) delete l;
	    return FALSE;
	}
    }

    ListIterator it(*l);
    ImageNode *in;
    DirectInteractionMode dim;

    if      (EqualString(mode, "camera"))   dim = CAMERA;
    else if (EqualString(mode, "cursors"))  dim = CURSORS;
    else if (EqualString(mode, "pick"))     dim = PICK;
    else if (EqualString(mode, "navigate")) dim = NAVIGATE;
    else if (EqualString(mode, "panzoom"))  dim = PANZOOM;
    else if (EqualString(mode, "roam"))     dim = ROAM;
    else if (EqualString(mode, "rotate"))   dim = ROTATE;
    else if (EqualString(mode, "zoom"))     dim = ZOOM;
    else dim = NONE;

    while (NULL != (in = (ImageNode *)it.getNext()))
    {
	ImageWindow *iw = (ImageWindow*)in->getDXWindow();
	if (!iw->setInteractionMode(dim)) {
	    char buffer[1024];
	    sprintf(buffer, "SetInteractionMode: could not set '%s' mode",mode);
	    a->sendPacket(DXPacketIF::PKTERROR,id, buffer);
	}
    }

    if ((dim == CURSORS || dim == PICK) && n >= 3)
    {
	List *pl;
	char *q;
	int instance = -1;
	const char *clss = (dim == PICK) ? ClassPickNode : ClassProbeNode;

	it.setPosition(1);

	if (EqualSubstring(qualifier1, "probelist", 9))
	    q = qualifier1 + 10;
	else if (EqualSubstring(qualifier1, "probe", 5))
	    q = qualifier1 + 6;
	else if (EqualSubstring(qualifier1, "pick", 4))
	    q = qualifier1 + 5;
	else
	    q = qualifier1;

	pl = MakeQualifiedNodeList(clss, q, FALSE);
	
	if (!pl || pl->getSize() == 0)
	{
	    char buffer[1024];
	    sprintf(buffer,
		"SetInteractionMode: no %s named %s in network", 
		   (dim == PICK) ? "pick node" : "probe or probe list node", q);
	    a->sendPacket(DXPacketIF::PKTERROR,id, buffer);
	    if (l) delete l;
	    return FALSE;
	}

	ListIterator plit(*pl);
	Node *nd = (Node *)plit.getNext();
	instance = nd->getInstanceNumber();

	while (NULL != (in = (ImageNode *)it.getNext()))
	{
	    ImageWindow *iw = (ImageWindow*)in->getDXWindow();
	    if (((dim == PICK) && !iw->selectProbeByInstance(instance)) ||
		((dim == PICK) && !iw->selectPickByInstance(instance)))
	    {
		char buffer[1024];
		sprintf(buffer,
		    "SetInteractionMode: could not current %s to '%s'",
		       (dim == PICK) ? "pick" : "probe or probe list", q);
		a->sendPacket(DXPacketIF::PKTERROR,id, buffer);
	    }
	}

	delete pl;
    }
    
    delete l;
    //
    // If we were in exec-on-change, then we just caused an execution,
    // lets be sure to wait until it is completed, before handling 
    // packets from the dxlink application.
    //
    if (theDXApplication->getExecCtl()->inExecOnChange())
	a->stallForExecutionIfRequired();
    return TRUE;
}

boolean DXLinkHandler::LoadMacroFile(const char *c, int id, void *va)
{
    char *msg;

    if (! MacroDefinition::LoadMacro(c, &msg))
    {
	DXLinkHandler *a = (DXLinkHandler*)va;
	char buffer[1024];
	sprintf(buffer, "LoadMacroFile: %s", msg);
	a->sendPacket(DXPacketIF::PKTERROR, id, buffer);
	delete msg;
	return FALSE;
    }

    return TRUE;
}


boolean DXLinkHandler::LoadMacroDirectory(const char *c, int id, void *va)
{
    char *msg;

    if (! MacroDefinition::LoadMacroDirectories(c, TRUE, &msg))
    {
	DXLinkHandler *a = (DXLinkHandler*)va;
	char buffer[1024];
	sprintf(buffer, "LoadMacroDirectory: %s", msg);
	a->sendPacket(DXPacketIF::PKTERROR, id, buffer);
	delete msg;
	return FALSE;
    }

    return TRUE;
}

boolean DXLinkHandler::ExecOnce(const char *c, int id, void *va)
{
    theDXApplication->getExecCtl()->executeOnce();
    DXLinkHandler *a = (DXLinkHandler*)va;
    a->stallForExecutionIfRequired();
    return TRUE;
}

boolean DXLinkHandler::ExecOnChange(const char *c, int id, void *va)
{
    theDXApplication->getExecCtl()->enableExecOnChange();
    DXLinkHandler *a = (DXLinkHandler*)va;
    a->stallForExecutionIfRequired();
    return TRUE;
}

boolean DXLinkHandler::EndExecution(const char *c, int id, void *va)
{
    DXExecCtl *ctl = theDXApplication->getExecCtl();
    DXLinkHandler *a = (DXLinkHandler*)va;

    // this kills the current graph after the next module finishes
    ctl->terminateExecution();
    a->stallForExecutionIfRequired();

    return TRUE;
}

boolean DXLinkHandler::EndExecOnChange(const char *c, int id, void *va)
{
    DXExecCtl *ctl = theDXApplication->getExecCtl();
    DXLinkHandler *a = (DXLinkHandler*)va;

    // this just takes the exec out of EOC mode, and lets the current
    // graph complete.
    ctl->endExecOnChange();
    a->stallForExecutionIfRequired();

    return TRUE;
}

boolean DXLinkHandler::QueryExecution(const char *c, int id, void *va)
{
    int execState;
    DXLinkHandler *a = (DXLinkHandler*)va;
    char buffer[1024];

    if (theDXApplication->getExecCtl()->isExecuting())
        execState = 1;
    else
	execState = 0;

    sprintf(buffer, "execution state: %d", execState);
    a->sendPacket(DXPacketIF::LRESPONSE, id, buffer);
    
    return TRUE;
}


boolean DXLinkHandler::PopupVCR(const char *c, int id, void *va)
{
    fprintf(stderr, "PopupVCR not implemented\n");
    return TRUE;
}


boolean DXLinkHandler::SequencerPlay(const char *line, int id, void *va)
{
    SequencerNode *s  = theDXApplication->network->sequencer;
    if (s) {
	DXLinkHandler *a = (DXLinkHandler*)va;
	DXExecCtl *ctl = theDXApplication->getExecCtl();    
	boolean forward = strstr(line,"forward") != NULL;
	ctl->vcrExecute(forward);
	if (!s->isLoopMode())
	    a->stallForExecutionIfRequired();
    }
    return TRUE;
}
boolean DXLinkHandler::SequencerPause(const char  *line, int id, void *va)
{
    SequencerNode *s  = theDXApplication->network->sequencer;
    if (s) {
	DXLinkHandler *a = (DXLinkHandler*)va;
	DXExecCtl *ctl = theDXApplication->getExecCtl();    
	ctl->vcrCommand(VCR_PAUSE, FALSE);
	a->stallForExecutionIfRequired();
    }
    return TRUE;
}
boolean DXLinkHandler::SequencerStep(const char  *line, int id, void *va)
{
    SequencerNode *s  = theDXApplication->network->sequencer;
    if (s) {
	DXLinkHandler *a = (DXLinkHandler*)va;
	DXExecCtl *ctl = theDXApplication->getExecCtl();    
	ctl->vcrCommand(VCR_STEP, FALSE);
	a->stallForExecutionIfRequired();
    }
    return TRUE;
}
boolean  DXLinkHandler::SequencerStop(const char  *line, int id, void *va)
{
    SequencerNode *s  = theDXApplication->network->sequencer;
    if (s) {
	DXLinkHandler *a = (DXLinkHandler*)va;
	DXExecCtl *ctl = theDXApplication->getExecCtl();    
	ctl->vcrCommand(VCR_STOP, FALSE);
	a->stallForExecutionIfRequired();
    }
    return TRUE;
}
boolean  DXLinkHandler::SequencerPalindrome(const char  *line, int id, void *va)
{
    SequencerNode *s  = theDXApplication->network->sequencer;
    if (s) {
	DXLinkHandler *a = (DXLinkHandler*)va;
	DXExecCtl *ctl = theDXApplication->getExecCtl();    
	boolean set = strstr(line,"off") == NULL;
	ctl->vcrCommand(VCR_PALINDROME, set);
	a->stallForExecutionIfRequired();
    }
    return TRUE;
}
boolean  DXLinkHandler::SequencerLoop(const char  *line, int id, void *va)
{
    SequencerNode *s  = theDXApplication->network->sequencer;
    if (s) {
	DXExecCtl *ctl = theDXApplication->getExecCtl();    
	DXLinkHandler *a = (DXLinkHandler*)va;
	boolean set = strstr(line,"off") == NULL;
	ctl->vcrCommand(VCR_LOOP, set);
	a->stallForExecutionIfRequired();
    }
    return TRUE;
}



DXLinkHandler::DXLinkHandler(PacketIF *pif) : LinkHandler(pif)
{
    this->addCommand("set", (Command *)NULL);
    this->addSubCommand("set", "tab", SetTabValue);
    this->addSubCommand("set", "value", SetGlobalValue);
    this->addSubCommand("set", "probevalue", SetProbePoint);
    this->addSubCommand("set", "activeprobe", SelectProbe);


    this->addCommand("execute", (Command *)NULL);
    this->addSubCommand("execute", "once",
		DXLinkHandler::ExecOnce);
    this->addSubCommand("execute", "onchange",
		DXLinkHandler::ExecOnChange);
    this->addSubCommand("execute", "end",
		DXLinkHandler::EndExecution);
    this->addSubCommand("execute", "endEOC",
		DXLinkHandler::EndExecOnChange);

    this->addCommand("StartServer", DXLinkHandler::StartServer);
    this->addCommand("ConnectToServer", DXLinkHandler::ConnectToServer);
    this->addCommand("disconnect", DXLinkHandler::Disconnect);
    this->addCommand("exit", DXLinkHandler::Terminate);
    this->addCommand("quit", DXLinkHandler::Terminate);

    this->addCommand("save", (Command *)NULL);
    this->addSubCommand("save", "network",
				DXLinkHandler::SaveNetwork);

    this->addCommand("open", (Command *)NULL);
    this->addSubCommand("open", "networkNoReset",
				DXLinkHandler::OpenNetworkNoReset);
    this->addSubCommand("open", "network",
				DXLinkHandler::OpenNetwork);
    this->addSubCommand("open", "config",
				DXLinkHandler::OpenConfig);
    this->addSubCommand("open", "controlpanel",
				DXLinkHandler::OpenControlPanel);
    this->addSubCommand("open", "VPE",
				DXLinkHandler::OpenVPE);
    this->addSubCommand("open", "sequencer",
				DXLinkHandler::OpenSequencer);
    this->addSubCommand("open", "image",
				DXLinkHandler::OpenImage);
    this->addSubCommand("open", "colormapEditor",
				DXLinkHandler::OpenColormapEditor);

    this->addCommand("query", (Command *)NULL);
    this->addSubCommand("query", "value", DXLinkHandler::QueryValue);
    this->addSubCommand("query", "sync", DXLinkHandler::Sync);
    this->addSubCommand("query", "sync-exec", DXLinkHandler::SyncExec);
    this->addSubCommand("query", "execution", DXLinkHandler::QueryExecution);

    this->addCommand("close", (Command *)NULL);
    this->addSubCommand("close", "controlpanel",
				DXLinkHandler::CloseControlPanel);
    this->addSubCommand("close", "VPE",
				DXLinkHandler::CloseVPE);
    this->addSubCommand("close", "colormapEditor",
				DXLinkHandler::CloseColormapEditor);
    this->addSubCommand("close", "image",
				DXLinkHandler::CloseImage);
    this->addSubCommand("close", "sequencer",
				DXLinkHandler::CloseSequencer);

    this->addCommand("sequencer", (Command *)NULL);
    this->addSubCommand("sequencer", "play", DXLinkHandler::SequencerPlay);
    this->addSubCommand("sequencer", "pause", DXLinkHandler::SequencerPause);
    this->addSubCommand("sequencer", "step", DXLinkHandler::SequencerStep);
    this->addSubCommand("sequencer", "stop", DXLinkHandler::SequencerStop);
    this->addSubCommand("sequencer", "palindrome", 
				DXLinkHandler::SequencerPalindrome);
    this->addSubCommand("sequencer", "loop", DXLinkHandler::SequencerLoop);

    this->addCommand("load", (Command *)NULL);
    this->addSubCommand("load", "macroFile",
				DXLinkHandler::LoadMacroFile);
    this->addSubCommand("load", "macroDirectory",
				DXLinkHandler::LoadMacroDirectory);

    this->addCommand("$version", DXLinkHandler::Version);
    this->addCommand("resend", DXLinkHandler::ResendParameters);
    this->addCommand("interactionMode", DXLinkHandler::SetInteractionMode);
    this->addCommand("reset", DXLinkHandler::ResetServer);

    this->addCommand("save", (Command *)NULL);
    this->addSubCommand("save", "config", DXLinkHandler::SaveConfig);

    this->addCommand("render-mode", (Command *)NULL);
    this->addSubCommand("render-mode", "hw", DXLinkHandler::SetHWRendering);
    this->addSubCommand("render-mode", "sw", DXLinkHandler::SetSWRendering);

    //
    // As of 6/12/95, we aren't using this in DXLink, but it may come in
    // handy for fixing bugs and may be of general utility.
    //
    this->addCommand("stall", (Command *)NULL);
    this->addSubCommand("stall", "until", DXLinkHandler::StallUntil);
}

