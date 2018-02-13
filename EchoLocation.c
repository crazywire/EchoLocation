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

float F_CPU = 14.7546/8, delay = 10, distance=0.0;
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
	printf("generate pulse start\n");
	PORTB |= (1<<PB1);
	TCCR1A =0;
	TCCR1B = (1<<CS11); //start timer
	TCNT1=0;
	OCR1A = (int)((F_CPU/1600e2)-1);
	TIFR1 = (1<<OCF1A);
	while(!(TIFR1 & (1<<OCF1A)));
	//TIFR1 = (1<<OCF1A);
	TCCR1B =0;
	TCNT1=0;
	PORTB &= ~(1<<PB1);
	printf("generate pulse end\n");
}

/*void detectEcho(){
	
	printf("detect echo start\n");
	TCCR1A =0;
	TCCR1B = (1<<CS11) | (1<<ICES1); //start timer and capture the rising edge
	TCNT1=0;
	TIFR1 |= (1<<ICF1);
	while (!(TIFR1 & (1<<ICF1)));
	t1 = ICR1L;
	t1 |= (ICR1H<<8);
	 //wait for the rising edge
	 //clear flag
	
	printf("detect echo middle\n");
	TCCR1A =0;
	TCCR1B =  (1<<CS11); //start timer and capture the rising edge
	TIFR1 |= (1<<ICF1);
	while (!(TIFR1 & (1<<ICF1))); //wait for the rising edge
	printf("detect echo end\n");
	t2 = ICR1L;
	t2 |= (ICR1H<<8); //clear flag
	TCNT1 =0;
	TCCR1B =0;
}
*/
int main(void)
{
	init_uart();
	DDRD = 0x00;
	DDRB = 0xFF;

	printf("starting measurements...\n");

	while (1)
	{
		triggered = PIND & (1<<PD7);

		if (triggered)
		{
			printf("triggered\n");
			generatePulse();
			//detectEcho();
		}
		
		/*distance = (double)((t2-t1)*4)/58;
		if (distance) {
			printf("distance = %f\n", distance);
			distance = 0;
		}
*/
	}
}
