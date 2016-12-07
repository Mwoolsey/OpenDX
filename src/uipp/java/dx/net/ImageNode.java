//
//


/*
 * $Header: /src/master/dx/src/uipp/java/dx/net/ImageNode.java,v 1.5 2005/12/02 23:37:27 davidt Exp $
 */

package dx.net;
import java.lang.*;
import java.util.*;
import java.applet.*;
import java.awt.*;
import java.net.*;

public class ImageNode extends dx.net.DrivenNode
{


        private final static int IMAGETAG = 1;
        private final static int WHERE = 3;
        private final static int USEVECTOR = 4;
        private final static int TO = 5;
        private final static int FROM = 6;
        private final static int WIDTH = 7;
        private final static int RESOLUTION = 8;
        private final static int ASPECT = 9;
        private final static int UP = 10;
        private final static int VIEWANGLE = 11;
        private final static int PROJECTION = 12;
        private final static int RESET_CAMERA = 21;
        private final static int INTERACTIONMODE = 41;
        private final static int IMAGENAME = 42;

        private final static int JAVA_OPTIONS = 49;

        //
        // These are input param numbers for the WebOptions macro.  They belong
        // in WebOptionsNode.C but there isn't any way to make a subclass of Node
        // just for a particular macro unless it's done in the style of ImageNode
        // which seems like too much work in this case.  Since WebOptions is just
        // a plain old macro created with the vpe, it's actually a MacroNode when
        // the ui creates one.
        //
        private final static int WEB_FORMAT = 1;
        private final static int WEB_FILE = 2;
        private final static int WEB_ENABLED = 3;
        private final static int WEB_COUNTERS = 4;
        private final static int WEB_ORBIT = 5;
        private final static int WEB_ID = 6;
        private final static int WEB_IMGID = 7;
        private final static int WEB_ORBIT_DELTA = 8;
        private final static int WEB_CACHE_SIZE = 9;
        private final static int WEB_GROUP_NAME = 10;

        public final static int NO_PALINDROME = 0;
        public final static int PALINDROME = 1;
        public final static int FASTER = 2;
        public final static int SLOWER = 3;
        public final static int RESET = 4;

        //
        // The origin of this value is WebOptions.imgId.  WebOptions
        // is an instance of a macro in the visual program.  We use
        // this value as an index into an array of applets.  We get
        // the array of applets from the browser's document object
        // model using javascript.
        //
        private int image_id;
        private boolean orbit_mode;
        private int interaction_mode;
        private ImageWindow iw;
        private String pick_list;
        private int reset_camera;
        private boolean perspective;
        private dx.net.WebOptionsNode web_options_node;
        private String group_name;
        private int cache_count;
        private int cache_speed;
        private boolean palindrome;

        public void setPalindromeMode( boolean onoroff )
        {
                this.palindrome = onoroff;
        }

        public boolean getPalindromeMode()
        {
                return this.palindrome;
        }

        public ImageNode( Network net, String name, int instance, String notation )
        {
                super( net, name, instance, notation );
                this.image_id = -1;
                this.orbit_mode = false;
                this.interaction_mode = ImageWindow.NO_MODE;
                this.iw = null;
                this.pick_list = null;
                this.web_options_node = null;
                this.reset_camera = 0;
                this.perspective = false;
                this.group_name = null;
                this.cache_count = 0;
                this.cache_speed = 200;
                this.palindrome = false;
        }

        public boolean isPerspective()
        {
                return this.perspective;
        }

        public synchronized void setCacheOption ( int sel )
        {
                int speed;

                switch ( sel ) {
                case ImageNode.FASTER:
                        speed = this.getCacheSpeed();
                        speed -= 50;
                        speed = ( speed < 50 ? 50 : speed );
                        this.setCacheSpeed( speed );
                        break;
                case ImageNode.SLOWER:
                        speed = this.getCacheSpeed();
                        speed += 50;
                        speed = ( speed > 2000 ? 2000 : speed );
                        this.setCacheSpeed( speed );
                        break;
                case ImageNode.PALINDROME:
                        this.setPalindromeMode( true );
                        break;
                case ImageNode.NO_PALINDROME:
                        this.setPalindromeMode( false );
                        break;
                case ImageNode.RESET:
                        this.setPalindromeMode( false );
                        this.setCacheSpeed( 200 );
                        this.setCacheSize( 0 );

                        if ( this.iw != null )
                                this.iw.clearImageCache();

                        break;
                }
        }

