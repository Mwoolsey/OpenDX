/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"



#ifndef _GARMainWindow_h
#define _GARMainWindow_h

#if defined(HAVE_FSTREAM)
#include <fstream>
#elif defined(HAVE_FSTREAM_H)
#include <fstream.h>
#endif

#include <Xm/Xm.h>

#include "../base/lex.h"
#include "../base/DXStrings.h"
#include "../base/Command.h"
#include "../base/List.h"
#include "../base/IBMMainWindow.h"
#include "Field.h"
#include "RecordSeparator.h"

//
// Class name definition:
//
#define ClassGARMainWindow	"GARMainWindow"

//
// XtCallbackProc (*CB), XtEventHandler (*EH) and XtActionProc (*AP)
// DialogCallback (*DCB), XtInputCallbackProc (*ICP), XtWorkProc (*WP)
// functions for this and derived classes
//
extern "C" void GARMainWindow_RecsepTBCB(Widget, XtPointer, XtPointer);
extern "C" void GARMainWindow_SameSepCB(Widget, XtPointer, XtPointer);
extern "C" void GARMainWindow_DiffSepCB(Widget, XtPointer, XtPointer);
extern "C" void GARMainWindow_BlockTBCB(Widget, XtPointer, XtPointer);
extern "C" void GARMainWindow_LayoutTBCB(Widget, XtPointer, XtPointer);
extern "C" void GARMainWindow_MoveFieldCB(Widget, XtPointer, XtPointer);
extern "C" void GARMainWindow_StructureMapCB(Widget, XtPointer, XtPointer);
extern "C" void GARMainWindow_FieldPMCB(Widget, XtPointer, XtPointer);
extern "C" void GARMainWindow_FieldOMCB(Widget, XtPointer, XtPointer);
extern "C" void GARMainWindow_recseplistOMCB(Widget, XtPointer, XtPointer);
extern "C" void GARMainWindow_recsepsingleOMCB(Widget, XtPointer, XtPointer);
extern "C" void GARMainWindow_MajorityCB(Widget, XtPointer, XtPointer);
extern "C" void GARMainWindow_OriginDeltaMVCB(Widget, XtPointer, XtPointer);
extern "C" void GARMainWindow_ScalarMVCB(Widget, XtPointer, XtPointer);
extern "C" void GARMainWindow_IntegerMVCB(Widget, XtPointer, XtPointer);
extern "C" void GARMainWindow_RecSepTextCB(Widget, XtPointer, XtPointer);
extern "C" void GARMainWindow_VerifyNonZeroCB(Widget, XtPointer, XtPointer);
extern "C" void GARMainWindow_FieldDirtyMVCB(Widget, XtPointer, XtPointer);
extern "C" void GARMainWindow_IdentifierMVCB(Widget, XtPointer, XtPointer);
extern "C" void GARMainWindow_HeaderTypeCB(Widget, XtPointer, XtPointer);
extern "C" void GARMainWindow_SeriesSepCB(Widget, XtPointer, XtPointer);
extern "C" void GARMainWindow_GridDimCB(Widget, XtPointer, XtPointer);
extern "C" void GARMainWindow_GridCB(Widget, XtPointer, XtPointer);
extern "C" void GARMainWindow_PostBrowserCB(Widget, XtPointer, XtPointer);
extern "C" void GARMainWindow_FileSelectCB(Widget, XtPointer, XtPointer);
extern "C" void GARMainWindow_StructureCB(Widget, XtPointer, XtPointer);
extern "C" void GARMainWindow_TypeCB(Widget, XtPointer, XtPointer);
extern "C" void GARMainWindow_ListSelectCB(Widget, XtPointer, XtPointer);
extern "C" void GARMainWindow_RecordSepListSelectCB(Widget, XtPointer, XtPointer);
extern "C" void GARMainWindow_FieldTextCB(Widget, XtPointer, XtPointer);
extern "C" void GARMainWindow_ModifyFieldCB(Widget, XtPointer, XtPointer);
extern "C" void GARMainWindow_InsertFieldCB(Widget, XtPointer, XtPointer);
extern "C" void GARMainWindow_DeleteFieldCB(Widget, XtPointer, XtPointer);
extern "C" void GARMainWindow_AddFieldCB(Widget, XtPointer, XtPointer);
extern "C" void GARMainWindow_VerifyPositionCB(Widget, XtPointer, XtPointer);
extern "C" void GARMainWindow_SeriesInterleavingCB(Widget, XtPointer, XtPointer);
extern "C" void GARMainWindow_VectorInterleavingCB(Widget, XtPointer, XtPointer);
extern "C" void GARMainWindow_FieldInterleavingCB(Widget, XtPointer, XtPointer);
extern "C" void GARMainWindow_Format2CB(Widget, XtPointer, XtPointer);
extern "C" void GARMainWindow_SeriesCB(Widget, XtPointer, XtPointer);
extern "C" void GARMainWindow_HeaderCB(Widget, XtPointer, XtPointer);
extern "C" void GARMainWindow_PositionsCB(Widget, XtPointer, XtPointer);
extern "C" void GARMainWindow_RegularityCB(Widget, XtPointer, XtPointer);
extern "C" void GARMainWindow_PostElipPopupEH(Widget, XtPointer, XEvent*, Boolean*);
extern "C" void GARMainWindow_MapEH(Widget, XtPointer, XEvent*, Boolean*);
extern "C" void GARMainWindow_StructureInsensCB(Widget , XtPointer , XtPointer);
extern "C" void GARMainWindow_StructureSensCB(Widget , XtPointer , XtPointer);
extern "C" void GARMainWindow_DirtyFileCB(Widget , XtPointer , XtPointer);

