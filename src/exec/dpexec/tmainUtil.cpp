/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 2004                                        */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

/* please read tmainUtil.h for explanation of class */

#pragma unmanaged

#include "tmainUtil.h"
#include <direct.h>
#include <windows.h>
#include <iostream>

using namespace std;

// forward declarations
int fillparms(string[], const char *, int*);

int getenvstr(char *name, string& value)
{
	size_t requiredSize;
	char *s=NULL;

	getenv_s( &requiredSize, NULL, 0, name);
	if(requiredSize > 1) {
		s = new char[requiredSize];
		if(!s)
		{
			printf("Failed to allocate memory!\n");
			exit(1);
		}
		getenv_s( &requiredSize, s, requiredSize, name);

		if (s) {
			value = s;
			// Go ahead and remove any enclosing quotes.
			for(int i=value.length()-1; i >= 0; i--) {
				if(value[i] == '"')
					value.erase(i, 1);
			}
		}
	}
	return ((s && *s) ? 1 : 0);
}

/*  In Windows, the name/value pair must be separated by = 	*/
/*  with no blanks on either side of =.  Null values would have */
/*  name=\0 to unset them in the environment.			*/
int putenvstr(char *name, const char *in)
{
	char *value = new char[strlen(in)+1];
	strcpy_s(value, strlen(in)+1, in);
    char s[MAXENV];
    char *p, *q;
    int rc;
    int len;
    int newlen;

    if (!*name)
	return 0;

    if (!*value)
        return 0;

    for(p = name; *p == ' '; p++);
    for(q = &name[strlen(name)-1]; *q == ' ' && q != p; q--);

    len = (int)(q-p)+1;
    strncpy_s(s, MAXENV, p, len);
    s[len] = '\0';
    strcat_s(s, MAXENV, "=");

    for(p = value; *p == ' '; p++);
    if(strlen(p)) {
	for(q = &value[strlen(value)-1]; *q == ' ' && q != p; q--);
	if (*p != ' ') {
	    newlen = strlen(s);
	    len = (int)(q-p)+1;
	    strncat_s(s, MAXENV, p, len);
	    s[len+newlen] = '\0';
	}
    }

    p = new char[strlen(s) + 1];
    strcpy_s(p, strlen(s)+1, s);
    
    rc = _putenv(p);
	delete p;
	delete value;

    return (!rc ? 1 :0);
}

DXEnvironment::~DXEnvironment() {
	if(exnumflags>0)
		delete [] exflags;
	if(uinumflags>0)
		delete [] uiflags;
}

void DXEnvironment::addExFlag(string s) {
	string *temp = new string[exnumflags+1];
	for(int i = 0; i < exnumflags; i++)
		temp[i] = exflags[i];

	temp[exnumflags] = s;
	exnumflags++;

	if(exnumflags>1)
		delete [] exflags;

	exflags = temp;
}

void DXEnvironment::addUiFlag(string s) {
	string *temp = new string[uinumflags+1];
	for(int i = 0; i < uinumflags; i++)
		temp[i] = uiflags[i];

	temp[uinumflags] = s;
	uinumflags++;

	if(uinumflags>1)
		delete [] uiflags;

	uiflags = temp;
}

string DXEnvironment::getExFlags() {
	string temp;

		for(int i=0; i < exnumflags; i++)
			temp += exflags[i] + " ";

	return temp;
}

string DXEnvironment::getUiFlags() {
	string temp;

		for(int i=0; i < uinumflags; i++)
			temp += uiflags[i] + " ";

	return temp;
}

int DXEnvironment::getExArgs(char ** &argv) {

	argv = new char*[exnumflags];
	for(int i = 0; i < exnumflags; i++) {
		argv[i] = new char[exflags[i].length() + 1];
		strcpy_s(argv[i], exflags[i].length()+1, exflags[i].c_str());
	}

	return exnumflags;
}

int DXEnvironment::getUiArgs(char ** &argv) {

	argv = new char*[uinumflags];
	for(int i = 0; i < uinumflags; i++) {
		argv[i] = new char[uiflags[i].length()+1];
		strcpy_s(argv[i], uiflags[i].length()+1, uiflags[i].c_str());
	}

	return uinumflags;
}


