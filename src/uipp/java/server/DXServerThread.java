//
package server;
import java.net.*;
import java.io.*;
import java.util.*;
import dx.protocol.*;

public class DXServerThread extends DXThread
{
    private Long dxlconnection;
    private int java_id;
    private String java_dir = null;
    private String url = null;
    private Vector shown = null;
    private int latest_sequence = -1;
    private String java_version;

    //
    // Track clients who want our messages
    //
    private Vector frame_threads = null;

    //
    // Keep track of the execution state we've requested from dxlink
    // using DXL{ExecuteOnce,ExecuteOnChange,EndExecuteOnChange}
    //
    private final static int NotLoaded = 0;
    private final static int NotExecuting = 1;
    private final static int ExecuteOnChange = 2;
    private final static int Executing = 3;
    private int exec_state = DXServerThread.NotLoaded;
    private Vector setvalues = null;

    //
    // control (re)reading of the path file
    //
    private static String PathFileUsed = null;
    private static long   PathFileModified = 0;
    private static Vector PathNames = null;

    private static int    RecentOperation;

    private static boolean Initialized = false;

    public final static int DISCONNECT = 1;
    public final static int EXECUTE = 2;
    public final static int SENDVALUE = 3;
    public final static int SETFILECNT = 4;
    public final static int LOAD = 5;
    public final static int SETUIVERSION = 7;
    public final static int ENDEXECUTION = 8;
    public final static int NOOP = 12;
    public final static int SEQUENCE = 13;

    private final static int START = 1;
    private final static int EXIT = 2;
    private final static int CLEANUP = 3;

    private native int DXLExecuteOnce(long dxl, String macro_name);
    private native int DXLHandlePendingMessages(long dxl);
    private native int DXLLoadVisualProgram(long dxl, String net_file);

    //
    // Locate the arch-specific shared library.  If the java-reported arch
    // name isn't recognizable, then assume cwd.  How do I know this will
    // be called only once?  ...and does it need to be synchronized?
    //
    static { 
	//
	// os.name is something like "Windows NT"
	// os.arch is something like "x86" or "80486"
	// If the server is started with -Djx.lib set
	//    then we try to use System.load() which allows a path specification
	//    where jx.lib is set to the full path and library name. for some archs we
	//    have a default for jx.lib which might work but increasingly does not.
	//   Otherwise or in case of failure, then we'll try to load the library from the current wd
	//    or based on the library search path using System.loadLibrary() .
	//
	boolean use_loadLibrary = true;
	String jxlib = System.getProperty("jx.lib");
	if (jxlib != null) {
	    try {
	    	String arch = System.getProperty("os.name");
	    	if (jxlib.length()==0) {
			if (arch.equals("AIX")) 
				jxlib ="../lib_ibm6000/libAnyDX.so";
			if (arch.equals("Solaris")) 
				jxlib ="../lib_solaris/libAnyDX.so";
	   		if (arch.equals("Irix")) 
				jxlib ="../lib_sgi/libAnyDX.so";
	   	} 
		if (jxlib.length()!=0) {
			System.load(jxlib);
			use_loadLibrary = false;
	    	} 
	   } catch (UnsatisfiedLinkError ule) { 
		System.out.println(" ");
		System.out.println("DXServer: normal library loading is suppressed because jx.lib is set.");
		System.out.println("However DXServer is unable to load AnyDX shared lib from file "+ jxlib);
		System.out.println("Put correct system-dependent path and name in -Djx.lib (startserver script?) ");
		System.out.println("Alternatively remove the jx.lib definition entirely and normal library loading will be the default ");
		System.out.println("We will now try to load the library normally via System.loadLibrary() .");
		use_loadLibrary=true;
	   }
	}
	if (use_loadLibrary)
	    try {
		System.loadLibrary("AnyDX");
	    } catch (UnsatisfiedLinkError ule) {
		System.out.println("Unable to load AnyDX shared library: is PATH or LD_LIBRARY_PATH correct?");
		System.out.println("Try setting -Djx.lib to the full name including path in startserver script");
		if(System.getProperty("os.name").equals("Irix"))
			System.out.println("Irix platform: if library exists then possibly its abi is not -n32 ");
		System.exit(1);
	    }
    }

