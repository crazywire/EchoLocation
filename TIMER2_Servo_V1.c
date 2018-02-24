/*
* uart_test.c
*
* Created: 2017-01-03 10:27:53 AM
* Author : Carl Mattatall
*/
#include <stdio.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/pgmspace.h>

const float fclk_1prescaler = 14.7546e6;
const float fclk_8prescaler = 1.8432e6;
const float fclk_64prescaler = 2.304e5;
const float fclk_128prescaler = 115200;
const float fclk_256prescaler = 57600;
const float fclk_1024prescaler = 14400;

const float servo_period = 2e-2; //20ms;



 

int uart_putchar(char c, FILE *stream);
int uart_getchar(FILE *stream);
FILE mystdout = FDEV_SETUP_STREAM(uart_putchar, NULL, _FDEV_SETUP_WRITE);
FILE mystdin = FDEV_SETUP_STREAM(NULL, uart_getchar, _FDEV_SETUP_READ);

int uart_putchar(char c, FILE *stream)
{
	loop_until_bit_is_set(UCSR0A, UDRE0);
	UDR0 = c;
	return 0;
}
int uart_getchar(FILE *stream)
{
	/* Wait until data exists. */
	loop_until_bit_is_set(UCSR0A, RXC0);
	return UDR0;
}
void init_uart(void)
{
	UCSR0B = (1<<RXEN0) | (1<<TXEN0);
	UBRR0 = 95;//7; // configures a baud rate of 115200
	stdout = &mystdout;
	stdin = &mystdin;
	printf("USART system booted\n");
}

volatile unsigned short num_resets = 0;

volatile unsigned short resets_per_pwm_cycle = 0;

volatile unsigned short reset_val = 0;

ISR(TIMER2_OVF_vect){

	//we are going to use this to prevent overflow

	//when this happens we want to set TCNT2 to OCR2B + 1
	//also have to increment number of resets to keep track of them

	TCNT2 = reset_val;

	num_resets += 1;

	if(num_resets >= resets_per_pwm_cycle){
		num_resets = 0;

		TIMSK2 &= ~(1<<TOIE2);	//disable future overflow interrupts
		//This means that in fast-pwm mode with 0xFF as top, TCNT2 will overflow
		//and we will set OC2B = high (and then OC2B = low at TCNT2 = OCR2B)
	}

}

ISR(TIMER2_COMPA_vect){

	TIMSK2 |= (1<<TOIE2); //enable overflow ISR
	//because we are in non-inverted fast-pwm mode
	//OC2B will be cleared upon this compare event.
	
	//We want to enable the overflow ISR so we can count out 
	//the rest of the pwm cycle (20ms = 2304 counts @ 128 prescale)


}







int main()
{
	
	
	
	init_uart();
	sei();
	DDRD |= (1<<PD7); //OC2A output pwm pin

	
	TIMSK2 |= (1<<OCIE2A); //enable COMP2A ISR

	TCCR2A |= (1<<WGM20)|(1<<WGM21); //Fast PWM Top @ 0xFF
	TCCR2A |= (1<<COM2A1); 
	//non-inverting pwm: OC2A = high at TCNT2 = 0x00, OC2A = low at TCNT2 = OCR2A
	//timer 2 cannot hold enough counts for a full 20ms pwm cycle.
	//Even at prescaler of 1024 we need 288 counts for 20ms.

	TCCR2B |= (1<<CS22)|(1<<CS20); //128 prescaler
	//want to use 128 prescaler so that 2ms = 230.4 counts ~ 230 counts as an int.
	// and 1ms pulse = 115.2 ~ 115 counts as an int
	//since all servo pulses are between 1 and 2 ms, we can fit this
	//inside the timer.


	//***LEFTMOST POSITION***
	OCR2B = (int)(fclk_128prescaler*1e-3); //1ms pulse is leftmost position
	
	reset_val = //need to pick a value of reset_val so that #resets is an integer



	DDRD |= (1<<PD7);   //PWM Pins as Out (OC2A)


	while(1){





	}
}

