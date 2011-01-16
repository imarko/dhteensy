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

uint8_t number_keys[10]=
	{KEY_0,KEY_1,KEY_2,KEY_3,KEY_4,KEY_5,KEY_6,KEY_7,KEY_8,KEY_9};

uint16_t idle_count=0;

int main(void)
{
	uint8_t b, reset_idle, selector;
	uint8_t b_prev[16];

	for (selector=0; selector<14; selector++) {
		b_prev[selector]=0;
	}
	// set for 16 MHz clock
	/* CPU_PRESCALE(0); */

	// Configure all port B and port D pins as inputs with pullup resistors.
	// See the "Using I/O Pins" page for details.
	// http://www.pjrc.com/teensy/pins.html
	DDRB = 0x00;
	PORTB = 0xFF;

	// Initialize the USB, and then wait for the host to set configuration.
	// If the Teensy is powered without a PC connected to the USB port,
	// this will wait forever.
	usb_init();
	while (!usb_configured()) /* wait */ ;

	LED_CONFIG;
	LED_ON;
	// Wait an extra second for the PC's operating system to load drivers
	// and do whatever it does to actually be ready for input
	_delay_ms(100);
	LED_OFF;

	// Configure timer 0 to generate a timer overflow interrupt every
	// 256*1024 clock cycles, or approx 61 Hz when using 16 MHz clock
	// This demonstrates how to use interrupts to implement a simple
	// inactivity timeout.
	TCCR0A = 0x00;
	TCCR0B = 0x05;
	TIMSK0 = (1<<TOIE0);

	DDRB = 0xFF;
	PORTB = 0xF0;
	while (1) {
		for (selector=0; selector<14; selector++) {
			DDRB = 0xFF;
			PORTB = (selector | 0xF0);
			DDRB = 0x00;
			_delay_us(100);
			// read all port B pins
			b = PINB;
			b &= 0xF0;
			// check if any pins are low, but were high previously
			reset_idle = 0;

			if (b_prev[selector] != b) {
				/* usb_keyboard_press(KEY_B, KEY_SHIFT); */
				/* print("selector "); */
				phex((selector << 4) + (b >>4));
				/* print(" bits "); */
				/* phex(b); */
				print("\n");
				reset_idle = 1;
				/* LED_ON; */
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
			// now the current pins will be the previous, and
			// wait a short delay so we're not highly sensitive
			// to mechanical "bounce".
			b_prev[selector] = b;
			/* _delay_ms(2); */
		}
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
		LED_OFF;
	}
}


/* 
   Local Variables:
   c-basic-offset: 8
   End:
*/
