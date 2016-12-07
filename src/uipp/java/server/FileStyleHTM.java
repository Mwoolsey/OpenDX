//
package server;
import java.net.*;
import java.io.*;
import java.util.*;

public class FileStyleHTM extends FileStyleWRAP {
    public FileStyleHTM (String file_name) { 
	super(file_name); 
	extensions = new Vector(2);
	extensions.addElement((Object)"gif");
    }
} // end FileStyleHTML
