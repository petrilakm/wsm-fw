/* Main source file of Measure car project.
 */

#include <stdbool.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/io.h>

#include "leds.h"
#include "lib/uart.h"

///////////////////////////////////////////////////////////////////////////////

int main();
void init();
void init_opto_interrupt();
void init_opto_icp();

///////////////////////////////////////////////////////////////////////////////

int main() {
	init();

	while (true) {
	}
}

void init() {
	leds_init();
	uart_init();

	led_red_on();
	led_yellow_on();
	led_green_on();

	_delay_ms(250);

	led_red_off();
	led_yellow_off();
	led_green_off();

	init_opto_icp();

	sei(); // enable interrupts globally
}

void init_opto_interrupt() {
	// PD0 is input by default
	// PD0 pull-up is disabled by default (pullup is hardware-based)
	EICRA |= 0x03; // configure INT0 interrupt on rising edge
	EIMSK |= 1 << INT0; // enable INT0
}

void init_opto_icp() {
	// PD0 is input by default
	// PD0 pull-up is disabled by default (pullup is hardware-based)
	// This function enabled ICP measurement and setups Timer1

	TIMSK1 |= 1 << ICIE1; // enable ICP capture
	TCCR1B |= 1 << ICES1; // capture rising edge
	TCCR1B |= 1 << ICNC1; // enable noise canceler on ICP
	TCCR1B |= 0x02; // prescaler 8×
}

ISR(INT0_vect) {
	led_yellow_toggle();
}

ISR(TIMER1_CAPT_vect) {
	uint16_t measured = ICR1L; // must read low byte first
	measured |= ICR1H << 8;
}

void send_speed(uint16_t speed) {
	char data[6];

	data[0] = 0x03;
	data[1] = 0x01;
	data[2] = speed >> 8;
	data[3] = speed & 0xFF;
	data[4] = 0x03 ^ 0x01 ^ data[2] ^ data[3];
	data[5] = 0;

	uart_putstr(data);
}
