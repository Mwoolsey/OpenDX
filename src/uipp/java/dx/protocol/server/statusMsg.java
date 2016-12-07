//

package dx.protocol.server;
import java.net.*;
import java.io.*;

public class statusMsg extends serverMsg {
    public static String GetCommand()  throws ClassNotFoundException
	{ return Class.forName("dx.protocol.server.statusMsg").getName(); }

    public statusMsg() { super(); }
    public statusMsg(String inputLine) { super(inputLine); }
}