#define checkna(str)	\
    if ((i >= numparams-1) || (params[i+1][0] == '-')) {	\
		throw(str);		\
    }					\
	i++;				\
	s = params[i];

int DXEnvironment::Setup(const int argc, char **argv) {
	char cwd[512];
	_getcwd(cwd, sizeof(cwd));
	curdir = cwd;
	string localpath;
	char path[MAXENV];
	bool seeflags = false;

	getenvstr("Path", localpath);
	strcpy_s(path, MAXENV, localpath.c_str());
	getenvstr("DXARGS", dxargs);
	getenvstr("DXROOT", dxroot);
	getenvstr("DXDATA", dxdata);
	getenvstr("DXMACROS", dxmacros);
	getenvstr("DXMODULES", dxmodules);
	getenvstr("DXMDF", dxmdf);
	getenvstr("DXINCLUDE", dxinclude);
	getenvstr("DXCOLORS", dxcolors);
	getenvstr("MAGICK_HOME", magickhome);

	if(dxroot == ""){
		TCHAR path[512];
		/* get dxroot by locating path to executable. */
		if(GetModuleFileName(NULL, path, 512)) {
			dxroot = path;
		}
	}

	if(dxroot.rfind(".exe") != string::npos) {
		// remove exe file name
		int lastbs = dxroot.rfind("\\");
		dxroot.erase(lastbs);
	}

	// remove the last slash or backslash.
	if(dxroot != "" && 
		(dxroot[dxroot.length()] == '/' || dxroot[dxroot.length()] == '\\'))
		dxroot.erase(dxroot.length());

	/* now add dxroot\samples\data to dxdata */
	if(dxdata != "")
		dxdata += ";";
	dxdata += dxroot;
	dxdata += "\\samples\\data";

	/* now add dxroot\samples\macros to dxmacros */
	if(dxmacros != "")
		dxmacros += ";";
	dxmacros += dxroot;
	dxmacros += "\\samples\\macros";

	// getparms

	// fill parms array, first with DXARGS values 
	// then with the command line options
	// then with pipe args
	// so they will have priority given to the last ones in

	int n = 0;
	int i = 0;

	fillparms(params, dxargs.c_str(), &n);

	int readpipe = 0;

	// The UI and EX need the exe given.
	addUiFlag(argv[0]);
	addExFlag(argv[0]);

	for(i=1; i < argc; i++) {
		params[n] = argv[i];
		if(params[n] == "-pipeargs")
			readpipe = 1;
		else {
			n++;
		}
	}

	if(readpipe) {
		char c;
		string pipestr;
		for(i = 0; (c=getchar()) != EOF; i++)
			pipestr[i] = c;
		pipestr[i] = '\0';
		fillparms(params, pipestr.c_str(), &n);	
	}

	numparams = n;

	/* now parse the paramters */
	//try {
		string exhighlight = "-B";

		for(i=0; i<numparams; i++) {
			string s = params[i];
			if(s == "-whereami") {
				printf("installed in %s\n", dxroot);
				exit(0);
			}
			if(s == "-whicharch") {
				printf("windows\n");
				exit(0);
			} 
			if(s == "-host") {
				checkna("-host: missing host name");
				exhost = s;
			} else 
			if(s == "-local") {
				exhost = "localhost";
			} else
			if(s == "-connect") {
				throw("Not able to run remote at this time.");
			} else
			if(s == "-directory") {
				checkna("-directory: missing directory name");
				cdto = s;
			} else
			if(s == "-memory") {
				checkna("-memory: missing parameter, must give number of Megabytes");
				addExFlag(" -M" + s );
			} else
			if(s == "-threads") {
				checkna("-threads: missing parameter, must give number of threads");
				addExFlag("-p");
				addExFlag(s);
			} else
			if(s == "-port") {
				checkna("-port: missing port number");
				port = s;
			} else
			if(s == "-image" || s == "-edit" || s == "-java" || s == "-kiosk" || s == "-menubar") {
				uimode = s;
				if(s == "-menubar") 
					uimode = " -kiosk";
				addExFlag(" -r");
			} else
			if(s == "-execute")
				addUiFlag(s);
			else
			if(s == "-execute_on_change")
				addUiFlag(s);
			else
			if(s == "-suppress")
				addUiFlag(s);
			else
			if(s == "-synchronous")
				addUiFlag(s);
			else
			if(s == "-log") {
				if( i >= numparams )
					throw("-log: missing parameter, must be on or off");
				i++; s = params[i];
				if(s == "on") {
					addUiFlag("-log");
					addUiFlag("on");
					addExFlag("-l");
				} else if (s == "off") {
					addUiFlag("-log");
					addUiFlag("off");
				} else
					throw("-log: bad parameter, must be on or off");
			}
			else
			if(s == "-cache") {
				if(i >= numparams )
					throw("-cache: missing parameter, must be on or off");
				i++; s = params[i];
				if(s == "on") {
					addUiFlag("-cache");
					addUiFlag("on");
				} else if (s == "off") {
					addUiFlag("-cache");
					addUiFlag("off");
					addExFlag("-c");
				} else
					throw("-cache: bad parameter, must be on or off");
			}
			else
			if(s == "-trace") {
				if(i >= numparams)
					throw("-trace: missing parameter, must be on or off");
				i++; s = params[i];
				if(s == "on") {
					addExFlag("-T");
					addUiFlag("-trace");
					addUiFlag("on");
				} else if (s == "off") {
					addUiFlag("-trace");
					addUiFlag("off");
				} else
					throw("-trace: bad parameter, must be on or off");
			}
			else
			if(s == "-readahead") {
				if(i >= numparams)
					throw("-readahead: missing parameter, must be on or off");
				i++; s = params[i];
				if(s == "on") {
					addUiFlag("-readahead");
					addUiFlag("on");
				} else if (s == "off") {
					addExFlag("-u");
					addUiFlag("-readahead");
					addUiFlag("off");
				} else
					throw("-readahead: bad parameter, must be on or off");
			}
			else
			if(s == "-timing") {
				if(i >= numparams)
					throw("-timing: missing parameter, must be on or off");
				i++; s = params[i];
				if(s == "on") {
					addExFlag("-m");
					addUiFlag("-timing");
					addUiFlag("on");
				} else if (s == "off") {
					addUiFlag("-timing");
					addUiFlag("off");
				} else
					throw("-timing: bad parameter, must be on or off");
			}
			else
			if(s == "-highlight") {
				if(i >= numparams)
					throw("-highlight: missing parameter, must be on or off");
				i++; s = params[i];
				if(s == "on") {
					addUiFlag("-highlight");
					addUiFlag("on");
				} else if (s == "off") {
					exhighlight = "";
					addUiFlag("-highlight");
					addUiFlag("off");
				} else
					throw("-highlight: bad parameter, must be on or off");
			}
			else
			if(s == "-optimize") {
				checkna("-optimize: missing parameter");
				if(s == "memory") {
					putenvstr("DXPIXELTYPE", "DXByte");
					putenvstr("DXDELAYEDCOLORS", "1");
				} else if (s == "precision") {
					putenvstr("DXPIXELTYPE", "DXFloat");
					putenvstr("DXDELAYEDCOLORS", "");
				} else
					throw("-optimize: parameter not recognized");
			}
			else
			if(s == "-script") {
				addExFlag("-R");
				exonly = true;
			}
			else
			if(s == "-exonly" || s == "-execonly") {
				addExFlag("-r");
				exonly = true;
			}
			else
			if(s == "-program") {
				checkna("-program: missing program name");
				filename = s;
			}
			else
			if(s == "-cfg") {
				checkna("-cfg: missing configuration file");
				addUiFlag("-cfg");
				addUiFlag(s);
			}
			else
			if(s == "-uionly") {
				uionly = true;
			}
			else
			if(s == "-dxroot") {
				checkna("-dxroot: missing directory name");
				dxroot = s;
			}
			else
			if(s == "-mdf") {
				checkna("-mdf: missing filename");
				dxmdf = s;
				addExFlag("-F");
				addExFlag(s);
				addUiFlag("-mdf");
				addUiFlag(s);
			}
			else
			if(s == "-data") {
				checkna("-data: missing directory list");
				dxdata = s;
			}
			else
			if(s == "-macros") {
				checkna("-macros: missing directory list");
				dxmacros = s;
			}
			else
			if(s == "-include") {
				checkna("-include: missing directory list");
				dxinclude = s;
			}
			else
			if(s == "-colors") {
				checkna("-colors: missing file name");
				dxcolors = s;
			}
			else
			if(s == "-verbose" || s == "-echo") {
				seeflags = true;
			}
			else
			if(s == "-help" || s == "-shorthelp" || s == "-h")
				ShortHelp();
			else
			if(s == "-morehelp" || s == "-longhelp")
				LongHelp();
			else
			if(s == "-version") {
				showversion = true;
			}
			else
			if(s == "-file") {
				checkna("-file: missing input filename");
				filename = s;
			}
			else
			if(s[0] == '-') {
				string err = "Unrecognized parameter: " + s;
				throw(err);
			}
			else
			if(filename != "") {
				string err = "input filename already set to '" + filename;
				err += "' ; '" + s + "' unrecognized";
				throw(err);
			}
			else
				filename = s;


		}

		addExFlag(exhighlight); // Since highlight can only be turned off with the flag.

	//} 
	//catch(char *err) {
	//	printf("%s\n", err);
	//	exit(1);
	//}

	strcat_s(path, MAXENV, ";");
	strcat_s(path, MAXENV, magickhome.c_str());

	putenvstr("DXDATA", dxdata.c_str());
	putenvstr("DXMACROS", dxmacros.c_str());
	putenvstr("DXMODULES", dxmodules.c_str());
	putenvstr("DXINCLUDE", dxinclude.c_str());
	putenvstr("DXMDF", dxmdf.c_str());
	putenvstr("DXCOLORS", dxcolors.c_str());
	putenvstr("MAGICK_HOME", magickhome.c_str());
	putenvstr("DXROOT", dxroot.c_str());

	if(exonly && uionly) {
		printf("-uionly and execonly are mutually exclusive.\n");
		exit(1);
	}

	if(seeflags) {
		//Need to print out env as well

		LPVOID d = GetEnvironmentStrings();
		LPTSTR s;

		if(d) {
			for(s = (LPSTR) d; *s; s++)
			{
				while( *s )
					putchar(*s++);
				putchar('\n');
			}
			FreeEnvironmentStrings((LPSTR)d);
		}

		cout << endl << "-----------------------------" << endl << endl;
		cout << "exflags: " << getExFlags() << endl;
		cout << "uiflags: " << getUiFlags() << endl;
		cout << "dxroot: " << dxroot << endl;
		cout << "dxargs: " << dxargs << endl;
		cout << "dxdata: " << dxdata << endl;
		cout << "dxmacros: " << dxmacros << endl;
		cout << "dxmodules: " << dxmodules << endl;
		cout << "dxmdf: " << dxmdf << endl;
		cout << "dxinclude: " << dxinclude << endl; 
		cout << "dxcolors: " << dxcolors << endl;
		cout << "magickhome: " << magickhome << endl; 
		cout << "curdir: " << curdir << endl;
		cout << "exhost: " << exhost << endl;
		cout << "cdto: " << cdto << endl;
		cout << "filename: " << filename << endl; 
		cout << "port: " << port << endl;
		exit(1);
	}

	if(cdto == "")
		cdto = cwd;
	_chdir(cdto.c_str());

	return 0;
}




