/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#define ONLY_IN_GRAPH_LAYOUT_CODE 1
#include "GraphLayout.h"
#include "EditorWindow.h"
#include "ListIterator.h"
#include "Network.h"
#include "EditorWorkSpace.h"
#include "Node.h"
#include "StandIn.h"
#include "Ark.h"
#include "ArkStandIn.h"
#include "ErrorDialogManager.h"
#include "VPEAnnotator.h"
#include "DecoratorStyle.h"
#include "Dictionary.h"
#include <math.h>
//#include <limits.h> // for INT_MIN and INT_MAX
//
// How it works:
//
// ...in a nutshell: Traverse the graph assigning a hop count
// to each node computed as the number of hops from the top of
// the graph to reach the node.  Sort the graph by hop count.
// Among nodes that share a hop count, sort by the x coord
// of the nodes' arc's destination.  Iterate over the sorted
// lists placing nodes with output tab above input tab
// whereever possible.
//
// ...in more detail:
// - For each decorator creating a list of nodes sorted by
// increasing distance from the decorator.  This is used at
// the end of the layout so that I can place the decorator close
// to some node that it was close to before the layout started.
// - Tag every node with the number of hops required to reach
// the node from the top of the graph.  After that's computed
// I use that number * a constant to compute the node's y coord.
// For many nodes I tweak the tag after the initial computation
// because many nodes could really be represented by a variety
// of values.  Example: The max hop count from top to bottom
// of the graph is 5, but there's also an Input->Output.  The
// Input and Output nodes could be tagged 1,2 or 4,5.  Either
// is correct and it just depends on how you want things to
// look.
// - Create group data structures.  There is 1 group for every
// set of connected nodes.  In other words any 2 nodes that can
// be reached without lifting pen from paper are in the same
// group.
// - Sort the graph according to descending hop tag.
// - Perform the group's layout method starting from the 
// beginning of the sorted list.  Within the group, create
// row data structures - 1 row for all nodes that share
// a hop tag.  The set of nodes placed in the layout method
// consists of all the nodes ABOVE the root node.
// - Perform the layout at the very bottom of the graph
// trivially by placing it at INITIAL_X,INITIAL_Y.
// - Perform each subsequent row's layout by sorting the
// nodes in the row by increasing x coord of the destination
// arc. The x coord is available after the preceeding row has
// been placed.
//

//
// ToDo:
// 1) When preparing the call to qsort() in LayoutRow there are some nodes
// that have 2 destination arcs on the next lower row.  On behalf of those nodes,
// it would be nice to perform the sort and layout multiple times and measure
// the quality of the result.  Then choose the one that worked the best.
//

//
// How far should successive 'levels' of the graph be separated
//
int GraphLayout::HeightPerLevel = 90;
#define ABSOLUTE_MINIMUM_HEIGHT_PER_LEVEL 70
#define MAXIMUM_HEIGHT_PER_LEVEL 200

//
//
//
const char* GraphLayout::ErrorMessages[] = {
    "-autoLayoutHeight is outside the legal range [80 - 200]. (The default is 90.)\n",
    "-autoLayoutGroupSpacing is outside the legal range [10 - 100]. (The default is 30.)\n",
    "-autoLayoutNodeSpacing is outside the legal range [5 - 100]. (The default is 15.)\n",
    NULL
};

//
// Allow a resource to control the vertical spreading out
// If there HeightPerLevel were too small, then it would become
// impossible to lay anything out.
//
const char* GraphLayout::SetHeightPerLevel(int hpl)
{
    if ((hpl < ABSOLUTE_MINIMUM_HEIGHT_PER_LEVEL) ||(hpl > MAXIMUM_HEIGHT_PER_LEVEL)) {
	return GraphLayout::ErrorMessages[0];
    }	
    GraphLayout::HeightPerLevel = hpl;
    return NUL(const char*);
}

//
// Identify original location of a decorator with respect to its
// closes node.
//
#define DECORATOR_ABOVE 1
#define DECORATOR_BELOW 2
#define DECORATOR_LEFT  4
#define DECORATOR_RIGHT 8

//
// x distance to separate disconnected groups of nodes from each other
//
int GraphLayout::GroupSpacing = 30;
#define MINIMUM_HORIZONTAL_GROUP_SPACING 10
#define MAXIMUM_HORIZONTAL_GROUP_SPACING 100

const char* GraphLayout::SetGroupSpacing(int gs)
{
    if ((gs<MINIMUM_HORIZONTAL_GROUP_SPACING)||(gs>MAXIMUM_HORIZONTAL_GROUP_SPACING)) {
	return ErrorMessages[1];
    }
    GraphLayout::GroupSpacing = gs;
    return NUL(const char*);
}

//
// When attempting to position nodes, we always check for collisions.
// In most cases there should never be any.  Only when merging two
// chunks of the same graph can it happen, but we check for it.  If
// there would be one then we place somewhere else.  Note: if you
// perform the layout, but find the Ctrl-U can't undo because it
// produces a warning message, the problem is probably that there
// was a collision when placing nodes.  The undo feature won't move
// nodes if doing so would cause bumper cars.
// Currently unused
#define COLLISION_OFFSET 15

//
// Leave a little extra horizontal room between standins because
// crowded levels must still allow lines to pass.
// We'll allow REQUIRED_ if that will let us make straight arcs
// but if we can't make straight arcs, then we'll offset by 
// DESIRED_
//
#define REQUIRED_HSEP 5
#define MINIMUM_DESIRED_HSEP 5
#define MAXIMUM_DESIRED_HSEP 100
int GraphLayout::NodeSpacing = 15;
const char* GraphLayout::SetNodeSpacing(int ns)
{
    if ((ns < MINIMUM_DESIRED_HSEP) || (ns > MAXIMUM_DESIRED_HSEP)) {
	return ErrorMessages[2];
    }
    GraphLayout::NodeSpacing = ns;
    return NUL(const char*);
}


//
// Starting coords for graph placement.  They have no meaning.  They
// just shouldn't get negative or greater than 64K since they might
// get stored into an unsigned short.  
//
#define INITIAL_X 0//5000 
#define INITIAL_Y 0//5000

//#define DEBUG_PLACEMENT
#if defined(DEBUG_PLACEMENT)
//
// $ setenv DEBUG_PLACEMENT
// in order to use this.
//
static boolean debug = FALSE;
#endif

//
// Comparator function used with qsort.  It says we're going to sort things
// by decreasing hop count. So the things early in the list are at the bottom
// of the graph.
//
int NodeInfo::SortByHop (const void* a, const void* b)
{
    Node** aptr = (Node**)a;
    Node** bptr = (Node**)b;
    Node* anode = (Node*)*aptr;
    Node* bnode = (Node*)*bptr;
    NodeInfo* ainfo = (NodeInfo*)anode->getLayoutInformation();
    NodeInfo* binfo = (NodeInfo*)bnode->getLayoutInformation();

    // descending order of hop count
    if (ainfo->hops > binfo->hops) return -1;
    if (ainfo->hops < binfo->hops) return 1;

    //
    // If the 2 nodes connect to different outputs of the same parent,
    // then sort by that.  Different strategies to try:
    // 1) output 1 1st, output 2 2nd, etc...
    // 2) middle output 1st, 1 left 2nd, 1 right 3rd, 2 left 4th, etc...
    // 3) odd numbered outputs first, then even numbered outputs.  This
    //    might produce a few straight lines.
    //
    boolean shares_output1;
    Ark* ark1 = GraphLayout::IsSingleInputNoOutputNode(anode, shares_output1, FALSE);
    if (ark1) {
	boolean shares_output2;
	Ark* ark2 = GraphLayout::IsSingleInputNoOutputNode(bnode, shares_output2, FALSE);

	if (ark2) {
	    Node *parent1, *parent2;
	    int input1, input2;
	    parent1 = ark1->getSourceNode(input1);
	    parent2 = ark2->getSourceNode(input2);
	    if (parent1 == parent2) {
		if (input1 != input2) {
		    int output_count = parent1->getOutputCount();
		    int middle = (output_count>>1)+1;
		    int diff1 = middle - input1;
		    int diff2 = middle - input2;
		    if (diff1<0) diff1=-diff1;
		    if (diff2<0) diff2=-diff2;
		    if (diff1 < diff2) return -1;
		    if (diff2 < diff1) return 1;
		}
	    }
	}
    }

    return 0;
}

void LayoutInfo::initialize (UIComponent* uic)
{
    ASSERT(uic);
    uic->getXYPosition(&this->orig_x, &this->orig_y);
    this->x = this->y = -1;
    uic->getXYSize(&this->w, &this->h);
    this->initialized = TRUE;
}

void NodeInfo::initialize(Node* n, int hops)
{
    n->setLayoutInformation(this);
    this->hops = hops;
    this->LayoutInfo::initialize(n->getStandIn());
}

void AnnotationInfo::initialize (VPEAnnotator* dec)
{
    dec->setLayoutInformation(this);
    this->LayoutInfo::initialize(dec);
}

