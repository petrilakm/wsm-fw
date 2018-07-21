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
void opto_init_interrupt();
void opto_init_icp();
void opto_hist_reset();
uint16_t opto_get_interval();
void send_speed(uint16_t speed);

///////////////////////////////////////////////////////////////////////////////

#define OPTO_HIST_LEN 8u
#define OPTO_TIMEOUT 50 // 500 ms
volatile uint16_t opto_hist[OPTO_HIST_LEN] = {0xFFFF, };
volatile int8_t opto_hist_next_index = 0;
volatile uint16_t opto_last_measure_time;
volatile bool opto_last_measure_time_ok = false;
volatile uint8_t opto_timeout_counter = 0;

///////////////////////////////////////////////////////////////////////////////

int main() {
	init();

	while (true) {
		// send current speed to PC each 100 ms
		_delay_ms(100);
		send_speed(opto_get_interval());
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

	opto_init_icp();

	// Setup main timer0 on 10 ms
	TCCR0B |= 0x04; // prescaler 256×
	TIMSK0 |= 1 << OCIE0A; // enable interrupt on compare match A
	OCR0A = 71; // set compare match A to match 10 ms

	sei(); // enable interrupts globally
}

void opto_init_interrupt() {
	// PD0 is input by default
	// PD0 pull-up is disabled by default (pullup is hardware-based)
	EICRA |= 0x03; // configure INT0 interrupt on rising edge
	EIMSK |= 1 << INT0; // enable INT0
}

void opto_init_icp() {
	// PB0 is input by default
	// PB0 pull-up is disabled by default (pullup is hardware-based)
	// This function enables ICP measurement and setups Timer1

	TIMSK1 |= 1 << ICIE1; // enable ICP capture
	TCCR1B |= 1 << ICES1; // capture rising edge
	TCCR1B |= 1 << ICNC1; // enable noise canceler on ICP
	TCCR1B |= 0x02; // prescaler 8×
}

ISR(INT0_vect) {
	led_yellow_toggle();
}

ISR(TIMER1_CAPT_vect) {
	uint16_t time = ICR1L; // must read low byte first
	time |= ICR1H << 8;

	if (opto_last_measure_time_ok) {
		opto_hist[opto_hist_next_index] = time - opto_last_measure_time; // TODO: check for behavior on overflow
		opto_hist_next_index = (opto_hist_next_index + 1) % OPTO_HIST_LEN;
	} else {
		opto_last_measure_time_ok = true;
	}
	opto_last_measure_time = time;
	opto_timeout_counter = 0;
}

ISR(TIMER0_COMPA_vect) {
	// Timer0 on 10 ms

	opto_timeout_counter++;
	if (opto_timeout_counter >= OPTO_TIMEOUT) {
		opto_timeout_counter = 0;

		if (opto_last_measure_time_ok) {
			TIMSK1 &= ~(1 << ICIE1); // temporary disable ICP capture
			opto_hist_reset();
			TIMSK1 |= 1 << ICIE1; // enable ICP capture
		}
	}
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

void opto_hist_reset() {
	for (size_t i = 0; i < OPTO_HIST_LEN; i++)
		opto_hist[i] = 0xFFFF;
	opto_hist_next_index = 0;
	opto_last_measure_time_ok = false;
}

uint16_t opto_get_interval() {
	uint32_t sum = 0;
	for (size_t i = 0; i < OPTO_HIST_LEN; i++)
		sum += opto_hist[i];
	return (uint16_t)(sum / OPTO_HIST_LEN);
}
