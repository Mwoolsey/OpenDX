///////////////////////////////////////////////////////////////////////
//                        DX SOURCEFILE                              //
///////////////////////////////////////////////////////////////////////

/*
 * $Header: /src/master/dx/src/uipp/java/dx/runtime/Interactor.java,v 1.4 2006/03/28 20:07:53 davidt Exp $
 */

//-----------------------------------------------------
// 3/17/2006 jer
// Changed Interactor behavior so that the user does
// not have to hit ENTER in the TextField in order
// for the input to be seen.
//-----------------------------------------------------

package dx.runtime;
import dx.net.PacketIF;
import dx.net.DXLinkApplication;
import layout.TableLayout;
import java.awt.*;
import java.awt.event.*;
import java.applet.*;

public class Interactor extends Panel
            implements KeyListener, FocusListener, ActionListener	// 3/17/2006 jer
{
    //
    // private member data
    //
    private LabelGroup label = null;
    private int developer = 1;
    private int label_lines = 1;
    private GridBagLayout gbl;
    private boolean requires_quotes = true;
    int x = 0;
    int y = 0;

    //
    // protected member data
    //
    protected int vertical = 1;
    protected int orientation = LEFT;
    protected Component ipart = null;
    protected int instance_number;
    protected dx.net.PacketIF node = null;
    protected Insets insets = new Insets(4, 4, 4, 4);

    public static int LEFT = 1;
    public static int RIGHT = 0;

    public void setNode( PacketIF node )
    {
        this.node = node;
    }

    public PacketIF getNode()
    {
        return this.node;
    }

    public void setInstanceNumber( int instnum )
    {
        this.instance_number = instnum;
    }

    public int getInstanceNumber()
    {
        return this.instance_number;
    }

    //--------------------------------------------------
    // BEGIN: 3/17/2006 jer
    //
    // Catch key events. If user hits enter,
    // change the value, loudly. Otherwise, don't
    // do anything (yet).
    //
    public void keyPressed(KeyEvent e){
        PacketIF pif = this.getNode();
        if(e.getKeyCode() == KeyEvent.VK_ENTER)
            pif.setOutputValue( getValue() );
    }
public void keyReleased(KeyEvent e){ }
    public void keyTyped(KeyEvent e){ }

    //----
    // Catch keybd focus events. If focus is lost,
    // change value quietly, i.e., make the assignment
    // without causing an execution.
    //
    public void focusLost(FocusEvent e){
        PacketIF pif = this.getNode();
        pif.setOutputValueQuietly( getValue() );
    }
    public void focusGained(FocusEvent e){ }

    //----
    // Catch action events. (Not sure if this
    // is really necessary, but just to be safe.)
    // This replaces the old "action" method.
    //
    public void actionPerformed(ActionEvent e){
        PacketIF pif = this.getNode();
        pif.setOutputValue( getValue() );
    }

    // DELETED: action(Event e){ ... }

    // END: 3/17/2006 jer
    //--------------------------------------------------

    //
    // By default we'll create a text widget for the interactor.  This should
    // probably be a virtual method, but there will be so may interactors for
    // which there is no java translation that we want to be able to use
    // a plain text widget by default
    //
    protected void createPart()
    {
        ipart = new TextField( "", 15 );

        //------------------------------------------
        // BEGIN: 3/17/2006 jer
        ipart.addKeyListener(this);
        ipart.addFocusListener(this);
        ((TextField)ipart).addActionListener(this);
        // END: 3/17/2006 jer
        //------------------------------------------

        if ( this.developer == 1 ) {
            ipart.setBackground( Color.white );
            ipart.setForeground( Color.black );
        }
    }

    public void setLabelLines( int lines )
    {
        label_lines = lines;
    }

    public void setStyle ( int dev )
    {
        developer = dev;
    }

    public int getStyle()
    {
        return developer;
    }


    //
    //
    //
    public Component getComponent()
    {
        return ipart;
    }

    //
    //
    //
    protected String getTypeName()
    {
        return "Value";
    }

    //
    // If the subclass supplies a createPart method then it must supply a getValue also
    //
    public String getValue()
    {
        TextField tf = ( TextField ) ipart;

        if ( requires_quotes ) return "\"" + tf.getText() + "\"";
        else return tf.getText();
    }

    public void setUseQuotes( boolean usem )
    {
        requires_quotes = usem;
    }

    //
    // If the subclass supplies a createPart method then it must supply a setValue also
    //
    public void setValue( String s )
    {
        TextField tf = ( TextField ) ipart;
        System.err.println("Setting: " + s);
        tf.setText( s );
    }

    //
    //
    //

    public Insets getLabelInsets()
    {
        if ( vertical == 1 )
            return new Insets ( 1, 0, 0, 0 );
        else
            return new Insets ( 1, 1, 1, 0 );
    }

    public Insets getComponentInsets()
    {
        if ( vertical == 1 )
            return new Insets ( 0, 0, 3, 0 );
        else
            return new Insets ( 2, 0, 2, 2 );
    }

    public Insets getInsets() {
        return insets;
    }

    public int getWeightY()
    {
        return 0;
    }

    public int getWeightX()
    {
        return 0;
    }

    //
    //
    //
    public void init()
    {
        //gbl = new GridBagLayout();
        //setLayout( gbl );

        double p = TableLayout.PREFERRED;
        double f = TableLayout.FILL;
        TableLayout tbl;
        if(vertical == 1) {
            if(this instanceof ListSelector) {
                double size[][] = {{f}, {2,p,2,f,2}};
                tbl = new TableLayout(size);
            } else {
                double size[][] = {{f}, {0.5,p,2,p,0.5}};
                tbl = new TableLayout(size);
            }
        }
        else {
            if(getTypeName().compareTo("Toggle") == 0 && orientation != LEFT) {
                double size[][] = {{0.5, 18, p, 0.5},{f}};
                tbl = new TableLayout(size);
            } else {
                double size[][] = {{0.5, p, p, 0.5},{f}};
                tbl = new TableLayout(size);
            }
        }
        setLayout( tbl );

        if ( developer == 1 ) {
            setBackground( new Color( 0x5f, 0x9e, 0xa0 ) );
            setForeground( Color.white );
        }

        try {
            if ( label_lines > 0 ) {
                if ( label == null )
                    label = new LabelGroup( label_lines );

                //label.setFont( new Font("Default", Font.BOLD, 10 ) );
                label.setFont( new Font( "Helvetica", Font.BOLD, 10 ) );
                label.init();

                if ( developer == 1 ) {
                    label.setBackground( new Color( 0x5f, 0x9e, 0xa0 ) );
                    label.setForeground( Color.white );
                }

                if ( vertical == 1 ) {
                    label.setAlignment( Label.CENTER );
                    add(label, "0, 1, c, b");
                }

                else {
                    label.setAlignment( Label.LEFT );
                    if(orientation == LEFT)
                        add(label, "1, 0, r, c");
                    else
                        add(label, "2, 0, l, c");
                }
            }

            createPart();
            //ipart.setFont( new Font("Default", Font.BOLD, 10) );
            ipart.setFont( new Font( "Helvetica", Font.BOLD, 10 ) );

            if ( developer == 1 ) {
                ipart.setBackground( new Color( 0x5f, 0x9e, 0xa0 ) );
                //ipart.setForeground(Color.white);
            }

            if ( vertical == 1 ) {
                int fill_rule = this.getVerticalFill();
                add(ipart, "0, 3, f, f");
            }

            else {
                if(orientation == LEFT)
                    add(ipart, "2, 0, l, c");
                else
                    add(ipart, "1, 0, r, c");
            }

        }

        catch ( Exception e ) {
            e.printStackTrace();
        }
    }

    public int getVerticalFill()
    {
        return GridBagConstraints.VERTICAL;
    }

    //
    //
    //
    public void setLabel ( String s )
    {
        if ( label == null )
            label = new LabelGroup( label_lines );

        label.setText( s );
    }

    //
    //
    //
    public void setVertical ()
    {
        vertical = 1;
    }

    //
    //
    //
    public void setHorizontal ()
    {
        vertical = 0;
    }

    public void setOrientation(int o) {
        orientation = o;
    }

    //
    // Add shadowThickness
    //
    public void paint( Graphics g )
    {
        if ( developer == 0 )
            return ;

        Dimension d = getSize();
        //System.out.println("Applet size in paint: " + d.width + ", " + d.height);
        int w = d.width - 2;
        int h = d.height - 2;
        Color bg = getBackground();
        g.setColor( bg );
        g.draw3DRect ( 0, 0, w, h, true );
    }


} // end class Interactor

