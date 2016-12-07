//

package dx.protocol;
import java.net.*;
import java.util.*;

//
//
public class execOnceMsg extends threadMsg {
    private String macro;

    public String getMacroName() { return macro; }
    public void setMacroName(String c) { macro = c; }

    public execOnceMsg() { super(); macro = null; }
    public execOnceMsg(String inputLine) { 
	super(inputLine);
	macro = null;
	try {
	    int comma = inputLine.indexOf((int)',');
	    macro = inputLine.substring(comma+1);

	    //
	    // hack to deal with the fact that there is a subclass
	    //
	    comma = macro.indexOf((int)',');
	    if (comma != -1) macro = macro.substring(0, comma);
	} catch (Exception e) {
	}
    }

    public static String GetCommand() throws ClassNotFoundException
	   { return Class.forName("dx.protocol.execOnceMsg").getName(); }
    public String toString() { 
	return getHeader() + "," + macro;
    }
}
