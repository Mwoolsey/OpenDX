//
package dx.protocol;
import java.net.*;
import java.io.*;

//
// JavaDir,path
//
public class clientMsg extends threadMsg {

    private String message;
    public void   setMessage(String msg) { this.message = msg; }
    public String getMessage() { return this.message; }

    public clientMsg() {  
	this.message = null;
    }

    //
    // Can't use StringTokenizer because of the possible contents of this.message
    //
    public clientMsg(String inputLine) {  
	super(inputLine);
	int comma = inputLine.indexOf((int)',');
	this.message = inputLine.substring(comma+1);
    }

    public static String GetCommand() throws ClassNotFoundException
	   { return Class.forName("dx.protocol.clientMsg").getName(); }
    public String toString() { 
	return getHeader() + "," + this.message;
    }
}
