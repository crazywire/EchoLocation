#include <avr/io.h>
#include <util/delay.h>

// initialize PWM
void pwm_init()
{
	// initialize timer0 in PWM mode
	TCCR0A |= (1<<WGM00)|(1<<COM0A1)|(1<<COM0B1)|(1<<WGM01);
	TCCR0B |=(1<<CS00)|(1<<CS02);
	
	// make sure to make OC0A pin (pin PB3)
	DDRB |= (1<<PB3);
}

int main(void)
{
	uint8_t zero_deg_pulse =7;
	uint8_t ninety_deg_pulse =19;
	uint8_t fifteenDegree =2;
	
	// initialize timer0 in PWM mode
	pwm_init();
	
	while(1)
	{
		
		for (int x = zero_deg_pulse; x <= ninety_deg_pulse; x += fifteenDegree)
		{
			OCR0A = x;
		
			_delay_ms(1000);
		}
		
		 
		for ( int x = ninety_deg_pulse; x > zero_deg_pulse; x -= fifteenDegree)
		{
			OCR0A =x;
			_delay_ms(1000);
		}
		
	}
}
