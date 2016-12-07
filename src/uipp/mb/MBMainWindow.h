/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"



#ifndef _MBMainWindow_h
#define _MBMainWindow_h

#include <Xm/Xm.h>
#include "lex.h"
#include "DXStrings.h"
#include "Command.h"
#include "List.h"
#include "IBMMainWindow.h"
#include "MBParameter.h"
#include "OpenFileDialog.h"
#include "SADialog.h"
#include "MBCommand.h"
#include "MBNewCommand.h"
#include "ConfirmedQCommand.h"
#include "CommentDialog.h"

//
// Class name definition:
//
#define ClassMBMainWindow	"MBMainWindow"

//
// XtCallbackProc (*CB), XtEventHandler (*EH) and XtActionProc (*AP)
// DialogCallback (*DCB), XtInputCallbackProc (*ICP), XtWorkProc (*WP)
// functions for this and derived classes
//
extern "C" void MBMainWindow_paramDefaultValueCB(Widget, XtPointer, XtPointer);
extern "C" void MBMainWindow_paramDescriptionCB(Widget, XtPointer, XtPointer);
extern "C" void MBMainWindow_paramNameCB(Widget, XtPointer, XtPointer);
extern "C" void MBMainWindow_typeCB(Widget, XtPointer, XtPointer);
extern "C" void MBMainWindow_dependencyCB(Widget, XtPointer, XtPointer);
extern "C" void MBMainWindow_elementTypeCB(Widget, XtPointer, XtPointer);
extern "C" void MBMainWindow_connectionsCB(Widget, XtPointer, XtPointer);
extern "C" void MBMainWindow_positionsCB(Widget, XtPointer, XtPointer);
extern "C" void MBMainWindow_dataShapeCB(Widget, XtPointer, XtPointer);
extern "C" void MBMainWindow_simpledataShapeCB(Widget, XtPointer, XtPointer);
extern "C" void MBMainWindow_dataTypeCB(Widget, XtPointer, XtPointer);
extern "C" void MBMainWindow_descriptiveCB(Widget, XtPointer, XtPointer);
extern "C" void MBMainWindow_descriptionCB(Widget, XtPointer, XtPointer);
extern "C" void MBMainWindow_requiredCB(Widget, XtPointer, XtPointer);
extern "C" void MBMainWindow_outboardCB(Widget, XtPointer, XtPointer);
extern "C" void MBMainWindow_runtimeCB(Widget, XtPointer, XtPointer);
extern "C" void MBMainWindow_asynchCB(Widget, XtPointer, XtPointer);
extern "C" void MBMainWindow_includefileCB(Widget, XtPointer, XtPointer);
extern "C" void MBMainWindow_IOCB(Widget, XtPointer, XtPointer);
extern "C" void MBMainWindow_paramNumberCB(Widget, XtPointer, XtPointer);
extern "C" void MBMainWindow_numOutputsCB(Widget, XtPointer, XtPointer);
extern "C" void MBMainWindow_numInputsCB(Widget, XtPointer, XtPointer);
extern "C" void MBMainWindow_existingCB(Widget, XtPointer, XtPointer);
extern "C" void MBMainWindow_structureCB(Widget, XtPointer, XtPointer);
extern "C" void MBMainWindow_setDirtyCB(Widget, XtPointer, XtPointer);
extern "C" void MBMainWindow_isIdentifierCB(Widget, XtPointer, XtPointer);
extern "C" void MBMainWindow_isFileNameCB(Widget, XtPointer, XtPointer);
extern "C" void MBMainWindow_InputTypeCB(Widget , XtPointer , XtPointer);
extern "C" void MBMainWindow_PostExistingPopupEH(Widget, XtPointer, XEvent*, Boolean*);
extern "C" void MBMainWindow_fieldCB(Widget, XtPointer clientData, XtPointer);
extern "C" void MBMainWindow_simpleparamCB(Widget, XtPointer clientData, XtPointer);