        public synchronized int getCacheSpeed ()
        {
                return this.cache_speed;
        }

        public synchronized void setCacheSpeed ( int speed )
        {
                this.cache_speed = speed;
        }


        public void setInitialInteractionMode( int imode )
        {
                if ( this.iw == null )
                        this.interaction_mode = imode;
        }

        public String getTitle()
        {
                return this.getInputValueString ( ImageNode.IMAGENAME );
        }

        public int getInteractionMode()
        {
                return this.interaction_mode;
        }

        public boolean handleAssignment ( String lhs, String rhs )
        {
                boolean handled = false;
                
                if ( lhs.equals( "imgId" ) ) {
                        //
                        // If my name matches the initial node name for my image window
                        // then I'm going to refuse to switch.  If you want me to switch
                        // then you wire the WebOptions.imgId param in the net.
                        //
						//System.out.println("ImageNode::handleAssignment(" + lhs + "," + rhs + "):imgId " + this.image_id + " [" + this.getMatchString() + "]");

                        if ( ( this.image_id != -1 ) && ( this.iw != null ) &&
                                ( this.iw.getInitialNodeName( this.getNetwork() ).equals( this.getMatchString() ) ) ) {
                                handled = true;
                        }

                        else {
                                try {
                                        Integer iv = new Integer( rhs );
                                        int new_image = iv.intValue();

                                        if ( new_image != this.image_id ) {
                                                this.image_id = new_image;
                                                this.iw = null;
                                        }

                                        handled = true;
                                }

                                catch ( NumberFormatException nfe ) {
                                        System.out.println ( "ImageNode: Couldn't parse: " + rhs );
                                }
                        }
                }

                else if ( lhs.equals( "orbit" ) ) {
                        try {
                                Integer iv = new Integer( rhs );
                                this.forceOrbitMode( ( iv.intValue() == 1 ) );
                                handled = true;
                        }

                        catch ( NumberFormatException nfe ) {
                                System.out.println ( "ImageNode: Couldn't parse: " + rhs );
                        }
                }

                else if ( lhs.equals( "show" ) ) {
                        DXApplication dxapp = this.getNetwork().getApplet();
                        String bwcompat = dxapp.getBWCompatString();
                        String newrhs = rhs;

                        if ( bwcompat != null ) newrhs = bwcompat + rhs;

                        if ( rhs.endsWith( ".wrl" ) )
                                handled = this.showWorld( newrhs );
                        else if ( rhs.endsWith( ".dx" ) )
                                handled = this.showObject( newrhs );
                        else
                                handled = this.showImage( newrhs );
                }

                else if ( lhs.equals( "reset" ) ) {
                        try {
                                Integer iv = new Integer( rhs );
                                this.reset_camera = iv.intValue();
                                handled = true;
                        }

                        catch ( NumberFormatException nfe ) {
                                System.out.println ( "ImageNode: Couldn't parse: " + rhs );
                                this.reset_camera = 0;
                        }
                }

                else if ( lhs.equals( "width" ) ) {
                        if ( this.reset_camera == 1 )
                                this.setInputValueQuietly ( ImageNode.WIDTH, rhs );

                        this.setInputValueString( ImageNode.WIDTH, rhs );

                        this.perspective = false;

                        handled = true;
                }

                else if ( lhs.equals( "angle" ) ) {
                        if ( this.reset_camera == 1 )
                                this.setInputValueQuietly ( ImageNode.VIEWANGLE, rhs );

                        this.setInputValueString( ImageNode.VIEWANGLE, rhs );

                        this.perspective = true;

                        handled = true;
                }

                else if ( lhs.equals( "to" ) ) {
                        if ( this.reset_camera == 1 )
                                this.setInputValueQuietly ( ImageNode.TO, rhs );

                        this.setInputValueString( ImageNode.TO, rhs );

                        handled = true;
                }

                else if ( lhs.equals( "from" ) ) {
                        if ( this.reset_camera == 1 )
                                this.setInputValueQuietly ( ImageNode.FROM, rhs );

                        this.setInputValueString ( ImageNode.FROM, rhs );

                        handled = true;
                }

                else if ( lhs.equals( "up" ) ) {
                        if ( this.reset_camera == 1 )
                                this.setInputValueQuietly ( ImageNode.UP, rhs );

                        this.setInputValueString( ImageNode.UP, rhs );

                        handled = true;
                }

                else if ( lhs.equals( "group" ) ) {
                        if ( rhs.equals( "untitled" ) ) this.setGroupName( null );
                        else this.setGroupName( rhs );

                        handled = true;
                }

                else if ( lhs.equals( "mode" ) ) {
                        if ( this.isInputConnected( ImageNode.INTERACTIONMODE ) ) {
                                DXApplication dxapp = this.getNetwork().getApplet();
                                boolean oK = false;

                                try {
                                        if ( rhs.equals( "none" ) )
                                                oK = dxapp.setInteractionMode( ImageWindow.NO_MODE, 0 );
                                        else if ( rhs.equals( "pick" ) )
                                                oK = dxapp.setInteractionMode( ImageWindow.PICK_MODE, 0 );
                                        else if ( rhs.equals( "zoom" ) )
                                                oK = dxapp.setInteractionMode( ImageWindow.ZOOM_MODE, 0 );
                                        else if ( rhs.equals( "panzoom" ) )
                                                oK = dxapp.setInteractionMode( ImageWindow.PAN_MODE, 0 );
                                        else if ( rhs.equals( "rotate" ) )
                                                oK = dxapp.setInteractionMode( ImageWindow.ROTATE_MODE, 0 );
                                }

                        catch ( Exception e ) {}

                        }

                        handled = true;
                }

                else if ( lhs.equals( "cache" ) ) {
                        try {
                                Integer iv = new Integer( rhs );
                                this.setCacheSize( iv.intValue() );
                        }

                        catch ( NumberFormatException nfe ) {
                                System.out.println ( "ImageNode: Couldn't parse: " + rhs );
                                this.cache_count = 0;
                        }
                }

                if ( handled ) return true;
                else return super.handleAssignment( lhs, rhs );
        }

