/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

/* 
 * DXEnvironment is a class that sets up the environment for running DX.
 * It provides the command line argument processing and sets appropriate
 * environment variables used by the main dx code.
 */

	/* Because there is so much that can be done with environment variables and
	 * command line arguments with dx, I created a class that can subdivide
	 * needed arguments between UI and exec and setup any extra needed environment
	 * varaibles. The user can then use the class to retrieve either UI or
	 * exec command line options. 
	 */

#ifndef _TMAINUTIL_H_
#define _TMAINUTIL_H_

#pragma unmanaged

#include <windows.h>
#include <string>
using std::string;

#define MAXPARMS 200
#define MAXENV  24576


int ShortHelp(); // Print out short help and exit
int LongHelp();  // Print out long help and exit


class __declspec(dllexport) DXEnvironment {
public:
	// Setup is the function that must be run first with main's argc, argv
	// startup. This will populate the structure.
	int Setup(const int, char **);

	string getExFlags(); // return a single string of all flags listed
	string getUiFlags(); // together.

	bool isExecOnly() { return exonly; }

	int getExArgs(char**&); // return argc and argv versions of args
	int getUiArgs(char**&); // return value is same as argc, does alloc
						   // char ** (must be freed)

	// Default constructor for initialization.
	DXEnvironment() : dxroot(""), dxargs(""),
		dxdata(""), dxmacros(""), dxmodules(""),
		dxmdf(""), dxinclude(""), dxcolors(""),
		magickhome(""), curdir(""), exhost(""),
		cdto(""), exflags(NULL), exnumflags(0), uiflags(NULL), 
		uinumflags(0), filename(""),
		port(""), numparams(0), exonly(false), uionly(false), 
		showversion(false) {}

	~DXEnvironment(); // Destructor to free memory.

private:
	string dxroot;		// root of the dx app
	string dxargs;		// any args set as an env var
	string dxdata;		// path to the dxdata - set by both cl and env var
	string dxmacros;	// path to dxmacros - set by both cl and env var
	string dxmodules;	// path to dxmodules - set by env var
	string dxmdf;		// path to dxmdf - set by env var
	string dxinclude;	// path to dxinclude - set by env var
	string dxcolors;	// path to dxcolors - set by env var
	string magickhome;  // path to ImageMagick home - set by env var
	string curdir;		// call to getcwd
	string params[MAXPARMS]; // all the params
	string exhost;		// executive host - set on command line
	int numparams;		// total number of parameters in params.
	string cdto;		// directory where app should be run from.
	string uimode;		// what mode flags to send to the ui
	string *exflags;		// flags for executive.
	int exnumflags;		// number of exargs
	string *uiflags;		// flags specific to the ui
	int uinumflags;		// number of uiargs
	bool exonly;		// Run executive only;
	bool uionly;		// Run ui only;
	string filename;    // Filename of network to load/run
	bool showversion;	// Show version
	string port;		// Port to share socket over

	void addExFlag(string);
	void addUiFlag(string);


};

#pragma managed

#endif /* _TMAINUTIL_H_ */