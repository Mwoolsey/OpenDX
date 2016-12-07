//

package dx.net;
import dx.client.AppletClient;
import java.util.*;
import java.applet.*;
import java.awt.*;
import java.awt.event.*;
import java.net.*;
import java.lang.Math;

public abstract class ImageWindow extends AppletClient 
	implements Runnable, KeyListener, MouseListener, MouseMotionListener
{

    public static final int NO_MODE = 0;
    public static final int ROTATE_MODE = 1;
    public static final int ORBIT_MODE = 2;
    public static final int PAN_MODE = 3;
    public static final int ZOOM_MODE = 4;
    public static final int LOOP_MODE = 5;
    public static final int PICK_MODE = 6;
    public static final int RESET_CAMERA = 7;
    public static final int LAST_MODE = 7;

    public static final int FRONT = 0;
    public static final int BACK = 1;
    public static final int TOP = 2;
    public static final int BOTTOM = 3;
    public static final int RIGHT = 4;
    public static final int LEFT = 5;
    public static final int DIAGONAL = 6;
    public static final int OFF_FRONT = 7;
    public static final int OFF_BACK = 8;
    public static final int OFF_TOP = 9;
    public static final int OFF_BOTTOM = 10;
    public static final int OFF_RIGHT = 11;
    public static final int OFF_LEFT = 12;
    public static final int OFF_DIAGONAL = 13;

    private static final int GRID_SIZE = 2;

    private final static double COS05 = 0.996;
    private final static double SIN15 = 0.259;
    private final static double COS15 = 0.966;
    private final static double SIN25 = 0.423;
    private final static double COS25 = 0.906;
    private final static double SIN45 = 0.707;
    private final static double SIN35 = 0.574;
    public final static double PI = 3.1415926;

    public static final int XAXIS = 1;
    public static final int YAXIS = 2;
    public static final int ZAXIS = 3;

    private String initial_node_name;

    //
    // Animation
    //
    private Thread animation = null;
    private long start_time;

    private ImageNode node = null;
    public void associate( ImageNode n )
    {
        this.node = n;
    }

    public Vector pick_list;

    private int interactionMode;
    private Point start_point;
    private Point pan_point;
    private Gnomon gnomon;
    private Point edge;
    private double aspect;

    public String getInitialNodeName( Network n )
    {
        DXApplication dxapp = n.getApplet();
        //System.out.println("ImageWindow::getInitialNodeName - " + this.initial_node_name);
        return dxapp.getMacroName() + "_" + this.initial_node_name;
    }

	public String getName() {
		return this.initial_node_name;
	}
	
    public static boolean IsGroupInteraction( int mode )
    {
        switch ( mode ) {
        case ImageWindow.NO_MODE:
        case ImageWindow.LOOP_MODE:
            return true;
        }

        return false;
    }

	public void mouseClicked(MouseEvent me) {
	}
	
	public void mouseEntered(MouseEvent me) {
	}
	
	public void mouseExited(MouseEvent me) {
	}
	
	public void mouseMoved(MouseEvent me) {
	}
	
    public void mouseDragged( MouseEvent evt )
    {
    	int x = evt.getX();
    	int y = evt.getY();
    	
    	//System.out.println("Drag event");
        if ( this.interactionMode == ImageWindow.ORBIT_MODE ) {
            this.orbitDrag( x, y );
            return;
        }

        else if ( this.interactionMode == ImageWindow.ROTATE_MODE ) {
            this.gnomon.doDrag( x, y );
            return;
        }

        else if ( this.interactionMode == ImageWindow.ZOOM_MODE ) {
            if ( this.node == null ) return;

            if ( x > this.edge.x ) return;

            if ( x < ( this.edge.x >> 1 ) ) return;

            this.rubberBand ( this.start_point );

            int newy = ( int ) ( this.aspect * x );

            this.start_point.setLocation( x, newy );

            this.rubberBand ( this.start_point );

            return;
        }

        else if ( this.interactionMode == ImageWindow.PAN_MODE ) {
            if ( this.node == null ) return;

            if ( x > this.edge.x ) return;

            if ( x < 0 ) return;

            if ( y > this.edge.y ) return;

            if ( y < 0 ) return;

            this.crosshair ( this.start_point );

            this.start_point.setLocation( x, y );

            this.crosshair ( this.start_point );

            return;
        }
    }

    public void mouseReleased( MouseEvent evt )
    {
    	//System.out.println("mouseReleased");
    	int x = evt.getX();
    	int y = evt.getY();
    	
        if ( this.interactionMode == ImageWindow.ROTATE_MODE ) {
            this.gnomon.doneDrag ();
            return;
        }

        else if ( this.interactionMode == ImageWindow.ZOOM_MODE ) {
            if ( this.node == null ) return;

            this.rubberBand ( this.start_point );

            int w = ( this.start_point.x - ( this.edge.x - this.start_point.x ) );

            float percent = ( float ) w / ( float ) this.edge.x;

            if ( ( evt.getModifiers() & InputEvent.CTRL_MASK ) != 0 )
                percent = ( float ) 1.0 + percent;

            if ( this.node.isPerspective() == false ) {
                double width = this.node.getWidth();
                width *= percent;
                this.node.setWidth( width, true );
            }

            else {
                double angle = this.node.getAngle();
                angle *= percent;
                this.node.setTo( this.node.getTo(), false );
                this.node.setFrom( this.node.getFrom(), false );
                this.node.setAngle( angle, true );
            }

            return;
        }

        else if ( this.interactionMode == ImageWindow.PAN_MODE ) {
            if ( this.node == null ) return;

            float percent = ( float ) 1.0 + ( float ) ( y - this.pan_point.y ) / ( float ) this.edge.y;

            double[] topt = this.node.getTo();
            double[] frompt = this.node.getFrom();
            double[] upvec = this.node.getUp();

            //
            // Pan along the upvector
            //
            double width = this.node.getWidth();
            double[] fullup = new double[ 3 ];
            double height = width * this.aspect;
            
            fullup[ 0 ] = upvec[ 0 ] * height;
            fullup[ 1 ] = upvec[ 1 ] * height;
            fullup[ 2 ] = upvec[ 2 ] * height;
            frompt[ 0 ] = ( frompt[ 0 ] - fullup[ 0 ] ) + ( fullup[ 0 ] * percent );
            frompt[ 1 ] = ( frompt[ 1 ] - fullup[ 1 ] ) + ( fullup[ 1 ] * percent );
            frompt[ 2 ] = ( frompt[ 2 ] - fullup[ 2 ] ) + ( fullup[ 2 ] * percent );
            topt[ 0 ] = ( topt[ 0 ] - fullup[ 0 ] ) + ( fullup[ 0 ] * percent );
            topt[ 1 ] = ( topt[ 1 ] - fullup[ 1 ] ) + ( fullup[ 1 ] * percent );
            topt[ 2 ] = ( topt[ 2 ] - fullup[ 2 ] ) + ( fullup[ 2 ] * percent );

            percent = ( float ) 1.0 + ( float ) ( x - this.pan_point.x ) / ( float ) this.edge.x;

            //
            // Pan along the horizontal axis which we manufacture
            // by crossing up and from
            //
            double[] fromvec = new double[ 4 ];

            fromvec[ 0 ] = frompt[ 0 ] - topt[ 0 ];
            fromvec[ 1 ] = frompt[ 1 ] - topt[ 1 ];
            fromvec[ 2 ] = frompt[ 2 ] - topt[ 2 ];
            fromvec[ 3 ] = ( double ) 0.0;

            double[] perp = ImageWindow.cross( upvec, fromvec );

            double[] fullover = new double[ 3 ];

            //
            // Normalize perp
            //
            double div = ( double ) ( Math.pow( perp[ 0 ], 2 ) +
                                      Math.pow( perp[ 1 ], 2 ) +
                                      Math.pow( perp[ 2 ], 2 ) );
            div = ( double ) Math.sqrt( div );
            perp[ 0 ] = perp[ 0 ] / div;
            perp[ 1 ] = perp[ 1 ] / div;
            perp[ 2 ] = perp[ 2 ] / div;
            fullover[ 0 ] = perp[ 0 ] * width;
            fullover[ 1 ] = perp[ 1 ] * width;
            fullover[ 2 ] = perp[ 2 ] * width;
            frompt[ 0 ] = ( frompt[ 0 ] + fullover[ 0 ] ) - ( fullover[ 0 ] * percent );
            frompt[ 1 ] = ( frompt[ 1 ] + fullover[ 1 ] ) - ( fullover[ 1 ] * percent );
            frompt[ 2 ] = ( frompt[ 2 ] + fullover[ 2 ] ) - ( fullover[ 2 ] * percent );
            topt[ 0 ] = ( topt[ 0 ] + fullover[ 0 ] ) - ( fullover[ 0 ] * percent );
            topt[ 1 ] = ( topt[ 1 ] + fullover[ 1 ] ) - ( fullover[ 1 ] * percent );
            topt[ 2 ] = ( topt[ 2 ] + fullover[ 2 ] ) - ( fullover[ 2 ] * percent );

            this.node.setTo( topt, false );
            this.node.setFrom( frompt, true );
        }
    }


    public void mousePressed( MouseEvent evt )
    {
    	//System.out.println("mousePressed");
    	int x = evt.getX();
    	int y = evt.getY();
    	
        Dimension d = this.getSize();
        this.edge.setLocation ( d.width, d.height );
        this.aspect = ( double ) d.height / ( double ) d.width;

        if ( ( evt.getModifiers() & InputEvent.SHIFT_MASK ) != 0 ) {
            try {
                this.getAppletContext().showDocument( new URL( this.getImageUrl() ), "temp" );
            }
            catch ( Exception e ) {}
            return;
        }

        else if ( this.interactionMode == ImageWindow.ORBIT_MODE ) {
            this.orbitDown ( x, y );
            return;
        }

        else if ( this.interactionMode == ImageWindow.PICK_MODE ) {
            if ( this.node == null ) return;

            //Dimension d = this.size();
            int actual_y = d.height - y;

            this.node.addPickLocation ( x, actual_y );

            return;
        }

        else if ( this.interactionMode == ImageWindow.ZOOM_MODE ) {
            if ( this.node == null ) return;

            int newy = ( int ) ( this.aspect * x );

            this.start_point.setLocation( x, newy );
            this.rubberBand ( this.start_point );

            return;
        }

        else if ( this.interactionMode == ImageWindow.ROTATE_MODE ) {
            if ( this.node == null ) return;

            this.gnomon.initDrag( evt.getModifiers() & InputEvent.CTRL_MASK, x, y );

            return;
        }

        else if ( this.interactionMode == ImageWindow.PAN_MODE ) {
            if ( this.node == null ) return;

            this.start_point.setLocation( x, y );
            this.pan_point = new Point( x, y );
            this.crosshair ( this.start_point );

        }

    }


    public int getInteractionMode()
    {
        return this.interactionMode;
    }

    public void setInteractionMode ( int mode, long time )
    {
        int old_mode = this.interactionMode;

        if ( ( mode >= ImageWindow.NO_MODE ) && ( mode <= ImageWindow.LAST_MODE ) )
            this.interactionMode = mode;

        if ( old_mode == ImageWindow.ROTATE_MODE )
            this.setRotateMode( false );

        if ( old_mode == ImageWindow.LOOP_MODE )
            this.setLoopMode( false, time );

        if ( mode == ImageWindow.ROTATE_MODE )
            this.setRotateMode( true );

        if ( mode == ImageWindow.LOOP_MODE )
            this.setLoopMode( true, time );

    }

    private void crosshair ( Point p )
    {
        Graphics g = this.getGraphics();
        g.setXORMode( Color.black );
        g.setColor( Color.white );
        g.drawLine ( p.x - 6, p.y, p.x + 6, p.y );
        g.drawLine ( p.x, p.y - 6, p.x, p.y + 6 );
    }

    private void rubberBand ( Point p )
    {
        Graphics g = this.getGraphics();
        int x = this.edge.x - p.x;
        int y = this.edge.y - p.y;
        int w = ( p.x - x );
        int h = ( p.y - y );
        g.setXORMode( Color.black );
        g.setColor( Color.white );
        g.drawRect ( x, y, w, h );
    }


    public void showImage ( String path )
    {
        this.requestImage( path );
    }

    //
    // In order to implement orbits
    //
    private int orbit_seq;
    private Vector images;
    private boolean using_image;
    private int orbit_x;
    private int orbit_y;
    private int orbit_frame;


    public ImageWindow()
    {
        super();
        this.orbit_seq = -1;
        this.images = null;
        this.using_image = false;
        this.initial_node_name = null;
        this.pick_list = null;
        this.interactionMode = ImageWindow.NO_MODE;
        this.gnomon = null;
        Date now = new Date();
        this.start_time = now.getTime();
        this.start_point = new Point();
        this.edge = new Point();
        this.aspect = 0.0;
    }

    public void setCurrentImage( int i )
    {
        try {
            Image img = ( Image ) this.images.elementAt( i );
            super.setCurrentImage ( img, this.getCurrentSequence(), this.getCurrentLoop() );

            if ( this.interactionMode == ImageWindow.ROTATE_MODE )
                if ( this.gnomon != null )
                    this.gnomon.setImage( img );
        }

    catch ( ArrayIndexOutOfBoundsException aibe ) {}
        catch ( NullPointerException npe ) {}

    }

    public synchronized void clearImageCache()
    {
        if ( this.images != null ) {
            images.removeAllElements();
            images = null;
        }
    }

    public synchronized int getCacheSize()
    {
        if ( this.images == null ) return 0;

        return this.images.size();
    }

    protected void setCurrentImage( Image i, int seq, int loop )
    {
        if ( this.interactionMode == ImageWindow.ORBIT_MODE ) {
            this.setOrbitImage ( i, seq, loop );
        }

        else {
            int cache_count = 0;
            boolean caching_enabled = false;

            if ( this.loadNextImage() == true ) {
                cache_count = loop + 1;
                caching_enabled = true;
            }

            else {
                try {
                    DXApplication dxapp = this.node.getNetwork().getApplet();
                    caching_enabled = dxapp.getCachingMode();
                }

                catch ( Exception npe ) {}

                if ( this.node != null ) {
                    cache_count = this.node.getCacheCount();
                }
            }

            if ( ( cache_count > 0 ) && ( caching_enabled ) ) {
                if ( this.images == null )
                    this.images = new Vector( cache_count );

                this.images.addElement( i );

                while ( cache_count < this.images.size() )
                    this.images.removeElementAt( 0 );
            }

            super.setCurrentImage( i, seq, loop );

            if ( this.interactionMode == ImageWindow.ROTATE_MODE )
                if ( this.gnomon != null )
                    this.gnomon.setImage( i );
        }
    }

    private void setOrbitImage( Image i, int seq, int loop )
    {
        if ( seq < this.orbit_seq ) return ;

        if ( seq > this.orbit_seq ) {
            if ( images != null )
                images.removeAllElements();

            images = null;

            this.orbit_frame = 0;

            super.setCurrentImage( i, seq, loop );
        }

        if ( images == null ) {
            images = new Vector( 10 );
        }

        images.addElement( ( Object ) i );
        this.orbit_seq = seq;
    }

    private boolean orbitDown( int x, int y )
    {
        this.orbit_x = x;
        this.orbit_y = y;
        return true;
    }

    private boolean orbitDrag( int x, int y )
    {
        int f = this.orbit_frame;

        if ( Math.abs( x - this.orbit_x ) > 10 ) {
            if ( ( x > this.orbit_x ) && ( f % 3 != 2 ) )
                f = f + 1;
            else if ( ( x < this.orbit_x ) && ( f % 3 != 0 ) )
                f = f - 1;

            this.orbit_x = x;
        }

        if ( Math.abs( y - this.orbit_y ) > 10 ) {
            if ( ( y > this.orbit_y ) && ( f / 3 != 2 ) )
                f = f + 3;
            else if ( ( y < this.orbit_y ) && ( f / 3 != 0 ) )
                f = f - 3;

            this.orbit_y = y;
        }

        if ( ( f != this.orbit_frame ) ) {
            try {
                Image im = ( Image ) images.elementAt( f );
                super.setCurrentImage ( im, this.orbit_seq, f );
                this.orbit_frame = f;
                repaint();
            }

            catch ( ArrayIndexOutOfBoundsException obe ) {}
            catch ( NullPointerException npe ) {}

        }

        return true;
    }

    //
    // If our ImageNode is known at compile time, then we connect to it
    // right away in order to update its resolution and aspect based on
    // our size.
    //
    public void init()
    {
    	addKeyListener( this );
    	addMouseListener( this );
    	addMouseMotionListener( this );
        super.init();
        this.initial_node_name = this.getParameter( "IMAGE_NODE" );
        String orbitstr = this.getParameter( "OPEN_IN_ORBIT_MODE" );

        if ( orbitstr != null ) {
            if ( Boolean.valueOf( orbitstr ).booleanValue() )
                this.setInteractionMode( ImageWindow.ORBIT_MODE, 0 );
        }
    }

    protected synchronized int getAnimateSpeed()
    {
        if ( this.node != null ) return this.node.getCacheSpeed();

        return 250;
    }

    protected void setLoopMode ( boolean onoroff, long start_time )
    {
        if ( onoroff ) {
            this.animate ( start_time );
        }

        else {
            if ( this.animation != null ) {
                this.animation.interrupt();
                this.animation = null;
            }

            try {
                Image i = ( Image ) this.images.lastElement();
                int s = this.getCurrentSequence();
                super.setCurrentImage ( i, s, this.getCurrentLoop() );
                repaint();
            }

            catch ( NullPointerException npe ) {}
            catch ( ArrayIndexOutOfBoundsException obe ) {}

        }

    }

    protected void animate( long start )
    {
        if ( this.animation != null ) {
            this.animation.interrupt();
            this.animation = null;
        }

        if ( this.images == null ) return ;

        this.start_time = start;

        this.animation = new Thread( this );

        this.animation.start();
    }

    public void interrupt()
    {
        if ( this.animation != null ) {
            this.animation.interrupt();
            this.animation = null;
        }
    }

    public void run()
    {
        boolean error = false;
        
        try {
            long target = this.start_time + this.getAnimateSpeed();

            while ( ( !error ) && ( this.images != null ) && ( this.images.size() > 1 ) ) {
                if ( this.node.getPalindromeMode() ) {
                    int i;
                    int size = this.images.size();

                    for ( i = size - 2; i >= 1; i-- ) {
                        Image im = ( Image ) this.images.elementAt( i );
                        super.setCurrentImage( im, getCurrentSequence(), 0 );
                        repaint();

                        try {
                            long now = ( new Date() ).getTime();
                            int time_to_wait = ( int ) ( target - now );

                            if ( time_to_wait > 0 )
                                Thread.sleep( time_to_wait );
                            else if ( time_to_wait < -10000 ) {
                                error = true;
                                break;
                            }
                        }

                        catch ( InterruptedException ie ) {
                            error = true;
                            break;
                        }

                        target += this.getAnimateSpeed();
                    }
                }

                Enumeration enum1 = this.images.elements();

                while ( ( !error ) && ( enum1.hasMoreElements() ) ) {
                    Image im = ( Image ) enum1.nextElement();
                    super.setCurrentImage( im, getCurrentSequence(), 0 );
                    repaint();

                    try {
                        long now = ( new Date() ).getTime();
                        int time_to_wait = ( int ) ( target - now );

                        if ( time_to_wait > 0 )
                            Thread.sleep( time_to_wait );
                        else if ( time_to_wait < -10000 ) {
                            error = true;
                            break;
                        }
                    }

                    catch ( InterruptedException ie ) {
                        error = true;
                        break;
                    }

                    target += this.getAnimateSpeed();
                }
            }
        }

        catch ( Exception e ) {
            //
            // We'll fall thru here if the user unchecks the
            // "Enable Animation" toggle while we're animating.
            //
        }
    }

    //
    // Should be called only from ImageNode.  There are 2 representations
    // of the set of picks: 1 is the spots in the image, the other is the
    // list in the node.  They must be in sync.
    //
    public void addPickLocation( int x, int y )
    {
        if ( this.pick_list == null )
            this.pick_list = new Vector( 4 );

        Dimension d = this.getSize();

        int actual_y = d.height - y;

        Point p = new Point( x, actual_y );

        this.pick_list.addElement( p );

        repaint();
    }

    public void paint( Graphics g )
    {
        super.paint( g );

        if ( this.interactionMode == ImageWindow.PICK_MODE ) {
            if ( this.pick_list == null ) return ;

            Enumeration enum1 = this.pick_list.elements();

            g.setColor( Color.white );

            while ( enum1.hasMoreElements() ) {
                Point p = ( Point ) enum1.nextElement();
                g.fill3DRect( p.x - 2, p.y - 2, 4, 4, true );
            }
        }

        else if ( this.interactionMode == ImageWindow.ROTATE_MODE ) {
            this.gnomon.repaint();
        }
    }

    public void resetPickList()
    {
        this.pick_list = null;
        repaint();
    }

    protected void setRotateMode( boolean onoroff )
    {
        if ( this.gnomon == null ) {
            if ( onoroff )
                this.createGnomon();
        }

        if ( onoroff ) {
            this.gnomon.setImage ( this.getCurrentImage() );
            this.add( this.gnomon );
        }

        else
            if ( this.gnomon != null )
                this.remove( this.gnomon );
    }

    private void createGnomon()
    {
        this.setLayout( null );
        Dimension dim = this.getSize();
        this.gnomon = new Gnomon ( this.node, dim.width, dim.height );
        int x = dim.width - 95;
        int y = dim.height - 95;
        this.gnomon.setBounds ( x, y, 90, 90 );
        this.gnomon.setImage( this.getCurrentImage() );
    }


    public static double[] normalize ( double[] f, double[] t )
    {
        double dist = Math.pow( ( f[ 0 ] - t[ 0 ] ), 2 );
        dist += Math.pow( ( f[ 1 ] - t[ 1 ] ), 2 );
        dist += Math.pow( ( f[ 2 ] - t[ 2 ] ), 2 );
        dist = Math.sqrt( dist );
        double[] norm = new double[ 3 ];

        norm[ 0 ] = ( f[ 0 ] - t[ 0 ] ) / dist;
        norm[ 1 ] = ( f[ 1 ] - t[ 1 ] ) / dist;
        norm[ 2 ] = ( f[ 2 ] - t[ 2 ] ) / dist;
        return norm;
    }

    //    private static void printMat(double[][] mat) {
    //
    // int i;
    // for (i=0; i<4; i++) {
    //     System.out.println
    //     ("   | " +mat[0][i]+ " " +mat[1][i]+ " " +mat[2][i]+ " " +mat[3][i]+ " |");
    // }
    //    }

    //
    // L I N E A R    A L G E B R A
    // L I N E A R    A L G E B R A
    // L I N E A R    A L G E B R A
    //

    //
    // The Rolling Ball algorithm from Graphics Gems.
    // N.B.  Always translate a point towards 0,0,0 before applying
    // a transformation matrix.  Otherwise you get big rounding problems.
    //
    public static double[][] getTransform( double[] f, double[] t, double theta )
    {
        double[] ndir = ImageWindow.normalize ( f, t );

        double[][] T = new double[ 4 ][ 4 ];
        ImageWindow.I44( T );

        double cost = ( double ) Math.cos( theta );
        double sint = ( double ) Math.sin( theta );

        T[ 0 ][ 0 ] = cost + ( Math.pow( ndir[ 0 ], 2 ) * ( 1.0 - cost ) );
        T[ 1 ][ 1 ] = cost + ( Math.pow( ndir[ 1 ], 2 ) * ( 1.0 - cost ) );
        T[ 2 ][ 2 ] = cost + ( Math.pow( ndir[ 2 ], 2 ) * ( 1.0 - cost ) );

        T[ 0 ][ 1 ] = ( ndir[ 0 ] * ndir[ 1 ] * ( 1.0 - cost ) ) + ( ndir[ 2 ] * sint );
        T[ 1 ][ 0 ] = ( ndir[ 0 ] * ndir[ 1 ] * ( 1.0 - cost ) ) - ( ndir[ 2 ] * sint );

        T[ 2 ][ 1 ] = ( ndir[ 1 ] * ndir[ 2 ] * ( 1.0 - cost ) ) - ( ndir[ 0 ] * sint );
        T[ 1 ][ 2 ] = ( ndir[ 1 ] * ndir[ 2 ] * ( 1.0 - cost ) ) + ( ndir[ 0 ] * sint );

        T[ 2 ][ 0 ] = ( ndir[ 0 ] * ndir[ 2 ] * ( 1.0 - cost ) ) + ( ndir[ 1 ] * sint );
        T[ 0 ][ 2 ] = ( ndir[ 0 ] * ndir[ 2 ] * ( 1.0 - cost ) ) - ( ndir[ 1 ] * sint );

        return T;
    }

    //
    // Multiply a 4-vector and a 4x4 matrix
    //
    public static double[] Rotate( double vec[], double mat[][] )
    {
        int i, j, k;
        double result[] = new double[ 4 ];

        for ( i = 0; i < 4; i++ ) result[ i ] = ( double ) 0.0;

        for ( k = 0; k < 4; k++ ) {
            for ( i = 0; i < 4; i++ ) {
                result[ k ] += vec[ i ] * mat[ i ][ k ];
            }
        }

        return result;
    }

    private static void I44 ( double w[][] )
    {
        int i, j;

        for ( i = 0; i < 4; i++ )
            for ( j = 0; j < 4; j++ )
                if ( i == j ) w[ i ][ j ] = ( double ) 1.0;
                else w[ i ][ j ] = ( double ) 0.0;
    }

    public static double[] cross ( double v[], double w[] )
    {
        double[] res = new double[ 4 ];
        res[ 0 ] = ( v[ 1 ] * w[ 2 ] ) - ( v[ 2 ] * w[ 1 ] );
        res[ 1 ] = ( v[ 2 ] * w[ 0 ] ) - ( v[ 0 ] * w[ 2 ] );
        res[ 2 ] = ( v[ 0 ] * w[ 1 ] ) - ( v[ 1 ] * w[ 0 ] );
        res[ 3 ] = ( double ) 1.0;
        return res;
    }

    public static double dot ( float v[], double w[] )
    {
        int i = v.length;

        if ( i != w.length ) return 0.0;

        double res = 0.0;

        int j;

        for ( j = 0; j < i; j++ )
            res += ( v[ j ] * w[ j ] );

        return res;
    }

    public void keyPressed ( KeyEvent e )
    {
    	int key = e.getKeyCode();
    	int m = e.getModifiers();
    	
        if ( ( this.node != null ) ) {
            boolean retval = false;

	        if ( key == KeyEvent.VK_E && ( m & InputEvent.CTRL_MASK ) > 0 ) {
    	        retval = true;
    	    }
    	    else if ( key == KeyEvent.VK_R && ( m & InputEvent.CTRL_MASK ) > 0 ) {
    	        retval = true;
    	    }
    	    else if ( key == KeyEvent.VK_F && ( m & InputEvent.CTRL_MASK ) > 0 ) {
				retval = true;
			}
    	    else if ( key == KeyEvent.VK_TAB && ( m & InputEvent.CTRL_MASK ) > 0 ) {
    	        retval = true;
    	    }
    	    else if ( key == KeyEvent.VK_SPACE && ( m & InputEvent.CTRL_MASK ) > 0 ) {
    	        retval = true;
    	    }
    	    else if ( key == KeyEvent.VK_C && ( m & InputEvent.CTRL_MASK ) > 0 ) {
    	        retval = true;
    	    }

            if ( retval ) {
                DXApplication dxapp = this.node.getNetwork().getApplet();
                dxapp.updateSelectedImageNode( e, this.node );
            }
        }
    }
    
    public void keyReleased ( KeyEvent e ) {
    }
    
    public void keyTyped ( KeyEvent e ) {
    }

    public void setView ( int direction, boolean send )
    {
        double[] frompt = this.node.getFrom();
        double[] up = this.node.getUp();
        double[] topt = this.node.getTo();
        double[] dir = new double[ 3 ];
        int X = 0;
        int Y = 1;
        int Z = 2;
        dir[ X ] = frompt[ 0 ] - topt[ 0 ];
        dir[ Y ] = frompt[ 1 ] - topt[ 1 ];
        dir[ Z ] = frompt[ 2 ] - topt[ 2 ];
        double length = Math.sqrt ( dir[ X ] * dir[ X ] + dir[ Y ] * dir[ Y ] + dir[ Z ] * dir[ Z ] );
        up[ X ] = 0.0;
        up[ Y ] = 1.0;
        up[ Z ] = 0.0;

        switch ( direction ) {
        case ImageWindow.FRONT:
            dir[ X ] = 0.0; dir[ Y ] = 0.0; dir[ Z ] = 1.0;
            break;
        case ImageWindow.OFF_FRONT:
            dir[ X ] = dir[ Y ] = ImageWindow.SIN15; dir[ Z ] = ImageWindow.COS15;
            break;
        case ImageWindow.BACK:
            dir[ X ] = 0.0; dir[ Y ] = 0.0; dir[ Z ] = -1.0;
            break;
        case ImageWindow.OFF_BACK:
            dir[ X ] = dir[ Y ] = ImageWindow.SIN15; dir[ Z ] = -ImageWindow.COS15;
            break;
        case ImageWindow.TOP:
            dir[ X ] = 0.0; dir[ Y ] = 1.0; dir[ Z ] = 0.0;
            up[ X ] = up[ Y ] = 0.0; up[ Z ] = -1.0;
            break;
        case ImageWindow.OFF_TOP:
            dir[ X ] = dir[ Z ] = -ImageWindow.SIN15; dir[ Y ] = ImageWindow.COS15;
            up[ X ] = up[ Y ] = 0.0; up[ Z ] = -1.0;
            break;
        case ImageWindow.BOTTOM:
            dir[ X ] = 0.0; dir[ Y ] = -1.0; dir[ Z ] = 0.0;
            up[ X ] = up[ Y ] = 0.0; up[ Z ] = 1.0;
            break;
        case ImageWindow.OFF_BOTTOM:
            dir[ X ] = dir[ Z ] = ImageWindow.SIN15; dir[ Y ] = -ImageWindow.COS15;
            up[ X ] = up[ Y ] = 0.0; up[ Z ] = 1.0;
            break;
        case ImageWindow.RIGHT:
            dir[ X ] = 1.0; dir[ Y ] = 0.0; dir[ Z ] = 0.0;
            break;
        case ImageWindow.OFF_RIGHT:
            dir[ X ] = ImageWindow.COS05; dir[ Y ] = dir[ Z ] = ImageWindow.SIN15;
            break;
        case ImageWindow.LEFT:
            dir[ X ] = -1.0; dir[ Y ] = 0.0; dir[ Z ] = 0.0;
            break;
        case ImageWindow.OFF_LEFT:
            dir[ X ] = -ImageWindow.COS05; dir[ Y ] = dir[ Z ] = ImageWindow.SIN15;
            break;
        case ImageWindow.DIAGONAL:
            dir[ X ] = dir[ Y ] = dir[ Z ] = ImageWindow.SIN45;
            break;
        case ImageWindow.OFF_DIAGONAL:
            dir[ X ] = dir[ Y ] = dir[ Z ] = ImageWindow.SIN35;
            break;
        }

        double tmp_length = Math.sqrt ( dir[ X ] * dir[ X ] + dir[ Y ] * dir[ Y ] + dir[ Z ] * dir[ Z ] );
        dir[ X ] = dir[ X ] / tmp_length;
        dir[ Y ] = dir[ Y ] / tmp_length;
        dir[ Z ] = dir[ Z ] / tmp_length;

        dir[ X ] *= length;
        dir[ Y ] *= length;
        dir[ Z ] *= length;

        frompt[ X ] = dir[ X ] + topt[ X ];
        frompt[ Y ] = dir[ Y ] + topt[ Y ];
        frompt[ Z ] = dir[ Z ] + topt[ Z ];

        this.node.setFrom( frompt, false );
        this.node.setUp( up, send );
    }

} // end ImageWindow
