//
package server;

import java.net.*;
import java.io.*;
import java.util.*;


public class FileStyleHTML extends FileStyleWRAP implements FilenameFilter {
    public FileStyleHTML (String file_name) { 
	super(file_name); 
	extensions = new Vector(2);
	extensions.addElement((Object)"wrl");
    }
    public void delete() {
	String filename = getFileName();
	File dir = null;
	try {
	    File us = new File(filename);
	    dir = new File(us.getParent());
	} catch (Exception e) {
	}
	super.delete();

	//
	// Now go back and catch the remaining ?_?_?.wrl files by
	// using a wildcard.
	//
	try {
	    String[] to_erase = dir.list(this);
	    int cnt = to_erase.length;
	    int i;
	    for (i=0; i<cnt; i++) {
		File df = new File(dir, to_erase[i]);
		if (df.exists())
		    df.delete();
	    }
	} catch (Exception e) {
	    System.err.println ("DXServer warning: unable to clean up " + filename);
	}
    }

    //
    // ?_?.wrl, ?_?_?.wrl
    //
    // name is just a file without full path
    // match is a full path.
    //
    public boolean accept(File dir, String name) {
	boolean retval = false;
	String match = this.getFileName();

	try {
	    int match_start = match.lastIndexOf((int)File.separatorChar);
	    int match_ext = match.lastIndexOf((int)'.');
	    int name_ext = name.lastIndexOf((int)'.');

	    String name_ext_str = name.substring(name_ext);

	    if (name_ext_str.equals(".wrl")) {
		String match_str = match.substring(match_start+1, match_ext);
		retval = name.startsWith(match_str);
	    } else {
		retval = false;
	    }
	} catch (Exception e) {
	}
	return retval;
    }

} // end FileStyleHTML
