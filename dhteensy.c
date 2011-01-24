/* Keyboard example with debug channel, for Teensy USB Development Board
 * http://www.pjrc.com/teensy/usb_keyboard.html
 * Copyright (c) 2008 PJRC.COM, LLC
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "usb_keyboard_debug.h"
#include "print.h"

#define CPU_PRESCALE(n)	(CLKPR = 0x80, CLKPR = (n))

uint8_t hex_keys[16]=
	{KEY_0,KEY_1,KEY_2,KEY_3,KEY_4,KEY_5,KEY_6,KEY_7,KEY_8,KEY_9,KEY_A,KEY_B,KEY_C,KEY_D,KEY_E,KEY_F};

uint16_t idle_count=0;

void init_ports() {
	/* led PB6-0 PE7 write only, covered below*/
	/* only using E for writing */
	/* DDRE=0xFF; */
	DDRE=0x81; /* 10000001 */
	/* only using B for writing */
	DDRB=0xFF;
	/* D is mixed 0,1 write, 2,3,4,5 read*/
	DDRD=0x03; /* 00000011 */
}


void set_selector(uint8_t selector) {

	/* selector:
	   original:
	   P1 lower 4 bits, 0,1,2,3
	   pins 1,2,3,4
	   
	   teensy:
	   PE0, PB7, PD0, PD1
	*/

	/* bit 0 */
	if (selector & (1<<0)) {
		PORTE |= (1<<0);
	} else {
		PORTE &= ~(1<<0);
	}

	/* bit 1 */
	if (selector & (1<<1)) {
		PORTB |= (1<<7);
	} else {
		PORTB &= ~(1<<7);
	}

	/* bit 2 */
	if (selector & (1<<2)) {
		PORTD |= (1<<0);
	} else {
		PORTD &= ~(1<<0);
	}

	/* bit 3 */
	if (selector & (1<<3)) {
		PORTD |= (1<<1);
	} else {
		PORTD &= ~(1<<1);
	}
}

int read_keys() {
	/* key:
	   original:
	   P1 upper 4 bits, 4,5,6,7
	   pins 5,6,7,8
	   teensy:
	   PD2,PD3,PD4,PD5
	*/

	uint8_t b;

	// read all port B pins
	b = PIND;
	b &= 0x3C; /* 00111100 */
	b >>= 2;
	return(b);

}

int scan_line(uint8_t selector) {
	set_selector(selector);
	_delay_us(100);
	return(read_keys());
}

void set_led(uint8_t led) {

	/*
	  nas: p0 0e
	  fun p0 0b
	  norm p0 0d
	  game mode p0 07

		DB 00h	;00h not used
		DB 0Dh	;01h normal mode
		DB 0Eh	;02h NAS mode
		DB 0Bh	;03h Function mode

	 */
	PORTB=(led & 0x7f);
	/* bit 7 */
	if (led & (1<<7)) {
		PORTE |= (1<<7);
	} else {
		PORTE &= ~(1<<7);
	}
}
int main(void)
{
	uint8_t b, reset_idle, selector;
	uint8_t b_prev[16];

	// set for 16 MHz clock
	CPU_PRESCALE(0);

	for (selector=0; selector<14; selector++) {
		b_prev[selector]=0;
	}

	// init ports
	init_ports();

	// Initialize the USB, and then wait for the host to set configuration.
	// If the Teensy is powered without a PC connected to the USB port,
	// this will wait forever.

	usb_init();
	while (!usb_configured()) /* wait */ ;

	set_led(0x01);

	// Wait an extra second for the PC's operating system to load drivers
	// and do whatever it does to actually be ready for input
	_delay_ms(1000);

	while (1) {
		for (selector=0; selector<14; selector++) {
			b=scan_line(selector);

			if (b_prev[selector] != b) {
				/* phex((selector << 4) + b); */
				/* print("\n"); */
				usb_keyboard_press(hex_keys[selector],0);
				usb_keyboard_press(hex_keys[b],0);
				usb_keyboard_press(KEY_SPACE,0);
				/* set_led((selector<<4)+b); */
			}

			b_prev[selector] = b;
		}
		_delay_ms(10);
		
	}
}

/* 
   Local Variables:
   c-basic-offset: 8
   End:
*/