        //
        // Need to update the size of the buffer in ImageWindow
        // and the size displayed by DXApplication
        //
        public void setCacheSize( int size )
        {
                this.cache_count = size;
                DXApplication dxapp = this.getNetwork().getApplet();

                if ( this == dxapp.getSelectedImageNode() )
                        dxapp.updateCaching( this );
        }

        public int getCacheCount()
        {
                return this.cache_count;
        }

        public String getGroupName()
        {
                return this.group_name;
        }

        public void setGroupName( String g )
        {
                this.group_name = g;
        }

        private void forceOrbitMode( boolean mode )
        {
                if ( mode == this.orbit_mode ) return ;

                this.orbit_mode = mode;

                DXApplication dxapp = this.getNetwork().getApplet();

                if ( this == dxapp.getSelectedImageNode() )
                        dxapp.enableModeSelector( this.orbit_mode == false );
        }

        //
        // 1) Form a complete url (prepending file:/ or http:/host/
        // depending on documentBase() onto path).
        // 2) Use image_id to locate an applet - similar to
        // ImageNode::associate in ImageNode.C.  I.O.W, we'll find our
        // image window.  We do that using javascript and the document
        // object model.
        // 3) Send that applet a message indicating that the
        // image is ready.
        // 4) Set resolution and aspect for the node to match the
        // values dictated by the new ImageWindow
        //
        protected boolean showImage ( String path )
        {
                Network net = this.getNetwork();
                DXApplication dxapp = this.getNetwork().getApplet();

                if ( this.iw == null ) {
                        this.associate( dxapp.getImageWindow( this.image_id ) );
                        if (dxapp.getImageWindow(this.image_id) == null) {
							System.err.println("this.iw is null and id is: " + this.image_id);
                        	System.err.println("ImageNode.showImage: ImageWindow still null - unable to display in applet.");
                        }
				}

                if ( this.iw == null ) {
                        AppletContext apcxt = dxapp.getAppletContext();

                        try {
                                URL url = new URL( dxapp.getCodeBase(), path );
                                apcxt.showDocument( url, this.getName() + "_" + this.getInstanceNumber() );

                        }

                        catch ( MalformedURLException mue ) {
                                return false;
                        }
                }

                else {
                        if ( this.orbit_mode == false ) {
                                if ( this.interaction_mode != this.iw.getInteractionMode() ) {
                                        long now = ( new Date() ).getTime();
                                        this.iw.setInteractionMode( this.interaction_mode, now );
                                }
                        }

                        else
                                this.iw.setInteractionMode( ImageWindow.ORBIT_MODE, 0 );

                        this.iw.showImage( path );
                }

                return true;
        }

