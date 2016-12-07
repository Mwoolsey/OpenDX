//

package dx.protocol;
import java.net.*;
import java.io.*;

//
// JavaDir,path
//
public class noopMsg extends threadMsg {
    public noopMsg() {  super(); }
    public noopMsg(String inputLine) {  super(inputLine); }
    public static String GetCommand() throws ClassNotFoundException
	   { return Class.forName("dx.protocol.noopMsg").getName(); }
}