#define FIELD_TYPE 			0x00000001
#define SCALAR_TYPE 			0x00000002
#define VECTOR_TYPE 	 		0x00000004
#define FLAG_TYPE 			0x00000008
#define INTEGER_TYPE 			0x00000010
#define SCALAR_LIST_TYPE	 	0x00000020
#define VECTOR_LIST_TYPE 		0x00000040
#define INTEGER_LIST_TYPE 		0x00000080
#define INTEGER_VECTOR_TYPE 		0x00000100
#define INTEGER_VECTOR_LIST_TYPE 	0x00000200
#define STRING_TYPE		 	0x00000400
#define OBJECT_TYPE		 	0x00000800
#define GROUP_TYPE		 	0x00001000
#define num_simple_types 10 
#define total_num_types  11 

#define DO_MDF		0x01
#define DO_C		0x02
#define DO_MAKE		0x04
#define EXECUTABLE	0x08
#define RUN		0x10

//
// Referenced classes:
//
class OpenFileDialog;
class SADialog;
class CommandScope;
class CommandInterface;
class TextEditDialog;
class OptionsDialog;

//
// MBMainWindow class definition:
//				
class MBMainWindow : public IBMMainWindow
{
    friend class MBCommand;
    friend class MBNewCommand;
    friend class ConfirmedQCommand;

  private:
    //
    // Private class data:
    //
    static boolean ClassInitialized;
    static String  DefaultResources[];

    static void reallyBuild(void *);

    OpenFileDialog 		*open_fd;
    SADialog 			*save_as_fd;
    TextEditDialog		*comment_dialog;
    OptionsDialog		*options_dialog;
    List			inputParamList;
    List			outputParamList;
    int				dimension;
    int				current_field;
    char			*filename;
    char			*comment_text;
    boolean			is_input;
    boolean			ignore_cb;
    struct
    {
	char     *name;
	unsigned long type;
	Widget   wid;
    } param_type[total_num_types];

    //
    //  Window components:
    //
    Widget	topform;
    Widget	mdfform;
    Widget	internalform;
    Widget	module_name_label;
    Widget	module_name_text;
    Widget	category_label;
    Widget	category_text;
    Widget	existing_category;
    Widget	exisiting_popup_menu;
    Widget	mod_description_label;
    Widget	mod_description_text;
    Widget	num_inputs_label;
    Widget	num_inputs_stepper;
    Widget	num_outputs_label;
    Widget	num_outputs_stepper;
    Widget	param_num_label;
    Widget	param_num_stepper;
    Widget	name_label;
    Widget	name_text;
    Widget	type_label;
    Widget	type_rc;
    Widget	required_tb;
    Widget	default_value_label;
    Widget	default_value_text;
    Widget	descriptive_tb;
    Widget	object_type_label;
    Widget	param_description_label;
    Widget	param_description_text;
    Widget	outboard_text;
    Widget	outboard_label;
    Widget	moduletype_label;
    Widget	outboard_tb;
    Widget	inboard_tb;
    Widget	runtime_tb;
    Widget	includefile_text;
    Widget	includefile_label;
    Widget	includefile_tb;
    Widget	asynch_tb;
    Widget	persistent_tb;
    Widget	host_label;
    Widget	host_text;

    Widget      module_label;
    Widget      inout_label;
    Widget      inorout_label;
    Widget	structure_label;
    Widget	structure_om;
    Widget	data_type_label;
    Widget	data_type_om;
    Widget	data_shape_label;
    Widget	data_shape_om;
    Widget	simple_data_type_label;
    Widget	simple_data_type_om;
    Widget	simple_data_shape_label;
    Widget	simple_data_shape_om;
    Widget	positions_label;
    Widget	positions_om;
    Widget	connections_label;
    Widget	connections_om;
    Widget	element_type_label;
    Widget	element_type_om;
    Widget	dependency_label;
    Widget	dependency_om;
    Widget	io_om;
    Widget	input_type_rb; 
    Widget	module_type_rb; 
    Widget	fieldForm;
    Widget	simpleParameterForm;
    Widget	field_tb;
    Widget	simple_param_tb;
    Widget	field_label;
    Widget	simple_param_label;

    Widget createIOPulldown           (Widget parent);
    Widget createDataTypePulldown     (Widget parent);
    Widget createDataShapePulldown    (Widget parent);
    Widget createSimpleDataShapePulldown    (Widget parent);
    Widget createPositionsPulldown    (Widget parent);
    Widget createConnectionsPulldown  (Widget parent);
    Widget createElementTypePulldown  (Widget parent);
    Widget createDependencyPulldown   (Widget parent);

