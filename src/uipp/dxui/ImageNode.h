/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#ifndef _ImageNode_h
#define _ImageNode_h


#include "DisplayNode.h"
#include "DXPacketIF.h"
#include "enums.h"


#define NO_RECORD_ENABLE              0
#define RECORD_ENABLE_NO_RERENDER     1
#define RECORD_ENABLE_RERENDER                2

//
// Class name definition:
//
#define ClassImageNode	"ImageNode"

//
// Referenced Classes
class ImageWindow;
class Network;

//
// ImageNode class definition:
//				
class ImageNode : public DisplayNode
{
  private:
    //
    // Private member data:
    //

    //
    //  Print/parse comments that are common to both .nets and .cfgs.
    //
    boolean printCommonComments(FILE *f, const char *indent = NULL);
    boolean parseCommonComments(const char* comment, const char *file,
                                        int lineno);
    //
    // The mode that was saved in the .net/.cfg file (when not data-driven). 
    //
    enum DirectInteractionMode savedInteractionMode;

    //
    // cache value for the nodes internal to the image macro.
    //
    enum Cacheability internalCache;

    //
    // FIXME!: this should really be being done through ImageWindow via
    // ImageNode::reflectStateChange().  ImageNode has no business setting
    // senstivities of another object.  That is ImageWindow's decision.
    //
    void  updateWindowSensitivities();

    Node* getWebOptionsNode();

    static boolean SendString(void* , PacketIFCallback , FILE* , char* , boolean);
    static void FormatMacro (FILE* , PacketIFCallback , void* , String [], boolean);


  protected:
    //
    // Protected member data:
    //
    static String  GifMacroTxt[];
    static String  VrmlMacroTxt[];
    static String  ImageMacroTxt[];
    static String  DXMacroTxt[];

    // The height which isn't stored in the parameter list.
    int height;

    // A boolean which says whether we're translating old values
    // into new ones.
    boolean translating;

    //
    // Fields and members to control the image macro.  When a parameter
    // that affects the image macro changes, macroDirty should be set
    // indicating that when ready, the system should update the macro.
    boolean macroDirty;
    virtual boolean sendMacro(DXPacketIF *pif);
    virtual boolean printMacro(FILE *f,
			       PacketIFCallback callback = NULL,
                               void *callbackData = NULL,
                               boolean viasocket  = FALSE);

    
    // Fields for handling and storing parts of the image messages
    // sent from the executive.
    virtual void handleImageMessage(int id, const char *line);

    virtual void openDefaultWindow(Widget parent);

    //
    // Let the caller of openDefaultWindow() know what kind of window she's getting.
    // This is intended for use in EditorWindow so that we can sanity check the number
    // of cdbs were going to open before kicking off the operation and so that we
    // don't question the user before opening large numbers of interactors.
    // A name describing the type of window can be written into window_name in order
    // to enable nicer warning messages.
    //
    virtual boolean defaultWindowIsCDB(char* window_name = NULL)
	{ if (window_name) strcpy (window_name, "Image Window"); return FALSE; }

    double  boundingBox[4][3];

    // Overrides of Node members to ensure that the correct stuff
    // gets printed and sent to the server.  The image node prevents
    // "RECENABLE", whether to record frames or not, from being saved.
    virtual char   *inputValueString(int i, const char *prefix);
    virtual boolean printIOComment(FILE *f, boolean input, int index, 
					const char *indent = NULL,
					boolean valuesOnly = FALSE);

    virtual void notifyProjectionChange(boolean newPersp);
    virtual void notifyUseVectorChange(boolean newUse);

    virtual char *netEndOfMacroNodeString(const char *prefix);

    virtual boolean netParseComment(const char* comment,
				    const char *file, int lineno);
    virtual boolean netPrintAuxComment(FILE *f);
    virtual boolean netParseAuxComment(const char *comment,
		const char *file,
		int lineno);



    virtual int  handleNodeMsgInfo(const char *line);
    virtual void reflectStateChange(boolean unmanage);

    virtual void ioParameterStatusChanged(boolean input, int index,
			    NodeParameterStatusChange status);

