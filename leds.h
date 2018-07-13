#ifndef _LEDS_H_
#define _LEDS_H_

#include <avr/io.h>

void leds_init() {
	DDRC |= 0x70;
	PORTC |= 0x70;
}

void led_red_on() { PORTC &= ~(1 << PORTC4); }
void led_red_off() { PORTC |= 1 << PORTC4; }
void led_red_toggle() { PORTC ^= 1 << PORTC4; }

void led_yellow_on() { PORTC &= ~(1 << PORTC5); }
void led_yellow_off() { PORTC |= 1 << PORTC5; }
void led_yellow_toggle() { PORTC ^= 1 << PORTC5; }

void led_green_on() { PORTC &= ~(1 << PORTC6); }
void led_green_off() { PORTC |= 1 << PORTC6; }
void led_green_toggle() { PORTC ^= 1 << PORTC6; }

#endif