//
// This is the entry point for automatic graph layout.  This is a public
// method called from EditorWindow.
//
boolean GraphLayout::entireGraph(WorkSpace* workSpace, const List& nodes, const List& decorators)
{
    int offset;
    List nodeList;

#   if defined(DEBUG_PLACEMENT)
    debug = (getenv("DEBUG_PLACEMENT") ? TRUE : FALSE);
#   endif

    ListIterator li;
    boolean retval = TRUE;
    Node* n;

    // exclude the nodes that aren't in the current page
    li.setList((List&)nodes);
    while ((n=(Node*)li.getNext())) {
	StandIn* si = n->getStandIn();
	if (!si) continue;
	if (si->getWorkSpace() != workSpace) continue;
	nodeList.appendElement(n);
    }

    //
    // Must store the nodes to reflow in an array because later
    // we're going to use qsort.
    //
    Node** reflow;
    int reflow_count=0;
    int reflow_size=nodeList.getSize();
    reflow = (Node**)malloc(sizeof(Node*) * reflow_size);
    li.setList(nodeList);
    while ((n=(Node*)li.getNext())) {
	reflow[reflow_count++] = n;
    }
    ASSERT (reflow_count == reflow_size);

    //
    // 1) For each node, record the hops required to reach the node
    //    from the top of the graph.  Optionally push nodes up or
    //    down as aesthetics require.
    //
    this->computeHopCounts(reflow, reflow_count);

    //
    // 2) Set up associations between annotations and nodes by finding
    // each annotation's nearest node.  This happens after computeHopCounts()
    // because we need to use the LayoutInfo that computeHopCounts() sets
    // up.
    //
    List decorators_in_current_page;
    int widest_decorator = 0;
    int tallest_decorator = 0;
    this->prepareAnnotationPlacement(decorators_in_current_page, 
	    reflow, reflow_count, decorators, workSpace, widest_decorator,
	    tallest_decorator);

    //
    // 3) Sort the list in descending order of hop,ancestor counts.
    //    Internal nodes are placed according to the order in which
    //    they're wired to inputs.  
    //
    qsort (reflow, reflow_count, sizeof(Node*), NodeInfo::SortByHop);

    //
    // 4) Group the nodes.  Any 2 nodes that have a path between them
    //      go into the same group.
    //
    int next;
    for (next=0; next<reflow_count; next++) {
	Node* n = reflow[next];
	NodeInfo* ni = (NodeInfo*)n->getLayoutInformation();
	List* connected = ni->getConnectedNodes(n);
	ListIterator iter(*connected);
	if (ni->getLayoutGroup()) continue;
	LayoutGroup* group = new LayoutGroup(this->layout_groups.getSize());
	this->layout_groups.appendElement(group);
	while ((n=(Node*)iter.getNext())) {
	    NodeInfo* info = (NodeInfo*)n->getLayoutInformation();
	    info->setLayoutGroup(group);
	}
    }
#   if defined(DEBUG_PLACEMENT)
    if (debug) {
	for (next=0; next<reflow_count; next++) {
	    Node* n = reflow[next];
	    NodeInfo* ni = (NodeInfo*)n->getLayoutInformation();
	    LayoutGroup* group = ni->getLayoutGroup();
	    ASSERT (group);
	    fprintf (stdout, "%s[%d] %s hops=%d in group %d\n",
		__FILE__,__LINE__,n->getNameString(), ni->getGraphDepth(), 
		group->getId());
	}
    }
#   endif

    //
    // 5) For the first node in the list, walk up the graph placing nodes.
    //
    n = reflow[0];

#   if defined(DEBUG_PLACEMENT)
    if (debug)
	fprintf (stdout, "%s[%d] %s placing\n", __FILE__,__LINE__,n->getNameString());
#   endif

    List positioned;
    NodeInfo* linfo = (NodeInfo*)n->getLayoutInformation();
    LayoutGroup* group = linfo->getLayoutGroup();
    group->layout(n, this, positioned);
    this->repositionNewPlacements (n, TRUE, positioned);

    //
    // 6) For each subsequent unmarked node in the list, walk up the graph
    //    until hitting a marked node.  
    //
    for (next=1 ; next<reflow_count; next++) {
	n = reflow[next];
	linfo = (NodeInfo*)n->getLayoutInformation();
	if (linfo->isPositioned()) continue;


#       if defined(DEBUG_PLACEMENT)
	if (debug)
	    fprintf (stdout, "%s[%d] %s placing...", 
		__FILE__,__LINE__,n->getNameString());
#       endif

	//
	// Locate a connected, positioned node so that we can
	// get the right y coordinate.
	//
	List* connected = linfo->getConnectedNodes(n);
	ListIterator iter(*connected);
	Node* node;
	boolean separated = TRUE;
	int y;
	while ((node=(Node*)iter.getNext())) {
	    NodeInfo* ninfo = (NodeInfo*)node->getLayoutInformation();
	    if (!ninfo->isPositioned()) continue;
	    int dummy;
	    ninfo->getXYPosition (dummy, y);
	    separated = FALSE;
	    int dd = ninfo->getGraphDepth() - linfo->getGraphDepth();
	    y-= (dd*GraphLayout::HeightPerLevel);
	    break;
	}

	if (!separated) {
#           if defined(DEBUG_PLACEMENT)
	    if (debug) fprintf (stdout, " [%d] connected\n", __LINE__);
#           endif
	    // The value 20000 isn't meaningful.  Its purpose is to
	    // keep new node placments clear of existing node placements 
	    // because we want the new ones to get a decent arrangement
	    // without colliding with others in the group.  After we've
	    // finished this section of the graph, then we'll take all
	    // the nodes just placed and move them as a group, to a better
	    // location relative to the others in their group.
	    linfo->setProposedLocation (20000, y);
	} else {
#           if defined(DEBUG_PLACEMENT)
	    if (debug) fprintf (stdout, " [%d] disconnected", __LINE__);
#           endif
	    // There was no positioned connected node. We are
	    // be working on a disconnected subgraph.  
	}
	positioned.clear();
	group = linfo->getLayoutGroup();
	group->layout (n, this, positioned);

	this->repositionNewPlacements(n, separated, positioned);
#       if defined(DEBUG_PLACEMENT)
	if (debug) fprintf (stdout, "\n");
#       endif
    }

    //
    // 7) All nodes have now been divied up into groups.  Within groups,
    // the nodes have been positioned.  Each group is a set of nodes with
    // connections internal to the group.  Now it's time to position each
    // group.  This should be accomplished with respect shown to the arrangement
    // of the groups within the canvas before automatic layout was kicked
    // off.  This is what gives authors a measure of control over the 
    // appearance of the result.
    // 
    this->repositionGroups(reflow, reflow_count);

    //
    // 9) To apply placmenets, compute the bounding box of the nodes'
    //    placments, then translate so that all nodes have positive x,y
    //    and so that the center of the graph is near the center of the
    //    vpe if possible.
    //
    int minx, miny, maxx, maxy;
    this->computeBoundingBox (reflow, reflow_count, minx, miny, maxx, maxy);

    workSpace->beginManyPlacements();

    //
    // The curtain is almost ready to go up.  We must adjust the location of all
    // nodes to be close to the upper left corner of the canvas.  The vpe annotation
    // still hasn't been placed, so we'll allow some extra room so that the annotation
    // can be placed conveniently.
    //
    offset = MIN(widest_decorator, 200);
    offset = MAX(offset, 10);
    minx-= offset;
    offset = MIN(tallest_decorator, 200);
    offset = MAX(offset, 10);
    miny-= offset;

    int l;
    for (l=0; l<reflow_count; l++) {
	int x,y;
	Node* n = reflow[l];
	LayoutInfo* linfo = (LayoutInfo*)n->getLayoutInformation();
	linfo->getXYPosition (x, y);
	x-= minx;
	y-= miny;
	linfo->setProposedLocation (x, y);
	// By uncommenting this line, I'm able to verify that everything
	// worked as planned.  I like this information but I don't
	// what it shown to users.
	//if (linfo->collided()) y+=COLLISION_OFFSET;
	this->editor->saveLocationForUndo(n->getStandIn(), FALSE, (boolean)l);
	n->setVpePosition (x,y);
    }

    this->repositionDecorators(decorators_in_current_page, (boolean)l, reflow, reflow_count);
    
    //
    // 10) Rebuild all the arcs by fetching the workspace data structure,
    //    destroying it and creating a new one.  The workspace lines can
    //    be created and destroyed but never modified.
    //
    int k;
    for (k=0; k<reflow_count; k++) {
	Node* n = reflow[k];
 	int input_count = n->getInputCount();
	StandIn* si = n->getStandIn();
	int j;
	for (j=1; j<=input_count; j++) {
	    if (!n->isInputVisible(j)) continue;
	    List* arcs = (List*)n->getInputArks(j);
	    Ark* arc;
	    ListIterator iter(*arcs);
	    while ((arc=(Ark*)iter.getNext())) {
		ArkStandIn *asi = arc->getArkStandIn();
		delete asi;
		si->addArk (this->editor, arc);
	    }
	}
    }
    workSpace->endManyPlacements();

cleanup:
    //
    // Each node will delete its NodeInfo
    //
    int i;
    for (i=0; i<reflow_count; i++) {
    	Node* n = reflow[i];
    	// deletes the layout information
    	n->setLayoutInformation(NULL);
    }
    free (reflow);

    //
    // erase all the group records
    //
    ListIterator iter(this->layout_groups);
    LayoutGroup* g;
    while ((g=(LayoutGroup*)iter.getNext())) delete g;
    this->layout_groups.clear();

    //
    // each decorator will deletes its AnnotationInfo
    //
    VPEAnnotator* dec;
    iter.setList(decorators_in_current_page);
    while ((dec=(VPEAnnotator*)iter.getNext())) {
	dec->setLayoutInformation(NUL(AnnotationInfo*));
    }
    return retval;
}

void GraphLayout::unmarkAllNodes(Node* reflow[], int reflow_count)
{
    int i;
    for (i=0; i<reflow_count; i++)
	reflow[i]->clearMarked();
}
boolean GraphLayout::hasConnectedInputs(Node *n)
{
    int input_count = n->getInputCount();
    int h;
    for (h=1; h<=input_count; h++) {
	if (!n->isInputVisible(h)) continue;
	List* inputs = (List*)n->getInputArks(h);
	if (!inputs) continue;
	if (!inputs->getSize()) continue;
	return TRUE;
    }
    return FALSE;
}

boolean GraphLayout::hasConnectedOutputs(Node *n, Node* other_than)
{
    int dummy;
    int output_count = n->getOutputCount();
    int h;
    for (h=1; h<=output_count; h++) {
	if (!n->isOutputVisible(h)) continue;
	List* outputs = (List*)n->getOutputArks(h);
	if (!outputs) continue;
	if (!outputs->getSize()) continue;
	if (other_than) {
	    boolean others_found = FALSE;
	    ListIterator iter(*outputs);
	    Ark* arc;
	    while ((arc=(Ark*)iter.getNext())) {
		Node* destination = arc->getDestinationNode(dummy);
		if (destination != other_than) {
		    others_found = TRUE;
		}
	    }
	    if (others_found) 
		return TRUE;
	    else
		continue;
	}
	return TRUE;
    }
    return FALSE;
}

//
// Returns true if the destination location is empty.  We need this because
// placing one StandIn on top of another leads to bumper cars.
//
boolean GraphLayout::nodeCanMoveTo (Node* n, int x, int y)
{
    NodeInfo* info = (NodeInfo*)n->getLayoutInformation();
    List* connected_nodes = info->getConnectedNodes(n);

    int width, height;
    info->getXYSize(width, height);

    ListIterator iter(*connected_nodes);
    Node* target;
    int tw, th, tx, ty;
    while ((target=(Node*)iter.getNext())) {
	NodeInfo* target_info = (NodeInfo*)target->getLayoutInformation();
	if (target_info == info) continue;
	if (!target_info->isPositioned()) continue;
	target_info->getXYPosition (tx, ty);
	target_info->getXYSize(tw,th);
	if ((x > (tx+tw)) || (y > (ty+th))) continue;
	if (((x+width) < tx) || ((y+height) < ty)) continue;
#       if defined(DEBUG_PLACEMENT)
	if (debug) {
	    fprintf (stdout, "%s[%d] %s movement to %d,%d conflicted with %s\n",
		    __FILE__,__LINE__,n->getNameString(), x,y,target->getNameString());
	}
#       endif
	return FALSE;
    }
    return TRUE;
}

boolean GraphLayout::CanMoveTo (LayoutInfo* info, int x, int y, Node* reflow[], int count, List* decorators)
{
    int width, height;
    info->getXYSize(width, height);

    int tw, th, tx, ty;
    int i;
    for (i=0; i<count; i++) {
	Node* target = reflow[i];
	NodeInfo* target_info = (NodeInfo*)target->getLayoutInformation();
	if (target_info == info) continue;
	if (!target_info->isPositioned()) continue;
	target_info->getXYPosition (tx, ty);
	target_info->getXYSize(tw,th);
	if ((x > (tx+tw)) || (y > (ty+th))) continue;
	if (((x+width) < tx) || ((y+height) < ty)) continue;
	return FALSE;
    }
    if (decorators) {
	ListIterator iter(*decorators);
	LayoutInfo* target_info;
	VPEAnnotator* dec;
	while ((dec=(VPEAnnotator*)iter.getNext())) {
	    target_info = (LayoutInfo*)dec->getLayoutInformation();
	    if (target_info == info) continue;
	    if (!target_info->isPositioned()) continue;
	    target_info->getXYPosition (tx, ty);
	    target_info->getXYSize(tw,th);
	    if ((x > (tx+tw)) || (y > (ty+th))) continue;
	    if (((x+width) < tx) || ((y+height) < ty)) continue;
	    return FALSE;
	}
    }
    return TRUE;
}

//
// Compute a location for the source node of arc.  The location should
// result in a straight wire from the source's output tab to the 
// destination's input tab.
//
boolean GraphLayout::positionSourceOverDest (Ark* arc, int& x, int& y)
{
    int input, output;
    int dx,dy, sx, sy;
    int xincr,yincr;
    Node* destination = arc->getDestinationNode(input);
    Node* source = arc->getSourceNode(output);
    NodeInfo* src_info = (NodeInfo*)source->getLayoutInformation();
    NodeInfo* dst_info = (NodeInfo*)destination->getLayoutInformation();
    StandIn* src_si = source->getStandIn();
    StandIn* dst_si = destination->getStandIn();
    ASSERT (dst_info->isPositioned());
    dst_info->getXYPosition (dx,dy);
    int dst_tab_x = dst_si->getInputParameterTabX(input);
    int src_tab_x = src_si->getOutputParameterTabX(output);
    sx = dx + (dst_tab_x - src_tab_x);
    sy = dy + (src_info->getGraphDepth()-dst_info->getGraphDepth()) * 
	      GraphLayout::HeightPerLevel;

    boolean collided = (this->nodeCanMoveTo(source, sx,sy) == FALSE);

    x = sx;
    y = sy;
    return collided;
}


//
// Position the standIn so that the tab used by arc will have its line
// at the the specified x coord.  
//
boolean GraphLayout::positionSource (Ark* arc, int xcoord_of_arc)
{
    int input,dummy,dx,dy, sx, sy;
    Node* destination = arc->getDestinationNode(dummy);
    Node* source = arc->getSourceNode(input);
    NodeInfo* src_info = (NodeInfo*)source->getLayoutInformation();
    NodeInfo* dst_info = (NodeInfo*)destination->getLayoutInformation();
    StandIn* src_si = source->getStandIn();
    ASSERT (dst_info->isPositioned());
    dst_info->getXYPosition (dummy,dy);
    int half_tab_width = 9;
    sy = dy + (src_info->getGraphDepth()-dst_info->getGraphDepth()) * 
	      GraphLayout::HeightPerLevel;

    int src_tab_x = src_si->getOutputParameterTabX(input);
    sx = xcoord_of_arc - (src_tab_x + half_tab_width);

    src_info->setProposedLocation (sx,sy);
    return (this->nodeCanMoveTo(source, sx,sy) == FALSE);
}

boolean GraphLayout::computeBoundingBox (Node* reflow[], int reflow_count,
	int& minx, int& miny, int& maxx, int& maxy, List* decorators)
{
    boolean valid_information = FALSE;
    minx =  miny = 99999999;
    maxx = maxy = -99999999;
    int x,y,w,h;
    int i;
    for (i=0; i<reflow_count; i++) {
	Node* n = reflow[i];
	LayoutInfo *li = (LayoutInfo*)n->getLayoutInformation();
	ASSERT (li->isPositioned());
	li->getXYPosition (x,y);
	li->getXYSize(w,h);
	if (x < minx) minx = x;
	if (y < miny) miny = y;
	if ((x+w) > maxx) maxx = x+w;
	if ((y+h) > maxy) maxy = y+h;
	valid_information = TRUE;
    }
    if (decorators) {
	ListIterator iter(*decorators);
	VPEAnnotator* dec;
	while ((dec=(VPEAnnotator*)iter.getNext())) {
	    LayoutInfo* info = (LayoutInfo*)dec->getLayoutInformation();
	    info->getXYPosition (x,y);
	    info->getXYSize(w,h);
	    if (x < minx) minx = x;
	    if (y < miny) miny = y;
	    if ((x+w) > maxx) maxx = x+w;
	    if ((y+h) > maxy) maxy = y+h;
	}
    }
    return valid_information;
}

boolean GraphLayout::computeBoundingBox (List& nodes, 
	int& minx, int& miny, int& maxx, int& maxy)
{
    boolean valid_information = FALSE;
    minx =  miny = 99999999;
    maxx = maxy = -99999999;
    ListIterator li(nodes);
    Node* n;
    while ((n=(Node*)li.getNext())) {
	LayoutInfo* linfo = (LayoutInfo*)n->getLayoutInformation();
	ASSERT (linfo->isPositioned());
	int x,y,w,h;
	linfo->getXYPosition(x,y);
	linfo->getXYSize(w,h);
	if (x < minx) minx = x;
	if (y < miny) miny = y;
	if ((x+w) > maxx) maxx = x+w;
	if ((y+h) > maxy) maxy = y+h;
	valid_information = TRUE;
    }
    return valid_information;
}

