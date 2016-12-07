//

package dx.protocol.server;
import java.net.*;
import java.io.*;

public class shutdownMsg extends serverMsg {
    public static String GetCommand()  throws ClassNotFoundException
	{ return Class.forName("dx.protocol.server.shutdownMsg").getName(); }

    public String toString() { 
       return getHeader() + "," + version_string;
    }
    public shutdownMsg() { super(); }
    public shutdownMsg(String inputLine) { super(inputLine); }
}
