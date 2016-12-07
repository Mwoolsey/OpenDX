//
package server;
import java.net.*;
import java.io.*;
import java.util.*;
import java.applet.*;
import java.text.*;
import dx.protocol.server.*;

public class DXServer extends Object
{

    private static ServerSocket serverSocket = null;
    private static int java_id = 1;
    public static Vector actions;
    private static Vector users;
    private static Date started_at;
    private static CleanUpDaemon Cud;
    private static boolean debug = false;
    private static String output_dir = null;
    private static String output_url = null;
    private static int max_sessions = 1;
    private static String max_session_str = null;

    //
    // How long in seconds should we let a copy of dxexec hang around
    // with no users before killing it.  We never flush cache or flush
    // dictionary because we assume that every copy of dxexec has
    // important state internally that we would rather not lose.
    //
    private final static int DXIdleTime = 600;

    private static Hashtable CachedConns = null;

    //
    // control (re)reading of the hosts file
    //
    private static String HostFileUsed = null;
    private static long HostFileModified = 0;
    private static Vector HostNames = null;
    private static Enumeration HostEnum;

    public static native int DXLSend( long dxl, String value );
    private static native long DXLStartDX( String cmdstr, String host );
    private static native int DXLExitDX( long dxl );

    public static String getStartDate()
    {
        return DateFormat.getDateInstance().format(started_at);
    }

    public static int getServedCount()
    {
        return java_id -1;
    }

    private static int maxThreads;
    public static int getMaxThreads()
    {
        return DXServer.maxThreads;
    }

    private final static int Port = 4655;

    final static int MajorVersion = 2;
    final static int MinorVersion = 0;
    final static int MicroVersion = 0;

    //
    // Minor version bumped:
    // 3: dx.protocol package, frame threads
    // 4: changed hardcoded strings to class names
    //

    public final static String VersionString =
        "" + MajorVersion + "." + MinorVersion + "." + MicroVersion;


    public final static int STARTDX = 1;
    public final static int STATUS = 4;
    public final static int SHUTDOWN = 5;

    public static String getOutputDir()
    {
        return output_dir;
    }

    public static String getOutputUrl()
    {
        return output_url;
    }


    public static void main( String[] args )
    {

        try {
            serverSocket = new ServerSocket( DXServer.Port );
        }

        catch ( IOException e ) {
            System.out.println( "Could not listen on port: " + DXServer.Port + ", " + e );
            System.exit( 1 );
        }

        String dbgmsgs = System.getProperty( "DXServer.debug" );

        if ( dbgmsgs != null ) debug = true;
        else debug = false;

        users = new Vector( 50 );

        DXServerThread.ClassInitialize();

        DXServer.HostFileUsed = null;

        DXServer.HostFileModified = 0;

        DXServer.HostNames = new Vector( 20 );

        DXServer.HostEnum = null;

        DXServer.ReadHostFile();

        DXServer.CachedConns = new Hashtable();

        DXServer.Cud = new CleanUpDaemon();

        DXServer.Cud.start();

        DXServer.maxThreads = 1;

        String mxstr = System.getProperty ( "DXServer.dxsessions" );

        if ( mxstr != null ) {
            try {
                DXServer.maxThreads = ( new Integer( mxstr ) ).intValue();
            }

            catch ( Exception e ) {}

        }

        output_dir = System.getProperty( "DXServer.outDir" );

        if ( output_dir == null ) output_dir = "./";

        output_url = System.getProperty( "DXServer.outUrl" );

        if ( output_url == null ) output_url = "./";

        if ( ( output_dir == null ) || ( output_url == null ) ) {
            System.out.println ( "DXServer requires output file name and url." );
            System.exit( 1 );
        }

        actions = new Vector( 10 );

        try {
            actions.addElement ( ( Object )
                                 new ServerCommand( startMsg.GetCommand(), DXServer.STARTDX ) );
            actions.addElement ( ( Object )
                                 new ServerCommand( statusMsg.GetCommand(), DXServer.STATUS ) );
            actions.addElement ( ( Object )
                                 new ServerCommand( shutdownMsg.GetCommand(), DXServer.SHUTDOWN ) );
        }

        catch ( ClassNotFoundException cnfe ) {
            cnfe.printStackTrace();
            System.exit( 1 );
        }

        System.out.println( "Starting DXServer... ready for clients" );
        started_at = new Date();

        while ( true ) {
            try {
                Socket clientSocket = serverSocket.accept();
                ConnectThread ct = new ConnectThread( clientSocket );
                ct.start();
                ct = null;
            }

            catch ( IOException e ) {
                System.out.println( "Accept failed: " + DXServer.Port + ", " + e );
                break;
            }

            catch ( Exception e ) {
                System.out.println( "Unknown error" );
                break;
            }
        }

        shutdown ( null, null );

    }

