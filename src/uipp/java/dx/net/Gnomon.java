//


/*
 * $Header: /src/master/dx/src/uipp/java/dx/net/Gnomon.java,v 1.4 2005/10/27 19:43:06 davidt Exp $
 */

package dx.net;
import java.awt.*;
import java.net.*;
import java.io.*;

// I do believe that the previous handleEvent didn't do anything. If it did,
// replace it with a MouseListener. Will need some testing.

public class Gnomon extends Canvas 
//	implements MouseListener, MouseMotionListener
{

        private ImageNode node;
        private Image bg;
        private float xaxis[];
        private float yaxis[];
        private float zaxis[];
        private int ixaxis[];
        private int iyaxis[];
        private int izaxis[];
        private double upvec[];
        private double frompt[];
        private double topt[];
        private final static byte[] Xbytes = ( new String( "X" ) ).getBytes();
        private final static byte[] Ybytes = ( new String( "Y" ) ).getBytes();
        private final static byte[] Zbytes = ( new String( "Z" ) ).getBytes();
        private int x, y;
        private int iww, iwh;
        private Point start_point;
        private boolean button_down;
        private boolean just_pushed;
        private boolean xydrag;

        public void setImage ( Image im )
        {
                this.bg = im;
                this.setup();
                this.setBackground( Color.black );
                repaint();
        }

        public Gnomon( ImageNode node, int ww, int wh )
        {
                super();
                this.node = node;
                this.bg = null;
                this.xaxis = null;
                this.yaxis = null;
                this.zaxis = null;
                this.x = this.y = 0;
                this.iww = ww;
                this.iwh = wh;
                this.button_down = false;
                this.just_pushed = false;
                this.xydrag = false;
                this.ixaxis = new int[ 2 ];
                this.iyaxis = new int[ 2 ];
                this.izaxis = new int[ 2 ];
                this.start_point = new Point();
//                addMouseListener(this);
//                addMouseMotionListener(this);
        }

        public void setBounds ( int x, int y, int w, int h )
        {
                super.setBounds ( x, y, w, h );
                this.x = x;
                this.y = y;
        }

//		public void mouseDragged(MouseEvent me) {
//			this.processMouseEvent(me);
//		}

//        public boolean handleEvent ( Event e )
//        {
//                if ( e.id == Event.MOUSE_DRAG )
//                        return this.mouseDrag ( e, e.x, e.y );
//                else
//                        return super.handleEvent( e );
//        }

        public void doneDrag ()
        {
                this.button_down = false;
                this.node.setTo ( this.topt, false );
                this.node.setFrom ( this.frompt, false );
                this.node.setUp ( this.upvec, true );
        }

        public void doDrag ( int x, int y )
        {
                if ( this.xydrag ) {
                        float halfway = this.iww >> 1;
                        float percent = ( x + halfway - this.start_point.x ) / halfway;
                        this.drag ( percent, ImageWindow.YAXIS );
                        halfway = this.iwh >> 1;
                        percent = ( y + halfway - this.start_point.y ) / halfway;
                        this.drag ( percent, ImageWindow.XAXIS );
                        this.start_point.setLocation( x, y );
                }

                else {
                        float halfway = this.iwh >> 1;
                        float percent = ( y + halfway - this.start_point.y ) / halfway;
                        this.drag ( percent, ImageWindow.ZAXIS );
                        this.start_point.setLocation( x, y );
                }
        }

        public void initDrag ( int ctrl_down, int x, int y )
        {
                this.just_pushed = true;
                this.button_down = true;
                this.start_point.setLocation( x, y );

                if ( ctrl_down == 0 )
                        this.xydrag = true;
                else
                        this.xydrag = false;
        }

        public void update( Graphics g )
        {
                paint( g );
        }

        //
        // If the mouse button is down, do XOR drawing otherwise drag the
        // gnomon in a way which ensures visibility
        //
        public void paint( Graphics g )
        {
                super.paint( g );
                Dimension dim = this.getSize();
                int x = dim.width >> 1;
                int y = dim.height >> 1;
                g.setPaintMode();
                Dimension d = this.getSize();

                if ( this.bg != null )
                        g.drawImage( this.bg, 0, 0, d.width, d.height,
                                     this.x, this.y, ( this.x + d.width ), ( this.y + d.height ),
                                     Color.black, this );


                if ( this.xaxis != null ) {
                        g.setColor( Color.black );
                        g.drawLine ( x + 1, y, this.ixaxis[ 0 ] + 1, this.ixaxis[ 1 ] );
                        g.drawLine ( x + 1, y, this.iyaxis[ 0 ] + 1, this.iyaxis[ 1 ] );
                        g.drawLine ( x + 1, y, this.izaxis[ 0 ] + 1, this.izaxis[ 1 ] );

                        g.setColor( Color.white );
                        g.drawLine ( x, y, this.ixaxis[ 0 ], this.ixaxis[ 1 ] );
                        g.drawLine ( x, y, this.iyaxis[ 0 ], this.iyaxis[ 1 ] );
                        g.drawLine ( x, y, this.izaxis[ 0 ], this.izaxis[ 1 ] );
                        this.labelAxis ( g, Gnomon.Xbytes, x, y, this.ixaxis[ 0 ], this.ixaxis[ 1 ] );
                        this.labelAxis ( g, Gnomon.Ybytes, x, y, this.iyaxis[ 0 ], this.iyaxis[ 1 ] );
                        this.labelAxis ( g, Gnomon.Zbytes, x, y, this.izaxis[ 0 ], this.izaxis[ 1 ] );
                }
        }

        private void labelAxis ( Graphics g, byte[] b, int x, int y, int x1, int y1 )
        {
                int tx, ty;

                if ( ( x <= x1 ) && ( y <= y1 ) ) {
                        // SouthEast
                        tx = x1 + 4;
                        ty = y1 + 4;
                }

                else if ( ( x <= x1 ) && ( y > y1 ) ) {
                        // NorthEast
                        tx = x1 + 4;
                        ty = y1;
                }

                else if ( ( x > x1 ) && ( y <= y1 ) ) {
                        // SouthWest
                        ty = y1 + 4;
                        tx = x1 - 8;
                }

                else if ( ( x > x1 ) && ( y > y1 ) ) {
                        // NorthWest
                        tx = x1 + 4;
                        ty = y1 - 4;
                }

                else {
                        tx = x;
                        ty = y;
                }

                g.drawBytes ( b, 0, 1, tx, ty );
        }

        private void drag( float percent, int axis )
        {
                double[] zero = new double[ 3 ];
                zero[ 0 ] = ( double ) 0.0; zero[ 1 ] = ( double ) 0.0; zero[ 2 ] = ( double ) 0.0;
                double full_scale = ImageWindow.PI / ( double ) 2.0;
                double theta = full_scale - ( percent * full_scale );
                double xform[][] = null;

                if ( axis == ImageWindow.XAXIS ) {
                        double[] fromvec = new double[ 4 ];
                        fromvec[ 0 ] = this.frompt[ 0 ] - this.topt[ 0 ];
                        fromvec[ 1 ] = this.frompt[ 1 ] - this.topt[ 1 ];
                        fromvec[ 2 ] = this.frompt[ 2 ] - this.topt[ 2 ];
                        fromvec[ 3 ] = ( double ) 0.0;
                        double[] perp = ImageWindow.cross( this.upvec, fromvec );
                        xform = ImageWindow.getTransform ( perp, zero, theta );

                        //
                        // Always translate the point being transformed to 0,0,0
                        // before applying the transformation matrix.  Then translate
                        // it back after the transformation.
                        //
                        frompt[ 0 ] -= this.topt[ 0 ];
                        frompt[ 1 ] -= this.topt[ 1 ];
                        frompt[ 2 ] -= this.topt[ 2 ];
                        double from_rotated[] = ImageWindow.Rotate ( frompt, xform );
                        from_rotated[ 0 ] += this.topt[ 0 ];
                        from_rotated[ 1 ] += this.topt[ 1 ];
                        from_rotated[ 2 ] += this.topt[ 2 ];

                        this.frompt = from_rotated;
                        this.upvec = ImageWindow.Rotate( upvec, xform );
                }

                else if ( axis == ImageWindow.YAXIS ) {
                        xform = ImageWindow.getTransform ( this.upvec, zero, theta );
                        //
                        // Always translate the point being transformed to 0,0,0
                        // before applying the transformation matrix.  Then translate
                        // it back after the transformation.
                        //
                        frompt[ 0 ] -= this.topt[ 0 ];
                        frompt[ 1 ] -= this.topt[ 1 ];
                        frompt[ 2 ] -= this.topt[ 2 ];
                        double from_rotated[] = ImageWindow.Rotate( frompt, xform );
                        from_rotated[ 0 ] += this.topt[ 0 ];
                        from_rotated[ 1 ] += this.topt[ 1 ];
                        from_rotated[ 2 ] += this.topt[ 2 ];
                        this.frompt = from_rotated;
                }

                else if ( axis == ImageWindow.ZAXIS ) {
                        xform = ImageWindow.getTransform( this.frompt, this.topt, -theta );
                        this.upvec = ImageWindow.Rotate( this.upvec, xform );
                }

                this.recompute();

                if ( this.button_down == false )
                        repaint();
        }

        private void setup ()
        {
                this.frompt = this.node.getFrom();
                this.topt = this.node.getTo();
                this.upvec = this.node.getUp();

                if ( this.xaxis == null ) {
                        this.xaxis = new float[ 4 ];
                        this.yaxis = new float[ 4 ];
                        this.zaxis = new float[ 4 ];
                }

                this.recompute();
        }

        private void recompute()
        {
                Dimension d = this.getSize();
                int ox = d.width >> 1;
                int oy = d.height >> 1;

                if ( this.button_down ) {
                        Graphics g = this.getGraphics();

                        if ( this.just_pushed == true ) {
                                float[] tmp = this.xaxis;
                                this.xaxis = null;
                                this.paint( g );
                                this.xaxis = tmp;
                        }

                        else {
                                g.setColor( Color.white );
                                g.setXORMode( Color.black );
                                g.drawLine ( ox, oy, this.ixaxis[ 0 ], this.ixaxis[ 1 ] );
                                g.drawLine ( ox, oy, this.iyaxis[ 0 ], this.iyaxis[ 1 ] );
                                g.drawLine ( ox, oy, this.izaxis[ 0 ], this.izaxis[ 1 ] );
                                this.labelAxis ( g, Xbytes, ox, oy, this.ixaxis[ 0 ], this.ixaxis[ 1 ] );
                                this.labelAxis ( g, Ybytes, ox, oy, this.iyaxis[ 0 ], this.iyaxis[ 1 ] );
                                this.labelAxis ( g, Zbytes, ox, oy, this.izaxis[ 0 ], this.izaxis[ 1 ] );
                        }

                        this.just_pushed = false;
                }

                double[] ndir = ImageWindow.normalize ( this.frompt, this.topt );
                double[] horiz = ImageWindow.cross ( this.upvec, ndir );

                for ( int i = 0; i < 4; i++ )
                        this.xaxis[ i ] = this.yaxis[ i ] = this.zaxis[ i ] = ( float ) 0.0;

                this.zaxis[ 2 ] = -( this.xaxis[ 0 ] = this.yaxis[ 1 ] = ( float ) 0.8 );

                float dx, dy;

                dx = ox + ( ox * ( float ) ImageWindow.dot ( this.xaxis, horiz ) );

                dy = oy - ( ox * ( float ) ImageWindow.dot ( this.xaxis, upvec ) );

                this.ixaxis[ 0 ] = ( int ) ( this.xaxis[ 0 ] = dx );

                this.ixaxis[ 1 ] = ( int ) ( this.xaxis[ 1 ] = dy );

                dx = ox + ( ox * ( float ) ImageWindow.dot ( this.yaxis, horiz ) );

                dy = oy - ( ox * ( float ) ImageWindow.dot ( this.yaxis, upvec ) );

                this.iyaxis[ 0 ] = ( int ) ( this.yaxis[ 0 ] = dx );

                this.iyaxis[ 1 ] = ( int ) ( this.yaxis[ 1 ] = dy );

                dx = ox + ( ox * ( float ) ImageWindow.dot ( this.zaxis, horiz ) );

                dy = oy - ( ox * ( float ) ImageWindow.dot ( this.zaxis, upvec ) );

                this.izaxis[ 0 ] = ( int ) ( this.zaxis[ 0 ] = dx );

                this.izaxis[ 1 ] = ( int ) ( this.zaxis[ 1 ] = dy );

                if ( this.button_down ) {
                        Graphics g = this.getGraphics();
                        g.setColor( Color.white );
                        g.setXORMode( Color.black );
                        g.drawLine ( ox, oy, this.ixaxis[ 0 ], this.ixaxis[ 1 ] );
                        g.drawLine ( ox, oy, this.iyaxis[ 0 ], this.iyaxis[ 1 ] );
                        g.drawLine ( ox, oy, this.izaxis[ 0 ], this.izaxis[ 1 ] );
                        this.labelAxis ( g, Xbytes, ox, oy, this.ixaxis[ 0 ], this.ixaxis[ 1 ] );
                        this.labelAxis ( g, Ybytes, ox, oy, this.iyaxis[ 0 ], this.iyaxis[ 1 ] );
                        this.labelAxis ( g, Zbytes, ox, oy, this.izaxis[ 0 ], this.izaxis[ 1 ] );
                }
        }

}

; // end Gnomon