    //
    // Used by setAutoAxes{X,Y,Z}TickLocs().
    //
    void setAutoAxesTickLocs (int param, double *t, int size, boolean send);

  public:
    //
    // Constructor:
    //
    ImageNode(NodeDefinition *nd, Network *net, int instnc);

    //
    // Destructor:
    //
    ~ImageNode();

    virtual boolean initialize();

    virtual void setTitle(const char *title, boolean fromServer = FALSE);
    virtual const char *getTitle();

    virtual Type setInputValue(int index,
		       const char *value,
		       Type t = DXType::UndefinedType,
		       boolean send = TRUE);
    virtual boolean sendValues(boolean ignoreDirty = TRUE);
    virtual boolean	printValues(FILE *f, const char *prefix, PrintType dest);
    virtual boolean associateImage(ImageWindow *w);

    enum Cacheability getInternalCacheability() { return this->internalCache; }
    void setInternalCacheability(enum Cacheability c) { this->internalCache = c; }

    //
    // Access functions
    boolean useVector();
    void    enableVector(boolean use, boolean send = TRUE);
    void    setTo(double *to, boolean send = TRUE);
    void    setFrom(double *from, boolean send = TRUE);
    void    setResolution(int x, int y, boolean send = TRUE);
    void    setWidth(double w, boolean send = TRUE);
    void    setAspect(double a, boolean send = TRUE);
    void    setThrottle(double a, boolean send = TRUE);
    void    setUp(double *up, boolean send = TRUE);
    void    setBox(double box[4][3], boolean send = TRUE);
    void    setProjection(boolean persp, boolean send = TRUE);
    void    setViewAngle(double angle, boolean send = TRUE);
    void    setButtonUp(boolean up, boolean send = TRUE);
    void    setApprox(boolean up, const char *approx, boolean send = TRUE);
    void    setDensity(boolean up, int density, boolean send = TRUE);
    void    setBackgroundColor(const char *color, boolean send = TRUE);

    void    getTo(double *to);
    void    getFrom(double *from);
    void    getResolution(int &x, int &y);
    void    getWidth(double &w);
    void    getAspect(double &a);
    void    getThrottle(double &v);
    void    getUp(double *up);
    void    getBox(double box[4][3]);
    void    getProjection(boolean &persp);
    void    getViewAngle(double &angle);
    boolean isButtonUp();
    void    getApprox(boolean up, const char *&approx);
    void    getDensity(boolean up, int &density);
    void    getBackgroundColor(const char *&color);
    void    getRecordEnable(int &x);
    void    getRecordResolution(int &x, int &y);
    void    getRecordAspect(double &aspect);

    boolean useAutoAxes();

    void    setRecordEnable(int use, boolean send = TRUE);
    void    setRecordResolution(int x, boolean send = TRUE);
    void    setRecordAspect(double aspect, boolean send = TRUE);
    void    setRecordFile(const char *file, boolean send = TRUE);
    void    setRecordFormat(const char *format, boolean send = TRUE);
    void    getRecordFile(const char *&file);
    void    getRecordFormat(const char *&format);

    void enableSoftwareRendering(boolean use, boolean send = TRUE);

    //
    //
    //
    void getAutoAxesCorners (double dval[]);
    void getAutoAxesCursor (double *x,  double *y,   double *z);
    int getAutoAxesStringList (int index, char *sval[]);
    int getAutoAxesLabels (char *sval[]);
    int getAutoAxesAnnotation (char *sval[]);
    int getAutoAxesColors (char *sval[]);
    int getAutoAxesXTickLabels (char *sval[]);
    int getAutoAxesYTickLabels (char *sval[]);
    int getAutoAxesZTickLabels (char *sval[]);
    int getAutoAxesString (int index, char *sval);
    int getAutoAxesFont (char *sval);
    void getAutoAxesTicks (int *t1, int *t2, int *t3);
    void getAutoAxesTicks (int *t);
    void getAutoAxesXTickLocs (double **t, int *size);
    void getAutoAxesYTickLocs (double **t, int *size);
    void getAutoAxesZTickLocs (double **t, int *size);
    int getAutoAxesInteger (int index);
    int getAutoAxesEnable ();
    double getAutoAxesDouble (int index);
    double getAutoAxesLabelScale ();
    int getAutoAxesTicksCount ();

