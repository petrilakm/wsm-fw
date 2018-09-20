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
void send_distance(uint32_t distance);

void opto_init_icp();
void opto_hist_reset();
uint16_t opto_get_interval();

void bat_start_measure();
void bat_init_measure();

void shutdown_all();

///////////////////////////////////////////////////////////////////////////////

typedef struct {
	uint32_t ticks_sum;
	uint16_t ticks_count;
} TicksHistoryItem;

#define OPTO_HIST_LEN 10u
#define OPTO_TIMEOUT 50 // 500 ms
#define OPTO_MIN_TICKS 250u // minimal 250 ticks (~ 300 kmph in TT)
volatile TicksHistoryItem opto_hist[OPTO_HIST_LEN];
volatile int8_t opto_hist_index = 0;
volatile uint16_t opto_last_measure_time;
volatile bool opto_last_measure_time_ok = false;
volatile uint8_t opto_timeout_counter = 0;
volatile uint32_t opto_counter = 0;
volatile bool should_shutdown = false;
volatile bool bat_send_voltage = false;
volatile uint16_t bat_voltage;
volatile bool bat_first_measure = true;

///////////////////////////////////////////////////////////////////////////////

const uint16_t BAT_CRITICAL = 783; // 3.5 V
const uint16_t BAT_LOW = 794; // 3.55 V

///////////////////////////////////////////////////////////////////////////////

int main() {
	init();
	bat_start_measure();
	uint8_t bat_timer = 0;
	uint8_t dist_timer = 0;
	#define BAT_TIMEOUT 50 // 5 s
	#define DISTANCE_TIMEOUT 5 // 500 ms

	while (true) {
		_delay_ms(100);

		// send current speed to PC each 100 ms
		send_speed(opto_get_interval());

		bat_timer++;
		if (bat_timer >= BAT_TIMEOUT) {
			// send battery voltage & speed counter mode to PC each 5 s
			led_green_on();
			bat_start_measure();
			bat_timer = 0;
		}

		dist_timer++;
		if (dist_timer == DISTANCE_TIMEOUT) {
			// send distance to PC
			send_distance(opto_counter);
			dist_timer = 0;
		}

		if (bat_send_voltage) {
			bat_send_voltage = false;
			send_battery_voltage(bat_voltage, bat_voltage < BAT_CRITICAL);
		}

		if (should_shutdown) {
			send_battery_voltage(bat_voltage, true);
			send_battery_voltage(bat_voltage, true);
			should_shutdown = false;

			for (size_t i = 0; i < 3; i++) {
				led_red_off();
				_delay_ms(200);
				led_red_on();
				_delay_ms(100);
			}
			shutdown_all();
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
	TCCR0A |= 1 << WGM01; // CTC mode
	TCCR0B |= 0x04; // prescaler 256×
	TIMSK0 |= 1 << OCIE0A; // enable interrupt on compare match A
	OCR0A = 142; // set compare match A to match 10 ms

	// configure shutdown pin as output
	PORTB |= 1 << PORTB2; // pin high
	DDRB |= 1 << PORTB2; // pin output

	sei(); // enable interrupts globally
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

ISR(TIMER1_CAPT_vect) {
	// Optosensor rising edge interrupt
	uint16_t time = ICR1L; // must read low byte first
	time |= ICR1H << 8;

	if (opto_last_measure_time_ok) {
		uint16_t delta = time - opto_last_measure_time;
		if (delta < OPTO_MIN_TICKS) {
			// protection against fast ticks
			opto_last_measure_time_ok = false;
		} else {
			opto_hist[opto_hist_index].ticks_count++;
			opto_hist[opto_hist_index].ticks_sum += delta;
		}
	} else {
		opto_last_measure_time_ok = true;
	}
	opto_last_measure_time = time;
	opto_timeout_counter = 0;

	led_yellow_toggle();
	opto_counter++;
}

ISR(TIMER0_COMPA_vect) {
	// Timer0 on 10 ms
	#define SPEED_TIMEOUT 10u // 100 ms
	volatile static uint8_t speed_timer = 0;

	opto_timeout_counter++;
	if (opto_timeout_counter >= OPTO_TIMEOUT) {
		opto_timeout_counter = 0;

		TIMSK1 &= ~(1 << ICIE1); // temporary disable ICP capture
		opto_hist_reset();
		TIMSK1 |= 1 << ICIE1; // enable ICP capture
	}

	speed_timer++;
	if (speed_timer >= SPEED_TIMEOUT) {
		// go to new frame of opto_hist each 100 ms
		speed_timer = 0;

		uint8_t new_opto_hist_index = (opto_hist_index + 1) % OPTO_HIST_LEN;
		opto_hist[new_opto_hist_index].ticks_count = 0;
		opto_hist[new_opto_hist_index].ticks_sum = 0;
		opto_hist_index = new_opto_hist_index;
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
	for (size_t i = 0; i < OPTO_HIST_LEN; i++) {
		opto_hist[i].ticks_count = 0;
		opto_hist[i].ticks_sum = 0;
	}
	opto_last_measure_time_ok = false;
}

uint16_t opto_get_interval() {
	uint32_t sum = 0;
	uint16_t count = 0;
	for (size_t i = 0; i < OPTO_HIST_LEN; i++) {
		sum += opto_hist[i].ticks_sum;
		count += opto_hist[i].ticks_count;
	}

	if (count == 0)
		return 0xFFFF;

	return (uint16_t)(sum / count);
}

void send_distance(uint32_t distance) {
	char data[9];

	data[0] = 0x96;
	data[1] = 0x82;
	data[2] = (distance >> 28) | 0x80;
	data[3] = (distance >> 21) | 0x80;
	data[4] = (distance >> 14) | 0x80;
	data[5] = (distance >> 7) | 0x80;
	data[6] = distance | 0x80;

	data[7] = 0x80 | (0x16 ^ 0x02 ^ (data[2] & 0x7F) ^ (data[3] & 0x7F) ^
	                 (data[4] & 0x7F) ^ (data[5] & 0x7F) ^ (data[6] & 0x7F));
	data[8] = 0;

	uart_putstr(data);
}

///////////////////////////////////////////////////////////////////////////////

void bat_init_measure() {
	// Initialize battery measurement.
	// This function sets ADC properties. No change should be done to these
	// registers in any other function in future!
	ADMUX |= (1 << REFS0); // AVCC with external capacitor at AREF pin
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
	bat_voltage = value;

	if (value < BAT_LOW && (value > BAT_CRITICAL || bat_first_measure))
		led_red_on();
	else
		led_red_off();

	bat_send_voltage = true;
	led_green_off();

	if (value < BAT_CRITICAL && !bat_first_measure)
		should_shutdown = true;

	bat_first_measure = false;
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
