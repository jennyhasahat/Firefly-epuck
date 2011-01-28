#include "e_po3030k.h"
#include "e_uart_char.h"
#include "e_I2C_protocol.h"
#include "e_init_port.h"

int buffsize = 2*40*40;
char buffer[2*40*40];

void takePhoto(void)
{
	e_send_uart1_char("taking photo2", 14);
	e_po3030k_launch_capture(buffer);
	while(!e_po3030k_is_img_ready());
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
		if(e_ischar_uart1())
		{
			e_getchar_uart1(&ch);
			e_send_uart1_char(&ch, 1);
		}
	}
	takePhoto();
	
	//send buffer contents to bluetooth
	e_send_uart1_char(buffer,buffsize);
	
	//wait to finish sending
	while(e_uart1_sending()){}
	
	return 0;
}
