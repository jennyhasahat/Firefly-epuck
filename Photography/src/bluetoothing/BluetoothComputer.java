package bluetoothing;

import java.awt.Dimension;
import java.awt.Image;
import java.awt.image.BufferedImage;
import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.Iterator;

import javax.bluetooth.BluetoothStateException;
import javax.bluetooth.DeviceClass;
import javax.bluetooth.DiscoveryAgent;
import javax.bluetooth.DiscoveryListener;
import javax.bluetooth.LocalDevice;
import javax.bluetooth.RemoteDevice;
import javax.bluetooth.ServiceRecord;
import javax.bluetooth.UUID;
import javax.imageio.ImageIO;
import javax.imageio.ImageReadParam;
import javax.imageio.ImageReader;
import javax.imageio.ImageTypeSpecifier;
import javax.imageio.metadata.IIOMetadata;
import javax.microedition.io.Connector;
import javax.microedition.io.StreamConnection;
import javax.swing.Action;
import javax.swing.Timer;

import com.intel.bluetooth.RemoteDeviceHelper;

/**
 * Class for dealing with bluetooth. This class is used when the local device is a computer (instead of say, an android phone). 
 * This class uses the JSR-82 java/bluetooth API from Bluecove.
 * */
public class BluetoothComputer implements BluetoothHandler
{
	//bluetooth devices
	private LocalDevice myBTRadio = null;
	private RemoteDevice epuck = null;
	
	//serial IO stuff
	public OutputStream sendData = null;
	public InputStream recData = null;
	private StreamConnection connection = null;
	private String epuckUrl;

	//flags
	private boolean inquiryRunning;
	private boolean epuckPaired;
	private boolean epuckRfcommServiceFound;
	private boolean serialInitialised;
	
	//other variables
	private int epuckPIN;

	public BluetoothComputer()
	{	
		inquiryRunning = false;
		epuckPaired = false;
		epuckPIN = 0;
		serialInitialised = false;
		try
		{
			myBTRadio = LocalDevice.getLocalDevice();
		} catch (BluetoothStateException e)
		{
			System.err.println("Could not find local bluetooth device. Is it plugged in?");
		}

		return;
	}

	/**Method to get the computer to pair with an epuck.
	 * First it searches for bluetooth devices.
	 * It checks each device found to see if it is an epuck
	 * If the device is an epuck it will try and authenticate with the provided pin number.
	 * Most of this code is taken from the RemoteDeviceDiscovery from the index of the bluecove javadocs.
	 * @param epuckNo the identification number of the epuck. This is the number written on the epuck.
	 * @return success 1 if the epuck is successfully discovered and authenticated. 0 if it is not.
	 * */
	public boolean initialiseEpuck(int epuckNo)
	{
		inquiryRunning = false;
		epuckPIN = epuckNo;
		epuckPaired = false;
		
		EpuckFinder searcher = new EpuckFinder();

		try 
		{
			System.out.println("searching for epuck "+epuckPIN+"...");
			inquiryRunning = myBTRadio.getDiscoveryAgent().startInquiry(DiscoveryAgent.GIAC, searcher);
		} catch (BluetoothStateException e) 
		{
			System.err.println("Local device couldn't start scanning for remote devices.");
			e.printStackTrace();
			return epuckPaired;
		}
		while(inquiryRunning){}	//wait for inquiry to complete
		if(!epuckPaired) System.err.println("could not find epuck");
		return epuckPaired;
	}

	/**
	 * Establishes an RFCOMM connection with the discovered epuck and sets up data streams for communicating with it.
	 * This function must be called if you want to pass data to and from the EPuck.
	 * @throws IOException  if the rfcomm service can't be found or established.
	 * @return success 1 if successful 0 otherwise
	 * */
	public boolean establishSerialIO() throws IOException
	{
		//do nothing if we haven't paired the epuck yet
		if(!epuckPaired) 
		{
			System.err.println("Cannot establish serial IO until epuck is initialised");
			return false;
		}
		serialInitialised = false;

		try 
		{
			connection = (StreamConnection)Connector.open(epuckUrl);
		} 
		catch (IOException ioe) 
		{
			System.out.println("couldn't find epuck's rfcomm service using saved url, running a service search.");
			if(!searchForEpuckRfcommService())
			{
				throw new IOException("could not find remote rfcomm service.", ioe);
			}
			try
			{
				connection = (StreamConnection)Connector.open(epuckUrl);
			} 
			catch(IOException ioe2) 
			{
				throw new IOException("Found, but could not establish remote rfcomm service.", ioe2);
			}
		}
		
		try 
		{
			sendData = connection.openOutputStream();
			recData = connection.openInputStream();
		} catch (IOException e) {
			System.err.println("could not open IO streams");
			e.printStackTrace();
			return false;
		}
		serialInitialised = true;
		System.out.println("serial IO has been established");
		
		return serialInitialised;
	}

	/**If serial has been established this will return the InputStream object used for serial IO with the robot.
	 * @return recData the input stream that receives data from the epuck. Null if serial is not established.
	 * */
	public InputStream getInputStream()
	{
		if(serialInitialised) return recData;
		else return null;
	}
	
	/**If serial has been established this will return the OutputStream object used for serial IO with the robot.
	 * @return sendData the output stream that sends data to the epuck. Null if serial is not established.
	 * */
	public OutputStream getOutputStream()
	{
		if(serialInitialised) return sendData;
		else return null;
	}
	
