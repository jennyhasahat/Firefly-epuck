#include <stdlib.h>	//for random number generator
#include <stdio.h>	//for sprintf

#include <uart/e_uart_char.h>
#include <motor_led/e_epuck_ports.h>
#include <motor_led/e_init_port.h>
#include <motor_led/advance_one_timer/e_led.h>
#include <motor_led/advance_one_timer/e_motors.h>
#include <motor_led/advance_one_timer/e_agenda.h>
#include <a_d/e_prox.h>
#include <camera/fast_2_timer/e_poxxxx.h>

//something which allows us to use an interrupt to do the led flash.
//#define _ISRFAST	__attribute__((interrupt,shadow))

#define IMAGE_WIDTH			80
#define IMAGE_HEIGHT		15
#define CAM_BUFFER_SIZE IMAGE_WIDTH*IMAGE_HEIGHT*2
#define NUM_IMAGE_SEGMENTS	1

//variables that are pretty useful to have globally

char camera_buffer[CAM_BUFFER_SIZE]; // __attribute__ ((far));
char isTurning = 0;	//flag indicating if epuck is currently turning (can't have bools apparently)
int flashCounter = 0; //variable that times when the robot should flash
char isFlashing = 0; //flag indicating if epuck is currently flashing (can't have bools apparently)


/**Gets the position of the selector thing on top of the robot.
@return value position of the selector from 0 to 15.
@author Renato Garcia (this function's code taken from epuck player driver)
*/
int get_selector()
{
  return SELECTOR0 + 2*SELECTOR1 + 4*SELECTOR2 + 8*SELECTOR3;
}

//==================================================================
//					INITIALISATION FUNCTIONS
//==================================================================

/**
Calls all the functions that initialise the epuck hardware we're going to use.
Initialises the camera to record an 80x15 capture of the top half of the camera image.
*/
void init_epuck()
{
	e_init_port();
	
	/* Reset if Power on (some problem for a few robots) */
	if (RCONbits.POR)
	{
		RCONbits.POR=0;
		RESET();
	}
	
	e_start_agendas_processing();
	e_init_motors();
	e_init_prox();
	e_init_uart1();
		
	e_poxxxx_init_cam();
	
	/*takes a 640x240 image reduces by 4 and 16 to a 160x15 image*/
	e_poxxxx_config_cam(0, 0, ARRAY_WIDTH, (ARRAY_HEIGHT*0.5), 8, 16,  RGB_565_MODE);
	//set mirror is needed because epuck camera is actually upside down
	e_poxxxx_set_mirror(1,1);
	//save settings
	e_poxxxx_write_cam_registers();
}

//==================================================================
//					USART FUNCTIONS
//==================================================================

/**
Sends a single character over bluetooth.
@param character the char to send.
@author Renato Garcia (this function's code taken from epuck player driver)
*/
void send_char(char character)
{
  e_send_uart1_char(&character, 1);
  while(e_uart1_sending());
}

/**
Sends an integer over bluetooth as a set of chars.
If you're monitoring the epuck's usart using tinyBld or Putty then no translation of ascii is required, this will display the int as a number directly.
*/
void send_int_as_char(int integer)
{
	int length = 0;
	int n = integer;
	
	//how many digits in the integer?
	if(integer > -1)
	{
		//algorithm adapted from wikipedia itoa page (K&R implementation)
		do length++;
		while( (n/=10) > 0);
		char message[length];
		sprintf(message, "%d", integer);
		//itoa(integer, message, 10);
		e_send_uart1_char(message, length);
	}
	else
	{
		//algorithm adapted from wikipedia itoa page (K&R implementation)
		do length++;
		while( (n/=10) > 0);
		length++;
		char message[length];
		sprintf(message, "-%d", integer);
		e_send_uart1_char(message, length);	
	}
	while( e_uart1_sending() ){}
	return;
}




//==================================================================
//					FLASHING FUNCTIONS
//==================================================================

/**
Function to turn off the LEDs. This function is automatically called by an agenda set up in startFlash().
*/
void stopFlash(void)
{
	//turn LEDs off
	e_set_led(10, 0);
	flashCounter = 0;	
	//tell agenda not to continue timing this function.
	isFlashing = 0;
	e_destroy_agenda(stopFlash);
}