    public static boolean versionMatch ( String client_version )
    {
        boolean retval = false;

        serverVersionMsg svm = new serverVersionMsg( client_version );

        //
        // We require a server whose Major,Minor are at least what ours are
        // If Micro is less than ours, we'll proceed.
        //

        if ( ( svm.getMajor() >= DXServer.MajorVersion ) &&
                ( svm.getMinor() >= DXServer.MinorVersion ) )
            retval = true;

        if ( debug ) {
            System.out.println ( "DXServer: versionMatch(" + retval + ") " +
                                 client_version );
        }

        return retval;
    }

    public synchronized static int getUserCount()
    {
        return users.size();
    }

    public synchronized static boolean startDX( dx.protocol.server.serverMsg msg, Socket csock )
    {
        if ( debug ) {
            System.out.println ( "DXServer: " + msg.toString() );
        }

        DXServerThread dxst = new DXServerThread( csock, java_id );
        dxst.start();
        users.addElement( ( Object ) dxst );
        java_id++;
        return true;
    }

    public static boolean status ( dx.protocol.server.serverMsg msg, Socket csock )
    {
        StatusThread st = new StatusThread( csock );
        st.start();
        return true;
    }

    public static boolean shutdown ( dx.protocol.server.serverMsg msg, Socket csock )
    {
        if ( debug )
            System.out.println ( "DXServer: " + msg.toString() );

        //
        // DXServer should always go this way when exiting.  It cleans up files.
        //
        DXServer.Cud.interrupt();

        DXServer.CleanUp();

        Enumeration enum1 = users.elements();

        while ( enum1.hasMoreElements() ) {
            DXServerThread dxst = ( DXServerThread ) enum1.nextElement();

            if ( dxst.isAlive() == true ) {
                if ( debug )
                    System.out.println ( "     ***** DXServer: requesting thread shutdown" );

                dxst.cleanUp();

                dxst.interrupt();
            }
        }

        //
        // don't return from this point.
        //
        System.exit( 0 );

        return true;
    }

    //
    // Check every thread for alive-ness
    //

    public synchronized static void CleanUp()
    {
        Vector dead_users;
        dead_users = new Vector( 10 );
        Enumeration enum1 = users.elements();

        while ( enum1.hasMoreElements() ) {
            DXServerThread old_dxst = ( DXServerThread ) enum1.nextElement();

            if ( old_dxst.isAlive() == false )
                dead_users.addElement( old_dxst );
        }

        enum1 = dead_users.elements();

        while ( enum1.hasMoreElements() ) {
            DXServerThread dead_user = ( DXServerThread ) enum1.nextElement();
            users.removeElement( ( Object ) dead_user );
        }

        synchronized ( DXServer.CachedConns ) {
            Vector killed = new Vector( 4 );
            enum1 = DXServer.CachedConns.elements();

            while ( enum1.hasMoreElements() ) {
                ConnectionEntry existing = ( ConnectionEntry ) enum1.nextElement();

                if ( existing.getUserCount() == 0 ) {
                    if ( existing.getIdleTime() > DXServer.DXIdleTime ) {
                        Long dxlcon = existing.getDXLConnection();
                        DXServer.DXLExitDX ( dxlcon.longValue() );
                        killed.addElement( dxlcon );

                        if ( debug )
                            System.out.println ( "DXServer: terminate Data Explorer session" );
                    }
                }
            }

            enum1 = killed.elements();

            while ( enum1.hasMoreElements() ) {
                Long dxlcon = ( Long ) enum1.nextElement();
                ConnectionEntry ce = ( ConnectionEntry ) DXServer.CachedConns.get( dxlcon );
                ce.deleteFiles();
                DXServer.CachedConns.remove( dxlcon );
            }

            killed.removeAllElements();
        }
    }


