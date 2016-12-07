//


/*
 * $Header: /src/master/dx/src/uipp/java/dx/net/DXApplication.java,v 1.9 2005/12/26 21:33:43 davidt Exp $
 */

// Notes:
// ddi.html explains how the animation panel is supposed to work. Use it as description
//  how to fix code.
// Loop, palindrome, etc. don't affect Sequence but it should.
// Sequence should not run unless Enable Animation is on.
// The frames text field should disable when Sequence is on.
// If mode is changed by key stroke "reset", picks are not disbaled.
// Need to work on listed set of Bugs/Issues.
//
// It appears that a lot of stuff doesn't quite work right. Need to work on every
// single object. 


// For some reason the image will pop up in an X window instead of being fed as a GIF.
	// This appears to happen whenever a build is out of sync. If you take and rebuild
	// everything from scratch--it appears to fix it. This entails
	//  -- deleting all the class files in uipp/java
	//  -- rebuild all with make install
	//  -- rebuild user applets to link with newly installed jars
	//  -- install user applets.

// There is a bug in some java awt implementations when changing the font. getBounds() returns
// improper values. Thus not changing font at this time.

// It appears that FontMetrics.getWidth() will sometimes return 0--depending on the Java. So whenever
// using that, make sure to check for this case and have a fallback in place.

// Missing Dial Interactor, missing slider interactor.

// Trying to add an IntegerList interactor dies in Parameter.C:262.


package dx.net;
import dx.client.*;
import dx.runtime.*;
import java.applet.*;
import java.awt.*;
import java.lang.Thread;
import java.util.*;
import java.awt.event.*;
import java.lang.reflect.*;
import layout.TableLayout;

