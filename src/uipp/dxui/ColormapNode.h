/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"


#ifndef _ColormapNode_h
#define _ColormapNode_h


#include "DrivenNode.h"


//
// Class name definition:
//
#define ClassColormapNode	"ColormapNode"

//
// Referenced Classes
class ColormapEditor;
class DeferrableAction;

//
// ColormapNode class definition:
//				
class ColormapNode : public DrivenNode
{
  friend class ColormapEditor;
  friend class ColormapEditCommand;
  friend class ColormapAddCtlDialog;
  friend class ColormapNBinsDialog;

  private:
    //
    // Private member data:
    //

    char	*title;

    //
    // read/written to the .cfg file and passed to the editor window
    // when it is created.
    //
    int	    xpos, ypos, width, height;

    char*   netSavedCMFilename;	// Name of .cm file stored in .net file
    char    *histogram;		// IntegerList of histgram values

    static void UpdateAttrParam(void *d, void *request_data);
    DeferrableAction *updateMinMaxAttr;
    void undeferMinMaxUpdates();
    void deferMinMaxUpdates();

    //
    // Rescale the values from the old min and max to the new ones.
    //
    static void RescaleDoubles(double *values, int count,
                                double newmin, double newmax);

    //
    // Install the given colormap and opacity map (control points) into the
    // node. *_data are the data values (should be within min and max) that
    // map onto the hue/sat/val or opacity maps in a 1 to 1 correspondence.
    // hsv_count is the number of items in hsv_data/hue/sat/val and
    // op_count is the number of items in op_data/opacity.
    // Currently, hsv_count and op_count should be greater than 0
    // for a change to take place to the respective maps.
    // send_how is one of
    //      1) SEND_NOW(0)     ; sends the value to the executive;
    //      2) NO_SEND (1)     ; don't send the values at all
    //      3) SEND_QUIET(2)   ; don't send the values without causing an 
    //				execution
    //
    //
    void installMaps(
                int h_count,  double  *h_level, double  *h_value,
                int s_count,  double  *s_level, double  *s_value,
                int v_count,  double  *v_level, double  *v_value,
                int op_count, double *op_level, double *op_value,
                boolean send_how);

    //
    // Save or read a .cm file which contains control point information about
    // the colormap editor.  If an error occurs an error message is generated
    // and FALSE is returned, otherwise TRUE.  If a failure occurs while reading
    // the .cm file and useDefaultOnOpenError is TRUE,  then the default map 
    // is installed (without being sent to the executive). 
    //
    boolean cmAccessFile(const char *cmapfile, boolean opening, 
					boolean useDefaultOnOpenError=TRUE);

    //
    // This method implements backwards compatibility for v 2.0.2 nets
    // and older.  Upon entering this method, it is assumed that the first
    // 4 inputs (private and hidden) contain old style output values.
    // After version 2.0.2 (next was 2.1) .net files the first four
    // inputs (unviewable) changed meaning.  They used to contain
    //          #1) hsvdata     (scalar list)
    //          #2) hsv         (3-vector list)
    //          #3) opdata      (scalar list)
    //          #4) op          (scalar list)
    // With hsvdata in 1:1 correspondence with hsv and the same for
    // opdata and op.
    // After version 2.0.2, we change the inputs as follows
    //          #1) huepts      (2-vector list)
    //          #2) satpts      (2-vector list)
    //          #3) valepts     (2-vector list)
    //          #4) oppts       (2-vector list)
    // Each vector is a 2-vector representing the position of a control
    // point in the color map where the first component is the data
    // value and the 2nd is the h/s/v/opacity value.
    //
    // This method converts the old values to the new values. Note, that
    // it may create more control points that were in the original map.
    //
    void convertOldInputs();

    //
    // Install the currently defined default min or max and send it
    // to the executive if requested.
    //
    void installCurrentDefaultMinOrMax(boolean min, boolean send);

    //
    //  Print/parse comments that are common to both .nets and .cfgs.
    //
    boolean printCommonComments(FILE *f, const char *indent = NULL);
    boolean parseCommonComments(const char* comment,
                                        const char *file,
                                        int lineno);

