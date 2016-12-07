/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


#ifndef _PostScriptImageFormat_h
#define _PostScriptImageFormat_h


#include "ImageFormat.h"

#include "List.h"
class DirtyQueue;

//
// Class name definition:
//
#define ClassPostScriptImageFormat	"PostScriptImageFormat"

extern "C" void PostScriptImageFormat_SizeTO (XtPointer, XtIntervalId*);
extern "C" void PostScriptImageFormat_PixelSizeTO (XtPointer, XtIntervalId*);
extern "C" void PostScriptImageFormat_ModifyCB (Widget, XtPointer, XtPointer);
extern "C" void PostScriptImageFormat_ParseSizeCB (Widget, XtPointer, XtPointer);
extern "C" void PostScriptImageFormat_ParseSizeEH (Widget, XtPointer, XEvent*, Boolean*);
extern "C" void PostScriptImageFormat_PageSizeTO (XtPointer, XtIntervalId*);
extern "C" void PostScriptImageFormat_PageModifyCB (Widget, XtPointer, XtPointer);
extern "C" void PostScriptImageFormat_ParsePageSizeCB (Widget, XtPointer, XtPointer);
extern "C" void PostScriptImageFormat_DpiCB (Widget, XtPointer, XtPointer);
extern "C" void PostScriptImageFormat_OrientCB (Widget, XtPointer, XtPointer);
extern "C" void PostScriptImageFormat_MarginCB (Widget, XtPointer, XtPointer);


class DirtyQueue {
    private:
	List*  data;
	int    max_size;

    protected:
	friend class PostScriptImageFormat;
	friend void PostScriptImageFormat_DpiCB (Widget, XtPointer, XtPointer);
	friend void PostScriptImageFormat_ParseSizeCB (Widget, XtPointer, XtPointer);
	void push(int dirty, int* all_flags) {
	    if (dirty <= 0) return ;
	    if (this->data == NUL(List*)) this->data = new List;

	    int size = this->data->getSize();
	    int top = 0;
	    if (size > 0) top = (int)(long)this->data->getElement(1);
	    if (dirty == top) return;

	    ASSERT(this->data->insertElement((void*)dirty, 1));
	    size++;
	    if (size > this->max_size) {
		int bottom = (int)(long)this->data->getElement(size);
		*all_flags&= ~bottom;
	    }
	}
	void clear() { if (this->data) this->data->clear(); }

	DirtyQueue() {
	    this->data = NUL(List*);
	    this->max_size = 2;
	}
	~DirtyQueue() {
	    if (this->data) delete this->data;
	}
};

//
// SaveImageDialog class definition:
//				
class PostScriptImageFormat : public ImageFormat
{
  private:

    static boolean 	ClassInitialized;
    static boolean	FmtConnectionWarning;
    int			dirty;
    char*		pixel_size_val;
    int			dpi;
    double		page_width;
    double		page_height;
    double		entered_printout_width;
    double		entered_printout_height;
    boolean		entered_dims_valid;
    Widget		size_text;
    Widget		pixel_size_text;
    Widget		page_size_text;
    Widget		dpi_number;
    Widget		margin_width;
    Widget		autoorient_button;
    Widget		portrait_button;
    Widget		landscape_button;
    Widget		chosen_layout;
    Widget		page_layout_om;
    Widget		dirty_text_widget;
    XtIntervalId 	size_timer;
    XtIntervalId 	page_size_timer;

    void		setTextString (Widget, char *, void*);
    void		setSizeTextString (Widget, char *);
    boolean		setVerifiedSizeTextString (double, double);

    //
    // a fifo queue of dirty bits.  The user has 3 fields in which to enter data.
    // With each entry push the dirty bit down.  If she enters all 3, then
    // mark the least recently modified bit as unset.  The value stored in
    // here must be one of {DirtyImageSize, DirtyDpi, DirtyResolution|DirtyAspect}
    // Storing DirtyResolution|DirtyAspect just means the user typed into 
    // pixel_size_text.
    //
    DirtyQueue		fifo_bits;

  protected:

    static String  DefaultResources[];

    void			parseImageSize(const char* );
    void			parsePictureSize(const char* );
    void			parsePageSize(const char*);

    virtual Widget 		createBody(Widget parent);
    virtual void 		setCommandActivation();
    virtual void 		shareSettings(ImageFormat*);

    //
    // Constructor:
    //
    PostScriptImageFormat(const char *name, ImageFormatDialog* dialog);

    enum {
	DirtyResolution		= 1<<5,
	DirtyAspect		= 1<<6,
	DirtyDpi		= 1<<7,
	DirtyOrient		= 1<<8,
	DirtyPageSize		= 1<<9,
	DirtyImageSize		= 1<<10
    };