void GraphLayout::repositionDecorators(List& decorators, boolean sef, Node* reflow[], int reflow_count)
{
    int minx, miny, maxx, maxy;
    this->computeBoundingBox (reflow, reflow_count, minx, miny, maxx, maxy);
    AnnotationInfo::NextX = maxx+4;
    AnnotationInfo::NextY = 10;
    ListIterator decs(decorators);
    boolean same_event_flag = sef;
    int decx, decy; 
    VPEAnnotator* dec;
    while ((dec=(VPEAnnotator*)decs.getNext())) {
	this->editor->saveLocationForUndo(dec, FALSE, same_event_flag);
	same_event_flag = TRUE;
	AnnotationInfo* ainfo = (AnnotationInfo*)dec->getLayoutInformation();
	ainfo->reposition(reflow, reflow_count, decorators);
	ainfo->getXYPosition (decx, decy);
	dec->setXYPosition (decx, decy);
    }
}

boolean GraphLayout::IsSingleOutputNoInputNode (Node* n)
{
    int i;
    int output_count = n->getOutputCount();
    int voc=0; // visible output count
    for (i=1; i<=output_count; i++) {
	if (n->isOutputVisible(i)) {
	    voc++;
	    if (voc > 1) return FALSE;
	}
    }
    int input_count = n->getInputCount();
    Ark* arc;
    for (i=1; i<=input_count; i++) {
	if (!n->isInputVisible(i)) continue;
	List* arcs = (List*)n->getInputArks(i);
	if (!arcs) continue;
	if (arcs->getSize()) return FALSE;
    }
    return TRUE;
}
Ark* GraphLayout::IsSingleInputNoOutputNode (Node* n, boolean& shares_an_output, boolean positioned)
{
    int output_count = n->getOutputCount();
    int output;
    for (output=1; output<=output_count; output++) {
	if (!n->isOutputVisible(output)) continue;
	List* outputs = (List*)n->getOutputArks(output);
	if (!outputs) continue;
	if (!outputs->getSize()) continue;
	return NULL;
    }

    int input_count = n->getInputCount();
    Ark* input_arc = NULL;
    Ark* arc;
    int input;
    for (input=1; input<=input_count; input++) {
	if (!n->isInputVisible(input)) continue;
	List* inputs = (List*)n->getInputArks(input);
	if (!inputs) continue;
	if (!inputs->getSize()) continue;
	ListIterator arcs(*inputs);
	while ((arc=(Ark*)arcs.getNext())) {
	    if (!input_arc) {
		// test for the output has only 1 destination node
		int output;
		Node* source = arc->getSourceNode(output);
		List* outputs = (List*)source->getOutputArks(output);
		ASSERT(outputs);
		if (outputs->getSize() != 1) {
		    shares_an_output = TRUE;
		} else {
		    shares_an_output = FALSE;
		}
		if (positioned) {
		    LayoutInfo* linfo = (LayoutInfo*)source->getLayoutInformation();
		    if (!linfo->isPositioned()) break;
		}
		input_arc = arc;
	    } else {
		return NULL;
	    }
	}
    }
    return input_arc;
}

boolean GraphLayout::positionDestBesideSibling (Ark* arc, int& x, int& y, boolean left,List* unusable_nodes)
{
    int output, input, dummy;
    Node* source = arc->getSourceNode(output);
    Node* destination = arc->getDestinationNode(input);
    List* arcs = (List*)source->getOutputArks(output);
    ListIterator iter(*arcs);
    Ark* oa;
    while ((oa=(Ark*)iter.getNext())) {
	Node* dst = oa->getDestinationNode(dummy);
	if (dst == destination) continue;

	// At this point, we've located a sibling.  Attempt a position
	// beside the standIn.  
	NodeInfo* dinfo = (NodeInfo*)dst->getLayoutInformation();
	if (!dinfo->isPositioned()) continue;

	// We cannot use a sibling if it's a member of the set of
	// nodes that we're trying to place.
	if (unusable_nodes->isMember(dst)) continue;

	int w,h;
	NodeInfo* info = (NodeInfo*)destination->getLayoutInformation();
	info->getXYSize(w,h);

	int sibling_x;
	dinfo->getXYPosition (sibling_x, y);
	if (left) {
	    x = sibling_x - (w+GraphLayout::NodeSpacing);
	} else {
	    int sibling_w;
	    dinfo->getXYSize(sibling_w, dummy);
	    x = sibling_x + sibling_w + GraphLayout::NodeSpacing;
	}
    }
    boolean collided = (this->nodeCanMoveTo(destination, x,y)==FALSE);
    return collided;
}

//
// Compute a location for the destination node of arc.  The location should
// result in a straight wire from the source's output tab to the 
// destination's input tab.
//
boolean GraphLayout::positionDestUnderSource (Ark* arc, int& x, int& y)
{
    int input, output;
    int dx,dy, sx,sy;
    int xincr,yincr;
    Node* destination = arc->getDestinationNode(input);
    Node* source = arc->getSourceNode(output);
    NodeInfo* src_info = (NodeInfo*)source->getLayoutInformation();
    NodeInfo* dst_info = (NodeInfo*)destination->getLayoutInformation();
    StandIn* src_si = source->getStandIn();
    StandIn* dst_si = destination->getStandIn();
    ASSERT (src_info->isPositioned());
    src_info->getXYPosition (sx,sy);
    int dst_tab_x = dst_si->getInputParameterTabX(input);
    int src_tab_x = src_si->getOutputParameterTabX(output);
    dx = sx + (-dst_tab_x + src_tab_x);
    dy = sy + ( (dst_info->getGraphDepth()-src_info->getGraphDepth()) * 
	   GraphLayout::HeightPerLevel
	 );
    boolean collided = (this->nodeCanMoveTo(destination, dx,dy)==FALSE);
    x = dx;
    y = dy;
    return collided;
}

void GraphLayout::bottomUpTraversal (Node* visit_parents_of, int current_depth, int& min)
{
    int input_count = visit_parents_of->getInputCount();
    NodeInfo* linfo = (NodeInfo*)visit_parents_of->getLayoutInformation();
    int next_depth = current_depth - 1;

    linfo->setGraphDepth(current_depth);
    int input;
    for (input=1; input<=input_count; input++) {
	if (!visit_parents_of->isInputVisible(input)) continue;

	List* inputs = (List*)visit_parents_of->getInputArks(input);
	ListIterator iter(*inputs);
	Ark* arc;
	while ((arc=(Ark*)iter.getNext())) {
	    int output;
	    Node* source = arc->getSourceNode(output);
	    NodeInfo* sinfo = (NodeInfo*)source->getLayoutInformation();
	    if (next_depth < min)
		min = next_depth;
	    int cd;
	    if (!sinfo) {
		sinfo = new NodeInfo();
		sinfo->initialize(source, next_depth);
		cd = next_depth;
	    } else {
		cd = sinfo->getGraphDepth();
	    }
	    if (next_depth < cd) {
		sinfo->setGraphDepth(next_depth);
		this->bottomUpTraversal(source, next_depth, min);
	    } else {
		this->bottomUpTraversal(source, cd, min);
	    }
	}
    }
}

//
// For any node that has an immediate ancestor with a hop count that
// is more than 1 away, we'll try to decrease the hop count so that the
// 2 nodes are adjacent.  This is necessary because we numbered the
// graph starting at the bottom.  We look only at leaf nodes.
// We also examine siblings of the node because that prevents us from
// increasing arc length between our ancestor and our sibling.
//
boolean GraphLayout::adjustHopCounts (Node* reflow[], int reflow_count, int& min)
{
    int i;
    boolean changes_made = FALSE;
    for (i=0; i<reflow_count; i++) {
	Node* n = reflow[i];
	if (this->hasConnectedOutputs(n)) continue;
	NodeInfo* linfo = (NodeInfo*)n->getLayoutInformation();
	int required_hops = this->computeRequiredHopsTo(n);
	int sibling_hops = this->computeRequiredHopsToSiblingsOf (n);
	required_hops = MAX(required_hops, sibling_hops);
	if (required_hops < linfo->getGraphDepth()) {
	    int gd = required_hops;
	    this->adjustAncestorHops (n,gd-1,min);
	    linfo->setGraphDepth(gd);
	    if (gd < min)
		min = gd;
	    changes_made = TRUE;
	}
    }

    return changes_made;
}

void GraphLayout::adjustAncestorHops (Node* parent, int new_hop_count, int& min)
{
    int input_count = parent->getInputCount();
    int output;
    int input;
    for (input=1; input<=input_count; input++) {
	if (!parent->isInputVisible(input)) continue;
	List* arcs = (List*)parent->getInputArks(input);
	if (!arcs) continue;
	ListIterator iter(*arcs);
	Ark* arc;
	while ((arc=(Ark*)iter.getNext())) {
	    Node* source = arc->getSourceNode(output);
	    NodeInfo* linfo = (NodeInfo*)source->getLayoutInformation();
	    if (new_hop_count < linfo->getGraphDepth()) {
		linfo->setGraphDepth(new_hop_count);
		if (new_hop_count < min) min = new_hop_count;
		this->adjustAncestorHops (source, new_hop_count-1, min);
	    }
	}
    }
}

//
// When we decrement a node's hop count, we also want to decrement any 
// descendants of the node if the descendant has no other inputs and
// no outputs.
//
void GraphLayout::adjustDescendantHops(Node* parent, int new_hop_count)
{
    int output_count = parent->getOutputCount();
    int output;
    int input;
    for (output=1; output<=output_count; output++) {
	if (!parent->isOutputVisible(output)) continue;
	List* arcs = (List*)parent->getOutputArks(output);
	if (!arcs) continue;
	ListIterator iter(*arcs);
	Ark* arc;
	while ((arc=(Ark*)iter.getNext())) {
	    Node* dest = arc->getDestinationNode(input);
	    boolean dummy;
	    if (GraphLayout::IsSingleInputNoOutputNode(dest, dummy, FALSE)) {
		NodeInfo* info = (NodeInfo*)dest->getLayoutInformation();
		ASSERT (new_hop_count <= info->getGraphDepth());
#               if defined(DEBUG_PLACEMENT)
		if (debug) {
		    fprintf (stdout, "%s[%d] moving %s:%d from %d to %d\n",
			__FILE__,__LINE__, dest->getNameString(), 
			dest->getInstanceNumber(), info->getGraphDepth(), new_hop_count);
		}
#               endif
		info->setGraphDepth(new_hop_count);
	    }
	}
    }
}

int GraphLayout::computeRequiredHopsTo(Node* n)
{
    int output;
    int max_count = 1;
    int input_count = n->getInputCount();
    int input;
    for (input=1; input<=input_count; input++) {
	if (!n->isInputVisible(input)) continue;
	List* arcs = (List*)n->getInputArks(input);
	if (!arcs) continue;
	ListIterator iter(*arcs);
	Ark* arc;
	while ((arc=(Ark*)iter.getNext())) {
	    int count = this->computeRequiredHopsTo(arc->getSourceNode(output));
	    max_count = MAX(count+1, max_count);
	}
    }
    return max_count;
}

int GraphLayout::computeRequiredHopsToSiblingsOf (Node* n)
{
    int dummy;
    int input_count = n->getInputCount();
    int max_hop_count = 1;
    int input;
    for (input=1; input<=input_count; input++) {
	if (!n->isInputVisible(input)) continue;
	List* arcs = (List*)n->getInputArks(input);
	if (!arcs) continue;
	ListIterator iter(*arcs);
	Ark* arc;
	while ((arc=(Ark*)iter.getNext())) {
	    Node* source = arc->getSourceNode(dummy);

	    int output_count = source->getOutputCount();
	    // for each output, fetch connected nodes.
	    int output;
	    for (output=1; output<=output_count; output++) {
		if (!source->isOutputVisible(output)) continue;
		List* oarcs = (List*)source->getOutputArks(output);
		if (!oarcs) continue;
		ListIterator it(*oarcs);
		Ark* oarc;
		// for each of those nodes fetch required hops
		while ((oarc=(Ark*)it.getNext())) {
		    Node* dest = oarc->getDestinationNode(dummy);
		    max_hop_count = MAX(max_hop_count, this->computeRequiredHopsTo(dest));
		}
	    }

	}
    }
    return max_hop_count;
}

//
// compute hops by starting at every leaf node and walking up the graph.
// Initialize the hop counter with a huge number and decrement by 1 at
// each hop.  Then translate all hop counts so that the smallest is set
// to 1.  Then adjust all hop counters so that there is no unneeded
// distance between connected nodes.  We get unneeded space because
// we default leaf nodes to the largest hop count even though that isn't
// accurate.
//
void GraphLayout::computeHopCounts (Node* reflow[], int reflow_count)
{
    int i;
    //
    // The starting hop counter has no units.  After we finish, we'll
    // translate all hop counters so that the smallest one in the graph
    // is set to 1.
    //
    int smallest_hop_count=99999;
    for (i=0; i<reflow_count; i++) {
	Node* n = reflow[i];
	if (!this->hasConnectedOutputs(n)) {
	    NodeInfo* linfo = new NodeInfo();
	    linfo->initialize (n, 99999);
	    this->bottomUpTraversal(n,99999,smallest_hop_count);
	}
    }

    boolean changes_made;

    //
    // We might want to make some adjustments to hop counts.  Sounds like
    // nonsense, but the problem is that there are some nodes that could
    // really have several different hop counts.  Think about a receiver
    // wired directly to an (and only to an) output.  Should the receiver
    // be marked 1 and the output be marked 2?  ...or should the output be
    // (max hops) and the receiver be 1-max?  Usually I've tried to keep
    // things pushed down towards the bottom of the graph.  Here we want
    // to push things back towards the top of the graph i.e. reduce the
    // hop count if that will have the effect of shorting the arcs.
    //
    changes_made = TRUE;
    while (changes_made) 
	changes_made = this->adjustHopCounts(reflow, reflow_count, smallest_hop_count);

    //
    // Rules say that we decrement the hop count of any node that is 
    // connected to multiple non-consecutive input tabs on 1 row.
    // The idea is to devote more screen realestate to displaying
    // lines if there are lots of lines between 1 pair of nodes.
    //
    do {
	changes_made = FALSE;
	for (i=0; i<reflow_count; i++) {
	    Node* n = reflow[i];
	    changes_made|= this->spreadOutSpaghettiFrom(n, smallest_hop_count);
	}
    } while (changes_made);
 
    //
    // Before we raise the curtain, just one more tweak.  Sometimes
    // (usually at the top of a graph) we get some nodes that have a slew of
    // recievers providing inputs.  It's difficult to place all of these
    // in a pleasing way, so  we'll do a final check for this.  If the
    // situation exists, then we'll artificially decrement the hop count
    // for every other reciever so that the pattern is a little garbagey
    // looking.  We sort before the call because if obviates the need
    // to make the call in a loop the way we do above.
    //
    qsort (reflow, reflow_count, sizeof(Node*), NodeInfo::SortByHop);
    for (i=0; i<reflow_count; i++) {
	Node* n = reflow[i];
	this->fixForTooManyReceivers(n, smallest_hop_count);
    }

    //
    // Translate to the origin
    //
    for (i=0; i<reflow_count; i++) {
	Node* n = reflow[i];
	NodeInfo* linfo = (NodeInfo*)n->getLayoutInformation();
	ASSERT (linfo);
	int gd = linfo->getGraphDepth();
	gd = (gd-smallest_hop_count) + 1;
	linfo->setGraphDepth(gd);
    }
}

