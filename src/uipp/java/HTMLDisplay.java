//////////////////////////////////////////////////////////////////////////////
//                        DX SOURCEFILE                                     //
//////////////////////////////////////////////////////////////////////////////

/* HTMLDisplay.java
 * 
 */
//
//
import dx.net.*;
import java.util.*;
import java.awt.*;
import java.applet.*;

import java.net.URL;
import java.net.MalformedURLException;
import javax.swing.*;
import javax.swing.text.*;
import javax.swing.text.html.*;
import javax.swing.event.*;


public class HTMLDisplay extends JApplet
    implements dx.net.DXLinkHandler, HyperlinkListener {

    public static final String CONTENT_PARAM_NAME = "CONTENT-TYPE";
    public static final String CONTENT_TYPE_HTML = "text/html";
    public static final String CONTENT_TYPE_PLAIN = "text/plain";
    public static final String CONTENT_TYPE_RTF = "text/rtf";
    public static final String CONTENT_TYPE_URL = "text/url";

    public static final String[] CONTENT_TYPES = {
                CONTENT_TYPE_HTML, CONTENT_TYPE_PLAIN, CONTENT_TYPE_RTF, CONTENT_TYPE_URL
            };

    private HashMap names2index = null;
    private String[] strings = null;
    private JEditorPane	pane = null;
    private String contentType = null;
    private boolean doingUrls = false;

    public HTMLDisplay() {
        super();
    }

    public synchronized void init() {

        // ---------------------------------------
        int size = 0;
        this.names2index = new HashMap();
        while (true) {
            String dxlNum = "DXLOutput" + size;
            String dxlName = this.getParameter(dxlNum);
            if (dxlName == null) break;
            this.names2index.put( dxlName, new Integer(size) );
            size++;
        }
        if (size == 0) return ;

        //debug( this.names2index );

        int i = 0;
        this.strings = new String[size];
        while (i<size) {
            this.strings[i] = "";
            i++;
        }

        // ---------------------------------------
        Font default_font = new Font("Helvetica", Font.BOLD, 14);
        Font new_font = null;
        String prop = this.getParameter("FONT");
        if ((prop == null) || ((new_font = Font.decode(prop)) == null))
            new_font = default_font;
        this.setFont(new_font);

        // ---------------------------------------
        Color bg = this.parseColor("BACKGROUND");
        if (bg!=null) this.setBackground(bg);
        Color fg = this.parseColor("FOREGROUND");
        if (fg!=null) this.setForeground(fg);

        // ---------------------------------------
        this.contentType = CONTENT_TYPE_HTML;
        this.doingUrls = false;
        String ct = this.getParameter(CONTENT_PARAM_NAME);
        if(ct != null) {
            int ii;
            ct = ct.toLowerCase();
            for(ii = 0; ii < CONTENT_TYPES.length; ii++){
                if( ct.equals(CONTENT_TYPES[ii]) ){
                    this.contentType = CONTENT_TYPES[ii];
                    break;
                }
            }

            if( ii >= CONTENT_TYPES.length ) {
                this.contentType = CONTENT_TYPE_PLAIN;
                System.err.println("WARNING: Unknown content-type: " + ct
                                   + ".  Defaulting to: " + this.contentType + ".");
            }
        }

        if( this.contentType == CONTENT_TYPE_URL ){
            this.contentType = CONTENT_TYPE_HTML;
            this.doingUrls = true;
        }

        //debug( "Content-type: " + this.contentType );

        // ---------------------------------------
        this.pane = new JEditorPane();
        this.pane.setContentType(this.contentType);
        if (fg != null) this.pane.setForeground(fg);
        if (bg != null) this.pane.setBackground(bg);
        this.pane.setEditable(false);
        if( this.contentType == CONTENT_TYPE_HTML )
            this.pane.addHyperlinkListener(this);

        JScrollPane editorScrollPane = new JScrollPane(pane);
        editorScrollPane.setPreferredSize(new Dimension(640, 480));
        editorScrollPane.setMinimumSize(new Dimension(10, 10));

        // ---------------------------------------
        this.getContentPane().setLayout (new BorderLayout());
        this.getContentPane().add(editorScrollPane, BorderLayout.CENTER);

    }

    public void outputHandler (String key, String msg, Object data) {
        //debug(">>> handler: key("+key+")  msg("+msg+")  data("+data+")");
        try {
            Integer N = (Integer) this.names2index.get(key);
            if(N == null)
                return;
            int n = N.intValue();
            this.strings[n] = msg;
            if(this.doingUrls)
                this.loadURL(n);
            this.display();
        }
        catch (Exception e){
            e.printStackTrace();
            return;
        }
    }

    private DXLinkApplication findDXLinkApplication(){
        AppletContext ac = this.getAppletContext();
        Enumeration en = ac.getApplets();
        while(en.hasMoreElements()){
            Applet a = (Applet) en.nextElement();
            if( a instanceof DXLinkApplication )
                return (DXLinkApplication) a;
        }
        throw new RuntimeException("No DXLinkApplication in applet context.");
    }

    private void display(){
        if (this.pane == null) return ;
        StringBuffer sb = new StringBuffer();
        for(int i = 0; i < this.strings.length; i++){
            sb.append(this.strings[i]);
        }
        this.pane.setDocument( this.pane.getEditorKit().createDefaultDocument() );
        this.pane.setText(sb.toString());
    }

    private void loadURL(int n){
        String msg = this.strings[n];
        String[] tokens = msg.split("[ \t]+");

        URL url = null;
        String target = null;

        try {
            url = new URL(tokens[0]);
            if(tokens.length >= 2)
                target = tokens[1];
            this.strings[n] = this.strings[n] + "<BR>";
        }
        catch (Exception e){
            System.err.println("URL string: " + tokens[0]);
            e.printStackTrace();
            return;
        }

        AppletContext ac = this.getAppletContext();
        if( target == null )
            ac.showDocument(url);
        else
            ac.showDocument(url, target);
    }

    public void hyperlinkUpdate(HyperlinkEvent e) {
        if (e.getEventType() == HyperlinkEvent.EventType.ACTIVATED) {
            JEditorPane pane = (JEditorPane) e.getSource();
            if (e instanceof HTMLFrameHyperlinkEvent) {
                System.err.println(
                    "Warning: frames support not verified. This might not work...");
                HTMLFrameHyperlinkEvent  evt =
                    (HTMLFrameHyperlinkEvent)e;
                HTMLDocument doc =
                    (HTMLDocument)pane.getDocument();
                doc.processHTMLFrameHyperlinkEvent(evt);
            } else {
                try {
                    URL url = e.getURL();

                    // If URL looks like 'http:#...',
                    // send the '...' part to
                    // DXLInputs.
                    if( (url.getAuthority() == null || url.getAuthority().equals(""))
                            && url.getHost().equals("")
                            && url.getFile().equals("") ){
                        String ref = url.getRef();
                        this.findDXLinkApplication().DXLSend(ref);
                        return;
                    }

                    String target = null;
                    AttributeSet aset = e.getSourceElement().getAttributes();
                    aset = (AttributeSet) aset.getAttribute(HTML.Tag.A);
                    if(aset == null){
                        System.err.println(
                            "Sorry, only A tags supported for hyperlinking.");
                        return;
                    }
                    if(url == null){
                        String href = (String) aset.getAttribute(HTML.Attribute.HREF);
                        url = new URL( this.getDocumentBase(), href );
                    }
                    target = (String) aset.getAttribute( HTML.Attribute.TARGET );
                    if(target == null)
                        this.getAppletContext().showDocument(url);
                    else
                        this.getAppletContext().showDocument(url, target);
                } catch (Throwable t) {
                    t.printStackTrace();
                }
            }
        }
    }

    public synchronized boolean hasHandler (String key) {
        boolean ret = this.names2index.containsKey(key);
        //debug("Has handler: key="+key+"  val="+ret);
        return ret;
    }

    private Color parseColor (String param) {
        Color c = null;
        String color = this.getParameter(param);
        if (color == null) return null;
        try {
            StringTokenizer stok = new StringTokenizer(color, "[, ]");
            String redstr = stok.nextToken();
            String greenstr = stok.nextToken();
            String bluestr = stok.nextToken();
            float red = Float.valueOf(redstr).floatValue();
            float green = Float.valueOf(greenstr).floatValue();
            float blue = Float.valueOf(bluestr).floatValue();
            c = new Color(red, green, blue);
        } catch (Exception e) {
        }
        return c;
    }

    private void debug(Object s){
        System.err.println(s.toString());
    }

}
