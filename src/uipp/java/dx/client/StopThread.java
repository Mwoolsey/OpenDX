//////////////////////////////////////////////////////////////////////////////
//                        DX SOURCEFILE					    //
//////////////////////////////////////////////////////////////////////////////

/*
 * $Header: /src/master/dx/src/uipp/java/dx/client/StopThread.java,v 1.1.1.1 1999/03/24 15:17:30 gda Exp $
 */
package dx.client;
import java.awt.*;
import java.util.*;

public class StopThread extends Thread {
    private final static int millis = 3000;
    private DXClient parent;

public StopThread(DXClient dxl) {
    parent = dxl;
}

public void run() {
    try { 
	sleep(millis); 
    } catch (Exception e) { 
    }

    //
    // Do not add any code after the call to disconnect because
    // the applet will stop this thread
    //
    parent.disconnect(this);
}

} // end StopThread