  protected:
    //
    // Protected member data:
    //
    ColormapEditor* colormapEditor;

    //
    // List of control points fed to us by the module message.
    //

    boolean netParseComment(const char* comment,
                                   const char* filename, int lineno);

    virtual boolean netPrintAuxComment(FILE *f);
    virtual boolean  netParseAuxComment(const char* comment,
                                                const char *file, 
						int lineno);

    void sendValue();

    //
    // Returns the number of attributes parsed.
    // Handles messages with 
    //   'min=%g' max='%g' 'histgram={%d %d ... }'
    // Returns the number of the above found in the given message.
    //
    virtual int handleNodeMsgInfo(const char *line);

    //
    // Update any UI visuals that may be based on the state of this
    // node.   Among other times, this is called after receiving a message
    // from the executive.
    //
    virtual void reflectStateChange(boolean unmanage /* ignored */);

    boolean initNBinsValue(const char *val);
    boolean setNBinsValue(double nbins);
    boolean initMinimumValue(const char *val);
    boolean setMinimumValue(double min);
    boolean initMaximumValue(const char *val);
    boolean setMaximumValue(double min);

    //
    // If the data input is disconnected, NULL the histogram data and then
    // call the superclass method.
    //
    virtual void ioParameterStatusChanged(boolean input, int index,
				NodeParameterStatusChange status); 

    //
    // Given two arrays of data levels and h/s/v/opacity value, construct
    // the vector list representing the given values.  Each 2-vector contains
    // a level and a corresponding value with the level in the 1st component.
    // If count is given as 0, the string value "NULL" is returned.
    // If valstr is provided, then it is used to place the vector list in and
    // is used as the return value. 
    // If not, then a new string which must be freed by the caller is 
    // returned.
    //
    char *makeControlPointList(int count,
                        double *level, double *value,
                        char *valstr = NULL);

    //
    // Parse lines out of the color map file that describe a particular
    // map (either hue/sat/val/opacity).  The map is parsed into a VectorList
    // and returned.  The returned string must be freed by the caller.  If
    // there are any errors parsing the file, then NULL is returned.
    //
    char *cmParseMap(FILE *fp, double min, double max);
    //
    // Print lines to the color map file that describe a particular
    // map (either hue/sat/val/opacity).
    // No error message is issued if an error occurs.
    //
    boolean cmPrintMap(FILE *fp, double min, double max, const char *map);

    //
    // Install the default h/s/v/opacity colormap and send it to the executive
    // if requested.
    //
    void installTHEDefaultMaps(boolean send = TRUE);

    //
    // The colormap node is always present in the network (i.e. there is a
    // module that executes on its behalf) and may receive messages even
    // when it does not have input connections.  We BELIEVE that the only
    // time it will receive messages while not having any connections is
    // when the network is marked dirty (i.e. the first execution of a new
    // network).
    //
    virtual boolean expectingModuleMessage();


  public:
    //
    // Constructor:
    //
    ColormapNode(NodeDefinition *nd, Network *net, int instnc);

    //
    // Destructor:
    //
    ~ColormapNode();

    virtual boolean initialize();
    void openDefaultWindow(Widget);

    //
    // Let the caller of openDefaultWindow() know what kind of window she's getting.
    // This is intended for use in EditorWindow so that we can sanity check the number
    // of cdbs were going to open before kicking off the operation and so that we
    // don't question the user before opening large numbers of interactors.
    // A name describing the type of window can be written into window_name in order
    // to enable nicer warning messages.
    //
    virtual boolean defaultWindowIsCDB(char* window_name = NULL)
	{ if (window_name) strcpy (window_name, "Colormap Window"); return FALSE; }

    boolean isTitleVisuallyWriteable();
    void setTitle(const char *title, boolean fromServer = FALSE);
    virtual const char *getTitle();

    virtual DXWindow *getDXWindow()
	{return (DXWindow *)(this->colormapEditor);}

