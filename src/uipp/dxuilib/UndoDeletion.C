/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
#include "UndoDeletion.h"
#include "EditorWindow.h"
#include "Node.h"
#include "StandIn.h"
#include "List.h"
#include "ListIterator.h"
#include "Ark.h"
#include "WarningDialogManager.h"
#include "DXApplication.h"
#include "LabelDecorator.h"

// included because we have to check for undoability
#include "SequencerNode.h"
#include "DXLOutputNode.h"

#include <stdio.h>
#include <sys/types.h>
#include <time.h>
#include <sys/stat.h>
#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif
#include <string.h> // for strerror
#include <errno.h> // for errno

char UndoDeletion::OperationName[] = "delete";

//
// Strategy:  
// 1) Select any node that is connected to a node in 'nodes', 
// 2) Use existing operations to save a .net for selected nodes,
// 3) Read in the result from the tmp file and keep it squirreled away.
// 4) Deselect nodes selected in step 1
//
// There are some conditions that make it impossible to undo.
//  1) If there is a Sequencer among the nodes that we select in order to
//  get all incident arcs on our node(s) of interest, then the operation will
//  fail because we won't be able to merge the net fragment back in.  It would
//  be aborted because only 1 sequence is permitted.
//  2) If there is a DXLOutput then we'll fail also because there will be a
//  name conflict.
//  We try to catch these but there are others:
//  1) What if you delete an Interactor, then place an Input?  (You've switched
//  over to working on a Macro.) Then you use undo?  The operation will fail
//  trying to add an interactor to a macro.
//
UndoDeletion::UndoDeletion (EditorWindow* editor, List* nodes, List* decorators) :
    UndoableAction(editor)
{
    int x,y;
    this->x = this->y = 999999;

    // must go back and unselected these nodes
    List nodes_selected;

    ListIterator iter;
    Node* n;
    if (nodes) {
	iter.setList(*nodes) ;
	while (n = (Node*)iter.getNext()) {
	    this->selectConnectedTo (n, nodes_selected);

	    StandIn* si = n->getStandIn();
	    ASSERT (si);

	    si->getXYPosition (&x, &y);
	    this->workSpace = (EditorWorkSpace*)si->getWorkSpace();
	    this->x = MIN(this->x, x);
	    this->y = MIN(this->y, y);
	}
    }

    if (decorators) {
	iter.setList(*decorators);
	LabelDecorator* dec;
	while (dec = (LabelDecorator*)iter.getNext()) {
	    dec->getXYPosition(&x,&y);
	    this->x = MIN(this->x, x);
	    this->y = MIN(this->y, y);
	    this->workSpace = (EditorWorkSpace*)dec->getWorkSpace();
	}
    }

    iter.setList(nodes_selected);
    while (n=(Node*)iter.getNext()) {
	n->getStandIn()->getXYPosition (&x, &y);
	this->x = MIN(this->x, x);
	this->y = MIN(this->y, y);
    }

    boolean is_undoable = TRUE;
    iter.setList(nodes_selected);
    while (n=(Node*)iter.getNext()) {
	if (n->isA(ClassSequencerNode)) {
	    is_undoable = FALSE;
	    break;
	} else if (n->isA(ClassDXLOutputNode)) {
	    is_undoable = FALSE;
	    break;
	}
    }

    if (is_undoable) {
	int dummy;
	this->buffer = this->editor->createNetFileFromSelection(dummy, NULL, dummy);
    } else {
	this->buffer = NUL(char*);
    }

    iter.setList(nodes_selected);
    while (n = (Node*)iter.getNext()) {
	n->getStandIn()->setSelected(FALSE);
    }
}

UndoDeletion::~UndoDeletion()
{
    if (this->buffer) delete this->buffer;
}

//
// Write out the contents in the buffer and let a new network read
// it back in.
//
void UndoDeletion::undo(boolean first_in_list) 
{
    char msg[128];
    if (!this->buffer) return ;

    //
    // Get temp file names
    //
    char net_file_name[256];
    char cfg_file_name[256];
    char directory[256];
    const char *tmpdir = theDXApplication->getTmpDirectory();
    int tmplen = STRLEN(tmpdir);
    if (!tmplen) return ;
    strcpy (directory, tmpdir);
    if (directory[tmplen-1] != '/') {
	directory[tmplen++] = '/';
	directory[tmplen] = '\0';
    }
    char* tmpFile = UniqueFilename(directory);
    tmplen = STRLEN(tmpFile);
    sprintf(net_file_name, "%s.net", tmpFile, getpid());
    sprintf(cfg_file_name, "%s.cfg", tmpFile, getpid());
    delete tmpFile;
    unlink (net_file_name);
    unlink (cfg_file_name);

#ifdef DXD_WIN
    const char* mode = "a+";
#else
    const char* mode = "a";
#endif

    //
    // Put the file contents into the files
    //
    FILE* netf;
    if ((netf = fopen(net_file_name, mode)) == NULL) {
	sprintf (msg, "Undo failed (fopen): %s", strerror(errno));
	WarningMessage(msg);
	return ;
    }
    int buflen = STRLEN(this->buffer);
    fwrite ((char*)this->buffer, sizeof(char), buflen, netf);
    fclose(netf);

    delete this->buffer;
    this->buffer = NUL(char*);

    // register the new network as the editor's pendingPaste,
    // then tell it to add the new nodes.
    if (this->editor->setPendingPaste (net_file_name, NUL(const char*), FALSE))
	this->editor->addCurrentNode (this->x, this->y, this->workSpace, TRUE);

    //if (first_in_list) this->editor->selectNode (n, TRUE, TRUE);
    unlink (net_file_name);
}

//
// How can I tell if the contents of the buffer are usable?
//
boolean UndoDeletion::canUndo()
{
    if (!this->buffer) return FALSE;
    return TRUE;
}

void UndoDeletion::selectConnectedTo (Node* n, List& nodes_selected)
{
    int input_count = n->getInputCount();
    for (int input=1; input<=input_count; input++) 
	this->selectConnectedInputs(n,input,nodes_selected);
    int output_count = n->getOutputCount();
    for (int output=1; output<=output_count; output++) 
	this->selectConnectedOutputs(n,output,nodes_selected);
}

void UndoDeletion::selectConnectedInputs (Node* n, int input, List& nodes_selected)
{
	if (!n->isInputVisible(input)) return;
	List* arcs = (List*)n->getInputArks(input);
	if (!arcs) return ;
	ListIterator iter(*arcs);
	Ark* arc;
	int dummy;
	while (arc = (Ark*)iter.getNext()) {
	    Node* source = arc->getSourceNode(dummy);
	    StandIn* si = source->getStandIn();
	    if (si->isSelected()) continue;
	    nodes_selected.appendElement(source);
	    si->setSelected(TRUE);
	}
}

void UndoDeletion::selectConnectedOutputs (Node* n, int output, List& nodes_selected)
{
	if (!n->isOutputVisible(output)) return;
	List* arcs = (List*)n->getOutputArks(output);
	if (!arcs) return ;
	ListIterator iter(*arcs);
	Ark* arc;
	int dummy;
	while (arc = (Ark*)iter.getNext()) {
	    Node* dest = arc->getDestinationNode(dummy);
	    StandIn* si = dest->getStandIn();
	    if (si->isSelected()) continue;
	    nodes_selected.appendElement(dest);
	    si->setSelected(TRUE);
	}
}
