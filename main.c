#include "./camera/fast_2_timer/e_po3030k.h"
#include "./uart/e_uart_char.h"
//#include "./I2C/e_I2C_protocol.h"
#include "./motor_led/e_init_port.h"
#include "./motor_led/e_led.h"

#include <stdlib.h>

#define CAM_BUFFER_SIZE	 2*120	//RGB 565 needs 16 bits 5 red 6 gr 5 blue
char cam_buffer[CAM_BUFFER_SIZE];
char red_buffer[CAM_BUFFER_SIZE/2];

void init_cam(void)
{
	e_po3030k_init_cam();
	//top left pixel is at (0, 120)
	//original picture is 640 x 4
	//zoom is 4 on both axes so final size is 120 x 1
	e_po3030k_config_cam(0,ARRAY_HEIGHT/4, ARRAY_WIDTH, 4, 4, 4,  RGB_565_MODE);
	//e_po3030k_set_mirror(1,1);
	e_po3030k_write_cam_registers();
}

void cuteFlash(void)
{
	int i, j;
	unsigned long delay = 500000;
	for(i=0; i<8; i++)
	{
		e_set_led(i, 1);
		for(j=0; j<delay; j++);
	}
	for(i=0; i<8; i++)
	{
		e_set_led(i, 0);
		for(j=0; j<delay; j++);
	}
	return;
}

void writeIntToUART(int value)
{
	e_send_uart1_char(&value, 2);
	while(e_uart1_sending() );
	e_send_uart1_char(' ', 1);
	while(e_uart1_sending() );
	return;
}

void capture(void)
{
	e_po3030k_launch_capture(cam_buffer);
	while(!e_po3030k_is_img_ready());
}

/**
Takes the image and extracts the red green and blue values. 
Saves to the red_buffer array the sum of each pixels R -G -B
*/
void extractRed(void)
{
	int i;
	for(i=0; i<CAM_BUFFER_SIZE/2; i++)
	{
		char red, green, blue;
		
		//RGB 565 stores R as 5 bytes, G as next 6 and B as last 5. Making 16bits
		//all values are stored as the 5 (or 6 for green) MSB in the char
		red = (cam_buffer[2*i] & 0xF8);
		blue = ((cam_buffer[2*i+1] & 0x1F) << 3);
		green = (((cam_buffer[2*i] & 0x07) << 5) | ((cam_buffer[2*i+1] & 0xE0) >> 3));
		
		red_buffer[i] = red - green - blue;
	}
	return;
}

void detectRed(int *store, int length)
{
	if(length != 5) return;
	static int meanDetected[5] = {0, 0, 0, 0, 0};
	static int numDetections = 1;
	static int lastDetection[5] = {0, 0, 0, 0, 0};
	
	const int buffWidth = 120;
	int i, j;
	int thisCapture[5] = {0, 0, 0, 0, 0};
	
	//take picture
	capture();
	//extract red elements
	extractRed();
	//divide red buffer into 5 vertical sections
	for(i=0; i<5; i++)
	{
		//sum red in each section
		for(j=0; j<buffWidth/5; j++)
		{
			thisCapture[i] += red_buffer[i*buffWidth/5 + j];
		}
		writeIntToUART(thisCapture[i]);
		//average this result with the results of previous detections.
		meanDetected[i] = (meanDetected[i]*numDetections) + thisCapture[i];
		meanDetected[i] = meanDetected[i]/(1+numDetections);
	}
	numDetections++;
	
	//check results for red
	for(i=0; i<5; i++)
	{
		//did we see a red light? If thisCapture > meanDetected then yes!
		if(thisCapture[i] > meanDetected[i])
		{
			//did we see a red light in this section last time? 
			if(lastDetection[i] == 1)
			{
				//If so then don't report the detection.
				thisCapture[i] = 0;
			}
			else thisCapture[i] = 1;
		}else thisCapture[i] = 0;
		
		//update lastDetection
		lastDetection[i] = thisCapture[i];
		//copy findings into array given to function
		store[i] = thisCapture[i];
	}	
	
	return;
}

 
 int main(void)
 {
        int i;
        long delay = 1000000;
        long j;
        
        e_init_port();
        e_init_uart1();
        init_cam();
        
        e_send_uart1_char("\f\a", 2);
        while(1)
        {
	        int redDetection[5];
	        //wander around
	        //avoid obstacles
	        //detect flashes
	        detectRed(redDetection, 5);
	        for(j=0; j<delay; j++){}
        }
 }