    void CreateExistingCategoryPopup();
    Widget CreateTypeList();

    friend void MBMainWindow_PostExistingPopupEH(Widget w, XtPointer client, 
					XEvent *e, Boolean *ctd);
    friend void MBMainWindow_setDirtyCB(Widget w, XtPointer clientData, XtPointer);
    friend void MBMainWindow_isIdentifierCB(Widget w, XtPointer clientData, XtPointer);
    friend void MBMainWindow_isFileNameCB(Widget w, XtPointer clientData, XtPointer);
    friend void MBMainWindow_structureCB(Widget w, XtPointer clientData, XtPointer);
    friend void MBMainWindow_existingCB(Widget w, XtPointer clientData, XtPointer);
    friend void MBMainWindow_numInputsCB(Widget w, XtPointer clientData, XtPointer);
    friend void MBMainWindow_numOutputsCB(Widget w, XtPointer clientData, XtPointer);
    friend void MBMainWindow_paramNumberCB(Widget w, XtPointer clientData, XtPointer);
    friend void MBMainWindow_IOCB(Widget w, XtPointer clientData, XtPointer);
    friend void MBMainWindow_outboardCB(Widget w, XtPointer clientData, XtPointer);
    friend void MBMainWindow_runtimeCB(Widget w, XtPointer clientData, XtPointer);
    friend void MBMainWindow_asynchCB(Widget w, XtPointer clientData, XtPointer);
    friend void MBMainWindow_includefileCB(Widget w, XtPointer clientData, XtPointer);
    friend void MBMainWindow_requiredCB(Widget w, XtPointer clientData, XtPointer);
    friend void MBMainWindow_descriptionCB(Widget w, XtPointer clientData, XtPointer);
    friend void MBMainWindow_descriptiveCB(Widget w, XtPointer clientData, XtPointer);
    friend void MBMainWindow_dataTypeCB(Widget w, XtPointer clientData, XtPointer);
    friend void MBMainWindow_dataShapeCB(Widget w, XtPointer clientData, XtPointer);
    friend void MBMainWindow_simpledataShapeCB(Widget w, XtPointer clientData, XtPointer);
    friend void MBMainWindow_positionsCB(Widget w, XtPointer clientData,XtPointer);
    friend void MBMainWindow_connectionsCB(Widget w, XtPointer clientData,XtPointer);
    friend void MBMainWindow_elementTypeCB(Widget w, XtPointer clientData, XtPointer);
    friend void MBMainWindow_dependencyCB(Widget w, XtPointer clientData, XtPointer);
    friend void MBMainWindow_typeCB(Widget w, XtPointer clientData, XtPointer);
    friend void MBMainWindow_paramNameCB(Widget w, XtPointer clientData, XtPointer);
    friend void MBMainWindow_paramDescriptionCB(Widget w, XtPointer clientData, XtPointer);
    friend void MBMainWindow_paramDefaultValueCB(Widget w, XtPointer clientData, XtPointer);
    friend void MBMainWindow_fieldCB(Widget w, XtPointer clientData, XtPointer);
    friend void MBMainWindow_simpleparamCB(Widget w, XtPointer clientData, XtPointer);
    friend void createFieldOptions();
    friend void manageFieldOptions();
    friend void unmanageFieldOptions();
    friend void createSimpleParameterOptions();
    friend void manageSimpleParameterOptions();
    friend void umanageSimpleParameterOptions();

    static void PostSaveAsDialog(MBMainWindow *, Command *cmd = NULL);

    MBParameter* getCurrentParam();
    void paramToWids(MBParameter *param);
    void setParamNumStepper();
    void setRequiredSens();
    void structureChanged();
    void setElementTypeSens();
    void setPositionsConnectionsSens();

    void createFieldOptions();
    void manageFieldOptions();
    void unmanageFieldOptions();
    void createSimpleParameterOptions();
    void manageSimpleParameterOptions();
    void unmanageSimpleParameterOptions();

