//////////////////////////////////////////////////////////////////////////////
//                         DX SOURCEFILE        //
//////////////////////////////////////////////////////////////////////////////


/*
 * $Header: /src/master/dx/src/uipp/java/dx/client/AppletLoadThread.java,v 1.2 2005/10/27 19:43:05 davidt Exp $
 */

package dx.client;

import java.net.*;
import java.awt.*;



public class AppletLoadThread extends Thread
{

    private AppletClient parent;
    private int seqno, loopno;
    String gifname;
    
    public int getSequence()
    {
        return seqno;
    }

    public AppletLoadThread ( AppletClient parent, String gifname, int seqno, int loopno )
    {
        this.parent = parent;
        this.seqno = seqno;
        this.loopno = loopno;
        this.gifname = gifname;
    }

    public void run()
    {
        Image i;
        String urlStr = null;

        try {
            URL erl = new URL( parent.getCodeBase(), gifname );
            URLConnection erlc = erl.openConnection();
            erlc.setUseCaches( false );
            i = parent.getImage( erl );
            urlStr = erl.toString();
        }

        catch ( Exception e ) {
            System.out.println ( "AppletLoadThread: can't load " + gifname );
            i = null;
        }

        if ( i != null ) {
            try {
                MediaTracker mt = new MediaTracker( parent );
                mt.addImage( i, 0 );
                mt.waitForID( 0 );

                if ( mt.isErrorID( 0 ) == true )
                    i = null;
            }

            catch ( Exception e ) {
                System.out.println( "AppletLoadThread: Image loading failed." );
                i = null;
            }
        }

        parent.imageAvailable( i, gifname, seqno, loopno, urlStr );
    }


}

; // end AppletLoadThread
