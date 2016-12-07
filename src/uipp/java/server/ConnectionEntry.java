//
package server;
import java.net.*;
import java.io.*;
import java.util.*;
import dx.protocol.server.*;

public class ConnectionEntry extends Object
{
    private Long dxlcon;
    private Hashtable users;
    private static int next_id = 100;
    private boolean using; 
    private int idle_time;
    private int user_time;
    private long lock_time;
    private static final long SecondScale = (new Date()).getTime() / 1000;
    private boolean failed;

    public boolean isFailed() { return this.failed; }

    private synchronized static int NextID() { return ConnectionEntry.next_id++; }

    //
    // Key in the Hashtable is the pathname, Object is a Vector of 
    // replaced names.
    //
    private Hashtable available_programs;
    private Hashtable inuse_programs;

    public Long getDXLConnection() { return dxlcon; }
    public int getUserCount() { return this.users.size(); }
    public int getUserTime() { return this.user_time; }

    public int getIdleTime() { 
	int now = (int)((((new Date()).getTime()) / 1000) - ConnectionEntry.SecondScale);
	return now - this.idle_time; 
    }

    public ConnectionEntry() { 
	super(); 
	this.dxlcon = null; 
	this.users = new Hashtable(10, 4);
	this.available_programs = new Hashtable (10,4);
	this.inuse_programs = new Hashtable (10,4);
	this.using = false;
	this.idle_time = 0;
	this.user_time = 0;
	this.lock_time = 0;
    }
    public ConnectionEntry(Long l) {
	super();
	this.dxlcon = l;
	this.users = new Hashtable(10, 4);
	this.available_programs = new Hashtable (10,4);
	this.inuse_programs = new Hashtable (10,4);
	this.using = false;
	this.idle_time = 0;
	this.user_time = 0;
	this.lock_time = 0;
    }


    public String getNetName(DXServerThread dxst) {
	return (String)this.users.get(dxst);
    }

    public void attachUser (DXServerThread dxst, String path) {
	try {
	    this.users.put(dxst, this.useNet(path, dxst.getIdDX()));
	    this.idle_time = 0;
	} catch (FileNotFoundException fnfe) {
	    System.out.println ("ConnectionEntry: file not found: " + path);
	    fnfe.printStackTrace();
	} catch (IOException ioe) {
	    System.out.println ("ConnectionEntry: io error: " + path);
	}
    }

    //
    // Delete munged nets from the file system.  This used to be
    // finalize(), but the garbage collector is very unpredictable
    // and I want to avoid leaving files around.
    //
    public void deleteFiles() {
	Enumeration enum1 = this.available_programs.elements();
	while (enum1.hasMoreElements()) {
	    Vector replacements = (Vector)enum1.nextElement();
	    Enumeration renum = replacements.elements();
	    while (renum.hasMoreElements()) {
		String munged_net = (String)renum.nextElement();
		try {
		    File mf = new File(munged_net);
		    mf.delete();
		} catch (Exception e) {
		}
	    }
	}
	this.available_programs.clear();
    }

    public void detachUser(DXServerThread dxst) {
	if (dxst.isFailed()) this.failed = true;
	this.unUseNet((String)this.users.remove(dxst));
	if (this.users.size() == 0) {
	    this.idle_time = (int)((((new Date()).getTime()) / 1000) - 
		ConnectionEntry.SecondScale);
	}
    }

    private void unUseNet(String path) {
	String original_path = (String)this.inuse_programs.remove(path);
	Vector replacements = (Vector)this.available_programs.get(original_path);
	replacements.addElement(path);
    }

    private String useNet(String path, int id) 
	throws java.io.FileNotFoundException , java.io.IOException {
	Vector replacements = (Vector)this.available_programs.get(path);

	//
	// The .net has never been loaded before
	//
	if (replacements == null) {
	    replacements = new Vector(4);
	    this.available_programs.put(path, replacements);
	}

	if (replacements.size() == 0) {
	    replacements.addElement(this.munge(path));
	}

	String replacement = (String)replacements.elementAt(0);
	replacements.removeElementAt(0);
	this.inuse_programs.put(replacement, path);

	return replacement;
    }

    private String munge (String path) 
	throws java.io.FileNotFoundException , java.io.IOException {
	String newid = "_" + ConnectionEntry.NextID();

	//
	// Make a duplicate of path changing only the filename
	//
	File original = new File(path);
	String parent = null;
	parent = original.getParent();
	//
	// getParent() fails on the pc when we're using UNIX
	// style slashes to separate names, so we have to 
	// go looking for the dir name by hand
	//
	boolean replaced = false;
	if ((parent == null) && (File.separatorChar == '\\')) {
	    String fake_path = path.replace ('/', '\\');
	    if (fake_path != path)
		parent = (new File(fake_path)).getParent();
	    replaced = true;
	}
	if (parent == null) parent = ".";
	String newpath = parent + "/" + newid + ".net";
	if (replaced)
	    newpath = newpath.replace('\\', '/');
	BufferedReader bis = new BufferedReader(new FileReader(original));
	(new File(newpath)).delete();
	BufferedWriter bos = new BufferedWriter(new FileWriter(new File(newpath)));

	//
	// Look for MODULE
	// having found it replace its arg with newid and then
	// replace all subsequent occurrences of it with newid as well.
	//
	String module_name = null;
	String outLine = null;
	String inLine = null;
	String modid = "// MODULE ";
	while ((inLine = bis.readLine()) != null) {
	    if ((module_name == null) && (inLine.startsWith(modid))) {
		module_name = inLine.substring(modid.length());
		module_name.trim();
		outLine = modid + newid + "\n";
	    } else {
		outLine = inLine + "\n";
		if (inLine.startsWith("//")) {
		} else if (inLine.startsWith("    //")) {
		} else if (module_name != null) {
		    int ofs;
		    while ((ofs = outLine.indexOf(module_name)) != -1) {
			String tmp = outLine.substring (0, ofs) + newid + 
			    outLine.substring(ofs + module_name.length());
			outLine = tmp;
		    }
		}
	    }
	    bos.write (outLine, 0, outLine.length());
	    bos.flush();
	}

	bis.close();
	bos.close();
	return newpath;
    }

    //
    // Implement a critical section around DXLExecuteOnce()
    //
    public synchronized void getConnectionLock() {
	while (lock_available() == false) try { wait(); } catch (Exception e) {}
	this.lock_time = (new Date()).getTime();
    }

    private synchronized boolean lock_available() {
	if (using == true) return false;
	return (using = true);
    }

    public synchronized void releaseConnectionLock() {
	using = false;
	notify();
	if (this.lock_time > 0) {
	    long unlock = (new Date()).getTime();
	    this.user_time+= unlock - this.lock_time;
	    this.lock_time = 0;
	}
    }

} // end ConnectionEntry