/*  This fills the parm array with tokens that have their	*/
/*  leading and trailing blanks removed.  In Windows, filepaths	*/
/*  may contain spaces if they were passed in as quoted strings	*/
/*  and the shell may have removed the quotes, leaving interior	*/
/*  blanks.							*/
int fillparms(string parm[], const char *orig, int* n)
{
	char *p, *q, *s;
	s = new char[strlen(orig) + 1];
	strcpy_s(s, strlen(orig)+1, orig);

	for (p = s; *p; p++) 
		if (*p == '\t' || *p == '\n' || *p == '\f')
			*p = ' ';

	for (p = s; *p; p = q) {			/* start next */
		for( ; *p == ' '; p++)			/* skip to nonblank */
			;
		if(*p == '"')
			for(q = p; *q && *q != '"'; q++)	/* find quoted token end */
				;
		else
			for (q = p; *q && *q != ' '; q++)	/* find token end */
				;
		if (p == q)				/* no more tokens */
			return 1;
		char *dest = new char[q-p+1];
		strncpy_s(dest, q-p+1, p, (int)(q-p));		/* load it */
		dest[(int)(q-p)] = '\0';
		parm[*n] = dest;
		delete dest;
		(*n)++;
	}

	delete s;

	return 1;
}


int ShortHelp()
{
    printf(

"command line parameters:\n"
" -program filename    start UI with this network\n"
" -image               start DX with an image window as the anchor window \n"
" -edit                start DX with an editor window as the anchor window \n"
" -menubar             start DX with a small menubar as the anchor window \n"
" -startup             start DX with an initial startup panel (default) \n"
"\n"
" -uionly              start the UI only (no exec)\n"
" -script filename     start the exec only, in script mode, running this script\n"
" -script              start the exec only, in script mode\n"
"\n"
" -host hostname       start executive on this machine\n"
" -memory #Mbytes      set the amount of memory the exec uses\n"
"\n"
" -macros pathlist     directory list to search for UI macros\n"
" -data pathlist       directory list to search for data files\n"
"\n"
" -prompter            start the DX Data Prompter\n"
" -builder             start the DX Module Builder\n"
" -tutor               start the DX Tutorial\n"
"\n"
" -morehelp            print longer help with information about other options\n"
 

    );

    exit(0);
}