        public void associate ( ImageWindow iw, int id )
        {
                this.image_id = id;
                this.associate ( iw );

                //
                // Activate the saved interaction mode if the input param
                // is unconnected
                //

                if ( this.isInputConnected( ImageNode.INTERACTIONMODE ) == false ) {
                        String imode = this.getInputValueString( ImageNode.INTERACTIONMODE );
                        DXApplication dxapp = this.getNetwork().getApplet();
                        ImageNode in = dxapp.getSelectedImageNode();

                        if ( ( imode != null ) && ( this.interaction_mode == ImageWindow.NO_MODE )
                                && ( in == this ) ) {
                                try {
                                        if ( imode.equals( "none" ) )
                                                dxapp.setInteractionMode( ImageWindow.NO_MODE, 0 );
                                        else if ( imode.equals( "pick" ) )
                                                dxapp.setInteractionMode( ImageWindow.PICK_MODE, 0 );
                                        else if ( imode.equals( "zoom" ) )
                                                dxapp.setInteractionMode( ImageWindow.ZOOM_MODE, 0 );
                                        else if ( imode.equals( "panzoom" ) )
                                                dxapp.setInteractionMode( ImageWindow.PAN_MODE, 0 );
                                        else if ( imode.equals( "rotate" ) )
                                                dxapp.setInteractionMode( ImageWindow.ROTATE_MODE, 0 );
                                }

                        catch ( Exception e ) {}

                        }

                }

                //
                // If our cache size is unset, then we'll see if the our new
                // image window is already caching something and use that cache
                // size.
                //
                if ( this.getCacheCount() == 0 ) {
                        int wcount = iw.getCacheSize();
                        this.setCacheSize( wcount );
                }
        }

        public void associate( ImageWindow iw )
        {
                if ( this.iw == iw ) return ;

                this.iw = iw;

                if ( this.iw == null ) return ;

                this.iw.associate( this );

                Network net = this.getNetwork();

                DXLinkApplication dxa = ( DXLinkApplication ) net.getApplet();

                boolean resume = dxa.getEocMode();

                if ( resume ) dxa.setEocMode( false );

                Dimension d = this.iw.getSize();

                this.setInputValue ( RESOLUTION, "" + d.width );

                this.setInputValue ( USEVECTOR, "1" );

                Double aspect = new Double( ( ( double ) d.height + 0.5 ) / ( double ) d.width );

                StringBuffer aspstr = new StringBuffer( aspect.toString() );

                if ( aspstr.length() > 8 )
                        aspstr.setLength( 8 );

                this.setInputValue ( ASPECT, new String( aspstr ) );

                if ( resume ) dxa.setEocMode( true );
        }

        protected boolean showWorld( String path )
        {
                Network net = this.getNetwork();
                DXApplication dxapp = this.getNetwork().getApplet();
                dxapp.showWorld( this.getName() + "_" + this.getInstanceNumber(),
                                 path, this.image_id );
                return true;
        }

        protected boolean showObject ( String path )
        {
                Network net = this.getNetwork();
                DXLinkApplication dxapp = ( DXLinkApplication ) this.getNetwork().getApplet();
                dxapp.showObject( this.getName() + "_" + this.getInstanceNumber(),
                                  path, this.image_id );
                return true;
        }


