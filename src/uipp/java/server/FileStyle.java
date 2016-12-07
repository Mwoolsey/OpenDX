//
package server;

import java.net.*;
import java.io.*;
import java.util.*;

public class FileStyle {

    //
    // The source of this data is a Data Explorer execution.  An assignment
    // is made to a DXLOutput tool.  The value is the name of a file written
    // by Export() or WriteImage().  Regardless of the platform, this always
    // comes back in the form of a unix path name i.e. with '/' separating
    // components of the pathname rather than '\' as one might expect when
    // Data Explorer is running on a pc.  (However, if it does come back
    // with '\' in the path we should still be OK.)
    //
    protected String fileName;

    //
    // If we see a filename like %d_%d_%d.* or %d_%d.%d.*, then we'll
    // assume that the 3rd is actually a sequence number.  We want to
    // be able to supply that sequence number so that files can be erased
    // ahead of time.
    //
    protected boolean sequenced;
    protected boolean sequenceChecked = false;
    protected int sequence_number;

    //
    // We'll change '/' to '\' if we're running on a pc.
    //
    protected FileStyle (String file_name) { 
	fileName = file_name; 
    }

    public void delete () {
	File df = new File(getFileName());
	if (df.exists()) {
	    df.delete();
	}
    }

    //
    // replace '/' with File.Separator
    //
    public String getFileName() { return fileName; }

    public boolean isSequenced() {
	if (sequenceChecked == true) return sequenced;
	sequence_number = -1;
	getSequence();
	if (sequence_number >= 0) sequenced = true;
	sequenceChecked = true;
	return sequenced;
    }

    public int getSequence() {
	int match_start = fileName.lastIndexOf((int)'/');
	if (match_start == -1) match_start = 0;
	int match_end = fileName.lastIndexOf((int)'.');
	if (match_end == -1) return -1;

	try {
	    String match = fileName.substring(match_start+1, match_end);
	    StringTokenizer stok = new StringTokenizer(match, "_.");
	    String s1 = stok.nextToken();
	    String s2 = stok.nextToken();
	    if (stok.hasMoreTokens()) {
		String seqstr = stok.nextToken();
		Integer seqi = new Integer(seqstr);
		sequence_number = seqi.intValue();
	    }
	} catch (Exception e) {
	}
	return sequence_number;
    }


} // end FileStyle
