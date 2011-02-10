/* Copyright 2008 Renato Florentino Garcia <fgar.renato@gmail.com>
 *
 * Some modifications by Jenny Owen. jowen@cs.york.ac.uk 
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3, as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <stdlib.h>

#include <uart/e_uart_char.h>
#include <motor_led/e_epuck_ports.h>
#include <motor_led/e_init_port.h>
#include <motor_led/advance_one_timer/e_led.h>
#include <motor_led/advance_one_timer/e_motors.h>
#include <motor_led/advance_one_timer/e_agenda.h>
#include <a_d/e_prox.h>
#include <camera/fast_2_timer/e_po3030k.h>
#include <I2C/e_I2C_protocol.h>

#include "epuck_utilities.h"

char recv_char()
{
  char message;

  while(!e_ischar_uart1()){} /* Wait until arrive a message */
  while(e_getchar_uart1(&message)==0);

  return message;
}

int recv_int()
{
  char message[2];

  while(!e_ischar_uart1()){} /* Wait until arrive a message */
  while(e_getchar_uart1(message)==0);

  while(!e_ischar_uart1()){} /* Wait until arrive a message */
  while(e_getchar_uart1(message + 1)==0);

  int m0 = message[0];
  int m1 = message[1];

  return (m1<<8) | (m0 & 0xFF);
}

void send_int(int integer)
{
  char message[2];
  message[0] = integer & 0x00FF;
  message[1] = (integer & 0xFF00) >> 8;

  e_send_uart1_char(message, 1);
  while( e_uart1_sending() ){}
  e_send_uart1_char(message+1, 1);
  while( e_uart1_sending() ){}
}

void send_char(char character)
{
  e_send_uart1_char(&character, 1);
  while(e_uart1_sending());
}



/** Stop the motor activities
 *
 * Set the speed for zero and clean the step counter
 */
void stop_motors()
{
  e_set_speed_right(0);
  e_set_speed_left(0);

  e_set_steps_right(0);
  e_set_steps_left(0);

 // send_char(1);/* It signalize the end of operation. */
}



/**Gets the position of the selector thing on top of the robot.
@return value position of the selector from 0 to 15.
*/
int getselector()
{
  return SELECTOR0 + 2*SELECTOR1 + 4*SELECTOR2 + 8*SELECTOR3;
}

/**A distinctive flash.
*/
void cute_flash(void)
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