    friend void PostScriptImageFormat_SizeTO (XtPointer, XtIntervalId*);
    friend void PostScriptImageFormat_PixelSizeTO (XtPointer, XtIntervalId*);
    friend void PostScriptImageFormat_ModifyCB (Widget, XtPointer, XtPointer);
    friend void PostScriptImageFormat_ParseSizeCB (Widget, XtPointer, XtPointer);
    friend void PostScriptImageFormat_ParseSizeEH (Widget, XtPointer, XEvent*, Boolean*);
    friend void PostScriptImageFormat_PageSizeTO (XtPointer, XtIntervalId*);
    friend void PostScriptImageFormat_PageModifyCB (Widget, XtPointer, XtPointer);
    friend void PostScriptImageFormat_ParsePageSizeCB (Widget, XtPointer, XtPointer);
    friend void PostScriptImageFormat_DpiCB (Widget, XtPointer, XtPointer);
    friend void PostScriptImageFormat_OrientCB (Widget, XtPointer, XtPointer);
    friend void PostScriptImageFormat_MarginCB (Widget, XtPointer, XtPointer);

  public:
    //
    // Destructor:
    //
    ~PostScriptImageFormat();

    virtual void		parseRecordFormat(const char *);
    virtual int			getRecordResolution() { return this->width; }
    virtual double		getRecordAspect() { return this->aspect; }
    virtual boolean		supportsPrinting() { return TRUE; }
    virtual void		changeUnits();
    virtual boolean		useLocalResolution();
    virtual boolean		useLocalAspect();
    virtual boolean		useLocalFormat();
    virtual const char*		getRecordFormat();
    virtual int			getRequiredHeight() { return 105; }
    virtual void		applyValues();
    virtual void		restore();

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName() { return ClassPostScriptImageFormat; }
    virtual boolean isA(Symbol classname);
};


//
// If you assume that all sheets of paper are tall and thin, then you can
// always compare the image's aspect ratio to 1.0 to determine if it's better
// to print out the image in landscape vs portrait.  However, you cannot tell if
// the height of the sheet of paper will limit the size of the image or if the
// width of the sheet of paper will limit it.  So you must calculate both dpi values
// and use the smaller of the 2.  As long as you're going to so much trouble, just
// handle arbitrary paper sizes.  The misfit field gets larger as the difference
// between image/paper aspect ratios increases.
//
class Orientation {
    private:
	friend class PostScriptImageFormat;

	int dpi;

    protected:
	boolean valid;
	int misfit;

	//
	// By default we'll increment dpi if there is any fractional part to it.  That's
	// necessary because rounding or truncating it would increase the size of the
	// printout and we might be trying to fit it onto a page.
	//
	Orientation (int w, int h, double page_w, double page_h, 
		    double margin, boolean rnd_up=TRUE) 
	{
	    valid = TRUE;

	    int vertical_dpi, horizontal_dpi;
	    double round_threshold = (rnd_up ? 0.0 : 0.5);
	    double divisorw = page_w - margin;
	    double divisorh = page_h - margin;

	    if ((divisorw > 0) && (divisorh > 0)) {
		//
		// Max possible dpi horizontally.
		//
		double diff = (double)w/ (page_w - margin);
		horizontal_dpi = (int)diff;
		if ((diff - horizontal_dpi) >= round_threshold) horizontal_dpi++;
		//
		// Max possible dpi vertically.
		//
		diff = (double)h/ (page_h - margin);
		vertical_dpi = (int)diff;
		if ((diff - vertical_dpi) >= round_threshold) vertical_dpi++;
		//
		// Dpi available is really the greater of the two.
		// (iow, the image size is limited to the greater extent.)
		//
		if (horizontal_dpi >= vertical_dpi) {
		    misfit = horizontal_dpi - vertical_dpi;
		    dpi = horizontal_dpi;
		} else {
		    misfit = vertical_dpi - horizontal_dpi;
		    dpi = vertical_dpi;
		}
	    } else {
		valid = FALSE;
		misfit = 9999999;
	    }
	}

	~Orientation(){};
};

class RestrictedOrientation: public Orientation {
    private:
	friend class PostScriptImageFormat;


    protected:
	RestrictedOrientation (int w, int h, double page_w, double page_h,
		double margin, double image_w, double image_h, boolean rnd_up=TRUE):
	    Orientation (w,h,image_w, image_h, 0.0, rnd_up)
	{
	    if ((page_w-margin) < image_w) valid = FALSE;
	    if ((page_h-margin) < image_h) valid = FALSE;
	    if (!valid) misfit = 9999999;
	}

    ~RestrictedOrientation(){};
};

#endif // _PostScriptImageFormat_h