	/**Sends a text string to the epuck.
	 * @throws IOException if it couldn't write to the epuck.
	 * */
	public void sendString(String input) throws IOException
	{
		BufferedOutputStream buffer = new BufferedOutputStream(sendData);

		buffer.write(input.getBytes(), 0, input.length());	
		buffer.flush();

		return;
	}
	
	/**Reads in a string from the epuck.
	 * @param timeoutMS the amount of time in milliseconds to wait for data to arrive from the epuck.
	 * @return output the data from the epuck in string form. Null if there is no data to read.
	 * */
	public String readString(int timeoutMS)
	{
		BufferedInputStream buffer = new BufferedInputStream(recData);
		
		//wait for input
		if(!isEpuckDataAvailable(timeoutMS))
		{
			return null;
		}
		
		StringBuilder strbuild = new StringBuilder();
		
		try 
		{
			while(buffer.available() > 0)
			{
				char c = (char)buffer.read();
				strbuild.append(c);
			}
		}
		catch (IOException e) 
		{
			System.err.println("couldn't read from epuck");
			e.printStackTrace();
			return null;
		}
		
		return strbuild.toString();
	}
	
	
	
	
	
	
	
	
	
	public void closeIO()
	{
		if(serialInitialised)
		{
			try 
			{
				sendData.close();
				recData.close();
				connection.close();
			} catch (IOException e) {
				System.err.println("couldn't close IO streams");
				e.printStackTrace();
			}
		}
		return;
	}
	
	
	
	/**Checks the inputstream from the epuck to see if there is data available.
	 * @param timeoutMS the amount of time in milliseconds to wait for data.
	 * @return available true when there is data available.
	 * */
	public boolean isEpuckDataAvailable(int timeoutMS)
	{
		Timer time = new Timer(timeoutMS, null);
		time.setRepeats(false);
		time.start();
		while(time.isRunning())
		{
			try 
			{
				if(recData.available() > 0)
				{
					time.stop();
					return true;
				}
			} 
			catch (IOException e) 
			{
				System.err.println("couldn't check epuck for data");
				e.printStackTrace();
				return false;
			}
		}
		
		return false;
	}
	
	
	
	
	//==========================================================================
	//								PRIVATE METHODS
	//==========================================================================
	
	/**Searches the epuck to see if it provides the rfcomm bluetooth service. 
	 * Gets the url of the rfcomm service from the epuck so that we can talk to it.
	 * */
	private boolean searchForEpuckRfcommService()
	{
		int[] attr = {}; //0x0003, 0x000, 0x0001, 0x0002};
		UUID rfcomm = new UUID(0x0003); //UUID of the rfcomm service
		UUID[] uuidList = {rfcomm};
		
		epuckRfcommServiceFound = false;
		EpuckFinder searcher = new EpuckFinder();

		try {
			myBTRadio.getDiscoveryAgent().searchServices(attr, uuidList, epuck, searcher);
		} catch (BluetoothStateException e) {
			return epuckRfcommServiceFound;
		}
		inquiryRunning = true;
		while(inquiryRunning){}
		return epuckRfcommServiceFound;
	}


	private boolean isDesiredEpuck(RemoteDevice dev)
	{
		String name;
		try 
		{
			name = dev.getFriendlyName(false);
		} catch (IOException e) 
		{
			System.err.println("can't get device name");
			return false;
		}
		return name.contains("e-puck_"+epuckPIN);
	}

	private boolean authenticateEpuck()
	{
		boolean success;
		try {
			success = RemoteDeviceHelper.authenticate(epuck, new Integer(epuckPIN).toString());
		} catch (IOException e) {
			System.err.println("this epuck couldn't be authenticated with PIN "+epuckPIN);
			return false;
		}
		return success;
	}
	
	//==========================================================================
	//							DISCOVERYLISTENER METHODS
	//==========================================================================
	
	private class EpuckFinder implements DiscoveryListener
	{
		public void servicesDiscovered(int transID, ServiceRecord[] servRecord) 
		{
			//System.out.println("there were "+servRecord.length+" service(s) discovered");
			for (int i = 0; i < servRecord.length; i++) 
			{
				String url = servRecord[i].getConnectionURL(ServiceRecord.NOAUTHENTICATE_NOENCRYPT, false);
				if (url == null) 
				{
					continue;
				}
				//save the url we found
				epuckUrl = url;		
				epuckRfcommServiceFound = true;
				//stop searching
				myBTRadio.getDiscoveryAgent().cancelServiceSearch(transID);
				break;
			}	
		}

		public void serviceSearchCompleted(int transID, int arg1)
		{
			inquiryRunning = false;
		}

		public void inquiryCompleted(int arg0)
		{
			inquiryRunning = false;
		}

		public void deviceDiscovered(RemoteDevice dev, DeviceClass arg1) 
		{
			if(isDesiredEpuck(dev))
			{
				epuck = dev;
				System.out.println("found the epuck at BT address "+epuck.getBluetoothAddress());
				//if this epuck can be authenticated with the provided pin then stop searching.
				if(authenticateEpuck())
				{
					System.out.println("epuck "+epuckPIN+" has been authenticated.");
					epuckPaired = true;
					//guess at the rfcomm url now we know the epuck BT address
					epuckUrl = new String("btspp://"+epuck.getBluetoothAddress()+
					":1;authenticate=false;encrypt=false;master=false");
				}	
				//stop searching because we found what we wanted
				myBTRadio.getDiscoveryAgent().cancelInquiry(this);
				inquiryRunning = false;
			}
		}
	}

}