    public synchronized static DXServerThread GetThread( int java_id )
    {
        DXServerThread dxst = null;
        Enumeration enum1 = users.elements();

        while ( enum1.hasMoreElements() ) {
            dxst = ( DXServerThread ) enum1.nextElement();

            if ( dxst.isAlive() ) {
                int id = dxst.getIdDX();

                if ( id == java_id )
                    break;
            }
        }

        return dxst;
    }

    public static void EndConnection( Long dxl, DXServerThread dxst )
    {
        synchronized ( DXServer.CachedConns ) {
            ConnectionEntry ce = ( ConnectionEntry ) DXServer.CachedConns.get( dxl );
            ce.detachUser( dxst );
        }
    }

    public synchronized static String createNet( String path, DXServerThread dxst )
    {
        ConnectionEntry ce = ( ConnectionEntry ) CachedConns.get( dxst.getDXLConnection() );
        return ce.getNetName( dxst );
    }

    public static void LockConnection( DXServerThread dxst )
    {
        ConnectionEntry ce = ( ConnectionEntry ) CachedConns.get( dxst.getDXLConnection() );
        ce.getConnectionLock();
    }

    public static void UnlockConnection( DXServerThread dxst )
    {
        ConnectionEntry ce = ( ConnectionEntry ) CachedConns.get( dxst.getDXLConnection() );
        ce.releaseConnectionLock();
    }

    public static Long NewConnection( DXServerThread dxst, String path )
    {
        ConnectionEntry ce = null;

        synchronized ( DXServer.CachedConns ) {
            if ( DXServer.CachedConns.size() > 0 ) {
                ConnectionEntry minEntry = null;
                Enumeration enum1 = CachedConns.elements();

                while ( enum1.hasMoreElements() ) {
                    ConnectionEntry existing = ( ConnectionEntry ) enum1.nextElement();

                    if ( existing.isFailed() ) {}
                    else if ( minEntry == null ) {
                        minEntry = existing;
                    }

                    else if ( existing.getUserCount() < minEntry.getUserCount() ) {
                        minEntry = existing;
                    }

                    else if ( existing.getUserCount() == minEntry.getUserCount() ) {
                        if ( existing.getUserTime() < minEntry.getUserTime() ) {
                            minEntry = existing;
                        }
                    }

                    if ( ( minEntry != null ) && ( minEntry.getUserCount() == 0 ) )
                        break;
                }

                ce = minEntry;
            }

            boolean more_capacity = false;

            if ( DXServer.GetMaxSessions() > DXServer.CachedConns.size() )
                more_capacity = true;

            if ( ( ce != null ) && ( ce.getUserCount() >= 1 ) && ( more_capacity ) )
                ce = null;

            if ( ce == null ) {
                String host = DXServer.GetNextHost();
                long dxlcon = 0;
                try {
                    dxlcon =
                    DXServer.DXLStartDX( "dx -optimize memory -execonly -processors 1 -highlight off", host );
                } catch (Exception e) {
                	System.out.println ( "Problem starting one of your hosts." );
                	System.out.println ("Startup was: dx -optimize memory -execonly -processors 1 -highlight off :" + host); 
            		System.exit( 1 );
            	}
            	if (dxlcon == 0) {
            		System.out.println ( "Problem starting one of your hosts." );
                	System.out.println ("Startup was: dx -optimize memory -execonly -processors 1 -highlight off :" + host); 
            		System.exit( 1 );
            	}
                	
                Long C = new Long( dxlcon );
                ce = new ConnectionEntry( C );
                CachedConns.put( C, ce );
            }

            if ( ce == null ) return null;

            ce.attachUser( dxst, path );
        }

        return ce.getDXLConnection();
    }

