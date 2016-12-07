//

package dx.protocol;
import java.net.*;
import java.io.*;

public class stopMsg extends threadMsg {
   public stopMsg() { super(); }
   public stopMsg(String inputLine) { super(inputLine); }
   public static String GetCommand() throws ClassNotFoundException
	  { return Class.forName("dx.protocol.stopMsg").getName(); }
}
