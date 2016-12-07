//

package dx.protocol;
import java.net.*;
import java.io.*;

//
// LOAD,foobar.net
//
public class loadMsg extends threadMsg {
    private String program;

    public loadMsg() { super(); program = null; }

    public loadMsg(String inputLine) {
	super(inputLine);
	int icomma = inputLine.indexOf(',');
	program = inputLine.substring(icomma+1);
    }

    public static String GetCommand() throws ClassNotFoundException
	   { return Class.forName("dx.protocol.loadMsg").getName(); }
    public String toString() {
	return getHeader() + "," + program;
    }

    public void setProgram(String program) { this.program = program; }
    public String getProgram() { return program; }
}