// Simplified prompter defines
#define GMW_GRID		0x00000001
#define GMW_POINTS		0x00000010
#define GMW_POSITIONS		0x00000100
#define GMW_SIMP_POSITIONS	0x00001000
#define GMW_POSITION_LIST	0x00010000
#define GMW_SPREADSHEET		0x00100000
#define GMW_SERIES		0x01000000
#define GMW_FULL		0x10000000

#define FULL			(this->mode & GMW_FULL)
#define GRID_IS_ON		(this->mode & GMW_GRID || FULL)
#define POINTS_IS_ON		(this->mode & GMW_POINTS || FULL)
#define POINTS_SELECTED		(this->mode & GMW_POINTS)
#define POSITIONS_IS_ON		(this->mode & GMW_POSITIONS)
#define POSITION_LIST_ON	(this->mode & GMW_POSITION_LIST)
#define SERIES_IS_ON		(this->mode & GMW_SERIES)
#define SIMP_POSITIONS		(this->mode & GMW_SIMP_POSITIONS)
#define SPREADSHEET		(this->mode & GMW_SPREADSHEET)
#define GRID_ONLY		(GRID_IS_ON && !POINTS_IS_ON)
#define POINTS_ONLY		(POINTS_IS_ON && !GRID_IS_ON)
#define FULL_POSITIONS		(FULL || (POSITIONS_IS_ON && !SIMP_POSITIONS))

//
// Referenced classes:
//
class FilenameSelectDialog;
class OpenFileDialog;
class SADialog;
class GARCommand;
class CommentDialog;
class CommandScope;
class CommandInterface;
class Browser;
class GARNewCommand;
class ConfirmedOpenCommand;

//
// GARMainWindow class definition:
//				
class GARMainWindow : public IBMMainWindow
{
    friend class GARCommand;
    friend class GARNewCommand;
    friend class Browser;
    friend class ConfirmedOpenCommand;
    friend class ConfirmedQCommand;
    friend class ConfirmedIDCommand;

  private:
    //
    // Private class data:
    //
    static boolean ClassInitialized;
    static String  DefaultResources[];

    FilenameSelectDialog 	*fsd;
    OpenFileDialog 		*open_fd;
    SADialog 			*save_as_fd;
    CommentDialog 		*comment_dialog;
    List			fieldList;
    List			recordsepList, recordsepSingle;
    int				dimension;
    int				current_field;
    int				current_rec_sep;
    char			*filename;
    char			*comment_text;
    boolean			vi_sens;
    int				dependency_restriction;
    boolean			layout_was_on;
    boolean			block_was_on;
    boolean			record_sep_was_on;

    Browser			*browser;
    unsigned long		mode;

    Widget			current_pb;

    //
    //  Window components:
    //
    Widget	topform;
    Widget	generalform;
    Widget	fieldform;
    Widget	file_label;
    Widget	grid_tb;
    Widget	points_tb;
    Widget	row_label;
    Widget	col_label;
    Widget	row_tb;
    Widget	col_tb;
    Widget	series_tb;
    Widget	series_n_label;
    Widget	series_start_label;
    Widget	series_delta_label;
    Widget	format_label;
    Widget	majority_label;
    Widget	header_tb;
    Widget	series_sep_tb;
    Widget	regularity_label;
    Widget	field_interleaving_label;
    Widget	vector_interleaving_label;
    Widget	series_interleaving_label;
    Widget	type_label;
    Widget	structure_label;
    Widget	layout_tb;
    Widget	layout_skip_label;
    Widget	layout_width_label;
    Widget	block_tb;
    Widget	block_skip_label;
    Widget	block_element_label;
    Widget	block_width_label;
    Widget	record_sep_tb;
    Widget	same_sep_tb;
    Widget	diff_sep_tb;
    Widget	dependency_label;
    Widget	modify_tb;

