//////////////////////////////////////////////////////////////////////////////
//                        DX SOURCEFILE                                     //
//////////////////////////////////////////////////////////////////////////////

/*
 * $Header: /src/master/dx/src/uipp/java/CaptionLabels.java,v 1.1 1999/07/09 17:27:26 daniel Exp $
 */

//
//
import dx.net.*;
import java.util.*;
import java.awt.*;
import java.applet.*;


public class CaptionLabels extends Applet implements dx.net.DXLinkHandler {

    private Hashtable handlers;

    public CaptionLabels() { 
	super(); 
	this.handlers = null;
    }

    public synchronized void init() {
	Font default_font = new Font("Helvetica", Font.BOLD, 14);
	Font new_font = null;
	String prop = this.getParameter("FONT");
	if ((prop == null) || ((new_font = Font.decode(prop)) == null))
	new_font = default_font;
	this.setFont(new_font);

	Color bg = this.parseColor("BACKGROUND");
	if (bg!=null) this.setBackground(bg);
	Color fg = this.parseColor("FOREGROUND");
	if (fg!=null) this.setForeground(fg);

	int size = 0;

	while (true) {
	    String param_name = "DXLOutput" + size;
	    if (this.getParameter(param_name) == null) break;
	    size++;
	}
	if (size == 0) return ;

	this.setLayout (new GridLayout(size, 1, 0, 0));
	this.handlers = new Hashtable(size*2);

	int i = 0;
	Label l = null;
	while (i<size) {
	    String param_name = "DXLOutput" + i++;
	    String param_val = this.getParameter(param_name);
	    this.add(l = new Label());
	    if (fg != null) l.setForeground(fg);
	    if (bg != null) l.setBackground(bg);
	    this.handlers.put(param_val, l);
	}
    }

    public void outputHandler (String key, String msg, Object data) {
	if (this.handlers == null) return ;
	Label l = (Label)this.handlers.get(key);
	if (l == null) return ;
	l.setText(msg);
    }

    public synchronized boolean hasHandler (String key) { 
	if (this.handlers == null) return false;
	return (this.handlers.get(key) != null); 
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
}