//
// if n has multiple arcs that go to the next higher hop count, and those
// arcs aren't consecutive, then decrement n's hop count.  Reasoning is that
// having too many arcs in a cramped area is hard to read.
//
boolean GraphLayout::spreadOutSpaghettiFrom (Node* n, int& min)
{
    int dummy;
    if (this->countConnectedOutputs (n, 1, dummy) <= 1) return FALSE;

    // don't screw around with gets and sets
    const char* name = n->getNameString();
    if (EqualString(name, "GetLocal") || EqualString(name, "GetGlobal") ||
	EqualString(name, "Get")) {
	return FALSE;
    }

    boolean retval = FALSE;

    NodeInfo* info = (NodeInfo*)n->getLayoutInformation();
    int depth = info->getGraphDepth();
    int output_count = n->getOutputCount();
    int output;
    Ark* arc;
    int levels_to_decr = 0;
    for (output=1; output<=output_count; output++) {
	if (!n->isOutputVisible(output)) continue;
	List* arcs = (List*)n->getOutputArks(output);
	if (!arcs) continue;
	ListIterator iter(*arcs);
	int input;
	Node* arc_dest = NUL(Node*);
	while ((arc=(Ark*)iter.getNext())) {
	    Node* dest = arc->getDestinationNode(input);
	    NodeInfo* dinfo = (NodeInfo*)dest->getLayoutInformation();
	    int cnctn_count = this->countConnectionsBetween(n,dest,FALSE);
	    int desired_depth = dinfo->getGraphDepth() - cnctn_count;
	    if (depth > desired_depth) {
		levels_to_decr = MAX(levels_to_decr, (depth-desired_depth));
	    }
	}
    }

    if (levels_to_decr) {
	int new_depth = depth-levels_to_decr;
#       if defined(DEBUG_PLACEMENT)
	if (debug) {
	    fprintf (stdout, "%s[%d] changing %s:%d from %d to %d\n", __FILE__,__LINE__,
		n->getNameString(), n->getInstanceNumber(), depth, new_depth);
	}
#       endif
	this->adjustAncestorHops (n,new_depth-1,min);
	this->adjustDescendantHops (n, new_depth+1);
	if (new_depth < min) min = new_depth;
	info->setGraphDepth(new_depth);
	retval = TRUE;
    }

    return retval;
}

//
// If a node has wide recievers (a.k.a no input-1 output node)
// connected to adjacent input tabs then
// we'll decrement the hop count of 1 receiver so that things aren't
// bunched up too much.  In general this is bad because anything that
// increases arc length is likely to increase the wire crossings.
//
void GraphLayout::fixForTooManyReceivers(Node* n, int& min)
{
    NodeInfo* ninfo = (NodeInfo*)n->getLayoutInformation();
    int dummy;
    int input;
    int input_count = n->getInputCount();
    List wide_nodes;
    for (input=1; input<=input_count; input++) {
	if (!n->isInputVisible(input)) continue;
	List* arcs = (List*)n->getInputArks(input);
	if (!arcs) continue;
	ListIterator iter(*arcs);
	Ark* arc;
	while ((arc=(Ark*)iter.getNext())) {
	    Node* source = arc->getSourceNode(dummy);
	    if (wide_nodes.isMember(source)) continue;
	    if (!this->hasNoCloserDescendant(source,n)) continue;
	    LayoutInfo* linfo = (LayoutInfo*)source->getLayoutInformation();
	    int w,h;
	    linfo->getXYSize(w,h);
	    wide_nodes.appendElement(source);
	}
    }
    int wnsize = wide_nodes.getSize();
    if (wnsize <= 2) return ;
    
    // saw tooth pattern
    //  We're controlling how high up/down the pattern goes.
    int state = 0;
    int max_state = wnsize>>1;
    if (wnsize&1) max_state++;
    int incr = 1;
    boolean had_inputs = FALSE;
    for (input=1; input<=input_count; input++) {
	if (!n->isInputVisible(input)) continue;
	List* arcs = (List*)n->getInputArks(input);
	if (!arcs) continue;
	ListIterator iter(*arcs);
	Ark* arc;
	while ((arc=(Ark*)iter.getNext())) {
	    Node* source = arc->getSourceNode(dummy);
	    if (state == 3) {
		if ((!this->hasConnectedInputs(source)) && (!had_inputs)) 
		    state = 0;
	    }
	    NodeInfo* linfo = (NodeInfo*)source->getLayoutInformation();
	    int gd;
	    switch (state) {
		case -1:
		    incr = -incr;
		case 0:
		    state = 1;
		    break;
		default:
		    gd = ninfo->getGraphDepth()-(state+1);
		    // if this actually does anything...
		    if (gd < linfo->getGraphDepth()) {
			this->adjustAncestorHops (source, gd-1, min);
			linfo->setGraphDepth(gd);
			if (gd < min) min = gd;
		    }
		    state+= incr;
		    break;
	    }
	    if (state == max_state) {
		incr = -incr;
		state+= 2*incr;
	    }
	    had_inputs = this->hasConnectedInputs(source);
	}
    }
}

//
// return TRUE if dest has the hop count that is closest to the hop
// count of source.  return FALSE if there is some other node that's connected
// to source that has a shorter arcs.  We use this information in order to
// cause source to be placed with its closest living relative.
// return FALSE if there is some other node that's connected to dest
// by more arks than source.
//
boolean GraphLayout::hasNoCloserDescendant (Node* source, Node* dest)
{
    int dummy;
    int output;
    NodeInfo* src_info = (NodeInfo*)source->getLayoutInformation();
    NodeInfo* dst_info = (NodeInfo*)dest->getLayoutInformation();

    int connection_count_to_beat = this->countConnectionsBetween(source, dest);

    int min_depth = dst_info->getGraphDepth();
    int output_count = source->getOutputCount();
    for (output=1; output<=output_count; output++) {
	if (!source->isOutputVisible(output)) continue;
	List* arcs = (List*)source->getOutputArks(output);
	if (!arcs) continue;
	ListIterator iter(*arcs);
	Ark* arc;
	while ((arc=(Ark*)iter.getNext())) {
	    Node* destination = arc->getDestinationNode(dummy);
	    if (destination == dest) continue;
	    NodeInfo* info = (NodeInfo*)destination->getLayoutInformation();
	    if (this->countConnectionsBetween(source,destination) < 
		    connection_count_to_beat) {
		continue;
	    }
	    if (info->getGraphDepth() < min_depth) {
		// a closer relative was found
		return FALSE;
	    }
	}
    }

    return TRUE;
}

