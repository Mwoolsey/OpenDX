DX(1V)                                                     DX(1V)



NNAAMMEE
       dx   -  start  the  Data  Explorer  visualization  system.
       Optionally directly start the User Interface  (_d_x_u_i),  the
       executive  (_d_x_e_x_e_c), the Data Prompter, the Module Builder
       or the Tutorial.

SSYYNNOOPPSSIISS
       dx   [ -8bitcmap < private | shared | 0-1 > ]
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

       dx   -prompter      ( start the Data Prompter )
            [ -full ]
            [ -file ] [ < header file name > ]

       dx   -builder  ( start the Module Builder )
            [-file ] [ < .mb file name > ]







DDEESSCCRRIIPPTTIIOONN
       _d_x is the command that starts the  Data  Explorer  Startup
       Window,  and  optionally  starts  the  _D_a_t_a  _E_x_p_l_o_r_e_r _U_s_e_r
       _I_n_t_e_r_f_a_c_e, _D_a_t_a _E_x_p_l_o_r_e_r  _E_x_e_c_u_t_i_v_e,  _D_a_t_a  _E_x_p_l_o_r_e_r  _D_a_t_a
       _P_r_o_m_p_t_e_r,   _D_a_t_a  _E_x_p_l_o_r_e_r  _M_o_d_u_l_e  _B_u_i_l_d_e_r  or  the  _D_a_t_a
       _E_x_p_l_o_r_e_r _T_u_t_o_r_i_a_l.

       The Data Explorer Startup Window  is  an  X  windows/Motif
       based  application  which  allows easy access to the other
       subsystemes of the Data Explorer visualization system.

       The _D_a_t_a _E_x_p_l_o_r_e_r _U_s_e_r _I_n_t_e_r_f_a_c_e  is  an  X  windows/Motif
       based application that provides a visual programming envi-
       ronment and an interface to the executive.  The  executive
       provides  the computational tools and mechanisms to create
       data visualizations.  The

       _D_a_t_a _E_x_p_l_o_r_e_r _D_a_t_a _P_r_o_m_p_t_e_r is an  X  windows/Motif  based
       application  that  helps users import data in a variety of
       formats, and provides general purpose visual  programs  to
       visual  imported  data,  leading  to  a shortened "time to
       first picture".

       The _D_a_t_a _E_x_p_l_o_r_e_r _M_o_d_u_l_e _B_u_i_l_d_e_r  is  an  X  windows/Motif
       based application that helps users build their own modules
       to be included in a custom version of Data Explorer.

       The _D_a_t_a _E_x_p_l_o_r_e_r _T_u_t_o_r_i_a_l is an  application  that  helps
       users learn to use Data Explorer by leading them through a
       sequence of tasks.  Users can easily skip  around  in  the
       tutorial  in  order to concentrate on tasks of most use to
       their work.

       For a more complete description of the _D_a_t_a _E_x_p_l_o_r_e_r  _U_s_e_r
       _I_n_t_e_r_f_a_c_e  _a_n_d  _E_x_e_c_u_t_i_v_e  see  the IIBBMM VViissuuaalliizzaattiioonn DDaattaa
       EExxpplloorreerr UUsseerr''ss GGuuiiddee.  For a more complete description of
       the  _D_a_t_a _E_x_p_l_o_r_e_r _D_a_t_a _P_r_o_m_p_t_e_r see the IIBBMM VViissuuaalliizzaattiioonn
       DDaattaa EExxpplloorreerr  QQuuiicckkSSttaarrtt  GGuuiiddee.   For  a  more  thorough
       description  of  the  _D_a_t_a _E_x_p_l_o_r_e_r _M_o_d_u_l_e _B_u_i_l_d_e_r see the
       IIBBMM VViissuuaalliizzaattiioonn DDaattaa EExxpplloorreerr PPrrooggrraammmmeerr''ss RReeffeerreennccee.