    boolean isNBinsAlterable();
    double  getMinimumValue();
    double  getMaximumValue();
    double  getNBinsValue();


    //
    // Returns a string vector list with each 2-vector containing a
    // data level and a h/s/v/opacity value. 
    //
    const char *getHueMapList();
    const char *getSaturationMapList();
    const char *getValueMapList();
    const char *getOpacityMapList();

    //
    // Get the name of the colormap file that was saved along with the
    // .net file.
    //
    const char *getNetSavedCMFilename() { return this->netSavedCMFilename; }

    //
    // Returns the array of histogram values with the size of the array returned
    // in nhist.  The returned array must be freed by the caller.
    //
    int *getHistogram(int *nhist);

    //
    // Install the new control points, and update the executive quietly (i.e.
    // with no executions when in execute-on-change mode).
    //
    void installNewMapsQuietly(
                int h_count,  double  *h_level, double  *h_value,
                int s_count,  double  *s_level, double  *s_value,
                int v_count,  double  *v_level, double  *v_value,
                int op_count, double *op_level, double *op_value);
    //
    // Install the new control points, and if requested, send the values
    // the to executive.
    //
    void installNewMaps(
                int h_count,  double  *h_level, double  *h_value,
                int s_count,  double  *s_level, double  *s_value,
                int v_count,  double  *v_level, double  *v_value,
                int op_count, double *op_level, double *op_value,
                boolean send);


    //
    // Install the currently defined default hsv colormap and send it
    // to the executive if requested.
    //
    void installCurrentDefaultHSVMap(boolean send);
    boolean hasDefaultHSVMap();
    //
    // Install the currently defined default opacity colormap and send it
    // to the executive if requested.
    //
    void installCurrentDefaultOpacityMap(boolean send);
    boolean hasDefaultOpacityMap();
    //
    // Install the currently defined default min and max and send it
    // to the executive if requested.
    //
    void installCurrentDefaultMin(boolean send);
    void installCurrentDefaultMax(boolean send);
    void installCurrentDefaultMinAndMax(boolean send);
    boolean hasDefaultMin();
    boolean hasDefaultMax();


    //
    // Read the .cm file and set the 1st - 4th inputs and the min/max with
    // the values found there.  If send is TRUE, then update the executive.
    // If an error occurs and installDefaultOnError is set, then the default
    // colormap is installed.
    //
    boolean cmOpenFile(const char *cmapfile, 
			boolean send = TRUE, 
			boolean installDefaultOnError = TRUE);

    //
    // Save and retrieve image window state (camera, approx, render mode...)
    // to and from the .cfg file.
    //
    virtual boolean hasCfgState();
    virtual boolean cfgParseComment(const char* comment,
                                const char* filename, int lineno);
    virtual boolean cfgPrintNode(FILE *f, PrintType);

    //
    // Save the .cm file from the 1st - 4th inputs and the min/max with
    // the values found there.
    //
    boolean cmSaveFile(const char *cmapfile);

    //  
    // Most nodes' id parameter is number 1 but a few aren't.  This number is
    // important because whenever you merge 2 nets, you update instance numbers.
    // When you do that, you must also change the id parameter and for that you
    // need its number.
    //
    virtual int getMessageIdParamNumber();

    //
    // In addition to superclass work, sendValues to the exec.  Sending values was
    // held off during node creation and initialization if the node's network !=
    // theDXApplication->network.  Reason:  ColormapNode initializes himself in
    // a noisy way using its instance number which in the case of mergeNetwork
    // operations is the wrong network.  The instance number will be updated
    // by switching networks at which time its safe to go ahead and spout off.
    // See node.h for a comment on 'silently'
    //
    virtual void switchNetwork(Network *from, Network *to, boolean silently=FALSE);

 
    //
    // Determine if this node is a node of the given class
    //
    virtual boolean isA(Symbol classname);

    //
    // Returns a pointer to the class name.
    //
    virtual const char* getClassName()
    {
	return ClassColormapNode;
    }
};


#endif // _ColormapNode_h
