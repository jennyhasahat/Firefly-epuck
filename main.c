 #include <motor_led/e_init_port.h>
 #include <uart/e_uart_char.h>
 
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


