/*
* uart_test.c
*
* Created: 2017-01-03 10:27:53 AM
* Author : JF
*/
#include <stdio.h>
#include <avr/io.h>
#include <avr/pgmspace.h>

const float fclck = 14.7546e6/64;
const float fservo = 50.0;
const float delta_t = 1.0e-3;



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

//Simple Wait Function
void delay(int delay)
{
	int i=0;
	for(i=0;i<delay;i++){
		TCCR0A=0;
		TCCR0B = (1<<CS00) | (1<<CS02);
		TCNT0=0;
		OCR0A = (int)(fclck*delta_t-1);
		TIFR1 = (1<<OCF1A);
		while(!(TIFR1 &(1<<OCF1A)));
	}
	TCCR0B =0;
}

int main()
{

	 
	 int zero_deg_pulse = (int)(fclck*0.49e-3);
	 int fifteen_deg_pulse = (int)(fclck*0.1391e-3);
	 int ninety_deg_pulse = (int)(fclck*1.325e-3);
	
	init_uart();
	//Configure TIMER1
	TCCR1A|=(1<<COM1A1)|(1<<COM1B1)|(1<<WGM11);        //NON Inverted PWM
	TCCR1B|=(1<<WGM13)|(1<<WGM12)|(1<<CS11)|(1<<CS10); //PRESCALER=64 MODE 14(FAST PWM)
	ICR1= (int)(fclck/fservo);  //fPWM=50Hz (Period = 20ms Standard).
	DDRD=(1<<PD4)|(1<<PD5);   //PWM Pins as Out
	//if you want use PIN D4, use 0CR1B in the while loop. This way you can run two servos.
	OCR1A = zero_deg_pulse;
	while(1)
	{
		for(int x = zero_deg_pulse; x <= ninety_deg_pulse; x += fifteen_deg_pulse){
			
			OCR1A = x;
			OCR1B = OCR1A;
			delay(50);
			
		}
		OCR1A = zero_deg_pulse;
		OCR1B = OCR1A;
		
		

	}
}


// 	 float zeroDegrees = 0.49e-3;
// 	 float fifteenDegrees = zeroDegrees+0.1391e-3;
// 	 float thirtyDegrees = fifteenDegrees+0.1391e-3;
// 	 float fortyFiveDegrees = thirtyDegrees+0.1391e-3;
// 	 float sixtyDegrees = fortyFiveDegrees+0.1391e-3;
// 	 float seventyFiveDegrees = sixtyDegrees+0.1391e-3;
// 	 float nintyDegrees = 1.325e-3;
// 	 float OneEightydegrees = 2.16e-3;