    Widget	format1_om;
    Widget	format2_om;
    Widget	field_interleaving_om;
    Widget	sens_vector_interleaving_om;
    Widget	insens_vector_interleaving_om;
    Widget	series_interleaving_om;
    Widget	series_sep_om;
    Widget	header_om;
    Widget	regularity_om;
    Widget	positions_om[4];
    Widget	type_om;
    Widget	structure_om;
    Widget	string_size;
    Widget	string_size_label;
    Widget	record_sep_om;
    Widget	record_sep_om_1;
    Widget	record_sep_list;
    Widget	dependency_om;

    Widget	file_text;
    Widget	points_text;
    Widget	grid_text[4];
    Widget	header_text;
    Widget	series_n_text;
    Widget	series_start_text;
    Widget	series_delta_text;
    Widget	series_sep_text;
    Widget	position_text[4];
    Widget	position_label[4];
    Widget	layout_skip_text;
    Widget	layout_width_text;
    Widget	block_skip_text;
    Widget	block_element_text;
    Widget	block_width_text;
    Widget	record_sep_text;
    Widget	record_sep_text_1;

    Widget	field_list;
    Widget	add_pb;
    Widget	delete_pb;
    Widget	insert_pb;
    Widget	modify_pb;
    Widget	field_text;
    Widget	move_up_ab;
    Widget	move_down_ab;
    Widget	move_label;

    Widget	elip_pb;
    Widget	elip_popup_menu;
    Widget	browser_pb;

    Pixmap	row_insens_pix;
    Pixmap	row_sens_pix;
    Pixmap	col_insens_pix;
    Pixmap	col_sens_pix;

    int		xpos, ypos;	// Position on display
    int		height, width;	// Size on display

    Widget createFormat1Pulldown           (Widget parent);
    Widget createFormat2Pulldown           (Widget parent);
    Widget createHeaderPulldown            (Widget parent, boolean setCB, boolean setCB1, boolean setCB2);
    Widget createSeriesSepPulldown         (Widget parent);
    Widget createFieldInterleavingPulldown (Widget parent);
    Widget createSeriesInterleavingPulldown(Widget parent);
    Widget createSensVectorInterleavingPulldown(Widget parent);
    Widget createInsensVectorInterleavingPulldown(Widget parent);
    Widget createTypePulldown              (Widget parent);
    Widget createStructurePulldown         (Widget parent);
    Widget createDependencyPulldown        (Widget parent);
    Widget createPositionsPulldown         (Widget parent);
    Widget createRegularityPulldown        (Widget parent);

    void setFieldDirty();
    void setFieldClean();
    void createRecordSeparatorList         (Widget parent);
    void createRecordSeparatorListStructure();
    void updateRecordSeparatorList();
    void setDimension(int dim);
    int  getDimension();
    friend void GARMainWindow_MapEH(Widget, XtPointer , XEvent *, Boolean *);
    friend void GARMainWindow_RegularityCB(Widget w, XtPointer clientData, XtPointer);
    friend void GARMainWindow_PositionsCB(Widget w, XtPointer clientData, XtPointer);
    friend void GARMainWindow_HeaderCB(Widget w, XtPointer clientData, XtPointer);
    friend void GARMainWindow_SeriesCB(Widget w, XtPointer clientData, XtPointer);
    friend void GARMainWindow_Format2CB(Widget w, XtPointer clientData, XtPointer);
    friend void GARMainWindow_FieldInterleavingCB(Widget w, XtPointer clientData,XtPointer);
    friend void GARMainWindow_VectorInterleavingCB(Widget w, XtPointer clientData,XtPointer);
    friend void GARMainWindow_SeriesInterleavingCB(Widget w, XtPointer clientData,XtPointer);
    friend void GARMainWindow_VerifyPositionCB(Widget w, XtPointer clientData, XtPointer);
    friend void GARMainWindow_AddFieldCB(Widget w, XtPointer clientData, XtPointer);
    friend void GARMainWindow_DeleteFieldCB(Widget w, XtPointer clientData, XtPointer);
    friend void GARMainWindow_InsertFieldCB(Widget w, XtPointer clientData, XtPointer);
    friend void GARMainWindow_ModifyFieldCB(Widget w, XtPointer clientData, XtPointer);
    friend void GARMainWindow_FieldTextCB(Widget w, XtPointer clientData, XtPointer);
    friend void GARMainWindow_ListSelectCB(Widget w, XtPointer clientData, XtPointer);
    friend void GARMainWindow_RecordSepListSelectCB(Widget w, XtPointer clientData, XtPointer);
    friend void GARMainWindow_TypeCB(Widget w, XtPointer clientData, XtPointer);
    friend void GARMainWindow_StructureCB(Widget w, XtPointer clientData, XtPointer);
    friend void GARMainWindow_FileSelectCB(Widget w, XtPointer clientData, XtPointer);
    friend void GARMainWindow_PostBrowserCB(Widget w, XtPointer clientData, XtPointer);
    static void PostSaveAsDialog(GARMainWindow *, Command *cmd = NULL);
    friend void GARMainWindow_GridCB(Widget w, XtPointer clientData, XtPointer);
    friend void GARMainWindow_GridDimCB(Widget w, XtPointer clientData, XtPointer);
    friend void GARMainWindow_SeriesSepCB(Widget w, XtPointer clientData, XtPointer);

