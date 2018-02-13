/*
* EchoLocation.c
*
* Created: 2018-02-12 10:12:49 PM
* Author : Abdul
*/

#include <stdio.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>

volatile int16_t t1=0, t2 =0;
volatile int16_t triggered =0;

float F_CPU = 14.7546e6, delay = 10e-6, distance;
int uart_putchar(char c, FILE *stream);
int uart_getchar(FILE *stream);

FILE mystdout = FDEV_SETUP_STREAM(uart_putchar, NULL, _FDEV_SETUP_WRITE);
FILE mystdin = FDEV_SETUP_STREAM(NULL, uart_getchar, _FDEV_SETUP_READ);



// send a char to PuTTY
int uart_putchar(char c, FILE *stream) {
	loop_until_bit_is_set(UCSR0A, UDRE0); // wait until UDRE0 is set (free)
	UDR0 = c;
	return 0;
}
// get a char from PuTTY
int uart_getchar(FILE *stream) {
	loop_until_bit_is_set(UCSR0A, RXC0); // wait until data exists
	return UDR0; // returns the char read from PuTTY
}
void init_uart(void) {
	UCSR0B = (1 << RXEN0) | (1 << TXEN0); // enables data transfer
	UBRR0 = 95; // configures a baud rate of 9600
	stdout = &mystdout; // set the standard out (printf) to PuTTY
}


void generatePulse(){
PORTB |= (1<<PB1);
TCCR1A =0;
TCCR1B = (1<<CS10); //start timer
TCNT0;
OCR1A = (int)(F_CPU*delay-1);
while(!(TIFR1 & (1<<OCF1A)));
TIFR1 = (1<<OCF1A);
TCCR1B =0;
PORTB &= ~(1<<PB1);
}

void detectEcho(){
TCCR1A =0;
TCCR1B = (1<<CS10) | (1<<CS11) | (1<<ICNC1) | (1<<ICES1); //start timer and capture the rising edge
TCNT0;
while (!(TIFR1 & (1<<ICF1))); //wait for the rising edge
t1 = ICR1L;
t1 |= (ICR1H<<8);
TIFR1 |= (1<<ICF1); //clear flag

TCCR1B &= ~(1<<ICES1); //capture the falling edge
while (!(TIFR1 & (1<<ICF1))); //wait for the falling edge
t1 = ICR1L;
t1 |= (ICR1H<<8);
TIFR1 |= (1<<ICF1); //clear flag
TCCR1B =0; //turn off the timer
}

int main(void)
{
	init_uart();
	DDRD = 0x00;
	DDRB = 0xFF;


	while (1)
	{
	triggered = PIND & (1<<PD7);

	if (triggered)
	{
	generatePulse();
	detectEcho();
	}
	
	if (distance) {
	
		distance = (double)((t2-t1)*4)/58;
		printf("distance = %f\n", distance);
		distance = 0;
	}

	}
}
