//
package server;
import java.net.*;
import java.io.*;


public class CleanUpDaemon extends Thread
{
    //
    // Number of milliseconds betwen clean up operations.
    //
    long millis = 30000; 

    public void run() 
    {
	boolean run = true;
	while (run) {
	    DXServer.CleanUp();
	    try {
		sleep(millis);
	    } catch (InterruptedException ie) {
		System.out.println ("CleanUpDaemon exiting due to InterruptedException");
		run = false;
	    }
	}
    }

}; // end CleanUpDaemon
