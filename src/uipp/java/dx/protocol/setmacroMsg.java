//

package dx.protocol;
import java.net.*;
import java.util.*;

//
// JavaDir,path
//
public class setmacroMsg extends threadMsg {
    private String macro;


    public String getMacroName() { return macro; }
    public void setMacroName(String c) { macro = c; }

    public setmacroMsg() { super(); macro = null; }
    public setmacroMsg(String inputLine) { 
	super(inputLine);
	macro = null;
	try {
	    int comma = inputLine.indexOf((int)',');
	    macro = inputLine.substring(comma+1);
	} catch (Exception e) {
	}
    }

    public static String GetCommand() throws ClassNotFoundException
	   { return Class.forName("dx.protocol.setmacroMsg").getName(); }
    public String toString() { 
	return getHeader() + "," + macro;
    }
}
