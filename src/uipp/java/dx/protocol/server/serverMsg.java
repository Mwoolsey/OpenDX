//

package dx.protocol.server;
import dx.protocol.*;
import java.net.*;
import java.io.*;

public abstract class serverMsg extends message {
   public String getCommand() { return this.getClass().getName(); }
   public String toString() { return getHeader(); }
   public serverMsg() { super(); }
   public serverMsg(String inputLine) { super(inputLine); }
}
