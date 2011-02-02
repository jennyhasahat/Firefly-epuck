#include "./camera/fast_2_timer/e_po3030k.h"
#include "./uart/e_uart_char.h"
#include "./I2C/e_I2C_protocol.h"
#include "./motor_led/e_init_port.h"
#include "./motor_led/e_led.h"

int buffsize = 2*40*40;
char buffer[2*40*40];

void cuteFlash(void)
{
	int i, j;
	unsigned long delay = 200000;
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
	e_po3030k_launch_capture(buffer);
	while(!e_po3030k_is_img_ready());
	cuteFlash();
}

int main(void) 
{
	char ch;
	e_init_port();
	e_init_uart1();
	
	e_po3030k_init_cam();
	e_po3030k_config_cam((ARRAY_WIDTH -160)/2,(ARRAY_HEIGHT-160)/2,
		160,160,4,4,RGB_565_MODE);
	e_po3030k_write_cam_registers();
	
	
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
	
	//send buffer contents to bluetooth
	e_send_uart1_char(buffer,buffsize);
	
	//wait to finish sending
	while(e_uart1_sending()){}
	
	return 0;
}