OOPPTTIIOONNSS
       With no options the _d_x command starts the Startup  Window,
       an  X  windows/Motif program from which the other parts of
       the Data Explorer system can be started.  The various com-
       mand  line  options and environment variables listed below
       can be specified to change the default behavior.

       Options set by including them in  the  DXARGS  environment
       variable are placed at the start of the command line, fol-
       lowed by any options set by  environment  variables,  fol-
       lowed  by explicit command line arguments.  When conflict-
       ing options are given, the last option has precedence.

       --88bbiittccmmaapp < private | shared | 0-1  | -1 >
              Overrides the DX8BITCMAP environment variable  (see
              ENVIRONMENT VARIABLES below).

       --ccaacchhee < on | off >
              Turn  on or off the executive caching system.  When
              the Data Explorer caching mechanism is enabled  the
              Data  Explorer  executive  automatically caches the
              intermediate results generated during  its  visual-
              ization   computations.   Unlike  standard  caching
              mechanisms, these results are accessed by means  of
              their functional derivation.  This allows for auto-
              matic graph optimization and  helps  to  avoid  the
              recomputation of previously utilized intermediates,
              especially when  the  Data  Explorer  sequencer  is
              being  used.   In certain rare instances, a machine
              with a small amount of memory may want  to  disable
              this  caching to conserve memory and reduce paging.
              Default = on.

       --ccoolloorrss < colors file >
              Override DXCOLORS environment variable  (see  ENVI-
              RONMENT VARIABLES below).

       --ccoonnnneecctt < host:port >
              Start  a distributed executive only (no user inter-
              face).

       --ddaattaa < path list >
              Override DXDATA environment variable (see  ENVIRON-
              MENT VARIABLES below).

       --ddiirreeccttoorryy < dir >
              Cause  the  executive to be run in the given direc-
              tory.

       --ddiissppllaayy < display name >
              Set the name of the X display to  be  used  by  the
              user interface and the executive.

       --ddxxrroooott < dir >
              Overrides the default dx root directory.

       --eecchhoo < dir >
              Echo the command lines without executing them.

       --eeddiitt  Cause  the  user  interface  to be started with the
              Visual Program Editor (VPE) window as the top level
              window  of the user interface (the top level window
              is denoted by an anchor in the upper left hand cor-
              ner).  Also see -image, -startup,  and -menubar.

       --eennccooddee < filename >
              Encode  the  given file into a binary format with a
              key (which must be specified with the -key option).
              For example
                  dx  -encode  -key  1234567890abcdef foo.net The
              resulting file can only be encoded by the  DX  user
              interface when the same key is used. For example
                  dx -image -key 1234567890abcdef foo-e.net

       --eexxeecc < executive executable >
              Use  the given file as a replacement for the execu-
              tive.  This option allows a user to build  and  use
              his/her own custom executive.

       --eexxeeccoonnllyy
              Start the executive in remote mode in which it will
              then await a connection from  the  user  interface.
              The user interface is not started with this option.
              Also see -script.

       --eexxeeccuuttee
              Execute once after the user interface  has  made  a
              connection with the executive.

       --eexxeeccuuttee__oonn__cchhaannggee
              Cause  Data  Explorer  to go into execute-on-change
              mode at start up.   Execute-on-change  mode  causes
              the  Visual  Program to be executed any time any of
              the values (i.e.  module  inupts,  interactor  val-
              ues...)  in the program change.

       --ffiillee < filename >
              Start  the  Data  Prompter, Module Builder, or Data
              Explorer User Interface with the header file  file-
              name.

       --ffuullll  When  used  with  -prompter,  starts  the full Data
              Prompter.

       --hheellpp  Print a brief listing of options and usage for  _d_x.

       --hhiigghhlliigghhtt < on | off >
              Turn  on  or off module execution highlighting.  If
              highlighting is turned on, the user interface  will
              highlight each module as it is executed.  Default =
              on.

       --hhoosstt < host name >
              Override DXHOST environment variable (see  ENVIRON-
              MENT VARIABLES below).

       --hhwwrreennddeerr < gl | opengl >
              Override  the  default hardware rendering choice on
              platforms where  both  are  supported.  Default  is
              opengl.  Sets  the  DXHWMOD environment variable to
              either DXhwdd.o or DXhwddOGL.o.

       --iimmaaggee Cause the user interface to  be  started  with  the
              image  window  as  the top level window of the user
              interface.  Also, control panels that  are  config-
              ured  to  open  automatically are opened.  Also see
              -edit, -startup and -menubar.

       --iinncclluuddee < path list >
              Override DXINCLUDE environment variable (see  ENVI-
              RONMENT VARIABLES below).

       --kkeeyy < 64bit hex >
              16  character  hexidecimal  (64bit)  number that is
              used to encode and decode .net files. See  -encode.

       --lliicceennssee < type >
              Force a specific license for either the user inter-
              face or executive.  The following are valid  values
              for type:

              runtime:  forces  a  run-time license (UI or execu-
              tive).

              timed: forces a timed  (limited  function)  license
              (UI only).

              develop:  forces  a developer license (UI or execu-
              tive).

       --llooccaall Override the DXHOST environment variable  (if  set)
              and  cause  the  executive  to  be run on the local
              host.  In the absence  of  the  DXHOST  environment
              variable, this is the default.

       --lloogg < on | off >
              Turn on or off executive logging.  If logging is on
              then the executive logs output to the  file  _d_x_._l_o_g
              in  the directory where the executive is executing.
              Default = off.

       --mmaaccrrooss < path list >
              Override the  DXMACROS  environment  variable  (see
              ENVIRONMENT VARIABLES below).

       --mmeennuubbaarr
              Start  Data  Explorer  with a menubar as the anchor
              window.  Also, control panels that  are  configured
              to open automatically are opened.  See also -image,
              -edit, and -startup.

       --mmddff < mdf file >
              Override the DXMDF environment variable (see  ENVI-
              RONMENT VARIABLES below).

       --mmeemmoorryy < # of MB >
              Set  the  maximum  amount  of  memory, in megabytes
              rounded up to a two  megabyte  boundary,  that  the
              executive  is  able  to  allocate.   Default  = the
              amount of physical memory minus 8 MB.

       --mmeettrriicc
              Have the ui use metric  units  when  possible,  for
              example  for  setting  the  image size for printing
              postscript images.

       --mmoodduulleess < pathlist >
              Set the list of directories to be searched for out-
              board modules.

       --mmoorreehheellpp
              Displays this information

       --nnooddeeiidd
              print  out the node id of the machine that the com-
              mand is run on.  The node id is used when  generat-
              ing a license.

       --ooppttiimmiizzee < memory | precision >
              Select  whether to minimize memory usage or produce
              more color-accurate images.  When memory  is  opti-
              mized,   image   objects   are  generated  with  24
              bits/pixel instead of 96, and ReadImage  will  pro-
              duce  delayed color images if supported by the for-
              mat. The default is precision.

       --oouuttbbooaarrddddeebbuugg
              Enable user to start outboard modules manually  for
              debugging when they are called in a visual program.

       --pprroocceessssoorrss
              For  Data  Explorer  SMP,  sets   the   number   of
              processors to use.

       --pprrooggrraamm < programname >
              Start  the  user  interface with the visual program
              filename.

       --pprroommpptteerr
              Start the data prompter.

       --rreeaaddaahheeaadd < on | off >
              Specify whether or not the executive reads ahead in
              its  input  stream before executing the commands it
              receives from the stream.  If this is  turned  off,
              then  the  executive will execute each command line
              as it is received.   This  is  useful  for  certain
              debugging  purposes, such as memory leak detection.
              Default = on.

       --ssccrriipptt [ < script file > ]
              Run just the executive in script mode.  If  a  file
              name  is provided then the executive reads and exe-
              cutes commands from the given script and then  ter-
              minates.   If  a file is not provided then when the
              executive starts  up  it  enters  _i_n_t_e_r_a_c_t_i_v_e  mode
              where it waits for script commands.  Also see -exe-
              conly.

       --ssttaarrttuupp
              Start Data Explorer with the startup window as  the
              initial  window.  This  is  the  default.  See also
              -image, -edit, and -menubar.

       --ssuupppprreessss
              Do not open any control panels at start-up. This is
              used in -image mode and -menubar mode only.

       --ttiimmeedd Start DX using a timed license.

       --ttiimmiinngg < on | off >
              Turn  on or off the executive timing mechanism.  If
              enabled, then when the executive exits it will list
              on  standard  output  or  the  logging  file timing
              statistics for each module that was used during the
              course of execution.  Default = off.

       --ttrraaccee < on | off >
              Turn on or off the executive trace mechanism.  When
              tracing is enabled the executive displays  informa-
              tive  messages  about  the  execution  sequence  on
              either standard output or the log file  if  logging
              is enabled.  Default = off.

       --ttrriiaallkkeeyy
              Determines the information necessary for generating
              a trialkey.

       --ttuuttoorr Start the Data Explorer tutorial.

       --uuiioonnllyy
              Cause the user interface to be started without mak-
              ing a connection to an executive.

       --vveerrbboossee
              Echo  commands  in  the  _d_x script as they are exe-
              cuted.

       --vveerrssiioonn
              Print version numbers of various executables.