/**
Function to flash all the leds.
Uses an interrupt timer to do this.
*/
void startFlash(void)
{
	//if the robot is not already flashing...
	if(!isFlashing)
	{
		//turn LEDs on...
		e_set_led(10, 1);	//using led value 0-7 sets a specific LED any other sets them all.
		//tell agenda to stop flashing after 3 secs.
		e_activate_agenda(stopFlash, 30000);
		isFlashing = 1;
	}
}



//==================================================================
//					IMAGING FUNCTIONS
//==================================================================


/**Fills the camera_buffer array with pixel data from the camera.*/
void capture()
{	
	e_poxxxx_launch_capture(camera_buffer);
	while(!e_poxxxx_is_img_ready());
}


/**
Takes the image and extracts the red green and blue values. 
If the red value of the pixel is above a certain threshold and the green is below it then the
pixel is given the value 10, otherwise it is zero. This is to try and extract red and magenta colours
but not white or yellow. 

After this function is called, the first CAM_BUFFER_SIZE/2 indices
will be the extracted data, the CAM_BUFFER_SIZE/2 to CAM_BUFFER_SIZE will be garbage.
*/
void extractRed(void)
{
	int i;
	
	for(i=0; i<CAM_BUFFER_SIZE/2; i++)
	{
		int red, green; //, blue;
		double threshold = 0.7;
		
		//RGB 565 stores R as 5 bytes, G as next 6 and B as last 5. Making 16bits
		//all values are stored as the 5 (or 6 for green) MSB in the char
		red = (camera_buffer[2*i] & 0xF8);
	//	blue = ((camera_buffer[2*i+1] & 0x1F) << 3);
		green = (((camera_buffer[2*i] & 0x07) << 5) | ((camera_buffer[2*i+1] & 0xE0) >> 3));
		
		//kinda want red and magenta but NOT white or yellow.
		//therefore green has negative effect.
		if( (red > 255*threshold) && (green < 255*(1-threshold)) )
			camera_buffer[i] = 10;
		else camera_buffer[i] = 0;
	}
	
	return;
}


/**
Fuction to detect if another epuck has flashed.
It will take a picture and called extractRed(). Then it divides the image into vertical segments. 
In each of these segments it sums the red pixels and saves that number as an indication of the amount of red in the segment.
As the program goes along it learns what the average amount of red it observes should be (it gets more accurate the more readings are taken)
this is compared to the current reading, if that is above the average then a flash has been detected!
Finally it compares the result for each segment with the result last time this function was called. 
If a flash is detected in the same segment twice in a row then it's assumed to be the same flash and the flash is ignored.
@return the number of segments that detected a flash.
*/
int numberFlashesDetected(void)
{
//	static unsigned int meanDetected[NUM_IMAGE_SEGMENTS] = {0, 0, 0, 0, 0};
//	static unsigned int lastCapture[NUM_IMAGE_SEGMENTS] = {0, 0, 0, 0, 0};
//	unsigned int thisCapture[NUM_IMAGE_SEGMENTS] = {0, 0, 0, 0, 0};

	const int pixelsPerSegment = IMAGE_WIDTH*IMAGE_HEIGHT/NUM_IMAGE_SEGMENTS;
	const int memoryLength = 3;
	static unsigned int lastCapture[NUM_IMAGE_SEGMENTS][3];
	unsigned int meanDetected[NUM_IMAGE_SEGMENTS] = {0};
	static int lastResult = 0;
	unsigned int thisCapture[NUM_IMAGE_SEGMENTS] = {0};
	
	int i, j;
	
	int result = 0; //the variable to return
	
	//take picture
	capture();
	extractRed();

	//divide red buffer into NUM_IMAGE_SEGMENTS vertical sections
	for(i=0; i<NUM_IMAGE_SEGMENTS; i++)
	{
		//sum red in each section
		for(j=0; j<pixelsPerSegment; j++)
		{
			thisCapture[i] += camera_buffer[i*pixelsPerSegment + j];
		}
		
		//average this result with the remembered results of previous detections.
		meanDetected[i] = 0;
		for(j=0; j<memoryLength; j++)
		{
			meanDetected[i] += lastCapture[i][j];
		}
		meanDetected[i] = meanDetected[i] / memoryLength;
		
/*		send_char('M');
		send_int_as_char(meanDetected[i]);	
		send_char('T');
		send_int_as_char(thisCapture[i]);
		send_char(' ');	
*/		
		//did we see a red light? If thisCapture > (meanDetected + some%) then yes!
		if(thisCapture[i] > (meanDetected[i] * 2.5))
		{
			//did we see a red light in this section last time? 
			if(lastResult > 0)
			{
				//If so then don't report the detection.
				thisCapture[i] = 0;
			}
			else result += 1;
		}else result = 0;
		
		//update lastCapture
		for(j=memoryLength; j>0; j--)
		{
			lastCapture[i][j] = lastCapture[i][j-1];
		}	
		lastCapture[i][0] = thisCapture[i];
	}

	//and return the sum of the number of segments which found a flash
	return result;
}



