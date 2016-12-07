//////////////////////////////////////////////////////////////////////////////
//                            DX SOURCEFILE        //
//////////////////////////////////////////////////////////////////////////////

/*
 * $Header: /src/master/dx/src/uipp/java/dx/client/AppletClient.java,v 1.3 2005/12/26 21:33:42 davidt Exp $
 */

/*
 *
 */

package dx.client;
import java.applet.*;
import java.awt.*;
import java.net.*;
import java.io.*;
import java.util.*;


//
//
//

public abstract class AppletClient extends Applet
{

    public void update( Graphics g )
    {
        paint( g );
    }

    //
    // Overriding these gives subclasses a way to control behavior without
    // implementing loading logic.
    //
    protected Image getCurrentImage()
    {
        return current_image;
    }

    protected int getCurrentSequence()
    {
        return current_sequence;
    }

    protected int getCurrentLoop()
    {
        return current_iteration;
    }

    protected boolean getImageHandling()
    {
        return true;
    }

    protected boolean getIsEmbedded()
    {
        return false;
    }

    protected int getNextIter( int i )
    {
        return i + 1;
    }

    protected boolean loadNextImage()
    {
        return initial_loading;
    }

    protected boolean isLoopValid( String gifname, int s, int l )
    {
        if ( s < current_sequence ) return false;

        if ( s > current_sequence ) return true;

        return ( l >= current_iteration );
    }

    protected synchronized void setCurrentImage( Image i, int s, int l )
    {
        current_image = i; current_sequence = s; current_iteration = l;
    }


    private Image current_image;
    private Vector fclts;
    private String gifname;
    private String mostRecentUrl;
    private int current_iteration;
    private int current_sequence;

    protected String getImageUrl()
    {
        return this.mostRecentUrl;
    }

    //
    // Set to true when initializing so that we'll look for each iteration
    // of a loop.  When running, we'll receive a notification for each iteration.
    //
    private boolean initial_loading;

    protected AppletClient()
    {
        super();
        gifname = null;
        mostRecentUrl = null;
        current_image = null;
        current_iteration = 0;
        fclts = new Vector( 4 );
    }

    public void init()
    {
        super.init();
        fclts.removeAllElements();

        gifname = getParameter( "INITIAL_IMAGE" );

        //
        // If our URL indicates a non-zero loop count then start loading
        // images earlier in the sequence.
        // Look for %s_%d.%d.%d.*
        //
        initial_loading = true;

        if ( getIsEmbedded() == false ) {
            current_sequence = 0;

            if ( getLoopCount( gifname ) > 0 ) {
                String s = getImageName( gifname, 0 );
                requestImage ( s, 0, 0 );
            }

            else {
                requestImage ( gifname, 0, 0 );
            }

            repaint();
        }
    }

    public void stop()
    {
        super.stop();
        Enumeration enum1 = this.fclts.elements();

        while ( enum1.hasMoreElements() ) {
            AppletLoadThread fclt = ( AppletLoadThread ) enum1.nextElement();

            if ( ( fclt != null ) && ( fclt.isAlive() ) )
                fclt.interrupt();
        }

        this.fclts.removeAllElements();
    }

    //
    // This is called in async fashion by AppletLoadThread.
    // It's possible to have 2 threads running.
    // The locking is to protect current_image.
    //

    public synchronized void imageAvailable
    ( Image i, String s, int sequence, int loop, String urlString )
    {
        if ( isLoopValid( s, sequence, loop ) == false ) return ;

        //
        // There isn't any way of knowing if another frame is available.
        // So we just have to go looking for it in the case where we're
        // just starting up.
        //
        if ( i != null ) {
            if ( loadNextImage() ) {
                int cnt = getNextIter( loop );
                String gn = getImageName ( s, cnt );
                requestImage( gn, sequence, cnt );
            }

            if ( sequence > current_sequence ) {
                Enumeration enum1 = fclts.elements();
                Vector deceased = new Vector( 4 );

                while ( enum1.hasMoreElements() ) {
                    AppletLoadThread fclt = ( AppletLoadThread ) enum1.nextElement();

                    if ( ( fclt != null ) && ( fclt.getSequence() < sequence ) ) {
                        if ( fclt.isAlive() ) 
                        	fclt.interrupt();

                        deceased.addElement( ( Object ) fclt );
                    }

                    else if ( ( fclt != null ) && ( fclt.isAlive() == false ) ) {
                        deceased.addElement( ( Object ) fclt );
                    }
                }

                enum1 = deceased.elements();

                while ( enum1.hasMoreElements() ) {
                    AppletLoadThread fclt = ( AppletLoadThread ) enum1.nextElement();
                    fclts.removeElement( ( Object ) fclt );
                }
            }

            this.mostRecentUrl = urlString;
            setCurrentImage( i, sequence, loop );
        }

        else {
            if ( initial_loading == false ) {
                System.out.println ( "AppletClient: couldn't load " + s );
            }

            else {
                initial_loading = false;
                current_sequence = -1;
            }
        }

        if ( i != null )
            repaint();
    }