//
// Compute the location of the new placements relative to the old
// placements and adjust the new placements' locations so that the
// 2 bounding boxes don't intersect.
// If known_disjoint is TRUE, then that means we can move the new set
// anywhere we want it.  Otherwise we have to maintain any left-of
// right-of relationships.
//
void GraphLayout::repositionNewPlacements (Node* root, boolean disjoint, List& placed)
{
    Node* n;
    NodeInfo* ninfo = (NodeInfo*)root->getLayoutInformation();
    LayoutGroup* group = ninfo->getLayoutGroup();
    placed.appendElement(root);

#   if defined(DEBUG_PLACEMENT)
    if (debug) {
	ListIterator li(placed);
	Node* node;
	fprintf (stdout, "%s[%d] placing in group %d...\n", 
		__FILE__,__LINE__, group->getId());
	while (node=(Node*)li.getNext()) {
	    fprintf (stdout, "\t%s:%d\n", node->getNameString(),
		    node->getInstanceNumber());
	}
    }
#   endif

    int minx2, miny2, maxx2, maxy2;
    this->computeBoundingBox (placed, minx2, miny2, maxx2,maxy2);

    boolean group_had_placements=TRUE;
    int minx1, miny1, maxx1, maxy1;

    // 
    // At this point n is not initialized but the method only reads from
    // n if the connected list hasn't been built yet.  At this point,
    // all such lists are built, so the uninitialized n is supplied here
    // only as a misfeature of the api.
    //
    List* connected = ninfo->getConnectedNodes(n);

    if (!disjoint) {
	// Form the bounding box of the starting condition 
	List original;
	Node* node;
	ListIterator iter(*connected);
	while ((node=(Node*)iter.getNext())) {
	    LayoutInfo* info = (LayoutInfo*)node->getLayoutInformation();
	    if (info->isPositioned()) {
		if (!placed.isMember(node)) original.appendElement(node);
	    }
	}
	group_had_placements = 
	    this->computeBoundingBox (original, minx1, miny1, maxx1,maxy1);
    }

    int ydelta, xdelta;
    if ((disjoint) || (!group_had_placements)) {
	xdelta = -minx2;
	ydelta = -miny2;
	ListIterator iter(placed);
	while ((n=(Node*)iter.getNext())) {
	    NodeInfo* info = (NodeInfo*)n->getLayoutInformation();
	    int x,y;
	    info->getXYPosition(x,y);
	    info->setProposedLocation(x+xdelta, y+ydelta);
	}
	group->initialize(&placed);
	return ;
    }

    ASSERT (group_had_placements);
    //
    // Now we have 2 bounding boxes, 1 for the nodes that were
    // already placed and 1 for the batch of additional placements.
    // We know we can scoot the new comers over so that the 2 bounding
    // boxes butt up against each other, but we might be able to do
    // better.  How to figure out how much we can scoot over without
    // having overlapping?  
    //
    // The goal is to scoot over far enough so that a wire that joins
    // 2 nodes - 1 in the new placments and 1 in the old placements -
    // becomes a straight line.
    //
    // Collect all the arcs that cross from 1 set to the other set.
    // Then sort those arcs by decreasing length.  Then for each 
    // arc, try to line things up without any collisions.  If none
    // works, then just butt the bounding boxes up against eachother.
    //
    List crossing_arcs;
    ListIterator iter(placed);
    while ((n=(Node*)iter.getNext())) {
	int input_count = n->getInputCount();
	int output_count = n->getOutputCount();
	int input, output;
	for (input=1; input<=input_count; input++) {
	    if (!n->isInputVisible(input)) continue;
	    List* arks = (List*)n->getInputArks(input);
	    if (!arks) continue;
	    ListIterator li(*arks);
	    Ark* arc;
	    while ((arc=(Ark*)li.getNext())) {
		Node* source = arc->getSourceNode(output);
		NodeInfo* sinfo = (NodeInfo*)source->getLayoutInformation();
		if (!sinfo->isPositioned()) continue;
		if (!placed.isMember(source)) {
		    if (crossing_arcs.isMember(arc) == FALSE)
			crossing_arcs.appendElement(arc);
		}
	    }
	}
	for (output=1; output<=output_count; output++) {
	    if (!n->isOutputVisible(output)) continue;
	    List* arks = (List*)n->getOutputArks(output);
	    if (!arks) continue;
	    ListIterator li(*arks);
	    Ark* arc;
	    while ((arc=(Ark*)li.getNext())) {
		Node* dest = arc->getDestinationNode(input);
		NodeInfo* dinfo = (NodeInfo*)dest->getLayoutInformation();
		if (!dinfo->isPositioned()) continue;
		if (!placed.isMember(dest)) {
		    if (crossing_arcs.isMember(arc) == FALSE)
			crossing_arcs.appendElement(arc);
		}
	    }
	}
    }
    int maxarcs = 128;
    int arc_count = crossing_arcs.getSize();
    arc_count = MIN(maxarcs, arc_count);
    // this assert says that we should have detected a disconnected
    // subgraph in an earlier test.
    //ASSERT(arc_count);
    // arc_count can be 0 if we the nodes to which the new placements
    // are connected haven't been placed yet.

    boolean prefer_left = TRUE;

    boolean arc_found = FALSE;
    Ark* aa[128];
    int i,j;
    if (arc_count) {
	for (i=0,j=1; i<arc_count; i++,j++) {
	    aa[i] = (Ark*)crossing_arcs.getElement(j);
	}
	qsort (aa, arc_count, sizeof(Ark*), GraphLayout::ArcComparator);

	//
	// There may come a time when we have to decide if we want to place
	// new nodes to the left or to the right of existing placements.  When
	// we use this info we're really just taking our chances.  Also, we don't
	// always need this info but we have to have it computed before we need
	// it.
	int center = 0;
	int j;
	for (j=0; j<arc_count; j++) {
	    //
	    // The choice of arc has an influence of the way we lay out the
	    // graph especially when Gets and Sets are involved.  I don't
	    // know of a 'proper' way to pick one arc. So, loop over all arcs
	    // involved, give each 1 vote which it can cast in 1 of 3 ways...
	    // go left, go right, don't care.
	    //
	    boolean prefer_left = FALSE;
	    Ark* arc = aa[j];

	    int input, output;
	    Node* src = arc->getSourceNode(output);
	    Node* dst = arc->getDestinationNode(input);
	    int nth_input_tab, nth_output_tab;
	    int vi = this->countConnectedInputs(dst, input, nth_input_tab);
	    // destination node casts his vote...
	    if ((vi==1)||((nth_input_tab==(vi>>1))&&((vi&1)==1))) {
		// don't care
#               if defined(DEBUG_PLACEMENT)
		if (debug)
		    fprintf (stdout, "\t%s destination didn't vote\n",
			dst->getNameString());
#               endif
	    } else if (nth_input_tab<=(vi>>1)) {
		prefer_left = (placed.isMember(dst)==FALSE);
		center+= (prefer_left?-1:1);
#               if defined(DEBUG_PLACEMENT)
		if (debug)
		    fprintf (stdout, "\t%s destination voted %s\n",
			dst->getNameString(), (prefer_left?"LEFT":"RIGHT"));
#               endif
	    } else if (nth_input_tab>(vi>>1)) {
		prefer_left = placed.isMember(dst);
		center+= (prefer_left?-1:1);
#               if defined(DEBUG_PLACEMENT)
		if (debug)
		    fprintf (stdout, "\t%s destination voted %s\n",
			dst->getNameString(), (prefer_left?"LEFT":"RIGHT"));
#               endif
	    }


	    // source node casts his vote..
	    int vo = this->countConnectedOutputs(src, output, nth_output_tab);
	    if ((vo==1)||((nth_output_tab==(vo>>1))&&((vo&1)==1))) {
		//don't care
#               if defined(DEBUG_PLACEMENT)
		if (debug)
		    fprintf (stdout, "\t%s source didn't vote\n",
			src->getNameString());
#               endif
	    } else if (nth_output_tab <= (vo>>1)) {
		prefer_left = (placed.isMember(src)==FALSE);
		center+= (prefer_left?-1:1);
#               if defined(DEBUG_PLACEMENT)
		if (debug)
		    fprintf (stdout, "\t%s source voted %s\n",
			src->getNameString(), (prefer_left?"LEFT":"RIGHT"));
#               endif
	    } else if (nth_output_tab > (vo>>1)) {
		prefer_left = placed.isMember(src);
		center+= (prefer_left?-1:1);
#               if defined(DEBUG_PLACEMENT)
		if (debug)
		    fprintf (stdout, "\t%s source voted %s\n",
			src->getNameString(), (prefer_left?"LEFT":"RIGHT"));
#               endif
	    }

	    // the source may have another node connected to the same output.
	    // It should cast a vote also.
	    List* arcs = (List*)src->getOutputArks(output);
	    ListIterator iter(*arcs);
	    Node* other_dst = NUL(Node*);
	    int oinput;
	    while ((arc=(Ark*)iter.getNext())) {
		other_dst = arc->getDestinationNode(oinput);
		if (other_dst == dst) continue;
		if (placed.isMember(other_dst)) continue;
		break;
	    }
	    if (other_dst) {
		int vi = this->countConnectedInputs(other_dst, oinput, nth_input_tab);
		if (vi==1) {
		    // don't care
#		    if defined(DEBUG_PLACEMENT)
		    if (debug)
			fprintf (stdout, "\t%s destination didn't vote\n",
			    dst->getNameString());
#		    endif
		} else if (nth_input_tab<=(vi>>1)) {
		    prefer_left = TRUE;
		    center+= (prefer_left?-1:1);
#		    if defined(DEBUG_PLACEMENT)
		    if (debug)
			fprintf (stdout, "\t%s source voted %s\n",
			    src->getNameString(), (prefer_left?"LEFT":"RIGHT"));
#		    endif
		} else if (nth_input_tab>(vi>>1)) {
		    prefer_left = FALSE;
		    center+= (prefer_left?-1:1);
#		    if defined(DEBUG_PLACEMENT)
		    if (debug)
			fprintf (stdout, "\t%s source voted %s\n",
			    src->getNameString(), (prefer_left?"LEFT":"RIGHT"));
#		    endif
		}
	    }
	}
	prefer_left = (center<=0);

	// for each arc, compute the x location that will make the wire
	// straight and test each node in placed to see if the entire
	// set can move.
	for (i=0; i<arc_count; i++) {
	    Ark* arc = aa[i];
	    int input, output;
	    Node* source = arc->getSourceNode(output);
	    Node* dest = arc->getDestinationNode(input);
	    NodeInfo* sinfo = (NodeInfo*)source->getLayoutInformation();
	    NodeInfo* dinfo = (NodeInfo*)dest->getLayoutInformation();
	    boolean node_can_move = FALSE;
	    int prevx, prevy;

	    int x,y,dummy;
	    if (placed.isMember(source)) {
		if (!this->positionSourceOverDest(arc, x,y)) {
		    node_can_move = TRUE;
		    sinfo->getXYPosition (prevx, prevy);
		    xdelta = x-prevx;
		    ydelta = y-prevy;
		}
	    } else {
		if (!this->positionDestUnderSource(arc, x,y)) {
		    node_can_move = TRUE;
		    dinfo->getXYPosition (prevx, prevy);
		    xdelta = x-prevx;
		    ydelta = y-prevy;
		} else if (!this->positionDestBesideSibling (arc, x,y,prefer_left,&placed)) {
		    // potential problem.  I'm not checking that the arc isn't 
		    // shorter than some other arc that the destination node
		    // has coming in.  If it is shorter, then I could place
		    // a destination node higher on the screen that it ought
		    // to be.  An example of this in InterpolateCameraMacro.net.
		    node_can_move = TRUE;
		    dinfo->getXYPosition (prevx, prevy);
		    xdelta = x-prevx;
		    ydelta = y-prevy;
		}
	    }

	    // now check all other nodes.
	    if (node_can_move) {
		arc_found = TRUE;
		//ListIterator iter(*connected);
		ListIterator iter(placed);
		Node* n;
		while ((n=(Node*)iter.getNext())) {
		    NodeInfo* info = (NodeInfo*)n->getLayoutInformation();
		    int nx,ny;
		    info->getXYPosition(nx,ny);
		    if (!this->nodeCanMoveTo (n, nx+xdelta, ny+ydelta)) {
			arc_found = FALSE;
			break;
		    }
		}
		if (arc_found) break;
	    }
	}
    }

    // If no arc is found, then compute the xdelta so that the
    // two bounding boxes butt up against eachother.  The only
    // missing information is which bounding box should be on
    // the left and which on the right.  This is difficult to 
    // figure out.  Possible clues are the positioning of the
    // input and output tabs in use.  A high-numbered input
    // wants to be on the left of its source.  A low-numbered
    // output wants to be on the right of its destination.
    if (!arc_found) {
#       if defined(DEBUG_PLACEMENT)
	if (debug)
	    fprintf (stdout, "%s[%d] %d arcs will vote on placement\n", 
		__FILE__,__LINE__,arc_count);
#       endif

	if (prefer_left) {
	    xdelta = -(maxx2 + GraphLayout::NodeSpacing - minx1);
	    ydelta = 0;
	} else {
	    xdelta =  (maxx1  + GraphLayout::NodeSpacing - minx2);
	    ydelta = 0;
	}
    }

    //
    // perform the translation
    //
    iter.setList(placed);
    while ((n=(Node*)iter.getNext())) {
	NodeInfo* linfo = (NodeInfo*)n->getLayoutInformation();
	int x,y;
	ASSERT (linfo->isPositioned());
	linfo->getXYPosition(x,y);
#       if defined(DEBUG_PLACEMENT)
	if (debug)
	    fprintf (stdout, "%s[%d] moving %s from %d,%d to %d,%d\n",
		    __FILE__,__LINE__,n->getNameString(),x,y,x+xdelta,y+ydelta);
#       endif
	linfo->setProposedLocation(x+xdelta, y+ydelta);
    }
    group->initialize(&placed);
}

void GraphLayout::prepareAnnotationPlacement(List& decorators, Node* reflow[], int reflow_count, const List& all_decorators, WorkSpace* workSpace, int& widest, int& tallest)
{
    //
    // exclude the nodes that aren't in the current page
    //
    ListIterator decs;
    VPEAnnotator* dec;
    decs.setList((List&)all_decorators);
    while ((dec=(VPEAnnotator*)decs.getNext())) {
	if (dec->getWorkSpace() != workSpace) continue;
	decorators.appendElement(dec);
	AnnotationInfo* ai = new AnnotationInfo();
	ai->initialize(dec);
	ai->findNearestNode(reflow, reflow_count);

	int w,h;
	ai->getXYSize(w,h);
	NodeDistance* nd = ai->nearby[0];
	if (nd->getLocation() & DECORATOR_LEFT) {
	    if (w > widest) widest = w;
	}
	if (nd->getLocation() & DECORATOR_ABOVE) {
	    if (h > tallest) tallest = h;
	}
    }
}

AnnotationInfo::~AnnotationInfo()
{
    if (this->nearby) {
	int i;
	for (i=0; i<this->nearbyCnt; i++)
	    delete this->nearby[i];
	free(this->nearby);
    }
}
//
// Distance between 2 rectangles is the vertical distance between 2 horizontal
// line segments or horizontal distance between 2 vertical line segments
// if they overlap.  If the line segments don't overlap, then the distance
// is the smallest of the 4 distances between the the endpoints of the line
// segments.
//
void AnnotationInfo::findNearestNode(Node* reflow[], int reflow_count)
{
    int i;
    ASSERT (this->isInitialized());

    int x1 = this->orig_x;
    int x2 = this->orig_x + (this->w-1);
    int y1 = this->orig_y;
    int y2 = this->orig_y + (this->h-1);

    this->nearby = (NodeDistance**)malloc(sizeof(NodeDistance*) * reflow_count);;
    this->nearbyCnt = reflow_count;

    int min_dist = 999999;
    for (i=0; i<reflow_count; i++) {
	Node* n = reflow[i];
	LayoutInfo* linfo = (LayoutInfo*)n->getLayoutInformation();
	int nx1,ny1,nw,nh;
	linfo->getOriginalXYPosition(nx1,ny1);
	linfo->getXYSize(nw,nh);
	int nx2 = nx1+(nw-1);
	int ny2 = ny1+(nh-1);

	int vdist = 999999;
	int hdist = 999999;

	// vertical edges
	int hloc=0;
	if (x1 > nx2) {
	    hloc = DECORATOR_RIGHT;
	    if ((y1>=ny1) && (y1<=ny2)) {
		hdist = x1-nx2;
	    } else if ((y2>=ny1) && (y2<=ny2)) {
		hdist = x1-nx2;
	    } else if ((y1<=ny1) && (y2>=ny2)) {
		hdist = x1-nx2;
	    } else if (y1>ny2) {
		hdist = (int)sqrt((double)((x1-nx2)*(x1-nx2) + (y1-ny2)*(y1-ny2)));
		hloc|= DECORATOR_BELOW;
	    } else {
		hdist = (int)sqrt((double)((x1-nx2)*(x1-nx2) + (y2-ny1)*(y2-ny1)));
		hloc|= DECORATOR_ABOVE;
	    }
	} else if (x2 < nx1) {
	    hloc = DECORATOR_LEFT;
	    if ((y1>=ny1) && (y1<=ny2)) {
		hdist = nx1-x2;
	    } else if ((y2>=ny1) && (y2<=ny2)) {
		hdist = nx1-x2;
	    } else if ((y1<=ny1) && (y2>=ny2)) {
		hdist = nx1-x2;
	    } else if (y1>ny2) {
		hdist = (int)sqrt((double)((x2-nx1)*(x2-nx1) + (y1-ny2)*(y1-ny2)));
		hloc|= DECORATOR_BELOW;
	    } else {
		hdist = (int)sqrt((double)((x2-nx1)*(x2-nx1) + (y2-ny1)*(y2-ny1)));
		hloc|= DECORATOR_ABOVE;
	    }
	}

	// horizontal edges
	int vloc=0;
	if (y1 > ny2) {
	    vloc = DECORATOR_BELOW;
	    if ((x1>=nx1) && (x1<=nx2)) {
		vdist = y1-ny2;
	    } else if ((x2>=nx1) && (x2<=nx2)) {
		vdist = y1-ny2;
	    } else if ((x1<=nx1) && (x2>=nx2)) {
		vdist = y1-ny2;
	    } else if (x1 > nx2) {
		vdist = (int)sqrt((double)((x1-nx2)*(x1-nx2) + (y1-ny2)*(y1-ny2)));
		vloc|= DECORATOR_RIGHT;
	    } else {
		vdist = (int)sqrt((double)((x2-nx1)*(x2-nx1) + (y1-ny2)*(y1-ny2)));
		vloc|= DECORATOR_LEFT;
	    }
	} else if (y2 < ny1) {
	    vloc = DECORATOR_ABOVE;
	    if ((x1>=nx1) && (x1<=nx2)) {
		vdist = ny1-y2;
	    } else if ((x2>=nx1) && (x2<=nx2)) {
		vdist = ny1-y2;
	    } else if ((x1<=nx1) && (x2>=nx2)) {
		vdist = ny1-y2;
	    } else if (x1 > nx2) {
		vdist = (int)sqrt((double)((x1-nx2)*(x1-nx2) + (y2-ny1)*(y2-ny1)));
		vloc|= DECORATOR_RIGHT;
	    } else {
		vdist = (int)sqrt((double)((x2-nx1)*(x2-nx1) + (y2-ny1)*(y2-ny1)));
		vloc|= DECORATOR_LEFT;
	    }
	}

	if (hdist < vdist) this->nearby[i] = new NodeDistance (n, hdist, hloc);
	else this->nearby[i] = new NodeDistance (n, vdist, vloc);
    }
    qsort (this->nearby, this->nearbyCnt, sizeof(NodeDistance*), AnnotationInfo::SortByDistance);
}