    void setAutoAxesCorners (double dval[], boolean send);
    void setAutoAxesCursor (double x,   double  y,   double  z,    boolean send);
    void setAutoAxesFlag (int index, boolean state, boolean send);
    void setAutoAxesFrame (boolean state, boolean send);
    void setAutoAxesGrid (boolean state, boolean send);
    void setAutoAxesAdjust (boolean state, boolean send);
    void setAutoAxesInteger (int index, int d, boolean send);
    void setAutoAxesEnable (int d, boolean send);
    void setAutoAxesDouble (int index, double d, boolean send);
    void setAutoAxesLabelScale (double d, boolean send);
    void setAutoAxesStringList (int index, char *value, boolean send);
    void setAutoAxesLabels (char *value, boolean send);
    void setAutoAxesAnnotation (char *value, boolean send);
    void setAutoAxesColors (char *value, boolean send);
    void setAutoAxesXTickLocs (double *t, int size, boolean send);
    void setAutoAxesYTickLocs (double *t, int size, boolean send);
    void setAutoAxesZTickLocs (double *t, int size, boolean send);
    void setAutoAxesXTickLabels (char *value, boolean send);
    void setAutoAxesYTickLabels (char *value, boolean send);
    void setAutoAxesZTickLabels (char *value, boolean send);
    void setAutoAxesString (int index, char *value, boolean send);
    void setAutoAxesFont (char *value, boolean send);
    void setAutoAxesTicks (int t1, int t2, int t3, boolean send);
    void setAutoAxesTicks (int t, boolean send);

    void disableAutoAxesCorners (boolean send);
    void unsetAutoAxesCorners (boolean send);
    void unsetAutoAxesCursor (boolean send);
    void unsetAutoAxesTicks (boolean send);
    void unsetAutoAxesXTickLocs (boolean send);
    void unsetAutoAxesYTickLocs (boolean send);
    void unsetAutoAxesZTickLocs (boolean send);
    void unsetAutoAxesStringList (int index, boolean send);
    void unsetAutoAxesAnnotation (boolean send);
    void unsetAutoAxesLabels (boolean send);
    void unsetAutoAxesColors (boolean send);
    void unsetAutoAxesXTickLabels (boolean send);
    void unsetAutoAxesYTickLabels (boolean send);
    void unsetAutoAxesZTickLabels (boolean send);
    void unsetAutoAxesString (int index, boolean send);
    void unsetAutoAxesFont (boolean send);
    void unsetAutoAxesEnable (boolean send);
    void unsetAutoAxesFrame (boolean send);
    void unsetAutoAxesGrid (boolean send);
    void unsetAutoAxesAdjust (boolean send);
    void unsetAutoAxesLabelScale (boolean send);

    void unsetRecordResolution (boolean send);
    void unsetRecordAspect (boolean send);
    void unsetRecordFile (boolean send);
    void unsetRecordFormat (boolean send);
    void unsetRecordEnable (boolean send);

    boolean isAutoAxesCornersSet ();
    boolean isAutoAxesCursorSet ();
    boolean isSetAutoAxesFlag (int index);
    boolean isSetAutoAxesFrame ();
    boolean isSetAutoAxesGrid ();
    boolean isSetAutoAxesAdjust ();
    boolean isAutoAxesStringListSet (int index);
    boolean isAutoAxesAnnotationSet ();
    boolean isAutoAxesLabelsSet ();
    boolean isAutoAxesColorsSet ();
    boolean isAutoAxesStringSet (int index);
    boolean isAutoAxesFontSet ();
    boolean isAutoAxesTicksSet ();
    boolean isAutoAxesXTickLocsSet ();
    boolean isAutoAxesYTickLocsSet ();
    boolean isAutoAxesZTickLocsSet ();
    boolean isAutoAxesXTickLabelsSet ();
    boolean isAutoAxesYTickLabelsSet ();
    boolean isAutoAxesZTickLabelsSet ();
    boolean isAutoAxesDoubleSet (int index);
    boolean isAutoAxesLabelScaleSet ();


