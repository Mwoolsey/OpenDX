//

package dx.protocol;
import java.net.*;
import java.util.*;

//
// JavaDir,path
//
public class visitsMsg extends threadMsg {
    private int visitCnt;

    public visitsMsg() { 
	super();
	visitCnt = 0; 
    }

    public int getVisitCount() { return visitCnt; }
    public void setVisitCount(int c) { visitCnt = c; }

    public visitsMsg(String inputLine) { 
	super(inputLine);
	visitCnt = 0;
	try {
	    StringTokenizer stok = new StringTokenizer(inputLine, ",");
	    String tok = stok.nextToken();
	    tok = stok.nextToken();
	    Integer uc = new Integer(tok);
	    visitCnt = uc.intValue();
	} catch (Exception e) {
	}
    }

    public static String GetCommand() throws ClassNotFoundException
	   { return Class.forName("dx.protocol.visitsMsg").getName(); }
    public String toString() { 
	return getHeader() + "," + visitCnt;
    }
}
