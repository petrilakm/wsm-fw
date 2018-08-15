#ifndef _LEDS_H_
#define _LEDS_H_

#include <avr/io.h>

static inline void leds_init() {
	DDRC |= 0x0E;
	PORTC |= 0x0E;
}

static inline void led_red_on() { PORTC &= ~(1 << PORTC3); }
static inline void led_red_off() { PORTC |= 1 << PORTC3; }
static inline void led_red_toggle() { PORTC ^= 1 << PORTC3; }

static inline void led_yellow_on() { PORTC &= ~(1 << PORTC2); }
static inline void led_yellow_off() { PORTC |= 1 << PORTC2; }
static inline void led_yellow_toggle() { PORTC ^= 1 << PORTC2; }

static inline void led_green_on() { PORTC &= ~(1 << PORTC1); }
static inline void led_green_off() { PORTC |= 1 << PORTC1; }
static inline void led_green_toggle() { PORTC ^= 1 << PORTC1; }

#endif