    public static int GetMaxSessions()
    {
        if ( DXServer.max_session_str == null ) {
            DXServer.max_session_str = System.getProperty( "DXServer.max_sessions" );

            if ( DXServer.max_session_str == null ) DXServer.max_session_str = "1";

            try {
                Integer I = new Integer( DXServer.max_session_str );
                DXServer.max_sessions = I.intValue();
            }

            catch ( NumberFormatException nfe ) {
                DXServer.max_session_str = "1";
                DXServer.max_sessions = 1;
            }
        }

        return DXServer.max_sessions;
    }

    //
    // Read the contents of dxserver.hosts
    // Look in:
    // 1) /etc
    // 2) current working dir
    // 3) environment variable DXServer.hostsFile
    // Don't look for multiple copies of the file.  Stop after reading the first one found.
    // N.B. The caller is responsible for locking before both of these class methods.
    //
    private static String GetNextHost()
    {
        if ( DXServer.HostEnum.hasMoreElements() == false )
            DXServer.HostEnum = HostNames.elements();

        return ( String ) DXServer.HostEnum.nextElement();
    }

    private static void ReadHostFile()
    {
        boolean file_found = false;
        boolean file_read = false;
        Vector paths;

        paths = new Vector( 5 );

        if ( DXServer.HostFileUsed != null )
            paths.addElement( ( Object ) DXServer.HostFileUsed );

        paths.addElement( ( Object ) File.separator + "etc" + File.separator + "dxserver.hosts" );

        paths.addElement( ( Object ) "." + File.separator + "dxserver.hosts" );

        paths.addElement( ( Object ) System.getProperty( "DXServer.hostsFile" ) );

        synchronized ( DXServer.HostNames ) {
            try {

                Enumeration enum1 = paths.elements();

                while ( ( !file_read ) && ( enum1.hasMoreElements() ) ) {
                    String path = ( String ) enum1.nextElement();
                    File hf = new File( path );

                    if ( hf.exists() == false ) continue;

                    if ( hf.canRead() == false ) continue;

                    if ( hf.isFile() == false ) continue;

                    file_found = true;

                    long modified = hf.lastModified();

                    //
                    // If we've already read an up-to-date hosts file, then
                    // do nothing.
                    //
                    if ( ( DXServer.HostFileUsed != null ) &&
                            ( DXServer.HostFileUsed.equals( path ) ) &&
                            ( DXServer.HostFileModified >= modified ) )
                        break;

                    DXServer.HostNames.removeAllElements();

                    FileReader hfr = new FileReader( hf );

                    BufferedReader hbr = new BufferedReader ( hfr );

                    String host;

                    while ( ( host = hbr.readLine() ) != null ) {
                    	host = host.trim();
                    	
                        if ( host.startsWith( "//" ) ) continue;

                        if ( host.startsWith( "!#" ) ) continue;

                        if ( host.startsWith( "#" ) ) continue;
                        
                        if ( host.length() == 0) continue;

                        HostNames.addElement( ( Object ) host );
                    }

                    DXServer.HostFileUsed = hf.getPath();
                    DXServer.HostFileModified = modified;
                    file_read = true;
                }
            }

            catch ( Exception e ) {
                e.printStackTrace();
            }

            if ( !file_found ) {
                DXServer.HostFileUsed = null;
                DXServer.HostNames.removeAllElements();
            }

            if ( DXServer.HostNames.size() == 0 )
                DXServer.HostNames.addElement( ( Object ) "localhost" );

            if ( ( !file_found ) || ( file_read ) )
                DXServer.HostEnum = DXServer.HostNames.elements();
        }
    }
} // end DXServer



