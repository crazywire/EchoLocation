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


const unsigned int period_counts_128ps_clk = 2304; 
//20ms * fclk_128prescale


 

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



ISR(TIMER2_OVF_vect){
	
	printf("OVERFLOW ISR WORKS\n #resets = %d\n", num_resets);
	
	//we are going to use this to prevent overflow of TCNT2 to 0x00 and
	//keep OC2A low until the end of the pwm period.

	//also have to increment number of resets to keep track of them

	TCNT2 = 1; //OC2A is only set at TCNT2 = 0x00 so by making TCNT2 = 1
				//we keep OC2A at low value

	num_resets += 1;

	if((num_resets*255) >= (period_counts_128ps_clk - OCR2A)){
		//num_resets*255 = num_resets*(256 - 1)
		//if we set TCNT2 = 12 above, we would have num_resets*(256 - 1)
		num_resets = 0;

		TIMSK2 &= ~(1<<TOIE2);	//disable future overflow interrupts
		//This means that in fast-pwm mode with 0xFF as top, TCNT2 will overflow
		//and we will set OC2A = high (and then OC2A = low at TCNT2 = OCR2A)

		TCNT2 = 0xFF; //the next timer increment, we will overflow and set OC2B
	}

}

ISR(TIMER2_COMPA_vect){

	
	TIMSK2 |= (1<<TOIE2); //enable overflow ISR
	//because we are in non-inverted fast-pwm mode
	//OC2A will be cleared upon this compare event.
	
	//We want to enable the overflow ISR so we can count out 
	//the rest of the pwm cycle (20ms = 2304 counts @ 128 prescale)


}





// int delay(unsigned int delay_in_ms){
// 	int timer_freeze = TCNT2;
// 	int i = 0;
// 	for(i = 0; i<= delay_in_ms; i++){
// 		TCNT2 = 1; //clear timer (value is saved in timer_freeze)
// 		//we have to set it to 1 instead of 0 
// 		//setting to TCNT2 = 0 would put OC2B -> HIGH due to fast-pwm.
// 
// 		//The other way is to clear all of TCCR2A and TCCR2B 
// 		//at the beginning of this function and then set their 
// 		//configs back to fast-pwm mode at the end.
// 		
// 		//It is simpler just to set TCNT2 = 1 and increment OCR2B as well. 
// 
// 		TCCR2B |= (1<<CS20)|(1<<CS22); //128 prescaler
// 		//timer is actually already running at 128 prescaler but set it anyway.
// 		//at 128 prescaler, 1ms is 115.2 counts ~ 115 counts as an integer
// 
// 		OCR2B = (int)(fclk_128prescaler*1e-3 + 1); //this is 115 counts
// 		TIFR2 = (1<<OCF2B); //clear flag
// 		while(!(TIFR2 &(1<<OCF2B))){} //wait until flag is set
// 	}
// 	//the FOR-loop makes us wait 1 ms * delay_in_ms until we return timer_freeze
// 	//(which was the original value of TCNT2)
// 	
// 	return timer_freeze;
// }


void delay(int delay)
{
	int i=0;
	for(i=0;i<delay;i++){
		TCCR0A=0; 
		TCCR0B = (1<<CS00) | (1<<CS02); //1024 prescaler @ timer0
		TCNT0=0;
		OCR0A = (int)(fclk_1024prescaler*1e-3 -1);
		TIFR1 = (1<<OCF1A); //clear flag 
		while(!(TIFR1 &(1<<OCF1A))); //wait until flag is set
	}
	TCCR0B =0; //turn off timer0 clock
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
	OCR2A = (int)(fclk_128prescaler*1e-3); //1ms pulse is leftmost position

	DDRD |= (1<<PD7);   //PWM Pins as Out (OC2A)

	const unsigned short one_ms_pulse = (int)(fclk_128prescaler*1e-3);
	//leftmost position (0 degrees)
	
	const unsigned short two_ms_pulse = (int)(fclk_128prescaler*2e-3);
	//rightmost position (180 degrees)

	unsigned short one_degree_pulse = (int)(one_ms_pulse/180); 
	//1ms pulse puts motor to 0deg position. 2ms pulse puts motor to 180deg
	//so a 1ms_pulse/180 degrees is the time increment

	const unsigned short delta_theta = 15;


	while(1){

		for(int x = one_ms_pulse; x <= two_ms_pulse; x += (one_degree_pulse*delta_theta)){

			OCR2A = x;
			

			//TCNT2 = delay(100);

			delay(50);
		}
		
		OCR2A = one_ms_pulse; //reset back to leftmost position		
	}
}


