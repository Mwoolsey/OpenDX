//

package dx.protocol;
import java.net.*;
import java.io.*;

//
// JavaDir,path
//
public class sendValueMsg extends threadMsg {
    private String value;

    public sendValueMsg() { 
	super();
	value = null;
    }

    //
    // Don't use StringTokenizer because the value may contain a comma
    //
    public sendValueMsg(String inputLine) { 
	super(inputLine);
	int icomma = inputLine.indexOf(',');
	value = inputLine.substring(icomma+1);
    }

    public void setValue(String value) { this.value = value; }
    public String getValue() { return this.value ; }

    public static String GetCommand() throws ClassNotFoundException
	   { return Class.forName("dx.protocol.sendValueMsg").getName(); }
    public String toString() {
	return getHeader() + "," + value;
    }
}