public abstract class DXApplication extends DXClient
    implements KeyListener, ActionListener, ItemListener {

    //
    // Bean Properties
    // Bean Properties
    // Bean Properties
    //
    // Does this class draw ui features or is it left to the subclass?
    private boolean makesGui = true;

    public void setMakesGui( boolean mk ) {
        this.makesGui = mk;
    }

    public boolean getMakesGui() {
        return this.makesGui;
    }

    private boolean shadows = true;
    private boolean eocMode = false;
    private String bwcompat_string;
    private boolean bwcompat_checked;
    protected boolean cachingMode;
    protected Network network;
    protected Button execute_once;
    protected Button end_execution;
    protected Button connect_btn;
    protected Checkbox exec_on_change;
    protected Checkbox set_caching;
    protected TextField cache_size;
    protected Choice cache_options;
    protected Label speed_label;
    private Vector imageWindows;
    private Vector named_image_list;
    private Vector panels;
    private int selected_panel;
    private TabbedPanel cardPanel;
    private TabbedPanel tabPanel;
    private CheckboxGroup selector;
    private static final int SHADOW = 8;
    private ControlPanel execCtrl;
    private Node sequencer;
    private boolean in_connect_callback;
    private boolean dirty;
    private String macro_name;
    private Panel executing;

    private int modes[];
    private int sequence_mode_choice;

    private Choice image_chooser;
    private dx.net.ImageNode selected_image;
    public dx.net.ImageNode getSelectedImageNode() {
        return this.selected_image;
    }

    private Choice pick_chooser;
    private PickNode selected_pick;
    public PickNode getSelectedPickNode() {
        return this.selected_pick;
    }

    private Choice mode_chooser;
    private Choice view_chooser;
    private int selected_mode;

    public boolean getCachingFeaturesEnabled() {
        return ( this.cache_size != null );
    }

    protected SequencerNode getSequencerNode() {
        return ( SequencerNode ) this.sequencer;
    }


    protected DXApplication() {
        super();
        this.network = null;
        this.execute_once = null;
        this.end_execution = null;
        this.connect_btn = null;
        this.exec_on_change = null;
        this.eocMode = false;
        this.imageWindows = new Vector( 4 );
        this.selected_panel = 0;
        this.panels = null;
        this.cardPanel = null;
        this.tabPanel = null;
        this.selector = null;
        this.selected_pick = null;
        this.pick_chooser = null;
        this.mode_chooser = null;
        this.view_chooser = null;
        this.selected_mode = ImageWindow.NO_MODE;
        this.named_image_list = null;
        this.execCtrl = null;
        this.executing = null;
        this.cache_size = null;
        this.cache_options = null;
        this.cachingMode = false;
        this.sequencer = null;
        this.set_caching = null;
        this.modes = new int[ 10 ];
        this.sequence_mode_choice = -1;
        this.dirty = false;
        this.macro_name = null;
        this.bwcompat_checked = false;
        this.bwcompat_string = null;
    }

    public String getMacroName() {
        return this.macro_name;
    }

    public void setNetwork( Network n ) {
        this.network = n;
        n.setApplet( this );

        Vector image_list = null;
        Class ic = null;

        try {
            ic = Class.forName( "dx.net.ImageNode" );
        }

        catch ( Exception e ) {
            System.out.println ( "DXApplication: failed to list ImageNodes" );
        }

        if ( ic != null )
            image_list = this.network.makeNodeList( ic, false );

        this.named_image_list = null;

        if ( ( image_list != null ) && ( image_list.size() > 0 ) ) {
            Enumeration enum1 = image_list.elements();

            while ( enum1.hasMoreElements() ) {
                ImageNode in = ( ImageNode ) enum1.nextElement();
                String iname = in.getTitle();
                boolean isWired = in.isInteractionModeConnected();

                if ( iname != null ) {
                    this.selected_image = in;

                    if ( isWired == false ) {
                        if ( this.named_image_list == null )
                            this.named_image_list = new Vector( 4 );

                        this.named_image_list.addElement( ( Object ) in );
                    }
                }
            }
        }

        Class sc = null;

        try {
            sc = Class.forName( "dx.net.SequencerNode" );
        }

        catch ( ClassNotFoundException cnfe ) { }

        Vector seq_list = this.network.makeNodeList ( sc, false );

        if ( ( seq_list != null ) && ( seq_list.size() >= 1 ) )
            this.sequencer = ( Node ) seq_list.elementAt( 0 );

    }

    public void start() {
        super.start();

        if ( ( this.getMakesGui() ) && ( this.network != null ) ) {
            Dimension d = this.getSize();
            int st = ( this.shadows ? SHADOW : 0 );
            int tpw = d.width - ( 2 + st );
			//System.out.print("Setting bounds on cardPanel to : 2, 2,");
			//System.out.print( tpw );
			//System.out.print(", ");
			//System.out.println(d.height - (2 + st));
            //cardPanel.setBounds( 2, 2, tpw, d.height - ( 2 + st ) );
        }
        if (this.executing != null) 
        	this.executing.setVisible(false);
    }

    public void init() {
        addKeyListener( this );
        int control_count = 0;

        super.init();
        
		this.setFont(new Font("Default", Font.PLAIN, 10));
		
        int sthick = ( this.shadows ? SHADOW : 0 );
        
        double border = 5;
        double p = TableLayout.PREFERRED;
        double f = TableLayout.FILL;
        // size[0] = the columns
        // size[1] = the rows
        double size[][] = 
        	{{0.50, 0.50},
        	 {p,p,p,p,p,p,p,p,p,f}};
        TableLayout layout = new TableLayout(size);

        if ( this.network != null ) {
            if ( this.getMakesGui() ) {
                Dimension d = this.getSize();
                int tpw = d.width - ( 2 + sthick );
               // cardPanel.setBounds( 2, 2, tpw, d.height - ( 2 + sthick ) );
            }

            return ;
        }

        //
        // Clumsy api here.  I should pass network into this.readNetwork but
        // I didn't want to force subclasses to change.
        //
        this.network = new Network();
        this.readNetwork();
        this.setNetwork( this.network );
        

        if ( this.getMakesGui() == false )
            return ;

        Dimension d = this.getSize();

        int panel_cnt = network.getPanelCount();
        
        cardPanel = new TabbedPanel();
        this.add( cardPanel );
     
        int i = 0;
        while ( i < panel_cnt ) {
        	ControlPanel cp = network.getPanel( i );
        	cp.buildPanel();
        	cardPanel.addPanel( cp, cp.getName() );
        	i++;
        }
	
        //
        // Add execution control panel
        //
        this.execCtrl = new ControlPanel( this.network, "Execution",
                                          this.network.getPanelCount() + 1 );
        //this.execCtrl.setFont ( new Font( "Helvetica", Font.PLAIN, 10 ) );

        this.execCtrl.buildPanel();
        this.execCtrl.setLayout(layout);

        cardPanel.addPanel( this.execCtrl, this.execCtrl.getName() );

        Dimension crd = cardPanel.getSize();
        crd.height = d.height - ( 35 + 12 + sthick + cardPanel.getInsets().top + cardPanel.getInsets().bottom);
		crd.width = d.width - (  20 + sthick + cardPanel.getInsets().left + cardPanel.getInsets().right);
		crd.width = (crd.width > 400 ? 400 : crd.width );
		
		this.execCtrl.setSize(new Dimension(crd.width, crd.height));
		//Dimension d2 = this.execCtrl.getSize();
		//System.out.print("size: ");
		//System.out.print( d2.width );
		//System.out.print(", ");
		//System.out.println(d2.height);
		
		// Executing Label
		this.executing = new Panel();
		//this.executing.setFont(new Font("Helvetica", Font.ITALIC, 12));
		//this.executing.setForeground(new Color((float)0.0, (float)1.0, (float)0.49));
		Label l = new Label("Running, Please Wait ...");
		l.setFont(new Font("Helvetica", Font.ITALIC, 12));
		l.setForeground(new Color((float)0.0, (float)1.0, (float)0.49));
		this.executing.add(l);
		//this.executing.setBounds(d.height+5, 60, 140, 40);
		this.executing.setLocation(d.height+5, 60);
		this.add(this.executing);
		this.executing.setVisible(true);

        //
        // Connect option
        //
        String cstr = getParameter( "CONNECT_OPTION" );

        boolean has_connect = false;

        if ( cstr != null )
            has_connect = Boolean.valueOf( cstr ).booleanValue();

        if ( has_connect ) {
        	
            this.connect_btn = new Button( "Connect" );
            this.connect_btn.setBackground( this.getBackground() );
            this.execCtrl.add( this.connect_btn, "0, 0, 1, 0" );            
            connect_btn.addActionListener( this );
            
            Separator sep = new Separator();
            sep.setForeground(new Color(200, 200, 200));
            sep.setSize(new Dimension(crd.width -20, 2));
            this.execCtrl.add(sep, "0, 1, 1, 1, c, c");
        }


        //
        // Execute - On - Change
        //
        
        FlowLayout cfl = new FlowLayout(FlowLayout.CENTER, 5, 0);
        
        String eoc = getParameter( "EXECUTE_ON_CHANGE" );

        if ( eoc != null )
            this.eocMode = Boolean.valueOf( eoc ).booleanValue();
        else this.eocMode = false;

        this.exec_on_change = new Checkbox( "Execute On Change" );
        this.exec_on_change.setBackground( this.getBackground() );
        this.exec_on_change.setState( this.eocMode );
        this.execCtrl.add( this.exec_on_change, "0, 2, 1, 2, c, c" );
        exec_on_change.addItemListener( this );

        //
        // Execute Once
        //
        this.execute_once = new Button( "Execute Once" );
        this.execute_once.setBackground( this.getBackground() );
        this.execCtrl.add(this.execute_once, "0, 3, c, c");        		
        execute_once.addActionListener( this );
        
         //
        // End Execution
        //
        this.end_execution = new Button( "End Execution" );
        this.end_execution.setBackground( this.getBackground() );        
        this.execCtrl.add(this.end_execution, "1, 3, c, c");
        end_execution.addActionListener( this );

        Separator sep1 = new Separator();
        sep1.setForeground(new Color(200, 200, 200));
        sep1.setSize(new Dimension(crd.width -20, 2));
        this.execCtrl.add(sep1, "0, 4, 1, 4, c, c");

        //
        // We'll conditionally allow users to set caching features
        // so that we don't wreck existing web pages by added
        // unexpected features.  When the version number jumps to
        // 1.1, then we should add caching features by default.
        //
        this.setCachingMode( false, false );
        String enabstr = getParameter( "CACHING_FEATURES" );
        boolean enable_animation = false;

        if ( enabstr != null )
            enable_animation = Boolean.valueOf( enabstr ).booleanValue();

        if ( enable_animation || ( this.sequencer != null) ) {
            //
            // Image caching
            //

            //this.setCachingMode( true, false );
            this.set_caching = new Checkbox( "Enable Animation" );
            this.set_caching.setState( this.getCachingMode() );
            this.set_caching.setBackground( this.getBackground() );
            this.execCtrl.add( this.set_caching, "0, 5, 1, 5, c, c" );
            set_caching.addItemListener( this );

            //
            // Image cache options
            //
            
            this.cache_options = new Choice();
            this.cache_options.setBackground( this.getBackground() );
            this.cache_options.addItem( "Loop" );
            this.cache_options.addItem( "Palindrome" );
            this.cache_options.addItem( "Faster" );
            this.cache_options.addItem( "Slower" );
            this.cache_options.addItem( "Reset" );
            this.execCtrl.add( this.cache_options, "0, 6, c, c" );
            this.cache_options.setEnabled( this.getCachingMode() );
            
            cache_options.addItemListener( this );
            

            //
            // Image cache size - number of frames to cache.
            //
            
            Panel pFrames = new Panel();
            pFrames.setLayout(cfl);
            
            Label lab = new Label( "Frames:" );
            lab.setAlignment( Label.RIGHT );
            pFrames.add(lab);

            this.cache_size = new TextField( "0", 4 );
            this.cache_size.setBackground( this.getBackground() );
            pFrames.add(this.cache_size);
            this.cache_size.setEnabled( this.getCachingMode() );
            this.execCtrl.add(pFrames, "1, 6, c, c" );
            			
            cache_size.addActionListener(this);
            // Only allow numbers to be entered.
            cache_size.addKeyListener(new KeyAdapter() {
            	public void keyTyped(KeyEvent e) {
					char c = e.getKeyChar();
					if(!(Character.isDigit(c) ||
						( c == KeyEvent.VK_BACK_SPACE) ||
						(c == KeyEvent.VK_DELETE) ||
						(c == KeyEvent.VK_ENTER) ||
						(c == KeyEvent.VK_ACCEPT))) {
						e.consume();
					}
            	}
            	public void keyPressed(KeyEvent e) {
            		char c = e.getKeyChar();
            		if(!(Character.isDigit(c) ||
            		   (c == KeyEvent.VK_BACK_SPACE) ||
            		   (c == KeyEvent.VK_DELETE) ||
            		   (c == KeyEvent.VK_ENTER) ||
            		   (c == KeyEvent.VK_ACCEPT))) {
            		       e.consume();
            		   }
				}
            });
            
            //this.cache_size.setEnabled( false );
            
            Separator sep = new Separator();
            sep.setForeground(new Color(200, 200, 200));
            sep.setSize(new Dimension(crd.width -20, 2));
            this.execCtrl.add(sep, "0, 7, 1, 7, c, c" );
        }

        
        //
        // Image chooser
        //
        
        double optList[][] = {{ 75, f}, {p, p, p, p, f}};
        Panel optionsList = new Panel();
        TableLayout optLayout = new TableLayout(optList);
		optionsList.setLayout(optLayout);
        
        
        if ( ( this.named_image_list != null ) && ( this.named_image_list.size() > 1 ) ) {
            Label lab = new Label( "Image: " );
            lab.setAlignment( Label.RIGHT );
            optionsList.add(lab, "0, 0, r, c");

            image_chooser = new Choice();
            image_chooser.setBackground( this.getBackground() );
            Enumeration enum1 = this.named_image_list.elements();

            while ( enum1.hasMoreElements() ) {
                ImageNode in = ( ImageNode ) enum1.nextElement();
                String name = in.getTitle();
                image_chooser.addItem( name );
            }

            image_chooser.addItemListener( this );
            image_chooser.select( 0 );
            this.selected_image = ( ImageNode ) this.named_image_list.elementAt( 0 );
            optionsList.add( image_chooser, "1, 0, l, c" );
        }


		//
		// Determine if pick should be active.
		//

        Vector pick_list = null;
        Class pc = null;

        try {
            pc = Class.forName( "dx.net.PickNode" );
        }
        catch ( Exception e ) {
            System.out.println ( "DXApplication: failed to list PickNodes" );
        }

        if ( pc != null )
            pick_list = this.network.makeNodeList( pc, false );



        //
        // Mode chooser
        //
        if ( ( this.named_image_list != null ) && ( this.named_image_list.size() > 0 ) ) {
            Label lab = new Label( "View: " );
            lab.setAlignment( Label.RIGHT );
			optionsList.add(lab, "0, 1, r, c");

            this.view_chooser = new Choice();
            this.view_chooser.setBackground( this.getBackground() );
            this.view_chooser.addItem( "Front" );
            this.view_chooser.addItem( "Back" );
            this.view_chooser.addItem( "Top" );
            this.view_chooser.addItem( "Bottom" );
            this.view_chooser.addItem( "Right" );
            this.view_chooser.addItem( "Left" );
            this.view_chooser.addItem( "Diagonal" );
            this.view_chooser.addItem( "Off Front" );
            this.view_chooser.addItem( "Off Back" );
            this.view_chooser.addItem( "Off Top" );
            this.view_chooser.addItem( "Off Bottom" );
            this.view_chooser.addItem( "Off Right" );
            this.view_chooser.addItem( "Off Left" );
            this.view_chooser.addItem( "Off Diagonal" );

            view_chooser.addItemListener( this );

            optionsList.add( this.view_chooser, "1, 1, l, c" );

            lab = new Label( "Mode: " );
            lab.setAlignment( Label.RIGHT );
			optionsList.add(lab, "0, 2, r, c");
			
            int m = 0;
            mode_chooser = new Choice();
            mode_chooser.setBackground( this.getBackground() );
            mode_chooser.addItem( "(None)" ); this.modes[ m++ ] = ImageWindow.NO_MODE;
            mode_chooser.addItem( "Rotate" ); this.modes[ m++ ] = ImageWindow.ROTATE_MODE;

            if ( pick_list != null ) {
                mode_chooser.addItem( "Pick" ); this.modes[ m++ ] = ImageWindow.PICK_MODE;
            }

            mode_chooser.addItem( "Pan" ); this.modes[ m++ ] = ImageWindow.PAN_MODE;
            mode_chooser.addItem( "Zoom" ); this.modes[ m++ ] = ImageWindow.ZOOM_MODE;
            mode_chooser.addItem( "Orbit" ); this.modes[ m++ ] = ImageWindow.ORBIT_MODE;
            mode_chooser.addItem( "Reset Camera" );this.modes[ m++ ] = ImageWindow.RESET_CAMERA;

            if ( this.sequencer != null ) {
                this.sequence_mode_choice = m;
                mode_chooser.addItem( "Sequence" );this.modes[ m++ ] = ImageWindow.LOOP_MODE;
            }

            else if ( enable_animation ) {
                mode_chooser.addItem( "Review" ); this.modes[ m++ ] = ImageWindow.LOOP_MODE;
            }

            mode_chooser.addItemListener( this );

            if ( this.selected_image != null ) {
                this.selected_mode = this.selected_image.getInteractionMode();
                mode_chooser.select( this.selected_mode );
            }

            else {
                mode_chooser.select( 0 );
                this.selected_mode = ImageWindow.NO_MODE;
            }

            optionsList.add( mode_chooser, "1, 2, l, c" );
        }
        

        //
        // Pick tool chooser
        //

        pick_chooser = null;

        if ( ( pick_list != null ) && ( pick_list.size() == 1 ) ) {
            this.selected_pick = ( PickNode ) pick_list.elementAt( 0 );
        }

        else if ( ( pick_list != null ) && ( pick_list.size() > 1 ) ) {
            Label lab = new Label( "Pick: " );
            lab.setAlignment( Label.RIGHT );
			optionsList.add( lab, "0, 3, r, c");

            pick_chooser = new Choice();
            pick_chooser.setBackground( this.getBackground() );
            Enumeration enum1 = pick_list.elements();

            while ( enum1.hasMoreElements() ) {
                PickNode pn = ( PickNode ) enum1.nextElement();
                String name = pn.getName() + "_" + pn.getInstanceNumber();
                pick_chooser.addItem( name );
            }
            //System.out.println("selected: " + mode_chooser.getSelectedItem());
			pick_chooser.setEnabled(false);
            pick_chooser.select( 0 );
            pick_chooser.addItemListener( this );
            this.selected_pick = ( PickNode ) pick_list.elementAt( 0 );
            optionsList.add( pick_chooser, "1, 3, l, c" );
        }

		this.execCtrl.add(optionsList, "0, 8, 1, 8, l, c");

        //
        // If this parameter is set to false, then we won't draw shadows
        // The purpose would be to let the user wrap in the applet in html
        // table dressing.
        //
        String nobrd = this.getParameter( "SHADOWS" );

        if ( nobrd != null )
            this.shadows = Boolean.valueOf( nobrd ).booleanValue();
    }


public void setJavaId ( int jid ) {}

    public void setMacroName ( String macro ) {
        this.macro_name = macro;

        if ( isConnected() ) {
            boolean resume_eoc = this.eocMode;
            this.setEocMode ( false );

            //
            // Install value handlers for all DrivenNodes
            //
            Enumeration enum1 = this.network.elements();

            while ( enum1.hasMoreElements() ) {
                dx.net.Node n = ( dx.net.Node ) enum1.nextElement();
                n.enableJava();
            }

            //
            // Set a special DXLInput tool
            //
            DXLSend ( "_java_control=1;" );

            //
            // If WebOptions.imgId was tab-down and unconnected in the
            // original net, then we can associate some ImageWindows with
            // their ImageNodes early rather than waiting for an execution.
            //
            this.locateImageWindows();

            enum1 = this.imageWindows.elements();

            int wid = -1;

            while ( enum1.hasMoreElements() ) {
                ImageWindow iw = ( ImageWindow ) enum1.nextElement(); wid++;
                String node_name = iw.getInitialNodeName( this.network );

                if ( node_name == null ) continue;

                Vector v = this.network.makeNodeList( node_name );

                if ( ( v == null ) || ( v.size() != 1 ) ) continue;

                ImageNode in = ( ImageNode ) v.elementAt( 0 );

                in.associate( iw, wid );
            }

            this.setEocMode( resume_eoc );
        }
        
        // Allow setting a DXInput from the html.
        
        String pstr = this.getParameter( "DXLInput" );
        if(pstr != null)
        	DXLSend(pstr);

        this.setEnabled( true );
    }

    protected void connect() {
        boolean was_connected = this.isConnected();

        if ( ( this.connect_btn == null ) || ( this.in_connect_callback ) ) {
            this.in_connect_callback = false;
            super.connect();
        }

        if ( ( this.connect_btn != null ) && ( this.isConnected() ) ) {
            this.connect_btn.setLabel( "Disconnect" );
        }

        if ( ( was_connected == false ) && ( this.isConnected() ) ) {
            this.setEnabled( false );
        }
        if(this.isConnected()) {
        	if(this.execute_once != null)
        		this.execute_once.setEnabled(true);
        	if(this.exec_on_change != null)
        		this.exec_on_change.setEnabled(true);
        	if(this.end_execution != null)
        		this.end_execution.setEnabled(true);
        	if(this.set_caching != null)
        		this.set_caching.setEnabled(true);
        } else {
            if(this.execute_once != null)
        		this.execute_once.setEnabled(false);
        	if(this.exec_on_change != null)
        		this.exec_on_change.setEnabled(false);
        	if(this.end_execution != null)
        		this.end_execution.setEnabled(false);
        	if(this.set_caching != null)
        		this.set_caching.setEnabled(false);
		}

    }

    public void updateCaching( ImageNode in ) {
        if ( this.cache_size == null ) return ;

        Integer Size = new Integer( in.getCacheCount() );

        this.cache_size.setText( Size.toString() );
    }

    protected void disconnect( Thread t ) {
        super.disconnect( t );

        if ( this.connect_btn != null )
            this.connect_btn.setLabel( "Connect" );

        if ( this.execCtrl != null ) {
        	if(this.executing != null) {
        		this.executing.setVisible(false);
			}	
        }
            if(this.execute_once != null)
        		this.execute_once.setEnabled(false);
        	if(this.exec_on_change != null)
        		this.exec_on_change.setEnabled(false);
        	if(this.end_execution != null)
        		this.end_execution.setEnabled(false);
        	if(this.set_caching != null)
        		this.set_caching.setEnabled(false);

        if ( this.mode_chooser != null ) {
            this.mode_chooser.select( ImageWindow.NO_MODE );
            this.selected_mode = ImageWindow.NO_MODE;

            if ( this.selected_image != null )
                this.applyInteractionMode ( ImageWindow.NO_MODE, 0 );
        }
    }

    protected abstract void readNetwork();
    protected abstract boolean isExecuting();

    protected void DXLExecuteOnce() {
       // if ( ( this.isExecuting() ) && ( this.executing != null ) ) {
		if(this.executing != null) {
        		//this.executing.setEnabled(false);
        		this.executing.setVisible(true);
        		this.executing.repaint();
        		//this.executing.setEnabled(true);
        }

        this.dirty = false;
    }

    public void updateSelectedImageNode ( KeyEvent e, ImageNode in ) {
    	//System.out.println("updateSelectedImageNode: " + in.getTitle());
        if ( ( this.named_image_list != null ) && ( this.selected_image != in ) ) {
            if ( ( in != null ) && ( this.named_image_list.contains( ( Object ) in ) ) ) {
                int i = this.named_image_list.indexOf( ( Object ) in );
                this.image_chooser.select( i );
                this.selected_image = in;
            }
        }
        this.keyPressed( e );
    }

    public void keyPressed ( KeyEvent e ) {
        int key = e.getKeyCode();
        int m = e.getModifiers();
        //System.out.println ( "DXApplication.keyPressed: " + e.getKeyText(key) );

        if ( key == KeyEvent.VK_END && ( m & InputEvent.CTRL_MASK ) > 0 ) {
            this.setEocMode( false );
        }

        else if ( key == KeyEvent.VK_E && ( m & InputEvent.CTRL_MASK ) > 0 ) {
            this.DXLExecuteOnce();
        }

        else if ( key == KeyEvent.VK_R && ( m & InputEvent.CTRL_MASK ) > 0 ) {
            this.setInteractionMode( ImageWindow.ROTATE_MODE, 0 );
        }

        else if ( key == KeyEvent.VK_F && ( m & InputEvent.CTRL_MASK ) > 0 ) {
            if ( this.selected_image != null ) {
                this.selected_image.resetCamera( true, false );
                this.DXLExecuteOnce();
                this.selected_image.resetCamera( false, false );
                this.setInteractionMode( ImageWindow.NO_MODE, 0 );
            }
        }

        else if ( key == KeyEvent.VK_TAB && ( m & InputEvent.CTRL_MASK ) > 0 ) {
            this.setInteractionMode( ImageWindow.PAN_MODE, 0 );
        }

        else if ( key == KeyEvent.VK_SPACE && ( m & InputEvent.CTRL_MASK ) > 0 ) {
            this.setInteractionMode( ImageWindow.ZOOM_MODE, 0 );
        }

        else if ( key == KeyEvent.VK_C && ( m & InputEvent.CTRL_MASK ) > 0 ) {
            this.setEocMode( true );
        }
    }
    
    public void keyReleased ( KeyEvent e ) {
    }
    
    public void keyTyped ( KeyEvent e ) {
    }

    public void actionPerformed( ActionEvent ae ) {
    	//System.out.println("DXApplication.actionPerformed: " + ae.getActionCommand());
        String str = ae.getActionCommand();
        
        if(ae.getSource() == this.cache_size) {
        	//System.out.println("Setting new cache.");
        	String sizestr = this.cache_size.getText();
        	int size;
        	Integer I = new Integer( sizestr );
        	size = I.intValue();
        	this.setCacheSize( size );
        }

        // Pressed Execute Button

        else if ( str.equals( "Execute Once" ) ) {
            this.DXLExecuteOnce();
        }

        else if ( str.equals( "End Execution" ) ) {
            this.DXLEndExecution();
        }

        else if ( str.equals( "Connect" ) ) {
                this.in_connect_callback = true;
                this.connect();
                this.in_connect_callback = false;
        }
        
        else if (str.equals("Disconnect")) {
        	if(this.isConnected()) 
        		this.disconnect(null);
        }
        
    }

    public void itemStateChanged( ItemEvent ie ) {
    	//System.out.println("itemStateChanged: " + ie.paramString());
        if ( ie.getSource() == this.exec_on_change ) {
            this.setEocMode( exec_on_change.getState() );
        }

        else if ( ie.getSource() == this.set_caching ) {
            this.setCachingMode( this.set_caching.getState(), false );
        }

        else if ( ie.getSource() == this.cache_options ) {
            int selected = this.cache_options.getSelectedIndex();
            this.setCacheOption( selected );
            if ( selected == ImageNode.RESET ) {
                this.set_caching.setState( false );
            	this.setCachingMode( false, true );
            	this.mode_chooser.select( 0 );
			}
        }

        else if ( ie.getSource() == this.pick_chooser ) {
			Date now = new Date();
            long event_time = now.getTime();

            Vector pick_list = null;
        	Class pc = null;

	        try {
    	        pc = Class.forName( "dx.net.PickNode" );
    	    }
    	    catch ( Exception e ) {
    	        System.out.println ( "DXApplication: failed to list PickNodes" );
    	    }

    	    if ( pc != null )
    	        pick_list = this.network.makeNodeList( pc, false );

            String pick_name = pick_chooser.getSelectedItem();
            PickNode old_pick = this.selected_pick;

            if ( ( pick_list != null ) && ( pick_list.size() > 0 ) ) {
                int i = this.pick_chooser.getSelectedIndex();
                this.selected_pick = ( PickNode ) pick_list.elementAt( i );
            }

            if ( this.selected_image != null ) {
                if ( this.selected_mode == ImageWindow.PICK_MODE ) {
                    PickNode tmp = this.selected_pick;
                    this.selected_pick = old_pick;
                    this.applyInteractionMode ( ImageWindow.NO_MODE, event_time );
                    this.selected_pick = tmp;
                    this.applyInteractionMode ( ImageWindow.PICK_MODE, event_time );
                }

                else {
                    this.applyInteractionMode ( this.selected_mode, event_time );
                }
            }
        }

        else if ( ie.getSource() == this.image_chooser ) {
            String image_name = image_chooser.getSelectedItem();
//            System.out.println("image now chosen: " + image_name);
			Date now = new Date();
            long event_time = now.getTime();

            if ( ( this.named_image_list != null ) && ( this.named_image_list.size() > 0 ) ) {
                if ( this.selected_image != null ) {
                    this.applyInteractionMode ( ImageWindow.NO_MODE, event_time );
                }

                int i = this.image_chooser.getSelectedIndex();
//            System.out.println("image id chosen: " + i);
                this.selected_image = ( ImageNode ) this.named_image_list.elementAt( i );
                this.applyInteractionMode ( this.selected_mode, event_time );

                if ( ( this.mode_chooser != null ) && ( this.modes != null ) ) {
                    int mode = this.selected_image.getInteractionMode();
                    //
                    // loop over modes in this.modes in order to find
                    // the index to stick into mode_chooser which
                    // corresponds to mode.
                    //
                    int k;
                    int length = this.modes.length;

                    for ( k = 0; k < length; k++ ) {
                        if ( this.modes[ k ] == mode ) {
                            this.mode_chooser.select( k );
                            break;
                        }
                    }
                }
            }

            this.updateCaching( this.selected_image );

        }

        //
        // Select a view
        //
        else if ( ie.getSource() == this.view_chooser ) {
            if ( this.selected_image != null )
                this.selected_image.setView( this.view_chooser.getSelectedIndex(), true );
        }

        //
        // Select an interaction mode
        //
        else if ( ie.getSource() == this.mode_chooser ) {
            int old_mode = this.selected_mode;

            int ndx = this.mode_chooser.getSelectedIndex();
            this.selected_mode = this.modes[ ndx ];

            if ( this.selected_mode == ImageWindow.RESET_CAMERA ) {
                if ( this.selected_image != null ) {
                    this.selected_image.resetCamera( true, false );
                    this.DXLExecuteOnce();
                    this.selected_image.resetCamera( false, false );
                }

                this.mode_chooser.select( 0 );
                this.selected_mode = ImageWindow.NO_MODE;
            }

            if ( ( this.selected_mode == old_mode ) && ( old_mode == ImageWindow.LOOP_MODE ) ) {
                this.applyInteractionMode( ImageWindow.NO_MODE, 0 );
                old_mode = ImageWindow.NO_MODE;
            }

            if ( this.selected_mode == ImageWindow.LOOP_MODE ) {
                if ( this.set_caching != null ) this.setCachingMode( true, false );
            }

            if ( this.pick_chooser != null ) {
                if ( this.selected_mode == ImageWindow.PICK_MODE )
                    this.pick_chooser.setEnabled( true );
                else
                    this.pick_chooser.setEnabled( false );
            }

            if ( this.selected_mode == ImageWindow.ORBIT_MODE ) {
                if ( this.sequencer != null ) {
                    SequencerNode sn = ( SequencerNode ) this.sequencer;
                    sn.reset();
                }
            }

            if ( this.selected_image != null ) {
                if ( ( this.dirty == true ) && ( ndx == this.sequence_mode_choice ) &&
                        ( this.sequencer != null ) ) {
                    SequencerNode sn = ( SequencerNode ) this.sequencer;
                    sn.reset();
                    this.setCacheOption( ImageNode.RESET );
                }

				Date now = new Date();
	            long event_time = now.getTime();
                boolean status = this.applyInteractionMode
                                 ( this.selected_mode, event_time );

                if ( ( status == false ) && ( this.selected_mode == ImageWindow.LOOP_MODE ) ) {
                    if ( ( ndx == this.sequence_mode_choice ) && ( this.sequencer != null ) ) {
                        this.applyInteractionMode( ImageWindow.NO_MODE, 0 );
                        SequencerNode sn = ( SequencerNode ) this.sequencer;
                        sn.reset();
                        this.setCacheOption( ImageNode.RESET );
                        this.setCacheSize( sn.getSteps() );
                        //sn.step(this);
                        this.DXLExecuteOnce();
                        sn.stepQuietly();
                        status = true;
                    }
                }

                if ( status == false ) {
                    this.mode_chooser.select( old_mode );
                    this.selected_mode = old_mode;
                }
            }
        }
    }

    private void setCacheOption ( int mode ) {
        if ( mode == ImageNode.RESET ) {
            this.cache_options.select( 0 );
        }

        ImageNode in = this.selected_image;

        String group_name = in.getGroupName();

        if ( group_name == null ) {
            in.setCacheOption( mode );
            return ;
        }

        Vector node_list = null;
        Class ic = null;

        try {
            ic = Class.forName( "dx.net.ImageNode" );
        }

        catch ( Exception e ) {
            System.out.println ( "DXApplication: failed to list ImageNodes" );
        }

        if ( ic != null )
            node_list = this.network.makeNodeList( ic, false );

        if ( node_list != null ) {
            Enumeration enum1 = node_list.elements();

            while ( enum1.hasMoreElements() ) {
                ImageNode n = ( ImageNode ) enum1.nextElement();

                if ( group_name.equals( n.getGroupName() ) )
                    n.setCacheOption( mode );
            }
        }
    }

    private void setCacheSize( int size ) {
        ImageNode in = this.selected_image;
        String group_name = in.getGroupName();

        if ( group_name == null ) {
            in.setCacheSize( size );
            return ;
        }

        Vector node_list = null;
        Class ic = null;

        try {
            ic = Class.forName( "dx.net.ImageNode" );
        }

        catch ( Exception e ) {
            System.out.println ( "DXApplication: failed to list ImageNodes" );
        }

        if ( ic != null )
            node_list = this.network.makeNodeList( ic, false );

        if ( node_list != null ) {
            Enumeration enum1 = node_list.elements();

            while ( enum1.hasMoreElements() ) {
                ImageNode n = ( ImageNode ) enum1.nextElement();

                if ( group_name.equals( n.getGroupName() ) )
                    n.setCacheSize( size );
            }
        }
    }

    public boolean setInteractionMode ( int mode, long time ) {
        if ( this.applyInteractionMode( mode, time ) == false ) return false;


        //
        // loop over modes in this.modes in order to find
        // the index to stick into mode_chooser which
        // corresponds to mode.
        //
        if ( this.mode_chooser != null ) {
            int k;
            int length = this.modes.length;

            for ( k = 0; k < length; k++ ) {
                if ( this.modes[ k ] == mode ) {
                    this.mode_chooser.select( k );
                    if(mode == ImageWindow.PICK_MODE)
                    	this.pick_chooser.setEnabled(true);
                    break;
                }
            }
        }

        this.selected_mode = mode;
        return true;
    }

    private boolean applyInteractionMode( int mode, long time ) {
        ImageNode in = this.selected_image;
        //System.out.println("applyInteractionMode in: " + in.getTitle());
        boolean status = in.setInteractionMode( mode, time );
	
        if ( status == false ) return false;

        String group_name = in.getGroupName();

        if ( group_name == null ) return true;

        if ( ImageWindow.IsGroupInteraction( mode ) ) {
            Vector node_list = null;
            Class ic = null;

            try {
                ic = Class.forName( "dx.net.ImageNode" );
            }

            catch ( Exception e ) {
                System.out.println ( "DXApplication: failed to list ImageNodes" );
            }

            if ( ic != null )
                node_list = this.network.makeNodeList( ic, false );

            if ( node_list != null ) {
                Enumeration enum1 = node_list.elements();

                while ( enum1.hasMoreElements() ) {
                    ImageNode n = ( ImageNode ) enum1.nextElement();

                    if ( group_name.equals( n.getGroupName() ) )
                        n.setInteractionMode( mode, time );
                }
            }
        }

        return true;
    }

    public void paint ( Graphics g ) {
        super.paint( g );

        if ( this.getMakesGui() == false ) return ;
        if ( this.shadows == false ) return ;

        Dimension d = getSize();
        g.setColor( Color.black );
        g.drawRect ( 0, 0, d.width - SHADOW, d.height - ( SHADOW ) );
        g.fillRect ( SHADOW, d.height - SHADOW, d.width - SHADOW, SHADOW );
        g.fillRect ( d.width - SHADOW, SHADOW, SHADOW, d.height - ( SHADOW ) );
    }

    public ImageWindow getImageWindow( int id ) {
        ImageWindow iw = null;

        if ( this.imageWindows.size() == 0 ) {
            this.locateImageWindows();
        }

        try {
            iw = ( ImageWindow ) this.imageWindows.elementAt( id );
        }

        catch ( ArrayIndexOutOfBoundsException aiobe ) {}
        catch ( NullPointerException npe ) {}

        return iw;
    }

    public void showWorld( String name, String path, int id ) { }

    private void locateImageWindows() {
    
        AppletContext apcxt = this.getAppletContext();
        Enumeration enuma = apcxt.getApplets();
				
        while ( enuma.hasMoreElements() ) {
            Applet a = ( Applet ) enuma.nextElement();
            
            // It is possible that the applet exists on the web page,
            // is a subclass of ImageWindow but still returns false for
            // instanceof ImageWindow. If the two applets are loaded 
            // under class loaders. 
            if ( ( a != null ) && ( a.isActive() ) && ( a instanceof ImageWindow ) ) {
                if ( this.imageWindows.contains( ( Object ) a ) == false )
                    this.imageWindows.addElement( ( Object ) a );
            }
        }
    }

    public synchronized void DXLSend ( String msg ) {
        if ( msg.lastIndexOf( "assign noexecute" ) == -1 ) {
            this.dirty = true;

            if ( this.eocMode )
                this.DXLExecuteOnce();
        }
    }


    public void enableModeSelector( boolean active ) {
        if ( this.mode_chooser == null ) return ;

        if ( active ) this.mode_chooser.setEnabled( true );
        else this.mode_chooser.setEnabled( false );
    }

    public synchronized void finishedExecuting() {
        if ( ( this.isExecuting() == false ) && ( this.executing != null ) ) {
        	this.executing.setVisible(false);

            //
            // Do another sequencer step?
            //
            if ( ( this.sequencer != null ) && ( this.selected_mode == ImageWindow.LOOP_MODE ) ) {
                int ndx = this.mode_chooser.getSelectedIndex();

                if ( ( ndx == this.sequence_mode_choice ) &&
                        ( this.selected_image.getInteractionMode() != ImageWindow.LOOP_MODE ) ) {
                    SequencerNode sn = ( SequencerNode ) this.sequencer;
                    int snsteps = sn.getSteps();
                    int isteps = this.selected_image.getCacheCount();

                    if ( snsteps != isteps )
                        this.setCacheSize( sn.getSteps() );

                    Date now = new Date();

                    long et = now.getTime();

                    if ( sn.hasMoreFrames() == true ) {
                        //sn.step(this);
                        this.DXLExecuteOnce();
                        sn.stepQuietly();
                    }

                    else if ( this.selected_image != null ) {
                        this.applyInteractionMode( this.selected_mode, et );
                    }
                }
            }

            else if ( this.selected_mode == ImageWindow.PICK_MODE ) {
                PickNode pn = this.getSelectedPickNode();
                ImageNode in = this.getSelectedImageNode();

                if ( ( pn != null ) && ( in != null ) ) {
                    in.resetPickList();
                }
            }
        }
    }

    public void DXLEndExecution() {
        if ( ( this.sequencer != null ) && ( this.selected_mode == ImageWindow.LOOP_MODE ) )
            this.setInteractionMode( ImageWindow.NO_MODE, 0 );
    }

    /*
    protected void page(boolean up) { 
    if (this.panels == null) return ;

    int index = this.selected_panel;

    if (up) ((CardLayout)cardPanel.getLayout()).previous (cardPanel); 
    else ((CardLayout)cardPanel.getLayout()).next (cardPanel); 

    if (up) index--;
    else index++;

    // There exist getPanelCount() + 1 panels
    if (index < 0)
     index = this.network.getPanelCount();
    else if (index > this.network.getPanelCount()) 
     index = 0;

    ControlPanel cp = (ControlPanel)this.panels.elementAt(index);
    Checkbox cb = cp.getTab();
    this.selected_panel = index;
    cb.setState(true);
    }
    */

    public String getBWCompatString() {
        if ( this.bwcompat_checked ) return this.bwcompat_string;

        this.bwcompat_string = null;

        this.bwcompat_checked = true;

        try {
            //
            // Pare down each one to the rightmost '/'
            //
            String docbase = this.getDocumentBase().toString();
            String codebase = this.getCodeBase().toString();
            int dind = docbase.lastIndexOf( ( int ) '/' );
            int cind = codebase.lastIndexOf( ( int ) '/' );

            docbase = docbase.substring ( 0, dind );
            codebase = codebase.substring ( 0, cind );

            if ( docbase.equals( codebase ) ) this.bwcompat_string = "../";
        }

    catch ( Exception e ) {}

        return this.bwcompat_string;
    }

    public boolean getEocMode() {
        return this.eocMode;
    }

    public void setEocMode ( boolean onoroff ) {
        if ( onoroff ) {
            this.eocMode = true;

            if ( this.exec_on_change != null )
                this.exec_on_change.setState( true );

            this.DXLExecuteOnce();
        }

        else {
            this.eocMode = false;

            if ( this.exec_on_change != null )
                this.exec_on_change.setState( false );

            if ( ( this.isExecuting() == false ) && ( this.execCtrl != null ) ) {
              // --Fixme, change color of tab
                //Checkbox cb = execCtrl.getTab();
                //cb.setForeground( Color.black );
            }
        }
    }

    // Does this class draw the fake 3d shadows around itself?
    public void setShadows( boolean s ) {
        this.shadows = s;
    }

    public boolean getShadows() {
        return this.shadows;
    }

    public boolean getCachingMode() {
        return this.cachingMode;
    }

    public void setCachingMode ( boolean onoroff, boolean clear_cache ) {
        this.cachingMode = onoroff;

        if ( onoroff ) {
            if ( this.cache_size != null )
                this.cache_size.setEnabled( true );

            if ( this.cache_options != null )
                this.cache_options.setEnabled( true );

 			if (this.selected_image != null ) {
	            if ( this.selected_image.getInteractionMode() != ImageWindow.LOOP_MODE ) {
 					Date now = new Date();
	 	            long event_time = now.getTime();
	 	            
	 	            this.applyInteractionMode( this.selected_mode, event_time);
	 	        }
             }	

        }

        else {
            if ( this.cache_size != null )
                this.cache_size.setEnabled( false );

            if ( this.cache_options != null )
                this.cache_options.setEnabled(false);

			if(this.selected_image != null )
				if (this.selected_image.getInteractionMode() == ImageWindow.LOOP_MODE )
					this.applyInteractionMode( ImageWindow.NO_MODE, 0 );
        }

        if ( this.imageWindows == null ) return ;

        if ( clear_cache ) {
            Enumeration enum1 = this.imageWindows.elements();

            while ( enum1.hasMoreElements() ) {
                ImageWindow iw = ( ImageWindow ) enum1.nextElement();
                iw.clearImageCache();
            }
        }
    }



}

; // end DXApplication