int GraphLayout::ArcComparator(const void *a, const void* b)
{
    Ark** aptr = (Ark**)a;
    Ark** bptr = (Ark**)b;
    Ark* aarc = *aptr;
    Ark* barc = *bptr;

    int ahops,bhops;
    Node *src, *dst;
    int dummy;

    src = aarc->getSourceNode(dummy);
    dst = aarc->getDestinationNode(dummy);
    NodeInfo* sinfo = (NodeInfo*)src->getLayoutInformation();
    NodeInfo* dinfo = (NodeInfo*)dst->getLayoutInformation();
    ahops = dinfo->getGraphDepth() - sinfo->getGraphDepth();

    src = barc->getSourceNode(dummy);
    dst = barc->getDestinationNode(dummy);
    sinfo = (NodeInfo*)src->getLayoutInformation();
    dinfo = (NodeInfo*)dst->getLayoutInformation();
    bhops = dinfo->getGraphDepth() - sinfo->getGraphDepth();

    if (ahops < bhops) return -1;
    if (ahops > bhops) return -1;
    return 0;
}

int AnnotationInfo::SortByDistance(const void *a, const void* b)
{
    NodeDistance** aptr = (NodeDistance**)a;
    NodeDistance** bptr = (NodeDistance**)b;
    NodeDistance* aNd = (NodeDistance*)*aptr;
    NodeDistance* bNd = (NodeDistance*)*bptr;
    if (aNd->getDistance() < bNd->getDistance()) return -1;
    if (aNd->getDistance() > bNd->getDistance()) return  1;
    return 0;
}

int AnnotationInfo::NextX;
int AnnotationInfo::NextY;

void AnnotationInfo::reposition(Node* reflow[], int reflow_count, List& others)
{
    // spot sequence
    int spots[] = {
	DECORATOR_RIGHT,
	DECORATOR_RIGHT | DECORATOR_ABOVE,
	DECORATOR_RIGHT | DECORATOR_BELOW,
	DECORATOR_LEFT,
	DECORATOR_LEFT | DECORATOR_ABOVE,
	DECORATOR_LEFT | DECORATOR_BELOW,
	DECORATOR_ABOVE,
	DECORATOR_BELOW
    };
    int spot_cnt = sizeof(spots) / sizeof(int);

    boolean did_it = FALSE;

    int node_to_try = 0;
    while ((!did_it) && (node_to_try < this->nearbyCnt)) {

	NodeDistance* nd = this->nearby[node_to_try];
	Node* n = nd->getNode();
	int loc = nd->getLocation();
	LayoutInfo* linfo = (LayoutInfo*)n->getLayoutInformation();
	int nodex, nodey;
	int nodew, nodeh;
	linfo->getXYPosition(nodex,nodey);
	linfo->getXYSize(nodew,nodeh);

	int spot_index = 0;
	while (spots[spot_index] != loc) spot_index++;

	int tries = 0;
	while (tries < spot_cnt) {
	    int location = spots[spot_index];
	    int x = nodex + ((nodew-this->w)/2);
	    int y = nodey + ((nodeh-this->h)/2);
	    if (location & DECORATOR_ABOVE) {
		y = nodey - (1+this->h);
	    } else if (location & DECORATOR_BELOW) {
		y = nodey + nodeh + 1;
	    }
	    if (location & DECORATOR_LEFT) {
		x = nodex - (1+this->w);
	    } else if (location & DECORATOR_RIGHT) {
		x = nodex + 1 + nodew;
	    }

	    if ((x>0) && (y>0) && 
		    (GraphLayout::CanMoveTo(this, x, y, reflow, reflow_count, &others))) {
		this->setProposedLocation (x, y);
		did_it = TRUE;
		break;
	    } 
	    tries++;
	    spot_index++;
	    if (spot_index==spot_cnt) spot_index = 0;
	}
	node_to_try++;
    }

    if (!did_it) {
	this->setProposedLocation (AnnotationInfo::NextX, AnnotationInfo::NextY);
	AnnotationInfo::NextY+= this->h+1;
    }
}

void LayoutGroup::initialize(List* nodes)
{
    ListIterator iter(*nodes);
    Node* n;
    while ((n=(Node*)iter.getNext())) {
	NodeInfo* linfo = (NodeInfo*)n->getLayoutInformation();
	int x,y,w,h;
	linfo->getOriginalXYPosition(x,y);
	linfo->getXYSize(w,h);
	if (x < this->orig_x1) this->orig_x1 = x;
	if (y < this->orig_y1) this->orig_y1 = y;
	if ((x+w) > this->orig_x2) this->orig_x2 = x+w;
	if ((y+h) > this->orig_y2) this->orig_y2 = y+h;
	linfo->getXYPosition(x,y);
	if (x < this->x1) this->x1 = x;
	if (y < this->y1) this->y1 = y;
	if ((x+w) > this->x2) this->x2 = x+w;
	if ((y+h) > this->y2) this->y2 = y+h;
	linfo->setLayoutGroup(this);
    }
    this->initialized = TRUE;
}

int LayoutGroup::Comparator (const void* a, const void* b)
{
    LayoutGroup** aptr = (LayoutGroup**)a;
    LayoutGroup** bptr = (LayoutGroup**)b;
    LayoutGroup* agroup = *aptr;
    LayoutGroup* bgroup = *bptr;

    //
    // If  the height ranges don't overlap then return the one with
    // the lower y.
    //
    if ((agroup->orig_y1 < bgroup->orig_y1) && (agroup->orig_y2 < bgroup->orig_y2))
	return -1;
    if ((bgroup->orig_y1 < agroup->orig_y1) && (bgroup->orig_y2 < agroup->orig_y2))
	return 1;
    
    //
    // If the height ranges overlap, then return the one with the
    // large x.
    //
    if ((agroup->orig_x1 < bgroup->orig_x1) && (agroup->orig_x2 < bgroup->orig_x2))
	return -1;
    if ((bgroup->orig_x1 < agroup->orig_x1) && (bgroup->orig_x2 < agroup->orig_x2))
	return 1;

    if (((agroup->orig_y1 + agroup->orig_y2)>>1) < 
	((bgroup->orig_y1 + bgroup->orig_y2)>>1))  
	return -1;
    if (((agroup->orig_y1 + agroup->orig_y2)>>1) >
	((bgroup->orig_y1 + bgroup->orig_y2)>>1))  
	return 1;
    if (((agroup->orig_x1 + agroup->orig_x2)>>1) < 
	((bgroup->orig_x1 + bgroup->orig_x2)>>1))  
	return -1;
    if (((agroup->orig_x1 + agroup->orig_x2)>>1) >
	((bgroup->orig_x1 + bgroup->orig_x2)>>1))  
	return 1;
    return 0;
}

void GraphLayout::repositionGroups(Node* reflow[], int reflow_count)
{
    int i;
    LayoutGroup** groups = (LayoutGroup**)malloc(sizeof(LayoutGroup*) * 
	    this->layout_groups.getSize());
    ListIterator gi(this->layout_groups);
    LayoutGroup* group;
    int gcnt = 0;
    while  ((group=(LayoutGroup*)gi.getNext())) groups[gcnt++] = group;
    qsort (groups, gcnt, sizeof(LayoutGroup*), LayoutGroup::Comparator);
    int gx=0,gy=0;
    int prevy2 = 0;
    int dummy;
    int w=0,h=0;
    int max_height_in_line = 0;

    groups[0]->getOriginalXYPosition(dummy, prevy2);
    groups[0]->getOriginalXYSize(dummy, h);
    prevy2+= h;

    w=h=0;
    boolean isNewLine = FALSE;
    for (i=0; i<gcnt; i++) {
	group = groups[i];
	int x1,y1;
	int ow,oh;
	group->getOriginalXYPosition(x1,y1);
	group->getOriginalXYSize(ow,oh);
	int w,h;
	group->getXYSize(w,h);
	isNewLine = (y1 > prevy2);
	if (isNewLine) {
	    gx = 0;
	    gy+=max_height_in_line + GraphLayout::HeightPerLevel;
	    max_height_in_line = h;
	} else if (h>max_height_in_line) {
	    max_height_in_line = h;
	}
	group->setProposedLocation(gx,gy);
	//printf ("%s[%d] group %d at %d,%d\n", __FILE__,__LINE__,group->getId(), gx, gy);
	gx+= w + GraphLayout::GroupSpacing;
	prevy2 = y1+oh;
    }

    for (i=0; i<reflow_count; i++) {
	Node* n = reflow[i];
	NodeInfo* ninfo = (NodeInfo*)n->getLayoutInformation();
	LayoutGroup* group = ninfo->getLayoutGroup();
	int x,y;
	group->getProposedLocation (x,y);
	int x1,y1;
	group->getXYPosition (x1,y1);
	int nx,ny;
	ninfo->getXYPosition (nx, ny);
	ninfo->setProposedLocation (nx+x-x1, ny+y-y1);
    }
    free(groups);
}

NodeInfo::~NodeInfo() 
{
    if (this->connected_nodes) {
	if (this->owns_list)
	    delete this->connected_nodes;
    }
}

void NodeInfo::setConnectedNodes(List* list)
{
    if (this->owns_list) return ;
    this->connected_nodes = list;
}
List* NodeInfo::getConnectedNodes(Node* n)
{
    if (this->connected_nodes) return this->connected_nodes;

    this->connected_nodes = new List();
    this->owns_list = TRUE;
    NodeInfo::BuildList (n, this->connected_nodes);
    return this->connected_nodes;
}

void NodeInfo::BuildList(Node* n, List* nodes)
{
    if (nodes->isMember(n)) return ;
    nodes->appendElement(n);

    NodeInfo* info = (NodeInfo*)n->getLayoutInformation();
    if (info) info->setConnectedNodes(nodes);

    int input_count = n->getInputCount();
    int input;
    int dummy;
    for (input=1; input<=input_count; input++) {
	if (!n->isInputVisible(input)) continue;
	List* arcs = (List*)n->getInputArks(input);
	if (!arcs) continue;
	ListIterator iter(*arcs);
	Ark* arc;
	while ((arc=(Ark*)iter.getNext())) {
	    Node* source = arc->getSourceNode(dummy);
	    NodeInfo::BuildList(source, nodes);
	}
    }
    int output_count = n->getOutputCount();
    int output;
    for (output=1; output<=output_count; output++) {
	if (!n->isOutputVisible(output)) continue;
	List* arcs = (List*)n->getOutputArks(output);
	if (!arcs) continue;
	ListIterator iter(*arcs);
	Ark* arc;
	while ((arc=(Ark*)iter.getNext())) {
	    Node* destination = arc->getDestinationNode(dummy);
	    NodeInfo::BuildList(destination, nodes);
	}
    }
}

int GraphLayout::countConnectedInputs(Node* n, int input, int& nth_tab)
{
    int input_count = n->getInputCount();
    int i, connected=0;
    nth_tab = 1;
    for (i=1; i<=input_count; i++) {
	if (!n->isInputVisible(i)) continue;
	if (n->isInputConnected(i)) connected++;
	if (i<input) nth_tab++;
    }
    return connected;
}

int GraphLayout::countConnectedOutputs (Node* n, int output, int& nth_tab)
{
    int output_count = n->getOutputCount();
    int i, connected=0;
    nth_tab = 1;
    for (i=1; i<=output_count; i++) {
	if (!n->isOutputVisible(i)) continue;
	if (n->isOutputConnected(i)) connected++;
	if (i<output) nth_tab++;
    }
    return connected;
}

