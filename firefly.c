#include <stdlib.h>	//for random number generator

#include <uart/e_uart_char.h>
#include <motor_led/e_epuck_ports.h>
#include <motor_led/e_init_port.h>
#include <motor_led/advance_one_timer/e_led.h>
#include <motor_led/advance_one_timer/e_motors.h>
#include <motor_led/advance_one_timer/e_agenda.h>
#include <a_d/e_prox.h>
#include <camera/fast_2_timer/e_poxxxx.h>

#include "epuck_utilities.h"

//something which allows us to use an interrupt to do the led flash.
//#define _ISRFAST	__attribute__((interrupt,shadow))

#define IMAGE_WIDTH			160
#define IMAGE_HEIGHT		15
#define CAM_BUFFER_SIZE IMAGE_WIDTH*IMAGE_HEIGHT*2
#define NUM_IMAGE_SEGMENTS	5

//variables that are pretty useful to have globally

char camera_buffer[CAM_BUFFER_SIZE]; // __attribute__ ((far));
char isTurning = 0;	//flag indicating if epuck is currently turning (can't have bools apparently)
int flashCounter = 0;

/**
Calls all the functions that initialise the epuck hardware we're going to use.
Initialises the camera to record an 80x15 capture of the top half of the camera image.
*/
void init_epuck()
{
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
	//e_init_ad_scan(ALL_ADC);
	e_init_uart1();
		
	e_poxxxx_init_cam();
	
	/*takes a 640x240 image reduces by 4 and 16 to a 160x15 image*/
	e_poxxxx_config_cam(0,ARRAY_HEIGHT/2, ARRAY_WIDTH, ARRAY_HEIGHT/2, 4, 16,  RGB_565_MODE);
	e_poxxxx_set_mirror(1,1);
	e_poxxxx_write_cam_registers();
}

/**Fills the camera_buffer array with pixel data from the camera.*/
void capture()
{	
	e_poxxxx_launch_capture(camera_buffer);
	while(!e_poxxxx_is_img_ready());
}


/**
Function to turn off the LEDs. This function is automatically called by an agenda set up in startFlash().
*/
void stopFlash(void)
{
	//turn LEDs off
	e_set_led(10, 0);
	flashCounter = 0;	
	//tell agenda not to continue timing this function.
	e_destroy_agenda(stopFlash);
}

/**
Function to flash all the leds.
Uses an interrupt timer to do this.
*/
void startFlash(void)
{
	//turn LEDs on...
	e_set_led(10, 1);	//using led value 0-7 sets a specific LED any other sets them all.
	//tell agenda to stop flashing every 5 secs.
	e_activate_agenda(stopFlash, 50000);
}


/**
Takes the image and extracts the red green and blue values. 
Saves to the camera_buffer array the red value of each pixels. After this function is called, the first CAM_BUFFER_SIZE/2 indices
will be the red data, the CAM_BUFFER_SIZE/2+1 to CAM_BUFFER_SIZE will be garbage.
*/
void extractRed(void)
{
	int i;
	
	for(i=0; i<CAM_BUFFER_SIZE/2; i++)
	{
		char red; //, green, blue;
		
		//RGB 565 stores R as 5 bytes, G as next 6 and B as last 5. Making 16bits
		//all values are stored as the 5 (or 6 for green) MSB in the char
		red = (camera_buffer[2*i] & 0xF8);
	//	blue = ((camera_buffer[2*i+1] & 0x1F) << 3);
	//	green = (((camera_buffer[2*i] & 0x07) << 5) | ((camera_buffer[2*i+1] & 0xE0) >> 3));
		
		camera_buffer[i] = red; // - green - blue;
	}
	
	return;
}

