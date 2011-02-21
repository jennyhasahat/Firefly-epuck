#include <camera/fast_2_timer/e_poxxxx.h>
#include <motor_led/e_init_port.h>
#include <motor_led/advance_one_timer/e_agenda.h>
#include <motor_led/advance_one_timer/e_led.h>
#include <motor_led/e_epuck_ports.h>
#include <uart/e_uart_char.h>

#include "epuck_utilities.h"

//#define CAM_BUFFER_SIZE 4*(480/2)*2
#define CAM_BUFFER_SIZE 40*40*2

char camera_buffer[CAM_BUFFER_SIZE]; // __attribute__ ((far));
//char red_buffer[CAM_BUFFER_SIZE/2] __attribute__ ((far));

void init_cam()
{
	e_poxxxx_init_cam();
	e_poxxxx_config_cam(0,0,640,480,16,12,RGB_565_MODE);
	e_poxxxx_set_mirror(1, 1);
	e_poxxxx_write_cam_registers();
}

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
	e_send_uart1_char("stopping flash ", 15);
	while( e_uart1_sending() ){}
	//turn LEDs off
	e_set_led(10, 0);
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
	int result = e_activate_agenda(stopFlash, 50000);
	send_int_as_char(result);
	while( e_uart1_sending() ){}
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
		int out;
		
		//RGB 565 stores R as 5 bytes, G as next 6 and B as last 5. Making 16bits
		//all values are stored as the 5 (or 6 for green) MSB in the char
		red = (camera_buffer[2*i] & 0xF8);
		//blue = ((camera_buffer[2*i+1] & 0x1F) << 3);
		//green = (((camera_buffer[2*i] & 0x07) << 5) | ((camera_buffer[2*i+1] & 0xE0) >> 3));
		
		out = red;// - green - blue;
	}
	
	return;
}

 
int main(void)
{
		e_init_port();
		
		/* Reset if Power on (some problem for few robots) */
		if (RCONbits.POR)
		{
			RCONbits.POR=0;
			RESET();
		}
		
		e_start_agendas_processing();
		init_cam();
        e_init_uart1();
        
		e_send_uart1_char("checkred ", 9);
        //cute_flash();
        
        while(1)
        {
	        //wait for signal to take picture
			char ch=' ';
			//get ch from uart until we receive an x
			while(ch != 'x')
			{
				//is_char checks for incoming data from uart1
				if(e_ischar_uart1())
				{
					e_getchar_uart1(&ch);
				}
			}
			startFlash();
		/*	e_poxxxx_launch_capture(camera_buffer);
			while(!e_poxxxx_is_img_ready());
			
			e_send_uart1_char(camera_buffer, CAM_BUFFER_SIZE);
			while(e_uart1_sending()){}
			//cute_flash();
		
				
			
			//wait for signal to take picture
			ch=' ';
			//get ch from uart until we receive an x
			while(ch != 'x')
			{
				//is_char checks for incoming data from uart1
				if(e_ischar_uart1())
				{
					e_getchar_uart1(&ch);
				}
			}
			extractRed();
			e_send_uart1_char(camera_buffer, CAM_BUFFER_SIZE/2);
			while(e_uart1_sending()){}
			*/
		}
		
		return 0;
}