//
// 0) Decrement the hop count of any node that is connected to multiple
//    non-consecutive input tabs on 1 row.
// 1) Divide all nodes into rows.
// 2) Sort the contents of each row by increasing x coord of the destination.
// 3) Starting with the middle node in the sorted list, position each
//    above its destination or in the case of node traveling more than 1 hops,
//    above a space between nodes, or place the node directly above its destination
//    if that is available.
//
void LayoutGroup::layout(Node* node, GraphLayout* mgr, List& positioned)
{
    positioned.clear();

    //
    // Starting with all nodes in this layout group, form an
    // array that we can sort by hop count.  This will be used
    // to form rows.  A node that is in our group is copied into this
    // array if we're going to place the node on this trip up the
    // graph.  We make this decision based on the node's connectivity
    // (and in rare cases by type i.e. Get/Set)  We won't place a node
    // on this trip if the node ought to be placed during some other
    // trip up the graph.  I.O.W. if the node has a closer descendant,
    // then let that descendant take care of placing the node so that
    // the node will be placed in close proximity to the nearby
    // relative instead of close proximity to the distant relative.
    //
    List one_hop_up_nodes;
    List one_hop_up;
    mgr->getSpecialAncestorsOf(node, one_hop_up_nodes, one_hop_up);
#   if defined(DEBUG_PLACEMENT)
    if (debug) {
	fprintf (stdout, "%s[%d] Placing ancestors of %s:%d\n",
	    __FILE__,__LINE__,node->getNameString(), node->getInstanceNumber());
	ListIterator li(one_hop_up_nodes);
	Node* n;
	while (n=(Node*)li.getNext()) {
	    fprintf (stdout, "\t%s:%d\n", n->getNameString(), n->getInstanceNumber());
	}
    }
#   endif
    Node** node_array;
    node_array = (Node**)malloc((1+one_hop_up_nodes.getSize()) * sizeof(Node*));
    int i = 1;
    int nodes=0;
    int size = one_hop_up_nodes.getSize();
    ASSERT (one_hop_up_nodes.getSize() == one_hop_up.getSize());
    while (i<=size) {
	Node* n = (Node*)one_hop_up_nodes.getElement(i);
	node_array[nodes++] = n;
	positioned.appendElement(n);
	NodeInfo* info = (NodeInfo*)n->getLayoutInformation();
	info->setDestination ((Ark*)one_hop_up.getElement(i));
	i++;
    }
    // at this point nodes  may or may not be equal to ->getSize()
    // add our param manually because it's not one hop up.
    node_array[nodes++] = node;
    qsort (node_array, nodes, sizeof(Node*), LayoutGroup::SortByHop);

    //
    // Create one data structure for each row in the array.
    // nodes is the wrong number to use here, but rows is guaranteed to
    // be <= nodes
    LayoutRow** row_array = (LayoutRow**)malloc(sizeof(LayoutRow*) * nodes);
    LayoutRow* row = NUL(LayoutRow*);
    int prev = -1;
    i=0;
    int rows = 0;
    while (i<nodes) {
	node = node_array[i];
	NodeInfo* ninfo = (NodeInfo*)node->getLayoutInformation();
	if (ninfo->getGraphDepth() != prev) {
	    row_array[rows++] = row = new LayoutRow(ninfo->getGraphDepth());
	}
	row->addNode(node);
	prev = ninfo->getGraphDepth();
	i++;
    }
    int prev_id;
    row_array[0]->sort();
    row_array[0]->layout(mgr, prev_id);
    SlotList* slots = row_array[0]->getSlotList();
    for (i=1; i<rows; i++) {
	row_array[i]->sort();
	row_array[i]->layout(slots, mgr, prev_id);
	slots = row_array[i]->getSlotList();
    }

    //
    // provide additional spacing for crowded situations
    //
    this->straightenArcs(row_array, rows);

    for (i=0; i<rows; i++) 
	delete row_array[i];
    free(row_array);
    free(node_array);
}

void LayoutRow::sort()
{
    int size = this->nodes.getSize();
    this->node_array = (Node**)malloc(sizeof(Node*) * size);
    ListIterator iter(this->nodes);
    Node* n;
    int i = 0;
    while ((n=(Node*)iter.getNext())) this->node_array[i++] = n;
    qsort (this->node_array, size, sizeof(Node*), LayoutRow::SortByDestinationX);
    this->sorted_by_destination_x = TRUE;
    this->sorted_by_x = FALSE;
#   if defined(DEBUG_PLACEMENT)
    if (debug) {
	fprintf (stdout, "%s[%d] Row %d contains...\n", __FILE__,__LINE__, this->getId());
	int j;
	for (j=0; j<size; j++) {
	    fprintf (stdout, "\t%s:%d\n", this->node_array[j]->getNameString(),
		this->node_array[j]->getInstanceNumber());
	}
    }
#   endif
}

LayoutRow::~LayoutRow()
{
    if (this->node_array) free(this->node_array);
}

// Among the nodes we're placing, there are no connected outputs.  There
// should be only 1 node in this row.
void LayoutRow::layout(GraphLayout* mgr, int& previous_id)
{
    Node* n = this->node_array[0];
    NodeInfo* info = (NodeInfo*)n->getLayoutInformation();
    if (!info->isPositioned()) info->setProposedLocation(INITIAL_X, INITIAL_Y);
    int x,y,w,h;
    info->getXYSize(w,h);
    info->getXYPosition(x,y);
    int left_edge = x - (w>>1);
    int right_edge = x + (w>>1);
    this->slot_list.clear();
    previous_id = this->getId();
}

SlotList::~SlotList()
{
    ListIterator li(*this);
    Slot* slot;
    while ((slot=(Slot*)li.getNext())) delete slot;
}
//
// Return x if that location is empty.  Otherwise return the next location to
// the left or right that is empty.
//
int SlotList::isAvailable (int x, boolean left)
{
    ListIterator li(*this);
    Slot *slot, *prev=NUL(Slot*);
    while ((slot=(Slot*)li.getNext())) {
	if ((slot->min <= x) && (slot->max >= x)) return x;
	if ((slot->min >= x) && (left)) {
	    // this assert says that we can't request something less
	    // than INT_MIN and that before advancing min past x, we
	    // would have to have gone thru the loop at least 1 time.
	    ASSERT(prev);
	    return prev->max-5;
	} else if ((slot->min > x) && (!left)) {
	    return slot->min+5;
	}
	prev = slot;
    }
    li.setList(*this);
#   if defined(DEBUG_PLACEMENT)
    if (debug) {
	fprintf (stdout, "%s[%d] failed %d (%x)\n", __FILE__,__LINE__,x,left);
	while (slot=(Slot*)li.getNext()) {
	    fprintf (stdout, "\t %d <--> %d\n", slot->min, slot->max);
	}
    }
#   endif

    ASSERT(FALSE);
    return 0;
}

//
// split a Slot
//
void SlotList::occupy (int x, int width)
{
    int size = this->getSize();
    Slot *slot;
    int found = 0;
    int i;
    for (i=1; i<=size; i++) {
	slot = (Slot*)this->getElement(i);
	if ((slot->min <= x) && (slot->max >= x)) {
	    found = i;
	    break;
	}
    }
#   if defined(DEBUG_PLACEMENT)
    if (debug) {
	if (!found) {
	    for (i=1; i<=size; i++) {
		slot = (Slot*)this->getElement(i);
		fprintf (stdout, "\t %d - %d\n", slot->min, slot->max);
	    }
	}
    }
#   endif
    ASSERT(found);

    int right = x+width;
    if (right < slot->max) {
	int old_max = slot->max;
	slot->max = x;
	this->insertElement (new Slot(right, old_max), found+1);
    } else {
	slot->max = x;
	Slot* prev=NUL(Slot*);
	List to_delete;
	for (i=found+1; i<=size; i++) {
	    slot = (Slot*)this->getElement(i);
	    if (slot->max > right) {
		if (slot->min < right) {
		    slot->min = right;
		} else {
#                   if defined(DEBUG_PLACEMENT)
		    if (debug) {
			fprintf (stdout, "%s[%d] Warning, splitting a slot failed\n", 
				__FILE__,__LINE__);
			Slot* s;
			for (int j=1; j<=size; j++) {
			    s = (Slot*)this->getElement(j);
			    fprintf (stdout, "\t %d to %d\n", s->min, s->max);
			}
		    }
#                   endif
		}
		break;
	    } else {
		to_delete.appendElement(slot);
	    }
	    prev = slot;
	}
	ListIterator iter(to_delete);
	while ((slot=(Slot*)iter.getNext())) {
	    ASSERT(this->removeElement(slot));
	    delete slot;
	}
    }
}

//
// 3) Starting with the middle node in the sorted list, position each
//    above its destination or in the case of node traveling more than 1 hop,
//    above a space between nodes, or place the node directly above its destination
//    if that is available.
//
void LayoutRow::layout(SlotList* slots, GraphLayout* mgr, int& previous_id)
{
    int size = this->nodes.getSize();
    int middle = (size>>1);
    int i,left_edge,right_edge;


    //
    // The decision to err on the side of {left,right}-wards movment is made
    // based on the location of the destination's input tab.  If the tab is more
    // than half-way across the standIn, then we favor moving right.
    //
    NodeInfo* info = (NodeInfo*)this->node_array[middle]->getLayoutInformation();
    boolean go_left = this->favorsLeftShift (info->getDestination(), mgr, TRUE);

    if (go_left) {
	left_edge  = -9999999; // could also be INT_MIN, INT_MAX
	right_edge =  9999999; 
    } else {
	left_edge  =  9999999; // could also be INT_MIN, INT_MAX
	right_edge = -9999999; 
    }

    this->position (this->node_array[middle], left_edge, right_edge, mgr, go_left, slots, previous_id);

    int saved_left_edge = left_edge;
    int saved_right_edge = right_edge;


    // for each node left and right of center
    go_left = TRUE;
    for (i=middle-1;i>=0;i--) {
	this->position (this->node_array[i], left_edge, right_edge, mgr, go_left, slots, previous_id);
    }
    go_left = FALSE;
    left_edge = saved_left_edge;
    right_edge = saved_right_edge;
    for (i=middle+1;i<size;i++) {
	this->position (this->node_array[i], left_edge, right_edge, mgr, go_left, slots, previous_id);
    }

    previous_id = this->getId();
}
void LayoutRow::position (Node* n, int& left_edge, int& right_edge, 
	GraphLayout* lay, boolean go_left, SlotList* slots, int previous_id)
{
    int half_tab_width = 9;
    int x,y,dx,dy,w,h;
    NodeInfo* info = (NodeInfo*)n->getLayoutInformation();
    info->getXYSize(w,h);
    int offset = 0;
    Ark* arc = info->getDestination();
    int hop;
    int output;
    arc->getSourceNode(output);

    StandIn *si = n->getStandIn();
    int tabx = si->getOutputParameterTabX(output);
    int xcoord_of_line = info->getDestinationLocation(hop);
    ASSERT (hop > info->getGraphDepth());
    if (go_left) {
	int desired_right_edge = xcoord_of_line + w - (tabx+half_tab_width); 
	if ((left_edge-REQUIRED_HSEP) > desired_right_edge) {
	} else if ((left_edge-GraphLayout::NodeSpacing) < desired_right_edge) {
	    offset = (left_edge-GraphLayout::NodeSpacing) - desired_right_edge;
	}
    } else {
	int desired_left_edge = xcoord_of_line - (half_tab_width + tabx);
	if ((right_edge+REQUIRED_HSEP) < desired_left_edge) {
	} else if ((right_edge+GraphLayout::NodeSpacing) > desired_left_edge) {
	    offset = (right_edge+GraphLayout::NodeSpacing) - desired_left_edge;
	}
    }
    //
    // If we can make the arc straight, then notify the destination that
    // he has achieved straightness and doesn't need to have his incoming
    // arcs further straightened, which will happen post-layout in 
    // LayoutGroup::straightenArcs();
    //
    int dummy;
    Node* dest = arc->getDestinationNode(dummy);
    NodeInfo* dinfo = (NodeInfo*)dest->getLayoutInformation();

    int target_x = xcoord_of_line + offset;
    // If we're wired to something on the previous row which is usually 1 hop down.
    // It could also be that the 2 nodes could be like Supervise{State,Window} with
    // much different hop counts but with no intervening rows.
    if ((hop == (info->getGraphDepth()+1)) || (hop == previous_id)) {
	if (lay->positionSource(info->getDestination(),target_x)) {
#           if defined(DEBUG_PLACEMENT)
	    if (debug) {
		fprintf (stdout, "%s[%d] %s:%d collided\n", __FILE__,__LINE__,
			n->getNameString(), n->getInstanceNumber());
	    }
#           endif
	}
	dinfo->registerStraightArc(offset, arc);
    } else {
	int available_x = slots->isAvailable(target_x, go_left);
	if (lay->positionSource(info->getDestination(),available_x)) {
#           if defined(DEBUG_PLACEMENT)
	    if (debug) {
		fprintf (stdout, "%s[%d] %s:%d collided\n", __FILE__,__LINE__,
			n->getNameString(), n->getInstanceNumber());
	    }
#           endif
	}
	dinfo->registerStraightArc(available_x-xcoord_of_line, arc);
    }
    info->getXYPosition(x,y);
#   if defined(DEBUG_PLACEMENT)
    if (debug) {
	fprintf (stdout, "%s[%d] %s:%d placed at %d,%d occupies %d to %d\n",
	    __FILE__,__LINE__,n->getNameString(),n->getInstanceNumber(),x,y,
	    x, x+w);
    }
#   endif
    // What's the meaning of the -3,+6?  Really only the space from x to x+w is
    // occupied.  Nevertheless, the line router won't place a line closer to
    // a node than 3 pixels.  So we pretend that we're occupying 3 pixels
    // either side of the left and right edges.  That way we won't try to 
    // use those areas in a subsequent pass.
    this->slot_list.occupy (x-3, w+6);
    left_edge = x;
    right_edge = x+w;
}

int NodeInfo::getDestinationLocation (int& hop)
{
    int input;
    Node* dst = this->destination->getDestinationNode(input);
    NodeInfo* dinfo = (NodeInfo*)dst->getLayoutInformation();
    hop = dinfo->getGraphDepth();
    StandIn* si = dst->getStandIn();
    int half_tab_width = 9;
    int dx,dy;
    dinfo->getXYPosition(dx,dy);
    int tabx = si->getInputParameterTabX(input);
    return tabx + dx + half_tab_width;
}

int LayoutGroup::SortByHop (const void* a, const void* b)
{
    Node** aptr = (Node**)a;
    Node** bptr = (Node**)b;
    Node* anode = (Node*)*aptr;
    Node* bnode = (Node*)*bptr;
    NodeInfo* ainfo = (NodeInfo*)anode->getLayoutInformation();
    NodeInfo* binfo = (NodeInfo*)bnode->getLayoutInformation();

    // descending order of hop count
    int ahops = ainfo->getGraphDepth();
    int bhops = binfo->getGraphDepth();
    if (ahops > bhops) return -1;
    if (ahops < bhops) return  1;
    return 0;
}

