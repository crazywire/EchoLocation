/*
 * EchoLocation.c
 *
 * Created: 2018-02-12 10:12:49 PM
 * Author : Abdul
 */ 

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>

volatile int16_t t1=0, t2 =0;
volatile int8_t triggered =0;

float F_CPU = 14.7546e6, delay = 10e-6, distance;


int uart_putchar(float distance)
{
	loop_until_bit_is_set(UCSR0A, UDRE0);
	UDR0 = distance;
	return 0;
}

void generatePulse(){
	PORTD |= (1<<PD7);
	TCCR1A =0;
	TCCR1B = (1<<CS10); //start timer
	TCNT0;
	OCR1A = (int)(F_CPU*delay-1);
	while(!(TIFR1 & (1<<OCF1A)));
	TIFR1 |= (1<<OCF1A);
	TCCR1B =0;
	PORTD &= ~(1<<PD7);	
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
	
	UCSR0B = (1<<RXEN0) | (1<<TXEN0);
	UBRR0 = 95;//7; // configures a baud rate of 115200
    while (1)
    {
		triggered = PINB & (1<<PB1);
		
		if (triggered)
		{
			generatePulse();
			detectEcho();
		}
		
		distance = (double)((t2-t1)*4)/58;
		uart_putchar(distance);
    }
}

