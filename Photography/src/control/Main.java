package control;

import java.awt.image.BufferedImage;
import java.io.BufferedInputStream;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

import javax.imageio.ImageIO;

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
		} catch (IOException e) {
			System.err.println("couldn't establish serial IO with epuck");
			e.printStackTrace();
			return;
		}
		recData = epuckBT.getInputStream();
		sendData = epuckBT.getOutputStream();
	}
	
	private void readImage(String filename)
	{
		byte bytebuffer[] = new byte[1024];
		int length;
		File f = new File(filename);
		FileOutputStream fstream;
		BufferedInputStream buff = new BufferedInputStream(recData);
		
		try 
		{
			fstream = new FileOutputStream(f);
		} catch (FileNotFoundException e) 
		{
			System.err.println("couldn't create a file at "+filename);
			e.printStackTrace();
			return;
		}
		
		//code I found on the internet for converting from inputstream to file
		try 
		{
			while(buff.available() >0) //length is no bytes read to buffer
			{
				length = buff.read(bytebuffer);
				fstream.write(bytebuffer, 0, length);
			}
			fstream.close();
		} catch (IOException e) {
			System.err.println("there were IO troubles writing the file");
			e.printStackTrace();
		}
		return;
	}
	
	private void readImage2(String filename)
	{
		BufferedImage picture;
		File f = new File(filename);
		
		try {
			picture = ImageIO.read(new BufferedInputStream(recData));
			System.out.println("writing image to file");
			ImageIO.write(picture, "png", f);
		} catch (IOException e) {
			System.err.println("ImageIO class couldn't read or possibly write image");
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
		try 
		{
			tmp.epuckBT.sendString("x");
		} catch (IOException e) {
			System.err.println("couldn't write to epuck");
			e.printStackTrace();
		}
		System.out.println("reading string");
		tmp.readImage2("./test.jpg");
		
		tmp.closeIO();
		return;
	}

}
