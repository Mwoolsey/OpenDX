//
package dx.protocol.server;
import java.net.*;
import java.io.*;

public class startMsg extends serverMsg {
    public static String GetCommand()  throws ClassNotFoundException
	{ return Class.forName("dx.protocol.server.startMsg").getName(); }

    public startMsg() { super(); }
    public startMsg(String inputLine) { super(inputLine); }
}
