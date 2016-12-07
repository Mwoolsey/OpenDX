//
package dx.net;

//-------------------------------------------------------
// 3/17/2006 jer
// Added method: setOutputValueQuietly(String) 
// This is to support Interactors that want to keep the 
// server informed of changed values without causing
// an execution.
//-------------------------------------------------------

public interface PacketIF {
    public void setInputValue (int param, String value);
    public void setOutputValue (String value);
    public void setOutputValues (String name, String value);
    public void setOutputValueQuietly (String value);  // 3/17/2006 jer
    public void handleMessage (String msg);
};

