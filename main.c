#include "./camera/fast_2_timer/e_po3030k.h"
#include "./uart/e_uart_char.h"
//#include "./I2C/e_I2C_protocol.h"
#include "./motor_led/e_init_port.h"
#include "./motor_led/e_led.h"

#define CAM_BUFFER_SIZE	 2*120	//RGB 565 needs 16 bits 5 red 6 gr 5 blue
char cam_buffer[CAM_BUFFER_SIZE];
char red_buffer[CAM_BUFFER_SIZE/2];

void init_cam(void)
{
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
	for(i=0; i<CAM_BUFFER_SIZE/2; I++)
	{
		char red, green, blue;
		
		//RGB 565 stores R as 5 bytes, G as next 6 and B as last 5. Making 16bits
		red = (cam_buffer[2*i] & 0xF8);
		blue = ((cam_buffer[2*i+1] & 0x1F) << 3);
		green = (((cam_buffer[2*i] & 0x07) << 5) | ((cam_buffer[2*i+1] & 0xE0) >> 3));
		
		red_buffer[i] = red - green - blue;
	}
	return;
}

 
 int main(void)
 {
        char ch;
        int count, i;
        
        e_init_port();
        e_init_uart1();
        
        e_send_uart1_char("\f\a", 2);
        while(1)
        {
	        //wait for signal from uart
	        while(e_ischar_uart1() == 0);
	        e_send_uart1_char("you wrote: ",11);
	        
	        //now there might have been a char
	        
	        count = e_ischar_uart1();
	        for(i=0; i<count; i++)
	        {
	        	e_getchar_uart1(&ch);
	        	e_send_uart1_char(&ch, 1);
				while(e_uart1_sending());
	        }
			e_send_uart1_char(" ", 1);
			while(e_uart1_sending());
        }
 }


