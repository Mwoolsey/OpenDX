/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

/*
 * $Header: /src/master/dx/src/uipp/java/Status.java,v 1.3 2006/05/16 16:04:02 davidt Exp $
 */
import java.applet.*;
import java.lang.*;
import java.awt.*;
import java.net.*;
import java.io.*;
import dx.client.*;
import dx.protocol.server.*;
import dx.protocol.*;

public class Status extends DXClient implements Runnable
{
        protected dx.protocol.server.serverMsg getRequest()
        {
                return new statusMsg();
        }

        protected Label[] outputs;
        protected Thread status_reader;

        public Status()
        {
                super();
        }

        protected DXClientThread createCommandThread()
        {
                return null;
        }

        public void start()
        {
                super.start();
                status_reader = new Thread( this );
                status_reader.start();
        }

        public void init()
        {
                super.init();
                Label lab;
                GridBagLayout gbl = new GridBagLayout();
                GridBagConstraints gbc;
                setLayout( gbl );
                Font font = new Font( "Helvetica", Font.BOLD, 14 );

                outputs = new Label[ 3 ];

                lab = new Label( "Users:" );
                lab.setBackground( super.getBackground() );
                lab.setAlignment ( Label.LEFT );
                lab.setFont( font );
                gbc = new GridBagConstraints();
                gbc.gridx = 0; gbc.gridy = 0; gbc.gridwidth = 1; gbc.gridheight = 1;
                gbc.ipadx = 0; gbc.ipady = 0; gbc.fill = GridBagConstraints.HORIZONTAL;
                gbc.weightx = 0; gbc.weighty = 1;
                gbc.anchor = GridBagConstraints.CENTER;
                gbl.setConstraints ( lab, gbc );

                add ( lab );

                outputs[ 0 ] = new Label();

                outputs[ 0 ].setBackground( super.getBackground() );

                outputs[ 0 ].setAlignment( Label.LEFT );

                outputs[ 0 ].setFont( font );

                gbc = new GridBagConstraints();

                gbc.gridx = 1; gbc.gridy = 0; gbc.gridwidth = 2; gbc.gridheight = 1;

                gbc.ipadx = 0; gbc.ipady = 0; gbc.fill = GridBagConstraints.HORIZONTAL;

                gbc.weightx = 1; gbc.weighty = 1;

                gbc.anchor = GridBagConstraints.CENTER;

                gbl.setConstraints ( outputs[ 0 ], gbc );

                add ( outputs[ 0 ] );

                lab = new Label( "Visits:" );

                lab.setBackground( super.getBackground() );

                lab.setAlignment ( Label.LEFT );

                lab.setFont( font );

                gbc = new GridBagConstraints();

                gbc.gridx = 0; gbc.gridy = 1; gbc.gridwidth = 1; gbc.gridheight = 1;

                gbc.ipadx = 0; gbc.ipady = 0; gbc.fill = GridBagConstraints.HORIZONTAL;

                gbc.weightx = 0; gbc.weighty = 1;

                gbc.anchor = GridBagConstraints.CENTER;

                gbl.setConstraints ( lab, gbc );

                add ( lab );

                outputs[ 1 ] = new Label();

                outputs[ 1 ].setBackground( super.getBackground() );

                outputs[ 1 ].setAlignment( Label.LEFT );

                outputs[ 1 ].setFont( font );

                gbc = new GridBagConstraints();

                gbc.gridx = 1; gbc.gridy = 1; gbc.gridwidth = 2; gbc.gridheight = 1;

                gbc.ipadx = 0; gbc.ipady = 0; gbc.fill = GridBagConstraints.HORIZONTAL;

                gbc.weightx = 1; gbc.weighty = 1;

                gbc.anchor = GridBagConstraints.CENTER;

                gbl.setConstraints ( outputs[ 1 ], gbc );

                add ( outputs[ 1 ] );

                lab = new Label( "Since:" );

                lab.setBackground( super.getBackground() );

                lab.setAlignment ( Label.LEFT );

                lab.setFont( font );

                gbc = new GridBagConstraints();

                gbc.gridx = 0; gbc.gridy = 2; gbc.gridwidth = 1; gbc.gridheight = 1;

                gbc.ipadx = 0; gbc.ipady = 0; gbc.fill = GridBagConstraints.HORIZONTAL;

                gbc.weightx = 0; gbc.weighty = 1;

                gbc.anchor = GridBagConstraints.CENTER;

                gbl.setConstraints ( lab, gbc );

                add ( lab );

                outputs[ 2 ] = new Label();

                outputs[ 2 ].setBackground( super.getBackground() );

                outputs[ 2 ].setAlignment( Label.LEFT );

                outputs[ 2 ].setFont( font );

                gbc = new GridBagConstraints();

                gbc.gridx = 1; gbc.gridy = 2; gbc.gridwidth = 2; gbc.gridheight = 1;

                gbc.ipadx = 0; gbc.ipady = 0; gbc.fill = GridBagConstraints.HORIZONTAL;

                gbc.weightx = 1; gbc.weighty = 1;

                gbc.anchor = GridBagConstraints.CENTER;

                gbl.setConstraints ( outputs[ 2 ], gbc );

                add ( outputs[ 2 ] );

                validate();
        }