    public  int     getIdDX() 			{ return java_id; }
    public  Long    getDXLConnection()		{ return dxlconnection; }
    public  void    executionFinished() 	{ transmit (new doneMsg()); }

    private long    getConnection() { 
	if (this.dxlconnection == null) return 0;
	return dxlconnection.longValue(); 
    }




//
//
public void fileNotify(String file_name) 
{
    boolean found = false;
    Enumeration enum1 = shown.elements();
    while (enum1.hasMoreElements()) {
	FileStyle fs = (FileStyle)enum1.nextElement();
	String shown_file = fs.getFileName();
	if (shown_file.equals(file_name) == true) {
	    found = true;
	    break;
	}
    }
    if (found == false) {
	FileStyle nfs = Allocator(file_name);
	if (nfs.isSequenced()) {
	    int seq = nfs.getSequence();
	    if (seq > latest_sequence) {
		enum1 = shown.elements();
		FileStyle ofs;
		Vector dying = new Vector(4);
		while (enum1.hasMoreElements()) {
		    ofs = (FileStyle)enum1.nextElement();
		    int sequence = ofs.getSequence();
		    if (sequence <= latest_sequence)
			dying.addElement((Object)ofs);
		}
		enum1 = dying.elements();
		while (enum1.hasMoreElements()) {
		    ofs = (FileStyle)enum1.nextElement();
		    shown.removeElement((Object)ofs);
		    ofs.delete();
		}
		enum1 = null;
		dying.removeAllElements(); 
	    }
	    latest_sequence = seq;
	}
	shown.addElement((Object)nfs);
    }
}


//
// Check for embedded newlines
//
// Check for a message of type:
//    0:  ECHO:  Image_%d ... <tag>=<value>
// show=%s is special.  It's a notification that an image file has been
// written and requires extra treatment.
// The %s will be a filename.  We're on the hook for removing this file after use.
//
// N.B.  The pc doesn't send the 0: portion of the message
//
private boolean isImageNotify(String s)
{
    String pcStr = "ECHO:  ";
    String unixStr = " 0:  ECHO:  ";
    int nxtChk = -1;
    if (s.startsWith (unixStr) == true) {
	nxtChk = unixStr.length();
    } else if (s.startsWith (pcStr) == true) {
	nxtChk = pcStr.length();
    }

    if (nxtChk == -1) return false;

    int loc = s.substring(nxtChk).indexOf("Image_");
    if (loc == -1) return false;
    if (loc > 6) return false;

    return true;
}
public void infoNotify(String s) 
{
    StringTokenizer stok = new StringTokenizer(s, "\n");
    if (this.isImageNotify(s)) {
	stok = new StringTokenizer (s, ";");
	String msg = null;
	while (stok.hasMoreTokens()) {
	    String tok = stok.nextToken();
	    StringTokenizer ctok = new StringTokenizer(tok, "=");
	    String lhs = null;
	    if (ctok.hasMoreTokens()) lhs=ctok.nextToken();
	    if ((lhs!=null) && (lhs.endsWith("show"))) {
		String path = ctok.nextToken();
		fileNotify(path);

		//
		// Now alter the message so that it reflects the url
		// instead of the complete path name on the host.
		//
		int l = path.lastIndexOf((int)'/');
		String file_name = null;
		if (l == -1)
		    file_name = path;
		else 
		    file_name = path.substring(l+1);
		String url = DXServer.getOutputUrl() + "/" + file_name;
		if (msg == null)
		    msg = lhs + "=" + url + ";";
		else
		    msg = msg + ";" + lhs + "=" + url + ";";
	    } else {
		if (msg == null) msg = tok;
		else msg = msg + ";" + tok;
	    }
	}
	messageMsg to_applet = new messageMsg();
	to_applet.setMessage(msg);
	transmit(to_applet);
    } else if (s.startsWith("Connection to DX broken")) {
	this.setFailed();
	this.executionFinished();
	messageMsg msg = new messageMsg();
	msg.setMessage("Connection to DX broken");
	transmit(msg);
    } else {
	while (stok.hasMoreTokens()) {
	    String tok = stok.nextToken();
	    messageMsg msg = new messageMsg();
	    msg.setMessage(tok);
	    transmit(msg);
	}
    }
}

//
// In lieu of a C++ style Dictionary.
//
public static FileStyle Allocator(String file_name) 
{
    int extid = file_name.lastIndexOf((int)'.');
    String ext = file_name.substring(extid+1);
    if (ext.equals("html")) return new FileStyleHTML(file_name);
    if (ext.equals("htm")) return new FileStyleHTM(file_name);
    return new FileStyleANY(file_name);
}


//
//
public DXServerThread(Socket client, int id)
{ 
    super(client);
    java_id = id; 
    java_dir = null;
    url = null;
    dxlconnection = new Long(0);
    frame_threads = new Vector(10);
    this.setName("DXServerThread " + id);

    //
    // Files for which we've received notification.  We'll normally have
    // between 1 and 4 of these.
    //
    shown = new Vector(4);

    //
    // We'll keep the strings we understand in a dictionary so that
    // the operations we support can be updated happily.
    // These should _not_ be static.  They must be 1 per thread because
    // the Command class has a reference to the thread object
    //
    try {
	actions.addElement ((Object)new ServerThreadCommand
	    (this, stopMsg.GetCommand(), DXServerThread.DISCONNECT));
	actions.addElement ((Object)new ServerThreadCommand
	    (this, execOnceMsg.GetCommand(), DXServerThread.EXECUTE));
	actions.addElement ((Object)new ServerThreadCommand
	    (this, stepSeqMsg.GetCommand(), DXServerThread.SEQUENCE));
	actions.addElement ((Object)new ServerThreadCommand
	    (this, endExecMsg.GetCommand(), DXServerThread.ENDEXECUTION));
	actions.addElement ((Object)new ServerThreadCommand
	    (this, sendValueMsg.GetCommand(), DXServerThread.SENDVALUE));
	actions.addElement ((Object)new ServerThreadCommand
	    (this, loadMsg.GetCommand(), DXServerThread.LOAD));
	actions.addElement ((Object)new ServerThreadCommand
	    (this, noopMsg.GetCommand(), DXServerThread.NOOP));
    } catch (ClassNotFoundException cnfe) {
	cnfe.printStackTrace();
    }

    DXServerThread.ClassInitialize();
    setidMsg sm = new setidMsg();
    sm.setOutputDir(DXServer.getOutputDir());
    sm.setOutputUrl(DXServer.getOutputUrl());
    sm.setJavaId(getIdDX());
    transmit (sm);
}


public synchronized static void ClassInitialize()
{
    if (DXServerThread.Initialized == false) {
	DXServerThread.PathFileUsed = null;
	DXServerThread.PathFileModified = 0;
	DXServerThread.PathNames = new Vector(20);

	DXServerThread.Initialized = true;

	DXServerThread.ReadPathFile();
    }
}

public synchronized boolean sendValue (threadMsg msg)
{
    if (getConnection() == 0) return true;
    boolean send_down_now = 
	(this.exec_state != DXServerThread.ExecuteOnChange) &&
	(this.exec_state != DXServerThread.NotLoaded);
    sendValueMsg svm = (sendValueMsg)msg;

    if (send_down_now) {
	int ret = dxlSend(svm.getValue());
	if (ret == 0)
	    setFailed();
    } else {
	if (this.setvalues == null)
	    this.setvalues = new Vector(20);
	this.setvalues.addElement((Object)msg);
    }

    return true;
}

public void cleanUp()
{
    super.cleanUp();
    if (getConnection() != 0) 
	DXServer.EndConnection(this.getDXLConnection(), this);
    this.dxlconnection = new Long(0);

    Enumeration enum1 = shown.elements();
    FileStyle fs;
    while (enum1.hasMoreElements()) {
	fs = (FileStyle)enum1.nextElement();
	fs.delete();
    }
    shown = null;
}

public synchronized boolean loadProgram(threadMsg msg)
{
    //
    // Load the program
    //
    loadMsg lm = (loadMsg)msg;
    String value = lm.getProgram();

    //
    // an array of search paths
    //
    String[] copy = new String[128];
    synchronized (DXServerThread.PathNames) {
	DXServerThread.PathNames.copyInto(copy);
    }
    int size = copy.length;

    //
    // Look for the net file before requesting the load.  We'll use
    // the path names we've read from our config file: dxserver.paths
    //
    int i = 0;
    String path = null;
    boolean net_file_found = false;
    while ((i<size) && (copy[i] != null)) {
	String dir = (String)copy[i];
	if (dir.endsWith("/"))
	    path = dir + value;
	else
	    path = dir + "/" + value;

	File netf = new File(path);
	if ((netf.exists()) && (netf.isFile()) && (netf.canRead())) {
	    net_file_found = true;
	    break;
	}

	i++;
    }


    int ret = 0;
    if (net_file_found) {
	if (this.getConnection() == 0) 
	    this.dxlconnection = DXServer.NewConnection(this, path);

	if (getConnection() == 0) {
	    messageMsg mm = new messageMsg();
	    mm.setMessage("Error: Unable to start Data Explorer");
	    transmit (mm);
	    return true;
	}

	String net_to_load = DXServer.createNet(path, this);
	DXServer.LockConnection(this);
	ret = this.DXLLoadVisualProgram (getConnection(), net_to_load);
	DXServer.UnlockConnection(this);
	if (ret == 0) {
	    messageMsg mm = new messageMsg();
	    mm.setMessage("Error: Visual program not loaded: " + value);
	    transmit (mm);
	} else {
	    String filename = null;
	    if (File.separatorChar == '\\') {
		String fake_file = net_to_load.replace ('/', '\\');
		filename = ((new File(fake_file)).getName()).replace ('\\', '/');
	    } else {
		filename = (new File(net_to_load)).getName();
	    }
	    int dot = filename.indexOf((int)'.');
	    setmacroMsg smm = new setmacroMsg();
	    smm.setMacroName(filename.substring(0,dot));
	    transmit (smm);

	    this.exec_state = DXServerThread.NotExecuting;
	    if (this.setvalues != null) {
		Enumeration enum1 = this.setvalues.elements();
		while (enum1.hasMoreElements()) {
		    threadMsg buffered = (threadMsg)enum1.nextElement();
		    Enumeration anum = actions.elements();
		    while (anum.hasMoreElements()) {
			ServerThreadCommand stc = (ServerThreadCommand)anum.nextElement();
			if (buffered.getCommand().equals(stc.getCommandString())) {
			    stc.execute(buffered);
			    break;
			}
		    }
		}
		this.setvalues.removeAllElements();
		this.setvalues = null;
	    }
	}
    } else {
	messageMsg mm = new messageMsg();
	mm.setMessage("Error: Visual program not found: " + value);
	transmit (mm);
    }

    return true;
}

public synchronized boolean checkConnection (threadMsg msg)
{
    boolean retval = true;
    DXServer.LockConnection(this);
    DXLHandlePendingMessages (getConnection());
    DXServer.UnlockConnection(this);
    return retval;
}

private void flushQueue() 
{
    if (getConnection() == 0) return ;
    this.exec_state = DXServerThread.NotExecuting;
    if (this.setvalues != null) {
	Enumeration enum1 = this.setvalues.elements();
	while (enum1.hasMoreElements()) {
	    threadMsg buffered = (threadMsg)enum1.nextElement();
	    Enumeration anum = actions.elements();
	    while (anum.hasMoreElements()) {
		ServerThreadCommand stc = (ServerThreadCommand)anum.nextElement();
		if (buffered.getCommand().equals(stc.getCommandString())) {
		    stc.execute(buffered);
		    break;
		}
	    }
	}
	this.setvalues.removeAllElements();
	this.setvalues = null;
    }
}

//
//
//

public synchronized boolean stepSequencer (threadMsg msg)
{
    int retval = 1;
    if (getConnection() == 0) return true;
    this.flushQueue();

    stepSeqMsg ssm = (stepSeqMsg)msg;

    if (retval > 0) {
	DXServer.LockConnection(this);
	DXServer.DXLSend(getConnection(), ssm.getSequencerParams());
	DXServer.DXLSend(getConnection(), ssm.getSequencerFrame());
	retval = DXLExecuteOnce(getConnection(), ssm.getMacroName());
	DXServer.UnlockConnection(this);
    }
    if (retval > 0) this.exec_state = DXServerThread.Executing;
    if (retval == 0) setFailed();

    executionFinished();
    return true;
}

public synchronized boolean executeOnce(threadMsg msg)
{
    int retval = 1;
    if (getConnection() == 0) return true;
    this.flushQueue();

    execOnceMsg eom = (execOnceMsg)msg;

    if (retval > 0) {
	DXServer.LockConnection(this);
	retval = DXLExecuteOnce(getConnection(), eom.getMacroName());
	DXServer.UnlockConnection(this);
    }
    if (retval > 0) this.exec_state = DXServerThread.Executing;
    if (retval == 0) setFailed();

    executionFinished();
    return true;
}

public synchronized int dxlSend(String s)
{
    if (getConnection() == 0) return 1;
    DXServer.LockConnection(this);
    int retval = DXServer.DXLSend(getConnection(), s);
    DXServer.UnlockConnection(this);
    if (retval <= 0) return 0;
    return 1;
}

//
// Read the contents of dxserver.paths
// Look in:
// 1) /etc
// 2) current working dir
// 3) environment variable defined by DXServer.pathsFile
// Don't look for multiple copies of the file.  Stop after reading the first one found.
// N.B. The caller is responsible for locking
//
private static void ReadPathFile()
{
boolean file_found = false;
boolean file_read = false;
Vector paths;

    paths = new Vector(5);
    if (DXServerThread.PathFileUsed != null)
	paths.addElement((Object)DXServerThread.PathFileUsed);
    paths.addElement((Object)File.separator+"etc"+File.separator+"dxserver.paths");
    paths.addElement((Object)"."+File.separator+"dxserver.paths");
    paths.addElement((Object)System.getProperty("DXServer.pathsFile"));

    synchronized (DXServerThread.PathNames) {
	try {

	    Enumeration enum1 = paths.elements();
	    while ((!file_read) && (enum1.hasMoreElements())) {
		String path = (String)enum1.nextElement();
		File hf = new File(path);
		if (hf.exists() == false) continue;
		if (hf.canRead() == false) continue;
		if (hf.isFile() == false) continue;
		file_found = true;

		long modified = hf.lastModified();

		//
		// If we've already read an up-to-date hosts file, then
		// do nothing.
		//
		if ((DXServerThread.PathFileUsed != null) && 
		    (DXServerThread.PathFileUsed.equals(path)) &&
		    (DXServerThread.PathFileModified >= modified))
		    break;
		PathNames.removeAllElements();

		FileReader hfr = new FileReader(hf);
		BufferedReader hbr = new BufferedReader (hfr);
		String search;
		int size = 0;
		while ((size < 128) && ((search = hbr.readLine()) != null))  {
		    if (search.startsWith("//")) continue;
		    if (search.startsWith("!#")) continue;
		    if (search.startsWith("#")) continue;
		    PathNames.addElement((Object)search);
		    size++;
		}
		DXServerThread.PathFileUsed = hf.getPath();
		DXServerThread.PathFileModified = modified;
		file_read = true;
	    }
	} catch (Exception e) {
	    e.printStackTrace();
	}

	if (!file_found) 
	    DXServerThread.PathFileUsed = null;
	if (PathNames.size() == 0) 
	    PathNames.addElement((Object)".");
    }
}


public int hasEndMsg()
{
    boolean has_end = false;
    try {
	String h = new endExecMsg().getCommand();
	String str = this.peekQueue(h, false);
	if (str != null) has_end = true;
    } catch (IOException ioe) {
    }
    if (this.isFailed()) has_end = true;
    return has_end?1:0 ;
}


} // end DXServerThread

