//

package dx.protocol;
import java.net.*;
import java.io.*;
import java.util.*;

//
// JavaDir,path
//
public class sequenceMsg extends threadMsg {
    int action;
    public int getAction() { return this.action; }
    public void setAction(int action) { this.action = action; }
    public sequenceMsg() { super(); this.action = 0; }
    public sequenceMsg(int action) { super(); this.action = action; }
    public sequenceMsg(String inputLine) { 
	super(inputLine); 
	try {
	    String hdr = this.getHeader();
	    String data_str = inputLine.substring(hdr.length()+1);
	    StringTokenizer stok = new StringTokenizer(data_str, ",");
	    String actionid = stok.nextToken();
	    this.action = new Integer(actionid).intValue();
	} catch (Exception e) {
	    this.action = 0;
	}
    }
    public String toString() { return getHeader() + "," + action; }
    public static String GetCommand() throws ClassNotFoundException
	   { return Class.forName("dx.protocol.sequenceMsg").getName(); }
}