        public void run()
        {
                boolean broken = false;

                while ( broken == false ) {
                        boolean written_to = false;
                        boolean read_from = false;

                        try {
                                if ( isConnected() == false ) {
                                        try {
                                                Thread.sleep( ( long ) 5000 );
                                        }

                                        catch ( InterruptedException ie ) {}

                                        connect();
                                }

                                int i;

                                for ( i = 0; i < 3; i++ ) {
                                        threadMsg msg = null;

                                        switch ( i ) {
                                        case 0:
                                                msg = new uptimeMsg();
                                                break;
                                        case 1:
                                                msg = new visitsMsg();
                                                break;
                                        case 2:
                                                msg = new usersMsg();
                                                break;
                                        }

                                        boolean msg_sent = false;

                                        if ( isConnected() == true ) {
                                                msg_sent = send( msg.toString() );
                                        }

                                        String inputLine = null;

                                        if ( msg_sent ) {
                                                try {
                                                        inputLine = is.readLine();
                                                        processInput( inputLine );
                                                        read_from = true;
                                                        written_to = true;
                                                }

                                                catch ( Exception e ) {
                                                        inputLine = null;
                                                }
                                        }

                                        if ( inputLine == null ) {
                                                outputs[ i ].setText( "" );
                                                written_to = true;
                                        }
                                }
                        }

                        catch ( Exception e ) {}

                        if ( read_from == false ) {
                                outputs[ 2 ].setText( "no response" );

                                if ( written_to == false ) {
                                        outputs[ 0 ].setText( "" );
                                        outputs[ 1 ].setText( "" );
                                }
                        }

                        validate();

                        try {
                                Thread.sleep( ( long ) 15000 );
                        }

                        catch ( InterruptedException ie ) {
                                broken = true;
                        }
                }
        }

        public void interrupt()
        {
                super.interrupt();

                if ( ( status_reader != null ) && ( status_reader.isAlive() ) ) {
                        status_reader.interrupt();
                        status_reader = null;
                }
        }

        private void processInput( String inputLine )
        {
                String output = null;
                int field = -1;
                usersMsg um = new usersMsg( inputLine );
                visitsMsg vm = new visitsMsg( inputLine );
                uptimeMsg upm = new uptimeMsg( inputLine );

                if ( inputLine.startsWith( um.getCommand() ) ) {
                        output = "" + um.getUserCount();
                        field = 0;
                }

                else if ( inputLine.startsWith( vm.getCommand() ) ) {
                        output = "" + vm.getVisitCount();
                        field = 1;
                }

                else if ( inputLine.startsWith( upm.getCommand() ) ) {
                        output = upm.getUpTime();
                        field = 2;
                }

                if ( output != null )
                        outputs[ field ].setText( output );
        }

} // end Status
