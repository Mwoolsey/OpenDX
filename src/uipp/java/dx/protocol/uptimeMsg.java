//

package dx.protocol;
import java.net.*;
import java.util.*;

//
// JavaDir,path
//
public class uptimeMsg extends threadMsg {
    private String uptime;


    public String getUpTime() { return uptime; }
    public void setUpTime(String c) { uptime = c; }

    public uptimeMsg() { super(); uptime = null; }
    public uptimeMsg(String inputLine) { 
	super(inputLine);
	uptime = null;
	try {
	    StringTokenizer stok = new StringTokenizer(inputLine, ",");
	    String tok = stok.nextToken();
	    uptime = stok.nextToken();
	} catch (Exception e) {
	}
    }

    public static String GetCommand() throws ClassNotFoundException
	   { return Class.forName("dx.protocol.uptimeMsg").getName(); }
    public String toString() { 
	return getHeader() + "," + uptime;
    }
}