    friend void GARMainWindow_HeaderTypeCB(Widget w, XtPointer clientData, XtPointer);
    friend void GARMainWindow_IdentifierMVCB(Widget, XtPointer, XtPointer);
    friend void GARMainWindow_FieldDirtyMVCB(Widget, XtPointer, XtPointer);
    friend void GARMainWindow_IntegerMVCB(Widget w, XtPointer clientData, XtPointer);
    friend void GARMainWindow_RecSepTextCB(Widget w, XtPointer clientData, XtPointer);
    friend void GARMainWindow_VerifyNonZeroCB(Widget w, XtPointer clientData, XtPointer);
    friend void GARMainWindow_ScalarMVCB(Widget w, XtPointer clientData, XtPointer);
    friend void GARMainWindow_OriginDeltaMVCB(Widget w, XtPointer , XtPointer call);

    friend void GARMainWindow_MajorityCB(Widget w, XtPointer clientData, XtPointer);

    friend void GARMainWindow_FieldOMCB(Widget w, XtPointer clientData,XtPointer);
    friend void GARMainWindow_recseplistOMCB(Widget w, XtPointer clientData,XtPointer);
    friend void GARMainWindow_recsepsingleOMCB(Widget w, XtPointer clientData,XtPointer);

    friend void GARMainWindow_FieldPMCB(Widget w, XtPointer clientData,XtPointer);

    friend void GARMainWindow_StructureMapCB(Widget w, XtPointer clientData,XtPointer);

    friend void GARMainWindow_PostElipPopupEH(Widget w, XtPointer client, 
						XEvent *e, Boolean *ctd);
    static void adjustTextCursor(XtPointer ,XtIntervalId *);

    friend void GARMainWindow_MoveFieldCB(Widget w, XtPointer clientData,XtPointer);

    friend void GARMainWindow_LayoutTBCB(Widget w, XtPointer clientData,XtPointer);

    friend void GARMainWindow_BlockTBCB(Widget w, XtPointer clientData,XtPointer);

    friend void GARMainWindow_RecsepTBCB(Widget w, XtPointer clientData,XtPointer);
    friend void GARMainWindow_SameSepCB(Widget w, XtPointer clientData,XtPointer);
    friend void GARMainWindow_DiffSepCB(Widget w, XtPointer clientData,XtPointer);

    friend void GARMainWindow_StructureInsensCB(Widget , XtPointer , XtPointer);
    friend void GARMainWindow_StructureSensCB(Widget , XtPointer , XtPointer);

    void CreateElipsesPopup();
    void changeHeaderType();
    void restrictStructure(Boolean rstrict);
    Boolean addField(int pos);
    void fieldToWids(Field *);
    void recsepToWids(RecordSeparator *);
    void widsToField(Field *);
    void createDefaultFields(int num_fields);
    int numFields(char *line);
    char *nextItem(char *line, int &ndx);
    void setMoveFieldSensitivity();
    void setRecordOptionsSensitivity();
    void setRecordSeparatorSensitivity();
    void setDependencyRestrictions();
    void synchDependencies();


  protected:

    void clearText(Widget w);
    void setTextSensitivity(Widget w, Boolean sens);
    void changeRegularity(Widget w);
    void changePositions(Widget w);
    void setBlockLayoutSensitivity();

    void setHeaderSensitivity			(Boolean set);
    void setGridSensitivity			(Boolean set);
    void setPointsSensitivity			(Boolean set);
    void setMajoritySensitivity			(Boolean set);
    void setSeriesSensitivity			(Boolean set);
    void setSeriesInterleavingSensitivity	();
    void setSeriesSepSensitivity		();
    void setLayoutTBSensitivity			();
    void setLayoutSensitivity			();
    void setBlockTBSensitivity			();
    void setBlockSensitivity			();
    void setRecordSepTBSensitivity		();
    void setRecordSepSensitivity		();
    void setDependencySensitivity		(Boolean set);
    void setPositionsSensitivity		();

