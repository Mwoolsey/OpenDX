.de SO
\fB\\$1\fR \\$2
..
.TH DX 1V "" "IBM Visualization Data Explorer" 
.SH NAME
dx \- start the Data Explorer visualization system.  Optionally
directly start the User Interface 
(\fIdxui\fR), the executive (\fIdxexec\fR), the Data Prompter,
the Module Builder or the Tutorial. 
.SH SYNOPSIS
.nf
.br
dx	[ -8bitcmap < private | shared | 0-1 > ]
 	[ -builder ]
 	[ -cache < on | off > ]
	[ -colors < colors file > ]
	[ -connect < host:port > ]
	[ -data < path list > ]
	[ -directory  < dir > ]
	[ -display < display name > ]
	[ -dxroot < dir > ]
	[ -echo ]
	[ -edit ]
	[ -encode ]
	[ -exec < executable > ]
	[ -execonly ]
	[ -execute ]
	[ -execute_on_change ]
	[ -full ]
	[ -file < filename > ]
	[ -help ]
	[ -highlight < on | off > ]
	[ -host < host name > ]
	[ -hwrender < gl | opengl > ]
	[ -image ]
	[ -include < path list > ]
	[ -key < 64bit hex > ]
	[ -license < type > ]
	[ -local ]
	[ -log < on | off > ]
	[ -macros  < path list > ]
	[ -mdf < filename > ]
	[ -memory < # of Mbytes > ]
	[ -menubar ]
	[ -metric ]
	[ -modules < path list > ]
	[ -morehelp ]
	[ -nodeid ]
	[ -optimize < memory | precision > ]
	[ -outboarddebug ]
	[ -processors ]
	[ -program < filename > ]
	[ -prompter ]
	[ -readahead < on | off > ]
	[ -script [ < script file > ] ]
	[ -startup ]
	[ -suppress ]
	[ -timing < on | off >]
	[ -trace < on | off > ]
	[ -trialkey ]
	[ -tutor ]
	[ -uionly | -execonly ]
	[ -verbose ]
	[ -version]
.sp
.br
dx	\-prompter 	( start the Data Prompter )
	[ -full ] 
	[ -file ] [ < header file name > ] 
.sp
.br
dx	\-builder	( start the Module Builder )
	[-file ] [ < .mb file name > ]
.in
.fi
.sp 6
.SH DESCRIPTION
.I dx 
is the command that starts the Data Explorer Startup Window,
and optionally starts the 
.IR "Data Explorer User Interface",
.IR "Data Explorer Executive" , 
.IR "Data Explorer Data Prompter", 
.IR "Data Explorer Module Builder" 
or the 
.IR "Data Explorer Tutorial".
.P
The Data Explorer Startup Window is an X windows/Motif
based application which allows easy access to the other 
subsystemes of the Data Explorer visualization system. 
.P
The 
.I "Data Explorer User Interface"
is an X windows/Motif based application that
provides a visual programming environment and an interface
to the executive.
The executive provides the computational tools and mechanisms
to create data visualizations.   
The 
.P
.I "Data Explorer Data Prompter" 
is an X windows/Motif based application that helps users import
data in a variety of formats, and provides general purpose visual
programs to visual imported data, leading to a shortened 
"time to first picture". 
.P
The 
.I "Data Explorer Module Builder" 
is an X windows/Motif based application that helps users build 
their own modules to be included in a custom version of
Data Explorer. 
.P
The 
.I "Data Explorer Tutorial" 
is an application that helps users learn to use Data
Explorer by leading them through a sequence of tasks.
Users can easily skip around in the tutorial in order
to concentrate on tasks of most use to their work. 
.P
For a more complete description
of the 
.I "Data Explorer User Interface and Executive"
see the
.BR "IBM Visualization Data Explorer User's Guide" .
For a more complete description of the 
.I "Data Explorer Data Prompter"
see the
.BR "IBM Visualization Data Explorer QuickStart Guide" .
For a more thorough description of the
.I "Data Explorer Module Builder"
see the
.BR "IBM Visualization Data Explorer Programmer's Reference" .
.SH OPTIONS
With no options the 
.I dx
command starts the Startup Window, an X windows/Motif program
from which the other parts of the Data Explorer system can
be started.  The various command line options and environment
variables listed below can be specified to change the default
behavior.
.P
Options set by including them in the DXARGS environment variable 
are placed at the start of the command line, followed by any options
set by environment variables, followed by explicit command line
arguments.  When conflicting options are given, the
last option has precedence.
.TP
.SO -8bitcmap "< private | shared | 0-1  | -1 >"
Overrides the DX8BITCMAP environment variable (see ENVIRONMENT
VARIABLES below).
.TP
.SO -cache "< on | off >"
Turn on or off the executive caching system. 
When the Data Explorer caching mechanism is enabled the Data Explorer
executive automatically caches the intermediate results generated
during its visualization computations.  Unlike standard caching
mechanisms, these results are accessed by means of their functional
derivation.  This allows for automatic graph optimization and helps
to avoid the recomputation of previously utilized intermediates,
especially when the Data Explorer sequencer is being used.  In 
certain rare instances, a machine with a small amount of memory may
want to disable this caching to conserve memory and reduce paging.
Default = on. 
.TP
.SO -colors "< colors file >"
Override DXCOLORS environment variable (see ENVIRONMENT VARIABLES below).
.TP
.SO -connect "< host:port >"
Start a distributed executive only (no user interface).
.TP
.SO -data "< path list >"
Override DXDATA environment variable (see ENVIRONMENT VARIABLES below).
.TP
.SO -directory  "< dir >"
Cause the executive to be run in the given directory.
.TP
.SO -display "< display name >"
Set the name of the X display to be used by the user interface and the
executive.
.TP
.SO -dxroot "< dir >"
Overrides the default dx root directory.  
.TP
.SO -echo "< dir >"
Echo the command lines without executing them.
.TP
.SO -edit
Cause the user interface to be started with the Visual Program
Editor (VPE) window as the top level window of the user interface
(the top level window is denoted by an anchor in the upper 
left hand corner). 
Also see -image, -startup,  and -menubar.
.TP
.SO -encode "< filename >"
Encode the given file into a binary format with a key
(which must be specified with the -key option). For example
    dx -encode -key 1234567890abcdef foo.net
The resulting file can only be encoded by the DX user interface
when the same key is used. For example
    dx -image -key 1234567890abcdef foo-e.net 
.TP
.SO -exec "< executive executable >"
Use the given file as a replacement for the executive. 
This option allows a user to build and use his/her own custom
executive. 
.TP
.SO -execonly
Start the executive in remote mode in which it will then await
a connection from the user interface.  The user interface
is not started with this option.   
Also see -script.
.TP
.SO -execute 
Execute once after the user interface has made a connection with
the executive.
.TP
.SO -execute_on_change
Cause Data Explorer to go into execute-on-change mode at start up.
Execute-on-change mode causes the Visual Program to be executed
any time any of the values (i.e. module inupts, interactor values...)
in the program change. 
.TP
.SO -file "< filename >"
Start the Data Prompter, Module Builder, or Data Explorer User Interface
with the header file filename. 
.TP
.SO -full
When used with -prompter, starts the full Data Prompter. 
.TP
.SO -help
Print a brief listing of options and usage for 
.IR dx .
.TP
.SO -highlight "< on | off >"
Turn on or off module execution highlighting. 
If highlighting is turned on, the user interface will highlight each
module as it is executed.
Default = on. 
.TP
.SO -host "< host name >"
Override DXHOST environment variable (see ENVIRONMENT VARIABLES below).
.TP
.SO -hwrender "< gl | opengl >"
Override the default hardware rendering choice on platforms where both
are supported. Default is opengl. Sets the DXHWMOD environment variable
to either DXhwdd.o or DXhwddOGL.o.
.TP
.SO -image
Cause the user interface to be started with the image window
as the top level window of the user interface.
Also, control panels that are configured to open automatically are opened.
Also see -edit, -startup and -menubar.
.TP
.SO -include "< path list >"
Override DXINCLUDE environment variable (see ENVIRONMENT VARIABLES below).
.TP
.SO -key "< 64bit hex >"
16 character hexidecimal (64bit) number that is used
to encode and decode .net files. See -encode.
.TP
.SO -license "< type >"
Force a specific license for either the user interface or executive.
The following are valid values for type:
.sp
runtime: forces a run-time license (UI or executive). 
.sp
timed: forces a timed (limited function) license (UI only).  
.sp
develop: forces a developer license (UI or executive). 
.TP
.SO -local 
Override the DXHOST environment variable (if set) and
cause the executive to be run on the local host.
In the absence of the DXHOST environment variable, this is the default.
.TP
.SO -log "< on | off >"
Turn on or off executive logging.  
If logging is on then the executive logs output to the file
.I dx.log
in the directory where the executive is executing.
Default = off.
.TP
.SO -macros "< path list >"
Override the DXMACROS environment variable (see ENVIRONMENT VARIABLES below).
.TP
.SO -menubar
Start Data Explorer with a menubar as the anchor window.
Also, control panels that are configured to open automatically are opened.
See also -image, -edit, and -startup.
.TP
.SO -mdf "< mdf file >"
Override the DXMDF environment variable (see ENVIRONMENT VARIABLES below).
.TP
.SO -memory "< # of MB >"
Set the maximum amount of memory, in megabytes rounded up to a two
megabyte boundary, that the executive is able to allocate. 
Default = the amount of physical memory minus 8 MB. 
.TP
.SO -metric
Have the ui use metric units when possible, for example for
setting the image size for printing postscript images.
.TP
.SO -modules "< pathlist >"
Set the list of directories to be searched for outboard modules.
.TP
.SO -morehelp
Displays this information
.TP
.SO -nodeid "
print out the node id of the machine that the command is run on. 
The node id is used when generating a license.
.TP
.SO -optimize "< memory | precision >"
Select whether to minimize memory usage or produce more
color-accurate images.  When memory is optimized, image objects are
generated with 24 bits/pixel instead of 96, and ReadImage will 
produce delayed color images if supported by the format. The
default is precision.
.TP
.SO -outboarddebug
Enable user to start outboard modules manually for debugging when
they are called in a visual program.
.TP
.SO -processors
For Data Explorer SMP, sets the number of processors to use.
.TP
.SO -program "< programname >"
Start the user interface with the visual program filename.
.TP
.SO -prompter
Start the data prompter.
.TP
.SO -readahead "< on | off >"
Specify whether or not the executive reads ahead
in its input stream before executing the commands it receives
from the stream.  If this is turned off, then the executive
will execute each command line as it is received.  This
is useful for certain debugging purposes, such as memory leak
detection.
Default = on. 
.TP
.SO -script "[ < script file > ]"
Run just the executive in script mode.  
If a file name is provided then the executive reads and executes commands
from the given script and then terminates.
If a file is not provided then when the executive starts up it enters
.I interactive
mode where it waits for script commands.
Also see -execonly.
.TP
.SO -startup
Start Data Explorer with the startup window as the initial window. This
is the default. See also -image, -edit, and -menubar.
.TP
.SO -suppress
Do not open any control panels at start-up. This is used in -image mode
and -menubar mode only.
.TP
.SO -timed
Start DX using a timed license.
.TP
.SO -timing "< on | off >"
Turn on or off the executive timing mechanism. 
If enabled, then when the executive exits it will list on
standard output or the logging file timing statistics 
for each module that was used during the course of execution.
Default = off. 
.TP
.SO -trace "< on | off >"
Turn on or off the executive trace mechanism. 
When tracing is enabled the executive displays informative messages
about the execution sequence on either standard output or the log file
if logging is enabled.
Default = off. 
.TP
.SO -trialkey 
Determines the information necessary for generating a trialkey.
.TP
.SO -tutor
Start the Data Explorer tutorial.
.TP
.SO -uionly
Cause the user interface to be started without making a connection
to an executive.
.TP
.SO -verbose
Echo commands in the
.I dx
script as they are executed.
.TP
.SO -version
Print version numbers of various executables. 
.SH "ENVIRONMENT VARIABLES"
The following environment variables provide an alternate way
to set certain of the command line options, or alter the
default behavior of Data Explorer in some way. Options set with
environment variables are overridden if the same option is
also specified on the command line.  Options set with DXARGS
are overridden by both environment variables and explicit
command line options.
.TP
.SO DX8BITCMAP 
When Data Explorer displays images in 8-bit windows, it makes every 
effort to create an accurate representation of the image using the
default colormap shared among applications using 8-bit windows.  If the
colors required by Data Explorer are not present in the colormap 
and there are not enough free color cells available in the shared
color map for their allocation, Data Explorer will find approximations
among the colors that are available.  However, in certain circumstances,
the difference between the desired color and the approximation is 
found to be too great to be acceptable.  In such circumstances, DX
will allocate a private colormap for more accurate results.  In
this case, depending on your X server, you may need to move the mouse to
select the Image or Display window in order for the correct colors
to appear.  The -8bitcmap flag sets the level at which the change to
using a private colormap is made.  The value of this parameter should
be a number between 0 and 1, or "private" or "shared", 
and it repressents the Euclidean distance in RGB color space, 
normalized to 1, for the maximum allowed discrepancy.  If you set 
this parameter to 1, then a private colormap will never be used. If
you set it to -1, then a private colormap will always be used. The
default is 0.1.
.TP
.SO DXARGS 
This contains the default set of arguments used by the 
.I dx
script. 
The arguments are prepended to those given to 
.I dx
on the command line.
This variable allows for the 
.I dx
command to be easily customized.
.TP
.SO DXAXESMAXWIDTH
Sets the number of digits in axes tick labels at which a switch to 
scientific notation is made. The default is 7.
.TP
.SO DXCOLORS
This contains the name of an alternate  file that contains
mappings of color names to red, green, blue triplets in a
format like that of the default color file found in
/usr/lpp/dx/lib/colors.txt.
.TP
.SO DXDATA 
This contains a colon separated list 
of directories in which the executive will search for 
data files.
Also see the -data option.
.TP
.SO DXDELAYEDCOLORS
If set to anything other than 0, enables ReadImage to create delayed
color images if the image is a tiff format image saved in a 
byte-with-colormap format or a gif image.  
.TP
.SO DXEXEC
Specifies an executive to be run at start-up. 
.TP
.SO DXFLING
If DXFLING is set to 1, then for hardware-rendered images, in 
rotation mode and execute-on-change mode, if you drag the mouse
across the image, and release the mouse button outside the image,
the object in the image will begin to rotate, and will continue
to rotate until you click inside the image.  The direction and speed
of the mouse motion before release will affect the rotation 
direction and rotation speed of the object in the window.
.TP
.SO DXGAMMA
Sets the gamma correction for software-rendered images displayed 
to the screen using the Display and Image tools.  The default value is 
2, except for 8-bit windows on the sgi architecture.
.TP
.SO DXGAMMA_8BIT, 
.SO DXGAMMA_12BIT, 
.SO DXGAMMA_24BIT
sets the gamma correction factor for software-rendered images for 
the windows with the indicated window depth.  Overrides the 
value set by DXGAMMA.
.TP
.SO DXHOST 
This contains the name of the host machine 
that will run the executive.  
If this is not set then a default of "localhost" is used,
unless the local host is a PVS console in which case the default is
to run the executive on the PVS.
Also see the -host, -local and -server options.
.TP
.SO DXHWGAMMA
Sets the gamma correction for hardware-rendered images displayed 
to the screen using the Display and Image tools.  The default value is 2. 
.TP
.SO DXHWMOD 
Specifies the name of the hardware rendering library to use when more
than one is supported. Should be either DXhwdd.o or DXhwddOGL.o.
.TP
.SO DXINCLUDE
This contains a colon separated list 
of directories in which the executive will search for "included" scripts.
Also see the -include option.
.TP
.SO DXMACROS 
This contains a colon separated list 
of directories in which the user interface searches for macros 
that will be automatically loaded.
When running in script mode one must "include" all the macros
by hand prior to referencing them.
Also see the -macros option.
.TP
.SO DXMDF 
This contains the name of a module description file (mdf) that
contains user defined modules for a customized version of 
the executive.
Also see the -mdf option.
.TP
.SO DXMEMORY
sets the amount of memory (in megabytes) that can be used by
the executive. 
.TP
.SO DXMODULES
This contains a colon separated list 
of directories in which the executive searches for outboard modules. 
Also see the -modules option.
.TP
.SO DX_NESTED_LOOPS
For faces, loops, and edges data, if set, allows loops other than
the enclosing loop for a face to be listed first. However, there is a 
consequent decrease in performance if this environment variable is set.
.TP
.SO DXNO_BACKING_STORE
If set to anything, disables framebuffer readbacks.  Setting this
environment variable may improve performance of interaction with
hardware rendered images, especially for machines for which readback
is slow. The default behavior is that framebuffer readbacks are 
enabled. If you set this environment variable, not that some interactions
using the Image tool will result in a black image while interaction
is taking place. However, if you are not using the Image tool, it 
is strongly recommended that you set this environment variable.
.TP
.SO DXPIXELTYPE
sets the image type to either 24-bit color images or floating-point-based
96-bit images (the default). This affects the behavior of Render and 
ReadImage.  This variable can be set to either DXByte (24 bits) or
DXFloat (96 bits).  Setting this variable to DXByte will result in
images taking up less memory.
.TP
.SO DXPROCESSORS
Sets the number of processors to be used for Data Explorer SMP.
.TP
.SO DXRSH
Specifies the remote shell command to use when connecting to other systems
(such as those when using distributed execution). The default is the the
path to rsh.
.TP
.SO DXRSH_NOENV
Prevents the remote "invoke dxexec" script from being written with a full
carbon copy of the local environment (only DISPLAY is set).
.TP
.SO DXROOT
Specifies the top-level directory for all the files and directories
needed by Data Explorer.  
.TP
.SO DXSHMEM
If set to anything other than -1, forces shared memory to be used (if
allowed by the architecture).  If set to -1, (if allowed by the architecture) 
will cause the data segment to simply be extended. Please see Appendix C of the
Data Explorer User Guide for more information.
.TP
.SO DX_SOCKET_BUFSIZE
Allows the user to specify the size of the send and recieve buffers for the
socket connection between the UI and the exec. This value is clamped to the
max allowed by the system. This can fix a problem where the executive and UI
hang involving a deadlock situation in the socket communication. The value
assigned to it is passed as the parameter to setsockopt for SOL_SNDBUF and
SOL_RCVBUF. A typcial increased value may be 262144.
.TP
.SO DX_USER_INTERACTOR_FILE
Specifies a file containing user interactors for use by the
SuperviseState and SuperviseWindow modules.
.TP
.SO DX_WEB_BROWSER
Under UNIX specifies the web browser to launch for viewing the HTML
documentation. Under Windows and MacOS X setting this environment variable
will launch the systems default web browser.
.SH EXAMPLES
The next few examples assume that none of the DX environment variables
have been set.
.P
1. To start both the user interface and the executive with the network named 
.IR cfd.net :
.RS
dx -edit -program cfd 
.RE
.sp
.P
2. To start and execute the visual program 
.I cfd.net 
in image mode with /usr/users/me and /usr/data as data directories:
.RS
dx -image -execute -data '/usr/users/me:/usr/data' cfd
.RE
.P
3. To run just the executive on the script named 
.IR f15wing :
.sp
.RS
dx -script -file f15wing
.RE
.P
4. To start the user interface and execute the user customized executive 
named 
.I mydxexec
containing user defined modules described in  
.IR custom.mdf :
.sp
.RS
dx -edit -mdf custom.mdf -exec mydxexec
.RE
.P
5. To start the Data Prompter :
.sp
.RS
dx -prompter
.RE
.P
6. To start the Module Builder :
.sp
.RS
dx -builder
.RE
7. To start the Tutorial :
.sp
.RS
dx -tutorial
.RE
.sp
.SH "SEE ALSO"
.TP
README* - The README file(s) found in the dx 
installation directory are an added source of information.
html/allguide.htm contains html versions of the documentation.
