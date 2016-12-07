//
package dx.protocol;
import java.net.*;
import java.io.*;

//
// JavaDir,path
//
public class endExecMsg extends threadMsg {
    public endExecMsg() { super(); }
    public endExecMsg(String inputLine) { super(inputLine); }
    public static String GetCommand() throws ClassNotFoundException
	   { return Class.forName("dx.protocol.endExecMsg").getName(); }
}
