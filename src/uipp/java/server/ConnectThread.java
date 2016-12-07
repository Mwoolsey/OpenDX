//
package server;
import java.net.*;
import java.io.*;
import java.util.*;
import dx.protocol.server.*;

public class ConnectThread extends Thread
{
    Socket clientSocket;
    public ConnectThread( Socket clientSocket )
    {
        this.clientSocket = clientSocket;
    }

    public void run()
    {
        try {
            BufferedReader dis = new BufferedReader( new 
            	InputStreamReader( clientSocket.getInputStream() ) );
            String inputLine = dis.readLine();

            if ( inputLine == null )
                return ;

            if ( DXServer.versionMatch( inputLine ) == true ) {
                PrintStream os = new PrintStream( clientSocket.getOutputStream() );
                serverVersionMsg svm = new serverVersionMsg();
                svm.setVersion( DXServer.MajorVersion, DXServer.MinorVersion,
                                DXServer.MicroVersion );
                os.println ( svm.toString() );
                os.flush();
                os = null;
            }

            else {
                dis.close(); dis = null;
                clientSocket.close(); clientSocket = null;
                return ;
            }

            Enumeration enum1 = DXServer.actions.elements();

            while ( enum1.hasMoreElements() ) {
                ServerCommand sc = ( ServerCommand ) enum1.nextElement();

                if ( inputLine.startsWith( sc.getCommandString() ) ) {
                    sc.execute( inputLine, clientSocket );
                    break;
                }
            }
        }

        catch ( Exception e ) {}

    }

}

; // end ConnectThread