    protected void requestImage ( String gifname, int sequence, int loop )
    {
        AppletLoadThread fclt = new AppletLoadThread( this, gifname, sequence, loop );
        fclt.start();
        fclts.addElement( ( Object ) fclt );
    }

    protected void requestImage ( String gifname )
    {
        int seq = getSequenceCount( gifname );
        int loop = getLoopCount( gifname );
        requestImage( gifname, seq, loop );
    }

    public void paint( Graphics g )
    {
        if ( getIsEmbedded() == false ) {
            Dimension d = getSize();

            if ( getCurrentImage() == null ) {
                g.setColor ( Color.black );
                g.fillRect ( 0, 0, d.width, d.height );
            }

            else {
                Image im = getCurrentImage();
                g.drawImage( im, 0, 0, d.width, d.height, Color.black, this );
            }
        }
    }

    //
    // Pull the loop counter out of the image name
    // The name looks like: foobar1.0.0.htm or
    // 1_1.0.0.htm
    //
    protected int getLoopCount( String s )
    {
        int slash = s.lastIndexOf( ( int ) /*File.separatorChar*/'/' );
        String basename;

        if ( slash >= 0 )
            basename = s.substring( slash + 1 );
        else
            basename = s;

        StringTokenizer stok = new StringTokenizer( basename, "." );

        int loop = 0;

        boolean loop_found = false;

        int tokens = 0;

        boolean parse_error = false;

        while ( ( parse_error == false ) && ( loop_found == false ) && ( stok.hasMoreTokens() ) ) {
            String tok = stok.nextToken();

            switch ( tokens ) {
            case 0:
                break;
            case 1:

                try {
                    Integer jidi = new Integer( tok );
                    int jid = jidi.intValue();
                }

                catch ( Exception e ) {
                    parse_error = true;
                }

                break;
            case 2:

                try {
                    Integer loopi = new Integer( tok );
                    loop = loopi.intValue();
                    loop_found = true;
                }

                catch ( Exception e ) {
                    parse_error = true;
                }

                break;
            default:
                parse_error = true;
                break;
            }

            tokens++;
        }

        if ( loop_found )
            return loop;

        initial_loading = false;

        return -1;
    }

    protected int getSequenceCount( String s )
    {
        int slash = s.lastIndexOf( ( int ) /*File.separatorChar*/'/' );
        String basename;

        if ( slash >= 0 )
            basename = s.substring( slash + 1 );
        else
            basename = s;

        StringTokenizer stok = new StringTokenizer( basename, "." );

        int seq = 0;

        boolean seq_found = false;

        int tokens = 0;

        boolean parse_error = false;

        while ( ( parse_error == false ) && ( seq_found == false ) && ( stok.hasMoreTokens() ) ) {
            String tok = stok.nextToken();

            switch ( tokens ) {
            case 0:
                break;
            case 1:

                try {
                    Integer seqi = new Integer( tok );
                    seq = seqi.intValue();
                    seq_found = true;
                }

                catch ( Exception e ) {
                    parse_error = true;
                }

                break;
            default:
                parse_error = true;
                break;
            }

            tokens++;
        }

        if ( seq_found )
            return seq;

        initial_loading = false;

        return -1;
    }

    protected String getImageName( String gifname, int loop )
    {
        int oldloop = getLoopCount( gifname );

        int slash = gifname.lastIndexOf( ( int ) /*File.separatorChar*/'/' );
        String basename;
        String fullPath;

        if ( slash >= 0 ) {
            basename = gifname.substring( slash + 1 );
            fullPath = gifname.substring( 0, slash + 1 );
        }

        else {
            basename = gifname;
            fullPath = null;
        }

        StringTokenizer stok = new StringTokenizer( basename, "." );
        boolean loop_found = false;
        int tokens = 0;
        boolean parse_error = false;

        while ( ( parse_error == false ) && ( loop_found == false ) && ( stok.hasMoreTokens() ) ) {
            String tok = stok.nextToken();

            switch ( tokens ) {
            case 0:

                if ( fullPath == null ) fullPath = tok;
                else fullPath = fullPath + tok;

                break;

            case 1:
                fullPath = fullPath + "." + tok;

                break;

            case 2:
                fullPath = fullPath + "." + loop;

                break;

            default:
                fullPath = fullPath + "." + tok;

                break;
            }

            tokens++;
        }

        return fullPath;
    }


} // end AppletClient


