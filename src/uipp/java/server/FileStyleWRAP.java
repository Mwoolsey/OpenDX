//
package server;

import java.net.*;
import java.io.*;
import java.util.*;


public class FileStyleWRAP extends FileStyle {
    protected Vector extensions;
    protected String base_name;
    protected FileStyleWRAP (String file_name) { 
	super(file_name); 
	int extid = file_name.lastIndexOf((int)'.');
	base_name = file_name.substring(0, extid);
    }
    public void delete() {
	super.delete();
	//
	// Delete the associated file(s)
	//
	Enumeration enum1 = extensions.elements();
	while (enum1.hasMoreElements()) {
	    String ext = (String)enum1.nextElement();
	    String path = base_name + "." + ext;
	    File df = new File(path);
	    if (df.exists()) df.delete();
	}
    }
} // end FileStyleWRAP
