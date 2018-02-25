// program to change brightness of an LED
// demonstration of PWM

#include <avr/io.h>
#include <util/delay.h>

// initialize PWM
void pwm_init()
{
	// initialize timer0 in PWM mode
	TCCR0A |= (1<<WGM00)|(1<<COM0A1)|(1<<COM0B1)|(1<<WGM01);
	TCCR0B |=(1<<CS00)|(1<<CS02);
	
	// make sure to make OC0 pin (pin PB3 for atmega32) as output pin
	DDRB |= (1<<PB3);
	ICR1 = 287;
}

int main(void)
{
	uint8_t zero_deg_pulse =7;
	uint8_t ninety_deg_pulse =19;
	uint8_t fifteenDegree =2;
	
	// initialize timer0 in PWM mode
	pwm_init();
	// run forever
	while(1)
	{
		// increasing brightness
		for (int x = zero_deg_pulse; x <= ninety_deg_pulse; x += fifteenDegree)
		{
			// set the brightness as duty cycle
			OCR0A = x;
			
			
			// delay so as to make the user "see" the change in brightness
			_delay_ms(1000);
		}
		
		 
		for ( int x = ninety_deg_pulse; x > zero_deg_pulse; x -= fifteenDegree)
		{
			// set the brightness as duty cycle
			OCR0A =x;
			_delay_ms(100);
			
			// delay so as to make the user "see" the change in brightness
			_delay_ms(100);
		}
		
		// repeat this forever
	}
}