int LayoutRow::SortByDestinationX (const void* a, const void* b)
{
    int ainput,binput,dummy;
    Node** aptr = (Node**)a;
    Node** bptr = (Node**)b;
    Node* anode = (Node*)*aptr;
    Node* bnode = (Node*)*bptr;
    NodeInfo* ainfo = (NodeInfo*)anode->getLayoutInformation();
    NodeInfo* binfo = (NodeInfo*)bnode->getLayoutInformation();

    int half_tab_width = 9;
    int ai = ainfo->getDestinationLocation(dummy);// + half_tab_width;
    int bi = binfo->getDestinationLocation(dummy);// + half_tab_width;
    if (ai < bi) return -1;
    if (ai > bi) return  1;

    int ahops = ainfo->getGraphDepth();
    int bhops = binfo->getGraphDepth();
    if (ahops > bhops) return -1;
    if (ahops < bhops) return  1;

    return 0;
}

int LayoutRow::SortByX (const void* a, const void* b)
{
    int ainput,binput,ax,bx,dummy;
    Node** aptr = (Node**)a;
    Node** bptr = (Node**)b;
    Node* anode = (Node*)*aptr;
    Node* bnode = (Node*)*bptr;
    NodeInfo* ainfo = (NodeInfo*)anode->getLayoutInformation();
    NodeInfo* binfo = (NodeInfo*)bnode->getLayoutInformation();

    ainfo->getXYPosition(ax,dummy);
    binfo->getXYPosition(bx,dummy);
    if (ax < bx) return -1;
    if (ax > bx) return  1;
    return 0;
}

void GraphLayout::getSpecialAncestorsOf (Node* root, List& ancestors, List& arcs, boolean last_call)
{
    int input, output;
    int input_count = root->getInputCount();
    int k;
    for (k=1; k<=input_count; k++) {
	if (!root->isInputVisible(k)) continue;
	List* inputs = (List*)root->getInputArks(k);
	if (!inputs) continue;
	ListIterator li(*inputs);
	Ark* arc;
	int dummy;
	while ((arc=(Ark*)li.getNext())) {
	    Node* src = arc->getSourceNode(output);
	    LayoutInfo* linfo = (LayoutInfo*)src->getLayoutInformation();
	    if (linfo->isPositioned()) continue;
	    if (ancestors.isMember(src) == FALSE) {
		const char* name = src->getNameString();
		if (EqualString("Get",name)||
		    EqualString("GetGlobal",name)||
		    EqualString("GetLocal",name)) {
		    if (output != 2) continue;
		    arcs.appendElement(arc);
		    ancestors.appendElement(src);
		    this->getSpecialAncestorsOf(src, ancestors, arcs, FALSE);
		} else if (this->hasNoCloserDescendant(src, root)) {
		    // only handle distant ancestor nodes if those node don't
		    // have a more closely related descendant.
		    arcs.appendElement(arc);
		    ancestors.appendElement(src);
		    this->getSpecialAncestorsOf(src, ancestors, arcs, FALSE);
		}
	    }
	}
    }

    if (last_call) {
	//
	// Examine each arc we've picked.  See if each arc ought to be replaced
	// with a better choice.  A choice is better if the src and dst nodes
	// have multiple arcs and it's more near the center of the set of arcs.
	//
	ListIterator iter(arcs);
	List to_add;
	List to_remove;
	Ark* arc;
	while ((arc=(Ark*)iter.getNext())) {
	    Node* src = arc->getSourceNode(output);
	    Node* dst = arc->getDestinationNode(input);
	    int cnt = this->countConnectionsBetween(src, dst);
	    if (cnt<=2) continue;
	    int to_skip = cnt>>1;
	    int skipped = 0;
	    input_count = dst->getInputCount();
	    boolean changed = FALSE;
	    for (input=1; input<=input_count; input++) {
		if (!dst->isInputVisible(input)) continue;
		List* inputs = (List*)dst->getInputArks(input);
		if (!inputs) continue;
		ListIterator li(*inputs);
		Ark* ia;
		while ((ia=(Ark*)li.getNext())) {
		    int output2;
		    Node* source = ia->getSourceNode(output2);
		    if (source == src) {
			if (skipped == to_skip) {
			    if (ia != arc) {
				to_add.appendElement(ia);
				to_remove.appendElement(arc);
#                               if defined(DEBUG_PLACEMENT)
				if (debug) {
				    fprintf (stdout, "%s[%d] replacing output %d of %s"
					" with output %d\n", __FILE__,__LINE__,
					output, src->getNameString(), output2);
				}
#                               endif
			    }
			    changed = TRUE;
			    break;
			}
			skipped++;
		    }
		}
		if (changed) break;
	    }
	}
	iter.setList(to_remove);
	while ((arc=(Ark*)iter.getNext())) 
	    arcs.removeElement(arc);
	iter.setList(to_add);
	while (arc=(Ark*)iter.getNext()) 
	    arcs.appendElement(arc);
	ancestors.clear();
	int size = arcs.getSize();
	for (int i=1; i<=size; i++) {
	    arc = (Ark*)arcs.getElement(i);
	    int dummy;
	    Node* src = arc->getSourceNode(dummy);
	    ancestors.appendElement(src);
	}
    }
}

//
// If we can't decide left vs right, then return the default value
//
boolean LayoutRow::favorsLeftShift (Ark* arc, GraphLayout* mgr, boolean default_value)
{
    boolean retval = default_value;

    int input, output, nth_tab;
    Node* src = arc->getSourceNode(output);
    Node* dst = arc->getDestinationNode(input);

    int ci = mgr->countConnectedInputs (dst, input, nth_tab);
    int input_count = dst->getInputCount();
    int visible_input_count = 0;
    for (input=1; input<input_count; input++) {
	if (dst->isInputVisible(input))
	    visible_input_count++;
    }

    if (nth_tab > (ci>>1)) {
	retval = FALSE;
    } else if (input > (visible_input_count>>1)) {
	retval = FALSE;
    }

    return retval;
}

void SlotList::clear()
{
    ListIterator iter(*this);
    Slot* slot;
    while ((slot=(Slot*)iter.getNext())) delete slot;
    this->List::clear();
    this->appendElement(new Slot(-999999, 999999));
}

int GraphLayout::countConnectionsBetween (Node* source, Node* dest, boolean count_consecutive)
{
    int output_count = source->getOutputCount();
    int output, input;
    int connections = 0;
    int prev = -1;
    for (output=1; output<=output_count; output++) {
	if (!source->isOutputVisible(output)) continue;
	List* arks = (List*)source->getOutputArks(output);
	if (!arks) continue;
	ListIterator iter(*arks);
	Ark* arc;
	while ((arc=(Ark*)iter.getNext())) {
	    Node* destination = arc->getDestinationNode(input);
	    if (destination == dest) {
		if (count_consecutive) {
		    connections++;
		} else {
		    int nth_tab;
		    this->countConnectedInputs(destination, input, nth_tab);
		    if (nth_tab != (prev+1)) {
			connections++;
		    }
		    prev = nth_tab;
		}
	    }
	}
    }
    return connections;
}

//
// Starting with the nodes at the top, try to shift them out farther in order
// to straighten some arc.  Nodes that get in the way are in turn
// shifted outward providing more space. 
//
boolean LayoutGroup::straightenArcs (LayoutRow* row_array[], int rows)
{
    boolean retval = FALSE;
    
    for (int i=rows-1; i>=0; i--) {
	LayoutRow* row = row_array[i];
	row->straightenArcs(retval);
    }
    return retval;
}

void LayoutRow::straightenArcs(boolean& changes_made_on_previous_row)
{
    if (!this->sorted_by_x) {
	qsort (this->node_array, this->nodes.getSize(), 
	    sizeof(Node*), LayoutRow::SortByX);
	this->sorted_by_x = TRUE;
	this->sorted_by_destination_x = FALSE;
    }

    boolean changes_made = changes_made_on_previous_row;
    boolean changes_made_on_current_row = FALSE;

    int input, output;
    int size = this->nodes.getSize();
    int midpt = size>>1;
    for (int j=0; j<size; j++) {
	Node* dest = this->node_array[j];
	NodeInfo* dinfo = (NodeInfo*)dest->getLayoutInformation();
	Ark* arc;
	int offset;
	arc = dinfo->hasStraightArc(offset);
	if (arc == NUL(Ark*)) continue;

	//
	// problem right here is that, the value we've recorded for
	// offset might not be correct any more if the src node was
	// moved in an earlier pass. FIXME
	//
	if ((!changes_made) && (!offset)) continue;

	ASSERT (dest == arc->getDestinationNode(input));
	Node* src = arc->getSourceNode(output);
	NodeInfo* sinfo = (NodeInfo*)src->getLayoutInformation();

	int sx,sy;
	sinfo->getXYPosition(sx,sy);
	int dx,dy;
	dinfo->getXYPosition(dx,dy);

	//
	// Compute the amount the dest has to shift in order to straighten
	// the arc.  This is based on the the locations of the standIns and
	// the relative locations of their input and output tabs.
	//
	StandIn* ssi = src->getStandIn();
	StandIn* dsi = dest->getStandIn();
	int dtx = dsi->getInputParameterTabX(input);
	int stx = ssi->getOutputParameterTabX(output);

	int delta = - ((dx+dtx) - (sx + stx));
	int end, incr;

	if (delta < 0) {
	    end = -1;
	    incr = -1;
	} else if (delta > 0) {
	    end = size;
	    incr = 1;
	} else {
	    dinfo->registerStraightArc (0, arc);
	    continue;
	}

	for (int k=j; k!=end; k+=incr) {
	    Node* n = this->node_array[k];
	    NodeInfo* info = (NodeInfo*)n->getLayoutInformation();
#	    if defined(DEBUG_PLACEMENT)
	    if (debug) {
		if (k==j) {
		    fprintf (stdout, "%s[%d] moving %s:%d %d row(%d)\n", 
			__FILE__,__LINE__, n->getNameString(), 
			n->getInstanceNumber(), delta, this->getId());
		} else {
		    fprintf (stdout, "\tmoving %s:%d \n", 
			n->getNameString(), n->getInstanceNumber());
		}
	    }
#	    endif
	    info->getXYPosition(dx,dy);
	    int dx2 = dx + delta;
	    info->setProposedLocation (dx2, dy);


	    //
	    // continue scooting neighbors over until we have created enough
	    // whitespace so that it's no longer necessary.
	    //
	    boolean keep_scooting = TRUE;

	    int nextk = k+incr;
	    if (nextk != end) {
		Node* nn = this->node_array[nextk];
		int nnx,nny;
		NodeInfo* nninfo = (NodeInfo*)nn->getLayoutInformation();
		nninfo->getXYPosition(nnx,nny);
		if (incr > 0) {
		    // does the right edge of n encroach on the left edge of nn
		    int dw,dh;
		    info->getXYSize(dw,dh);
		    if ((dx2 + dw + REQUIRED_HSEP) < nnx) 
			keep_scooting = FALSE;
		} else {
		    // does the left edge of n encroach on the right edge of nn
		    int nnw,nnh;
		    nninfo->getXYSize(nnw,nnh);
		    if ((nnx+nnw+REQUIRED_HSEP) < dx2) 
			keep_scooting = FALSE;
		}
	    }

	    if (!keep_scooting) break;
	}
	changes_made = TRUE;
	changes_made_on_current_row = TRUE;
    }
    if (!changes_made_on_current_row) changes_made = FALSE;
}

Ark* NodeInfo::hasStraightArc(int& offset)
{
    if (!this->straightness_set) return NUL(Ark*);
    offset = this->offset_for_straightness;
    return this->straightness_opportunity;
}

void NodeInfo::registerStraightArc(int offset, Ark* arc)
{
    int dummy;
    Node* src;
    NodeInfo* sinfo;

    //
    // The this is the info for the node at the destination end of arc
    //
    if (!this->straightness_set) {
	this->offset_for_straightness = offset;
	this->straightness_opportunity = arc;
	this->straightness_set = TRUE;

	src = arc->getSourceNode(dummy);
	sinfo = (NodeInfo*)src->getLayoutInformation();
	sinfo->setStraightnessDestination(this);

	return ;
    }
    if (abs(offset) < abs(this->offset_for_straightness)) {
	this->offset_for_straightness = offset;

	src = arc->getSourceNode(dummy);
	sinfo = (NodeInfo*)src->getLayoutInformation();
	sinfo->setStraightnessDestination(NUL(NodeInfo*));
	this->straightness_opportunity = arc;
	
	// find the NodeInfo at the src end of the arc.
	// Notify that object that this of a partnership.
	src = arc->getSourceNode(dummy);
	sinfo = (NodeInfo*)src->getLayoutInformation();
	sinfo->setStraightnessDestination(this);
    }
}

//
// The purpose for shiftStraightArc(), setStraightnessDestination(), 
// and a virtual setProposedLocation() is to modify our notion
// of straightness so that if a src is moved in order to straighten
// one of its incoming arcs, we'll notify downstream arcs that had
// once believed themselves to be straight, that they're no longer
// straight.  All these methods are used as a result of ::straightenArcs().
// Note that by storing a reference to a NodeInfo* inside the 
// NodeInfo* object we create a potential crash bug if that NodeInfo*
// is deleted.
//
void NodeInfo::shiftStraightArc(int dx)
{
    if (!this->straightness_set) return ;
    this->offset_for_straightness+= dx;
}

void NodeInfo::setProposedLocation (int x, int y)
{
    int dx;
    boolean was_positioned = this->positioned_yet;

    if (was_positioned) dx = x - this->x;

    this->LayoutInfo::setProposedLocation(x,y);
    if (!was_positioned) return ;

    //
    // If the destination is using this node as it's
    // targeted straight arc and the destination thought
    // that the arc was stragith, then ensure that the
    // destination knows that it's arc isn't straight
    // any longer.
    //
    if (this->straightness_destination) 
	this->straightness_destination->shiftStraightArc(dx);
}

void NodeInfo::setStraightnessDestination(NodeInfo* dest)
{
    this->straightness_destination = dest;
}