        //
        // This won't actually do anything until after the first execution
        // because ImageWindows aren't associated with ImageNodes until
        // the ImageNode gets a new picture.
        //
        public boolean setInteractionMode( int mode, long time )
        {
                //
                // sanity check
                //

				//System.out.println("IN::setInteractionNode: " + getTitle());

                if ( ( mode == ImageWindow.ROTATE_MODE ) ||
                        ( mode == ImageWindow.PICK_MODE ) ||
                        ( mode == ImageWindow.ZOOM_MODE ) ||
                        ( mode == ImageWindow.PAN_MODE ) ) {
                        if ( this.hasCamera() == false ) return false;
                }

                if ( mode == ImageWindow.PAN_MODE ) {
                        if ( this.isPerspective() ) return false;
                }

                if ( mode == ImageWindow.LOOP_MODE ) {
                	if ( this.getCacheCount() <= 1 ) return false;
                	if ( this.iw != null )
                    	if ( this.iw.getCacheSize() <= 1 ) 
                        	return false;
	                DXApplication dxapp = this.getNetwork().getApplet();
                    if ( dxapp.getCachingMode() == false ) return false;
                }

                if ( mode == this.interaction_mode ) return true;

                int old_mode = this.interaction_mode;

                this.interaction_mode = mode;

                DXApplication dxapp = this.getNetwork().getApplet();
                
                PickNode pn = dxapp.getSelectedPickNode();

                if ( ( this.iw != null ) && ( this.orbit_mode == false ) ) {
                	this.iw.setInteractionMode( this.interaction_mode, time );

                    if ( this.interaction_mode == ImageWindow.PICK_MODE ) {
                    	if ( pn != null ) {
                        	pn.setInputValueQuietly ( 2, "\"" + this.getMatchString() + "\"" );
                        }
                    }
                    else if ( this.interaction_mode == ImageWindow.ORBIT_MODE ) {
                    	this.orbit_mode = true;
                        this.setCacheSize( 9 );

                        if ( this.web_options_node != null )
                        	this.web_options_node.setOrbitMode( true );
                    }
				}

				if ( old_mode == ImageWindow.PICK_MODE ) {
					if ( pn != null ) {
						pn.setInputValueQuietly ( 2, "NULL" );
						pn.setInputValueQuietly ( 3, "NULL" );
						this.resetPickList();
					}
				}
				else if ( old_mode == ImageWindow.ORBIT_MODE ) {
					this.orbit_mode = false;
					if ( this.web_options_node != null )
                         this.web_options_node.setOrbitMode( false );
				}
                return true;
        }

        public void addPickLocation ( int x, int y )
        {
                String new_pick = "[" + x + ", " + y + "]}";

                if ( this.pick_list == null ) {
                        this.pick_list = "{" + new_pick;
                }

                else {
                        this.pick_list = this.pick_list.replace ( '}', ',' );
                        this.pick_list = this.pick_list + new_pick;
                }

                DXApplication dxapp = this.getNetwork().getApplet();
                PickNode pn = dxapp.getSelectedPickNode();

                if ( pn != null ) {
                        if ( this.iw != null )
                                this.iw.addPickLocation( x, y );

                        pn.setInputValue ( 3, this.pick_list );
                }
        }

        protected void resetPickList()
        {
                this.pick_list = null;

                if ( this.iw != null )
                        this.iw.resetPickList();

                DXApplication dxapp = this.getNetwork().getApplet();

                PickNode pn = dxapp.getSelectedPickNode();

                pn.setInputValueQuietly ( 3, "NULL" );
        }

        public double[] getTo()
        {
                return this.getVector( this.getInputValueString( ImageNode.TO ) );
        }

        public double[] getFrom()
        {
                return this.getVector( this.getInputValueString( ImageNode.FROM ) );
        }

        public double[] getUp()
        {
                return this.getVector( this.getInputValueString( ImageNode.UP ) );
        }

        public boolean hasCamera()
        {
                if ( this.getInputValueString( ImageNode.TO ) == null ) return false;

                if ( this.getInputValueString( ImageNode.FROM ) == null ) return false;

                if ( this.getInputValueString( ImageNode.UP ) == null ) return false;

                if ( ( this.getInputValueString( ImageNode.WIDTH ) == null ) &&
                        ( this.getInputValueString( ImageNode.VIEWANGLE ) == null ) )
                        return false;

                DXApplication dxapp = this.getNetwork().getApplet();

                if ( dxapp.isConnected() == false ) return false;

                return true;
        }

        public double getWidth()
        {
                String wstr = this.getInputValueString( ImageNode.WIDTH );
                Double dv = new Double( wstr );
                return dv.doubleValue();
        }

        public double getAngle()
        {
                Double dv = new Double( this.getInputValueString( ImageNode.VIEWANGLE ) );
                return dv.doubleValue();
        }

        private double[] getVector( String str )
        {
                double[] result = new double[ 4 ];
                int i = 0;
                StringTokenizer stok = new StringTokenizer( str, "[, ]" );

                while ( stok.hasMoreTokens() ) {
                        String tok = stok.nextToken();
                        Double dv = new Double( tok );
                        result[ i++ ] = dv.doubleValue();
                }

                if ( i < 3 ) result[ 3 ] = ( double ) 1.0;

                return result;
        }

