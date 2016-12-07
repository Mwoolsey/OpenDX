//

package dx.protocol;
import java.net.*;
import java.io.*;

public abstract class threadMsg extends message {
    public String getCommand() { return getClass().getName(); }
    protected threadMsg() { super(); }
    public String toString() { return getHeader(); }
    protected threadMsg(String inputLine) { super(inputLine); }
}
