package bluetoothing;

import java.io.InputStream;
import java.io.OutputStream;

import javax.bluetooth.BluetoothStateException;

/**Handler that deals with all that pesky bluetooth I/O so the rest of my  code doesn't have to.
 * This is an interface for the handlers to conform to so my code can be ported to other hardware platforms later (eg my android phone).
 * */
public interface BluetoothHandler 
{
	//initialisation stuff
	public boolean initialiseEpuck(int epuckNumber);
	public boolean establishSerialIO() throws BluetoothStateException;
	
	//actual IO stuff
	public InputStream getInputStream();
	public OutputStream getOutputStream();
	public void closeIO();
}