EENNVVIIRROONNMMEENNTT VVAARRIIAABBLLEESS
       The following environment variables provide  an  alternate
       way  to  set certain of the command line options, or alter
       the default behavior of Data Explorer in some way. Options
       set  with environment variables are overridden if the same
       option is also specified on the command line.  Options set
       with  DXARGS  are overridden by both environment variables
       and explicit command line options.

       DDXX88BBIITTCCMMAAPP
              When Data Explorer displays images  in  8-bit  win-
              dows,  it  makes every effort to create an accurate
              representation of the image using the default  col-
              ormap  shared  among  applications using 8-bit win-
              dows.  If the colors required by Data Explorer  are
              not  present  in  the  colormap  and  there are not
              enough free color cells  available  in  the  shared
              color  map for their allocation, Data Explorer will
              find  approximations  among  the  colors  that  are
              available.   However, in certain circumstances, the
              difference  between  the  desired  color  and   the
              approximation  is  found  to  be  too  great  to be
              acceptable.  In such circumstances, DX  will  allo-
              cate  a private colormap for more accurate results.
              In this case, depending on your X server,  you  may
              need  to move the mouse to select the Image or Dis-
              play window in order  for  the  correct  colors  to
              appear.  The -8bitcmap flag sets the level at which
              the change to using a  private  colormap  is  made.
              The  value  of  this  parameter  should be a number
              between 0 and 1, or "private" or "shared",  and  it
              repressents  the  Euclidean  distance  in RGB color
              space, normalized to 1,  for  the  maximum  allowed
              discrepancy.   If you set this parameter to 1, then
              a private colormap will never be used. If  you  set
              it  to  -1,  then a private colormap will always be
              used. The default is 0.1.

       DDXXAARRGGSS This contains the default set of arguments used  by
              the  _d_x  script.   The  arguments  are prepended to
              those given to _d_x on the command line.  This  vari-
              able  allows  for  the _d_x command to be easily cus-
              tomized.

       DDXXAAXXEESSMMAAXXWWIIDDTTHH
              Sets the number of digits in axes  tick  labels  at
              which  a switch to scientific notation is made. The
              default is 7.

       DDXXCCOOLLOORRSS
              This contains the name of an alternate   file  that
              contains  mappings  of  color  names to red, green,
              blue triplets in a format like that of the  default
              color file found in /usr/lpp/dx/lib/colors.txt.

       DDXXDDAATTAA This contains a colon separated list of directories
              in which the executive will search for data  files.
              Also see the -data option.

       DDXXDDEELLAAYYEEDDCCOOLLOORRSS
              If  set to anything other than 0, enables ReadImage
              to create delayed color images if the  image  is  a
              tiff  format  image  saved  in a byte-with-colormap
              format or a gif image.

       DDXXEEXXEECC Specifies an executive to be run at start-up.

       DDXXFFLLIINNGG
              If DXFLING is set to 1, then for  hardware-rendered
              images,  in  rotation  mode  and  execute-on-change
              mode, if you drag the mouse across the  image,  and
              release  the  mouse  button  outside the image, the
              object in the image will begin to rotate, and  will
              continue  to  rotate  until  you  click  inside the
              image.  The direction and speed of the mouse motion
              before  release  will affect the rotation direction
              and rotation speed of the object in the window.

       DDXXGGAAMMMMAA
              Sets the  gamma  correction  for  software-rendered
              images  displayed  to  the screen using the Display
              and Image tools.  The default value  is  2,  except
              for 8-bit windows on the sgi architecture.

       DDXXGGAAMMMMAA__88BBIITT,,
              DDXXGGAAMMMMAA__1122BBIITT,, DDXXGGAAMMMMAA__2244BBIITT sets the gamma correc-
              tion factor for software-rendered  images  for  the
              windows with the indicated window depth.  Overrides
              the value set by DXGAMMA.

       DDXXHHOOSSTT This contains the name of  the  host  machine  that
              will  run the executive.  If this is not set then a
              default of "localhost" is used,  unless  the  local
              host  is a PVS console in which case the default is
              to run the executive on  the  PVS.   Also  see  the
              -host, -local and -server options.

       DDXXHHWWGGAAMMMMAA
              Sets  the  gamma  correction  for hardware-rendered
              images displayed to the screen  using  the  Display
              and Image tools.  The default value is 2.

       DDXXHHWWMMOODD
              Specifies   the  name  of  the  hardware  rendering
              library to use when more  than  one  is  supported.
              Should be either DXhwdd.o or DXhwddOGL.o.

       DDXXIINNCCLLUUDDEE
              This contains a colon separated list of directories
              in which the executive will search  for  "included"
              scripts.  Also see the -include option.

       DDXXMMAACCRROOSS
              This contains a colon separated list of directories
              in which the user  interface  searches  for  macros
              that will be automatically loaded.  When running in
              script mode one must "include" all  the  macros  by
              hand  prior  to  referencing  them.   Also  see the
              -macros option.

       DDXXMMDDFF  This contains the name of a module description file
              (mdf) that contains user defined modules for a cus-
              tomized version of the  executive.   Also  see  the
              -mdf option.

       DDXXMMEEMMOORRYY
              sets  the  amount of memory (in megabytes) that can
              be used by the executive.

       DDXXMMOODDUULLEESS
              This contains a colon separated list of directories
              in  which  the executive searches for outboard mod-
              ules.  Also see the -modules option.

       DDXX__NNEESSTTEEDD__LLOOOOPPSS
              For faces, loops, and edges data,  if  set,  allows
              loops  other  than the enclosing loop for a face to
              be listed first. However,  there  is  a  consequent
              decrease  in  performance if this environment vari-
              able is set.

       DDXXNNOO__BBAACCKKIINNGG__SSTTOORREE
              If set to anything, disables framebuffer readbacks.
              Setting  this environment variable may improve per-
              formance  of  interaction  with  hardware  rendered
              images,  especially for machines for which readback
              is slow. The default behavior is  that  framebuffer
              readbacks  are enabled. If you set this environment
              variable, not  that  some  interactions  using  the
              Image  tool  will  result  in  a  black image while
              interaction is taking place. However,  if  you  are
              not  using  the  Image  tool, it is strongly recom-
              mended that you set this environment variable.

       DDXXPPIIXXEELLTTYYPPEE
              sets the image type to either 24-bit  color  images
              or    floating-point-based   96-bit   images   (the
              default). This affects the behavior of  Render  and
              ReadImage.   This  variable  can  be  set to either
              DXByte (24 bits) or  DXFloat  (96  bits).   Setting
              this  variable to DXByte will result in images tak-
              ing up less memory.

       DDXXPPRROOCCEESSSSOORRSS
              Sets the number of processors to be used  for  Data
              Explorer SMP.

       DDXXRRSSHH  Specifies the remote shell command to use when con-
              necting to other systems (such as those when  using
              distributed execution). The default is the the path
              to rsh.

       DDXXRRSSHH__NNOOEENNVV
              Prevents the remote  "invoke  dxexec"  script  from
              being  written with a full carbon copy of the local
              environment (only DISPLAY is set).

       DDXXRROOOOTT Specifies the top-level directory for all the files
              and directories needed by Data Explorer.

       DDXXSSHHMMEEMM
              If  set  to  anything  other than -1, forces shared
              memory to be used (if allowed by the architecture).
              If set to -1, (if allowed by the architecture) will
              cause the  data  segment  to  simply  be  extended.
              Please  see  Appendix  C  of the Data Explorer User
              Guide for more information.

       DDXX__SSOOCCKKEETT__BBUUFFSSIIZZEE
              Allows the user to specify the size of the send and
              recieve  buffers  for the socket connection between
              the UI and the exec. This value is clamped  to  the
              max  allowed  by the system. This can fix a problem
              where the executive and UI hang involving  a  dead-
              lock  situation  in  the  socket communication. The
              value assigned to it is passed as the parameter  to
              setsockopt for SOL_SNDBUF and SOL_RCVBUF. A typcial
              increased value may be 262144.

       DDXX__UUSSEERR__IINNTTEERRAACCTTOORR__FFIILLEE
              Specifies a file containing  user  interactors  for
              use  by the SuperviseState and SuperviseWindow mod-
              ules.

       DDXX__WWEEBB__BBRROOWWSSEERR
              Under UNIX specifies the web browser to launch  for
              viewing  the  HTML documentation. Under Windows and
              MacOS X  setting  this  environment  variable  will
              launch the systems default web browser.

EEXXAAMMPPLLEESS
       The  next few examples assume that none of the DX environ-
       ment variables have been set.

       1. To start both the user interface and the executive with
       the network named _c_f_d_._n_e_t:
              dx -edit -program cfd


       2.  To  start  and  execute  the visual program _c_f_d_._n_e_t in
       image mode with /usr/users/me and /usr/data as data direc-
       tories:
              dx  -image -execute -data '/usr/users/me:/usr/data'
              cfd

       3. To run just the executive on the script named _f_1_5_w_i_n_g:

              dx -script -file f15wing

       4. To start the user interface and execute the  user  cus-
       tomized  executive  named _m_y_d_x_e_x_e_c containing user defined
       modules described in _c_u_s_t_o_m_._m_d_f:

              dx -edit -mdf custom.mdf -exec mydxexec

       5. To start the Data Prompter :

              dx -prompter

       6. To start the Module Builder :

              dx -builder
       7. To start the Tutorial :

              dx -tutorial


SSEEEE AALLSSOO
       README* - The README file(s) found in the dx
              installation  directory  are  an  added  source  of
              information.   html/allguide.htm contains html ver-
              sions of the documentation.



IBM Visualization Data Explorer                            DX(1V)