    boolean useSoftwareRendering();

    virtual const char *getPickIdentifier();


    //
    // Save and retrieve image window state (camera, approx, render mode...)
    // to and from the .cfg file.
    //
    virtual boolean cfgParseComment(const char* comment,
                                const char* filename, int lineno);
    virtual boolean cfgPrintNode(FILE *f, PrintType);


    virtual void setDefaultCfgState();


    virtual Type setInputSetValue(int index, const char *value,
				Type type = DXType::UndefinedType,
				boolean send = TRUE);
    
    boolean isAutoAxesEnableConnected();
    boolean isAutoAxesCornersConnected();
    boolean isAutoAxesCursorConnected();
    boolean isAutoAxesFrameConnected();
    boolean isAutoAxesGridConnected();
    boolean isAutoAxesAdjustConnected();
    boolean isAutoAxesAnnotationConnected();
    boolean isAutoAxesLabelsConnected();
    boolean isAutoAxesColorsConnected();
    boolean isAutoAxesFontConnected();
    boolean isAutoAxesTicksConnected();
    boolean isAutoAxesXTickLocsConnected();
    boolean isAutoAxesYTickLocsConnected();
    boolean isAutoAxesZTickLocsConnected();
    boolean isAutoAxesXTickLabelsConnected();
    boolean isAutoAxesYTickLabelsConnected();
    boolean isAutoAxesZTickLabelsConnected();
    boolean isAutoAxesLabelScaleConnected();
    boolean isBGColorConnected();
    boolean isThrottleConnected();

    boolean isViewControlInputSet();
    boolean isImageNameInputSet();
    boolean isRecordEnableSet();
    boolean isRecordFileSet();
    boolean isRecordFormatSet();
    boolean isRecordResolutionSet();
    boolean isRecordAspectSet();
    boolean isRecFileInputSet();

    boolean isRecordEnableConnected();
    boolean isRecordFileConnected();
    boolean isRecordFormatConnected();
    boolean isRecordResolutionConnected();
    boolean isRecordAspectConnected();
    boolean isInteractionModeConnected();
    
    boolean isRenderModeSet();
    boolean isButtonUpApproxSet();
    boolean isButtonDownApproxSet();
    boolean isButtonUpDensitySet();
    boolean isButtonDownDensitySet();
    boolean isRenderModeConnected();
    boolean isButtonUpApproxConnected();
    boolean isButtonDownApproxConnected();
    boolean isButtonUpDensityConnected();
    boolean isButtonDownDensityConnected();

    boolean isDataDriven();

    boolean setInteractionMode(const char *mode);
    void    setInteractionModeParameter (DirectInteractionMode mode);
    virtual void    openImageWindow(boolean manage = TRUE);

    //  
    // Most nodes' id parameter is number 1 but a few aren't.  This number is
    // important because whenever you merge 2 nets, you update instance numbers.
    // When you do that, you must also change the id parameter and for that you
    // need its number.
    //
    virtual int getMessageIdParamNumber();

    //
    // On behalf of ImageFormatDialog (Save/Print Image dialogs) which needs to
    // know what strategy to use for saving the current image.
    //
    boolean hardwareMode();
 

    //
    // Determine if this node is a node of the given class
    //
    virtual boolean isA(Symbol classname);

    virtual void enableJava(const char* base_name, boolean send=FALSE);
    virtual void disableJava(boolean send=FALSE);
    virtual boolean isJavified();
    virtual boolean isJavified(Node* webOptions);
    virtual void javifyNode(Node* webOptions, Node*);
    virtual void unjavifyNode();
    virtual const char* getJavaNodeName() { return "ImageNode"; }
    virtual boolean printInputAsJava(int input);
    const char* getWebOptionsFormat();
    boolean isWebOptionsOrbit();
    virtual boolean printAsJava(FILE*);
    boolean isWebOptionsImgIdConnected();

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassImageNode;
    }
};


#endif // _ImageNode_h
