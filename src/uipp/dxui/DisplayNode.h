/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"


#ifndef _DisplayNode_h
#define _DisplayNode_h



#include "DrivenNode.h"


#define WHERE                   3

//
// Class name definition:
//
#define ClassDisplayNode	"DisplayNode"

//
// Referenced Classes
class ImageWindow;
class Network;
class PanelAccessManager;

//
// DisplayNode class definition:
//				
class DisplayNode : public DrivenNode
{
  private:
    //
    // Private member data:
    //
    static void HandleImageMessage(void *clientData, int id, void *line);
    PanelAccessManager *panelAccessManager;

    boolean      printCommonComments(FILE *f, const char *indent = NULL);

    boolean      parseCommonComments(const char *comment, const char *file,
			 int lineno);
    //
    // read/written to the .cfg file and passed to the editor window
    // when it is created.
    //
    int xpos,ypos, width, height; 

  protected:
    //
    // Protected member data:
    //
    boolean	userSpecifiedWhere;
    ImageWindow *image;
    char	*title;
    int		depth;
    int		windowId;
    boolean     lastImage;

    virtual void handleImageMessage(int id, const char *line);

    void         prepareToSendValue(int index, Parameter *p);
    void         prepareToSendNode();

    virtual boolean netPrintAuxComment(FILE *f);
    virtual boolean netParseAuxComment(const char *comment,
			 const char *file,
			 int lineno);

    virtual char        *inputValueString(int i, const char *prefix);
    virtual boolean      printIOComment(FILE *f, boolean input, int index,
				const char *indent = NULL, 
				boolean valueOnly = FALSE);

    //
    // Search through the networks list of image windows trying to find one
    // that is not associated with a (display) node.  If canBeAnchor is TRUE
    // then any ImageWindow will do and if available we return an anchor
    // window, otherwise the returned ImageWindow must not by an anchor window.
    // If one is not found in the current list, then create one if requested.
    //
    ImageWindow *getUnassociatedImageWindow(
			boolean alloc_one = TRUE, boolean canBeAnchor = TRUE);

    virtual void switchNetwork(Network *from, Network *to, boolean silently=FALSE);

    //
    // Update any UI visuals that may be based on the state of this
    // node.   Among other times, this is called after receiving a message
    // from the executive.
    //
    virtual void reflectStateChange(boolean unmanage);

    //
    // Parse the node specific info from an executive message.
    // Returns the number of attributes parsed.
    //
    virtual int  handleNodeMsgInfo(const char *line);


    //
    // This node does not have a message id param, so it 
    // returns 0.
    //
    virtual int getMessageIdParamNumber(); 

    //
    // Monitor the status of the WHERE param.  If the tab is connected, then
    // treat it as if the user had supplied a value.
    //
    virtual void ioParameterStatusChanged(boolean input, int index,
			NodeParameterStatusChange status);

  public:
    //
    // Constructor:
    //
    DisplayNode(NodeDefinition *nd, Network *net, int instnc);

    //
    // Destructor:
    //
    ~DisplayNode();

    virtual boolean cfgParseComment(const char* comment,
                                const char* filename, int lineno);
    virtual boolean cfgPrintNode(FILE *f, PrintType);

    virtual boolean initialize();

    virtual void setTitle(const char *title, boolean fromServer = FALSE);
    virtual const char *getTitle();

    void setDepth(int depth);
    int getDepth() { return this->depth;}

    virtual Type setInputValue(int index,
		       const char *value,
		       Type t = DXType::UndefinedType,
		       boolean send = TRUE);
    virtual boolean associateImage(ImageWindow *w);
    void    notifyWhereChange(boolean send);
    virtual void    openImageWindow(boolean manage = TRUE);

#if WORKSPACE_PAGES
    virtual void setGroupName(GroupRecord *grec, Symbol);
#else
    virtual void setGroupName(const char *name);
#endif
    virtual void setDefaultCfgState();


    void    setLastImage(boolean last);
    boolean isLastImage();


    PanelAccessManager* getPanelManager()
    {
	return this->panelAccessManager;
    }

    virtual boolean useSoftwareRendering(){return TRUE;};

    virtual DXWindow *getDXWindow() {return (DXWindow *)(this->image);}

    //
    // Determine if this node is a node of the given class
    //
    virtual boolean isA(Symbol classname);

    //
    // Return TRUE if this node has state that will be saved in a .cfg file.
    //
    virtual boolean hasCfgState();

    virtual boolean needsFastSort() { return TRUE; }

    //
    // Returns a pointer to the class name.
    //
    virtual const char* getClassName()
    {
	return ClassDisplayNode;
    }
};


#endif // _DisplayNode_h
