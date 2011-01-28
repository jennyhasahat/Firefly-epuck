package control;

import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.OutputStream;

import javax.bluetooth.BluetoothStateException;

import bluetoothing.BluetoothComputer;
import bluetoothing.BluetoothHandler;

public class Main {

	private BluetoothHandler epuckBT;
	private InputStream recData;
	private OutputStream sendData;
	
	public Main() 
	{
		epuckBT = new BluetoothComputer();
		epuckBT.initialiseEpuck(1837);
		try {
			epuckBT.establishSerialIO();
		} catch (BluetoothStateException e) {
			System.err.println("couldn't establish serial");
			e.printStackTrace();
			return;
		}
		recData = epuckBT.getInputStream();
		sendData = epuckBT.getOutputStream();
	}
	
	/**reads in a data packet from the epuck*/
	private void readPacket1()
	{
		InputStreamReader isr = new InputStreamReader(recData);
		try {
			//while(recData.available() > 0 && isr.ready())
			while(isr.ready())
			{
				try {
					System.out.print(isr.read());
				} catch (IOException e) {
					System.err.println("didn't like reading a char");
					e.printStackTrace();
				}
			}
		} catch (IOException e) {
			System.err.println("didn't like checking if the input stream reader was ready");
			e.printStackTrace();
		}
		return;
	}
	
	private void readPacket2()
	{
		int bufferLimit = 128;
		BufferedInputStream buffer = new BufferedInputStream(recData, bufferLimit);
		
		//wait for input
		try {
			while(buffer.available() == 0){}
		} catch (IOException e1) {
			System.err.println("error waiting for something from robot");
			e1.printStackTrace();
		}	
		
		try 
		{
			while(buffer.available() > 0)
			{
				try 
				{
					System.out.print((char)buffer.read());
				} catch (IOException e) {
					System.err.println("didn't like reading a char");
					e.printStackTrace();
				}
				//buffer.mark(bufferLimit);
			}
		} catch (IOException e) {
			System.err.println("didn't like checking if the input stream reader was ready");
			e.printStackTrace();
		}
		return;
	}
	
	/**sends a data packet to the epuck*/
	private void sendPacket()
	{
		BufferedOutputStream buffer = new BufferedOutputStream(sendData);
		BufferedInputStream in = new BufferedInputStream(System.in);
		
		System.out.println("press a key to send: ");
		int inchar;
		try {
			inchar = in.read();
			System.out.println("you entered "+inchar+" or (char)"+(char)inchar);
		} catch (IOException e1) {
			System.err.println("in didn't work somehow");
			e1.printStackTrace();
			return;
		}
		try 
		{
			buffer.write(inchar);	
			buffer.flush();
		} 
		catch (IOException e) 
		{
			System.err.println("some troubles sending to epuck");
			e.printStackTrace();
		}
		return;
	}
	
	private void closeIO()
	{
		epuckBT.closeIO();
		System.out.println("streams closed");
		return;
	}

	
	/**
	 * @param args
	 */
	public static void main(String[] args) {
		Main tmp = new Main();
		for(int i=0; i<1; i++)
		{
			tmp.sendPacket();
			System.out.println("reading from epuck");
			tmp.readPacket2();
			tmp.readPacket2();
		}
		tmp.closeIO();
		return;
	}

}