    void setVectorInterleavingSensitivity();
    Boolean getVectorInterleavingSensitivity();
    void setVectorInterleavingOM(char *name);

    void parseFile(char *line);
    void parseGrid(char *line);
    void parsePoints(char *line);
    void parseField(char *line);
    void parseFormat(char *line);
    void parseStructure(char *line);
    void parseType(char *line);
    void parseInterleaving(char *line);
    void parseHeader(char *line);
    void parseSeries(char *line);
    void parseMajority(char *line);
    void parseLayout(char *line);
    void parseBlock(char *line);
    void parsePositions(std::istream *from, char *line);
    void parseComment(char *line);
    void parseDependency(char *line);
    void parseRecordSep(char *line);

    //
    // Menus & pulldowns:
    //
    Widget		fileMenu;
    Widget		editMenu;
    Widget		optionMenu;

    Widget		fileMenuPulldown;
    Widget		editMenuPulldown;
    Widget		optionMenuPulldown;


    //
    // File menu options:
    //
    CommandInterface*	newOption;
    CommandInterface*	openOption;
    CommandInterface*	saveOption;
    CommandInterface*	saveAsOption;
    CommandInterface*	closeGAROption;
    CommandInterface*	commentOption;
    CommandInterface*	fullOption;
    CommandInterface*	simplifyOption;
    CommandInterface*	initialDialogOption;

    Command		*newCmd;
    ConfirmedOpenCommand *openCmd;
    Command		*saveCmd;
    Command		*saveAsCmd;
    Command 	        *closeGARCmd;
    Command		*commentCmd;
    Command		*fullCmd;
    Command		*simplifyCmd;
    Command		*initialDialogCmd;
    

    //
    // Help menu options 
    //
    CommandInterface*	helpGAFormatOption;
    CommandInterface*   helpTutorialOption;	

    //
    // Install the default resources for this class and then call the
    // same super class method to get the default resources from the
    // super classes.
    //
    virtual void installDefaultResources(Widget baseWidget);

  public:
    //
    // Flags:
    //
    enum {
           NEW,
           SAVE,
           SAVE_AS,
           CLOSE,
           COMMENT,
           FULLGAR,
           CLOSE_GAR,
           SIMPLIFY
    };

    void newGAR();
    void newGAR(unsigned long mode);
    boolean saveGAR(char *filenm);
    void saveGAR(std::ostream *);

    boolean openGAR(char *filenm);
    boolean openGAR(std::istream *);

    unsigned long getMode(){return this->mode;};
    static unsigned long getMode(char *filenm);
    static unsigned long getMode(std::istream *);

    void postOpenFileDialog();
    void postBrowser(char *filenm);
    void unpostBrowser();

    Boolean validateGAR(Boolean complete = True);

    void setFilename(char *filenm);
    void setDataFilename(const char *filenm);
    char *getFilename();

    Boolean addField(char *name, char *structure);

    //
    // Creates the editor window workarea (as required by MainWindow class).
    //
    Widget createWorkArea(Widget parent);

    //
    // Creates the editor window menus (as required by MainWindow class).
    //
    void createMenus(Widget parent);

    void createFileMenu(Widget parent);
    void createEditMenu(Widget parent);
    void createOptionMenu(Widget parent);
    void createHelpMenu(Widget parent);


    GARMainWindow(unsigned long mode);

    //
    // Destructor:
    //
    ~GARMainWindow();

    //
    // Call the superclass initialize and then realize the
    // widget so placement and size can be done.
    //
    void manage();

    Widget getFileTextWid(){ return this->file_text; };
    Boolean originDeltaFormat(char *string);
    Boolean positionListFormat(char *string, int expected_count);

    Boolean IsSingleInteger(char *string);
    Boolean IsSingleScalar(char *string);

    const char* getCommentText()
    {
	return (const char*)this->comment_text;
    }
    void setCommentText(char *s);

    //
    // called by MainWindow::CloseCallback().
    //
    void closeWindow();

    //
    // support ezstart
    //
    Command* getFullCmd() { return this->fullCmd; }
    Command* getSimplifyCmd() { return this->simplifyCmd; }
    List *getFieldList() { return &this->fieldList; }
    boolean isSeries();
    boolean isGridded();
    int getDimensionality();
 
    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassGARMainWindow;
    }
};


#endif // _GARMainWindow_h