int LongHelp()
{
    printf(

"command line parameters:\n"
" -host hostname       start executive on this machine               \n"
" -local               start executive on the current machine (default)\n"
"\n"
" -program filename    start UI with this network\n"
" -script filename     run exec only, in script mode, with this script\n"
" -script              run exec only, in script mode\n"
"\n"
" -image               start DX with an image window as the anchor window \n"
" -edit                start DX with the VP Editor as the anchor \n"
" -menubar             start DX with a small menubar as the anchor window \n"
" -startup             start DX with an initial dialog (default) \n"
"\n"
" -uionly              start the UI only (no exec)\n"
" -execonly            start the exec only (no UI) in remote mode\n"
" -connect host:port   start a distributed exec only (no UI)\n"
"\n"
" -prompter            start the DX Data Prompter\n"
" -jdxserver           start the JavaDX Server\n"
" -full                start the Full Data Prompter\n"
" -file filename       start the Data Prompter with this header file\n"
"\n"
" -builder             start the DX Module Builder\n"
" -tutor               start the DX Tutorial\n"
"\n"
" -suppress            do not open control panels at startup in image mode\n"
" -execute             execute the program automatically at startup\n"
" -execute_on_change   go into execute_on_change mode at startup\n"
"\n"
" -optimize [memory|precision]\n"
"                      select whether to minimize memory usage or to produce \n"
"                      more color-accurate images.  When memory is optimized, \n"
"                      image objects are generated with 24 bits/pixel instead \n"
"                      of 96, and ReadImage will produce delayed color images \n"
"                      if supported by the format. (default = precision)\n"
"\n"
" -memory #Mbytes      set the amount of memory the exec uses\n"
" -threads #threads    set the number of threads the exec uses \n"
"                      (SMP versions only)\n"
" -log [on|off]        executive and ui logging: (default = off)\n"
" -cache [on|off]      executive cache: (default = on)\n"
" -trace [on|off]      executive trace: (default = off)\n"
" -readahead [on|off]  executive readahead: (default = on)\n"
" -timing [on|off]     module timing: (default = off)\n"
" -highlight [on|off]  node execution highlighting: (default = on)\n"
" -directory dirname   cd to this directory before starting exec\n"
" -metric              have the UI use metric units when possible\n"
"\n"
" -exec filename       execute this user executive\n"
" -mdf filename        use this user .mdf file\n"
"\n"
" -key <64bit hex>     16 character hexidecimal (64 bit) number that is used\n"
"		      to encode and decode .net files.\n"
" -encode              Encode a .net file into a binary format with a key \n"
"                      that must be specified with the -key option.   \n"
"                      For example, \n"
"                        dx -encode -key 193495946952abed foo.net \n"
"                      The resulting file can only be decoded by the DX\n"
"                      user interface when using the same -key option.\n"
"                      For example, \n"
"                        dx -key 193495946952abed bar.net \n"
"\n"
" -dxroot dirname      dx root directory; defaults to /usr/lpp/dx\n"
"\n"
" -macros pathlist     directory list to search for UI macros\n"
" -data pathlist       directory list to search for data files\n"
" -include pathlist    directory list to search for script files\n"
" -modules pathlist    directory list to search for outboard modules\n"
"\n"
" -colors filename     replace default color names/RGB values file\n"
" -8bitcmap [private|shared|0-1|-1]\n"
"                      private/shared colormap error threshold (default=0.1)\n"
"                      -1 is equivalent to private.\n"
" -hwrender [gl|opengl]  \n"
"                      override default hardware rendering library on platforms\n"
"                      where both are supported.  (default = opengl).\n"
"\n"
" -verbose             echo command lines before executing\n"
" -echo                echo the command lines without executing them\n"
" -outboarddebug       let user start outboard modules by hand\n"
" -version             show version numbers of dxexec, dxui, dx.exe, and X server\n"
" -synchronous         force synchronization of X events between dx and the x server\n"
"\n"
" -help                print a short help message\n"
" -morehelp            print this help message\n"
"\n"
"environment variables:\n"
" DXHOST               sets hostname for -host\n"
"\n"
" DXPROCESSORS         sets number of processors for multiprocessor DX\n"
" DXMEMORY             sets memory limit in megabytes\n"
" DXAXESMAXWIDTH       sets the maximum number of digits in axes tick labels\n"
"                      before a switch to scientific notation is made\n"
" DXDEFAULTIMAGEFORMAT Sets the image type to either 24-bit color or floating\n"
"                      point color (96-bit) images depending on the value\n"
"                      DXByte (24-bit) or DXFloat (96-bit)\n"
" DXDELAYEDCOLORS      If set to anything other than 0, enables ReadImage to\n"
"                      created delayed color images if the image is stored in\n"
"                      a tiff byte-with-colormap format or a gif image\n"
" DX_NESTED_LOOPS      For faces, loops, and edges data, if set, allows loops\n"
"                      other than the enclosing loop for a face to be listed\n"
"                      first, with a consequent decrease in performance\n"
" DXPIXELTYPE          Sets the image type to either 24-bit color or floating\n"
"                      point (96-bit) color.  This affects the behavior of\n"
"                      Render and ReadImage.  This can be set to either DXByte\n"
"                      or DXFloat (default).\n"
" DX_USER_INTERACTOR_FILE Specifies a file containing user interactors for use by\n"
"                      the SuperviseState and SuperviseWindow modules\n"
"\n"
" DXEXEC               sets filename for -exec\n"
" DXMDF                sets filename for -mdf\n"
"\n"
" DXMACROS             sets pathlist for -macros\n"
" DXDATA               sets pathlist for -data\n"
" DXINCLUDE            sets pathlist for -include\n"
" DXMODULES            sets pathlist for -modules\n"
"\n"
" DXCOLORS             sets filename for -colors\n"
" DX8BITCMAP           sets threshold for -8bitcmap\n"
"\n"
" DXGAMMA              sets gamma correction for displayed images. Default is 2.\n"
" DXGAMMA_8BIT\n"
" DXGAMMA_12BIT\n"
" DXGAMMA_24BIT        sets the gamma correction factor for the windows with \n"
"                      the indicated window depth.  Overrides the value set \n"
"                      by DXGAMMA.\n"
"\n"
" DXHWMOD              specifies the name of the hardware rendering library \n"
"                      to use when more than one is supported. Should be\n"
"                      either DXhwdd.o or DXhwddOGL.o\n"
"\n"
" DXROOT               sets directory for -dxroot\n"
"\n"
" DXARGS               prepends these args to the command line\n"
"\n"
"command line parameters override environment variables.\n"
"If conflicting parameters are given, the last one has precedence.\n"
"Also, there are some other less frequently used command line options\n"
"that are not documented here.  See the User's Guide for a complete\n"
"list of options.\n"
"\n"


    );

    exit(0);
}
