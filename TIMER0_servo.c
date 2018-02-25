#include <avr/io.h>
#include <util/delay.h>
const float fclk = 14.7546e6/1024;
const float fservo = 60.0;
const float delta_t = 1.0e-3;
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
	uint8_t zero_deg_pulse =(int)(fclk*0.49e-3);
	uint8_t ninety_deg_pulse =(int)(fclk*1.325e-3);
	uint8_t fifteenDegree =(int)(fclk*0.139e-3);
	
	// initialize timer0 in PWM mode
	pwm_init();
	OCR0A = (int)(fclk/fservo);
	while(1)
	{
		
		for (int x = zero_deg_pulse; x < ninety_deg_pulse; x += fifteenDegree)
		{
			OCR0A = x;
			_delay_ms(1000);
			OCR0A =(int)(fclk/fservo); //top value 
		}
		 
		for ( int x = ninety_deg_pulse; x > zero_deg_pulse; x -= fifteenDegree)
		{
			OCR0A =x;
			_delay_ms(1000);
			OCR0A =(int)(fclk/fservo); //top value is reset
		}
		
	}
}
