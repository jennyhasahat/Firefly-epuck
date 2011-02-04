#include "./camera/fast_2_timer/e_po3030k.h"
#include "./uart/e_uart_char.h"
#include "./I2C/e_I2C_protocol.h"
#include "./motor_led/e_init_port.h"
#include "./motor_led/e_led.h"

/*
#define CAM_BUFFER_SIZE	 2*120	//RGB 565 needs 16 bits 5 red 6 gr 5 blue
char cam_buffer[CAM_BUFFER_SIZE];

void init_cam(void)
{
	//top left pixel is at (0, 120)
	//original picture is 640 x 4
	//zoom is 4 on both axes so final size is 120 x 1
	e_po3030k_config_cam(0,ARRAY_HEIGHT/4, ARRAY_WIDTH, 4, 4, 4,  RGB_565_MODE);
	//e_po3030k_set_mirror(1,1);
	e_po3030k_write_cam_registers();
}*/

#define CAM_BUFFER_SIZE 2*40*120
char cam_buffer[CAM_BUFFER_SIZE];

void init_cam(void)
{
	e_po3030k_init_cam();
	e_po3030k_config_cam((ARRAY_WIDTH -160)/2, 0,
		160,ARRAY_HEIGHT,4,4,RGB_565_MODE);
	e_po3030k_write_cam_registers();
	return;
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

void takePhoto(void)
{
	e_po3030k_launch_capture(cam_buffer);
	while(!e_po3030k_is_img_ready());
}

int main(void) 
{
	char ch;
	e_init_port();
	e_init_uart1();
	init_cam();
	
	
	//get ch from uart until we receive an x
	while(ch != 'x')
	{
		//is_char checks for incoming data from uart1
		if(e_ischar_uart1())
		{
			e_getchar_uart1(&ch);
		}
	}
	takePhoto();
	cuteFlash();
	
	//send buffer contents to bluetooth
	e_send_uart1_char(buffer,buffsize);
	
	//wait to finish sending
	while(e_uart1_sending()){}
	
	cuteFlash();
	
	while(1){}
	return 0;
}

