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
/*
#include <uart/e_uart_char.h>
#include <motor_led/e_epuck_ports.h>
#include <motor_led/e_init_port.h>
#include <motor_led/advance_one_timer/e_led.h>
#include <motor_led/advance_one_timer/e_motors.h>
#include <motor_led/advance_one_timer/e_agenda.h>
#include <a_d/e_prox.h>
#include <camera/fast_2_timer/e_po3030k.h>
#include <I2C/e_I2C_protocol.h>
*/
#ifndef _EPUCK_UTILITIES_H
#define _EPUCK_UTILITIES_H

//unsigned char *G_camera_buffer __attribute__ ((far));
//unsigned int G_image_bytes_size = 0;


//UART functions
char recv_char();
int recv_int();
void send_int(int integer);
void send_char(char character);

//motor functions
void stop_motors();



//other functions
int getselector();
void cute_flash(void);







#endif

