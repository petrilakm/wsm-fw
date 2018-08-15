/* Main source file of Wireless Speedometer project.
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
void send_speed(uint16_t speed);
void send_battery_voltage(uint16_t voltage, bool critical);

void opto_init_interrupt();
void opto_init_icp();
void opto_hist_reset();
uint16_t opto_get_interval();

void bat_start_measure();
void bat_init_measure();

void shutdown_all();

///////////////////////////////////////////////////////////////////////////////

#define OPTO_HIST_LEN 8u
#define OPTO_TIMEOUT 50 // 500 ms
volatile uint16_t opto_hist[OPTO_HIST_LEN];
volatile int8_t opto_hist_next_index = 0;
volatile uint16_t opto_last_measure_time;
volatile bool opto_last_measure_time_ok = false;
volatile uint8_t opto_timeout_counter = 0;

///////////////////////////////////////////////////////////////////////////////

const uint16_t BAT_THRESHOLD = 814; // 3.5 V = 814, 4.2 V = 977

///////////////////////////////////////////////////////////////////////////////

int main() {
	init();
	uint8_t bat_timer = 0;
	#define BAT_TIMEOUT 50 // 50 ms

	while (true) {
		// send current speed to PC each 100 ms
		_delay_ms(100);
		send_speed(opto_get_interval());

		bat_timer++;
		if (bat_timer >= BAT_TIMEOUT) {
			led_green_toggle();
			bat_start_measure();
			bat_timer = 0;
		}
	}
}

void init() {
	leds_init();
	bat_init_measure();
	opto_hist_reset();
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

	// configure shutdown pin as output
	PORTB |= 1 << PORTB2; // pin high
	DDRB |= 1 << PORTB2; // pin output

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
	TCCR1B |= 0x03; // prescaler 64×
}

ISR(INT0_vect) {
	led_yellow_toggle();
}

ISR(TIMER1_CAPT_vect) {
	uint16_t time = ICR1L; // must read low byte first
	time |= ICR1H << 8;

	if (opto_last_measure_time_ok) {
		opto_hist[opto_hist_next_index] = time - opto_last_measure_time;
		opto_hist_next_index = (opto_hist_next_index + 1) % OPTO_HIST_LEN;
	} else {
		opto_last_measure_time_ok = true;
	}
	opto_last_measure_time = time;
	opto_timeout_counter = 0;

	led_yellow_toggle();
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
	char data[7];

	data[0] = 0x94;
	data[1] = 0x81;
	data[2] = (speed >> 14) | 0x80;
	data[3] = ((speed >> 7) & 0x7F) | 0x80;
	data[4] = (speed & 0x7F) | 0x80;
	data[5] = 0x80 | (0x14 ^ 0x01 ^ (data[2] & 0x7F) ^ (data[3] & 0x7F) ^
	                  (data[4] & 0x7F));
	data[6] = 0;

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

///////////////////////////////////////////////////////////////////////////////

void bat_init_measure() {
	// Initialize battery measurement.
	// This function sets ADC properties. No change should be done to these
	// registers in any other function in future!
	ADMUX |= (1 << REFS0) | (1 << REFS1); // Use internal 1V1 reference
	ADMUX |= 0x0; // use ADC0
	ADCSRA |= 1 << ADIE; // enable ADC interrupt
	ADCSRA |= 0x5; // prescaler 16× (115 kHz is in 50-200 kHz)
}

void bat_start_measure() {
	// Measures voltage on battery.
	// Beware: no pins PC* could be switched during measurement! (LEDs)
	ADCSRA |= 1 << ADEN; // enable ADC!
	ADCSRA |= 1 << ADSC; // start conversion
}

ISR(ADC_vect) {
	uint16_t value = ADCL;
	value |= (ADCH << 8);

	send_battery_voltage(value, value < BAT_THRESHOLD);
	led_green_toggle();

	if (value < BAT_THRESHOLD) {
		send_battery_voltage(value, true);
		send_battery_voltage(value, true);
		shutdown_all();
	}
}

void send_battery_voltage(uint16_t voltage, bool critical) {
	char data[5];

	data[0] = 0xA2;
	data[1] = (voltage >> 7) | (critical << 6) | 0x80;
	data[2] = (voltage & 0x7F) | 0x80;
	data[3] = 0x80 | (0x22 ^ (data[1] & 0x7F) ^ (data[2] & 0x7F));
	data[4] = 0;

	uart_putstr(data);
}

///////////////////////////////////////////////////////////////////////////////

void shutdown_all() {
	PORTB &= ~(1 << PORTB2);
}

///////////////////////////////////////////////////////////////////////////////
