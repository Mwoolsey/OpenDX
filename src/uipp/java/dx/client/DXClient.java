//////////////////////////////////////////////////////////////////////////////
//                        DX SOURCEFILE         //
//////////////////////////////////////////////////////////////////////////////

/*
 * $Header: /src/master/dx/src/uipp/java/dx/client/DXClient.java,v 1.4 2005/10/27 19:43:05 davidt Exp $
 */

/*
 *
 */

package dx.client;

import java.applet.*;
import java.awt.*;
import java.net.*;
import java.io.*;
import java.io.BufferedReader.*;
import java.util.*;
import dx.protocol.server.*;

//
//
//

public abstract class DXClient extends Applet
{

        private StopThread stoptr = null;
        private DXClientThread extr = null;
        protected abstract DXClientThread createCommandThread();

        private Socket dxSocket;
        protected PrintStream os;
        protected BufferedReader is;
        protected int getPort()
        {
                return 4655;
        }

        protected abstract dx.protocol.server.serverMsg getRequest();

        //
        // These are set later on.
        //
        private int ServerMajor = 0;
        private int ServerMinor = 0;
        private int ServerMicro = 0;

        protected final static int MajorVersion = 2;
        protected final static int MinorVersion = 0;
        protected final static int MicroVersion = 0;
        protected final static String version_string =
                MajorVersion + "." + MinorVersion + "." + MicroVersion;
        protected final static String formal_version_string =
                "IBM Visualization Data Explorer / Java Explorer version " + version_string;


        public boolean isConnected()
        {
                if ( dxSocket != null ) {
                        if ( os.checkError() == true ) {
                                dxSocket = null;
                                disconnect( null );
                        }
                }

                return ( dxSocket != null );
        }

        protected DXClient()
        {
                dxSocket = null;
                stoptr = null;
                extr = null;
        }

        public void init()
        {
                disconnect( null );

                try {
                        String bgcolor = this.getParameter( "BACKGROUND" );

                        if ( bgcolor != null ) {
                                StringTokenizer stok = new StringTokenizer( bgcolor, "[, ]" );
                                String redstr = stok.nextToken();
                                String greenstr = stok.nextToken();
                                String bluestr = stok.nextToken();
                                float red = Float.valueOf( redstr ).floatValue();
                                float green = Float.valueOf( greenstr ).floatValue();
                                float blue = Float.valueOf( bluestr ).floatValue();
                                Color c = new Color( red, green, blue );

                                if ( c != null )
                                        this.setBackground( c );
                        }
                }

        catch ( Exception e ) {}

        }

        public void destroy()
        {
                disconnect( null );
        }

        protected void disconnect( Thread t )
        {
                try {
                        if ( dxSocket != null ) dxSocket.close();

                        if ( os != null ) os.close();

                        if ( is != null ) is.close();
                }

        catch ( Exception e ) {}

                dxSocket = null;

                os = null;
                is = null;

                if ( ( extr != null ) && ( extr.isAlive() ) ) {
                        if ( extr != t )
                                extr.interrupt();
                }

                //
                // Either of these 2 threads can call this routine.  By
                // stopping the thread, you're actually interrupting the call.
                // So we want the very last thing we do in this routine to be
                // the call the stop the StopThread.  We must make sure we
                // get it halted.
                //
                if ( ( stoptr != null ) && ( stoptr.isAlive() ) ) {
                        if ( stoptr != t )
                                stoptr.interrupt();
                }

                stoptr = null;

                //
                // Don't add any code after the call to t.stop() because that
                // call will terminate execution of this routine in the case where
                // we were called from a thread.
                //

                if ( ( t != null ) && ( t.isAlive() ) )
                        t.interrupt();
        }

        public void interrupt()
        {
                if ( ( stoptr != null ) && ( stoptr.isAlive() == true ) )
                        stoptr.interrupt();

                stoptr = new StopThread( this );

                stoptr.start();
        }

        public void start()
        {
                if ( ( stoptr != null ) && ( stoptr.isAlive() == true ) )
                        stoptr.interrupt();

                stoptr = null;

                if ( isConnected() == false )
                        connect();
        }

        protected void connect()
        {
                String host = getDocumentBase().getHost();

                try {
                        dxSocket = new Socket( host, getPort() );
                        os = new PrintStream( dxSocket.getOutputStream() );
                        is = new BufferedReader( new InputStreamReader( dxSocket.getInputStream() ) );
                        serverMsg msg = getRequest();
                        send( msg.toString() + "," + version_string );
                        String servers_version = is.readLine();

                        if ( versionMatch( servers_version ) == false ) {
                                System.out.println ( "ERROR: " + formal_version_string +
                                                     " is incompatible with server " + servers_version );
                                disconnect( null );
                        }
                }

                catch ( Exception e ) {
                        System.err.println( "DXClient: Couldn't get I/O for " + host );
                        dxSocket = null;
                        os = null;
                        is = null;
                }

                if ( isConnected() ) {
                        extr = this.createCommandThread();
                }
        }

        protected boolean send( String args )
        {

                boolean ok = isConnected();

                if ( isConnected() ) {
                        try {
                                os.println( args );
                                os.flush();
                        }

                        catch ( Exception e ) {
                                dxSocket = null;
                                disconnect( null );
                                ok = false;
                        }
                }

                return ok;
        }

        protected boolean versionMatch( String server_version )
        {
                boolean retval = false;

                serverVersionMsg svm = new serverVersionMsg( server_version );
                ServerMajor = svm.getMajor();
                ServerMinor = svm.getMinor();
                ServerMicro = svm.getMicro();

                //
                // We require a server whose Major,Minor are at least what ours are
                // If Micro is less than ours, we'll give a warning and proceed.
                //

                if ( ( ServerMajor >= DXClient.MajorVersion ) &&
                        ( ServerMinor >= DXClient.MinorVersion ) )
                        retval = true;

                if ( ( retval ) && ( ServerMicro < DXClient.MicroVersion ) )
                        System.out.println ( "WARNING: Server is a earlier version." );


                return retval;
        }

} // end DXClient


