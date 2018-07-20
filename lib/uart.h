#ifndef _UART_H_
#define _UART_H_

#include <stdio.h>

void uart_putchar(char c);
char uart_getchar();

void uart_putstr(char *str);

void _uart_putchar(char c, FILE *stream);
char _uart_getchar(FILE *stream);

void uart_init(void);

/* http://www.ermicro.com/blog/?p=325 */

extern FILE uart_output;
extern FILE uart_input;

#endif
