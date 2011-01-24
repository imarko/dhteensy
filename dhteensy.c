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

// Teensy 2.0: LED is active high
#if defined(__AVR_ATmega32U4__) || defined(__AVR_AT90USB1286__)
#define LED_ON          (PORTD |= (1<<6))
#define LED_OFF         (PORTD &= ~(1<<6))

// Teensy 1.0: LED is active low
#else
#define LED_ON  (PORTD &= ~(1<<6))
#define LED_OFF (PORTD |= (1<<6))
#endif

#define LED_CONFIG	(DDRD |= (1<<6))
#define CPU_PRESCALE(n)	(CLKPR = 0x80, CLKPR = (n))

uint8_t hex_keys[16]=
	{KEY_0,KEY_1,KEY_2,KEY_3,KEY_4,KEY_5,KEY_6,KEY_7,KEY_8,KEY_9,KEY_A,KEY_B,KEY_C,KEY_D,KEY_E,KEY_F};

uint16_t idle_count=0;

void init_ports() {
	/* led PB6-0 PE7 write only, covered below*/
	/* only using E for writing */
	DDRE=0xFF;
	/* only using B for writing */
	DDRB=0xFF;
	/* D is mixed 0,1 write, 2,3,4,5 read*/
	DDRD=0x03; /* 00000011 */

	/* turn P7 off? */
	/* DDRC=0xFF; */
	/* PORTC=0x00; */
}


void set_selector(uint8_t selector) {

	/* selector:
	   original:
	   P1 lower 4 bits, 0,1,2,3
	   pins 1,2,3,4
	   
	   teensy:
	   PE0, PB7, PD0, PD1
	*/

	/* set port modes */

	/* DDRE |= (1<<0); */
	/* DDRB |= (1<<7); */
	/* DDRD |= (1<<0); */
	/* DDRD |= (1<<1); */

	/* DDRD = 0xFF; */
	/* PORTD |= (1<<2); */
	/* PORTD |= (1<<3); */
	/* PORTD |= (1<<4); */
	/* PORTD |= (1<<5); */

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

	/* set port modes */

	/* DDRD &= ~(1<<2); */
	/* DDRD &= ~(1<<3); */
	/* DDRD &= ~(1<<4); */
	/* DDRD &= ~(1<<5); */

	/* DDRD = 0x00; */

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
	PORTB=led;
}
int main(void)
{
	uint8_t b, reset_idle, selector;
	uint8_t b_prev[16];

	for (selector=0; selector<14; selector++) {
		b_prev[selector]=0;
	}
	// set for 16 MHz clock
	CPU_PRESCALE(0);

	// init ports
	init_ports();

	// Initialize the USB, and then wait for the host to set configuration.
	// If the Teensy is powered without a PC connected to the USB port,
	// this will wait forever.
	/* LED_CONFIG; */
	/* LED_ON; */

	usb_init();
	while (!usb_configured()) /* wait */ ;

	/* LED_OFF; */
	set_led(0x00);

	// Wait an extra second for the PC's operating system to load drivers
	// and do whatever it does to actually be ready for input
	_delay_ms(1000);

	// Configure timer 0 to generate a timer overflow interrupt every
	// 256*1024 clock cycles, or approx 61 Hz when using 16 MHz clock
	// This demonstrates how to use interrupts to implement a simple
	// inactivity timeout.
	TCCR0A = 0x00;
	TCCR0B = 0x05;
	TIMSK0 = (1<<TOIE0);

	print("starting\n");

	while (1) {
		for (selector=0; selector<14; selector++) {
			b=scan_line(selector);

			if (b_prev[selector] != b) {
				/* phex((selector << 4) + b); */
				/* print("\n"); */
				usb_keyboard_press(hex_keys[selector],0);
				usb_keyboard_press(hex_keys[b],0);
				usb_keyboard_press(KEY_SPACE,0);
				reset_idle = 1;
			}

			// if any keypresses were detected, reset the idle counter
			if (reset_idle) {
				// variables shared with interrupt routines must be
				// accessed carefully so the interrupt routine doesn't
				// try to use the variable in the middle of our access
				cli();
				idle_count = 0;
				sei();
			}

			b_prev[selector] = b;
		}
		_delay_ms(10);
		
	}
}

// This interrupt routine is run approx 61 times per second.
// A very simple inactivity timeout is implemented, where we
// will send a space character and print a message to the
// hid_listen debug message window.


ISR(TIMER0_OVF_vect)
{
	idle_count++;
	if (idle_count > 61 * 8) {
		idle_count = 0;
		print("Timer Event :)\n");
		/* LED_OFF; */
	}
}


/* 
   Local Variables:
   c-basic-offset: 8
   End:
*/
