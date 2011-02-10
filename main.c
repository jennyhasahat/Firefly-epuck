#include <stdlib.h>

#include <uart/e_uart_char.h>
#include <motor_led/e_epuck_ports.h>
#include <motor_led/e_init_port.h>
#include <motor_led/advance_one_timer/e_led.h>
#include <motor_led/advance_one_timer/e_motors.h>
#include <motor_led/advance_one_timer/e_agenda.h>
#include <a_d/e_prox.h>
#include <camera/fast_2_timer/e_po3030k.h>

#include "epuck_utilities.h"

#define CAM_BUFFER_SIZE 2*160

unsigned char camera_buffer[CAM_BUFFER_SIZE] __attribute__ ((far));
unsigned char red_buffer[CAM_BUFFER_SIZE/2] __attribute__ ((far));

void init_cam()
{
	e_po3030k_init_cam();
	
	//top left pixel is at (0, 120)
	//original picture is 640 x 4
	//zoom is 4 on both axes so final size is 120 x 1
	e_po3030k_config_cam(0,ARRAY_HEIGHT/4, ARRAY_WIDTH, 4, 4, 4,  RGB_565_MODE);
//	e_po3030k_set_mirror(1,1);
	e_po3030k_write_cam_registers();
	
	//int actualSize = (ARRAY_WIDTH/4) * (4/4) * 2;
	//send_int_as_char(actualSize);
}

void capture()
{
	e_po3030k_launch_capture((char *)camera_buffer);
	while(!e_po3030k_is_img_ready());
	//cute_flash();
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
		red = (camera_buffer[2*i] & 0xF8);
		blue = ((camera_buffer[2*i+1] & 0x1F) << 3);
		green = (((camera_buffer[2*i] & 0x07) << 5) | ((camera_buffer[2*i+1] & 0xE0) >> 3));
		
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
	
	e_set_led(4, 1);
	
	const int buffWidth = CAM_BUFFER_SIZE/2;
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
		//average this result with the results of previous detections.
		meanDetected[i] = (meanDetected[i]*numDetections) + thisCapture[i];
		meanDetected[i] = meanDetected[i]/(1+numDetections);
		
		send_int_as_char(thisCapture[i]);
		send_char(' ');
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
	e_set_led(4, 0);
	return;
}

/**
Decides a left speed and right speed for the robot wheels so that it would be wandering around.
@param left address of int to write new left speed into. Between -800 and 800.
@param right address of int to write new right speed into. Between -800 and 800.
*/
void wander(int* left, int* right)
{
	static char isTurning = 0;	//can't have bools apparently
	static unsigned int refreshCounter = 0;	//counts how often this func is called
	const unsigned int forwardCount = 5;
	const unsigned int turnCount = 3;
	
	refreshCounter++;
	
	//if we're turning and should stop...
	if(isTurning && (refreshCounter > turnCount) )
	{
		//stop turning and start going forwards at say 600 steps/s
		*left = 400;
		*right = 400;
		isTurning = 0;
		refreshCounter = 0;
	}
	//if we're going forward and should stop
	else if( !isTurning && (refreshCounter > forwardCount) )
	{
		//start turning a random amount
		const int turnFactors[10] = { -4, -3, -2, -1, 0, 1, 2, 3, 4, 5};
		*left = turnFactors[rand()%10] * 100;
		*right = turnFactors[rand()%10] * 100;
		
		isTurning = 1;
		refreshCounter = 0;
	}
	//otherwise maintain current wheel speeds
	//i.e. do nothing.
	
	return;
}

 
int main(void)
{
		long delay = 250000;
		long j;
		
		e_init_port();
		
		/* Reset if Power on (some problem for few robots) */
		if (RCONbits.POR)
		{
			RCONbits.POR=0;
			RESET();
		}
		
		e_start_agendas_processing();
		e_init_motors();
		e_init_prox();
		e_init_uart1();
		init_cam();
        
        //srand(100);
        
		e_send_uart1_char("firefly", 7);
        
        int leftSpeed = 600;
        int rightSpeed = 600;
        cute_flash();
        
        while(1)
        {
			int redDetection[5];
			
			//wander around
			wander(&leftSpeed, &rightSpeed);
			//avoid obstacles
			//detect flashes
			detectRed(redDetection, 5);
			
			//check that selector is set at position 0
			int selector = get_selector();
			if(selector == 0) 
			{
				e_set_speed_left(leftSpeed);
				e_set_speed_right(rightSpeed);
				//break;
			}else stop_motors();
			//wait for refresh
			for(j=0; j<delay; j++){}
		}
		
		//e_stop_prox();
		e_end_agendas_processing();
		return 0;
}


