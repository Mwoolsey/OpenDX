//
package dx.protocol;
import java.net.*;
import java.io.*;

//
// JavaDir,path
//
public class doneMsg extends threadMsg {

    public static String GetCommand() throws ClassNotFoundException
	   { return Class.forName("dx.protocol.doneMsg").getName(); }
    public doneMsg() { super(); }
    public doneMsg(String inputLine) { super(inputLine); }
}