        public void setTo( double vec[], boolean send )
        {
                String tostr = "[ " + ( new Float( vec[ 0 ] ) ).toString() +
                               ", " + ( new Float( vec[ 1 ] ) ).toString() +
                               ", " + ( new Float( vec[ 2 ] ) ).toString() + " ]";
                this.setInputValueString( ImageNode.TO, tostr );

                if ( send )
                        this.setInputValue ( ImageNode.TO, tostr );
                else
                        this.setInputValueQuietly ( ImageNode.TO, tostr );
        }

        public void setUp( double vec[], boolean send )
        {
                String upstr = "[ " + ( new Float( vec[ 0 ] ) ).toString() +
                               ", " + ( new Float( vec[ 1 ] ) ).toString() +
                               ", " + ( new Float( vec[ 2 ] ) ).toString() + " ]";
                this.setInputValueString( ImageNode.UP, upstr );

                if ( send )
                        this.setInputValue ( ImageNode.UP, upstr );
                else
                        this.setInputValueQuietly ( ImageNode.UP, upstr );
        }

        public void setFrom( double vec[], boolean send )
        {
                String fromstr = "[ " + ( new Float( vec[ 0 ] ) ).toString() +
                                 ", " + ( new Float( vec[ 1 ] ) ).toString() +
                                 ", " + ( new Float( vec[ 2 ] ) ).toString() + " ]";
                this.setInputValueString( ImageNode.FROM, fromstr );

                if ( send )
                        this.setInputValue ( ImageNode.FROM, fromstr );
                else
                        this.setInputValueQuietly ( ImageNode.FROM, fromstr );
        }

        public void setWidth ( double w, boolean send )
        {
                String wstr = ( new Float( w ) ).toString();
                this.setInputValueString( ImageNode.WIDTH, wstr );

                if ( send )
                        this.setInputValue ( ImageNode.WIDTH, wstr );
                else
                        this.setInputValueQuietly ( ImageNode.WIDTH, wstr );
        }

        public void setAngle ( double w, boolean send )
        {
                String astr = ( new Float( w ) ).toString();
                this.setInputValueString( ImageNode.VIEWANGLE, astr );

                if ( send )
                        this.setInputValue ( ImageNode.VIEWANGLE, astr );
                else
                        this.setInputValueQuietly ( ImageNode.VIEWANGLE, astr );
        }

        public void resetCamera ( boolean reset, boolean send )
        {
                String val = ( reset ? "1" : "0" );

                if ( this.isPerspective() ) {
                        if ( send ) {
                                this.setInputValue ( ImageNode.RESET_CAMERA, val );
                                this.setInputValue ( ImageNode.VIEWANGLE, "30.0" );
                        }

                        else {
                                this.setInputValueQuietly ( ImageNode.RESET_CAMERA, val );
                                this.setInputValueQuietly ( ImageNode.VIEWANGLE, "30.0" );
                        }
                }

                else {
                        if ( send ) this.setInputValue ( ImageNode.RESET_CAMERA, val );
                        else this.setInputValueQuietly ( ImageNode.RESET_CAMERA, val );
                }
        }


        public void addInputArc ( int input, Node src, int output )
        {
                super.addInputArc( input, src, output );

                if ( input == JAVA_OPTIONS ) {
                        this.web_options_node = ( WebOptionsNode ) src;
                        this.setGroupName( src.getInputValueString( ImageNode.WEB_GROUP_NAME ) );
                }
        }

        public boolean isInteractionModeConnected()
        {
                return this.isInputConnected( ImageNode.INTERACTIONMODE );
        }

        public void setView ( int direction, boolean send )
        {
                if ( this.iw != null )
                        this.iw.setView ( direction, send );
        }

        public String getMatchString()
        {
                DXLinkApplication dxa = ( DXLinkApplication ) this.getNetwork().getApplet();
                return dxa.getMacroName() + "_Image_" + this.getInstanceNumber();
        }

        public void enableJava()
        {
                super.enableJava();
                DXLinkApplication dxa = ( DXLinkApplication ) this.getNetwork().getApplet();
                this.setInputValueQuietly( ImageNode.IMAGETAG, "\"" + this.getMatchString() + "\"" );
        }

}

; // end ImageNode