    void parseModuleName(char *line);
    void parseCategory(char *line);
    void parseModuleDescription(char *line);
    void parseOutboardExecutable(char *line);
    void parseLoadableExecutable(char *line);
    void parseIncludeFileName(char *line);
    void parseParamName(MBParameter *param, char *line);
    void parseParamDescription(MBParameter *param, char *line);
    void parseParamRequired(MBParameter *param, char *line);
    void parseParamDefaultValue(MBParameter *param, char *line);
    void parseParamDescriptive(MBParameter *param, char *line);
    void parseParamTypes(MBParameter *param, char *line);
    void parseParamStructure(MBParameter *param, char *line);
    void parseParamDataType(MBParameter *param, char *line);
    void parseParamDataShape(MBParameter *param, char *line);
    void parseParamPositions(MBParameter *param, char *line);
    void parseParamConnections(MBParameter *param, char *line);
    void parseParamElementType(MBParameter *param, char *line);
    void parseParamDependency(MBParameter *param, char *line);
    void parseComment(char *line);
    void parseOutboardHost(char *line);
    void parseOutboardPersistent(char *line);
    void parseAsychronous(char *line);
    void parsePin(char *line);
    void parseSideEffect(char *line);
    char *nextItem(char *line, int &ndx);
    

  protected:

    virtual void clearText(Widget w);
    virtual void setTextSensitivity(Widget w, Boolean sens);

    //
    // Menus & pulldowns:
    //
    Widget		fileMenu;
    Widget		editMenu;
    Widget		buildMenu;

    Widget		fileMenuPulldown;
    Widget		editMenuPulldown;
    Widget		buildMenuPulldown;

    //
    // File menu options:
    //
    CommandInterface*	newOption;
    CommandInterface*	openOption;
    CommandInterface*	saveOption;
    CommandInterface*	saveAsOption;
    CommandInterface*	quitOption;
    CommandInterface*	commentOption;
    CommandInterface*	optionsOption;
    CommandInterface*	buildMDFOption;
    CommandInterface*	buildCOption;
    CommandInterface*	buildMakefileOption;
    CommandInterface*	buildAllOption;
    CommandInterface*	buildExecutableOption;
    CommandInterface*	buildRunOption;

    Command		*newCmd;
    Command		*openCmd;
    Command		*saveCmd;
    Command		*saveAsCmd;
    ConfirmedQCommand   *quitCmd;
    Command		*commentCmd;
    Command		*optionsCmd;
    Command*		buildMDFCmd;
    Command*		buildCCmd;
    Command*		buildMakefileCmd;
    Command*		buildAllCmd;
    Command*		buildExecutableCmd;
    Command*		buildRunCmd;

    //
    // Help menu
    //
    CommandInterface*	helpTutorialOption;

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
           OPEN,
           SAVE,
           SAVE_AS,
           CLOSE,
	   COMMENT,
	   OPTIONS,
	   BUILD_MDF,
	   BUILD_C,
	   BUILD_MAKEFILE,
	   BUILD_ALL,
	   BUILD_EXECUTABLE,
	   BUILD_RUN
    };

    void setAsynchronous(boolean);
    virtual void newMB(boolean create_default_params);
    virtual boolean saveMB(char *filenm);
    virtual void openMB(char *filenm);
    virtual boolean validateMB();
    virtual void setFilename(char *filenm);
    virtual char *getFilename(){return DuplicateString(this->filename);};
    virtual void build(int flag);
    virtual void notifyOutboardStateChange(Boolean set);
    virtual void notifyLoadableStateChange(Boolean set);
    boolean checkOutboardToggle();

    //
    // Creates the editor window workarea (as required by MainWindow class).
    //
    virtual Widget createWorkArea(Widget parent);

    //
    // Creates the editor window menus (as required by MainWindow class).
    //
    virtual void createMenus(Widget parent);

    virtual void createFileMenu(Widget parent);
    virtual void createEditMenu(Widget parent);
    virtual void createBuildMenu(Widget parent);
    virtual void createHelpMenu(Widget parent);

    MBMainWindow();

    //
    // Destructor:
    //
    ~MBMainWindow();

    //
    // Sets the default resources before calling the superclass
    // initialize() function.
    //
    virtual void initialize();

    void	manage();

    void setCommentText(char *s);

    const char* getCommentText()
    {
	return (const char*)this->comment_text;
    }

    //
    // called by MainWindow::CloseCallback().
    //
    virtual void closeWindow();
 
    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassMBMainWindow;
    }
};


#endif // _MBMainWindow_h
