/* Main source file of Measure car project.
 */

#include <stdbool.h>
#include <util/delay.h>
#include <avr/interrupt.h>

#include "leds.h"
#include "lib/uart.h"

///////////////////////////////////////////////////////////////////////////////

int main();
void init();
void init_opto_measure();

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

	init_opto_measure();

	sei(); // enable interrupts globally
}

void init_opto_measure() {
	// PD0 is input by default
	// PD0 pull-up is disabled by default (pullup is hardware-based)
	EICRA |= 0x03; // configure INT0 interrupt on rising edge
	EIMSK |= 1 << INT0; // enable INT0
}

ISR(INT0_vect) {
	led_yellow_toggle();
}
