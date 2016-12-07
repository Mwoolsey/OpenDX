/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"



// 
//
//

#ifndef _VPEAnnotator_h
#define _VPEAnnotator_h

#include <X11/Intrinsic.h>

#include "LabelDecorator.h"
#if WORKSPACE_PAGES
#include "GroupedObject.h"
#endif

class Dictionary;

#define ClassVPEAnnotator	"VPEAnnotator"

#if WORKSPACE_PAGES
class VPEAnnotator : public LabelDecorator, public GroupedObject
#else
class VPEAnnotator : public LabelDecorator
#endif
{

  // P R I V A T E   P R I V A T E   P R I V A T E
  // P R I V A T E   P R I V A T E   P R I V A T E
  private:
    static boolean VPEAnnotatorClassInitialized;

    // D R A G - N - D R O P
    // D R A G - N - D R O P
    static Dictionary *DragTypeDictionary;

  // P R O T E C T E D   P R O T E C T E D   P R O T E C T E D   
  // P R O T E C T E D   P R O T E C T E D   P R O T E C T E D   
  protected:
    static String 	   DefaultResources[]; 
    static Widget          DragIcon;
    static boolean         DragDictionaryInitialized;
    static  void           PixelToRGB(Widget, Pixel, float *, float *, float *);

    virtual void           completeDecorativePart();

    const char*		   getPostScriptFont();

    //
    // There are 2 constructors - 1 public, 1 protected - because this class
    // can be instantiated or subclassed.
    //
    VPEAnnotator(boolean developerStyle, const char *name);

    virtual boolean resizeOnUpdate() { return TRUE; }
    virtual boolean requiresLineReroutingOnResize() { return TRUE; }

    Base* layout_information;

  // P U B L I C   P U B L I C   P U B L I C
  // P U B L I C   P U B L I C   P U B L I C
  public:
    static    Decorator*     AllocateDecorator (boolean devStyle);
    virtual   void	     openDefaultWindow();
    virtual   boolean 	     printPostScriptPage(FILE *f);

#if WORKSPACE_PAGES
    virtual   Network*	     getNetwork() { return this->LabelDecorator::getNetwork(); }

    // C O N T R O L   P A N E L   C O M M E N T   F U N C T I O N S
    // C O N T R O L   P A N E L   C O M M E N T   F U N C T I O N S
    virtual   boolean	     printComment (FILE *f);
    virtual   boolean	     parseComment (const char *comment,
				const char *filename, int line);
#endif

    //
    // FIXME
    // When you use the SetAnnotatorTextDialog to make new text, a large change
    // in size of the VPEAnnotator makes hash out of the line routing.  
    // At this level it's really not nice to use knowledge of lines.
    // Rather than change line routing code, I'll just reroute the lines 
    // after getting new text. 
    // 
    virtual void    postTextGrowthWork();

    virtual Dictionary* getDragDictionary() { return VPEAnnotator::DragTypeDictionary; }

    //
    // On behalf of automatic graph layout...
    //
    void setLayoutInformation (Base* info);
    Base* getLayoutInformation () { return this->layout_information; }

    // T H E   T H I N G S   W E   U S E   A L L   T H E   T I M E
    // T H E   T H I N G S   W E   U S E   A L L   T H E   T I M E
    // T H E   T H I N G S   W E   U S E   A L L   T H E   T I M E
    virtual   void initialize();
    	  	   VPEAnnotator(boolean developerStyle =TRUE);
    	  	  ~VPEAnnotator(); 
    const    char* getClassName() { return ClassVPEAnnotator; }
    virtual  boolean isA(Symbol classname);
};


#endif // _VPEAnnotator_h
