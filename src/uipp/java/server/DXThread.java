//
package server;
import java.net.*;
import java.io.*;
import java.util.*;
import dx.protocol.*;

public abstract class DXThread extends Thread
{
    protected Socket clientSocket = null;
    protected Vector actions;

    private BufferedReader is;
    private PrintStream os;
    private Vector msg_queue;

    //
    // Failure status
    //
    private boolean failed = false;
    protected void setFailed()
    {
        failed = true;
    }

    protected boolean isFailed()
    {
        return failed;
    }

    //
    // debug status
    //
    private boolean debug = false;
    protected boolean isDebug()
    {
        return debug;
    }

    public void cleanUp()
    { }


    protected DXThread ( Socket client )
    {
        clientSocket = client;

        try {
            is = new BufferedReader( new InputStreamReader( clientSocket.getInputStream() ) );
            os = new PrintStream( clientSocket.getOutputStream(), false );
        }

        catch ( Exception e ) {
            e.printStackTrace();
            is = null;
            os = null;
        }

        actions = new Vector( 20 );
        msg_queue = new Vector( 10 );

        String debug_option_string = getClass().getName() + ".debug";
        String dbgmsgs = System.getProperty( debug_option_string );

        if ( dbgmsgs != null ) debug = true;
        else debug = false;

        failed = false;
    }

    protected String getNextString() throws IOException
    {
        String retval = null;

        synchronized ( msg_queue ) {
            if ( getQueueSize() > 0 ) {
                String msg = ( String ) msg_queue.firstElement();
                msg_queue.removeElementAt( 0 );
                retval = msg;
            }

            else
                retval = is.readLine();
        }

        return retval;
    }

    protected int getQueueSize() throws IOException
    {
        int qsize = 0;

        synchronized ( msg_queue ) {
            while ( is.ready() ) {
                String inputLine = is.readLine();
                msg_queue.addElement( ( Object ) inputLine );
            }

            qsize = msg_queue.size();
        }

        return qsize;
    }

    public final void run()
    {
        String inputLine = null;

        try {
            while ( ( failed == false ) && ( ( inputLine = getNextString() ) != null ) ) {
                if ( debug )
                    System.out.println ( getName() + ": " + inputLine );

                Enumeration enum1 = actions.elements();

                boolean executed = false;

                while ( enum1.hasMoreElements() ) {
                    DXThreadCommand stc = ( DXThreadCommand ) enum1.nextElement();

                    if ( inputLine.startsWith( stc.getCommandString() ) ) {
                        executed = stc.execute( inputLine );

                        if ( executed ) break;
                    }
                }

                if ( !executed )
                    System.out.println ( getName() + ": Unrecognized: " +
                                         inputLine );
            }
        }

    catch ( SocketException se ) {}
        catch ( Exception e ) {
            e.printStackTrace();
            setFailed();
        }

        disconnect();
    }

    protected void transmit( threadMsg msg )
    {
        if ( isDebug() ) {
            System.out.println ( "                                       " +
                                 getName() + ": " + msg.toString() );
        }

        os.println ( msg.toString() );
        os.flush();
    }

    //
    // There's a problem here I don't understand.  The thread is generally sitting
    // in getNextString() waiting for input from the browser.  If DXServer decides
    // it's time to shutdown, then he wants to call disconnect() for every running
    // thread.  The problem with that is that the call to close a socket blocks.
    // I don't know why - must have something to do with having a thread inside
    // readLine.  The book says these methods aren't synchronized but apparently
    // something is synchronized inside there.  So, I wrote virtual cleanUp()
    // so that DXServer can call run clean-up type work before shutting down.
    // That's important because we need a change to erase image files.
    //
    protected boolean disconnect( threadMsg msg )
    {
        cleanUp();

        try {
            clientSocket.close();
        }

        catch ( Exception e ) { }

        try {
            is.close();
        }

        catch ( Exception e ) { }

        try {
            os.close();
        }

        catch ( Exception e ) { }

        return true;
    }

    protected boolean disconnect()
    {
        return disconnect( null );
    }

    protected String peekQueue( String cmd, boolean remove ) throws IOException
    {
        String retval = null;

        synchronized ( msg_queue ) {
            if ( getQueueSize() > 0 ) {
                Enumeration enum1 = msg_queue.elements();

                while ( enum1.hasMoreElements() ) {
                    String msg = ( String ) enum1.nextElement();

                    if ( msg.startsWith( cmd ) ) {
                        retval = msg;
                        break;
                    }
                }

                if ( remove )
                    msg_queue.removeElement( retval );
            }
        }

        return retval;
    }


} // end DXThread