//==================================================================
//					MOTOR FUNCTIONS
//==================================================================


/** Stop the motor activities
 *
 * Set the speed for zero and clean the step counter
@author Renato Garcia (this function's code taken from epuck player driver)
 */
void stop_motors()
{
  e_set_speed_right(0);
  e_set_speed_left(0);

  e_set_steps_right(0);
  e_set_steps_left(0);
}

/**
Turns the robot to avoid obstacles.
Checks the front IR sensors and if something is detected it will turns the 
robot until the front IR sensors detect nothing in the way.
*/
void avoidObstacles(int* left, int* right)
{
	int scoreLeft = 0.3*e_get_prox(5) + 0.7*e_get_prox(6) + e_get_prox(7);
	int scoreRight = 0.3*e_get_prox(2) + 0.7*e_get_prox(1) + e_get_prox(0);
	const int turnspeed = 150;
	
	//if there is something straight ahead
	if(scoreLeft > 900 && scoreRight > 900)
	{
		//indicate that you're turning
	//	isTurning = 1;
		*left = -turnspeed;
		*right = turnspeed;
		e_send_uart1_char("ahead ", 6);
		while( e_uart1_sending() ){}
	}//if there is something to the left
	else if(scoreLeft > 900)
	{
		//indicate that you're turning
	//	isTurning = 1;
		//turn right slowly
		*left = turnspeed;
		*right = -turnspeed;
		e_send_uart1_char("left ", 5);
		while( e_uart1_sending() ){}
	}
	else if(scoreRight > 900)
	{
		//indicate that you're turning
	//	isTurning = 1;
		//turn right slowly
		*left = -turnspeed;
		*right = turnspeed;
		e_send_uart1_char("right ", 6);
		while( e_uart1_sending() ){}
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
	else if(!isTurning)
	{
		*left = 400;
		*right = 400;
	}
	
	return;
}

/**
Initialises the epuck and calls the behaviour functions. 
This function is where we decide whether to flash or not using information from numberFlashesDetected().
*/
int main(void)
{
		long delay = 250000;
		long j;
		const int flashThreshold = 30;
		const int flashIncrement = 5;
		
	
		init_epuck();
        
        srand(100);
        
		e_send_uart1_char("firefly", 7);
		while( e_uart1_sending() ){}
        
        int leftSpeed = 600;
        int rightSpeed = 600;
	//	snake_led();
        
        while(1)
        {			
			//wander around
			wander(&leftSpeed, &rightSpeed);
			//avoid obstacles
			avoidObstacles(&leftSpeed, &rightSpeed);
			//detect flashes
			if(numberFlashesDetected() > 0)
			{
				flashCounter += flashIncrement;
				e_send_uart1_char("flash detected", 14);
				while( e_uart1_sending() ){}
			}
			
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
	//		e_set_led(4, 2);
			send_int_as_char(flashCounter);
			send_char(' ');
		}
		
		e_stop_prox();
		e_end_agendas_processing();
		return 0;
}


