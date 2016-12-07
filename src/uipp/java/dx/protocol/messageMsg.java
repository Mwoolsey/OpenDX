//

package dx.protocol;
import java.net.*;
import java.util.*;

//
// JavaDir,path
//
public class messageMsg extends threadMsg {
    private String message;


    public String getMessage() { return message; }
    public void setMessage(String c) { message = c; }

    public messageMsg() { super(); message = null; }
    public messageMsg(String inputLine) { 
	super(inputLine);
	message = null;
	//
	// Don't use StringTokenizer because the message might have
	// a separator within and we want the entire thing.
	//
	try {
	    int comma = inputLine.indexOf((int)',');
	    message = inputLine.substring(comma+1);
	} catch (Exception e) {
	}
    }

    public static String GetCommand() throws ClassNotFoundException
	   { return Class.forName("dx.protocol.messageMsg").getName(); }
    public String toString() { 
	return getHeader() + "," + message;
    }
}