/**
Fuction to detect if another epuck has flashed.
It will take a picture and extract the red contents from it. Then it divides the image into "length" number of vertical segments. 
In each of these segments it sums the red pixels and saves that number as an indication of the amount of red in the segment.
As the program goes along it learns what the average amount of red it observes should be (it gets more accurate the more readings are taken)
this is compared to the current reading, if that is above the average then a flash has been detected!
Finally it compares the result for each segment with the result last time this function was called. 
If a flash is detected in the same segment twice in a row then it's assumed to be the same flash and the flash is ignored.
@return the number of segments that detected a flash.
*/
int numberFlashesDetected(void)
{
	static int meanDetected[NUM_IMAGE_SEGMENTS] = {0, 0, 0, 0, 0};
	static int numDetections = 1;
	static int lastDetection[NUM_IMAGE_SEGMENTS] = {0, 0, 0, 0, 0};
	
	const int bytesPerSegment = 2*IMAGE_WIDTH*IMAGE_HEIGHT/NUM_IMAGE_SEGMENTS;
	int i, j;
	int thisCapture[NUM_IMAGE_SEGMENTS] = {0, 0, 0, 0, 0};
	int result = 0; //the variable to return
	
	//take picture
	capture();

	//divide red buffer into NUM_IMAGE_SEGMENTS vertical sections
	for(i=0; i<NUM_IMAGE_SEGMENTS; i++)
	{
		//sum red in each section
		for(j=0; j<bytesPerSegment; j+=2)
		{
			//sum only red elements for each pixel
			thisCapture[i] += (camera_buffer[j] & 0xF8);
		}
		
		send_int_as_char(thisCapture[i]);
		
		//average this result with the results of previous detections.
		meanDetected[i] = (meanDetected[i]*numDetections) + thisCapture[i];
		meanDetected[i] = meanDetected[i]/(1+numDetections);
		
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
		//sum findings
		result = result+thisCapture[i];	
	}
	numDetections++;
	
	//and return the sum
	return result;
}

/**
Turns the robot to avoid obstacles.
Checks the front IR sensors and if something is detected it will turns the 
robot until the front IR sensors detect nothing in the way.
*/
void avoidObstacles(int* left, int* right)
{
	int scoreLeft = 0.7*e_get_prox(6) + e_get_prox(7);
	int scoreRight = 0.7*e_get_prox(1) + e_get_prox(0);
	
	//if there is something to the left
	if(scoreLeft > 900)
	{
		//indicate that you're turning
		isTurning = 1;
		//turn right slowly
		*left = 500;
		*right = 0;
	}
	else if(scoreRight > 900)
	{
		//indicate that you're turning
		isTurning = 1;
		//turn right slowly
		*left = 0;
		*right = 500;
	}
	
	return;
}

/**
Decides a left speed and right speed for the robot wheels so that it would be wandering around.
@param left address of int to write new left speed into. Between -800 and 800.
@param right address of int to write new right speed into. Between -800 and 800.
*/
void wander(int* left, int* right)
{
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

/**
Initialises the epuck and calls the behaviour functions. This function is where we decide whether to flash or not.
*/
int main(void)
{
		long delay = 500000;
		long j;
		const int flashThreshold = 50;
		const int flashIncrement = 10;
		
	
		init_epuck();
        
        //srand(100);
        
		e_send_uart1_char("firefly", 7);
		while( e_uart1_sending() ){}
        
        int leftSpeed = 600;
        int rightSpeed = 600;
		snake_led();
        
        while(1)
        {			
			//wander around
			wander(&leftSpeed, &rightSpeed);
			//avoid obstacles
			avoidObstacles(&leftSpeed, &rightSpeed);
			//detect flashes
			flashCounter += flashIncrement*numberFlashesDetected();
			
			//if the flash counter reaches our threshold then flash leds
			if(flashCounter > flashThreshold)
			{
				//reset counter
				flashCounter = 0;
				//flash lights
				startFlash();
			}
			
			//check that selector is set at position 0
			int selector = get_selector();
			if(selector == 0) 
			{
				e_set_speed_left(leftSpeed);
				e_set_speed_right(rightSpeed);
				//break;
			}else stop_motors();
			
			//increment flash counter
			flashCounter++;
			
			//wait for refresh
			for(j=0; j<delay; j++){}
			
			//toggle LED 4
			e_set_led(4, 2);
			send_int_as_char(flashCounter);
			send_char(' ');
		}
		
	//	e_stop_prox();
		e_end_agendas_processing();
		return 0;
}


